cmake_minimum_required(VERSION 3.16)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "Vcpkg toolchain file")

project(wowpkg VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)

set(wowpkg_enable_sanitizers ON)
set(wowpkg_enable_tests ON)

# Disable tests on release builds because NDEBUG is most likely set so they
# won't be able to be ran.
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(wowpkg_enable_tests OFF)
endif()

if (wowpkg_enable_tests)
    include(CTest)
    enable_testing()
endif()

if (WIN32)
    find_package(unofficial-minizip CONFIG REQUIRED)
else()
    find_package(minizip CONFIG REQUIRED)
endif()

find_package(CURL CONFIG REQUIRED)
find_package(cJSON CONFIG REQUIRED)


set(src_files
    ${PROJECT_SOURCE_DIR}/src/addon.c
    ${PROJECT_SOURCE_DIR}/src/appstate.c
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
else()
    set (cflags
        -Wall
        -Wextra
        -Wshadow
        -Wdouble-promotion
        -Wconversion
        -Wpedantic
        -Wcast-align
        -Wstrict-prototypes

        -Wwrite-strings
        -Winit-self
        -Wformat=2
        -Wstrict-overflow=2
        -Wcast-qual
        -Wundef
        -Wswitch-default
        -fstack-protector-strong
        -Wparentheses
        -Wunused-macros
        -Wswitch-enum
    )
endif()

if (wowpkg_enable_sanitizers)
    message(STATUS "[${PROJECT_NAME}] enabling sanitizers")
    if (MSVC)
        set(cflags ${cflags} /fsanitize=address)
    else()
        set (sanflags
            -fsanitize=address
            -fsanitize=undefined
            -fno-sanitize-recover=all
            -fsanitize=float-divide-by-zero
            -fsanitize=float-cast-overflow
            -fno-sanitize=null
            -fno-sanitize=alignment
        )

        set (cflags ${cflags} ${sanflags})

        if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
            set(ldflags ${ldflags} ${sanflags} -static-libsan)
        else()
            set(ldflags ${ldflags} ${sanflags} -static-libasan)
        endif()
    endif()
endif()

add_subdirectory(src)

if (wowpkg_enable_tests)
    add_subdirectory(test)
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
