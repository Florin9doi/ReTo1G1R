cmake_minimum_required(VERSION 3.10)

find_package(LibXml2 REQUIRED)
include_directories(ReTo1G1R ${LIBXML2_INCLUDE_DIR})

add_executable (ReTo1G1R ReTo1G1R/Source.c)

target_link_libraries(ReTo1G1R ${LIBXML2_LIBRARIES})
