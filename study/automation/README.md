# Vulkan 학습 계획 / Calendar 연동

실제 학습 진도는 `../PLAN.md`가 관리한다. Codex가 소스·실행 증거를 읽고 한 과제를 선정하고,
Python 보조 도구가 가용 시간, 중복 식별자, 미완료 재계획, Calendar 호출 인수를 계산한다.
렌더러 구현/빌드와 별개이며 웹사이트·서버·추가 Python 패키지는 필요 없다.

## 현재 연결과 쓰기 정책

Codex Google Calendar MCP의 조회, 개인 일정 생성/수정과 재조회가 성공했다.
이 연결을 우선 사용하므로 OAuth Desktop 앱을 새로 만들거나 토큰을 저장할 필요가 없다.
연결 인증/갱신은 연결 서비스가 관리한다. 만료/권한 오류는 연결 설정에서 재연결한다.
현재 연결이 실패했다는 이유만으로 로컬 API/OAuth 구현을 추가하지 않는다.

`config.json`의 기본 캘린더는 `primary`다. `calendar_id`와 `conflict_calendar_ids`를 함께 바꾸면
다른 쓰기 대상 및 충돌 확인 캘린더를 지정할 수 있다. 캘린더 ID만 저장하며 자격증명은 저장하지 않는다.

자동 학습 이벤트는 실제 운영 모드다. 사용자가 2026-10-14 기존 ChatGPT 예약 등록 중지와 Codex 단독 담당 전환을 직접 확인/승인했다. 다음 세 조건을 충족한 상태다.

- `calendar_writes_enabled=true`
- `writer_owner="codex"`
- `chatgpt_schedule_verified_disabled=true`

ChatGPT 예약 중지는 사용자의 확인을 근거로 하며 그 서비스 설정을 직접 조회/변경했다고 주장하지 않는다.
이 세 값을 자동화가 변경하면 안 된다. 사용자가 승인한 실제 오늘/내일 일정 5개를 생성/재조회했다.
같은 계획을 다시 실행해 동일 ID의 내용만 갱신했고 고유 식별자별 이벤트 1개와 개인 일정 보존을 재조회했다.
쓰기를 다시 중지하려면 사용자가 `calendar_writes_enabled=false`로 변경하도록 요청할 수 있다.

## 예약 작업의 실행 절차

1. `AGENTS.md`, `CODEX_AUTOMATION_PROMPT.txt`, `study/PLAN.md`를 읽는다.
2. Git과 관련 소스를 읽고, 실제 완료 증거를 확인한다. 새 빌드/실행으로 파일을 바꾸지 않는다.
3. Calendar에서 다음 학습일 전체 구간을 조회한다. 각 캘린더의 pagination을 끝까지 읽는다.
4. 아래 형태의 이벤트 입력을 `runtime/calendar-input.json`에 기록한다. 개인 일정은 시간/상태만 보관해도 된다.
5. 기존 미완료 과제를 줄이거나, 검증 완료 후 다음 과제 하나를 PLAN에 반영한다.
6. `planner.py draft --events ... --update-plan`을 실행한다. 별도 날짜가 없으면 다음 평일을 정하며 설정의 외출 날짜를 건너뛴다.
7. dry-run이면 초안과 결과만 저장한다. 쓰기 모드일 때만 아래 독점/재조회/receipt 절차를 수행한다.

```json
{
  "complete": true,
  "queried_calendar_ids": ["primary"],
  "fetched_at": "2026-10-14T19:00:00+09:00",
  "time_min": "2026-10-15T00:00:00+09:00",
  "time_max": "2026-10-16T00:00:00+09:00",
  "events": [
    {"calendar_id": "primary", "id": "opaque-id", "summary": "개인 일정",
     "start": "2026-10-15T12:00:00+09:00", "end": "2026-10-15T13:00:00+09:00"}
  ]
}
```

예시의 `queried_calendar_ids`는 설정의 모든 충돌 확인 캘린더를 실제 조회한 ID 목록으로 바꾼다.
실제 조회 실패/누락/미완료 페이지이면 `complete=false`로 기록한다. 오래되거나 불완전한 조회로 Calendar에 쓰지 않는다.
Google API의 `{dateTime: ...}`/`{date: ...}` 경계와 연결 도구의 문자열 경계를 모두 받는다.
종일 일정의 종료일은 다음 날짜로 취급한다. 취소/거절/transparent 일정은 가용 시간을 막지 않는다.

```powershell
python -B study/automation/planner.py snapshot
python -B study/automation/planner.py draft --events study/automation/runtime/calendar-input.json --update-plan
python -B -m unittest discover -s study/automation -p test_planner.py -v
```

