cmake_minimum_required(VERSION 3.22)

project(mioty_at_client_c VERSION 1.0.1 DESCRIPTION "mioty-at-client fork by julian bischof")

# add sources
add_library(mioty_at_client_c STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/miotyAtClient.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/data_tools/char_tools.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/data_tools/string_tools.c
)

# add headers
target_include_directories(mioty_at_client_c PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/data_tools
)

# public API
set_target_properties(mioty_at_client_c PROPERTIES PUBLIC_HEADER 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/miotyAtClient.h
)

