cmake_minimum_required(VERSION 3.0)

project(Sacred_2_Modmerge VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(s2modmerge src/s2modmerge.cpp)
# Output name can be changed with this
# set_target_properties(${THE_TARGET_NAME_VARIABLE} PROPERTIES OUTPUT_NAME "${BINARY_NAME})

add_subdirectory(lib/json)
target_link_libraries(s2modmerge PRIVATE nlohmann_json::nlohmann_json)