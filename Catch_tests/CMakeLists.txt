cmake_minimum_required(VERSION 3.22)

if(TESTS)
	include(utility)

	find_package(Catch2 CONFIG REQUIRED)

	subdir_list(SUBDIRS ${CMAKE_CURRENT_SOURCE_DIR})

	foreach(subdir ${SUBDIRS})
		add_test_og(${subdir} ${CMAKE_CURRENT_SOURCE_DIR})
	endforeach()
endif()