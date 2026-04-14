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
		meshes.at(id)->Clear();
		idStack.push_back(id);
		selectedMeshes.push_back(false);
	}
}

HalfEdgeMesh* Scene::NewMesh() {

	if (idStack.empty()) {
		HalfEdgeMesh* newMesh = new HalfEdgeMesh();
		meshes.push_back(newMesh);
		selectedMeshes.push_back(false);
		return newMesh;
	}

	uint index = idStack.back();
	idStack.pop_back();
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

uint Scene::SelectedMeshCount() const { return selectedMeshes.size(); }

void Scene::AddToSelectedMeshes(uint id)
{
	HalfEdgeMesh* mesh = meshes.at(id);
	if (mesh) {
		selectedMeshes.at(id) = true;
	}

	std::cout << "Selected object: " << id << ": " << selectedMeshes.at(id) << std::endl;
}

void Scene::RemoveFromSelectedMeshes(uint id)
{
	HalfEdgeMesh* mesh = meshes.at(id);
	if (mesh) {
		selectedMeshes.at(id) = false;
	}

	std::cout << "Selected object: " << id << ": " << selectedMeshes.at(id) << std::endl;
}

bool Scene::IsMeshSelected(uint id)
{
	return selectedMeshes[id];
}
