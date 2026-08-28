#include "mesh.h"
#include "helper.h"
#include "scene.h"

// TODO: Remove selection responsibility.

Scene::~Scene() {
	// Loop through all meshes and delete them.
	for (HalfEdgeMesh* mesh : meshes) {
		delete mesh;
	}
}

void Scene::DeleteMesh(uint id) {
	// Get the mesh at id.
	HalfEdgeMesh* mesh = meshes.at(id);
	// If the mesh exists:
	if (mesh) {
		// Clear the mesh, , ensure mesh is de-selected.
		meshes.at(id)->Clear();
		idStack.push_back(id);
		selectedMeshes.at(id) = false;
	}
}

void Scene::DuplicateMesh(uint id) {
	HalfEdgeMesh& newMesh = *NewMesh();
	newMesh = *meshes.at(id);
}

HalfEdgeMesh* Scene::NewMesh() {
	// If there are no deleted meshes in the scene:
	if (idStack.empty()) {
		// Create a new mesh, push it back, and push back false by default for selection.
		HalfEdgeMesh* newMesh = new HalfEdgeMesh();
		meshes.push_back(newMesh);
		selectedMeshes.push_back(false);
		return newMesh;
	}

	// Else there are deleted meshes in the scene.
	// We can use their allocated space.

	// Remove the earliest deleted mesh id.
	uint index = idStack.back();
	idStack.pop_back();

	// Set selection status of the new mesh to false by default.
	selectedMeshes.at(index) = false;
	return meshes.at(index);
}

HalfEdgeMesh* Scene::operator[](uint index) { return meshes.at(index); }

const HalfEdgeMesh* Scene::operator[](uint index) const { return meshes.at(index); }

const std::vector<HalfEdgeMesh*>& Scene::Meshes() const { return meshes; }

const std::vector<bool>& Scene::SelectedMeshes() const { return selectedMeshes; }

std::vector<HalfEdgeMesh*>& Scene::Meshes() { return meshes; }

std::vector<bool>& Scene::SelectedMeshes() { return selectedMeshes; }

uint Scene::MeshCount() const { return meshes.size(); }

uint Scene::SelectedMeshCount() const { return selectCount; }

/// <summary>
/// Sets boolean at mesh id to true.
/// </summary>
/// <param name="id"></param>
void Scene::AddToSelectedMeshes(uint id) {
	// If id out of bounds or the mesh at id is already selected, do nothing.
	if (id >= meshes.size() || selectedMeshes[id] == true) {
		return;
	}

	// Else, set mesh boolean at id to true, up the selection count.
	selectedMeshes[id] = true;
	++selectCount;
}

/// <summary>
/// Sets boolean at mesh id to false.
/// </summary>
/// <param name="id"></param>
void Scene::RemoveFromSelectedMeshes(uint id) {
	// If id out of bounds or the mesh is already deselected, do nothing.
	if (id >= meshes.size() || selectedMeshes[id] == false) { 
		return;
	}

	// Else, set mesh boolean at id to false, decrement the selection
	selectedMeshes[id] = false;
	--selectCount;
}

/// <summary>
/// Returns if the mesh at id is selected.
/// </summary>
/// <param name="id"></param>
/// <returns></returns>
bool Scene::IsMeshSelected(uint id) {
	// If mesh id in bounds, return selection boolean.
	if (id < meshes.size()) {
		return selectedMeshes[id];
	}
	// Else out of bounds, return false.
	else {
		return false;
	}
}
