#ifndef _ABC3D_RENDER_2_0_H_
#define _ABC3D_RENDER_2_0_H_

#define GLEW_STATIC
#include <GL/glew.h>

#include <iostream>
#include <chrono>

#include <cyGL.h>
#include <cyMatrix.h>

#include "helper.h"
#include "scene.h"
#include "mesh.h"
#include "camera.h"
#include "renderBufferBuilder.h"

const std::array<Vec3f, 8> CUBE_VERTEX_POSITIONS = {
	Vec3f(0.5f, 0.5f, -0.5f),
	Vec3f(0.5f, -0.5f, -0.5f),
	Vec3f(0.5f, 0.5f, 0.5f),
	Vec3f(0.5f, -0.5f, 0.5f),
	Vec3f(-0.5f, 0.5f, -0.5f),
	Vec3f(-0.5f, -0.5f, -0.5f),
	Vec3f(-0.5f, 0.5f, 0.5f),
	Vec3f(-0.5f, -0.5f, 0.5f)
};

const std::array<Vec3u, 12> CUBE_TRIANGLE_ELEMENTS = {
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

class VertexRenderer {
	GLuint vao;
	
	enum Uniform : int { SCALE = 1, VIEW_PROJ };
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
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * CUBE_VERTEX_POSITIONS.size() * 3, CUBE_VERTEX_POSITIONS.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(VERT);
		glVertexAttribPointer(VERT, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

		// Generate element buffer object for unit cube triangles
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufs.ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * CUBE_TRIANGLE_ELEMENTS.size() * 3, CUBE_TRIANGLE_ELEMENTS.data(), GL_STATIC_DRAW);

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

		bool success = prog.BuildFiles("assets/shaders/VertexDraw.vert", "assets/shaders/VertexDraw.frag");
		if (!success) {
			std::cout << "Failed to compile vertex shader program!" << std::endl;
			return false;
		}

		prog.Bind();

		prog.RegisterUniform(SCALE, "scale");
		prog.RegisterUniform(VIEW_PROJ, "viewProj");

		return true;
	}

	void Draw() {
		if (count == 0) { return; }

		glBindVertexArray(vao);

		prog.Bind();
		glDrawElementsInstanced(GL_TRIANGLES, CUBE_TRIANGLE_ELEMENTS.size() * 3, GL_UNSIGNED_INT, 0, count);

		glBindVertexArray(0);
	}

	void SetScale(float scale) {
		this->scale = scale;
		prog.Bind();
		prog.SetUniform1(SCALE, &this->scale);
	}

	void SetViewProjection(Matrix4f& viewProj) {
		prog.Bind();
		float data[16];
		viewProj.Get(data);
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
		prog.RegisterUniform(0, "viewProj");

		return true;
	}

	void Draw() {
		glBindVertexArray(vao);
		prog.Bind();
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
	}

	void SetTransform(Matrix4f& mvp) {
		prog.Bind();
		float data[16];
		mvp.Get(data);
		prog.SetUniformMatrix4(0, data);
	}
};

class Renderer {

	struct Renderers {
		GridRenderer grid;
		VertexRenderer vert;
	} rndrs;

	struct Textures {
		GLuint geom;
		GLuint norm;
		GLuint id;
	} tex;

	struct PixelBufferObjects {
		GLuint geom;
		GLuint id;
	} pbo;

	struct VertexArrayObjects {
		GLuint mesh;
		GLuint frame;
	} vao;

	struct Buffers {
		GLuint meshVBO;
	} bufs;

	struct Programs {
		GLSLProgram mesh;
		GLSLProgram defered;
	} prog;

	struct FrameBuffer {
		int width;
		int height;
		GLuint fbo;
		GLuint depthBuf;
	} frame;

	std::vector<uint> faceOffsets;
	std::vector<uint> edgeOffsets;
	std::vector<uint> vertOffsets;

	RenderBufferBuilder bufBuilder;

	const Scene* scene;

	bool updateRenderBuffers = false;
	bool updateCamera = false;
	uint meshVertDrawCount;

