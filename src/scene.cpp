#include "mesh.h"
#include "helper.h"
#include "scene.h"

Scene::~Scene() {
	for (HalfEdgeMesh* mesh : meshes) {
		delete mesh;
	}
}

void Scene::DeleteMesh(uint id) {
	HalfEdgeMesh* mesh = meshes.at(id);
	if (mesh) {
		delete mesh;
		idStack.push_back(id);
	}
}

HalfEdgeMesh* Scene::NewMesh() {
	HalfEdgeMesh* newMesh = new HalfEdgeMesh();

	if (idStack.empty()) {
		meshes.push_back(newMesh);
	}
	else {
		meshes.at(idStack.back()) = newMesh;
		idStack.pop_back();
	}

	return newMesh;
}

HalfEdgeMesh* Scene::operator[](uint index) { return meshes.at(index); }

const HalfEdgeMesh* Scene::operator[](uint index) const { return meshes.at(index); }

const std::vector<HalfEdgeMesh*>& Scene::Meshes() const { return meshes; }

std::vector<HalfEdgeMesh*>& Scene::Meshes() { return meshes; }

uint Scene::MeshCount() const { return meshes.size(); }