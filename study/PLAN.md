# Vulkan Renderer · 학습 계획

> 갱신: 2026-10-21 (Asia/Seoul). 학습 상태의 단일 기준은 이 파일이다.
> 코드 존재, 빌드 성공, 실행/화면 검증, 이해도 확인을 서로 구분한다.

## 오늘의 단 하나의 핵심 과제 — 2026-10-21 (수)

**현재 렌더 루프의 CPU/present FPS 기준선을 측정하고 기록한다.**
예상 30~60분. 컴퓨트 블러는 미구현·대기 상태로 유지하고, 비교 기준이 생기기 전에는 시작하지 않는다.

- 측정 정의: `std::chrono::steady_clock`으로 연속된 렌더 프레임 시작 사이의 `deltaTime`을 초 단위로 측정한다. 창 제목에는 매 프레임 `1 / deltaTime` FPS와 `deltaTime × 1000` 밀리초를 표시한다.
- 구현 위치: 프레임 전체를 소유하는 `Engine`에 `UpdateFrameStats()`와 `ResetFrameStats()`를 둔다. `Renderer`에는 FPS 책임을 넣지 않는다.
- 첫 20분:
  1. 0~5분: 지표 이름을 `CPU/present loop FPS`와 `CPU/present loop ms`로 정한다. GPU 실행 시간이라고 부르지 않는다.
  2. 5~10분: `Engine`에 이전 프레임 시각과 초 단위 `deltaTime`을 멤버로 두고, 성공적으로 획득한 프레임에서 갱신하도록 호출 위치를 표시한다.
  3. 10~20분: 매 프레임 창 제목에 `FPS`, `deltaTime ms`, present mode(`FIFO`)를 출력하고, 최소화·0 크기·스왑체인 재생성으로 건너뛸 때 시간을 reset하는 흐름을 작성한다.
- 측정 조건:
  - 창 1280×720, 리사이즈·최소화·다른 코드 변경 없이 측정한다.
  - 처음 3초는 워밍업으로 버리고 다음 10초 동안 실시간 표시값의 범위를 관찰한다.
  - Debug는 validation 확인용, Release는 성능 기준선용으로 각각 구분한다.
  - 현재 `VK_PRESENT_MODE_FIFO_KHR`를 유지한다. 숫자가 모니터 주사율에 제한될 수 있음을 결과에 함께 기록한다.
- 완료 기준:
  - Debug/Release 빌드가 성공하고 FPS와 deltaTime이 창 제목에 매 프레임 표시된다.
  - 최소화 및 resize 후 비정상적으로 낮은 표본이 누적되지 않는다.
  - Release에서 10초 동안 관찰한 FPS와 CPU/present loop ms 범위를 기록한다.
  - Debug validation error/warning 0을 별도로 확인한다.
  - 이 값은 GPU pass 시간은 아니라는 한계를 기록하고, 블러 전/후 GPU 비용 비교는 timestamp query 후속 과제로 남긴다.
- 이해 확인 질문: FIFO에서 60 FPS가 측정됐다고 해서 GPU가 한 프레임을 16.67 ms 동안 렌더링했다고 결론 내릴 수 없는 이유는 무엇인가?

### 2026-10-21 구현·검증 기록

