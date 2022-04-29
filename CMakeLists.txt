cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

set(GEODE_BUILD_CODEGEN on)

project(SmallEnhancements VERSION 0.1.0 LANGUAGES C CXX)

set(SOURCE_FILES src/main.cpp sdk/include/implicitEntry.cpp)
set(BINARY_NAME SmallEnhancements)
include(sdk/Geode.cmake)
