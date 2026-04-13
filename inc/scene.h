#ifndef _ABC3D_SCENE_H_
#define _ABC3D_SCENE_H_

#include <vector>

#include "mesh.h"
#include "helper.h"

class Scene {
	std::vector<HalfEdgeMesh*> meshes;
	std::vector<uint> selectedMeshes;
	std::vector<uint> idStack;

public:

	~Scene();
	void DeleteMesh(uint id);
	HalfEdgeMesh* NewMesh();
	HalfEdgeMesh* operator[](uint index);
	const HalfEdgeMesh* operator[](uint index) const;
	const std::vector<HalfEdgeMesh*>& Meshes() const;
	std::vector<HalfEdgeMesh*>& Meshes();
	uint MeshCount() const;
};

#endif