- `Engine`에 초 단위 `deltaTime`과 `steady_clock` 기반 프레임 시각을 추가했다. 최소화·스왑체인 skip 때 시각을 reset한다.
- 사용자 요청에 따라 창 제목은 매 프레임 `Daylight | 순간 FPS | deltaTime ms`로 갱신하고, 로그는 평균을 별도로 계산하지 않고 1초마다 현재 창 제목 문자열을 그대로 출력한다.
- Debug 직접 실행에서 FPS 로그 시각 `13:05:37.743`, `38.744`, `39.745` 등 약 1.001초 간격을 확인했다. 테스트 종료는 PTY에서 Ctrl+C로 수행했다.
- 실제 X 버튼 재현에서 MSVC Run-Time Check Failure #2(`engine` 주변 스택 손상)를 확인했다. `Engine.h`에 로그 멤버를 추가한 뒤 Ninja가 헤더 의존성을 기록하지 않아 `main.cpp`가 예전 클래스 크기로 남은 것이 직접 원인이었다. clean 전체 빌드 후 실제 X 버튼 종료에서 오류 창 없이 프로세스 종료와 로그 flush를 확인했다.
- 재발 방지를 위해 SDL3 `SDL_EVENT_WINDOW_CLOSE_REQUESTED`를 정상 종료 경로로 연결하고, CMake 프리셋의 `VSLANG=1033`을 configure/build 양쪽에 고정해 Ninja `/showIncludes` 접두사와 MSVC 출력을 일치시켰다.
- Visual Studio 번들 CMake로 fresh configure/build 28단계 성공. Ninja deps에서 `main.cpp`가 `Engine.h`를 추적하는 것을 확인했고, fresh 실행본의 실제 X 버튼 클릭 후 런타임 오류 창 없이 프로세스 종료 및 로그 flush를 확인했다.
- `Daylight/Icon.png`를 런타임 `Assets/Image/Icon.png`로 복사하고, 창 생성 직후 `stb_image` RGBA 데이터와 `SDL_SetWindowIcon`으로 창 아이콘을 설정한다.
- 아이콘 변경 후 Debug 재빌드 성공. 런타임 아이콘 파일 복사를 확인했고 약 2초 실행에서 아이콘 관련 경고 없이 정상 종료 code 0.
- 사용자 피드백에 따라 원본의 넓은 검은 여백을 제거하고, 둥근 아이콘이 캔버스를 거의 채우며 모서리 바깥은 투명한 PNG로 교체했다. 소스와 Debug 런타임 복사본의 SHA-256 일치를 확인했다.
- 일반 Debug 빌드의 MSVC `C1902 Program database manager mismatch`와 임시 `/Z7` 산출물로 인한 PCH `C2855`를 재현했다. `Daylight`와 `tinygltf` Debug 정보를 CMake의 `Embedded`(`/Z7`)로 통일해 외부 PDB writer 의존성과 옵션 불일치를 제거했다.
- 수정 후 Visual Studio 개발 환경에서 별도 `_CL_` 우회 없이 clean Debug 전체 28단계 컴파일·링크 성공. Debug 설정은 프로젝트 차원에서 `/Z7`로 일관된다.
- 매 프레임 표시로 변경하기 전 Debug 실행 1280×720/FIFO 약 4초: 1초 표본 `241.32`, `240.27`, `239.70 FPS`; 평균 약 `240.43 FPS`. 정상 종료 exit code 0, 캡처된 로그에서 validation warning/error 없음.
- Release는 전체 소스 컴파일 성공 후 기존 `/LTCG:INCREMENTAL` 링크에서 linker access violation. Release 10초 기준선은 아직 미완료이며 현재 과제를 `verified_complete`로 처리하지 않는다.
- 사용자 요청으로 파일명 `VulkanPipeline.h/.cpp`는 유지하면서 래퍼를 `VulkanGraphicsPipeline`과 `VulkanComputePipeline`으로 분리했다. 기존 그래픽 호출부의 타입 변경을 포함한 Debug 전체 빌드·링크는 성공했다. 컴퓨트 래퍼는 코드·빌드만 확인했으며 아직 인스턴스 생성, dispatch, 화면 또는 validation으로 검증하지 않았다.
- 이후 사용자 작업에서 compute descriptor layout/pool/set 할당 초안과 `Blur.hlsl`이 추가됐다. 요청에 따라 depth-stencil image는 `MaxFramesInFlight` 수만큼 생성하고 현재 frame slot으로 선택하도록 분리했다. 현재 Debug 빌드는 descriptor 초기화 함수명 불일치, 미선언 `computeDescriptorSets`, set 할당 구문 오류 등 작성 중 코드에서 실패하므로 depth 변경도 아직 빌드·실행 검증 완료로 기록하지 않는다.

## 코드와 Git에서 확인한 현재 상태

기준 HEAD: `de83def` (2026-10-15 19:26, 컴포넌트 타입 ID 방식으로 변경). 2026-10-21 확인 시 원격 `origin/master`와 같았고, 검토 시작 전 작업 트리는 깨끗했다. 이번 검토는 렌더러 소스를 바꾸지 않고 이 계획만 갱신했다.
사용자가 설명한 리소스 핸들/리소스 매니저 코드는 현재 체크아웃과 원격 브랜치에서 찾지 못했다. 별도 Git 저장소의 구현은 소스를 받기 전까지 코드 분석·완료 검증으로 기록하지 않는다. 현재 저장소에는 `VulkanResourceState`만 있다.

