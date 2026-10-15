# Vulkan Renderer · 학습 계획

> 갱신: 2026-10-15 (Asia/Seoul). 학습 상태의 단일 기준은 이 파일이다.
> 코드 존재, 빌드 성공, 실행/화면 검증, 이해도 확인을 서로 구분한다.

## 다음 학습일의 단 하나의 핵심 과제 — 2026-10-19 (월)

**TinyGLTF v3로 Lumine 모델을 읽고 mesh / primitive / POSITION accessor 메타데이터를 검증한다.**
예상 2시간. 완료 증거가 없는 기존 과제를 10/19로 이동한다. 10/15 오전 검토이므로 오늘 학습의 실패/지연을 단정하지 않고 attempts=0을 유지한다. GPU 업로드 과제는 추가하지 않는다.

- 목표: 준비된 `Daylight/Asset/Lumine/lumine/scene.gltf`를 실제 프로젝트의 TinyGLTF v3 C API로 읽고 연결 정보를 로그로 확인한다.
- 첫 20분:
  1. 0~5분: `CMakePresets.json`, `cmake/FindTinyGLTF.cmake`, 설치된 `C:/tinygltf-3.0.1/README.md`에서 v3 API를 확인한다.
  2. 5~10분: `tg3_parse_options_init`, `tg3_error_stack_init`, `tg3_parse_file`, 반환 오류와 모델/오류 메모리 해제 흐름을 읽는다.
  3. 10~20분: 프로젝트 파일에서 mesh → primitive → POSITION accessor 연결을 찾고 출력할 필드와 예상값을 적는다.
- 완료 기준:
  - 사용자가 작성한 로딩 경로를 빌드/실행하여 `TG3_OK` 및 오류 처리 결과를 확인한다.
  - meshes=3, primitives=3, accessors=12를 실제 로더 결과로 출력한다.
  - POSITION accessor 0/4/8의 count=28102/2919/1589, type=VEC3, componentType=5126(FLOAT)를 출력한다.
  - primitive의 indices accessor와 componentType=5125(UNSIGNED_INT)를 확인한다.
  - 실행 명령, 날짜, 핵심 출력 또는 실패 원인을 이 파일에 기록한다. JSON 파일을 읽은 것만으로 로더 구현 완료를 처리하지 않는다.
