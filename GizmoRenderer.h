#ifndef ABC3D_GIZMO_RENDERER_H
#define ABC3D_GIZMO_RENDERER_H

#include <cstddef>
#include <vector>

#define GLEW_STATIC
#include <GL/glew.h>

#include <cyGL.h>
#include <cyMatrix.h>

#include "Gizmo.h"

class GizmoRenderer {
public:
	enum class Type { Translation, Scale, Rotation };

private:
	/// <summary>
	/// Defines a piece of a gizmo, that being an arrow, a mace, or a ring.
	/// </summary>
	struct Part {
		std::vector<Vec3f> vertices;
		std::vector<Vec3u> triangles;
		GLSLProgram visual;
		GLSLProgram pick;
		GLuint vao = 0;
		GLuint vbo = 0;
		GLuint ebo = 0;
		GLuint instances = 0;
		GLsizei instanceCount = 0;

		/// <summary>
		/// Setup shaders and buffers for individual parts.
		/// </summary>
		/// <param name="vertexShader"> Path to vertex shader. </param>
		/// <param name="vertices"> Vertex data. </param>
		/// <param name="triangles"> Face data. </param>
		/// <returns> False if errors. </returns>
		bool Initialize(const char* vertexShader, const std::vector<Vec3f>& vertices,
			const std::vector<Vec3u>& triangles);

		/// <summary>
		/// Upload part settings to shaders.
		/// </summary>
		/// <param name="settings"> Part settings. </param>
		/// <param name="usage"></param>
		void Upload(const std::vector<GizmoInstanceSettings>& settings, GLenum usage = GL_DYNAMIC_DRAW);

		/// <summary>
		/// Set view projection matrix in part settings.
		/// </summary>
		/// <param name="data"> Matrix. </param>
		/// <param name="screenScale"></param>
		void SetViewProjection(float data[16], float screenScale);

		/// <summary>
		/// Set rotation in part settings.
		/// </summary>
		/// <param name="rotation"></param>
		void SetRotation(const Matrix3f& rotation);

		/// <summary>
		/// Set rotation ring specific uniforms.
		/// </summary>
		/// <param name="tolerance"> epsilon for hovering over rings. </param>
		void SetRingMode(float tolerance);

		/// <summary>
		/// Draw part.
		/// </summary>
		/// <param name="picking"> Pick pass or draw pass. </param>
		void Draw(bool picking);

		/// <summary>
		/// Clear part.
		/// </summary>
		void Clear() { instanceCount = 0; }
	};

	// The gizmo renderer keeps track of each type of part, and will render them in instances.
	Part translation;
	Part scale;
	Part rotation;
	Part free;

	// The gizmo renderer keeps trakc of the settings for each type of part.
	std::vector<GizmoInstanceSettings> arrows;
	std::vector<GizmoInstanceSettings> maces;
	std::vector<GizmoInstanceSettings> hoops;
	std::vector<GizmoInstanceSettings> cube;

	/// <summary>
	/// Set the rotation of the gizmo part.
	/// </summary>
	/// <param name="rotation"></param>
	void SetRotation(const Matrix3f& rotation);

	/// <summary>
	/// Upload gizmo parts to the GPU for rendering.
	/// </summary>
	void UploadParts();

public:
	/// <summary>
	/// Setup shader programs and buffers.
	/// </summary>
	/// <returns> False if error. </returns>
	bool Initialize();

	/// <summary>
	/// Send the view projection matrix to the gizmo shader.
	/// </summary>
	/// <param name="data"> Matrix </param>
	/// <param name="fov"> Camera fov </param>
	/// <param name="viewportHeight"></param>
	void SetViewProjection(float data[16], float fov, int viewportHeight);

	/// <summary>
	/// Update base positions of gizmos, color for axes, rotation.
	/// </summary>
	/// <param name="base"> Base position to draw at. </param>
	/// <param name="hoveredAxis"> ID of axis being hovered. </param>
	/// <param name="type"> Gizmo type. </param>
	/// <param name="rotation"> Rotation matrix. </param>
	void UpdateGizmos(const Vec3f& base, uint hoveredAxis, Type type, const Matrix3f& rotation);

	/// <summary>
	/// Draw the visible gizmo overlay and refresh its picking framebuffer.
	/// </summary>
	/// <param name="defaultFramebuffer"> The viewport's onscreen framebuffer. </param>
	/// <param name="pickFramebuffer"> The off-screen integer picking framebuffer. </param>
	/// <param name="width"> Picking framebuffer width. </param>
	/// <param name="height"> Picking framebuffer height. </param>
	void Draw(GLuint defaultFramebuffer, GLuint pickFramebuffer, int width, int height);

	/// <summary>
	/// Render gizmo IDs into the off-screen picking framebuffer for hover and drag hit-testing.
	/// </summary>
	void DrawPick();

	/// <summary>
	/// Clear gizmos.
	/// </summary>
	void Clear();
};

#endif
