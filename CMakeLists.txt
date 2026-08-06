cmake_minimum_required(VERSION 3.30)

# The toolchain file setup should ideally be in a CMakePresets.json file,
# but we'll keep it here for simplicity
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "Vcpkg toolchain file")

project(abc3d VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add dependencies
find_package(OpenGL REQUIRED)
find_package(GLEW CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(FLTK CONFIG REQUIRED)


# No C++20 modules
set(CMAKE_CXX_SCAN_FOR_MODULES OFF) 

# Executable
add_executable(abc3d)

# Sources
file(GLOB_RECURSE abc3d_SOURCES CONFIGURE_DEPENDS 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# Headers
file(GLOB_RECURSE abc3d_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/inc/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/inc/*.hpp"
)

target_sources(abc3d PRIVATE 
    ${abc3d_SOURCES}
    ${abc3d_HEADERS}
)

# Include dirs
target_include_directories(abc3d PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/inc"
)
target_include_directories(abc3d SYSTEM PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/external/cyCodeBase-master"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/im3d"
)

# Preprocessor defines
target_compile_definitions(abc3d PRIVATE
    MY_TEST_DEFINE
)

# Automatically links to these libs
target_link_libraries(abc3d PRIVATE
    OpenGL::GL
    GLEW::GLEW
    glm::glm
    fltk
    fltk_gl
)

# Add directories with any random loose .dlls or .libs here
target_link_directories(abc3d PRIVATE
  
)

# Set configuration properties
if(MSVC)
    target_compile_options(abc3d PRIVATE
        /W3
        /MP # multithreaded build
        /WX- # warnings as errors
        /ZI # program database for edit and continue
    )
else()
    target_compile_options(abc3d PRIVATE 
        -Wall 
        -Wextra 
        -Wpedantic
    )
endif()

# Set abc3d as the startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT abc3d)

# Filters
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Source Files" FILES ${abc3d_SOURCES})

# Enable solution folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)