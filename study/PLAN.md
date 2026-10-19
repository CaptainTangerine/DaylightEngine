# Vulkan Renderer · 학습 계획

> 갱신: 2026-10-19 (Asia/Seoul). 학습 상태의 단일 기준은 이 파일이다.
> 코드 존재, 빌드 성공, 실행/화면 검증, 이해도 확인을 서로 구분한다.

## 학습 방향과 작업 범위 — 2026-10-15 사용자 정정

**엔진 아키텍처 → 리소스 관리 → 모델 로딩 → Forward PBR → Toon/Deferred/Bindless 확장** 순서로 학습한다.
주요 자료는 [Khronos Building a Simple Engine](https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/introduction.html)이다.
전체 게임 엔진이 아닌 PBR/Toon 실험용 소규모 렌더링 엔진을 목표로 한다.

**학습 방향·계획·Google Calendar 변경 요청은 학습 문서와 일정 수정 요청이다.**
과제에 구현·빌드·렌더링 검증이 적혀 있어도 수행자는 사용자이며, Codex가 소스를 대신 수정하거나 실행할 허가로 해석하지 않는다.
별도의 명시적인 코드 작업 요청 전에는 소스·셰이더·CMake를 수정하거나 빌드/렌더러를 실행하지 않는다.
기존 기능은 유지하고 Bindless 확장은 후순위로 둔다. 이전 glTF 메타데이터 과제는 미완료 상태로 보류하며 실패 횟수를 늘리지 않는다.

## 다음 학습일의 단 하나의 핵심 과제 — 2026-10-20 (화)

**Renderer와 ResourceManager의 책임·소유권을 현재 코드에 맞춰 설계한다.** 예상 2시간.
이번 과제는 설계 노트 작성이며 실제 구현은 설계 확인 뒤 사용자가 작은 단위로 진행한다.
10/19 오전 검토에서 사용자 설계 노트/완료 보고를 찾지 못해 같은 과제를 다음 학습일 10/20로 이어간다. 오늘 10~12시 일정은 진행 가능한 기존 약속으로 보존하며 실패·지연으로 판단하지 않는다. pending/attempts=0/completion_evidence=null을 유지한다.

- 목표: 현재 소스/CMake를 읽고 Renderer·리소스 관리·렌더링 데이터의 책임과 생성/해제 주체를 정한다.
- 첫 20분:
  1. 0~10분: 공식 Architectural Patterns의 책임 분리 부분과 Engine/Renderer의 소유 객체를 함께 읽는다.
  2. 10~20분: Resource Management의 Resource Handles/Basic Resource Manager를 읽고 현재 초안과 차이를 적는다.
- 완료 기준:
  - Engine/Renderer/Buffer/Image/Sampler/Descriptor/ResourceManager의 책임·소유권 표를 작성한다.
  - 유지할 부분·옮길 책임·보류할 시스템을 구분하고 최소 draw 데이터 및 첫 리소스 범위를 정한다.
  - CPU 객체 소멸과 GPU 사용 완료의 차이, device보다 먼저 해제할 객체와 종료 순서를 개인 노트 5~10줄로 설명한다.
  - 노트/설계 확인 증거가 있을 때만 완료 처리한다. 구현·빌드 성공을 추측하지 않는다.
- 자료:
  - [Architectural Patterns — 책임 분리](https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/Engine_Architecture/02_architectural_patterns.html)
  - [Resource Management — Handles / Basic Resource Manager](https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/Engine_Architecture/04_resource_management.html)
- 이해 질문: ResourceManager가 CPU 객체를 소유하고 Renderer가 VkBuffer나 descriptor를 참조할 때, 언제 GPU 자원을 안전하게 해제할 수 있는가?
- 다음 단계: 설계 확인 후 리소스 한 종류의 동기 로딩/캐싱을 사용자 구현 과제로 선정한다. 그 기반 이후 glTF를 재개한다.
- 시간 부족 시: Engine/Renderer/ResourceManager 책임·종료 순서 한 페이지만 정리하는 1시간 범위로 줄인다.

### 이번 실행의 실제 Calendar 검증 — 2026-10-19 오전

