cmake_minimum_required(VERSION 3.16)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "Vcpkg toolchain file")
set(VCPKG_TARGET_TRIPLET "x64-windows"
    CACHE STRING "Vcpkg target triplet")

project(wowpkg VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)

set(wowpkg_enable_san ON)
set(wowpkg_enable_tests ON)

if (wowpkg_enable_tests)
    include(CTest)
    enable_testing()
endif()

find_package(unofficial-minizip CONFIG REQUIRED)
find_package(CURL CONFIG REQUIRED)
find_package(cJSON CONFIG REQUIRED)


set(
    src_files
    ${PROJECT_SOURCE_DIR}/src/addon.c
    ${PROJECT_SOURCE_DIR}/src/app_state.c
    ${PROJECT_SOURCE_DIR}/src/command.c
    ${PROJECT_SOURCE_DIR}/src/config.c
    ${PROJECT_SOURCE_DIR}/src/list.c
    ${PROJECT_SOURCE_DIR}/src/osapi.c
    ${PROJECT_SOURCE_DIR}/src/zipper.c
)

if (MSVC)
    set(cflags_release /O2 /GL /DNDEBUG /Zi /Gy)
    set(cflags /W4 "$<$<CONFIG:Release>:${cflags_release}>")

    set(ldflags_release /DEBUG /INCREMENTAL:NO /OPT:REF /OPT:ICF)
    set(ldflags "$<$<CONFIG:Release>:${ldflags_release}>")

    set(cdefs _CRT_SECURE_NO_WARNINGS)
endif()

if (wowpkg_enable_san)
    message(STATUS "[${PROJECT_NAME}] enabling sanitizers")
    set(cflags ${cflags} /fsanitize=address)
endif()

add_subdirectory(src)

if (wowpkg_enable_tests)
    add_subdirectory(test)
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