| 기능 | 구현 상태 | 검증 상태와 근거 |
|---|---|---|
| SDL3 창, Vulkan 장치/스왑체인 | 코드 존재 | 10/21 기존 로그에 창 생성과 RTX 4060 Ti 선택 기록. 이번 검토에서 새 실행/화면은 확인하지 않음 |
| HLSL → SPIR-V, 삼각형/텍스처 사각형 파이프라인 | 코드 존재·빌드 성공 | DXC가 임의 프로파일을 받으므로 `cs_6_0` 경로를 재사용 가능. graphics/compute pipeline wrapper를 분리했으며 compute wrapper의 실제 생성·dispatch는 미검증 |
| 정점·인덱스 버퍼, staging 업로드 | 코드 존재 | GPU 복사와 transfer → vertex/index 의존성 있음. 현재 draw는 하드코딩 사각형 |
| Dynamic Rendering, 깊이 attachment | 코드 존재 | `Renderer::Render`의 `vkCmdBeginRendering`/`vkCmdEndRendering`. 3D 깊이 테스트 결과 미검증 |
| Synchronization2, timeline/바이너리 semaphore | 코드 존재 | `BeginFrame`/`EndFrame`, `vkCmdPipelineBarrier2`, `vkQueueSubmit2`, timeline wait 확인. 동기화 validation 결과 미확보 |
| Bindless 기초 | 일부 구현 | 64개 sampled-image 배열, PARTIALLY_BOUND/UPDATE_AFTER_BIND, sampler 별도 세트. 셰이더는 `textures[0]`만 사용. 동적/비균일 인덱싱 미구현·미검증 |
| Compute blur | 미구현 | `CS_Main` 문자열만 존재. compute pipeline, storage-image descriptor, dispatch, compute 상태/배리어는 없음 |
| Entity/Component/Transform/Camera | 구조·일부 구현 | Mesh Render는 주석만 있고 Renderer 연결/행렬 실행 검증 증거 없음 |
| Resource handle / manager | 이 체크아웃에서 미확인 | 별도 Git 구현이라는 사용자 설명만 있으며 현재 저장소에는 해당 소스가 없음 |
| TinyGLTF v3 라이브러리/모델 준비 | 설정·파일 확인 완료 | C11 정적 라이브러리 연결, FS 매크로, 로컬 v3 헤더 존재. 애플리케이션의 parse 호출은 없음 |
| glTF CPU 파싱/데이터 추출 | 미구현 | Source/Shaders 검색과 Renderer 초기화 경로에서 로더 부재 확인 |
| 정적 glTF 모델 렌더링·카메라·glTF 재질 | 미구현 | 현재 POSITION은 Vector2, 모델 행렬/카메라/모델 재질 경로 없음 |
| PBR / Deferred / Toon | 미구현 | 후속 로드맵 단계 |

**검증의 한계 (10/21):** `cmake --build --preset windows-debug`는 exit code 0으로 성공했지만 변경 대상이 없어 runtime 파일 복사만 수행했다. 앱 실행, 화면 캡처, validation 결과는 이번 검토에서 새로 확인하지 않았다. `out/build/windows-debug/log.txt`의 창/GPU 로그는 10:49의 기존 실행 기록이므로 오늘 컴퓨트 과제의 증거가 아니다.

### 오늘 과제의 선행 조건

- `VK_FORMAT_R8G8B8A8_UNORM`의 optimal tiling이 `VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT`를 지원하는지 물리 장치에서 확인한다.
- HLSL storage image의 `rgba8` 형식과 `VK_FORMAT_R8G8B8A8_UNORM` image view를 정확히 맞춘다. 첫 과제에서는 `Texture2D.Load`를 사용해 sampler와 implicit LOD 문제를 범위에서 뺀다.
- storage image 출력은 compute 접근 중 `VK_IMAGE_LAYOUT_GENERAL`을 사용한다.
- 출력 이미지는 입력과 분리한다. 같은 픽셀 주변값을 읽으면서 같은 이미지를 덮어쓰는 in-place blur는 호출 간 순서가 없어 첫 과제에서 제외한다.
- `EResourceState`에는 compute storage read/write 상태가 없으므로, `COMPUTE_SHADER` stage와 `SHADER_READ/WRITE` access 및 `GENERAL` layout을 표현할 수 있어야 한다.
- compute dispatch는 `vkCmdBeginRendering` 밖에서 기록하고, 출력은 이후 fragment shader가 읽기 전에 배리어로 가시성을 확보한다.

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

