cmake_minimum_required(VERSION 3.16)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "Vcpkg toolchain file")

project(wowpkg VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)

set(WOWPKG_ENABLE_SANITIZERS ON)
set(WOWPKG_ENABLE_TESTS ON)

# Disable tests on release builds because NDEBUG is most likely set.
# With NDEBUG set tests will have weird results or fail to compile since
# 'assert' will be turned in to a no op.
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(WOWPKG_ENABLE_TESTS OFF)
endif()

if (WOWPKG_ENABLE_TESTS)
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

if (WIN32)
    set(WOWPKG_LIBS unofficial::minizip::minizip CURL::libcurl cjson)
else()
    set(WOWPKG_LIBS minizip::minizip CURL::libcurl cjson)
endif()


set(SRC_FILES
    ${PROJECT_SOURCE_DIR}/src/addon.c
    ${PROJECT_SOURCE_DIR}/src/appstate.c
    ${PROJECT_SOURCE_DIR}/src/command.c
    ${PROJECT_SOURCE_DIR}/src/config.c
    ${PROJECT_SOURCE_DIR}/src/list.c
    ${PROJECT_SOURCE_DIR}/src/osapi.c
    ${PROJECT_SOURCE_DIR}/src/zipper.c
)

if (MSVC)
    set(WFLAGS_RELEASE /O2 /GL /DNDEBUG /Zi /Gy)
    set(WFLAGS /W4 "$<$<CONFIG:Release>:${WFLAGS_RELEASE}>")

    set(LDFLAGS_RELEASE /DEBUG /INCREMENTAL:NO /OPT:REF /OPT:ICF)
    set(LDFLAGS "$<$<CONFIG:Release>:${LDFLAGS_RELEASE}>")

    set(WOWPKG_DEFINES _CRT_SECURE_NO_WARNINGS)
else()
    set (WFLAGS
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
        -Wstrict-aliasing
    )
endif()

if (WOWPKG_ENABLE_SANITIZERS)
    message(STATUS "[${PROJECT_NAME}] enabling sanitizers")
    if (MSVC)
        set(SANFLAGS /fsanitize=address)

        set(WFLAGS ${WFLAGS} ${SANFLAGS})
    else()
        set(SANFLAGS
            -fsanitize=address
            -fsanitize=undefined
            -fno-sanitize-recover=all
            -fsanitize=float-divide-by-zero
            -fsanitize=float-cast-overflow
            -fno-sanitize=null
            -fno-sanitize=alignment
        )

        set (WFLAGS ${WFLAGS} ${SANFLAGS})

        if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
            set(LDFLAGS ${SANFLAGS} -static-libsan)
        else()
            set(LDFLAGS ${SANFLAGS} -static-libasan)
        endif()
    endif()
endif()

add_subdirectory(src)

if (WOWPKG_ENABLE_TESTS)
    add_subdirectory(test)
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
