cmake_minimum_required(VERSION 3.10)
project(MeshRepair)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/vcglib
    SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/vcglib/eigenlib
)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/vcglib/wrap/ply VCG_PLY)

add_executable(mesh_repair main.cpp ${VCG_PLY})


