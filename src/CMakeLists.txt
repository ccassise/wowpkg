add_executable(wowpkg main.c ${SRC_FILES})

target_link_libraries(wowpkg PRIVATE ${WOWPKG_LIBS})

target_compile_options(wowpkg PRIVATE ${WFLAGS})
target_link_options(wowpkg PRIVATE ${LDFLAGS})
target_compile_definitions(wowpkg PRIVATE ${WOWPKG_DEFINES})