## 과거 일정 기록 — 10/14~10/19

Google Calendar의 기본 캘린더, 두 수업 캘린더, 대한민국 휴일 캘린더를 실제 조회했다.
10/14에 사용자 정정한 **10/16(금) 외출, 10/17(토) 결혼식** 종일 일정을 10/15에도 재조회했다. 이번 실행은 개인 일정을 변경하지 않는다.
10/18(일)은 평일 학습 패턴 밖의 휴식일이다.

- 당시 핵심 목표: 모델 파싱 메타데이터를 실제 로더 로그로 검증하는 것이었다. 완료 증거는 없으며 10/21 사용자 우선순위 변경으로 대기열에 둔다.
- 10/15: 오늘 과제가 미완료이면 같은 과제를 축소/재배치한다. 완료 증거가 있으면 첫 primitive의 CPU 추출 하나만 선정한다.
- 10/16~18: 개발 과제 없음. 미완료 작업은 10/19에 재평가하며 3일 분량을 누적하지 않는다.
- 10/15 오전 실제 실행: 다음 학습일 10/19 전체 날짜를 네 캘린더에서 조회했다. 예약 시각 19:00에 시작했다고 주장하지 않는다. 이후 리뷰는 최신 진도/일정을 재평가한다.
- 10/19: Vulkan 10:00~12:00, Blender 20:00~20:30 실제 등록 및 read_event/receipt/중복 재조회 검증 완료. 선형대수는 anchor 10/14와 5일 차이여서 대상 아님. 10/16·18 격일 분량은 외출/주말로 생략하고 누적하지 않는다. 다음 격일 대상 10/20은 그날 Calendar 확인 후 배치한다.
- 개발 기본 시간: 평일 10:00~20:00 중 1~3시간 블록 하나. 충돌 시 빈 시간으로 옮기거나 작은 검증 단위로 줄인다.
- 선형대수: 사용자 요청으로 10/14를 새 격일 루틴의 시작점으로 설정했다. 현재 강의/문제 진도 확인과 최근 개념 복습을 30분으로 등록한다. 과거 학습일이나 수강 진도를 추측하지 않는다. 외출/주말로 빠진 분량은 누적하지 않는다.
- Blender: 가용 저녁 20:00~22:00 중 30분. 현재 챌린지 일차는 먼저 확인하며 10/14~15에는 20:00~20:30으로 등록했다.
- 블로그: 주 1회 초안 정리/발행 선택. 개발 과제 완료 조건과 분리한다.

## 짧은 복습 노트 — 이해도는 아직 평가하지 않음

- **Compute / storage image:** workgroup과 invocation, `ceil(size / local_size)` dispatch, storage image의 `GENERAL` layout, compute write → fragment sampled read 배리어를 연결해 설명한다.
- **동기화:** transfer write → vertex/index read 또는 fragment sampled read 의존성을 현재 `ConvertResourceState`와 연결해 설명한다. `vkQueueWaitIdle`과 메모리 배리어가 담당하는 역할을 구분한다.
- **프레임 동기화:** image-acquired/프레젠트 완료 바이너리 semaphore와 CPU 프레임 재사용을 제한하는 timeline semaphore를 구분한다.
- **Bindless:** 배열 64개를 만들고 slot 0을 읽는 단계와 런타임/비균일 인덱싱은 다르다. PARTIALLY_BOUND는 실제 접근한 미초기화 descriptor를 유효하게 만들지 않는다.
- **Pipeline/Shader:** 현재 Vector2/float2와 모델 VEC3의 차이, 정점 stride/attribute format 및 행렬 변환을 5~10줄로 적는다.
- 막힌 개념만 개인 노트로 남긴다. 블로그 발행은 요구하지 않는다.

