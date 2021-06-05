cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(dCRDT CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# need for clangd.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# install module that speeds up the build process.
set(COTIRE_DOWNLOAD_VERSION 1.8.1)
set(COTIRE_DOWNLOAD_LOCATION
    "${CMAKE_BINARY_DIR}/cmake/COTIRE_${COTIRE_DOWNLOAD_VERSION}.cmake")

if(NOT (EXISTS ${COTIRE_DOWNLOAD_LOCATION}))
  message(STATUS "Downloading contire.cmake")
  file(
    DOWNLOAD
    https://github.com/sakra/cotire/releases/download/cotire-${COTIRE_DOWNLOAD_VERSION}/cotire.cmake
    ${COTIRE_DOWNLOAD_LOCATION})
endif()

include(${COTIRE_DOWNLOAD_LOCATION})

include(FetchContent)

FetchContent_Declare(
  rapidcheck
  GIT_REPOSITORY https://github.com/emil-e/rapidcheck
  GIT_TAG master)

FetchContent_MakeAvailable(rapidcheck)

# install coverage module.
set(COVERAGE_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CodeCoverage.cmake")

if(NOT (EXISTS ${COVERAGE_DOWNLOAD_LOCATION}))
  message(STATUS "Downloading CodeCoverage.cmake")
  file(
    DOWNLOAD
    https://raw.githubusercontent.com/bilke/cmake-modules/master/CodeCoverage.cmake
    ${COVERAGE_DOWNLOAD_LOCATION})
endif()

include(${COVERAGE_DOWNLOAD_LOCATION})
append_coverage_compiler_flags()

include(CTest)
enable_testing()

add_subdirectory(include)
