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
		selectedMeshes.at(id) = false;
	}
}

void Scene::DuplicateMesh(uint id) {
	HalfEdgeMesh* original = meshes.at(id); 
	if (!original)
		return;
	
	HalfEdgeMesh* duplicate = new HalfEdgeMesh(*original);
	meshes.push_back(duplicate); 
	selectedMeshes.push_back(false);

	if (!idStack.empty()) {
		selectedMeshes.at(idStack.back()) = false;
		idStack.pop_back();
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

/// <summary>
/// Sets boolean at mesh id to true.
/// </summary>
/// <param name="id"></param>
void Scene::AddToSelectedMeshes(uint id)
{
	HalfEdgeMesh* mesh = meshes.at(id);
	if (mesh) {
		selectedMeshes.at(id) = true;
	}

	std::cout << "Selected object: " << id << ": " << selectedMeshes.at(id) << std::endl;
}

/// <summary>
/// Sets boolean at mesh id to false.
/// </summary>
/// <param name="id"></param>
void Scene::RemoveFromSelectedMeshes(uint id)
{
	HalfEdgeMesh* mesh = meshes.at(id);
	if (mesh) {
		selectedMeshes.at(id) = false;
	}

	std::cout << "Selected object: " << id << ": " << selectedMeshes.at(id) << std::endl;
}

/// <summary>
/// Returns if the mesh at id is selected.
/// </summary>
/// <param name="id"></param>
/// <returns></returns>
bool Scene::IsMeshSelected(uint id)
{
	return selectedMeshes[id];
}
