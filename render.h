#ifndef _ABC3D_RENDER_H_
#define _ABC3D_RENDER_H_

#define GLEW_STATIC
#include <GL/glew.h>

#include <iostream>
#include <chrono>

#include "cyGL.h"
#include "cyMatrix.h"

#include "helper.h"
#include "scene.h"
#include "renderBufferBuilder.h"
#include "selectionManager.h"

using std::chrono::steady_clock;
using std::chrono::milliseconds;
using std::chrono::duration_cast;

class Renderer {
public:
	virtual void Draw(GLuint defaultFrameBuffer) = 0;
	virtual bool Initialize(GLuint defaultFrameBuffer) = 0;
	virtual void Resize(int width, int height, GLuint defaultFrameBuffer) = 0;
	virtual void SetCamPos(const Vec3f& pos) = 0;
	virtual void SetCamTarget(const Vec3f& target) = 0;
	virtual void SetCamera(const Vec3f& pos, const Vec3f& target = ORIGIN) = 0;
	virtual Vec3f GetCamPos() = 0;
	virtual Vec3f GetCamTarget() = 0;
};

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

/// <summary>
/// Renders world-space "mace" to the framebuffer with a given View-Projection matrix.
/// </summary>
class MaceRenderer {
	// Generic mace geometry data
	static const std::vector<Vec3f> VERTICES;
	static const std::vector<Vec3u> TRIANGLES;

	// Data for OpenGL operations
	GLSLProgram program; // Shader program
	GLuint vao = 0, vbo = 0, ebo = 0; // Vertex Array Obj, Vertex Buf Obj, Element Buf Obj
	GLuint instancedVbo = 0; // VBO for arrow data (mace struct)
	GLsizei instanceCount = 0; // Number of maces

public:

	struct Mace {
		Vec3f base;
		Vec3f dir;
		float length;
		float radius;
		float tipSize;
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
	/// Draws maces to framebuffer.
	/// </summary>
	void Draw();

	/// <summary>
	/// Sets the View-Projection transformation.
	/// </summary>
	/// <param name="data">View-Projection Matrix</param>
	void SetViewProjectionUniform(float data[16], float fov, int viewportHeight);

	/// <summary>
	/// Sets the maces to draw.
	/// </summary>
	/// <param name="maces">List of maces</param>
	/// <param name="usage">Usage hint (GL_STATIC_DRAW by default)</param>
	void SetMaces(const std::vector<Mace>& arrows, GLenum usage = GL_STATIC_DRAW);

	/// <summary>
	/// Creates a vector of maces to be drawn.
	/// </summary>
	/// <returns> Maces to draw. </returns>
	std::vector<Mace> CreateMaces(const Vec3f& base, uint hoveredAxis);
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
	Vec3f camPos = Vec3f(0.0f);
	Vec3f camTarget = Vec3f(0.0f);
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

	ArrowRenderer arrowRenderer;
	MaceRenderer maceRenderer;

public:

	void ToggleGrid();
	void ToggleVertexPoints();

	//Saving needs to read the current value of grid, vertex points visible
	//current value and loading needs to set an value 
	bool GridVisible() const; 
	bool VertexPointsVisible() const;
	void SetGridVisible(bool visible); 
	void SetVertexPointsVisible(bool visible); 

	Vec3f GetCamPos() override;
	Vec3f GetCamTarget() override;
	Vec3f GetCamUp();
	Vec3f GetCamRight();
	void GetCameraDirs(Vec3f& forward, Vec3f& up, Vec3f& right);

	void SetCamPos(const Vec3f& pos) override;
	void SetCamTarget(const Vec3f& target) override;
	void SetCamera(const Vec3f& pos, const Vec3f& target = ORIGIN) override;

	Vec3u SelectionIds(uint x, uint y);

	bool Initialize(GLuint defaultFrameBuffer) override;

	void Draw(GLuint defaultFrameBuffer) override;

	void Resize(int width, int height, GLuint defaultFrameBuffer) override;

	void UpdateRenderBuffers(const Scene& scene);

	/// <summary>
	/// Shows the translation arrows at the supplied world-space position.
	/// </summary>
	void UpdateArrows(const Vec3f& base, uint hoveredAxis);

	/// <summary>
	/// Shows the scale maces at the supplied world-space position.
	/// </summary>
	/// <param name="base"> Where the base of the maces will be drawn. </param>
	/// <param name="hoveredAxis"> The currently hovered over axis. </param>
	void UpdateMaces(const Vec3f& base, uint hoveredAxis);

	/// <summary>
	/// Hides all arrows.
	/// </summary>
	void ClearArrows();

	/// <summary>
	/// Hides all maces.
	/// </summary>
	void ClearMaces();

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
