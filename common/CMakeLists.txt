add_library(ground_seg_common STATIC src/plane_fit.cpp)
target_include_directories(ground_seg_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(ground_seg_common PUBLIC Eigen3::Eigen)
