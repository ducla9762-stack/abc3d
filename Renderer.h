#ifndef _ABC3D_RENDER_2_0_H_
#define _ABC3D_RENDER_2_0_H_

#define GLEW_STATIC
#include <GL/glew.h>

#include <iostream>
#include <chrono>

#include <cyGL.h>
#include <cyMatrix.h>

#include "helper.h"
#include "selectionManager.h"
#include "scene.h"
#include "mesh.h"
#include "camera.h"
#include "renderBufferBuilder.h"

#undef max

inline const char* HEADER = "#version 450 core\n #define MAX_OBJECT_COUNT 64\n";

const std::array<Vec3f, 8> TRI_CUBE_VERTICES = {
	Vec3f(0.5f, 0.5f, -0.5f),
	Vec3f(0.5f, -0.5f, -0.5f),
	Vec3f(0.5f, 0.5f, 0.5f),
	Vec3f(0.5f, -0.5f, 0.5f),
	Vec3f(-0.5f, 0.5f, -0.5f),
	Vec3f(-0.5f, -0.5f, -0.5f),
	Vec3f(-0.5f, 0.5f, 0.5f),
	Vec3f(-0.5f, -0.5f, 0.5f)
};

const std::array<Vec3u, 12> TRI_CUBE_ELEMENTS = {
	Vec3u(0, 4, 6),
	Vec3u(0, 6, 2),
	Vec3u(3, 2, 6),
	Vec3u(3, 6, 7),
	Vec3u(7, 6, 4),
	Vec3u(7, 4, 5),
	Vec3u(5, 1, 3),
	Vec3u(5, 3, 7),
	Vec3u(1, 0, 2),
	Vec3u(1, 2, 3),
	Vec3u(5, 4, 0),
	Vec3u(5, 0, 1)
};

constexpr float HALF_SQR_2 = 0.7071068f;
const std::vector<Vec3f> TRI_CYLINDER_VERTICES = {
	// Cylinder Base
	Vec3f(1.0f, 0.0f, 0.0f), Vec3f(HALF_SQR_2, 0.0f, -HALF_SQR_2), Vec3f(0.0f, 0.0f, -1.0f),
	Vec3f(-HALF_SQR_2, 0.0f, -HALF_SQR_2), Vec3f(-1.0f, 0.0f, 0.0f), Vec3f(-HALF_SQR_2, 0.0f, HALF_SQR_2),
	Vec3f(0.0f, 0.0f, 1.0f), Vec3f(HALF_SQR_2, 0.0f, HALF_SQR_2),
	// Cylinder Top
	Vec3f(1.0f, 0.0f, 0.0f), Vec3f(HALF_SQR_2, 0.0f, -HALF_SQR_2), Vec3f(0.0f, 0.0f, -1.0f),
	Vec3f(-HALF_SQR_2, 0.0f, -HALF_SQR_2), Vec3f(-1.0f, 0.0f, 0.0f), Vec3f(-HALF_SQR_2, 0.0f, HALF_SQR_2),
	Vec3f(0.0f, 0.0f, 1.0f), Vec3f(HALF_SQR_2, 0.0f, HALF_SQR_2),
};

const std::vector<Vec3u> TRI_CYLINDER_ELEMENTS = {
	// Clinder Base
	Vec3u(0, 1, 2), Vec3u(0, 2, 3), Vec3u(0, 3, 4), Vec3u(0, 4, 5), Vec3u(0, 5, 6), Vec3u(0, 6, 7),
	// Cyclinder Top
	Vec3u(8, 10, 9), Vec3u(8, 11, 10), Vec3u(8, 12, 11), Vec3u(8, 13, 12), Vec3u(8, 14, 13), Vec3u(8, 15, 14),
	// Cylinder Sides
	Vec3u(0, 8, 1), Vec3u(1, 8, 9),
	Vec3u(1, 9, 2), Vec3u(2, 9, 10),
	Vec3u(2, 10, 3), Vec3u(3, 10, 11),
	Vec3u(3, 11, 4), Vec3u(4, 11, 12),
	Vec3u(4, 12, 5), Vec3u(5, 12, 13),
	Vec3u(5, 13, 6), Vec3u(6, 13, 14),
	Vec3u(6, 14, 7), Vec3u(7, 14, 15),
	Vec3u(7, 15, 0), Vec3u(0, 15, 8)
};