기본 Python 실행 경로를 찾지 못하면 Codex의 `load_workspace_dependencies`로 찾은 Python을 사용한다.
소스 분석과 과제 선정은 Codex가 한다. Python은 Git 통계만으로 완료/이해도를 추측하지 않는다.

## 중복·시간 보존·등록 확인

- `YYYY-MM-DD|kind`에 캘린더 ID를 결합한 고유 key와 설명의 독립된 줄 `[DaylightEngine-study:v1:날짜:kind]`로 소유권을 식별한다. kind는 vulkan/linear_algebra/blender다. 설명 맨 위에는 짧은 목표/첫 행동/완료 기준을 둔다.
- 직접 Calendar API용 이벤트 본문에는 `extendedProperties.private`를 포함한다.
  현재 MCP 생성/수정 schema는 이 필드를 받지 않으므로 MCP에서는 설명 식별자 + 로컬 ID 기록을 사용한다.
- 제목만 비슷한 개인 일정/ChatGPT 학습 일정은 변경하지 않는다. 다른 담당자의 학습 일정이 있으면 추가도 중지한다.
- 기존 소유 이벤트면 내용만 갱신하고 시간 필드를 보내지 않는다. 사용자가 옮긴 시간과 설명 끝 이후 메모를 보존한다.
- `runtime/registry.json`에 있는 ID가 날짜 검색에서 안 보이면 `read_event`로 직접 조회한다. 이동된 이벤트를 다시 만들지 않는다.
- 둘 이상 소유 이벤트가 있거나 개인 일정과 충돌하면 자동 변경을 멈추고 PLAN에 재검토 사유를 기록한다.
- 쓰기 전 `planner.py lock`으로 독점 토큰을 얻고, Calendar를 다시 조회해 초안을 재생성한다. 같은 로컬 작성자의 동시 생성 방지용이다.
- `ready_for_connector`일 때만 JSON의 `connector_tool` 및 `connector_args`로 실제 MCP를 호출한다.
  생성은 Google Meet/참석자/알림을 추가하지 않는다. dry-run JSON 생성은 Calendar 성공이 아니다.
- 생성/수정 후 `read_event` 결과 전체를 `runtime/read-receipt.json`에 저장한다.
  `planner.py receipt --draft runtime/날짜-draft.json --receipt runtime/read-receipt.json`으로
  성공/ID/소유권/제목/시간을 검증하고 registry를 저장한다.
- 종료 시 `planner.py unlock --token 받은토큰`을 실행한다. 오류 후 생성 여부가 불명확하면 소유 식별자 검색/ID 조회부터 하며 즉시 재생성하지 않는다.
- crash로 lock이 남으면 실행 중인 작성자가 없는지 확인한 뒤 해당 토큰으로 수동 해제한다. 자동으로 오래된 lock을 무시하지 않는다.

연결 도구는 클라이언트 지정 이벤트 ID/private property 검색을 제공하지 않으므로 서버 차원의 원자적 upsert는 제공하지 않는다.
단일 담당자·로컬 lock·재조회로 중복을 방지한다. 실제 오늘/내일 이벤트 5개의 동일 ID 갱신·중복 없음과 26개 단위 테스트를 확인했다.

## Windows / 실패 시 동작

Codex Scheduled Tasks를 메인 로컬 프로젝트 모드로 사용한다. 매일 19:00 Asia/Seoul에 다음 가능한 학습일을 준비하고 실제 Calendar 등록/수정 및 재조회까지 수행한다.
새 worktree를 만들지 않는다. 코드가 과제를 고르는 Windows 작업 스케줄러를 별도로 만들지 않는다.
Scheduled Tasks가 생성/조회되면 Windows 대체 작업은 필요 없다. 앱/PC 종료나 절전 때의 놓친 실행을 자동 보충한다고 가정하지 않는다.
복귀 후 Scheduled에서 Run now로 재검토한다. 인터넷/인증 실패는 오프라인 초안으로 표시한다.
토큰 갱신은 연결 서비스가 담당하며 읽기 실패 시 재연결한다. 오류 로그에는 시간/오류 유형만 저장한다.

자동 실행에서 수정 가능한 파일은 PLAN과 runtime 기록뿐이다. 코드/설정/프롬프트의 유지보수는 사용자 요청 시에만 한다.
runtime, 가상환경, pycache, 일반 OAuth 파일 이름은 이 디렉터리의 `.gitignore`로 제외한다.
나중에 API fallback을 추가하더라도 토큰/클라이언트 비밀은 저장소 밖 `%LOCALAPPDATA%`에 두고 최소 scope를 따로 검토한다.

자료: [Google Calendar Extended properties](https://developers.google.com/workspace/calendar/api/guides/extended-properties),
[Codex/ChatGPT Scheduled Tasks](https://learn.chatgpt.com/docs/automations?surface=app).
