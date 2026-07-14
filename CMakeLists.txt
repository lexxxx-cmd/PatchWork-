cmake_minimum_required(VERSION 3.16)
project(patchworkpp VERSION 1.4.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Eigen3 ---
# vcpkg provides Eigen3::Eigen via find_package; otherwise FetchContent download.
find_package(Eigen3 QUIET)
if(NOT TARGET Eigen3::Eigen)
    message(STATUS "Eigen3 not found via find_package, downloading via FetchContent...")
    include(FetchContent)
    set(EIGEN_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(eigen
        URL https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz
    )
    FetchContent_MakeAvailable(eigen)
    add_library(Eigen3::Eigen ALIAS eigen)
endif()

# --- TBB (optional, for classic Patchwork parallel speedup) ---
find_package(TBB CONFIG QUIET)
if(NOT TBB_FOUND)
    find_package(TBB MODULE QUIET)
endif()
if(TBB_FOUND)
    message(STATUS "TBB found — classic Patchwork will use tbb::parallel_for.")
else()
    message(STATUS "TBB not found — classic Patchwork falls back to sequential loop.")
endif()

# --- Subdirectories ---
add_subdirectory(common)
add_subdirectory(patchworkpp)
add_subdirectory(patchwork)
