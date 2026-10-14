"""Local, standard-library planner. Calendar I/O uses Codex's connected tools.

This module never sends network requests or changes renderer/build files.
PLAN.md owns learning state; runtime/ owns drafts, receipts and safe logs.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
import uuid
from datetime import date, datetime, time, timedelta, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
RUNTIME = HERE / "runtime"
KST = timezone(timedelta(hours=9), "Asia/Seoul")
STATE_START = "<!-- study-state:start -->"
STATE_END = "<!-- study-state:end -->"
DRAFT_START = "<!-- calendar-draft:start -->"
DRAFT_END = "<!-- calendar-draft:end -->"
END_DESCRIPTION = "--- 자동 계획 끝 ---"


def read_json(path):
    return json.loads(Path(path).read_text(encoding="utf-8-sig"))


def write_json(path, value):
    path = Path(path).resolve()
    if not path.is_relative_to(RUNTIME.resolve()):
        raise ValueError("Output must be inside study/automation/runtime")
    path.parent.mkdir(parents=True, exist_ok=True)
    temp = path.with_name(path.name + "." + uuid.uuid4().hex + ".tmp")
    temp.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    os.replace(temp, path)


def load_state(text):
    body = text.split(STATE_START, 1)[1].split(STATE_END, 1)[0]
    match = re.search(r"```json\s*(.*?)\s*```", body, re.S)
    if not match:
        raise ValueError("Missing learning state in PLAN.md")
    return json.loads(match.group(1))


def next_study_day(today, config):
    candidate = today + timedelta(days=1)
    for _ in range(366):
        if (candidate.weekday() in config["study_weekdays"]
                and candidate.isoformat() not in config["unavailable_dates"]):
            return candidate
        candidate += timedelta(days=1)
    raise ValueError("No study day available")


def parse_time(value):
    if isinstance(value, dict):
        value = value.get("dateTime", value.get("date"))
    if not value:
        raise ValueError("Missing calendar boundary")
    parsed = datetime.fromisoformat(value.replace("Z", "+00:00"))
    return parsed.replace(tzinfo=KST) if parsed.tzinfo is None else parsed.astimezone(KST)


def active_event(event):
    return (event.get("status") != "cancelled"
            and event.get("transparency") != "transparent"
            and event.get("my_response_status") != "declined")


def overlaps(start, end, other_start, other_end):
    return start < other_end and other_start < end


def select_task(state, proposal=None):
    current = state["current_task"]
    # A new proposal cannot silently displace an unfinished task.
    if current["status"] != "verified_complete":
        if current.get("attempts", 0) > 0 and current.get("reduced"):
            selected = dict(current["reduced"])
            selected["id"] = current["id"]
            selected["replanned_from"] = current["title"]
            return selected
        return current
    if not current.get("completion_evidence"):
        raise ValueError("Completion requires evidence")
    if proposal is None:
        raise ValueError("Review must select one new task after verified completion")
    return proposal


def validate_task(task, kind='vulkan'):
    for key in ("id", "title", "goal", "first_20_minutes", "done", "sources",
                "question", "next", "duration_minutes", "minimum_minutes"):
        if not task.get(key):
            raise ValueError("Incomplete task: " + key)
    minimum, maximum = (60, 180) if kind == 'vulkan' else (20, 60)
    if not minimum <= task["duration_minutes"] <= maximum:
        raise ValueError("Core task must fit 1-3 hours")
    if not 20 <= task["minimum_minutes"] <= task["duration_minutes"]:
        raise ValueError("Invalid reduced duration")
    if not all(source["url"].startswith("https://") for source in task["sources"]):
        raise ValueError("Sources must have concrete HTTPS URLs")


def marker(day, kind='vulkan'):
    return f"[DaylightEngine-study:v1:{day.isoformat()}:{kind}]"


def description(task, day, duration, kind='vulkan'):
    brief = task.get('calendar_brief', {})
    sections = ["오늘의 목표: " + brief.get('goal', task['goal']),
                "첫 20분 행동: " + brief.get('first_step', ' / '.join(task['first_20_minutes'])),
                "완료 기준: " + brief.get('done', ' / '.join(task['done'])),
                "학습 자료\n" + "\n".join(x["title"] + "\n" + x["url"] for x in task["sources"]),
                "이해 확인 질문\n" + task["question"], "다음 단계\n" + task["next"]]
    if duration < task["duration_minutes"]:
        sections.append("시간 축소\n" + task["short_session_goal"])
    sections.extend([marker(day, kind), END_DESCRIPTION])
    return "\n\n".join(sections)


def build_draft(day, config, state, snapshot, registry=None, proposal=None, now=None, not_before=None, kind='vulkan'):
    now = now or datetime.now(KST)
    registry = registry or {}
    task = select_task(state, proposal)
    validate_task(task, kind)
    key = config["calendar_id"] + "|" + day.isoformat() + "|" + kind
    result = {"date": day.isoformat(), "key": key, "kind": kind, "task_id": task["id"],
              "mode": "dry_run", "calendar_write_allowed": False,
              "action": "defer", "warnings": [], "generated_at": now.isoformat()}
    if day.weekday() not in config["study_weekdays"] or day.isoformat() in config["unavailable_dates"]:
        result["reason"] = "휴식/개인 일정: " + config["unavailable_dates"].get(day.isoformat(), "주말")
        return result
    window_start = datetime.combine(day, time.fromisoformat(config["work_start"]), KST)
    window_end = datetime.combine(day, time.fromisoformat(config["work_end"]), KST)
    if not_before:
        boundary = parse_time(not_before)
        if boundary.date() != day:
            raise ValueError('Earliest start must be on the planned date')
        window_start = max(window_start, boundary)
    if day == now.date():
        window_start = max(window_start, now.replace(second=0, microsecond=0) + timedelta(minutes=1))
    elif day < now.date():
        result["reason"] = "과거 일정은 만들지 않음"
        return result
    queried = set(snapshot.get("queried_calendar_ids", []))
    complete = (snapshot.get("complete") is True
                and set(config["conflict_calendar_ids"]).issubset(queried))
    if complete:
        complete = (parse_time(snapshot["time_min"]) <= window_start
                    and parse_time(snapshot["time_max"]) >= window_end)
    if not complete:
        result["warnings"].append("Calendar 조회 불완전: 기본 학습 시간으로 만든 오프라인 초안. 쓰기 금지.")
    events = [e for e in snapshot.get("events", []) if active_event(e)]
    target_events = [e for e in events if e.get("calendar_id", "primary") == config["calendar_id"]]
    saved = registry.get(key)
    matches = [e for e in target_events if marker(day, kind) in e.get("description", "").splitlines()
               or (saved and e.get("id") == saved["event_id"])]
    if len(matches) > 1:
        result.update(action="needs_review", reason="같은 고유 식별자에 여러 이벤트: 자동 변경 중지")
        return result
    if saved and not matches:
        result.update(action="needs_fetch", event_id=saved["event_id"], reason="이동된 기존 이벤트를 ID로 먼저 조회")
        return result
    existing = matches[0] if matches else None
    conflict_start, conflict_end = window_start, window_end
    if existing:
        conflict_start, conflict_end = parse_time(existing['start']), parse_time(existing['end'])
        if complete:
            complete = (parse_time(snapshot['time_min']) <= conflict_start
                        and parse_time(snapshot['time_max']) >= conflict_end)
    busy = []
    for event in events:
        if event is existing:
            continue
        start, end = parse_time(event["start"]), parse_time(event["end"])
        if overlaps(conflict_start, conflict_end, start, end):
            busy.append((start, end))
            prefixes = {'vulkan': ('[Vulkan 학습]', '[Vulkan]'), 'linear_algebra': ('[선형대수]',), 'blender': ('[Blender]',)}
            if event in target_events and event.get("summary", "").startswith(prefixes[kind]):
                result.update(action="needs_review", reason="다른 담당자가 만든 Vulkan 학습 일정 존재: 변경/추가 중지")
                return result
    if existing:
        start, end = parse_time(existing["start"]), parse_time(existing["end"])
        if any(overlaps(start, end, a, b) for a, b in busy):
            result.update(action="needs_review", reason="기존 학습 일정이 개인 일정과 겹침: 이동 시간 보존, 자동 수정 중지")
            return result
        duration = int((end - start).total_seconds() / 60)
        # Preserve ALL existing times, including moves outside the planned day.
        result.update(action="update", event_id=existing["id"], preserve_existing_time=True)
    else:
        gaps = []
        cursor = window_start
        for start_busy, end_busy in sorted(busy):
            start_busy, end_busy = max(start_busy, window_start), min(end_busy, window_end)
            if cursor < start_busy:
                gaps.append((cursor, start_busy))
            cursor = max(cursor, end_busy)
        if cursor < window_end:
            gaps.append((cursor, window_end))
        preferred = task["duration_minutes"]
        slot = next(((a, b) for a, b in gaps if (b - a).total_seconds() >= preferred * 60), None)
        duration = preferred
        if slot is None and gaps:
            slot = max(gaps, key=lambda x: x[1] - x[0])
            duration = min(preferred, int((slot[1] - slot[0]).total_seconds() / 60))
        if slot is None or duration < task["minimum_minutes"]:
            result["reason"] = "가용 시간이 부족함: 과제 추가 없이 다음 학습일에 재평가"
            return result
        if duration < preferred and not task.get("short_session_goal"):
            result["reason"] = "짧은 시간에 맞는 완료 기준을 먼저 정해야 함"
            return result
        start, end = slot[0], slot[0] + timedelta(minutes=duration)
        result["action"] = "create"
    if duration < task['duration_minutes']:
        reduced = state['current_task'].get('reduced') if task['id'] == state['current_task']['id'] else None
        if not reduced or duration < reduced['duration_minutes']:
            result.update(action='needs_review', reason='보존된 시간/짧은 슬롯에 맞는 작은 과제를 먼저 선정해야 함')
            return result
        task = dict(reduced, id=task['id'])
        validate_task(task, kind)
        result['scope_reduced'] = True
    desc = description(task, day, duration, kind)
    if existing and END_DESCRIPTION in existing.get("description", ""):
        desc += existing["description"].split(END_DESCRIPTION, 1)[1]
    prefix = {'vulkan': '[Vulkan]', 'linear_algebra': '[선형대수]', 'blender': '[Blender]'}[kind]
    title = prefix + ' ' + task.get('calendar_title', task['title'])
    result["event"] = {"summary": title, "description": desc,
                       "start": {"dateTime": start.isoformat(), "timeZone": config["timezone"]},
                       "end": {"dateTime": end.isoformat(), "timeZone": config["timezone"]},
                       "extendedProperties": {"private": {"daylightStudy": "v1", "studyKey": key}},
                       "reminders": {"useDefault": False}, "attendees": []}
    args = {"calendar_id": config["calendar_id"], "title": title, "description": desc,
            "add_google_meet": False, "reminders": {"use_default": False},
            "timezone_str": config["timezone"]}
    if existing:
        args["event_id"] = existing["id"]
    else:
        args.update(start_time=start.isoformat(), end_time=end.isoformat(), attendees=[])
    result["connector_tool"] = "update_event" if existing else "create_event"
    result["connector_args"] = args
    fresh = False
    if snapshot.get("fetched_at"):
        age = (now - parse_time(snapshot["fetched_at"])).total_seconds()
        fresh = 0 <= age <= config["snapshot_max_age_minutes"] * 60
    if existing and not complete:
        result['warnings'].append('이동된 이벤트의 시간까지 Calendar 조회 범위를 넓혀야 함. 쓰기 금지.')
    allowed = (config["calendar_writes_enabled"] is True and config["writer_owner"] == "codex"
               and config["chatgpt_schedule_verified_disabled"] is True and complete and fresh)
    result["calendar_write_allowed"] = allowed
    if allowed:
        result["mode"] = "ready_for_connector"
    return result


def build_routines(day, config, snapshot, registry=None, core=None, now=None):
    """Optional routines; never select a second Vulkan development task."""
    if not config.get('plan_routines'):
        return []
    candidates = []
    anchor = config.get('linear_algebra_anchor')
    if anchor and (day - date.fromisoformat(anchor)).days >= 0 and (day - date.fromisoformat(anchor)).days % 2 == 0:
        candidates.append(('linear_algebra', {
            'id': 'linear-algebra-review', 'status': 'pending', 'title': '현재 진도 확인 · 개념 복습',
            'goal': '오늘을 격일 루틴 시작점으로 삼고 마지막 강의/문제 진도를 확인한 뒤 최근 개념 하나를 복습한다.',
            'first_20_minutes': ['최근 강의 위치/문제 번호를 확인한다.', '가장 최근 배운 개념을 예제 하나로 설명한다.'],
            'done': ['최근 학습 위치와 복습한 예제 하나를 기록한다.'],
            'sources': [{'title': '대학 선형대수 참고서 — 현재 배운 주제만 참고', 'url': 'https://textbooks.math.gatech.edu/ila/'}],
            'question': '오늘 복습한 개념을 구체적인 벡터/행렬 예로 설명할 수 있는가?',
            'next': '확인한 강의/문제 순서로 다음 격일 복습을 진행한다.',
            'duration_minutes': 30, 'minimum_minutes': 30
        }, config.get('linear_algebra_start', '14:00'), config.get('linear_algebra_end', '18:00')))
    candidates.append(('blender', {
        'id': 'blender-challenge', 'status': 'pending', 'title': '30일 챌린지 · 현재 단계 연습',
        'goal': '진행 중인 Blender 챌린지를 가볍게 30분 연습한다. 현재 일차는 먼저 확인한다.',
        'first_20_minutes': ['마지막 완료 일차/파일을 확인한다.', '현재 강좌의 다음 작업을 짧게 따라 한다.'],
        'done': ['오늘 작업 파일을 저장하고 마지막 진행 위치를 기록한다.'],
        'sources': [{'title': 'Blender 공식 매뉴얼 — 필요한 조작만 참고', 'url': 'https://docs.blender.org/manual/en/latest/'}],
        'question': '오늘 사용한 조작 하나를 도움 없이 다시 할 수 있는가?',
        'next': '같은 챌린지의 다음 구간을 이어간다.',
        'duration_minutes': config.get('blender_minutes', 30), 'minimum_minutes': 30
    }, config.get('blender_start', '20:00'), config.get('blender_end', '22:00')))
    working = dict(snapshot, events=list(snapshot.get('events', [])))
    def reserve(draft):
        if draft and draft.get('event'):
            event = dict(draft['event'], calendar_id=config['calendar_id'], id='planned-' + draft['kind'])
            working['events'].append(event)
    reserve(core)
    result = []
    for kind, task, start, end in candidates:
        routine_config = dict(config, work_start=start, work_end=end)
        routine = build_draft(day, routine_config, {'current_task': task}, working, registry, now=now, kind=kind)
        result.append(routine)
        reserve(routine)
    return result


def render_draft(draft):
    lines = [DRAFT_START, "## Calendar 등록 초안", "", "- 대상 학습일: " + draft["date"],
             "- 모드: " + draft["mode"] + " · 동작: " + draft["action"]]
    if "event" in draft:
        event = draft["event"]
        lines += ["- 시간: " + event["start"]["dateTime"] + " ~ " + event["end"]["dateTime"],
                  "", event["description"]]
    if draft.get("reason"):
        lines.append("- 사유: " + draft["reason"])
    lines += ["- " + x for x in draft["warnings"]]
    lines.append(DRAFT_END)
    return "\n".join(lines)


def update_plan(path, original, draft):
    path = Path(path)
    # Re-read before replacing so user edits made during analysis are retained.
    if path.read_text(encoding="utf-8-sig") != original:
        raise RuntimeError("PLAN changed during review; re-read and merge")
    section = render_draft(draft)
    if DRAFT_START in original:
        updated = original.split(DRAFT_START, 1)[0] + section + original.split(DRAFT_END, 1)[1]
    else:
        updated = original.rstrip() + "\n\n" + section + "\n"
    path.write_text(updated, encoding="utf-8")


def record_receipt(draft, receipt, registry):
    # Require a successful READ after mutation, not merely a create attempt.
    if not draft.get("calendar_write_allowed") or receipt.get("isError") is not False:
        raise ValueError("Writes disabled or receipt reports failure")
    event = receipt.get("structuredContent", {})
    if not event.get("id") or event.get("status") != "confirmed":
        raise ValueError("Read-back confirmation required")
    if marker(date.fromisoformat(draft["date"]), draft.get('kind', 'vulkan')) not in event.get("description", "").splitlines():
        raise ValueError("Read-back ownership marker missing")
    if draft.get("event_id") and draft["event_id"] != event["id"]:
        raise ValueError("Unexpected event ID")
    for field in ("start", "end"):
        if parse_time(event[field]) != parse_time(draft["event"][field]):
            raise ValueError("Read-back time differs from draft")
    if event.get("summary") != draft["event"]["summary"]:
        raise ValueError("Read-back title differs from draft")
    registry = dict(registry)
    registry[draft["key"]] = {"event_id": event["id"], "start": event["start"], "end": event["end"]}
    return registry


def acquire_lock():
    RUNTIME.mkdir(parents=True, exist_ok=True)
    token = uuid.uuid4().hex
    with (RUNTIME / "writer.lock").open("x", encoding="utf-8") as handle:
        json.dump({"token": token, "pid": os.getpid(), "created_at": datetime.now(KST).isoformat()}, handle)
    return token


def release_lock(token):
    path = RUNTIME / "writer.lock"
    if read_json(path)["token"] != token:
        raise ValueError("Lock token differs")
    path.unlink()


def git_snapshot():
    def run(*args):
        completed = subprocess.run(["git", "-C", str(ROOT), *args], capture_output=True,
                                   text=True, encoding="utf-8", errors="replace", timeout=30)
        if completed.returncode:
            raise RuntimeError("Git read failed")
        return completed.stdout.strip()
    return {"captured_at": datetime.now(KST).isoformat(), "head": run("rev-parse", "HEAD"),
            "status": run("status", "--short"),
            "commits": run("log", "-8", "--format=%h %aI %s"),
            "unstaged": run("diff", "--stat"), "staged": run("diff", "--cached", "--stat"),
            "note": "Git change summaries do not prove rendering success or understanding."}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    draft_parser = commands.add_parser("draft")
    draft_parser.add_argument("--date")
    draft_parser.add_argument("--events", required=True)
    draft_parser.add_argument("--proposal")
    draft_parser.add_argument("--not-before", help="Optional earliest start with timezone, e.g. 2026-10-14T11:30:00+09:00")
    draft_parser.add_argument("--update-plan", action="store_true")
    commands.add_parser("snapshot")
    commands.add_parser("lock")
    unlock = commands.add_parser("unlock")
    unlock.add_argument("--token", required=True)
    receipt_parser = commands.add_parser("receipt")
    receipt_parser.add_argument("--draft", required=True)
    receipt_parser.add_argument("--receipt", required=True)
    args = parser.parse_args(argv)
    config = read_json(HERE / "config.json")
    if config["timezone"] != "Asia/Seoul":
        raise ValueError("This minimal planner currently supports Asia/Seoul")
    if args.command == "lock":
        print(acquire_lock())
        return
    if args.command == "unlock":
        release_lock(args.token)
        return
    if args.command == "snapshot":
        value = git_snapshot()
        write_json(RUNTIME / "git-snapshot.json", value)
        print(json.dumps(value, ensure_ascii=False))
        return
    registry_path = RUNTIME / "registry.json"
    registry = read_json(registry_path) if registry_path.exists() else {}
    if args.command == "receipt":
        registry = record_receipt(read_json(args.draft), read_json(args.receipt), registry)
        write_json(registry_path, registry)
        print("Calendar read-back confirmed; registry updated.")
        return
    path = ROOT / "study" / "PLAN.md"
    original = path.read_text(encoding="utf-8-sig")
    day = date.fromisoformat(args.date) if args.date else next_study_day(datetime.now(KST).date(), config)
    proposal = read_json(args.proposal) if args.proposal else None
    value = build_draft(day, config, load_state(original), read_json(args.events), registry, proposal, not_before=args.not_before)
    routines = build_routines(day, config, read_json(args.events), registry, core=value)
    for routine in routines:
        write_json(RUNTIME / (day.isoformat() + '-' + routine['kind'] + '-draft.json'), routine)
    value['routines'] = [{'kind': x['kind'], 'action': x['action'], 'mode': x['mode'], 'calendar_write_allowed': x['calendar_write_allowed']} for x in routines]
    write_json(RUNTIME / (day.isoformat() + "-draft.json"), value)
    if args.update_plan:
        update_plan(path, original, value)
    write_json(RUNTIME / "last-run.json", {"at": datetime.now(KST).isoformat(), "result": "draft_generated",
               "date": day.isoformat(), "action": value["action"], "calendar_write_allowed": value["calendar_write_allowed"]})
    print(json.dumps(value, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        # Do not echo API responses, credentials, personal events or raw exceptions.
        write_json(RUNTIME / "last-error.json", {"at": datetime.now(KST).isoformat(), "result": "failed",
                   "error_type": type(error).__name__})
        print("Planner failed: " + type(error).__name__ + ". Check inputs; no Calendar success claimed.", file=sys.stderr)
        sys.exit(1)
