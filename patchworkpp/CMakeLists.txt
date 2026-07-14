add_library(ground_seg_cores STATIC src/patchworkpp.cpp)
target_include_directories(ground_seg_cores PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(ground_seg_cores PUBLIC Eigen3::Eigen ground_seg_common)
