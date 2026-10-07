# Daylight Engine

C++와 Vulkan으로 렌더링 엔진을 직접 구현하며 학습하는 프로젝트입니다.

그래픽스 API의 동작과 렌더링 구조를 이해하는 것을 목표로, 삼각형 출력부터 단계적으로 기능을 확장하고 있습니다.

## 개발 환경

- **언어:** C++17
- **그래픽스 API:** Vulkan 1.4
- **윈도우 및 이벤트 처리:** SDL3
- **개발 도구:** Visual Studio 2022 / Windows x64
- **빌드 시스템:** CMake 3.25 이상 / Ninja / MSVC

## 빌드 및 실행

Visual Studio의 **파일 → 열기 → 폴더**에서 저장소 루트 폴더를 엽니다.
`CMakeLists.txt`와 `CMakePresets.json`이 들어 있는 폴더가 기준입니다.

필요한 설치 항목은 Visual Studio 2022의 C++ 데스크톱 개발 및 Windows용 C++ CMake 도구와 Vulkan SDK입니다.
`VULKAN_SDK` 환경 변수는 설치된 SDK 폴더를 가리켜야 합니다.
Vulkan 헤더, SDL3, DXC는 해당 SDK에서 찾고, volk와 VMA도 SDK 헤더를 사용합니다.
volk와 VMA 구현은 `VulkanDevice.cpp`에서 기존 구현 매크로로 한 번 컴파일합니다.
DirectXMath와 SimpleMath는 `External` 폴더의 파일을 사용합니다.

1. 상단 구성 선택 목록에서 `Windows x64 Debug` 또는 `Windows x64 Release`를 선택합니다.
2. CMake 구성이 완료되면 **빌드 → 모두 빌드**를 실행합니다.
3. 선택한 구성에 해당하는 `out/build/windows-debug/Daylight.exe` 또는 `out/build/windows-release/Daylight.exe`를 실행합니다.

빌드할 때 SDL3와 DXC의 DLL, `Shaders` 폴더가 실행 파일 옆으로 복사됩니다.
셰이더 경로는 실행 파일의 위치를 기준으로 찾습니다.
`out/build`는 자동 생성되는 빌드 결과이며 Git에서 제외합니다.

빌드 설정은 `CMakeLists.txt`에서, 구성별 설정과 출력 폴더는 `CMakePresets.json`에서 관리합니다.

## 사용 라이브러리

| 라이브러리 | 용도 |
|---|---|
| Volk | Vulkan 함수 로딩 |
| Vulkan Memory Allocator | GPU 메모리 할당 및 관리 |
| DirectXMath / SimpleMath | 벡터·행렬·쿼터니언 연산 |

## 프로젝트 구조

```text
DaylightEngine/
├─ Daylight/
│  ├─ Shaders/
│  └─ Source/
│     ├─ Core/
│     ├─ Graphics/
│     │  └─ Vulkan/
│     ├─ Math/
│     └─ Rendering/
├─ External/
│  ├─ DirectXMath/
│  └─ SimpleMath/
├─ CMakeLists.txt
└─ CMakePresets.json
```

## 수학 규칙

- 왼손 좌표계, Y-Up
- 행벡터 방식
- Vulkan viewport에서 Y 방향 반전 처리