- 다음 학습일 **10/20(화), Asia/Seoul**의 동일 미완료 설계 과제 하나를 유지했다. 완료 증거가 없다는 뜻이며 오늘 과제 실패나 미학습을 뜻하지 않는다. `pending`, `attempts=0`, `completion_evidence=null`이다.
- **Vulkan 10:00~12:00**: 엔진 구조 · 책임과 리소스 소유권 설계. 실제 생성 ID `d56t8bf6gsg97qkcn5mu9knf38`.
- **선형대수 14:00~14:30**: 현재 진도 확인 · 개념 복습. 10/14 격일 시작점에서 +6일로 대상. 실제 생성 ID `et6s1p8n27sr6o6c3006k1c93g`.
- **Blender 20:00~20:30**: 마지막 일차/파일 확인 후 현재 단계 연습. 실제 생성 ID `lrb1g7bphmvsj79j09fl0k1f00`.
- 오늘 10/19 Vulkan/Blender도 기존 ID의 내용만 갱신하고 시간 필드는 보내지 않았다. 10:00~12:00 / 20:00~20:30과 설명 끝 이후 메모를 보존했다. 오늘 선형대수는 비대상이며 새 일정 없음.
- 생성 3개·내용 갱신 2개 모두 read_event의 confirmed/ID/제목/시간/고유 marker를 확인하고 날짜-kind receipt가 planner 검증을 통과했다. 네 캘린더 최종 전체 날짜 검색에서 marker별 1개(총 5개), 기존 ID/시간 보존 및 충돌 없음을 확인했다. 개인 이벤트 변경/삭제 없음.
- 최종 재생성한 10/20 Vulkan·두 routine 초안은 모두 동일 registry ID의 update이며 시간 필드가 없다. 이 중복 방지 검증 때문에 추가 API 갱신을 하지는 않았다. 상세 결과: `study/automation/runtime/2026-10-19-review-result.json`.
- 첫 20분·완료 기준·공식 자료 2개·이해 질문·다음 단계는 활성 과제와 Calendar 설명에 함께 기록했다. 블로그 발행을 완료 조건으로 두지 않는다. 이번 오전 실행은 매일 19:00 자동 트리거의 검증 근거가 아니다.

### 방향 변경 후 실제 Calendar 반영 — 2026-10-15

- 10/19(월) 10:00~12:00의 기존 Vulkan 이벤트를 **엔진 구조 · 책임과 리소스 소유권 설계**로 실제 수정했다.
- 기존 ID `1ufg1ljbcle9d22pdoq77tmqf0`와 시간은 보존했다. 목표·첫 20분·완료 기준·공식 자료 2개·이해 질문·다음 단계를 새 과제로 바꿨다.
- 수정 후 read_event와 planner receipt를 통과했고 최종 검색에서 고유 marker별 1개를 확인했다. Blender 20:00~20:30 및 개인 일정은 변경하지 않았다.
- 이전 glTF 제목의 등록 기록과 아래 저녁 자동화 기록은 **방향 변경 전 이력**이다. 현재 활성 과제/로드맵/current_task와 이 기록이 최신 기준이다.
- 소스 변경은 원복 완료. 향후 학습·일정 요청으로 소스 수정/빌드/렌더러 실행을 하지 않는다.

## 코드와 Git에서 확인한 현재 상태

기준 HEAD: `de83def` (컴포넌트 타입 ID 조회 구조).
현재 Renderer가 샘플 정점/인덱스·이미지 파일 로딩·GPU 리소스 소유·descriptor 갱신·draw 기록을 함께 담당한다.
VulkanDevice는 초기화/장치·VMA·swapchain·프레임 동기화·동기 업로드와 descriptor pool/layout/set을 소유한다.
VulkanBuffer/Image/Sampler/Pipeline은 device를 참조하는 GPU 객체 래퍼이고, 별도 고수준 Texture·Descriptor 클래스는 없다.
Entity/Component/Camera/MeshComponent는 아직 실제 draw 경로에 연결되지 않았다.
Resources/Resource·ResourceHandle·ResourceManager는 사용자 미추적 작성 중인 초안이다.
이번 정정에서 Codex가 잘못 적용했던 소스 변경과 새 파일은 요청 전 상태로 원복했고 사용자 초안을 보존했다.
해당 임시 코드의 빌드/실행은 사용자 학습 완료 증거로 사용하지 않는다. 이후 문서·일정만 변경한다.

