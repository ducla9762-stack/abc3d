#include "mesh.h"
#include "helper.h"
#include "scene.h"
#include <algorithm>

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
	names.clear();
	referenceImages.clear();
	nextReferenceImageId = 0;
}

void Scene::DeleteMesh(uint id) {
	delete meshes.at(id);
	names[id].erase();
	std::swap(meshes[id], meshes.back());
	std::swap(names[id], names.back());
	meshes.pop_back();
	names.pop_back();
}

std::pair<std::unique_ptr<HalfEdgeMesh>, std::string> Scene::DetachMesh(uint id) {
	std::unique_ptr<HalfEdgeMesh> mesh(meshes.at(id));
	std::string name = std::move(names[id]);
	std::swap(meshes[id], meshes.back());
	std::swap(names[id], names.back());
	meshes.pop_back();
	names.pop_back();
	return { std::move(mesh), std::move(name) };
}

void Scene::InsertMeshAt(uint id, std::unique_ptr<HalfEdgeMesh> mesh, std::string name) {
	if (id > meshes.size()) { id = static_cast<uint>(meshes.size()); }
	meshes.push_back(mesh.release());
	names.push_back(std::move(name));
	std::swap(meshes[id], meshes.back());
	std::swap(names[id], names.back());
}

void Scene::DuplicateMesh(uint id, std::string name) {
	HalfEdgeMesh& newMesh = *NewMesh(name);
	newMesh = *meshes.at(id);
	names.back() = names[id];
}

HalfEdgeMesh* Scene::NewMesh(std::string name) {
	meshes.push_back(new HalfEdgeMesh());
	names.push_back(name);
	return meshes.back();
}

ReferenceImage* Scene::NewReferenceImage(const std::string& sourcePath) {
	referenceImages.push_back(ReferenceImage());
	ReferenceImage& image = referenceImages.back();
	image.id = nextReferenceImageId++;
	image.sourcePath = sourcePath;
	return &image;
}

ReferenceImage* Scene::AddReferenceImage(const ReferenceImage& image) {
	referenceImages.push_back(image);
	nextReferenceImageId = std::max(nextReferenceImageId, image.id + 1);
	return &referenceImages.back();
}

void Scene::DeleteReferenceImage(uint id) {
	for (auto it = referenceImages.begin(); it != referenceImages.end(); ++it) {
		if (it->id == id) {
			referenceImages.erase(it);
			return;
		}
	}
}

ReferenceImage* Scene::FindReferenceImage(uint id) {
	for (ReferenceImage& image : referenceImages) {
		if (image.id == id) return &image;
	}
	return nullptr;
}

const ReferenceImage* Scene::FindReferenceImage(uint id) const {
	for (const ReferenceImage& image : referenceImages) {
		if (image.id == id) return &image;
	}
	return nullptr;
}

HalfEdgeMesh* Scene::operator[](uint index) { return meshes.at(index); }

const HalfEdgeMesh* Scene::operator[](uint index) const { return meshes.at(index); }

const std::vector<HalfEdgeMesh*>& Scene::Meshes() const { return meshes; }

const std::vector<string>& Scene::Names() const { return names; }

void Scene::setName(int index, std::string name) {
	names[index] = name;
}

std::vector<HalfEdgeMesh*>& Scene::Meshes() { return meshes; }

uint Scene::MeshCount() const { return meshes.size(); }

const std::vector<ReferenceImage>& Scene::ReferenceImages() const { return referenceImages; }
std::vector<ReferenceImage>& Scene::ReferenceImages() { return referenceImages; }
uint Scene::ReferenceImageCount() const { return static_cast<uint>(referenceImages.size()); }
