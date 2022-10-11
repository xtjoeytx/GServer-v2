function(add_test_og TARGET_NAME TARGET_PATH)
  cmake_minimum_required(VERSION 3.22)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

  file(GLOB_RECURSE TESTS "${TARGET_PATH}/${TARGET_NAME}/*.cpp")

  add_executable(${TARGET_NAME} ${TESTS})
  target_link_libraries(${TARGET_NAME} PRIVATE gs2emu_lib Catch2::Catch2WithMain)

  target_include_directories(${TARGET_NAME} PUBLIC ${GS2LIB_INCLUDE_DIRECTORY})

  target_include_directories(${TARGET_NAME} PUBLIC ${GS2COMPILER_INCLUDE_DIRECTORY})

  target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/server/include)
  target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/server/include/TLevel)
  target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/server/include/Scripting)
  target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/server/include/Scripting/v8)
  target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/server/include/Scripting/interface)
  target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/server/include/Misc)
  target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/server/include/utilities)
  target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/server/include/Animation)

  add_dependencies(${TARGET_NAME} gs2emu)

  if(V8NPCSERVER)
    include_directories(${V8_INCLUDE_DIR})
  endif()

  list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

  include(CTest)
  include(Catch)
  catch_discover_tests(${TARGET_NAME})

  message(STATUS "Added test ${TARGET_NAME}")
endfunction()
