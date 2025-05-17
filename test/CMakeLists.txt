# Find GTest package
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Test directories
file(GLOB_RECURSE TEST_SOURCES "*.cpp")

# Create test executables
add_executable(linknet_tests ${TEST_SOURCES})

# Link against GTest and our library
target_link_libraries(linknet_tests 
    ${GTEST_LIBRARIES} 
    ${OPENSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${Protobuf_LIBRARIES}
    sodium
    pthread
)

# Add tests to CTest
add_test(NAME AllTests COMMAND linknet_tests)