class VertexRenderer {
	GLuint vao;
	
	enum Uniform : int { SCALE = 0, VIEW_PROJ = 1 };
	enum Attribute : GLuint { VERT = 0, POS = 1, ID = 2 };

	struct Buffers {
		GLuint vbo;
		GLuint ebo;
		GLuint pos;
		GLuint id;
	} bufs;

	float scale;

	uint count;

	GLSLProgram prog;

public:
	bool Initialize() {
		// Generate and bind VAO
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		// Generate buffer handles
		glGenBuffers(4, &bufs.vbo);

		// Generate vertex buffer object for unit cube vertices
		glBindBuffer(GL_ARRAY_BUFFER, bufs.vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * TRI_CUBE_VERTICES.size() * 3, TRI_CUBE_VERTICES.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(VERT);
		glVertexAttribPointer(VERT, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

		// Generate element buffer object for unit cube triangles
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufs.ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * TRI_CUBE_ELEMENTS.size() * 3, TRI_CUBE_ELEMENTS.data(), GL_STATIC_DRAW);

		// Generate instanced position buffer
		glBindBuffer(GL_ARRAY_BUFFER, bufs.pos);
		glEnableVertexAttribArray(POS);
		glVertexAttribPointer(POS, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
		glVertexAttribDivisor(POS, 1);

		// Generate instanced id buffer
		glBindBuffer(GL_ARRAY_BUFFER, bufs.id);
		glEnableVertexAttribArray(ID);
		glVertexAttribIPointer(ID, 2, GL_UNSIGNED_INT, 0, (GLvoid*)0);
		glVertexAttribDivisor(ID, 1);

		// Unbind VAO & Buffers
		glBindVertexArray(0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		bool success = prog.BuildFiles("assets/shaders/VertexDraw.vert", "assets/shaders/VertexDraw.frag",
			nullptr, nullptr, nullptr, HEADER, &std::cout);
		if (!success) {
			std::cout << "Failed to compile vertex shader program!" << std::endl;
			return false;
		}

		prog.Bind();

		prog.RegisterUniform(SCALE, "scale");
		prog.RegisterUniform(VIEW_PROJ, "viewProj");
		prog.SetUniform(SCALE, 0.2f);

		return true;
	}

	void Draw() {
		if (count == 0) { return; }

		glBindVertexArray(vao);

		prog.Bind();
		glDrawElementsInstanced(GL_TRIANGLES, TRI_CUBE_ELEMENTS.size() * 3, GL_UNSIGNED_INT, 0, count);

		glBindVertexArray(0);
	}

	void SetScale(float scale) {
		this->scale = scale;
		prog.Bind();
		prog.SetUniform(SCALE, this->scale);
	}

	void SetViewProjection(float data[16]) {
		prog.Bind();
		prog.SetUniformMatrix4(VIEW_PROJ, data);
	}

	void SetVertices(const std::vector<Vec2u>& ids, const std::vector<Vec3f>& pos, GLenum hint = GL_STATIC_DRAW) {
		if (ids.size() != pos.size()) { return; }

		count = uint(ids.size());

		glBindBuffer(GL_ARRAY_BUFFER, bufs.id);
		glBufferData(GL_ARRAY_BUFFER, sizeof(uint) * ids.size() * 2, ids.data(), hint);

		glBindBuffer(GL_ARRAY_BUFFER, bufs.pos);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * pos.size() * 3, pos.data(), hint);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
};

class EdgeRenderer {
	GLuint vao;

	enum Uniform : int { RADIUS = 1, VIEW_PROJ = 2 };
	enum Attribute : GLuint { POS = 0, P0 = 1, P1 = 2, ID = 3 };

	struct Buffers {
		GLuint vbo;
		GLuint ebo;
		GLuint edge;
		GLuint id;
	} bufs;

	float radius;

	uint count;

	GLSLProgram prog;

public:
	bool Initialize() { 
		// Generate and bind VAO
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		// Generate buffer handles
		glGenBuffers(4, &bufs.vbo);

		// Generate vertex buffer object for unit cube vertices
		glBindBuffer(GL_ARRAY_BUFFER, bufs.vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * TRI_CYLINDER_VERTICES.size() * 3, TRI_CYLINDER_VERTICES.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(POS);
		glVertexAttribPointer(POS, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

		// Generate element buffer object for unit cube triangles
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufs.ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * TRI_CYLINDER_ELEMENTS.size() * 3, TRI_CYLINDER_ELEMENTS.data(), GL_STATIC_DRAW);

		// Generate instanced position buffer
		glBindBuffer(GL_ARRAY_BUFFER, bufs.edge);

		glEnableVertexAttribArray(P0);
		glVertexAttribPointer(P0, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3f) * 2, (GLvoid*)0);
		glVertexAttribDivisor(P0, 1);

		glEnableVertexAttribArray(P1);
		glVertexAttribPointer(P1, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3f) * 2, (GLvoid*)sizeof(Vec3f));
		glVertexAttribDivisor(P1, 1);

		// Generate instanced id buffer
		glBindBuffer(GL_ARRAY_BUFFER, bufs.id);
		glEnableVertexAttribArray(ID);
		glVertexAttribIPointer(ID, 2, GL_UNSIGNED_INT, 0, (GLvoid*)0);
		glVertexAttribDivisor(ID, 1);

		// Unbind VAO & Buffers
		glBindVertexArray(0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		bool success = prog.BuildFiles("assets/shaders/EdgeDraw.vert", "assets/shaders/EdgeDraw.frag",
			nullptr, nullptr, nullptr, HEADER, &std::cout);
		if (!success) {
			std::cout << "Failed to compile edge shader program!" << std::endl;
			return false;
		}

		prog.Bind();

		prog.RegisterUniform(RADIUS, "radius");
		prog.RegisterUniform(VIEW_PROJ, "viewProj");
		prog.SetUniform(RADIUS, 0.05f);

		return true;
	}

	void Draw() {
		if (count == 0) { return; }

		glBindVertexArray(vao);

		prog.Bind();
		glDrawElementsInstanced(GL_TRIANGLES, TRI_CYLINDER_ELEMENTS.size() * 3, GL_UNSIGNED_INT, 0, count);

		glBindVertexArray(0);
	}

	void SetRadius(float radius) {
		this->radius = radius;
		prog.Bind();
		prog.SetUniform1(RADIUS, &this->radius);
	}

	void SetViewProjection(float data[16]) {
		prog.Bind();
		prog.SetUniformMatrix4(VIEW_PROJ, data);
	}

	void SetEdges(const std::vector<Vec2u>& ids, const std::vector<Vec3f>& edges, GLenum hint = GL_STATIC_DRAW) {
		if (ids.size() != edges.size() / 2) { return; }

		count = uint(ids.size());

		glBindBuffer(GL_ARRAY_BUFFER, bufs.id);
		glBufferData(GL_ARRAY_BUFFER, sizeof(uint) * ids.size() * 2, ids.data(), hint);

		glBindBuffer(GL_ARRAY_BUFFER, bufs.edge);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * edges.size() * 3, edges.data(), hint);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
};

class GridRenderer {
	GLSLProgram prog;
	GLuint vao;
public:
	bool Initialize() {
		glGenVertexArrays(1, &vao);

		bool success = prog.BuildFiles("assets/shaders/Grid.vert", "assets/shaders/Grid.frag");
		if (!success) {
			std::cout << "Failed to compile grid shader program!" << std::endl;
			return false;
		}

		prog.Bind();
		prog.RegisterUniform(0, "mvp");

		return true;
	}

	void Draw() {
		glBindVertexArray(vao);
		prog.Bind();
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
	}

	void SetTransform(float data [16]) {
		prog.Bind();
		prog.SetUniformMatrix4(0, data);
	}
};

class ABC3DRenderer {

	struct Renderers {
		GridRenderer grid;
		VertexRenderer vert;
		EdgeRenderer edge;
	} rndrs;

	enum TexBindings : int { POS = 0, NORM = 1, ID = 2 };

	enum Uniforms : uint { VIEW = 0 };

	struct Textures {
		GLuint geom;
		GLuint norm;
		GLuint id;
	} tex;

	struct VertexArrayObjects {
		GLuint mesh;
		GLuint frame;
	} vao;

	struct Buffers {
		GLuint meshVBO;
		GLuint matUBO;
		GLuint clrSSBO;
		void Initialize(){ 
			glGenBuffers(sizeof(Buffers) / sizeof(GLuint), &meshVBO);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, clrSSBO);
			glBindBufferBase(GL_UNIFORM_BUFFER, 0, matUBO);
		}
	} bufs;

	struct Programs {
		GLSLProgram mesh;
		GLSLProgram shade;
	} prog;

	struct FrameBuffer {
		int width = 0;
		int height = 0;
		GLuint fbo;
		GLuint depthBuf;
	} frame;

	struct CameraSettings {
		OrbitCamera orbit;
		FreeCamera free;
		float fov = 70.0f;
		CameraType type = ORBIT;
	} cam;

	struct VisibleSettings {
		bool vertices = true;
		bool grid = true;
		bool edges = true;
		bool surfaces = true;
	} show;

	struct UpdateQueue {
		bool renderBufs = false;
		bool camera = false;
	} update;

	struct TransformCache {
		Matrix4f proj;
		Matrix4f view;
	} transform;

	struct Offsets {
		uint faces = 0;
		uint edges = 0;
		uint verts = 0;
		std::vector<uint> meshFaces;
		std::vector<uint> meshEdges;
		std::vector<uint> meshVerts;
		void Clear() { meshFaces.clear(); meshEdges.clear(); meshVerts.clear(); }
	} offset;

	struct Counts {
		uint faces;
		uint edges;
		uint verts;
	} count;

	RenderBufferBuilder bufBuilder;

	const Scene* scene;

	uint meshVertDrawCount;

public:

	bool Initialize(GLuint defaultFrameBuffer) {

		glGenVertexArrays(2, &vao.mesh);
		bufs.Initialize();

		// Create textures
		glGenTextures(3, &tex.geom);

		// Compile shaders
		bool success = prog.mesh.BuildFiles("assets/shaders/GeomPass.vert", "assets/shaders/GeomPass.frag",
			nullptr, nullptr, nullptr, HEADER, &std::cout);
		if (!success) {
			std::cout << "Failed to compile mesh shader program!" << std::endl;
			return false;
		}

		prog.mesh.RegisterUniform(VIEW, "view");
		
		success = prog.shade.BuildFiles("assets/shaders/BasicLightPass.vert", "assets/shaders/BasicLightPass.frag");
		if (!success) {
			std::cout << "Failed to compile deferred shader program!" << std::endl;
			return false;
		}

		prog.shade.SetUniform("posTex", TexBindings::POS);
		prog.shade.SetUniform("normTex", TexBindings::NORM);
		prog.shade.SetUniform("idTex", TexBindings::ID);

		rndrs.vert.Initialize();
		rndrs.edge.Initialize();
		rndrs.grid.Initialize();

		// Initialize gizmo renderer
		//if (!gizmoRenderer.Initialize()) {
			//return false;
		//}
		// Clear so we don't draw on startup.
		//gizmoRenderer.Clear();

		// The gizmo programs must exist before their camera uniforms are uploaded.
		//updateUniforms();

		// Set clear color & blend function
		glClearColor(0, 0, 0, 1);
		glEnable(GL_DEPTH_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		return true;
	}

	void UpdateRenderBuffers();

	void Draw(GLuint defaultFramebuffer) {

		if (update.renderBufs) { UpdateRenderBuffers(); update.renderBufs = false; }
		if (update.camera) { updateUniforms(); update.camera = false; }

		//glFinish();
		//steady_clock::time_point start = steady_clock::now();

		glBindFramebuffer(GL_FRAMEBUFFER, frame.fbo);

		glClear(GL_DEPTH_BUFFER_BIT);
		float clearClr[3] = { 0.0f, 0.0f, 0.0f };
		uint clearId[3] = { 0, 0, 0 };
		glClearBufferfv(GL_COLOR, 0, clearClr);
		glClearBufferfv(GL_COLOR, 1, clearClr);
		glClearBufferuiv(GL_COLOR, 2, clearId);

		if (show.surfaces) {
			glBindVertexArray(vao.mesh);
			prog.mesh.Bind();
			glDrawArrays(GL_TRIANGLES, 0, meshVertDrawCount);
		}

		if (show.vertices) {
			rndrs.vert.Draw();
		}

		if (show.edges) {
			rndrs.edge.Draw();
		}

		// Draw gizmos before checking the selection texture.
		/*glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		//arrowRenderer.Draw();
		gizmoRenderer.Draw();
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);*/

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, defaultFramebuffer);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBlitFramebuffer(0, 0, frame.width, frame.height, 0, 0, frame.width, frame.height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

		glDepthMask(GL_FALSE);

		glBindVertexArray(vao.frame);
		prog.shade.Bind();
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		if (show.grid) {
			glEnable(GL_BLEND);
			
			rndrs.grid.Draw();

			glDisable(GL_BLEND);
		}

		glDepthMask(GL_TRUE);

		//glFinish();
		//steady_clock::time_point end = steady_clock::now();
		//milliseconds ms = duration_cast<milliseconds>(end - start);
		//std::cout << "Milliseconds: " << ms.count() << std::endl;
	}

	void InitFramebuffer(GLuint defaultFrameBuffer) {
		// Delete old framebuffer
		glDeleteFramebuffers(1, &frame.fbo);
		glDeleteRenderbuffers(1, &frame.depthBuf);

		// Create new framebuffer
		glGenFramebuffers(1, &frame.fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, frame.fbo);

		// Allocate textures and render buffer

		glGenRenderbuffers(1, &frame.depthBuf);
		glBindRenderbuffer(GL_RENDERBUFFER, frame.depthBuf);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, frame.width, frame.height);

		glActiveTexture(GL_TEXTURE0 + TexBindings::POS);
		glBindTexture(GL_TEXTURE_2D, tex.geom);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, frame.width, frame.height, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glActiveTexture(GL_TEXTURE0 + TexBindings::NORM);
		glBindTexture(GL_TEXTURE_2D, tex.norm);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, frame.width, frame.height, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glActiveTexture(GL_TEXTURE0 + TexBindings::ID);
		glBindTexture(GL_TEXTURE_2D, tex.id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32UI, frame.width, frame.height, 0, GL_RGB_INTEGER, GL_UNSIGNED_INT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		// Attach textures and depth buffer
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, frame.depthBuf);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex.geom, 0);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, tex.norm, 0);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, tex.id, 0);

