add_executable(wowpkg main.c ${src_files})

target_link_libraries(wowpkg PRIVATE unofficial::minizip::minizip CURL::libcurl cjson)

target_compile_options(wowpkg PRIVATE ${cflags})

target_link_options(wowpkg PRIVATE ${ldflags})

target_compile_definitions(wowpkg PRIVATE ${cdefs})
