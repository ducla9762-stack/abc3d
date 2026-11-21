#ifndef _ABC3D_SCENE_H_
#define _ABC3D_SCENE_H_

#include <vector>
#include <string>
#include <memory>
#include <utility>

#include "mesh.h"
#include "helper.h"

/// <summary>
/// A Scene is a collection of meshes to be displayed.
/// The scene also keeps track of which of those meshes are currently selected for operations.
/// </summary>
struct ReferenceImage {
	uint id = 0;
	std::string sourcePath;

	Vec3f position = Vec3f(0.0f);
	Vec3f rotation = Vec3f(0.0f);
	Vec3f scale = Vec3f(1.0f);

	float opacity = 1.0f;
	bool visible = true;
	bool locked = false;

	bool isNull() const { return sourcePath.empty(); }
};

class Scene {
	std::vector<HalfEdgeMesh*> meshes;
	std::vector<string> names;
	std::vector<ReferenceImage> referenceImages;
	uint nextReferenceImageId = 0;

public:
	/// <summary>
	/// Clears the scene list by deleting it's meshes.
	/// </summary>
	~Scene();
	/// <summary>
	/// Deletes every mesh and reuse stack, leaving the scene empty. 
	/// Mesh ids restart at 0 afterwards, so anything holding an id must remap. 
	/// </summary>
	void Clear(); 
	/// <summary>
	/// Delete the mesh at id from the mesh list.
	/// </summary>
	/// <param name="id"> ID of mesh to be deleted. </param>
	void DeleteMesh(uint id);

	/// <summary>
	/// Removes the mesh at id exactly like DeleteMesh (the last mesh moves into the hole),
	/// but hands the mesh and its name to the caller instead of deleting it. Undo keeps
	/// removed meshes this way without copying them.
	/// </summary>
	std::pair<std::unique_ptr<HalfEdgeMesh>, std::string> DetachMesh(uint id);

	/// <summary>
	/// Exact inverse of DetachMesh(id): appends the mesh, then swaps it into slot id, which
	/// sends the mesh that was moved into the hole back to the end. id == MeshCount() appends.
	/// </summary>
	void InsertMeshAt(uint id, std::unique_ptr<HalfEdgeMesh> mesh, std::string name);

	/// <summary>
	/// Duplicate the mesh at id from the mesh list.
	/// </summary>
	/// <param name="id"> ID of mesh to be duplicated. </param>
	void DuplicateMesh(uint id, std::string name);

	/// <summary>
	/// Add a new mesh to the mesh list.
	/// Push back a new mesh to the list, set it's corresponding selection boolean to false by default.
	/// </summary>
	/// <returns> Pointer to new mesh. </returns>
	HalfEdgeMesh* NewMesh(std::string name);

	ReferenceImage* NewReferenceImage(const std::string& sourcePath);
	ReferenceImage* AddReferenceImage(const ReferenceImage& image);
	void DeleteReferenceImage(uint id);
	ReferenceImage* FindReferenceImage(uint id);
	const ReferenceImage* FindReferenceImage(uint id) const;
	const std::vector<ReferenceImage>& ReferenceImages() const;
	std::vector<ReferenceImage>& ReferenceImages();
	uint ReferenceImageCount() const;

	/// <summary>
	/// Returns the mutable mesh at index.
	/// </summary>
	/// <param name="index"> Index of mesh to get. </param>
	/// <returns> Pointer to mesh at index. </returns>
	HalfEdgeMesh* operator[](uint index);

	/// <summary>
	/// Returns the non-mutable mesh at index.
	/// </summary>
	/// <param name="index"> Index of mesh to get. </param>
	/// <returns> Pointer to mesh at index. </returns>
	const HalfEdgeMesh* operator[](uint index) const;

	/// <summary>
	/// Returns the non-mutable list of meshes in scene.
	/// </summary>
	/// <returns> Address of list of meshes in scene. </returns>
	const std::vector<HalfEdgeMesh*>& Meshes() const;

	/// <summary>
	/// Returns the list of names in the scene
	/// </summary>
	/// <returns> List of strings in the scene </returns>
	const std::vector<string>& Names() const;

	void setName(int index, std::string name);

	/// <summary>
	/// Returns the mutable list of meshes in scene.
	/// </summary>
	/// <returns> Address of list of meshes in scene. </returns>
	std::vector<HalfEdgeMesh*>& Meshes();

	/// <summary>
	/// Returns the number of meshes in the scene.
	/// </summary>
	/// <returns> Size of the list of meshes in the scene. </returns>
	uint MeshCount() const;
};

#endif
