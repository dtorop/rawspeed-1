add_custom_target(tests ALL)

FILE(GLOB RAWSPEED_TEST_SOURCES
  "RawSpeed.cpp"
)

if(CMAKE_BUILD_TYPE MATCHES "^[Cc][Oo][Vv][Ee][Rr][Aa][Gg][Ee]$")
  # want all the symbols.
  add_library(rawspeed_test SHARED $<TARGET_OBJECTS:rawspeed> ${RAWSPEED_TEST_SOURCES})
else()
  add_library(rawspeed_test STATIC $<TARGET_OBJECTS:rawspeed> ${RAWSPEED_TEST_SOURCES})
endif()

target_link_libraries(rawspeed_test PUBLIC ${RAWSPEED_LIBS})
target_compile_definitions(rawspeed_test PUBLIC "$<TARGET_PROPERTY:rawspeed,COMPILE_DEFINITIONS>")
target_include_directories(rawspeed_test PUBLIC "$<TARGET_PROPERTY:rawspeed,INCLUDE_DIRECTORIES>")
add_dependencies(rawspeed_test rawspeed)

target_link_libraries(rawspeed_test PUBLIC gtest gmock_main)

if(WITH_OPENMP AND OPENMP_FOUND)
  target_link_libraries(rawspeed_test INTERFACE ${OpenMP_CXX_FLAGS})
endif()

FILE(GLOB RAWSPEED_TESTS_SOURCES
  "../common/CommonTest.cpp"
  "../common/MemoryTest.cpp"
  "../common/PointTest.cpp"
  "../io/EndiannessTest.cpp"
  "../metadata/BlackAreaTest.cpp"
  "../metadata/CameraMetaDataTest.cpp"
  "../metadata/CameraSensorInfoTest.cpp"
  "../metadata/CameraTest.cpp"
  "../metadata/ColorFilterArrayTest.cpp"
  "ExceptionsTest.cpp"
)

CHECK_CXX_COMPILER_FLAG_AND_ENABLE_IT(-Wno-suggest-attribute=const)
CHECK_CXX_COMPILER_FLAG_AND_ENABLE_IT(-Wno-suggest-override)

 # should be < 64Kb
math(EXPR MAX_MEANINGFUL_SIZE 4*1024)
CHECK_CXX_COMPILER_FLAG_AND_ENABLE_IT(-Wstack-usage=${MAX_MEANINGFUL_SIZE})

if(NOT (APPLE AND (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang") AND CMAKE_BUILD_TYPE MATCHES "^[Cc][Oo][Vv][Ee][Rr][Aa][Gg][Ee]$"))
  # Apple XCode seems to generate HUGE stack/frames, much bigger than anything else.
  CHECK_CXX_COMPILER_FLAG_AND_ENABLE_IT(-Wframe-larger-than=${MAX_MEANINGFUL_SIZE})
endif()

# as small as possible, but 1Mb+ is ok.
math(EXPR MAX_MEANINGFUL_SIZE 32*1024)
CHECK_CXX_COMPILER_FLAG_AND_ENABLE_IT(-Wlarger-than=${MAX_MEANINGFUL_SIZE})

set(CMAKE_CXX_CLANG_TIDY_SAVE "${CMAKE_CXX_CLANG_TIDY}")

unset(CMAKE_CXX_CLANG_TIDY)

foreach(IN ${RAWSPEED_TESTS_SOURCES})
  get_filename_component(TESTNAME ${IN} NAME_WE)
  add_executable(${TESTNAME} ${IN})
  target_link_libraries(${TESTNAME} rawspeed_test)
  # GTEST_ADD_TESTS(${TESTNAME} "" AUTO)
  add_test(NAME ${TESTNAME} COMMAND ${TESTNAME})
  add_dependencies(tests ${TESTNAME})
endforeach()

set(CMAKE_CXX_CLANG_TIDY "${CMAKE_CXX_CLANG_TIDY_SAVE}")