| 기능 | 구현 상태 | 검증 상태와 근거 |
|---|---|---|
| SDL3 창, Vulkan 장치/스왑체인 | 코드 존재 | 기존 10/12 로그에서 창 생성 및 MX450 선택만 확인 |
| HLSL → SPIR-V, 삼각형/텍스처 사각형 파이프라인 | 코드 존재 | 화면 출력은 커밋/기존 사용자 보고. 이번 검토에서 화면 재검증하지 않음 |
| 정점·인덱스 버퍼, staging 업로드 | 코드 존재 | GPU 복사와 transfer → vertex/index 의존성 있음. 현재 draw는 하드코딩 사각형 |
| Dynamic Rendering, 깊이 attachment | 코드 존재 | `Renderer::Render`의 `vkCmdBeginRendering`/`vkCmdEndRendering`. 3D 깊이 테스트 결과 미검증 |
| Synchronization2, timeline/바이너리 semaphore | 코드 존재 | `BeginFrame`/`EndFrame`, `vkCmdPipelineBarrier2`, `vkQueueSubmit2`, timeline wait 확인. 동기화 validation 결과 미확보 |
| Bindless 기초 | 일부 구현 | 64개 sampled-image 배열, PARTIALLY_BOUND/UPDATE_AFTER_BIND, sampler 별도 세트. 셰이더는 `textures[0]`만 사용. 동적/비균일 인덱싱 미구현·미검증 |
| Entity/Component/Transform/Camera/ResourceManager | 구조·일부 구현 | Mesh Render는 주석만 있고 Renderer 연결/현재 전체 빌드·행렬 검증 증거 없음 |
| TinyGLTF v3 라이브러리/모델 준비 | 설정·파일 확인 완료 | C11 정적 라이브러리 연결, FS 매크로, 로컬 v3 헤더 존재. 애플리케이션의 parse 호출은 없음 |
| glTF CPU 파싱/데이터 추출 | 미구현 | Source/Shaders 검색과 Renderer 초기화 경로에서 로더 부재 확인 |
| 정적 glTF 모델 렌더링·카메라·glTF 재질 | 미구현 | 현재 POSITION은 Vector2, 모델 행렬/카메라/모델 재질 경로 없음 |
| PBR / Deferred / Toon | 미구현 | 후속 로드맵 단계 |

**검증의 한계 (10/15 재조회):** Debug exe는 10/14 14:06, 별도 vulkan-image-verify/Daylight.exe는 10/14 14:32 산출물이다. 파일 존재/시각으로 빌드 과정이나 화면/validation 성공을 단정하지 않는다. Debug log.txt는 창/GPU 선택만 기록한다. 10/12 pipeline-check.stdout.log는 창/GPU 2줄, stderr는 비어 있다. image-fallback-check/log.txt의 fallback 경고는 별도 확인용 산출물이며 현재 Image.cpp의 abort 경로와 구분한다.
CTest LastTest.log는 저녁 재조회에서 10/15 18:21 시작/종료만 있으며 테스트 통과 증거가 아니다. Daylight/log.txt는 기존 10/7 로그이며 glTF 파싱 성공/메타데이터 출력 증거는 찾지 못했다.
이번 자동화는 빌드/렌더러 실행/validation 테스트를 수행하지 않았다. Git diff 부재로 학습 부재/이해도를 판단하지 않는다.

### 설계 시 구분할 현재 책임과 소유권

| 대상 | 현재 책임/소유 | 학습 설계에서 판단할 점 |
|---|---|---|
| Engine | 창·이벤트 루프, VulkanDevice/Renderer 소유, 종료 시 GPU 대기 | ResourceManager를 명시적으로 소유할 위치와 종료 순서 |
| Renderer | 샘플 CPU 데이터·GPU buffer/image/sampler, pipeline, descriptor 갱신·draw | 자원 로딩/캐시와 draw 기록의 분리 경계 |
| VulkanDevice / Swapchain | 장치·VMA·동기화·업로드·descriptor 및 swapchain/depth 수명 | 기존 기반 재사용. 한 번에 분해하지 않음 |
| VulkanBuffer / VulkanImage | VkBuffer 또는 VkImage/view와 VMA allocation | 실제 owner가 device보다 먼저 해제하고 GPU 마지막 사용을 기다릴 책임 |
| Image / VulkanSampler | CPU 픽셀과 GPU 샘플링 설정을 각각 담당 | Texture에서 이미지·색 공간·sampler 연결을 어떻게 표현할지 |
| Descriptor | device의 pool/layout/set, Renderer가 갱신/바인딩 | descriptor는 참조 대상 GPU 객체의 소유자가 아님 |
| ResourceManager / Resource / Handle | 사용자 작성 중인 초안, 캐시/해제 정책 미완성 | 우선 한 종류의 동기 로딩·중복 key·실패·명시적 종료만 설계 |
| Entity / Component | Entity가 컴포넌트 소유, 타입 ID 조회 | 전체 ECS로 교체하지 않고 Mesh/Material 참조에 필요한 최소 역할만 유지 |

