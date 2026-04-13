#ifndef _ABC3D_RENDER_H_
#define _ABC3D_RENDER_H_

#include <GL/glew.h>

#include <iostream>
#include <chrono>

#include "cyGL.h"
#include "cyMatrix.h"

#include "helper.h"
#include "scene.h"

using std::chrono::steady_clock;
using std::chrono::milliseconds;
using std::chrono::duration_cast;

class Renderer {
public:
	virtual void Draw() = 0;
	virtual bool Initialize() = 0;
	virtual void Resize(int width, int height) = 0;
	virtual void SetCamPos(const Vec3f& pos) = 0;
	virtual void SetCamTarget(const Vec3f& target) = 0;
	virtual void SetCamera(const Vec3f& pos, const Vec3f& target = ORIGIN) = 0;
	virtual Vec3f GetCamPos() = 0;
	virtual Vec3f GetCamTarget() = 0;
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

	std::vector<Vec3f> meshPosData;
	std::vector<Vec3f> meshNormData;
	std::vector<Vec2u> meshIdData;
	std::vector<float> meshDistData;
	std::vector<Vec3u> meshEdgeIdData;

	uint meshVertDrawCount = 0;
	uint pointVertDrawCount = 0;

	float pointScale = 1.02f; // For rendering vertex points

	bool displayPoints = true;
	bool displayGrid = true;

	int width = 0;
	int height = 0;

	std::vector<Vec3u> idSelectBuf;

public:

	void ToggleGrid();
	void ToggleVertexPoints();

	Vec3f GetCamPos() override;
	Vec3f GetCamTarget() override;

	void SetCamPos(const Vec3f& pos) override;
	void SetCamTarget(const Vec3f& target) override;
	void SetCamera(const Vec3f& pos, const Vec3f& target = ORIGIN) override;

	Vec3u SelectionIds(uint x, uint y);

	bool Initialize() override;

	void Draw() override;

	void Resize(int width, int height) override;

	void UpdateRenderBuffers(const Scene& scene);

private:

	void SetPerspectiveAndViewport();

	void UpdateProjectionUniforms();

	void updateUniforms();

	void InitFramebuffer();
};

#endif