		// Enable all color attachments for drawing
		GLenum drawBuffers[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		glDrawBuffers(3, drawBuffers);

		// Make sure framebuffer is valid
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			std::cout << "Framebuffer is invalid!" << std::endl;
		}

		// Re-bind default framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, defaultFrameBuffer);
	}

	void UpdateProjectionUniforms();
	void SetPerspectiveAndViewport() {
		transform.proj.SetPerspective(degreesToRadians(cam.fov), float(frame.width) / float(frame.height), 0.1f, 1000.0f);
		UpdateProjectionUniforms();

		glViewport(0, 0, frame.width, frame.height);
	}

	void updateUniforms() {
		transform.view = cam.type == ORBIT ? cam.orbit.transform() : cam.free.transform();

		float data[16];
		transform.view.Get(data);
		prog.mesh.Bind();
		prog.mesh.SetUniformMatrix4(VIEW, data);

		Matrix4f viewProj = transform.proj * transform.view;
		viewProj.Get(data);

		rndrs.grid.SetTransform(data);
		rndrs.edge.SetViewProjection(data);
		rndrs.vert.SetViewProjection(data);

		//arrowRenderer.SetViewProjectionUniform(data, fov, height);
		//gizmoRenderer.SetViewProjectionUniform(data, fov, height);
	}

	void Resize(int width, int height, GLuint defaultFrameBuffer) {
		frame.width = width;
		frame.height = height;
		SetPerspectiveAndViewport();
		InitFramebuffer(defaultFrameBuffer);
	}

	std::vector<CompactTransform140> transformBuf;

	void UpdateUniformBuffer() {
		transformBuf.clear();
		for (uint i = 0; i < scene->MeshCount(); ++i) {
			transformBuf.emplace_back();
			scene->operator[](i)->getTransform(transformBuf.back());
		}

		glBindBuffer(GL_UNIFORM_BUFFER, bufs.matUBO);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(CompactTransform140) * transformBuf.size(), transformBuf.data(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	std::vector<uint> clrs;

	template <bool send>
	void ResetColorBuffer() {
		clrs.clear();

		uint num = count.faces + count.edges + count.verts;

		if (num == 0) {
			clrs.push_back(0);
		}
		else {
			clrs.insert(clrs.end(), num, 0);
		}

		if constexpr (send) {
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufs.clrSSBO);
			glBufferData(GL_SHADER_STORAGE_BUFFER, clrs.size() * sizeof(uint), clrs.data(), GL_DYNAMIC_DRAW);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		}
	}

	void UpdateTransformBuffer(const Scene& scene) {
		this->scene = &scene; update.renderBufs = true;
	}

	void UpdateRenderBuffers(const Scene& scene) {
		this->scene = &scene; update.renderBufs = true;
	}

	void ToggleGrid() { show.grid = !show.grid; }
	void ToggleVertexPoints() { show.vertices = !show.vertices; }

	bool GridVisible() const { return show.grid; }
	bool VertexPointsVisible() const { return show.vertices; }
	void SetGridVisible(bool visible) { show.grid = visible; }
	void SetVertexPointsVisible(bool visible) { show.vertices = visible; }

	void mouseDrag(const Vec2f& drag) {
		if (cam.type == ORBIT) { cam.orbit.mouseDrag(drag); }
		else { cam.free.mouseDrag(drag); }
		update.camera = true;
	}

	void mouseWheel(float dx, float dy) {
		if (cam.type == ORBIT) { cam.orbit.mouseWheel(dx, dy); }
		else { cam.free.mouseWheel(dx, dy); }
		update.camera = true;
	}

	CameraType GetActiveCamType() const { return cam.type; }

	void setCameraPosition(const Vec3f& pos) {
		if (cam.type == ORBIT) { cam.orbit.setPosition(pos); }
		else { cam.free.setPosition(pos); }
		update.camera = true;
	}

	void setOrbitOrigin(const Vec3f& origin) { cam.orbit.setOrigin(origin); update.camera = true; }
	const Camera* GetCamera() const { return cam.type == ORBIT ? (Camera*)&cam.orbit : (Camera*)&cam.free; }

	void toggleCamType() {
		if (cam.type == ORBIT) {
			cam.type = FREE;
			cam.free.setPosition(cam.orbit.position());
			cam.free.setForward(cam.orbit.forward());
		}
		else {
			cam.type = ORBIT;
			cam.orbit.setOrigin(Vec3f(0.0f));
			cam.orbit.setPosition(cam.free.position());
		}

		update.camera = true;
	}

	void setCameraMode(CameraType mode) {
		if (mode == cam.type) { return; }
		toggleCamType();
	}

	Vec2u SelectionIds(Element element, uint x, uint y) {
		if (x >= frame.width || y >= frame.height) {
			return Vec2u(MAX_UINT);
		}

		// read back ids
		Vec3u ids;
		glBindFramebuffer(GL_READ_FRAMEBUFFER, frame.fbo);
		glReadBuffer(GL_COLOR_ATTACHMENT2);	
		glReadPixels(x, y, 1, 1, GL_RGB_INTEGER, GL_UNSIGNED_INT, &ids.x);

		if (ids.z == 0) { return Vec2u(MAX_UINT); }

		switch (element) {
		case OBJECT:
			return Vec2u(ids.x, ids.z - 1);

		case FACE:
			if (ids.z != 1) { return Vec2u(ids.x, MAX_UINT); }
			return Vec2u(ids.x, ids.y - offset.meshFaces[ids.x]);

		case EDGE:
			if (ids.z == 3) { return Vec2u(ids.x, MAX_UINT); }
			if (ids.z == 2) { return Vec2u(ids.x, ids.y - offset.meshEdges[ids.x]); }

			// read back pos tex and find closest edge in face
			Vec3f pos;
			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glReadPixels(x, y, 1, 1, GL_RGB_INTEGER, GL_UNSIGNED_INT, &pos.x);
			return Vec2u(ids.x, scene->operator[](ids.x)->NearestEdge(ids.y - offset.meshEdges[ids.x], pos));

		case VERTEX:
			if (ids.z == 2) { return Vec2u(ids.x, MAX_UINT); }
			if (ids.z == 3) { return Vec2u(ids.x, ids.y - offset.meshVerts[ids.x]); }

			// read pack pos tex and find closest vertex in face
			Vec3f pos;
			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glReadPixels(x, y, 1, 1, GL_RGB_INTEGER, GL_UNSIGNED_INT, &pos.x);
			return Vec2u(ids.x, scene->operator[](ids.x)->NearestVertex(ids.y - offset.meshEdges[ids.x], pos));
		}
	}
};

#endif
