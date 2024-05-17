cmake_minimum_required(VERSION 3.10...3.21)

project(milter_altermime)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)



add_executable ( milter_altermime src/milter_altermime.cpp src/altermimeWrapper.cpp src/configdata.cpp)
target_include_directories(milter_altermime PRIVATE src/)
target_link_libraries ( milter_altermime milter pthread)
