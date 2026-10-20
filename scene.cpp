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
	
	meshes.clear();
}

void Scene::DeleteMesh(uint id) {
	delete meshes.at(id);
	std::swap(meshes[id], meshes.back());
	meshes.pop_back();
}

void Scene::DuplicateMesh(uint id) {
	HalfEdgeMesh& newMesh = *NewMesh();
	newMesh = *meshes.at(id);
}

HalfEdgeMesh* Scene::NewMesh() {
	meshes.push_back(new HalfEdgeMesh());
	return meshes.back();
}

HalfEdgeMesh* Scene::operator[](uint index) { return meshes.at(index); }

const HalfEdgeMesh* Scene::operator[](uint index) const { return meshes.at(index); }

const std::vector<HalfEdgeMesh*>& Scene::Meshes() const { return meshes; }

std::vector<HalfEdgeMesh*>& Scene::Meshes() { return meshes; }

uint Scene::MeshCount() const { return meshes.size(); }