- **유지:** CMake, Vulkan C API/기존 래퍼, HLSL/DXC, DirectXMath/SimpleMath와 현재 좌표계, VMA, Dynamic Rendering, Sync2/timeline, 기존 이미지 출력·Bindless 코드.
- **설계 후 사용자 수정 대상:** Renderer의 자원 로딩/소유와 draw 책임 분리, ResourceManager의 생성·로딩·캐싱·해제 계약, Mesh/Texture/Material의 필요한 데이터.
- **보류:** Bindless 확장, RenderGraph, 비동기 streaming/hot reload, 범용 서비스/이벤트 시스템과 전체 게임 엔진 기능.
- CPU 참조가 사라졌다는 이유만으로 제출 중인 GPU 자원을 해제하지 않는다. 우선 종료 시 GPU 완료 대기 후 리소스→device 순서로 정리하는 단순 정책을 이해하고, 실행 중 삭제가 필요할 때 timeline 기반 지연 해제를 별도 과제로 선정한다.

### 이후 모델 단계의 선행 조건 — 현재 과제 아님

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

- 현재 핵심 목표: Renderer/ResourceManager 책임·소유권 설계. 10/19에는 이 설계 과제 하나만 활성화한다. glTF 메타데이터·CPU 추출·모델 출력은 아키텍처/리소스 관리 기반 뒤로 보류한다.
- 10/15 방향 변경: 기존 glTF 과제를 미완료 보류로 옮기고 아키텍처·리소스 관리 설계를 먼저 진행한다.
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

| 단계 | 학습·사용자 개발 순서 | 검증 가능한 기준 | 상태 |
|---|---|---|---|
| 1. 엔진 아키텍처 | 현재 코드/CMake 분석 → 책임·소유권 설계 → 사용자 최소 분리 → 기존 렌더링 회귀 확인 | 책임 표·수명 계약을 설명하고 이후 실제 변경 시 빌드/화면/validation 증거를 따로 기록 | 현재: 책임·소유권 설계만 활성 |
| 2. 리소스 관리 | ResourceManager에서 한 종류 생성/로딩/캐싱/실패/해제 → Mesh/Texture/Material로 점진 확장 | 같은 key 중복 로딩 방지, 실패 처리, CPU/GPU 수명과 device 이전 해제 근거 | 대기 |
| 3. 모델 로딩 | TinyGLTF v3 glTF/GLB 메타데이터 → 첫 primitive 정점/인덱스 → 기본 Mesh draw → Texture/Material | 실제 파싱 로그, offset/stride/범위·인덱스 타입, 단색 모델 화면과 이후 재질 검증 | 기존 glTF 과제 미완료 보류 |
| 4. Forward PBR | glTF Metallic-Roughness factor·texture → 기본 Forward 직접광 → normal/색 공간 검증 | 서로 다른 metal/roughness의 일관된 기준 화면, 기본 재질·빛·좌표계 설명 | 대기 |
| 5. 기능 확장 | PBR 안정화 후 Toon, Deferred, Bindless를 필요에 따라 하나씩 | 각 기능의 기준 화면과 의존성·성능·validation 결과 | 후순위, 기존 코드 삭제 없음 |

