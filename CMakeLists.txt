cmake_minimum_required(VERSION 3.17)

if (NOT DEFINED ${CMAKE_TOOLCHAIN_FILE})
    set(
        CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file"
    )
endif()

project(
    wowpkg
    VERSION 0.3.3
    DESCRIPTION "A CLI World of Warcraft addon manager"
    LANGUAGES C
)

set(WOWPKG_C_STANDARD 11)

option(WOWPKG_ENABLE_SANITIZERS "Build with or without sanitizers" OFF)
option(WOWPKG_ENABLE_TESTS "Build tests" OFF)
option(WOWPKG_ENABLE_INTEGRATION_TESTS "Build integration tests" OFF)
option(WOWPKG_USE_DEVELOPMENT_PATHS "Determines what paths will be used to find some files" OFF)

# Compiler flags that enable warnings.
set(WFLAGS) 

# Mainly used for Windows. Adds extra optimization flags when building for
# release builds. These flags get added to WFLAGS.
set(WFLAGS_RELEASE)

# Linker flags.
set(LDFLAGS)

# Mainly used for Windows. Adds extra optimization linker flags for release
# builds. These flags get added to LDFLAGS.
set(LDFLAGS_RELEASE)

# Sanitizer flags.
set(SANSFLAGS)

# Path to catalog directory in project.
set(WOWPKG_CATALOG_PATH)

# Includes all compile time defines.
set(WOWPKG_DEFINES)

# List of libraries that will be linked to targets.
set(WOWPKG_LIBS)

# Path to user files for project. This location is where the program will search
# for user config files and is where it will save user specific data.
set(WOWPKG_USER_FILE_DIR)

if (WOWPKG_ENABLE_TESTS AND NOT WOWPKG_USE_DEVELOPMENT_PATHS)
    message(WARNING "[${PROJECT_NAME}] enabling tests without using development paths may cause tests to fail")
endif()

if (WOWPKG_ENABLE_TESTS)
    include(CTest)
    enable_testing()
endif()

find_package(CURL REQUIRED)
find_package(cJSON CONFIG REQUIRED)
find_package(LibArchive REQUIRED)

set(WOWPKG_LIBS CURL::libcurl cjson LibArchive::LibArchive)

if (MSVC)
    # CMake does not set proper release flags for MSVC.
    set(WFLAGS_RELEASE /O2 /GL /DNDEBUG /Zi /Gy)
    set(WFLAGS /W4 "$<$<CONFIG:Release>:${WFLAGS_RELEASE}>")

    set(LDFLAGS_RELEASE /DEBUG /INCREMENTAL:NO /OPT:REF /OPT:ICF)
    set(LDFLAGS "$<$<CONFIG:Release>:${LDFLAGS_RELEASE}>")

    # A lot of the functions suggested by this are not available on other
    # systems -- so just ignore the warnings.
    set(WOWPKG_DEFINES _CRT_SECURE_NO_WARNINGS)
else()
    set(
        WFLAGS

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
        set(
            SANFLAGS

            -fsanitize=address
            -fsanitize=undefined
            -fsanitize=float-divide-by-zero
            -fsanitize=float-cast-overflow
            -fno-sanitize-recover=all
            -fno-sanitize=null
            -fno-sanitize=alignment
        )

        set (WFLAGS ${WFLAGS} ${SANFLAGS})

        if (APPLE)
            set(LDFLAGS ${SANFLAGS} -static-libsan)
        else()
            set(LDFLAGS ${SANFLAGS} -static-libasan)
        endif()
    endif()
endif()

set(
    SRC_FILES

    ${PROJECT_SOURCE_DIR}/src/addon.c
    ${PROJECT_SOURCE_DIR}/src/appstate.c
    ${PROJECT_SOURCE_DIR}/src/command.c
    ${PROJECT_SOURCE_DIR}/src/config.c
    ${PROJECT_SOURCE_DIR}/src/ini.c
    ${PROJECT_SOURCE_DIR}/src/list.c
    ${PROJECT_SOURCE_DIR}/src/osapi.c
    ${PROJECT_SOURCE_DIR}/src/zipper.c
)

list(APPEND WOWPKG_DEFINES WOWPKG_VERSION="${PROJECT_VERSION}")

if(WOWPKG_USE_DEVELOPMENT_PATHS)
    message(STATUS "[${PROJECT_NAME}] building with development paths")
    list(APPEND WOWPKG_DEFINES
        WOWPKG_USER_FILE_DIR="${PROJECT_SOURCE_DIR}/dev_only"
        WOWPKG_CATALOG_PATH="${PROJECT_SOURCE_DIR}/catalog"
    )
endif()

add_subdirectory(src)

if (WOWPKG_ENABLE_TESTS)
    message(STATUS "[${PROJECT_NAME}] enabling tests")
    add_subdirectory(tests)

    if (WOWPKG_ENABLE_INTEGRATION_TESTS)
        add_subdirectory(tests/integration)
    endif()
endif()

if (APPLE)
    # Assume if Apple that DragNDrop generator is being used. This generator will
    # be setup so that there is a single directory that can be dragged to the
    # Applications directory.
    install(
        TARGETS wowpkg
        CONFIGURATIONS Release
        RUNTIME DESTINATION ${PROJECT_NAME}/bin
    )

    install(DIRECTORY catalog DESTINATION ${PROJECT_NAME})
else()
    install(
        TARGETS wowpkg
        CONFIGURATIONS Release
        RUNTIME DESTINATION bin
    )

    install(DIRECTORY catalog DESTINATION .)
endif()

if (WIN32)
    install(FILES ${PROJECT_SOURCE_DIR}/build/src/Release/cjson.dll DESTINATION bin)
    install(FILES ${PROJECT_SOURCE_DIR}/build/src/Release/libcurl.dll DESTINATION bin)
    install(FILES ${PROJECT_SOURCE_DIR}/build/src/Release/zlib1.dll DESTINATION bin)
endif()

set(CPACK_VERBATIM_VARIABLES true)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

set(CPACK_PACKAGE_VENDOR ${PROJECT_NAME})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${PROJECT_NAME})

set(CPACK_RESOURCE_FILE_LICENSE ${PROJECT_SOURCE_DIR}/LICENSE)
set(CPACK_RESOURCE_FILE_README ${PROJECT_SOURCE_DIR}/README.md)

set(CPACK_MONOLITHIC_INSTALL true)

include(CPack)
