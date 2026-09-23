cmake_minimum_required(VERSION 3.30)

# The toolchain file setup should ideally be in a CMakePresets.json file,
# but we'll keep it here for simplicity
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "Vcpkg toolchain file")

project(abc3d VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add dependencies (glm not used)
find_package(OpenGL REQUIRED)
find_package(GLEW CONFIG REQUIRED)
find_package(FLTK CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)


# No C++20 modules
set(CMAKE_CXX_SCAN_FOR_MODULES OFF) 

# AddressSanitizer build, configure with -DABC3D_ASAN=ON into its own build folder
  option(ABC3D_ASAN "Build with AddressSanitizer" OFF)

  if(ABC3D_ASAN AND MSVC)
      # MSVC ASan refuses /RTC1 and incremental linking, both in CMake's default Debug flags
      string(REGEX REPLACE "/RTC[1csu]+" "" CMAKE_CXX_FLAGS_DEBUG
  "${CMAKE_CXX_FLAGS_DEBUG}")
      string(REGEX REPLACE "/RTC[1csu]+" "" CMAKE_C_FLAGS_DEBUG
  "${CMAKE_C_FLAGS_DEBUG}")
      string(REPLACE "/INCREMENTAL" "/INCREMENTAL:NO"
  CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
  endif()

# Set configuration properties
add_library(abc3d_flags INTERFACE)

if(MSVC)
    target_compile_options(abc3d_flags INTERFACE
        /W3
        /MP # multithreaded build
        /WX- # warnings NOT as errors
        $<$<CONFIG:Debug>:/ZI> # program database for edit and continue, debug only
    )
else()
    target_compile_options(abc3d_flags INTERFACE
        -Wall
        -Wextra
        -Wpedantic
    )
    if(ABC3D_ASAN)
        target_compile_options(abc3d_flags INTERFACE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(abc3d_flags INTERFACE -fsanitize=address)
    endif()
endif()

#abc3d_core: geometry and IO. No FLTK, no OpenGL, no main()
#This is what tests link to
add_library(abc3d_core STATIC
    src/mesh.cpp
    src/scene.cpp
    src/sceneFile.cpp
    inc/mesh.h
    inc/helper.h
    inc/shapes.h
    inc/scene.h
    inc/sceneFile.h
    inc/selection.h
    inc/selectionManager.h
    inc/renderBufferBuilder.h
)

#PUBLIC to allow inheritance of these paths
target_include_directories(abc3d_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/inc"
)

target_link_libraries(abc3d_core PUBLIC abc3d_flags)

target_include_directories(abc3d_core SYSTEM PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/external/cyCodeBase-master"
)

#abc3d_render: OpenGL context. No FLTK
add_library(abc3d_render STATIC
    src/render.cpp
    inc/render.h
)

target_link_libraries(abc3d_render PUBLIC
    abc3d_core
    abc3d_flags
    OpenGL::GL
    GLEW::GLEW
)

#abc3d: FLTK application
add_executable(abc3d
    src/main.cpp
    src/viewport.cpp
    src/customWindow.cpp
    src/customSlider.cpp
    inc/viewport.h
    inc/ui.h
    inc/customWindow.h
    inc/customSlider.h
)

target_link_libraries(abc3d PRIVATE
    abc3d_render
    abc3d_flags
    fltk
    fltk_gl
    fltk_images
)

# Set abc3d as the startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT abc3d)

set_property(TARGET abc3d_core PROPERTY FOLDER "Libraries")
set_property(TARGET abc3d_render PROPERTY FOLDER "Libraries")

enable_testing()
add_subdirectory(tests)

# Enable solution folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)