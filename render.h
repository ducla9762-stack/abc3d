#ifndef _ABC3D_RENDER_H_
#define _ABC3D_RENDER_H_

#define GLEW_STATIC
#include <GL/glew.h>

#include <iostream>
#include <chrono>

#include "cyGL.h"
#include "cyMatrix.h"

#include "helper.h"
#include "Gizmo.h"
#include "scene.h"
#include "renderBufferBuilder.h"
#include "selectionManager.h"
#include "camera.h"

using std::chrono::steady_clock;
using std::chrono::milliseconds;
using std::chrono::duration_cast;

class Renderer {
public:
	virtual void Draw(GLuint defaultFrameBuffer) = 0;
	virtual bool Initialize(GLuint defaultFrameBuffer) = 0;
	virtual void Resize(int width, int height, GLuint defaultFrameBuffer) = 0;
};

#pragma region Old Arrow Renderer
/// <summary>
/// Renders world-space arrows to the framebuffer with a given View-Projection matrix.
/// </summary>
class ArrowRenderer {
	// Generic arrow geometry data
	static const std::vector<Vec3f> VERTICES;
	static const std::vector<Vec3u> TRIANGLES;

	// Data for OpenGL operations
	GLSLProgram program; // Shader program
	GLuint vao = 0, vbo = 0, ebo = 0; // Vertex Array Obj, Vertex Buf Obj, Element Buf Obj
	GLuint instancedVbo = 0; // VBO for arrow data (Arrow struct)
	GLsizei instanceCount = 0; // Number of arrows

public:

	struct Arrow {
		Vec3f base;
		Vec3f dir;
		float length;
		float radius;
		float tipHeight;
		Vec3f color;
		Vec2u ids;
	};

	/// <summary>
	/// Initializes OpenGL state (program, buffers, and vao).
	/// Must be called before anything else.
	/// </summary>
	/// <returns>True if the shader program compiled. Otherwise, false.</returns>
	bool Initialize();

	/// <summary>
	/// Draws arrows to framebuffer.
	/// </summary>
	void Draw();

	/// <summary>
	/// Sets the View-Projection transformation.
	/// </summary>
	/// <param name="data">View-Projection Matrix</param>
	void SetViewProjectionUniform(float data[16], float fov, int viewportHeight);

	/// <summary>
	/// Sets the arrows to draw.
	/// </summary>
	/// <param name="arrows">List of arrows</param>
	/// <param name="usage">Usage hint (GL_STATIC_DRAW by default)</param>
	void SetArrows(const std::vector<Arrow>& arrows, GLenum usage = GL_STATIC_DRAW);

	/// <summary>
	/// Creates a vector of arrows to be drawn.
	/// </summary>
	/// <returns> Arrows to draw. </returns>
	std::vector<Arrow> CreateArrows(const Vec3f& base, uint hoveredAxis);
};

#pragma endregion

class GizmoRenderer {
public:
	enum class Type { Translation, Scale, Rotation };

private:
	/// <summary>
	/// The complete gizmo to draw. Has it's own geometry data, buffers, and shader program.
	/// </summary>
	struct Gizmo {
		// Generic geometry data
		std::vector<Vec3f> VERTICES;
		std::vector<Vec3u> TRIANGLES;

		// Data for OpenGL operations
		GLSLProgram program; // Shader program
		GLuint vao = 0, vbo = 0, ebo = 0; // Vertex Array Obj, Vertex Buf Obj, Element Buf Obj
		GLuint instancedVbo = 0; // VBO for gizmo data
		GLsizei instanceCount = 0; // Number of instances

		// The list of settings for each piece of the gizmo.
		std::vector<GizmoInstanceSettings> settings;