주요 학습 자료:
[Engine Architecture](https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/Engine_Architecture/02_architectural_patterns.html),
[Resource Management](https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/Engine_Architecture/04_resource_management.html),
[Rendering Pipeline](https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/Engine_Architecture/05_rendering_pipeline.html),
[Loading Models](https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/Loading_Models/03_model_system.html),
[Lighting & Materials](https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/Lighting_Materials/04_lighting_implementation.html).
튜토리얼은 설계 참고자료다. 예제의 Vulkan-Hpp/GLM/Slang/구형 TinyGLTF API를 그대로 옮기지 않는다.
매일 현재 과제에 맞는 구간 1~2개만 고르고, 단계별 변경 의도·실제 검증·개인 학습 개념을 기록한다.
블로그 발행은 완료 조건이 아니다.

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
| 2026-10-19 오전 | HEAD de83def·미추적 초안·기존 로그 및 실제 Calendar 재검토, 3개 생성/2개 갱신 후 read_event·receipt·marker 검증 | 설계 완료 증거 없음; pending/attempts=0. 현재 빌드·화면·validation 미검증 | 10/20 동일 소유권 설계 10~12시, 선형대수 14시·Blender 20시 |

### 10/15 저녁 재검토 — 실행 증거 및 일정 처리

- 실제 시작은 약 18:56 KST이며 19:00 자동 트리거 검증으로 해석하지 않는다. 오전 실행 이후의 새 커밋/미커밋 파일을 재평가했다.
- 기존 `out/build/precommit-review-20261015/build.log`에는 /ZI와 /Z7 PCH 옵션 불일치(C2855/C1903)가 있다. component-probe.log에는 AddComponent 컴파일 오류가 있으나 현재 Entity.h는 해당 과거 코드와 달라 현재 실패로 단정하지 않는다. component-unit/resource-manager obj 존재도 전체 빌드/실행 성공의 근거가 아니다. 이번 자동화는 빌드·렌더러·테스트를 실행하지 않았다.
- 코드 읽기에서 TransformComponent의 translation 행렬이 최종 합성에 사용되지 않고 CameraComponent의 perspective 계산 반환값이 ProjectionMatrix에 저장되지 않는 점을 확인했다. 향후 카메라 단계에서 재검증할 메모이며 별도 활성 과제를 추가하지 않는다.
- glTF 과제는 pending, attempts=0, completion_evidence=null, 10/19 유지. 남은 시간/실패가 확인되면 기존 1시간 첫 primitive 범위로 줄인다.
- Calendar 네 개를 10/15~19 전체 날짜로 모든 페이지 조회하고 독점 lock 후 재조회했다. 10/16 외출·10/17 결혼식 및 10/18 주말은 생략한다. 10/19 Vulkan 10:00~12:00·Blender 20:00~20:30 기존 소유 ID의 내용만 갱신 대상으로 결정했다.
- 날짜별 planner 초안에서 10/16·18 선형대수/Blender 및 Vulkan은 defer, 10/17 Vulkan/Blender도 defer다. 10/19는 Blender만 대상이며 선형대수는 격일 미해당이다. 빠진 분량을 누적하지 않으며 다음 격일 대상 10/20은 다음 검토에서 실제 일정을 확인한다.
- 18:59 KST 실제 갱신 완료: Vulkan ID `1ufg1ljbcle9d22pdoq77tmqf0`, Blender ID `51l8q4ffm1014h1smi1jq723e8`. 시간 필드 없이 기존 ID의 내용만 갱신했고 각각 read_event 및 planner receipt를 통과했다. 최종 네 캘린더 재검색에서 식별자별 1개, 10/15 기존 학습·10/16 외출·10/17 결혼식의 ID/시간 보존을 확인했다. 상세 결과: `study/automation/runtime/2026-10-15-evening-review-result.json`.

### 10/19 오전 재검토 — 완료 증거와 다음 학습일

