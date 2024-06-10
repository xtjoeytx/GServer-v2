cmake_minimum_required(VERSION 3.22)

if(MAKE_TESTS)
	include(FetchContent)
	include(AddTest)
	include(SubdirList)

	FetchContent_Declare(
			Catch2
			GIT_REPOSITORY https://github.com/catchorg/Catch2.git
			GIT_TAG        v3.4.0 # or a later release
	)

	FetchContent_MakeAvailable(Catch2)

	subdir_list(SUBDIRS ${CMAKE_CURRENT_SOURCE_DIR})

	foreach(subdir ${SUBDIRS})
		add_test_og(${subdir} ${CMAKE_CURRENT_SOURCE_DIR})
	endforeach()
endif()