- 학습 자료:
  - [TinyGLTF v3 Quick start — 파일 파싱·오류·해제](https://github.com/syoyo/tinygltf#quick-start)
  - [Khronos glTF Tutorial 5 — Buffers, BufferViews, Accessors](https://github.com/KhronosGroup/glTF-Tutorials/blob/main/gltfTutorial/gltfTutorial_005_BuffersBufferViewsAccessors.md)
- 이해 확인 질문: POSITION의 accessor 번호가 bufferView 번호와 다른 이유는 무엇이며, bufferView와 accessor는 각각 무엇을 설명하는가?
- 다음 단계: 완료가 검증된 뒤에만 첫 primitive의 CPU POSITION/indices 추출을 선정한다.
- 실패/시간 부족 시: 첫 mesh의 primitive 1개만 읽고 POSITION 메타데이터를 출력하는 1시간 과제로 축소한다. 이전 과제 위에 새 과제를 쌓지 않는다.

## 코드와 Git에서 확인한 현재 상태

기준 HEAD: `4238937` (2026-10-14 VulkanTexture → VulkanImage 명칭 및 Image 초기화/오류 처리 정리).
이전 기준 `b7c4d83` 이후 diff를 읽었다. staged/unstaged diff와 미추적 파일은 없으며 학습 파일은 `d0f8fdc`에 커밋되어 있다. Renderer는 Image/Test.jpg와 하드코딩 사각형을 사용한다. Daylight/Source 및 Shaders에서 glTF 파싱 호출은 찾지 못했다. 사용자 소스/빌드 설정을 보존한다.

| 기능 | 구현 상태 | 검증 상태와 근거 |
|---|---|---|
| SDL3 창, Vulkan 장치/스왑체인 | 코드 존재 | 기존 10/12 로그에서 창 생성 및 MX450 선택만 확인 |
| HLSL → SPIR-V, 삼각형/텍스처 사각형 파이프라인 | 코드 존재 | 화면 출력은 커밋/기존 사용자 보고. 이번 검토에서 화면 재검증하지 않음 |
| 정점·인덱스 버퍼, staging 업로드 | 코드 존재 | GPU 복사와 transfer → vertex/index 의존성 있음. 현재 draw는 하드코딩 사각형 |
| Dynamic Rendering, 깊이 attachment | 코드 존재 | `Renderer::Render`의 `vkCmdBeginRendering`/`vkCmdEndRendering`. 3D 깊이 테스트 결과 미검증 |
| Synchronization2, timeline/바이너리 semaphore | 코드 존재 | `BeginFrame`/`EndFrame`, `vkCmdPipelineBarrier2`, `vkQueueSubmit2`, timeline wait 확인. 동기화 validation 결과 미확보 |
| Bindless 기초 | 일부 구현 | 64개 sampled-image 배열, PARTIALLY_BOUND/UPDATE_AFTER_BIND, sampler 별도 세트. 셰이더는 `textures[0]`만 사용. 동적/비균일 인덱싱 미구현·미검증 |
| TinyGLTF v3 라이브러리/모델 준비 | 설정·파일 확인 완료 | C11 정적 라이브러리 연결, FS 매크로, 로컬 v3 헤더 존재. 애플리케이션의 parse 호출은 없음 |
| glTF CPU 파싱/데이터 추출 | 미구현 | Source/Shaders 검색과 Renderer 초기화 경로에서 로더 부재 확인 |
| 정적 glTF 모델 렌더링·카메라·glTF 재질 | 미구현 | 현재 POSITION은 Vector2, 모델 행렬/카메라/모델 재질 경로 없음 |
| PBR / Deferred / Toon | 미구현 | 후속 로드맵 단계 |

**검증의 한계 (10/15 재조회):** Debug exe는 10/14 14:06, 별도 vulkan-image-verify/Daylight.exe는 10/14 14:32 산출물이다. 파일 존재/시각으로 빌드 과정이나 화면/validation 성공을 단정하지 않는다. Debug log.txt는 창/GPU 선택만 기록한다. 10/12 pipeline-check.stdout.log는 창/GPU 2줄, stderr는 비어 있다. image-fallback-check/log.txt의 fallback 경고는 별도 확인용 산출물이며 현재 Image.cpp의 abort 경로와 구분한다.
CTest LastTest.log는 10/15 10:18 시작/종료만 있으며 테스트 통과 증거가 아니다. Daylight/log.txt는 기존 10/7 로그이며 glTF 파싱 성공/메타데이터 출력 증거는 찾지 못했다.
이번 자동화는 빌드/렌더러 실행/validation 테스트를 수행하지 않았다. Git diff 부재로 학습 부재/이해도를 판단하지 않는다.

### 다음 단계의 선행 조건

- **v3 API:** 기존 TinyGLTF v1/v2 C++ 예제의 `LoadASCIIFromFile`을 그대로 적용하지 않는다. 실제 설치 헤더의 v3 선언을 따른다.
- **파일 경로:** 현재 빌드는 Assets/Image만 복사한다. `Paths::GetAssetPath`는 exe 옆 Assets를 사용하므로 Lumine가 자동으로 그곳에 있다고 가정하지 않는다. 오늘은 준비된 모델의 명시적 경로로 검증한다.
- **CPU 추출:** bufferView.byteOffset + accessor.byteOffset, stride, componentType, count, 경계 검사를 먼저 확인한다. sparse/압축 등 지원 밖 형식은 명시적으로 거부한다.
- **GPU 모델:** Vector3 위치, 실제 accessor 타입에 맞는 인덱스, 노드 변환, 고정 MVP와 깊이 테스트가 필요하다. 현재 사각형의 UINT16을 모델에 그대로 쓰지 않는다.
- **3D 화면 검증:** 처음에는 첫 primitive/단색/고정 시점으로 범위를 제한하고, 그 후 전체 모델·카메라·재질로 확대한다.

### 준비된 모델의 비교 기준 — JSON 메타데이터만 직접 확인

| mesh / primitive | POSITION accessor | 정점 수 | indices 수 | 위치 / 인덱스 componentType |
|---|---|---|---|---|
| 0 / 0 | 0 | 28102 | 148536 | 5126 / 5125 |
| 1 / 0 | 4 | 2919 | 9753 | 5126 / 5125 |
| 2 / 0 | 8 | 1589 | 6915 | 5126 / 5125 |

3 meshes, 3 primitives, 12 accessors, buffer 1개, bufferViews 3개, skins/animations 0개.
이는 PowerShell JSON 조회 결과이며 TinyGLTF 실행 결과나 전체 glTF 유효성 검증 결과가 아니다.

## 일정과 주간 목표 — 10/14~10/19

Google Calendar의 기본 캘린더, 두 수업 캘린더, 대한민국 휴일 캘린더를 실제 조회했다.
10/14에 사용자 정정한 **10/16(금) 외출, 10/17(토) 결혼식** 종일 일정을 10/15에도 재조회했다. 이번 실행은 개인 일정을 변경하지 않는다.
10/18(일)은 평일 학습 패턴 밖의 휴식일이다.

- 현재 핵심 목표: 모델 파싱 메타데이터를 실제 로더 로그로 검증한다. 완료 증거가 없어 10/19에도 이 과제 하나만 유지한다. CPU 전체 추출/모델 출력은 대기한다.
- 10/15: 오늘 과제가 미완료이면 같은 과제를 축소/재배치한다. 완료 증거가 있으면 첫 primitive의 CPU 추출 하나만 선정한다.
- 10/16~18: 개발 과제 없음. 미완료 작업은 10/19에 재평가하며 3일 분량을 누적하지 않는다.
- 10/15 오전 실제 실행: 다음 학습일 10/19 전체 날짜를 네 캘린더에서 조회했다. 예약 시각 19:00에 시작했다고 주장하지 않는다. 이후 리뷰는 최신 진도/일정을 재평가한다.
- 10/19: Vulkan 10:00~12:00, Blender 20:00~20:30 실제 등록 및 read_event/receipt/중복 재조회 검증 완료. 선형대수는 anchor 10/14와 5일 차이여서 대상 아님. 10/16·18 격일 분량은 외출/주말로 생략하고 누적하지 않는다. 다음 격일 대상 10/20은 그날 Calendar 확인 후 배치한다.
- 개발 기본 시간: 평일 10:00~20:00 중 1~3시간 블록 하나. 충돌 시 빈 시간으로 옮기거나 작은 검증 단위로 줄인다.
- 선형대수: 사용자 요청으로 10/14를 새 격일 루틴의 시작점으로 설정했다. 현재 강의/문제 진도 확인과 최근 개념 복습을 30분으로 등록한다. 과거 학습일이나 수강 진도를 추측하지 않는다. 외출/주말로 빠진 분량은 누적하지 않는다.
- Blender: 가용 저녁 20:00~22:00 중 30분. 현재 챌린지 일차는 먼저 확인하며 10/14~15에는 20:00~20:30으로 등록했다.
- 블로그: 주 1회 초안 정리/발행 선택. 개발 과제 완료 조건과 분리한다.

## 짧은 복습 노트 — 이해도는 아직 평가하지 않음

- **동기화:** transfer write → vertex/index read 또는 fragment sampled read 의존성을 현재 `ConvertResourceState`와 연결해 설명한다. `vkQueueWaitIdle`과 메모리 배리어가 담당하는 역할을 구분한다.
- **프레임 동기화:** image-acquired/프레젠트 완료 바이너리 semaphore와 CPU 프레임 재사용을 제한하는 timeline semaphore를 구분한다.
- **Bindless:** 배열 64개를 만들고 slot 0을 읽는 단계와 런타임/비균일 인덱싱은 다르다. PARTIALLY_BOUND는 실제 접근한 미초기화 descriptor를 유효하게 만들지 않는다.
- **Pipeline/Shader:** 현재 Vector2/float2와 모델 VEC3의 차이, 정점 stride/attribute format 및 행렬 변환을 5~10줄로 적는다.
- 막힌 개념만 개인 노트로 남긴다. 블로그 발행은 요구하지 않는다.

## 장기 로드맵 — 매일 활성 과제는 하나

| 순서 | 마일스톤 | 검증 가능한 완료 기준 | 상태 |
|---|---|---|---|
| 0 | 기존 이미지 출력 기준선 | 새 Debug 실행 화면, 정상 종료, resize/minimize, validation 결과 기록 | 코드 있음·재검증 필요 |
| 1 | glTF 파싱 | 실제 TinyGLTF 로그가 위 메타데이터와 일치 | 현재 활성 과제 |
| 2 | 첫 primitive CPU 추출 | 위치/인덱스 개수·범위·stride·샘플 값 확인 | 대기 |
| 3 | 정적 glTF 출력 | 3D 위치/적절한 인덱스 업로드, 고정 MVP·깊이로 단색 모델 출력 | 대기 |
| 4 | 카메라/노드/전체 모델 | 좌표계와 노드 변환 검증, orbit 및 전체 primitives 출력 | 대기 |
| 5 | glTF 텍스처·재질 | UV/baseColor·sampler 연결, 여러 재질 인덱스 검증 | 대기 |
| 6 | PBR | metallic/roughness와 normal, 직접광 결과를 기준 모델과 비교 | 대기 |
| 7 | Deferred Rendering | G-Buffer 위치/normal/material 시각화, lighting pass·의존성 검증 | 대기 |
| 8 | Toon Shading | 명암 단계화 후 윤곽선 효과를 각각 비교 검증 | 대기 |

후속 자료: [Khronos glTF Tutorial 9 — Meshes](https://github.com/KhronosGroup/glTF-Tutorials/blob/main/gltfTutorial/gltfTutorial_009_Meshes.md),
[Vulkan Tutorial — Vertex input description](https://docs.vulkan.org/tutorial/latest/04_Vertex_buffers/00_Vertex_input_description.html),
[Vulkan Guide — Synchronization Examples](https://docs.vulkan.org/guide/latest/synchronization_examples.html),
[Vulkan Guide — Descriptor indexing](https://docs.vulkan.org/guide/latest/extensions/VK_EXT_descriptor_indexing.html).
매일 읽을 자료는 활성 과제에 맞는 구체적 구간 1~2개만 고른다.

## 자동화 운영 상태

- Google Calendar: Codex 연결 도구에서 읽기 및 개인 일정 생성/수정/재조회 성공. 로컬 OAuth 파일/토큰 발급은 필요 없음.
- 자동 학습 일정: **실제 운영**, `calendar_writes_enabled=true`, 담당자 `codex`. 사용자가 2026-10-14 기존 ChatGPT Calendar 등록 자동화 비활성화와 Codex 단독 담당을 직접 확인/승인했다.
- ChatGPT 예약 중지는 사용자 확인을 근거로 한다. Codex가 해당 서비스의 예약 설정을 직접 조회/변경했다고 주장하지 않는다.
- 실제 학습 이벤트 5개를 생성하고 read_event 및 receipt로 확인했다. 같은 작업을 다시 실행해 동일 ID 5개를 갱신한 뒤 각각 1개만 존재함을 재조회했다.
- Codex 예약: **실제 갱신 및 조회 확인**, ID `vulkan-daily-study-review`, ACTIVE. **매일 19:00 Asia/Seoul**, 메인 로컬 `C:/DaylightEngine`, worktree 사용 안 함. 모델/추론은 기존 로컬 설정의 gpt-6.1-sol/high. 저녁 리뷰가 다음 학습일의 Calendar 등록·수정·재조회까지 수행한다. 아직 예약 시각에 자동으로 시작하는 것은 검증 전이다.
- 자동 실행 쓰기 범위: `study/PLAN.md`, `study/automation/runtime/`의 초안/결과/오류/이벤트 ID 기록만. 렌더러 소스, 빌드 설정, 자동화 코드/설정, AGENTS.md/프롬프트는 자동 변경 금지.
- PC가 켜져 있고 앱이 실행 중이어야 로컬 예약이 작동한다. 컴퓨터 종료/절전 시 예약 실행은 보장되지 않으므로 복귀 후 수동 Run now로 재검토한다.
- 인터넷/Calendar 접근 실패 시 PLAN에 제약을 기록하고 10~20시 오프라인 초안만 준비한다. 실패한 등록을 성공으로 보고하지 않는다.

### 연동 검증 결과 — 2026-10-14

- 실제 Calendar 조회 및 개인 일정 날짜 정정/재조회: 성공. 외출 10/16, 결혼식 10/17.
- 실제 Calendar 등록: 아래 오늘/내일 표의 5개 이벤트. 짧은 제목과 설명 첫 세 항목에 목표·첫 20분·완료 기준을 넣고, 자료 URL/이해 질문/다음 단계도 포함했다.
- 계획 코드 테스트: **26개 통과**. 시간 충돌/종일 일정, 범위 축소, 중복 시 update 선택, 사용자 이동 시간·메모 보존, 날짜 밖 이동 재조회, 미완료 재계획, 외부 작성자 보호, 단독 쓰기 설정, 실패 receipt, 파일 동시 수정, 독점 lock, 격일 선형대수/Blender 및 routine 중복 검증.
- 10/16 계획 요청: 외출 때문에 defer, 이벤트 생성 인수 없음.
- 라이브 재실행 중복 검증: **성공**. 승인된 실제 학습 이벤트를 같은 ID로 갱신했고, 최종 검색에서 각 고유 식별자에 1개만 존재한다. 기존 10/16 외출·10/17 결혼식의 ID/시간도 보존 확인했다.
- 최초 수동 OAuth 인증: 기존 Codex 연결이 작동하므로 없음. Calendar 쓰기 전환에 필요한 추가 승인은 남아 있지 않다.

### 이전 실행에서 실제 등록한 일정 — 10/14~15 (Asia/Seoul)

| 날짜 | 시간 | 등록 내용 |
|---|---|---|
| 10/14 | 11:30~13:30 | [Vulkan] glTF 모델 읽기 · 정보 출력 |
| 10/14 | 14:00~14:30 | [선형대수] 현재 진도 확인 · 개념 복습 |
| 10/14 | 20:00~20:30 | [Blender] 30일 챌린지 · 현재 단계 연습 |
| 10/15 | 10:00~12:00 | [Vulkan] glTF 모델 읽기 · 정보 출력 |
| 10/15 | 20:00~20:30 | [Blender] 30일 챌린지 · 현재 단계 연습 |

10/14 등록 당시 완료 증거가 없어 10/15 같은 과제를 유지했다. 10/15 오전 검토는 이 기존 이벤트의 시간/내용을 보존하고 다음 평일 10/19를 준비했다.

### 이번 실행의 실제 Calendar 검증 — 2026-10-15

- 다음 학습일 **2026-10-19 (월), Asia/Seoul**. 네 캘린더의 전체 날짜/모든 페이지를 읽고 lock 획득 후 재조회했다.
- **Vulkan 10:00~12:00**: [glTF 모델 읽기 · 메타데이터 검증](https://www.google.com/calendar/event?eid=MXVmZzFsamJjbGU5ZDIycGRvcTc3dG1xZjAgd25zcWhkNTU1NTVAbQ). ID `1ufg1ljbcle9d22pdoq77tmqf0`.
- **Blender 20:00~20:30**: [현재 단계 30분 연습](https://www.google.com/calendar/event?eid=NTFsOHE0ZmZtMTAxNGgxc21pMWpxNzIzZTggd25zcWhkNTU1NTVAbQ). ID `51l8q4ffm1014h1smi1jq723e8`.
- 생성 뒤 read_event에서 confirmed/ID/제목/시간/고유 marker를 확인하고 두 receipt가 planner 검증을 통과해 registry에 저장되었다. 최종 네 캘린더 검색 완료, 새 marker별 정확히 1개이며 충돌 없음. 초안 파일의 update는 다음 실행용 중복 방지 결과이지 추가 수정 호출을 뜻하지 않는다.
- 기존 10/14~15 학습 이벤트 5개 및 10/16 외출·10/17 결혼식의 ID/시간 보존. 개인 이벤트 변경/삭제 없음.
- planner 날짜별 초안: 10/16 외출, 10/17~18 주말은 Vulkan/해당 routine 모두 defer. 10/19 Blender 처리 완료. 선형대수는 격일 비대상으로 초안이 없으며 미등록. 빠진 분량은 누적하지 않는다.
- TinyGLTF 공식 Quick start와 Khronos Tutorial 5를 웹에서 재확인하고 설치된 v3 헤더와 대조했다. 모델 JSON 비교값도 재조회했다. JSON 확인은 로더 실행 완료가 아니며 current_task는 pending, attempts=0, completion_evidence=null이다.
- 상세 결과: `study/automation/runtime/2026-10-15-review-result.json`, 날짜별 receipt와 registry. 빌드/렌더러 실행/자동화 코드·설정 변경은 수행하지 않았다.

## 학습 기록

| 날짜 | 검증된 사실 | 미완료 / 제약 | 다음 학습일 첫 행동 |
|---|---|---|---|
| 2026-10-14 | 소스/Git/미커밋 분석, 모델 JSON 비교값, Calendar 조회 및 개인 일정 날짜 정정 | TinyGLTF 애플리케이션 로더 없음. 렌더링 재검증/이해도 확인 없음 | v3 파싱 API 확인 후 실제 메타데이터 로그 검증 |
| 2026-10-15 | HEAD 4238937 및 diff, 소스/기존 로그, 네 캘린더 조회와 lock 후 재조회 | glTF 호출/성공 로그 없음. pending/attempts=0 유지. 새 빌드/실행 안 함 | 10/19 동일 메타데이터 과제 하나 유지 |

## 자동화가 읽는 현재 과제 상태

미완료 횟수는 사용자의 실행 기록/실패 근거가 있을 때만 늘린다. Git diff 유무로 늘리지 않는다.
`verified_complete`로 바꾸려면 `completion_evidence`에 실행 결과를 남긴다.
아래 JSON은 이 문서의 활성 과제와 같은 내용이며, 별도 상태 파일로 학습 진도를 관리하지 않는다.

<!-- study-state:start -->
```json
{
  "current_task": {
    "id": "gltf-v3-metadata",
    "status": "pending",
    "attempts": 0,
    "completion_evidence": null,
    "title": "TinyGLTF v3 모델 파싱과 Accessor 메타데이터 검증",
    "goal": "준비된 Lumine scene.gltf를 실제 v3 API로 읽고 mesh/primitive/POSITION 연결 정보를 확인한다.",
    "first_20_minutes": [
      "CMake와 로컬 tiny_gltf_v3.h에서 현재 v3 연결과 파싱/오류/해제 API를 확인한다.",
      "scene.gltf에서 mesh → primitive → POSITION accessor 관계와 예상 필드를 찾는다."
    ],
    "done": [
      "실제 프로젝트 로더가 TG3_OK를 반환하며 mesh=3, primitive=3, accessor=12를 출력한다.",
      "POSITION accessor 0/4/8의 count=28102/2919/1589, VEC3/FLOAT와 indices의 UNSIGNED_INT를 확인한다.",
      "실행 명령과 핵심 로그 또는 실패 원인을 PLAN.md에 남긴다."
    ],
    "sources": [
      {
        "title": "TinyGLTF v3 Quick start — 파싱/오류/해제",
        "url": "https://github.com/syoyo/tinygltf#quick-start"
      },
      {
        "title": "Khronos glTF Tutorial 5 — Buffers, BufferViews, Accessors",
        "url": "https://github.com/KhronosGroup/glTF-Tutorials/blob/main/gltfTutorial/gltfTutorial_005_BuffersBufferViewsAccessors.md"
      }
    ],
    "question": "BufferView와 Accessor는 각각 무엇을 설명하며 POSITION accessor 번호와 bufferView 번호가 다른 이유는 무엇인가?",
    "next": "실제 파싱 완료를 검증한 뒤 첫 primitive의 CPU 위치/인덱스 데이터를 추출한다.",
    "duration_minutes": 120,
    "minimum_minutes": 60,
    "short_session_goal": "첫 mesh의 primitive 하나만 파싱하고 POSITION count/type/componentType과 오류 처리를 검증한다.",
    "reduced": {
      "title": "첫 primitive의 POSITION 메타데이터 로딩 검증",
      "goal": "첫 mesh의 primitive 하나로 범위를 줄여 v3 파싱과 POSITION 연결을 검증한다.",
      "first_20_minutes": [
        "이전 실패 로그를 읽고 파싱 경로와 오류 처리만 확인한다.",
        "첫 POSITION accessor 0의 예상 count=28102와 VEC3/FLOAT를 적는다."
      ],
      "done": [
        "TG3_OK 및 첫 POSITION accessor의 count/type/componentType을 실제 로더에서 출력한다.",
        "실행 결과/막힌 원인을 PLAN.md에 기록한다."
      ],
      "sources": [
        {
          "title": "TinyGLTF v3 Quick start",
          "url": "https://github.com/syoyo/tinygltf#quick-start"
        },
        {
          "title": "Khronos glTF Tutorial 5",
          "url": "https://github.com/KhronosGroup/glTF-Tutorials/blob/main/gltfTutorial/gltfTutorial_005_BuffersBufferViewsAccessors.md"
        }
      ],
      "question": "첫 POSITION accessor는 어느 bufferView를 참조하는가?",
      "next": "성공한 범위부터 나머지 primitive 검증 또는 CPU 추출을 재평가한다.",
      "duration_minutes": 60,
      "minimum_minutes": 60,
      "short_session_goal": "이 과제 전체가 최소 검증 단위이다."
    },
    "calendar_title": "glTF 모델 읽기 · 메타데이터 검증",
    "calendar_brief": {
      "goal": "TinyGLTF v3로 Lumine 모델의 mesh/primitive/POSITION 정보를 실제 로더로 확인한다.",
      "first_step": "0~10분: 로컬 v3 파싱·오류·해제 API 확인. 10~20분: mesh → primitive → POSITION 예상값 정리.",
      "done": "TG3_OK, mesh=3·primitive=3·accessor=12, POSITION 0/4/8의 count=28102/2919/1589·VEC3/FLOAT 및 indices UNSIGNED_INT를 출력하고 실행 명령·로그 기록."
    },
    "planned_date": "2026-10-19"
  }
}
```
<!-- study-state:end -->

### 재계획 규칙

1. 새 과제 전 기존 과제의 완료 증거와 막힌 부분을 읽는다.
2. 미완료면 축소하거나 다음 학습일로 이동하며 하루 핵심 과제는 하나만 유지한다.
3. 코드 존재/Git 커밋만으로 실행 검증과 이해를 완료 처리하지 않는다.
4. Calendar 개인 일정은 읽기만 한다. 학습 자동화의 고유 식별자가 없는 이벤트는 수정/삭제하지 않는다.
5. 사용자가 옮긴 학습 이벤트는 시간을 보존한다. 충돌 시 자동 덮어쓰기 대신 재검토한다.
6. 파일 저장 전에 다시 읽어 사용자의 동시 수정을 감지하고, 달라졌으면 병합 후 재검토한다.

<!-- calendar-draft:start -->
## Calendar 등록 초안

- 대상 학습일: 2026-10-19
- 모드: ready_for_connector · 동작: update
- 시간: 2026-10-19T10:00:00+09:00 ~ 2026-10-19T12:00:00+09:00

오늘의 목표: TinyGLTF v3로 Lumine 모델의 mesh/primitive/POSITION 정보를 실제 로더로 확인한다.

첫 20분 행동: 0~10분: 로컬 v3 파싱·오류·해제 API 확인. 10~20분: mesh → primitive → POSITION 예상값 정리.

완료 기준: TG3_OK, mesh=3·primitive=3·accessor=12, POSITION 0/4/8의 count=28102/2919/1589·VEC3/FLOAT 및 indices UNSIGNED_INT를 출력하고 실행 명령·로그 기록.

학습 자료
TinyGLTF v3 Quick start — 파싱/오류/해제
https://github.com/syoyo/tinygltf#quick-start
Khronos glTF Tutorial 5 — Buffers, BufferViews, Accessors
https://github.com/KhronosGroup/glTF-Tutorials/blob/main/gltfTutorial/gltfTutorial_005_BuffersBufferViewsAccessors.md

이해 확인 질문
BufferView와 Accessor는 각각 무엇을 설명하며 POSITION accessor 번호와 bufferView 번호가 다른 이유는 무엇인가?

다음 단계
실제 파싱 완료를 검증한 뒤 첫 primitive의 CPU 위치/인덱스 데이터를 추출한다.

[DaylightEngine-study:v1:2026-10-19:vulkan]

--- 자동 계획 끝 ---


<!-- calendar-draft:end -->