	bool displayVertices = false;
	bool displayGrid = true;

	bool Initialize(GLuint defaultFrameBuffer) {

		glGenVertexArrays(2, &vao.mesh);
		glGenBuffers(1, &bufs.meshVBO);
		glGenBuffers(2, &pbo.geom);

		// Create textures
		glGenTextures(3, &tex.geom);

		bool success = prog.mesh.BuildFiles("assets/shaders/Mesh.vert", "assets/shaders/Mesh.frag");
		if (!success) {
			std::cout << "Failed to compile mesh shader program!" << std::endl;
			return false;
		}

		success = prog.defered.BuildFiles("assets/shaders/Deferred.vert", "assets/shaders/Deferred.frag");
		if (!success) {
			std::cout << "Failed to compile deferred shader program!" << std::endl;
			return false;
		}

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

		if (updateRenderBuffers) { UpdateRenderBuffers(); updateRenderBuffers = false; }
		//if (updateCamera) { updateUniforms(); updateCamera = false; }

		//glFinish();
		//steady_clock::time_point start = steady_clock::now();

		glBindFramebuffer(GL_FRAMEBUFFER, frame.fbo);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindVertexArray(vao.mesh);
		prog.mesh.Bind();

		glDrawArrays(GL_TRIANGLES, 0, meshVertDrawCount);

		if (displayVertices) {
			rndrs.vert.Draw();
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

		glReadBuffer(GL_COLOR_ATTACHMENT1);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo.id);
		glReadPixels(0, 0, frame.width, frame.height, GL_RGB_INTEGER, GL_UNSIGNED_INT, 0);
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo.geom);
		glReadPixels(0, 0, frame.width, frame.height, GL_RGB, GL_FLOAT, 0);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, defaultFramebuffer);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindVertexArray(vao.frame);
		prog.defered.Bind();
		glDrawArrays(GL_LINE_STRIP, 0, 4);

		if (displayGrid) {
			glEnable(GL_BLEND);
			glDepthMask(GL_FALSE);

			rndrs.grid.Draw();

			glDisable(GL_BLEND);
			glDepthMask(GL_TRUE);
		}

		//void* selectBuf = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
		//if (selectBuf) {
		//	uint selectBufSize = width * height;
		//	idSelectBuf.resize(0);
		//	Vec3u* idSelectBuffer = (Vec3u*)selectBuf;
		//	idSelectBuf.insert(idSelectBuf.begin(), idSelectBuffer, idSelectBuffer + selectBufSize);
		//}
		//else {
		//	std::cout << "ERROR with mapping" << std::endl;
		//}
		//glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

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

		glBindTexture(GL_TEXTURE_2D, tex.geom);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, frame.width, frame.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glBindTexture(GL_TEXTURE_2D, tex.norm);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, frame.width, frame.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glBindTexture(GL_TEXTURE_2D, tex.id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32UI, frame.width, frame.height, 0, GL_RGB_INTEGER, GL_UNSIGNED_INT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		// Allocate pixel buffer objects for streaming
		uint pboSize = frame.width * frame.height * 3 * 4;

		glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo.geom);
		glBufferData(GL_PIXEL_PACK_BUFFER, pboSize, NULL, GL_STREAM_READ);

		glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo.id);
		glBufferData(GL_PIXEL_PACK_BUFFER, pboSize, NULL, GL_STREAM_READ);

		// Attach textures and depth buffer
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, frame.depthBuf);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex.geom, 0);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, tex.norm, 0);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, tex.id, 0);

		// Enable all color attachments for drawing
		GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, drawBuffers);

		// Make sure framebuffer is valid
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			std::cout << "Framebuffer is invalid!" << std::endl;
		}

		// Re-bind default framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, defaultFrameBuffer);
	}

	Matrix4f projection;
	float fov = 90.0f;

	void UpdateProjectionUniforms();
	void SetPerspectiveAndViewport() {
		projection.SetPerspective(degreesToRadians(fov), float(frame.width) / float(frame.height), 0.1f, 1000.0f);
		UpdateProjectionUniforms();

		glViewport(0, 0, frame.width, frame.height);
	}

	void UpdateProjectionUniforms() {
		/*Matrix3f viewTransform = camType == ORBIT ? orbitCamera.transform() : freeCamera.transform();

		float data[16];

		projection.Get(data);
		prog.mesh.Bind();
		prog.mesh.SetUniformMatrix4("projection", data);

		Matrix4f viewProj = projection * viewTransform;
		viewProj.Get(data);
		gridProgram.Bind();
		gridProgram.SetUniformMatrix4("mvp", data);
		pointProgram.Bind();
		pointProgram.SetUniformMatrix4("mvp", data);*/

		//arrowRenderer.SetViewProjectionUniform(data, fov, height);
		//gizmoRenderer.SetViewProjectionUniform(data, fov, height);
	}

	void updateUniforms() {
		/*Matrix3f viewTransform = camType == ORBIT ? orbitCamera.transform() : freeCamera.transform();

		float data[16];
		viewTransform.Get(data);
		meshProgram.Bind();
		meshProgram.SetUniformMatrix4("modelView", data);

		Matrix3f normalTransform(viewTransform);
		//normalTransform.Invert();
		//normalTransform.Transpose();
		normalTransform.Get(data);
		meshProgram.SetUniformMatrix3("normalTransform", data);

		Matrix4f viewProj = projection * viewTransform;
		viewProj.Get(data);
		gridProgram.Bind();
		gridProgram.SetUniformMatrix4("mvp", data);
		pointProgram.Bind();
		pointProgram.SetUniformMatrix4("mvp", data);*/

		//arrowRenderer.SetViewProjectionUniform(data, fov, height);
		//gizmoRenderer.SetViewProjectionUniform(data, fov, height);
	}

	void Resize(int width, int height, GLuint defaultFrameBuffer) {
		frame.width = width;
		frame.height = height;
		SetPerspectiveAndViewport();
		InitFramebuffer(defaultFrameBuffer);
	}

	void UpdateRenderBuffers() {
		uint offset = 0;
		for (uint i = 0; i < scene->MeshCount(); ++i) {
			const HalfEdgeMesh* mesh = scene->operator[](i);
			bufBuilder.AddMeshToVertexPosList(*mesh, i + 1, offset);
			vertOffsets.push_back(mesh->VertexCount());
			offset += vertOffsets.back();
		}

		rndrs.vert.SetVertices(bufBuilder.VertexIds(), bufBuilder.VertexPositions(), GL_DYNAMIC_DRAW);

		/*
		offset = 0;
		for (uint i = 0; i < scene->MeshCount(); ++i) {
			const HalfEdgeMesh* mesh = scene->operator[](i);
			bufBuilder.AddMeshToEdgeList(*mesh, i + 1, offset);
			edgeOffsets.push_back(mesh->EdgeCount());
			offset += edgeOffsets.back();
		}*/

		offset = 0;
		for (uint i = 0; i < scene->MeshCount(); ++i) {
			const HalfEdgeMesh* mesh = scene->operator[](i);
			bufBuilder.AddMeshToAttributes(*mesh, i + 1, offset);
			faceOffsets.push_back(mesh->FaceCount());
			offset += faceOffsets.back();
		}

		glBindVertexArray(vao.mesh);

		glBindBuffer(GL_ARRAY_BUFFER, bufs.meshVBO);
		glBufferData(GL_ARRAY_BUFFER, bufBuilder.Attributes().size() * sizeof(VertexAttributes), bufBuilder.Attributes().data(), GL_DYNAMIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexAttributes), (GLvoid*)offsetof(VertexAttributes, pos));

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexAttributes), (GLvoid*)offsetof(VertexAttributes, norm));

		glEnableVertexAttribArray(2);
		glVertexAttribIPointer(2, 2, GL_UNSIGNED_INT, sizeof(VertexAttributes), (GLvoid*)offsetof(VertexAttributes, ids));

};

#endif
