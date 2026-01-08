cmake_minimum_required(VERSION 3.13)

# Project setup
project(3dsTok)

# Find devkitPro specific stuff
if(NOT DEFINED DEVKITPRO)
    message(FATAL_ERROR "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitPro")
endif()

# Include 3DS CMake tools
include(${DEVKITPRO}/cmake/3DS.cmake)

# Source files
file(GLOB_RECURSE SOURCES source/*.c source/*.cpp)

# Executable setup
add_executable(3dsTok ${SOURCES})

# Target setup
target_include_directories(3dsTok PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(3dsTok PUBLIC -std=c11 -O2 -ffunction-sections)
target_link_options(3dsTok PUBLIC -specs=3dsx.specs)

# Libraries: ctru (base), citro3d (gpu), citro2d (2d api), m (math)
target_link_libraries(3dsTok citro2d citro3d ctru m)

# Create .3dsx and .smdh
db_3dsx_target(3dsTok 
    NO_SMDH # We can generate one manually if needed, or let cmake do it with default metadata
)
