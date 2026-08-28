#ifndef _ABC3D_SCENE_H_
#define _ABC3D_SCENE_H_

#include <vector>

#include "mesh.h"
#include "helper.h"

/// <summary>
/// A Scene is a collection of meshes to be displayed.
/// The scene also keeps track of which of those meshes are currently selected for operations.
/// </summary>
class Scene {
	std::vector<HalfEdgeMesh*> meshes;
	std::vector<bool> selectedMeshes;
	std::vector<uint> idStack;
	uint selectCount = 0;

public:
	/// <summary>
	/// Clears the scene list by deleting it's meshes.
	/// </summary>
	~Scene();

	/// <summary>
	/// Delete the mesh at id from the mesh list.
	/// </summary>
	/// <param name="id"> ID of mesh to be deleted. </param>
	void DeleteMesh(uint id);

	/// <summary>
	/// Duplicate the mesh at id from the mesh list.
	/// </summary>
	/// <param name="id"> ID of mesh to be duplicated. </param>
	void DuplicateMesh(uint id);

	/// <summary>
	/// Add a new mesh to the mesh list.
	/// Push back a new mesh to the list, set it's corresponding selection boolean to false by default.
	/// </summary>
	/// <returns> Pointer to new mesh. </returns>
	HalfEdgeMesh* NewMesh();

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
	/// Returns the non-mutable list of selection booleans for selected meshes in scene.
	/// </summary>
	/// <returns> Address of list of selected meshes in scene. </returns>
	const std::vector<bool>& SelectedMeshes() const;

	/// <summary>
	/// Returns the mutable list of meshes in scene.
	/// </summary>
	/// <returns> Address of list of meshes in scene. </returns>
	std::vector<HalfEdgeMesh*>& Meshes();

	/// <summary>
	/// Returns a mutable list of selection boolesn for selected meshes in scene.
	/// </summary>
	/// <returns> Address of list of selected meshes in scene. </returns>
	std::vector<bool>& SelectedMeshes();

	/// <summary>
	/// Returns the number of meshes in the scene.
	/// </summary>
	/// <returns> Size of the list of meshes in the scene. </returns>
	uint MeshCount() const;

	/// <summary>
	/// Returns the number of selected meshes in scene.
	/// </summary>
	/// <returns> Returns the select count. </returns>
	uint SelectedMeshCount() const;

	/// <summary>
	/// Sets the selection boolean of the mesh at id to true.
	/// </summary>
	/// <param name="id"> The id of the mesh in scene. </param>
	void AddToSelectedMeshes(uint id);

	/// <summary>
	/// Sets the selection boolean of the mesh at id to false.
	/// </summary>
	/// <param name="id"> The id of the mesh in scene.</param>
	void RemoveFromSelectedMeshes(uint id);

	/// <summary>
	/// Returns if the mesh at id is selected or not.
	/// </summary>
	/// <param name="id"> The id of the mesh in scene. </param>
	/// <returns> True if the mesh is selected. </returns>
	bool IsMeshSelected(uint id);
};

#endif