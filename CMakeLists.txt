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

# Check if running in a conda environment
if(DEFINED ENV{CONDA_PREFIX})
    message(STATUS "Running in a conda environment: $ENV{CONDA_PREFIX}")
    
    # Explicitly set library paths to prefer system libraries over conda ones
    list(APPEND CMAKE_PREFIX_PATH "/usr/lib" "/usr/local/lib")
    
    # Force system OpenSSL over conda's OpenSSL
    set(OPENSSL_ROOT_DIR "/usr/lib/ssl" "/usr/local/ssl" "/usr/local/openssl")
else()
    message(STATUS "Not running in a conda environment")
endif()

# Dependencies - modified to be more specific
find_package(OpenSSL REQUIRED)
find_package(Boost 1.70.0 REQUIRED COMPONENTS system)
find_package(Protobuf REQUIRED)

# Find libsodium using pkg-config
find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(SODIUM libsodium)
endif()

# If pkg-config failed, try direct approach
if(NOT SODIUM_FOUND)
    # Look for include files
    find_path(SODIUM_INCLUDE_DIR 
        NAMES sodium.h
        PATH_SUFFIXES include sodium
        PATHS
            /usr/local
            /usr
            /opt/local
            /opt
            $ENV{CONDA_PREFIX}
    )
    
    # Look for library
    find_library(SODIUM_LIBRARIES
        NAMES sodium libsodium
        PATH_SUFFIXES lib lib64
        PATHS
            /usr/local
            /usr
            /opt/local
            /opt
            $ENV{CONDA_PREFIX}
    )
    
    # Set as found if both include directory and library were found
    if(SODIUM_INCLUDE_DIR AND SODIUM_LIBRARIES)
        set(SODIUM_FOUND TRUE)
    endif()
    
    # Display status
    if(SODIUM_FOUND)
        message(STATUS "Found libsodium: ${SODIUM_LIBRARIES}")
        message(STATUS "Found libsodium include: ${SODIUM_INCLUDE_DIR}")
    else()
        message(FATAL_ERROR "libsodium not found. Please install libsodium development package.")
    endif()
endif()

# Print OpenSSL information for debugging
message(STATUS "OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OpenSSL libraries: ${OPENSSL_LIBRARIES}")

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OPENSSL_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${Protobuf_INCLUDE_DIRS}
    ${SODIUM_INCLUDE_DIR}
)

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Define the executable
add_executable(linknet ${SOURCES})

# Link libraries
target_link_libraries(linknet 
    ${OPENSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${Protobuf_LIBRARIES}
    ${SODIUM_LIBRARIES}
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
