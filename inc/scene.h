#ifndef _ABC3D_SCENE_H_
#define _ABC3D_SCENE_H_

#include <vector>

#include "mesh.h"
#include "helper.h"

class Scene {
	std::vector<HalfEdgeMesh*> meshes;
	std::vector<bool> selectedMeshes;
	std::vector<uint> idStack;

public:

	~Scene();
	void DeleteMesh(uint id);
	void DuplicateMesh(uint id); 
	HalfEdgeMesh* NewMesh();
	HalfEdgeMesh* operator[](uint index);
	const HalfEdgeMesh* operator[](uint index) const;
	const std::vector<HalfEdgeMesh*>& Meshes() const;
	const std::vector<bool>& SelectedMeshes() const;
	std::vector<HalfEdgeMesh*>& Meshes();
	std::vector<bool>& SelectedMeshes();
	uint MeshCount() const;
	uint SelectedMeshCount() const;
	void AddToSelectedMeshes(uint id);
	void RemoveFromSelectedMeshes(uint id);
	bool IsMeshSelected(uint id);
};

#endif