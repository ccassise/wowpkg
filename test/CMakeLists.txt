set(
	TESTS

	addon
	appstate
	command
	config
	list
	osapi
	zipper
)

foreach(TEST IN LISTS TESTS)
	set(TEST_NAME ${TEST}_test)
	add_executable(${TEST_NAME} ${TEST_NAME}.c ${SRC_FILES})

	target_link_libraries(${TEST_NAME} PRIVATE ${WOWPKG_LIBS})

	target_compile_definitions(${TEST_NAME} PRIVATE
		${WOWPKG_DEFINES}
		WOWPKG_TEST_TMPDIR="${PROJECT_SOURCE_DIR}/dev_only/tmp/"
		WOWPKG_TEST_MOCKSDIR="${PROJECT_SOURCE_DIR}/test/mocks/"
	)
	target_compile_options(${TEST_NAME} PRIVATE ${WFLAGS})
	target_link_options(${TEST_NAME} PRIVATE ${LDFLAGS})

	target_include_directories(${TEST_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/src)

	add_test(${TEST_NAME} ${TEST_NAME})
endforeach()