		/// <summary>
		/// Initialize the gizmo.
		/// Builds shader program, initializes buffers, and set geometry and setting data.
		/// </summary>
		/// <param name="vsPath"> Path to the vertex shader. </param>
		/// <param name="fsPath"> Path to the fragment shader. </param>
		/// <param name="settings"> Reference to the gizmo instance settings. </param>
		/// <param name="vertices"> Reference to the vertex data. </param>
		/// <param name="faces"> Reference to the face data. </param>
		/// <returns></returns>
		bool initialize(const char* vsPath, const char* fsPath, const std::vector<Vec3f>& vertices, const std::vector<Vec3u>& faces);

		/// <summary>
		/// Draw the gizmo.
		/// </summary>
		void draw();

		/// <summary>
		/// Sets the View-Projection transformation.
		/// </summary>
		/// <param name="data">View-Projection Matrix</param>
		void setViewProjectionUniform(float data[16], float fov, int viewportHeight);

		/// <summary>
		/// Sets the gizmos to draw.
		/// </summary>
		/// <param name="maces">List of maces</param>
		/// <param name="usage">Usage hint (GL_STATIC_DRAW by default)</param>
		void setGizmos(const std::vector<GizmoInstanceSettings>& gizmos, GLenum usage = GL_STATIC_DRAW);

		/// <summary>
		/// Clears the gizmo.
		/// </summary>
		void clear();
	};

	// Settings for gizmo instances.
	std::vector<GizmoInstanceSettings> arrows; // translation.
	std::vector<GizmoInstanceSettings> maces; // scale.
	// todo: rotation;

	// Gizmo programs.
	Gizmo translation;
	Gizmo scale;
	// todo: rotation;

public:
	/// <summary>
	/// Initializes OpenGL state (program, buffers, and vao).
	/// Must be called before anything else.
	/// </summary>
	/// <returns> True if the shader programs were compiled. Otherwise, false. </returns>
	bool Initialize();

	/// <summary>
	/// Updates the gizmos draw position and color.
	/// </summary>
	/// <param name="base"> The position to draw the gizmo. </param>
	/// <param name="hoveredAxis"> The axis being hovered on. </param>
	/// <param name="type"> The type of gizmo to draw. </param>
	void UpdateGizmos(const Vec3f& base, uint hoveredAxis, Type type);

	/// <summary>
	/// Sets the initial settings for each gizmo.
	/// </summary>
	/// <param name="arrows"></param>
	/// <param name="maces"></param>
	void SetGizmoInstanceSettings(const std::vector<GizmoInstanceSettings>& arrows, const std::vector<GizmoInstanceSettings>& maces);

	/// <summary>
	/// Sets the View-Projection transformation for all gizmos.
	/// </summary>
	/// <param name="data">View-Projection Matrix</param>
	void SetViewProjectionUniform(float data[16], float fov, int viewportHeight);

	/// <summary>
	/// Draw the gizmos.
	/// </summary>
	void Draw();

	/// <summary>
	/// Clears all gizmos.
	/// </summary>
	void Clear();
};

class ModelingRenderer : public Renderer {
	// Shader Program
	GLSLProgram meshProgram;
	GLSLProgram pointProgram;
	GLSLProgram gridProgram;

	// Vertex Array Objects
	GLuint meshVAO;
	GLuint pointVAO;
	GLuint gridVAO;

	// Textures
	GLuint clrTex;
	GLuint idTex;
	GLuint selTex; // Selection texture.

	// Buffers
	GLuint meshVBO; // For mesh data (positions, normals, ids, etc.)
	GLuint pointVBO; // Holds unique mesh vertex positions for rendering vertex points
	GLuint pointIdVBO; // Holds mesh id and offset for rendering vertex points
	GLuint fbo; // Framebuffer
	GLuint fboDepth; // Depth buffer
	GLuint pbo; // Pixel Buffer Object for pixel streaming

	// Transformations
	Matrix4f viewTransform;
	Matrix4f projection;

	// Camera Parameters
	CameraType camType = ORBIT;
	OrbitCamera orbitCamera;
	FreeCamera freeCamera;
	float fov = 90.0f;

	std::vector<Vec3f> sceneVertices;
	std::vector<Vec2u> vertObjData;

