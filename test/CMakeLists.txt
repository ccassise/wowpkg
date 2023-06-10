set(
	tests

	addon
	app_state
	command
	config
	list
	osapi
	zipper
)

foreach(test IN LISTS tests)
	set(test_name ${test}_test)
	add_executable(${test_name} ${test_name}.c ${src_files})

	target_compile_options(${test_name} PRIVATE ${cflags})
	target_compile_definitions(${test_name} PRIVATE ${cdefs})

	if (WIN32)
		target_link_libraries(${test_name} PRIVATE unofficial::minizip::minizip CURL::libcurl cjson)
	else()
		target_link_libraries(${test_name} PRIVATE minizip::minizip CURL::libcurl cjson)
	endif()

	target_include_directories(${test_name} PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/lib)

	add_test(${test_name} ${test_name})
endforeach()
