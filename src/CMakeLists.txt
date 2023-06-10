add_executable(wowpkg main.c ${src_files})

if (WIN32)
	target_link_libraries(wowpkg PRIVATE unofficial::minizip::minizip CURL::libcurl cjson)
else()
	target_link_libraries(wowpkg PRIVATE minizip::minizip CURL::libcurl cjson)
endif()

target_compile_options(wowpkg PRIVATE ${cflags})

target_link_options(wowpkg PRIVATE ${ldflags})

target_compile_definitions(wowpkg PRIVATE ${cdefs})

target_include_directories(wowpkg PRIVATE ${PROJECT_SOURCE_DIR}/lib)
