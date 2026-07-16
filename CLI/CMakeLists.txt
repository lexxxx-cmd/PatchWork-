# yaml-cpp: FetchContent 方式引入，无需系统预装
find_package(yaml-cpp)

add_executable(patchwork_cli main.cpp
    ${CMAKE_SOURCE_DIR}/src/PCDReader.cpp
    ${CMAKE_SOURCE_DIR}/src/LASWriter.cpp
)
target_include_directories(patchwork_cli PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/common/include
    ${CMAKE_SOURCE_DIR}/patchwork/include
    ${CMAKE_SOURCE_DIR}/patchworkpp/include
)
target_link_libraries(patchwork_cli PRIVATE
    ground_seg_common
    ground_seg_cores
    ground_seg_classic
    Eigen3::Eigen
    yaml-cpp
)
set_target_properties(patchwork_cli PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
