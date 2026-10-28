# Minimum required version of CMake
cmake_minimum_required(VERSION 3.16)

# Project name
project(ppm)

# Set C++ standard to 11 and make it required
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Discover headers and both libraries using standard prefixes or CMAKE_PREFIX_PATH.
find_path(RDKAFKA_INCLUDE_DIR NAMES librdkafka/rdkafkacpp.h)
find_library(RDKAFKA_LIBRARY NAMES rdkafka)
find_library(RDKAFKA_CPP_LIBRARY NAMES rdkafka++)
if(NOT RDKAFKA_INCLUDE_DIR OR NOT RDKAFKA_LIBRARY OR NOT RDKAFKA_CPP_LIBRARY)
    message(FATAL_ERROR "Install librdkafka development headers and C/C++ libraries, or set CMAKE_PREFIX_PATH to their installation prefix.")
endif()

# Confluent Cloud rejects protocols used by older librdkafka clients.
include(CheckCXXSourceCompiles)
include(CMakePushCheckState)
cmake_push_check_state(RESET)
set(CMAKE_REQUIRED_INCLUDES "${RDKAFKA_INCLUDE_DIR}")
# Each header defines RD_KAFKA_VERSION; check it before the next include.
check_cxx_source_compiles("
#include <librdkafka/rdkafka.h>
#if RD_KAFKA_VERSION < 0x010802ff
#error librdkafka 1.8.2 or later is required
#endif
#include <librdkafka/rdkafkacpp.h>
#if RD_KAFKA_VERSION < 0x010802ff
#error librdkafka C++ headers 1.8.2 or later are required
#endif
int main() { return 0; }
" RDKAFKA_VERSION_SUPPORTED)
cmake_pop_check_state()
if(NOT RDKAFKA_VERSION_SUPPORTED)
    message(FATAL_ERROR "Install librdkafka development headers >= 1.8.2 and matching runtime libraries (required for Confluent Cloud).")
endif()

# Optimization flags
set(CMAKE_CXX_FLAGS "-O3")
set(CMAKE_C_FLAGS "-O3")

# Set options for macOS
if (${APPLE})
    set(CMAKE_CXX_EXTENSIONS OFF)
endif ()

# Add coverage compiler option for GNU C++
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
    set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1)
endif()

# Build the library.
add_subdirectory("cv-lib")

# Include the cvlib headers; defined in cv-lib/CMakeLists.txt
include_directories(${CVLIB_INCLUDE})

# Include application, JSON parser headers.
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/rapidjson")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/spdlog")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/general-redaction")
include_directories("${RDKAFKA_INCLUDE_DIR}")

#### Build target for the PPM
# List all the source files in project
set(SOURCES
    "src/general-redaction/redactionPropertiesManager.cpp"
    "src/general-redaction/rapidjsonRedactor.cpp"
    "src/bsm.cpp"
    "src/bsmHandler.cpp"
    "src/idRedactor.cpp"
    "src/tool.cpp"
    "src/velocityFilter.cpp"
    "src/ppmLogger.cpp"
)

# Create a library target for the shared sources
add_library(ppm-lib STATIC ${SOURCES})

# Link the library target with the Kafka libraries
target_link_libraries(ppm-lib PUBLIC 
    "${RDKAFKA_LIBRARY}"
    "${RDKAFKA_CPP_LIBRARY}"
)

#### Create a target for the PPM executable
add_executable(ppm "src/ppm.cpp")

# Link the PPM executable with the PPM library target
target_link_libraries(ppm PUBLIC ppm-lib CVLib)

#### Create a target for the Kafka consumer executable
add_executable(kafka_consumer "src/kafka_consumer.cpp")

# Link the Kafka consumer executable with the PPM library target and the Kafka libraries
target_link_libraries(kafka_consumer PUBLIC ppm-lib 
    "${RDKAFKA_LIBRARY}"
    "${RDKAFKA_CPP_LIBRARY}"
    CVLib
)

#### Build target for the PPM unit tests and code coverage
set(PPM_TEST_SRC "src/tests.cpp")   # unit tests

# Include the Catch header-only test framework
set(CATCH_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/catch")
add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE ${CATCH_INCLUDE_DIR})

# Build the tests executable
add_executable(ppm_tests ${PPM_TEST_SRC} ${SOURCES})
target_link_libraries(ppm_tests pthread CVLib "${RDKAFKA_CPP_LIBRARY}" Catch)
target_compile_definitions(ppm_tests PRIVATE _PPM_TESTS)

#### Build target for the Kafka test tool
add_subdirectory(kafka-test)

# Copy the data and config directories to the build directory
set(BSM_DATA_DIR $<TARGET_FILE_DIR:ppm>/unit-test-data)
set(BSM_CONFIG_DIR $<TARGET_FILE_DIR:ppm>/config)

# Make the base data directory and copy the data files
add_custom_command(TARGET ppm PRE_BUILD COMMAND ${CMAKE_COMMAND} 
                   -E make_directory ${BSM_DATA_DIR})
add_custom_command(TARGET ppm PRE_BUILD COMMAND ${CMAKE_COMMAND} 
                   -E copy_directory ${PROJECT_SOURCE_DIR}/unit-test-data
                   ${BSM_DATA_DIR})

# Make the base data directory.
add_custom_command(TARGET ppm POST_BUILD COMMAND ${CMAKE_COMMAND} 
                   -E make_directory ${BSM_CONFIG_DIR})
# Copy the data files.
add_custom_command(TARGET ppm POST_BUILD COMMAND echo "Copying the config directory")
add_custom_command(TARGET ppm POST_BUILD COMMAND ${CMAKE_COMMAND} 
                   -E copy_directory ${PROJECT_SOURCE_DIR}/config
                   ${BSM_CONFIG_DIR})
