cmake_minimum_required(VERSION 3.14)
project(FuzzyMatcher)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /utf-8 /EHsc")
endif()

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_library(${PROJECT_NAME} STATIC
    src/FuzzyMatcher.cpp
)
target_include_directories(${PROJECT_NAME} PUBLIC src)


enable_testing()
add_executable(${PROJECT_NAME}-test
    test/test_FuzzyMatcher.cpp
)
target_include_directories(${PROJECT_NAME}-test PRIVATE
    ${gtest_SOURCE_DIR}/include
    ${gtest_SOURCE_DIR}
)
target_link_libraries(${PROJECT_NAME}-test PRIVATE
    ${PROJECT_NAME}
    GTest::gtest
    GTest::gtest_main
)
include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}-test DISCOVERY_MODE PRE_TEST)