## 장기 로드맵 — 매일 활성 과제는 하나

| 순서 | 마일스톤 | 검증 가능한 완료 기준 | 상태 |
|---|---|---|---|
| 0 | FPS 기준선 | CPU/present 순간 FPS와 deltaTime을 매 프레임 표시하고 Release 관찰 범위·조건을 기록 | **현재 활성 과제** |
| 1 | 기존 이미지 출력 기준선 | 새 Debug 실행 화면, 정상 종료, resize/minimize, validation 결과 기록 | 코드 있음·재검증 필요 |
| 2 | Compute blur 1-pass | dispatch·storage image·compute→fragment 배리어 후 블러 화면과 validation 0 | 대기 |
| 3 | glTF 파싱 | 실제 TinyGLTF 로그가 위 메타데이터와 일치 | 미완료·대기 |
| 4 | 첫 primitive CPU 추출 | 위치/인덱스 개수·범위·stride·샘플 값 확인 | 대기 |
| 5 | 정적 glTF 출력 | 3D 위치/적절한 인덱스 업로드, 고정 MVP·깊이로 단색 모델 출력 | 대기 |
| 6 | 카메라/노드/전체 모델 | 좌표계와 노드 변환 검증, orbit 및 전체 primitives 출력 | 대기 |
| 7 | glTF 텍스처·재질 | UV/baseColor·sampler 연결, 여러 재질 인덱스 검증 | 대기 |
| 8 | PBR | metallic/roughness와 normal, 직접광 결과를 기준 모델과 비교 | 대기 |
| 9 | Deferred Rendering | G-Buffer 위치/normal/material 시각화, lighting pass·의존성 검증 | 대기 |
| 10 | Toon Shading | 명암 단계화 후 윤곽선 효과를 각각 비교 검증 | 대기 |

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
| 2026-10-15 저녁 | HEAD ce490eb·사용자 미커밋 Scene/Resources·기존 빌드/검사 로그·Calendar 재조회 | glTF 완료 증거 없음; 현재 전체 빌드/행렬 검증 미확보. 두 이벤트 update/read_event/receipt/중복 검증 성공 | 10/19 동일 핵심 과제; Blender 유지·선형대수 미해당 |

### 10/15 저녁 재검토 — 실행 증거 및 일정 처리

- 실제 시작은 약 18:56 KST이며 19:00 자동 트리거 검증으로 해석하지 않는다. 오전 실행 이후의 새 커밋/미커밋 파일을 재평가했다.
- 기존 `out/build/precommit-review-20261015/build.log`에는 /ZI와 /Z7 PCH 옵션 불일치(C2855/C1903)가 있다. component-probe.log에는 AddComponent 컴파일 오류가 있으나 현재 Entity.h는 해당 과거 코드와 달라 현재 실패로 단정하지 않는다. component-unit/resource-manager obj 존재도 전체 빌드/실행 성공의 근거가 아니다. 이번 자동화는 빌드·렌더러·테스트를 실행하지 않았다.
- 코드 읽기에서 TransformComponent의 translation 행렬이 최종 합성에 사용되지 않고 CameraComponent의 perspective 계산 반환값이 ProjectionMatrix에 저장되지 않는 점을 확인했다. 향후 카메라 단계에서 재검증할 메모이며 별도 활성 과제를 추가하지 않는다.
- glTF 과제는 pending, attempts=0, completion_evidence=null, 10/19 유지. 남은 시간/실패가 확인되면 기존 1시간 첫 primitive 범위로 줄인다.
- Calendar 네 개를 10/15~19 전체 날짜로 모든 페이지 조회하고 독점 lock 후 재조회했다. 10/16 외출·10/17 결혼식 및 10/18 주말은 생략한다. 10/19 Vulkan 10:00~12:00·Blender 20:00~20:30 기존 소유 ID의 내용만 갱신 대상으로 결정했다.
- 날짜별 planner 초안에서 10/16·18 선형대수/Blender 및 Vulkan은 defer, 10/17 Vulkan/Blender도 defer다. 10/19는 Blender만 대상이며 선형대수는 격일 미해당이다. 빠진 분량을 누적하지 않으며 다음 격일 대상 10/20은 다음 검토에서 실제 일정을 확인한다.
- 18:59 KST 실제 갱신 완료: Vulkan ID `1ufg1ljbcle9d22pdoq77tmqf0`, Blender ID `51l8q4ffm1014h1smi1jq723e8`. 시간 필드 없이 기존 ID의 내용만 갱신했고 각각 read_event 및 planner receipt를 통과했다. 최종 네 캘린더 재검색에서 식별자별 1개, 10/15 기존 학습·10/16 외출·10/17 결혼식의 ID/시간 보존을 확인했다. 상세 결과: `study/automation/runtime/2026-10-15-evening-review-result.json`.

