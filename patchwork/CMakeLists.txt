add_library(ground_seg_classic STATIC src/patchwork.cpp)
target_include_directories(ground_seg_classic PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(ground_seg_classic PUBLIC Eigen3::Eigen ground_seg_common)
if(TBB_FOUND)
    target_link_libraries(ground_seg_classic PUBLIC TBB::tbb)
    target_compile_definitions(ground_seg_classic PUBLIC PATCHWORK_HAS_TBB)
endif()
