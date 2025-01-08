function(add_test_og TARGET_NAME TARGET_PATH)
  cmake_minimum_required(VERSION 3.22)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

  file(GLOB_RECURSE TESTS "${TARGET_PATH}/${TARGET_NAME}/*.cpp")

  add_executable(${TARGET_NAME} ${TESTS})
  target_link_libraries(${TARGET_NAME} PRIVATE ${APP_LIBRARY_NAME} Catch2::Catch2WithMain)
  if(STATIC)
    target_link_options(${TARGET_NAME} PRIVATE -static -fstack-protector)
    target_link_libraries(${TARGET_NAME} PUBLIC -static-libgcc -static-libstdc++)
  endif()

  target_include_directories(${TARGET_NAME} PUBLIC ${GS2LIB_INCLUDE_DIRECTORY})

  target_include_directories(${TARGET_NAME} PUBLIC ${GS2COMPILER_INCLUDE_DIRECTORY})

  target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/server/include)
  target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/server/include/level)
  target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/server/include/scripting)
  target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/server/include/scripting/v8)
  target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/server/include/scripting/interface)
  target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/server/include/misc)
  target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/server/include/utilities)
  target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/server/include/animation)

  add_dependencies(${TARGET_NAME} ${APP_LIBRARY_NAME})

  if(V8NPCSERVER)
    include_directories(${V8_INCLUDE_DIR})
  endif()

  list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

  include(CTest)
  include(Catch)
  catch_discover_tests(${TARGET_NAME})

  message(STATUS "Added test ${TARGET_NAME}")
endfunction()