## 자동화가 읽는 현재 과제 상태

미완료 횟수는 사용자의 실행 기록/실패 근거가 있을 때만 늘린다. Git diff 유무로 늘리지 않는다.
`verified_complete`로 바꾸려면 `completion_evidence`에 실행 결과를 남긴다.
아래 JSON은 이 문서의 활성 과제와 같은 내용이며, 별도 상태 파일로 학습 진도를 관리하지 않는다.

<!-- study-state:start -->
```json
{
  "current_task": {
    "id": "baseline-cpu-present-fps",
    "status": "in_progress",
    "attempts": 0,
    "completion_evidence": {
      "debug_build": "Daylight와 tinygltf Debug를 CMake Embedded(/Z7)로 통일한 뒤 clean 전체 컴파일·링크 성공",
      "debug_run": "1280x720 FIFO, 정상 종료 code 0, FPS 241.32/240.27/239.70, 평균 약 240.43",
      "validation": "약 4초 캡처 로그에서 warning/error 없음",
      "remaining": "Release LTCG 링크 access violation 해결 후 3초 워밍업과 10초 평균 측정"
    },
    "title": "현재 렌더 루프 FPS 기준선 측정",
    "goal": "블러 구현 전에 현재 루프의 CPU/present 순간 FPS와 deltaTime을 동일 조건에서 매 프레임 표시하고 기준 범위를 기록한다.",
    "first_20_minutes": [
      "지표를 CPU/present loop FPS와 ms로 정의하고 GPU 실행 시간과 구분한다.",
      "Engine에 steady_clock 이전 프레임 시각과 초 단위 deltaTime을 두고 성공적으로 획득한 프레임에서 갱신하는 흐름을 설계한다.",
      "매 프레임 창 제목을 갱신하며 최소화와 스왑체인 skip 때 이전 시각을 reset하도록 설계한다."
    ],
    "done": [
      "Debug와 Release 빌드가 성공하고 순간 FPS와 deltaTime이 매 프레임 표시된다.",
      "1280x720 FIFO에서 3초 워밍업 뒤 Release 10초 관찰 범위와 대표값을 기록한다.",
      "Debug validation error/warning 0을 별도로 확인하고 FIFO 제한 및 GPU 시간 미측정이라는 한계를 기록한다."
    ],
    "question": "FIFO에서 측정한 60 FPS를 GPU 렌더링 시간 16.67 ms로 해석할 수 없는 이유는 무엇인가?",
    "next": "CPU/present 기준선 완료 뒤 compute blur를 구현하며, 세부 GPU 비용 비교가 필요하면 timestamp query를 추가한다.",
    "duration_minutes": 60,
    "minimum_minutes": 30,
    "short_session_goal": "순간 FPS와 deltaTime ms를 매 프레임 창 제목에 표시하고 Release 관찰 범위를 기록한다.",
    "deferred_tasks": [
      {
        "id": "compute-blur-first-dispatch",
        "status": "pending",
        "completion_evidence": null,
        "reason": "블러 전 FPS 기준선 측정을 먼저 수행"
      },
      {
        "id": "gltf-v3-metadata",
        "status": "pending",
        "completion_evidence": null,
        "reason": "10/21 사용자 우선순위가 compute shader 학습으로 변경되어 대기"
      }
    ],
    "planned_date": "2026-10-21"
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

- 대상 학습일: 2026-10-21
- 모드: not_requested · 동작: 없음
- 사유: 이번 대화에서는 오늘의 기술 과제만 재선정했으며 Calendar 변경은 요청받지 않았다.
- 활성 과제: 현재 렌더 루프의 CPU/present FPS 기준선 측정


<!-- calendar-draft:end -->