- 실제 검토 시각은 10:08 KST 이후이며 19:00 예약 트리거 검증으로 보지 않는다. HEAD `de83def`, staged 변경 없음, 기존 PLAN/README 수정과 미추적 Resources 4개 파일을 보존했다.
- Engine은 VulkanDevice/Renderer를 소유하고 Shutdown에서 vkDeviceWaitIdle 뒤 renderer/device 순서로 해제한다. Renderer는 이미지 로딩·GPU buffer/image/sampler 소유·descriptor 갱신·draw를 함께 맡는다. ResourceManager는 GetResource 선언뿐이며 ResourceHandle/Resource 초안은 생성자명·식별자·시그니처 정합성도 아직 확인이 필요하다. 코드 읽기 결과이며 빌드 실패를 실행 검증한 것은 아니다.
- 소유권 설계의 사용자 노트/완료 보고는 확인되지 않았다. PLAN에 자동 작성된 현재 책임 표만으로 사용자의 설계 과제를 완료 처리하지 않는다. glTF는 계속 보류하며 새 핵심 과제를 추가하지 않는다.
- 기존 component-unit 로그는 비어 있고 build.log는 과거 PCH 옵션 불일치 실패를 기록한다. windows-debug CTest LastTest.log는 10/19 10:06 시작/종료만 있어 통과 증거가 아니다. 실행 파일 시각이나 이 로그로 현재 빌드·화면·validation·이해도를 단정하지 않는다. 빌드/테스트/렌더러를 실행하지 않았다.
- 공식 Architectural Patterns는 직접 열기 timeout 후 공식 검색 결과에서 책임 분리 내용을 확인했고 Resource Management의 Resource Handles/Basic Resource Manager 구간을 읽었다. 튜토리얼 전체 구현을 복제하지 않고 현재 소규모 엔진의 책임 설계 참고로 사용한다.
- lock 획득 후 네 캘린더의 10/19~20 전체 날짜를 모든 페이지 조회했다. 10/20 개인 충돌 일정은 없으며 Vulkan 10:00~12:00, 선형대수 14:00~14:30(10/14 +6일), Blender 20:00~20:30 초안을 처리한다. 오늘 10/19는 선형대수 비대상이며 기존 Vulkan/Blender 시간과 메모를 보존한다. 빠진 주말 분량은 누적하지 않는다.
- Calendar 실제 반영/재조회/receipt 결과는 아래 최종 검증 기록에서 확정한다.

## 자동화가 읽는 현재 과제 상태

미완료 횟수는 사용자의 실행 기록/실패 근거가 있을 때만 늘린다. Git diff 유무로 늘리지 않는다.
`verified_complete`로 바꾸려면 `completion_evidence`에 실행 결과를 남긴다.
아래 JSON은 이 문서의 활성 과제와 같은 내용이며, 별도 상태 파일로 학습 진도를 관리하지 않는다.

