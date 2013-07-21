
cmake_minimum_required(VERSION 2.8)

project(fakefibre)

add_subdirectory(FakeFibre)

add_executable(fftest test.cpp)
target_link_libraries(fftest fakefibre)

#install(TARGETS fakefibre RUNTIME DESTINATION bin)

