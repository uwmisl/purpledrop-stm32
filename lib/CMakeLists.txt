cmake_minimum_required(VERSION 3.15)


# Auto-compute version info into version.cpp
set(PRE_CONFIGURE_FILE "src/version.cpp.in")
set(POST_CONFIGURE_FILE "src/version.cpp")
include(version.cmake)

# Make sure version.cpp exists prior to the GLOB, so it gets included in the
# source list for build
file(TOUCH src/version.cpp)

file(GLOB SOURCE_FILES
    src/*.c
    src/*.cpp
)

add_library(purpledrop_lib STATIC ${SOURCE_FILES})

add_dependencies(purpledrop_lib update_version)



set_property(TARGET purpledrop_lib PROPERTY CXX_STANDARD 20)