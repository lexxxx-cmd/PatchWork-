find_package(yaml-cpp REQUIRED)
find_package(PCL CONFIG REQUIRED COMPONENTS common io)

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
    pcl_common pcl_io
)
if(MSVC)
    target_compile_options(patchwork_cli PRIVATE /wd4068 /wd4267 /wd4244 /wd4018)
endif()
set_target_properties(patchwork_cli PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
