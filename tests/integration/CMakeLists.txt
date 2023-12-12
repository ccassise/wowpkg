set(
	TESTS

    "integration"
)

foreach(TEST IN LISTS TESTS)
	set(TEST_NAME test_${TEST})
	add_executable(${TEST_NAME} ${TEST_NAME}.c ${SRC_FILES})

	target_link_libraries(${TEST_NAME} PRIVATE ${WOWPKG_LIBS})

	target_compile_options(${TEST_NAME} PRIVATE ${WFLAGS})
	target_link_options(${TEST_NAME} PRIVATE ${LDFLAGS})
	set_target_properties(${TEST_NAME} PROPERTIES C_STANDARD ${WOWPKG_C_STANDARD})
	target_compile_definitions(${TEST_NAME} PRIVATE
		${WOWPKG_DEFINES}
		# WOWPKG_TEST_TMPDIR="${PROJECT_SOURCE_DIR}/dev_only/tmp/"
		# WOWPKG_TEST_DIR="${PROJECT_SOURCE_DIR}/tests"
		WOWPKG_EXEC_PATH="${CMAKE_BINARY_DIR}/src/wowpkg"
	)

	target_include_directories(${TEST_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/src)

	add_test(${TEST_NAME} ${TEST_NAME})
endforeach()
