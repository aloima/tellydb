cmake_minimum_required(VERSION 3.10)
project(tellydb C)

set(CMAKE_EXPORT_COMPILE_COMMANDS yes)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  include(CheckIPOSupported)
  check_ipo_supported(RESULT supported OUTPUT error)

  if(supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    message(STATUS "LTO/IPO optimization is enabled.")
  else()
    message(WARNING "LTO/IPO optimization is not supported: ${error}")
  endif()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(CMAKE_OSX_ARCHITECTURES ${CMAKE_SYSTEM_PROCESSOR}
    CACHE STRING "Target architecture for macOS"
    FORCE
  )

  list(APPEND CMAKE_PREFIX_PATH 
    "/opt/homebrew" 
    "/usr/local"
  )

  set(MATH_LIB "")
  set(LINK_FLAGS "-Wl-bind_at_load")
else()
  set(MATH_LIB m)
  set(LINK_FLAGS "-Wl,-z,now")
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(GMP REQUIRED QUIET gmp)
pkg_check_modules(JEMALLOC REQUIRED QUIET jemalloc)


execute_process(
  COMMAND gperf -L ANSI-C --ignore-case command_hash.gperf
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/utils
  OUTPUT_FILE ${CMAKE_SOURCE_DIR}/src/utils/command_hash.c
)

execute_process(
  COMMAND git rev-parse HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND git describe --abbrev=0 --tags
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

file(GLOB SOURCES "src/**/**/*.c" "src/**/*.c" "src/*.c")
list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/src/utils/memcpy_aligned.c ${CMAKE_SOURCE_DIR}/src/utils/memset_aligned.c")


# telly target
add_executable(telly ${SOURCES})
set_property(TARGET telly PROPERTY C_STANDARD 23)
set_property(TARGET telly PROPERTY C_STANDARD_REQUIRED ON)
target_sources(telly PRIVATE src/utils/memcpy_aligned.c src/utils/memset_aligned.c)
target_link_options(telly PRIVATE ${LINK_FLAGS})

include(CheckCCompilerFlag)
check_c_compiler_flag(-mavx2 COMPILER_SUPPORTS_AVX2)
check_c_compiler_flag(-msse2 COMPILER_SUPPORTS_SSE2)

if(COMPILER_SUPPORTS_AVX2)
  set_source_files_properties(src/utils/memcpy_aligned.c src/utils/memset_aligned.c PROPERTIES COMPILE_OPTIONS "-mavx2")
elseif(COMPILER_SUPPORTS_SSE2)
  set_source_files_properties(src/utils/memcpy_aligned.c src/utils/memset_aligned.c PROPERTIES COMPILE_OPTIONS "-msse2")
endif()

target_include_directories(telly PRIVATE
  ${CMAKE_SOURCE_DIR}/headers
  ${GMP_INCLUDE_DIRS}
  ${JEMALLOC_INCLUDE_DIRS}
)

target_link_libraries(telly PRIVATE
  OpenSSL::SSL
  OpenSSL::Crypto
  Threads::Threads
  ${GMP_LINK_LIBRARIES}
  ${JEMALLOC_LINK_LIBRARIES}
  ${MATH_LIB}
)

target_compile_definitions(telly
  PUBLIC GIT_HASH="${GIT_HASH}"
  PUBLIC VERSION="${GIT_VERSION}"
  PUBLIC _TIME_BITS=64
  PUBLIC _GNU_SOURCE
  PUBLIC _FILE_OFFSET_BITS=64
)

install(TARGETS telly)
