cmake_minimum_required (VERSION 2.8.11)
project (sandbox)

if (NOT CMAKE_TOOLCHAIN_FILE)
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.10.x
)
FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        master
  GIT_SHALLOW    ON
  PREFIX         ${CMAKE_CURRENT_BINARY_DIR}/external/tmp/benchmark
  CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DBENCHMARK_ENABLE_LTO=ON
)
FetchContent_MakeAvailable(googletest googlebenchmark)
else ()
find_package(GTest)
find_package(benchmark CONFIG REQUIRED)
endif ()

if (MSVC)
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} /std:c++latest /experimental:preprocessor /W4 /WX")
    set(CMAKE_CXX_FLAGS_RELEASE
        "${CMAKE_CXX_FLAGS_RELEASE} /O2 /arch:AVX512")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO
        "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /O2 /arch:AVX512")
    add_definitions(/DNOMINMAX /DWIN32_LEAN_AND_MEAN /D_CRT_SECURE_NO_WARNINGS /D_CRT_NONSTDC_NO_WARNINGS)
else ()
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -std=gnu++2a -Wall -Werror -pedantic -O2 -march=native")
    set(CMAKE_CXX_FLAGS_DEBUG
        "${CMAKE_CXX_FLAGS_DEBUG} -ggdb")
    set(CMAKE_CXX_FLAGS_RELEASE
        "${CMAKE_CXX_FLAGS_RELEASE} -O2 -march=native")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO
        "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g -ggdb -O2 -march=native")
endif ()

find_package(Threads REQUIRED)

add_subdirectory(vectorization)
