# Daylight Engine

C++와 Vulkan으로 렌더링 엔진을 직접 구현하며 학습하는 프로젝트입니다.

그래픽스 API의 동작과 렌더링 구조를 이해하는 것을 목표로, 삼각형 출력부터 단계적으로 기능을 확장하고 있습니다.

## 개발 환경

- **언어:** C++17
- **그래픽스 API:** Vulkan 1.4
- **윈도우 및 이벤트 처리:** SDL3
- **개발 도구:** Visual Studio 2022 / Windows x64

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
│  └─ Source/
│     ├─ Core/
│     ├─ Graphics/
│     │  └─ Vulkan/
│     ├─ Math/
│     └─ Rendering/
├─ External/
│  ├─ DirectXMath/
│  └─ SimpleMath/
└─ DaylightEngine.sln
```

## 수학 규칙

- 왼손 좌표계, Y-Up
- 행벡터 방식
- Vulkan viewport에서 Y 방향 반전 처리
