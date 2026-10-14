import copy
import tempfile
import unittest
from datetime import date, datetime, timedelta
from pathlib import Path
from unittest.mock import patch

import planner as p


class PlannerTests(unittest.TestCase):
    def setUp(self):
        self.config = p.read_json(p.HERE / 'config.json')
        self.config.update(calendar_writes_enabled=False, writer_owner='chatgpt', chatgpt_schedule_verified_disabled=False)
        self.state = p.load_state((p.ROOT / 'study/PLAN.md').read_text(encoding='utf-8-sig'))
        self.day = date(2026, 10, 15)
        self.now = datetime(2026, 10, 14, 19, tzinfo=p.KST)
        self.snapshot = {'complete': True, 'queried_calendar_ids': self.config['conflict_calendar_ids'],
                         'fetched_at': self.now.isoformat(), 'time_min': '2026-10-15T00:00:00+09:00',
                         'time_max': '2026-10-16T00:00:00+09:00', 'events': []}

    def draft(self, **kwargs):
        return p.build_draft(self.day, self.config, self.state, self.snapshot, now=self.now, **kwargs)

    def event(self, start='2026-10-15T10:00:00+09:00', end='2026-10-15T12:00:00+09:00', **kwargs):
        return dict(id='personal', start=start, end=end, calendar_id='primary', **kwargs)

    def managed(self, **kwargs):
        event = self.event(summary='[Vulkan 학습] 기존 과제', description=p.marker(self.day), **kwargs)
        event['id'] = 'managed'
        return event

    def test_default_dry_run_and_sources(self):
        draft = self.draft()
        self.assertEqual(draft['action'], 'create')
        self.assertFalse(draft['calendar_write_allowed'])
        self.assertEqual(draft['event']['start']['dateTime'], '2026-10-15T10:00:00+09:00')
        for section in ('오늘의 목표', '첫 20분 행동', '완료 기준', '학습 자료', '이해 확인 질문', '다음 단계'):
            self.assertIn(section, draft['event']['description'])
        self.assertIn('https://github.com/syoyo/tinygltf#quick-start', draft['event']['description'])
        self.assertEqual(draft['event']['summary'], '[Vulkan] glTF 모델 읽기 · 정보 출력')
        self.assertTrue(draft['event']['description'].startswith('오늘의 목표:'))

    def test_busy_moves_to_free_slot(self):
        self.snapshot['events'] = [self.event()]
        self.assertEqual(self.draft()['event']['start']['dateTime'], '2026-10-15T12:00:00+09:00')

    def test_explicit_earliest_start(self):
        draft = self.draft(not_before='2026-10-15T11:30:00+09:00')
        self.assertEqual(draft['event']['start']['dateTime'], '2026-10-15T11:30:00+09:00')

    def test_overlapping_busy_and_other_calendars(self):
        first = self.event(end='2026-10-15T13:00:00+09:00')
        second = self.event(start='2026-10-15T12:00:00+09:00', end='2026-10-15T14:00:00+09:00')
        second['calendar_id'] = self.config['conflict_calendar_ids'][1]
        self.snapshot['events'] = [second, first]
        self.assertEqual(self.draft()['event']['start']['dateTime'], '2026-10-15T14:00:00+09:00')

    def test_all_day_blocks(self):
        self.snapshot['events'] = [self.event(start='2026-10-15', end='2026-10-16')]
        self.assertEqual(self.draft()['action'], 'defer')

    def test_short_gap_shrinks_scope(self):
        self.snapshot['events'] = [self.event(start='2026-10-15T11:00:00+09:00', end='2026-10-15T20:00:00+09:00')]
        draft = self.draft()
        self.assertEqual(draft['event']['end']['dateTime'], '2026-10-15T11:00:00+09:00')
        self.assertTrue(draft['scope_reduced'])
        self.assertIn('첫 primitive', draft['event']['summary'])
        self.assertNotIn('mesh=3', draft['event']['description'])

    def test_duplicate_run_updates_owned_only(self):
        draft = self.draft()
        event = self.managed()
        event['description'] = draft['event']['description']
        self.snapshot['events'] = [event]
        second = self.draft()
        self.assertEqual(second['action'], 'update')
        self.assertEqual(second['event_id'], 'managed')
        self.assertNotIn('start_time', second['connector_args'])

    def test_manual_move_and_notes_preserved(self):
        event = self.managed(start='2026-10-15T15:00:00+09:00', end='2026-10-15T17:00:00+09:00')
        event['description'] += '\n' + p.END_DESCRIPTION + '\n내가 남긴 메모'
        self.snapshot['events'] = [event]
        draft = self.draft()
        self.assertEqual(draft['event']['start']['dateTime'], event['start'])
        self.assertIn('내가 남긴 메모', draft['event']['description'])

    def test_missing_moved_event_requires_fetch(self):
        key = self.draft()['key']
        draft = self.draft(registry={key: {'event_id': 'moved'}})
        self.assertEqual(draft['action'], 'needs_fetch')

    def test_move_outside_query_window_requires_new_read(self):
        self.config.update(calendar_writes_enabled=True, writer_owner='codex', chatgpt_schedule_verified_disabled=True)
        self.snapshot['events'] = [self.managed(start='2026-10-19T15:00:00+09:00', end='2026-10-19T17:00:00+09:00')]
        draft = self.draft()
        self.assertEqual(draft['action'], 'update')
        self.assertFalse(draft['calendar_write_allowed'])

    def test_move_outside_date_checks_conflicts_at_moved_time(self):
        self.snapshot['time_max'] = '2026-10-20T00:00:00+09:00'
        self.snapshot['events'] = [self.managed(start='2026-10-19T15:00:00+09:00', end='2026-10-19T17:00:00+09:00'),
                                   self.event(start='2026-10-19T16:00:00+09:00', end='2026-10-19T18:00:00+09:00')]
        self.assertEqual(self.draft()['action'], 'needs_review')

    def test_manually_moved_conflict_is_not_overwritten(self):
        self.snapshot['events'] = [self.managed(), self.event()]
        self.assertEqual(self.draft()['action'], 'needs_review')

    def test_external_writer_event_not_modified(self):
        self.snapshot['events'] = [self.event(summary='[Vulkan 학습] ChatGPT 일정')]
        draft = self.draft()
        self.assertEqual(draft['action'], 'needs_review')
        self.assertNotIn('connector_args', draft)

    def test_multiple_owned_events_stop(self):
        self.snapshot['events'] = [self.managed(), self.managed()]
        self.assertEqual(self.draft()['action'], 'needs_review')

    def test_incomplete_is_replanned_without_adding_task(self):
        self.state['current_task']['attempts'] = 1
        draft = self.draft(proposal={'title': '새 과제'})
        self.assertEqual(draft['task_id'], 'gltf-v3-metadata')
        self.assertIn('첫 primitive', draft['event']['summary'])

    def test_completion_needs_evidence(self):
        self.state['current_task']['status'] = 'verified_complete'
        with self.assertRaises(ValueError):
            self.draft()

    def test_friday_absence_skips_to_monday(self):
        self.assertEqual(p.next_study_day(self.day, self.config), date(2026, 10, 19))
        value = p.build_draft(date(2026, 10, 16), self.config, self.state, self.snapshot, now=self.now)
        self.assertEqual(value['action'], 'defer')

    def test_writes_need_all_three_gates_and_fresh_complete_reads(self):
        self.config['calendar_writes_enabled'] = True
        self.config['writer_owner'] = 'codex'
        self.assertFalse(self.draft()['calendar_write_allowed'])
        self.config['chatgpt_schedule_verified_disabled'] = True
        self.assertTrue(self.draft()['calendar_write_allowed'])
        self.snapshot['complete'] = False
        self.assertFalse(self.draft()['calendar_write_allowed'])
        self.snapshot['complete'] = True
        self.snapshot['fetched_at'] = (self.now - timedelta(hours=1)).isoformat()
        self.assertFalse(self.draft()['calendar_write_allowed'])

    def test_failed_receipt_never_records_success(self):
        with self.assertRaises(ValueError):
            p.record_receipt(self.draft(), {'isError': True}, {})

    def test_confirmed_receipt_stores_stable_id(self):
        self.config.update(calendar_writes_enabled=True, writer_owner='codex', chatgpt_schedule_verified_disabled=True)
        draft = self.draft()
        event = dict(draft['event'], id='stableid', status='confirmed')
        saved = p.record_receipt(draft, {'isError': False, 'structuredContent': event}, {})
        self.assertEqual(saved[draft['key']]['event_id'], 'stableid')

    def test_plan_concurrent_edit_is_preserved(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'PLAN.md'
            path.write_text('사용자 새 수정', encoding='utf-8')
            with self.assertRaises(RuntimeError):
                p.update_plan(path, '오래된 내용', self.draft())
            self.assertEqual(path.read_text(encoding='utf-8'), '사용자 새 수정')

    def test_exclusive_writer_lock(self):
        with tempfile.TemporaryDirectory() as folder, patch.object(p, 'RUNTIME', Path(folder)):
            token = p.acquire_lock()
            with self.assertRaises(FileExistsError):
                p.acquire_lock()
            with self.assertRaises(ValueError):
                p.release_lock('wrong')
            p.release_lock(token)
            self.assertFalse((Path(folder) / 'writer.lock').exists())

    def test_cancelled_and_transparent_events_do_not_block(self):
        self.snapshot['events'] = [self.event(status='cancelled'), self.event(transparency='transparent')]
        self.assertEqual(self.draft()['event']['start']['dateTime'], '2026-10-15T10:00:00+09:00')

    def test_routines_on_odd_day_only_blender(self):
        routines = p.build_routines(self.day, self.config, self.snapshot, now=self.now)
        self.assertEqual([x['kind'] for x in routines], ['blender'])
        self.assertEqual(routines[0]['event']['start']['dateTime'], '2026-10-15T20:00:00+09:00')

    def test_routine_dedup_uses_own_kind(self):
        first = p.build_routines(self.day, self.config, self.snapshot, now=self.now)[0]
        owned = dict(first['event'], id='blender-owned', calendar_id='primary')
        self.snapshot['events'] = [owned]
        second = p.build_routines(self.day, self.config, self.snapshot, now=self.now)[0]
        self.assertEqual(second['action'], 'update')
        self.assertEqual(second['event_id'], 'blender-owned')

    def test_even_day_routines_and_absence(self):
        day = date(2026, 10, 14)
        snapshot = dict(self.snapshot, time_min='2026-10-14T00:00:00+09:00')
        now = datetime(2026, 10, 14, 11, tzinfo=p.KST)
        routines = p.build_routines(day, self.config, snapshot, now=now)
        self.assertEqual([x['kind'] for x in routines], ['linear_algebra', 'blender'])
        skipped = p.build_routines(date(2026, 10, 16), self.config, snapshot, now=now)
        self.assertTrue(all(x['action'] == 'defer' for x in skipped))


if __name__ == '__main__':
    unittest.main()
