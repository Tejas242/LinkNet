# Find GTest package
find_package(GTest REQUIRED)
message(STATUS "Found GTest include directory: ${GTEST_INCLUDE_DIRS}")
message(STATUS "Found GTest libraries: ${GTEST_LIBRARIES}")

include_directories(${GTEST_INCLUDE_DIRS})

# Get source files, excluding main.cpp
file(GLOB_RECURSE MAIN_SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")
list(FILTER MAIN_SOURCES EXCLUDE REGEX ".*main\\.cpp$")

# Test directories
file(GLOB_RECURSE TEST_SOURCES "*.cpp")

# Create test executables
add_executable(linknet_tests ${TEST_SOURCES} ${MAIN_SOURCES})

# Link against GTest and required libraries
target_link_libraries(linknet_tests 
    ${GTEST_LIBRARIES} 
    ${OPENSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${Protobuf_LIBRARIES}
    ${SODIUM_LIBRARIES}
    pthread
)

# Add tests to CTest
add_test(NAME AllTests COMMAND linknet_tests)
