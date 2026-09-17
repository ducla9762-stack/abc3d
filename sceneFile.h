#ifndef _ABC3D_SCENE_FILE_H_
#define _ABC3D_SCENE_FILE_H_

#include <string>
#include <vector>
#include <iosfwd>

#include "helper.h"

//Add 1 when the layout changes in a way that the older reader would misread
constexpr int SCENE_FILE_VERSION = 1; 

/// <summary>
/// One saved object, its transform, local-space .obj text for its geometry
/// The transform is kept beside the geometry
/// This way, thethe scene restores the same pos/rot/scale that the UI has
/// </summary>
struct SceneFileObject {
	uint id = 0; 
	std::string name; 
	Vec3f pos = Vec3f(0.0f);
	Vec3f rot = Vec3f(0.0f);
	Vec3f scale = Vec3f(1.0f);
	std::string objText; 
};

/// <summary>
/// Everything an ABC3D scene file might carry (materials, lights, UV's, hierarchy are absent)
/// When they exist, the version number will go up
/// </summary>
struct SceneFile {
	Vec3f camera = Vec3f(0.0f, 90.0f, 5.0f); 
	bool grid = true; 
	bool points = true; 
	std::vector<uint> selection; 
	std::vector<SceneFileObject> objects; 
	std::vector<std::string> specs; //any metadata that we might want to include in the future
};

/// <returns>true if the whole document is written</returns>
bool WriteSceneFile(const std::string& filePath, const SceneFile& doc);

bool ReadSceneFile(const std::string& filePath, SceneFile& doc); 

#endif //_ABC3D_SCENE_FILE_H
