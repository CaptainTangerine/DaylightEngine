cmake_minimum_required(VERSION 3.25)

project(DaylightEngine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED COMPONENTS dxc)
find_package(SDL3 CONFIG REQUIRED COMPONENTS SDL3-shared
    HINTS "$ENV{VULKAN_SDK}/cmake"
)

add_executable(Daylight)

file(GLOB_RECURSE DAYLIGHT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Daylight/Source/*.cpp"
)

target_sources(Daylight PRIVATE
    ${DAYLIGHT_SOURCES}
    External/SimpleMath/SimpleMath.cpp
)

# 컴파일러가 헤더 파일을 찾을 경로 설정
target_include_directories(Daylight PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Daylight/Source"
    "${CMAKE_CURRENT_SOURCE_DIR}/External/DirectXMath/Inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/External/SimpleMath"
)

# 라이브러리 타깃 연결
target_link_libraries(Daylight PRIVATE
    Vulkan::Headers
    Vulkan::dxc_lib
    SDL3::SDL3-shared
)

# 전처리기 정의
target_compile_definitions(Daylight PRIVATE
    NOMINMAX
    VK_NO_PROTOTYPES
    VK_USE_PLATFORM_WIN32_KHR
    _CONSOLE
    UNICODE
    _UNICODE

    "$<$<CONFIG:Debug>:_DEBUG>"
    "$<$<CONFIG:Release>:NDEBUG>"
)

# 기존 MSVC 경고 수준 / 코드 검사 / 런타임, 디버깅, 최적화 설정
if(MSVC)
    set_target_properties(Daylight PROPERTIES
        MSVC_RUNTIME_LIBRARY
            "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"

        MSVC_DEBUG_INFORMATION_FORMAT
            "$<IF:$<CONFIG:Debug>,EditAndContinue,ProgramDatabase>"
    )

    target_compile_options(Daylight PRIVATE
        /W3
        /sdl
        /permissive-

        "$<$<CONFIG:Release>:/Gy>"
        "$<$<CONFIG:Release>:/Oi>"
        "$<$<CONFIG:Release>:/GL>"
    )

    target_link_options(Daylight PRIVATE
        "$<$<CONFIG:Release>:/DEBUG>"
        "$<$<CONFIG:Release>:/LTCG:INCREMENTAL>"
    )

endif()

# pch 설정
target_precompile_headers(Daylight PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Daylight/Source/pch.h"
)

set_source_files_properties(
    External/SimpleMath/SimpleMath.cpp
    PROPERTIES
        SKIP_PRECOMPILE_HEADERS ON
)


get_filename_component(DAYLIGHT_VULKAN_SDK_DIR
    "${Vulkan_INCLUDE_DIR}" DIRECTORY
)

# 컴파일 이외 작업 등록
# 셰이더 파일 실행 파일 옆으로 복사
add_custom_target(DaylightRuntimeFiles
    COMMAND "${CMAKE_COMMAND}" -E make_directory
        "$<TARGET_FILE_DIR:Daylight>"

    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "$<TARGET_FILE:SDL3::SDL3-shared>"
        "${DAYLIGHT_VULKAN_SDK_DIR}/Bin/dxcompiler.dll"
        "$<TARGET_FILE_DIR:Daylight>"

    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Daylight/Shaders"
        "$<TARGET_FILE_DIR:Daylight>/Shaders"

    COMMENT "Copying runtime DLLs and shaders"
    VERBATIM
)

add_dependencies(Daylight DaylightRuntimeFiles)
