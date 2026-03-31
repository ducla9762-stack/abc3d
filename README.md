# ABC3D

## How To Install

Run GENERATE_SLN.bat, this will prompt vcpkg to install all required packages, and then CMAKE will build a solution in a newly created build folder in the CMakeLists directory.

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