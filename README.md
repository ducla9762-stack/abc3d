# ABC3D

## How To Install

Run GENERATE_SLN.bat, this will prompt vcpkg to install all required packages, and then CMAKE will build a solution in a newly created build folder in the CMakeLists directory. Ensure that vcpkg.json exists, without this file vcpkg will not install required packages, and the CMake build will fail.

## Troubleshooting

If any difficulty was encountered during the running of GENERATE_SLN.bat, delete the build folder before attempting to run again. The setup script generates Visual Studio 2022 x64 files; Visual Studio 2022 must be installed.

If CMake reports `NMake makefiles does not support platform specifications`, delete the build folder and run the script again. This means the build folder was previously configured with the NMake generator while the script requested the x64 platform, which is only supported by Visual Studio generators.

## Structure

### assets

This folder contains all non-compiled assets such as .obj models, shaders, textures, and materials.

### build

This folder is generated when running the setup batch file. This contains your Visual Studio solution. This folder should not be pushed to the repository.

### external

Any header-only libraries that vcpkg does not have access to. For now, this includes cyCodeBase.

### inc

All ABC3D headers.

### src 

All ABC3D .cpp files.

### vcpkg

Contains your vcpkg install, leave this where it is, as CMAKE is pointed to this folder.
