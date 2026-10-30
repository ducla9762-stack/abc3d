#include "mesh.h"
#include "helper.h"
#include "scene.h"

Scene::~Scene() {
	Clear(); 
}

void Scene::Clear() {
	// Loop through all meshes and delete them.
	for (HalfEdgeMesh* mesh : meshes) {
		delete mesh;
	}

	// Loop through all names and delete them.
	for (string name : names) {
		name.erase();
	}
	
	meshes.clear();
}

void Scene::DeleteMesh(uint id) {
	delete meshes.at(id);
	names[id].erase();
	std::swap(meshes[id], meshes.back());
	meshes.pop_back();
}

void Scene::DuplicateMesh(uint id) {
	HalfEdgeMesh& newMesh = *NewMesh();
	newMesh = *meshes.at(id);
	names.push_back(names[id] + std::to_string(id));
}

HalfEdgeMesh* Scene::NewMesh() {
	meshes.push_back(new HalfEdgeMesh());
	names.push_back("Object " + std::to_string(meshes.size() - 1));
	return meshes.back();
}

HalfEdgeMesh* Scene::operator[](uint index) { return meshes.at(index); }

const HalfEdgeMesh* Scene::operator[](uint index) const { return meshes.at(index); }

const std::vector<HalfEdgeMesh*>& Scene::Meshes() const { return meshes; }

const std::vector<string> Scene::Names() const { return names; }

std::vector<HalfEdgeMesh*>& Scene::Meshes() { return meshes; }

uint Scene::MeshCount() const { return meshes.size(); }