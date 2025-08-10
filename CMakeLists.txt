cmake_minimum_required(VERSION 3.10)
project(tellydb C)

set(CMAKE_EXPORT_COMPILE_COMMANDS yes)
set(CMAKE_BUILD_TYPE Release)

find_package(OpenSSL REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

file(GLOB SOURCES "src/**/**/*.c" "src/**/*.c" "src/*.c")
list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/src/utils/memcpy_aligned.c")
add_executable(telly ${SOURCES})
target_sources(telly PRIVATE src/utils/memcpy_aligned.c)

include(CheckCCompilerFlag)
check_c_compiler_flag(-mavx2 COMPILER_SUPPORTS_AVX2)
check_c_compiler_flag(-msse2 COMPILER_SUPPORTS_SSE2)

if(COMPILER_SUPPORTS_AVX2)
  set_source_files_properties(src/utils/memcpy_aligned.c PROPERTIES COMPILE_OPTIONS "-mavx2")
elseif(COMPILER_SUPPORTS_SSE2)
  set_source_files_properties(src/utils/memcpy_aligned.c PROPERTIES COMPILE_OPTIONS "-msse2")
endif()

execute_process(
  COMMAND git rev-parse HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND git describe --abbrev=0 --tags
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

target_include_directories(telly PUBLIC ${CMAKE_SOURCE_DIR}/headers)

target_link_libraries(telly
  OpenSSL::SSL
  OpenSSL::Crypto
  Threads::Threads
  m
)

target_compile_definitions(telly
  PUBLIC GIT_HASH="${GIT_HASH}"
  PUBLIC VERSION="${GIT_VERSION}"
  PUBLIC _TIME_BITS=64
  PUBLIC _GNU_SOURCE
  PUBLIC _FILE_OFFSET_BITS=64
)

install(TARGETS telly)

add_executable(tests utils/tests.c)
target_link_libraries(tests
  Threads::Threads
  hiredis
)

add_executable(benchmark utils/benchmark.c)
target_link_libraries(benchmark
  Threads::Threads
  hiredis
)