<!-- study-state:start -->
```json
{
  "current_task": {
    "id": "engine-resource-ownership-design",
    "status": "pending",
    "attempts": 0,
    "completion_evidence": null,
    "title": "Renderer와 ResourceManager의 책임·소유권 설계",
    "goal": "현재 소스와 CMake를 읽고 소규모 렌더링 엔진에서 Renderer·ResourceManager·Mesh/Texture/Material의 책임과 생성·해제 순서를 설계한다.",
    "first_20_minutes": [
      "0~10분: 공식 Engine Architecture의 Architectural Patterns에서 책임 분리 구간을 읽고 Engine/Renderer가 소유한 객체를 적는다.",
      "10~20분: Resource Management의 Resource Handles/Basic Resource Manager를 읽고 현재 Resource/ResourceManager 초안과 차이를 적는다."
    ],
    "done": [
      "현재 Engine/Renderer/Buffer/Image/Sampler/Descriptor와 리소스 초안의 책임·소유권 표를 작성한다.",
      "유지할 부분·옮길 책임·보류할 시스템을 나누고 Renderer에 전달할 최소 draw 데이터 및 ResourceManager의 첫 리소스 범위를 정한다.",
      "CPU 객체 소멸과 GPU 사용 완료가 다른 이유, device보다 먼저 해제할 객체와 종료 순서를 5~10줄 개인 노트로 설명한다. 구현·빌드·화면 성공은 이 설계 과제의 완료로 추측하지 않는다."
    ],
    "sources": [
      {
        "title": "Khronos Simple Engine — Architectural Patterns: 책임 분리",
        "url": "https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/Engine_Architecture/02_architectural_patterns.html"
      },
      {
        "title": "Khronos Simple Engine — Resource Management: Handles / Basic Resource Manager",
        "url": "https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/Engine_Architecture/04_resource_management.html"
      }
    ],
    "question": "ResourceManager가 CPU 객체를 소유하고 Renderer가 VkBuffer나 descriptor를 참조할 때, 언제 GPU 자원을 안전하게 해제할 수 있는가?",
    "next": "설계가 확인된 뒤 ResourceManager에서 한 종류의 리소스만 동기 생성·로딩·캐싱하는 작은 사용자 구현 과제로 진행한다. glTF는 리소스 관리 기반 뒤에 재개한다.",
    "duration_minutes": 120,
    "minimum_minutes": 60,
    "short_session_goal": "Engine/Renderer/ResourceManager의 책임과 GPU 종료 순서만 한 페이지로 정리한다.",
    "reduced": {
      "title": "Renderer·ResourceManager 소유권 한 페이지 설계",
      "goal": "Engine/Renderer/ResourceManager의 책임과 CPU/GPU 수명 차이를 작은 범위로 정리한다.",
      "first_20_minutes": [
        "공식 Architectural Patterns의 책임 분리 부분과 현재 Engine/Renderer를 읽는다.",
        "생성/해제 주체와 GPU 사용 완료 시점을 표시한다."
      ],
      "done": [
        "세 클래스의 책임·소유권 표와 종료 순서를 적는다.",
        "CPU 참조 해제가 GPU 완료와 같지 않은 이유를 한 예로 설명한다."
      ],
      "sources": [
        {
          "title": "Khronos Simple Engine — Architectural Patterns",
          "url": "https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/Engine_Architecture/02_architectural_patterns.html"
        },
        {
          "title": "Khronos Simple Engine — Resource Management",
          "url": "https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/Engine_Architecture/04_resource_management.html"
        }
      ],
      "question": "누가 GPU 자원을 소유하고 어떤 완료 증거 뒤에 파괴하는가?",
      "next": "남은 리소스 종류의 책임을 확인한 뒤 작은 캐시 구현 과제를 선정한다.",
      "duration_minutes": 60,
      "minimum_minutes": 60,
      "short_session_goal": "이 설계 노트가 최소 검증 단위다."
    },
    "calendar_title": "엔진 구조 · 책임과 리소스 소유권 설계",
    "calendar_brief": {
      "goal": "현재 코드와 CMake를 바탕으로 Renderer·ResourceManager의 책임 및 CPU/GPU 자원 소유권을 설계한다.",
      "first_step": "0~10분: 공식 Architecture와 Engine/Renderer 소유 객체 확인. 10~20분: Resource Handles/Manager를 읽고 현재 초안과 비교.",
      "done": "책임·소유권 표, 유지/수정/보류 목록, 생성·해제 순서를 개인 노트에 기록한다. 코드 구현은 사용자가 직접 진행한다."
    },
    "planned_date": "2026-10-20"
  },
  "deferred_tasks": [
    {
      "id": "gltf-v3-metadata",
      "status": "deferred",
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
      "planned_date": "2026-10-19",
      "defer_reason": "2026-10-15 사용자 방향 변경: 엔진 아키텍처와 리소스 관리 기반 이후 재개. 완료로 처리하지 않음."
    }
  ],
  "learning_direction": [
    "engine_architecture",
    "resource_management",
    "model_loading_tinygltf_v3",
    "forward_pbr_metallic_roughness",
    "toon_deferred_bindless_extensions"
  ]
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

- 대상 학습일: 2026-10-20
- 모드: ready_for_connector · 동작: update
- 시간: 2026-10-20T10:00:00+09:00 ~ 2026-10-20T12:00:00+09:00

오늘의 목표: 현재 코드와 CMake를 바탕으로 Renderer·ResourceManager의 책임 및 CPU/GPU 자원 소유권을 설계한다.

첫 20분 행동: 0~10분: 공식 Architecture와 Engine/Renderer 소유 객체 확인. 10~20분: Resource Handles/Manager를 읽고 현재 초안과 비교.

완료 기준: 책임·소유권 표, 유지/수정/보류 목록, 생성·해제 순서를 개인 노트에 기록한다. 코드 구현은 사용자가 직접 진행한다.

학습 자료
Khronos Simple Engine — Architectural Patterns: 책임 분리
https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/Engine_Architecture/02_architectural_patterns.html
Khronos Simple Engine — Resource Management: Handles / Basic Resource Manager
https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/Engine_Architecture/04_resource_management.html

이해 확인 질문
ResourceManager가 CPU 객체를 소유하고 Renderer가 VkBuffer나 descriptor를 참조할 때, 언제 GPU 자원을 안전하게 해제할 수 있는가?

다음 단계
설계가 확인된 뒤 ResourceManager에서 한 종류의 리소스만 동기 생성·로딩·캐싱하는 작은 사용자 구현 과제로 진행한다. glTF는 리소스 관리 기반 뒤에 재개한다.

[DaylightEngine-study:v1:2026-10-20:vulkan]

--- 자동 계획 끝 ---


<!-- calendar-draft:end -->
