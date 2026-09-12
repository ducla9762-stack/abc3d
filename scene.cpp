#include "mesh.h"
#include "helper.h"
#include "scene.h"

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
		// Clear the mesh.
		meshes.at(id)->Clear();
		idStack.push_back(id);
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
		return newMesh;
	}

	// Else there are deleted meshes in the scene.
	// We can use their allocated space.

	// Remove the earliest deleted mesh id.
	uint index = idStack.back();
	idStack.pop_back();
	return meshes.at(index);
}

HalfEdgeMesh* Scene::operator[](uint index) { return meshes.at(index); }

const HalfEdgeMesh* Scene::operator[](uint index) const { return meshes.at(index); }

const std::vector<HalfEdgeMesh*>& Scene::Meshes() const { return meshes; }

std::vector<HalfEdgeMesh*>& Scene::Meshes() { return meshes; }

uint Scene::MeshCount() const { return meshes.size(); }