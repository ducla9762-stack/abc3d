cmake_minimum_required(VERSION 3.30)

# The toolchain file setup should ideally be in a CMakePresets.json file,
# but we'll keep it here for simplicity
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "Vcpkg toolchain file")

project(abc3d VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Only the test build lives in CMake. The Qt app builds from ABC3D.vcxproj.
find_package(GTest CONFIG REQUIRED)


# No C++20 modules
set(CMAKE_CXX_SCAN_FOR_MODULES OFF) 

# AddressSanitizer build, configure with -DABC3D_ASAN=ON into its own build folder
option(ABC3D_ASAN "Build with AddressSanitizer" OFF)

if(ABC3D_ASAN AND MSVC)
    # MSVC ASan refuses /RTC1 and incremental linking, both in CMake's default Debug flags
    string(REGEX REPLACE "/RTC[1csu]+" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REGEX REPLACE "/RTC[1csu]+" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    string(REPLACE "/INCREMENTAL" "/INCREMENTAL:NO" CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
endif()

# Set configuration properties
add_library(abc3d_flags INTERFACE)

if(MSVC)
    target_compile_options(abc3d_flags INTERFACE
        /W3
        /MP # multithreaded build
        /WX- # warnings NOT as errors
        $<$<AND:$<CONFIG:Debug>,$<NOT:$<BOOL:${ABC3D_ASAN}>>>:/ZI> # program database for edit and continue, debug only (not with ASan)
        $<$<BOOL:${ABC3D_ASAN}>:/fsanitize=address>
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

#abc3d_core: geometry and IO. No Qt, no OpenGL, no main()
#This is what tests link to
add_library(abc3d_core STATIC
    mesh.cpp
    scene.cpp
    sceneFile.cpp
    mesh.h
    helper.h
    shapes.h
    scene.h
    sceneFile.h
    selectionManager.h
    renderBufferBuilder.h
)

#PUBLIC to allow inheritance of these paths
target_include_directories(abc3d_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(abc3d_core PUBLIC abc3d_flags)

target_include_directories(abc3d_core SYSTEM PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/external/cyCodeBase"
)

enable_testing()
add_subdirectory(tests)
