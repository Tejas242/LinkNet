cmake_minimum_required(VERSION 3.15)
project(LinkNet VERSION 0.1.0 LANGUAGES CXX)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")
endif()

# Dependencies
find_package(OpenSSL REQUIRED)
find_package(Boost 1.70.0 REQUIRED COMPONENTS system)
find_package(Protobuf REQUIRED)
find_package(sodium REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OPENSSL_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${Protobuf_INCLUDE_DIRS}
)

# Add include directory to include path
include_directories(include)

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Define the executable
add_executable(linknet ${SOURCES})

# Link libraries
target_link_libraries(linknet 
    ${OPENSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${Protobuf_LIBRARIES}
    sodium
    pthread
)

# Enable testing
enable_testing()
add_subdirectory(test)

# Install targets
install(TARGETS linknet DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)

# Package configuration
include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
include(CPack)