	RenderBufferBuilder bufBuilder;

	uint meshVertDrawCount = 0;
	uint pointVertDrawCount = 0;

	float pointOffset = 0.01f; // For rendering vertex points

	bool displayPoints = true;
	bool displayGrid = true;

	int width = 0;
	int height = 0;

	std::vector<Vec3u> idSelectBuf;
	GLuint hoveredItems[2][3] = {
		{ MAX_UINT, MAX_UINT, MAX_UINT },
		{ MAX_UINT, MAX_UINT, MAX_UINT }
	};
	bool hoverStateInitialized = false;

	//ArrowRenderer arrowRenderer;
	//MaceRenderer maceRenderer;
	GizmoRenderer gizmoRenderer;

public:

	void ToggleGrid();
	void ToggleVertexPoints();

	//Saving needs to read the current value of grid, vertex points visible
	//current value and loading needs to set an value 
	bool GridVisible() const; 
	bool VertexPointsVisible() const;
	void SetGridVisible(bool visible); 
	void SetVertexPointsVisible(bool visible); 

	void UpdateCamera();

	void mouseDrag(const Vec2f& drag) {
		if (camType == ORBIT)	{ orbitCamera.mouseDrag(drag); }
		else					{ freeCamera.mouseDrag(drag); }
		UpdateCamera();
	}

	void mouseWheel(float dx, float dy) {
		if (camType == ORBIT)	{ orbitCamera.mouseWheel(dx, dy); }
		else					{ freeCamera.mouseWheel(dx, dy); }
		UpdateCamera();
	}

	CameraType GetActiveCamType() const { return camType; }

	void setCameraPosition(const Vec3f& pos) {
		if (camType == ORBIT)	{ orbitCamera.setPosition(pos); }
		else					{ freeCamera.setPosition(pos); }
		UpdateCamera();
	}

	void setOrbitOrigin(const Vec3f& origin) { orbitCamera.setOrigin(origin); UpdateCamera(); }
	const Camera* GetCamera() const { return camType == ORBIT ? (Camera*) &orbitCamera : (Camera*) &freeCamera; }

	void toggleCamType() {
		if (camType == ORBIT) {
			camType = FREE;
			freeCamera.setPosition(orbitCamera.position());
			freeCamera.setForward(orbitCamera.forward());
		}
		else {
			camType = ORBIT;
			orbitCamera.setOrigin(Vec3f(0.0f));
			orbitCamera.setPosition(freeCamera.position());
		}

		UpdateCamera();
	}

	void setCameraMode(CameraType mode) {
		if (mode == camType) { return; }
		toggleCamType();
	}

	Vec3u SelectionIds(uint x, uint y);

	bool Initialize(GLuint defaultFrameBuffer) override;

	void Draw(GLuint defaultFrameBuffer) override;

	void Resize(int width, int height, GLuint defaultFrameBuffer) override;

	void UpdateRenderBuffers(const Scene& scene);

	void UpdateGizmos(const Vec3f& base, uint hoveredAxis, GizmoRenderer::Type type);

	void ClearGizmos();

	void UpdateSelectionTexture(std::vector<bool> selections);

	/// <summary>
	/// Sends selection data to the GPU via uniforms for visualization.
	/// 
	/// NOTE: This is a mostly AI generated function, and might not represent the best possible
	/// implementation.
	/// </summary>
	/// <param name="selectedItems"></param>
	void UpdateSelection(const std::vector<SelectionItem>& selectedItems, Element element);

	// AI-generated code: Renderer interface for active-element hover state.
	/// Updates the active-element hover state from raw framebuffer IDs.
	bool UpdateHover(const Vec3u& selectionIds, Element activeElement, const Scene& scene);

private:

	void SetPerspectiveAndViewport();

	void UpdateProjectionUniforms();

	void updateUniforms();

	void updateTextureUniforms();

	void InitFramebuffer(GLuint defaultFrameBuffer);
};

#endif
