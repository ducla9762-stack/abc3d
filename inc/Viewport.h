#ifndef _ABC3D_VIEWPORT_H_
#define _ABC3D_VIEWPORT_H_

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/names.h>
#include <FL/Fl_Spinner.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Button.H>
#undef max

#include <GL/glew.h>

#include <iostream>

#include "cyGL.h"
#include "cyMatrix.h"

#include "helper.h"
#include "structures.h"
#include "shapes.h"

// Useful constants
const Vec3f WORLD_Y = Vec3f(0.0f, 1.0f, 0.0f);
const Vec3f WORLD_X = Vec3f(1.0f, 0.0f, 0.0f);
const Vec3f WORLD_Z = Vec3f(0.0f, 0.0f, 1.0f);
const Vec3f ORIGIN = Vec3f(0.0f);

bool initGlew = true;

class Viewport : public Fl_Gl_Window {
	// Shader Program
	GLSLProgram meshProgram;
	GLSLProgram wireProgram;
	GLSLProgram gridProgram;

	// Vertex Array Objects
	GLuint meshVAO;
	GLuint wireVAO;
	GLuint gridVAO;

	// Textures
	GLuint clrTex;
	GLuint posTex;
	GLuint idTex;

	// Framebuffer and its depth buffer
	GLuint fbo;
	GLuint fboDepth;

	// Render Buffers
	GLuint meshVBO;
	GLuint wireVBO;
	GLuint wireElementBuffer;

	// Transformations
	Matrix4f viewTransform;
	Matrix4f projection;

	// Global State Data
	//int lastClickedButton = GLUT_LEFT_BUTTON;
	Vec2i mouseDragPrevPos = Vec2i(0);

	Vec3f camPos = Vec3f(0.0f, 90.0f, 5.0f); // Spherical Coordinates
	float fov = 90.0f;

	HalfEdgeMesh backup;
	HalfEdgeMesh mesh;
	uint wireElementCount = 0;

	std::vector<Vec3f> meshPosData;
	std::vector<Vec3f> meshNormData;
	std::vector<Vec3u> meshIdData;

	uint vertCount;

	float wireframeScale = 1.001f;

	bool displayWire = true;
	bool displayGrid = true;

	std::vector<Vec3f> posSelectBuf;
	std::vector<Vec3u> idSelectBuf;

	bool initialize() {
		// Create VAO for grid
		glGenVertexArrays(1, &gridVAO);

		// Create VAO for wire
		glGenVertexArrays(1, &wireVAO);
		glBindVertexArray(wireVAO);

		glGenBuffers(1, &wireVBO);
		glBindBuffer(GL_ARRAY_BUFFER, wireVBO);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

		glGenBuffers(1, &wireElementBuffer);

		// Create VAO for mesh
		glGenVertexArrays(1, &meshVAO);
		glBindVertexArray(meshVAO);

		glGenBuffers(1, &meshVBO);
		glBindBuffer(GL_ARRAY_BUFFER, meshVBO);

		updateRenderBuffers();

		// Create textures
		glGenTextures(1, &posTex);
		glGenTextures(1, &idTex);
		glGenTextures(1, &clrTex);

		// Initialize Shader Programs
		bool success = gridProgram.BuildFiles("../assets/shaders/Grid.vert", "../assets/shaders/Grid.frag");
		if (!success) {
			std::cout << "Failed to compile grid shader program!" << std::endl;
			//Fl::hide_all_windows();
			return false;
		}

		success = meshProgram.BuildFiles("../assets/shaders/Mesh.vert", "../assets/shaders/BlinnPhong.frag");
		if (!success) {
			std::cout << "Failed to compile mesh shader program!" << std::endl;
			//Fl::hide_all_windows();
			return false;
		}

		meshProgram.Bind();
		meshProgram.SetUniform("lightPosition", 0.0f, 0.0f, 0.0f);

		success = wireProgram.BuildFiles("../assets/shaders/Wireframe.vert", "../assets/shaders/Wireframe.frag");
		if (!success) {
			std::cout << "Failed to compile wireframe shader program!" << std::endl;
			//Fl::hide_all_windows();
			return false;
		}

		wireProgram.Bind();
		wireProgram.SetUniform("scale", wireframeScale);

		// Initialize Model-View uniform in mesh shader program.
		// Other uniforms are initialized with setPerspectiveAndViewport callback.
		initMeshModelViewUniform();

		// Set clear color & blend function
		glClearColor(0, 0, 0, 1);
		glEnable(GL_DEPTH_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		return true;
	}

	void draw() {
		if (!context_valid()) {
			bool success = initialize();
			if (!success) { return; }
		}
		if (!valid()) {
			setPerspectiveAndViewport();
			initFramebuffer();
		}

		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindVertexArray(meshVAO);
		meshProgram.Bind();
		glDrawArrays(GL_TRIANGLES, 0, vertCount);

		int selectBufSize = pixel_w() * pixel_h();

		glReadBuffer(GL_COLOR_ATTACHMENT1);
		posSelectBuf.resize(selectBufSize);
		glReadPixels(0, 0, pixel_w(), pixel_h(), GL_RGB, GL_FLOAT, posSelectBuf.data());

		glReadBuffer(GL_COLOR_ATTACHMENT2);
		idSelectBuf.resize(selectBufSize);
		glReadPixels(0, 0, pixel_w(), pixel_h(), GL_RGB_INTEGER, GL_UNSIGNED_INT, idSelectBuf.data());

		glReadBuffer(GL_COLOR_ATTACHMENT0);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBlitFramebuffer(0, 0, pixel_w(), pixel_h(), 0, 0, pixel_w(), pixel_h(), GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

		if (displayGrid) {
			glEnable(GL_BLEND);
			glDepthMask(GL_FALSE);

			glBindVertexArray(gridVAO);
			gridProgram.Bind();
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			glDisable(GL_BLEND);
			glDepthMask(GL_TRUE);
		}

		if (displayWire) {
			glBindVertexArray(wireVAO);
			wireProgram.Bind();
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wireElementBuffer);
			glDrawElements(GL_LINES, wireElementCount, GL_UNSIGNED_INT, 0);
		}
	}

	int handle(int event) {
		// Make sure Glew is initialized. This comes from FLTK's manual (modified).
		if (initGlew && event == FL_SHOW && shown()) {
			initGlew = false;
			make_current();
			glewInit();
		}

		if (context() == nullptr) { return 0; }

		make_current();
		// Process event
		switch (event) {
		case FL_PUSH:
			take_focus();
			return mousePush();
		case FL_DRAG:
			mouseDrag();
			break;
		case FL_MOUSEWHEEL:
			mouseWheel();
			break;
		case FL_KEYDOWN:
			keyDown();
			break;
		case FL_FOCUS:
		case FL_UNFOCUS:
			break;
		default:
			return 0; // Tell FLTK this widget doesn't want the event
		}

		// For debugging
		std::cout << "Window Event: " << fl_eventnames[event] << std::endl;

		return 1;
	}

	void mouseDrag() {
		float x = Fl::event_x();
		float y = Fl::event_y();

		Vec2f change;
		change.y = mouseDragPrevPos.y - y;
		change.x = x - mouseDragPrevPos.x;

		if (change.y == 0 && change.x == 0) return;

		camPos.x -= change.x;
		camPos.y += change.y;
		clamp(camPos.y, 1.0f, 179.0f);
		updateViewUniforms();

		mouseDragPrevPos.Set(x, y);
		redraw();
	}

	int mousePush() {
		if (Fl::event_button() != FL_LEFT_MOUSE) return 0;

		uint x = (float(Fl::event_x()) / w()) * pixel_w();
		uint y = (1.0f - (float(Fl::event_y()) / h())) * pixel_h();

		Vec3f pos = posSelectBuf.at(x + y * pixel_w());
		Vec3u id = idSelectBuf.at(x + y * pixel_w());

		std::cout << "Position: " << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
		std::cout << "Object Id: " << id.x << std::endl;
		std::cout << "Component Id: " << id.y << std::endl;
		std::cout << "Sub-Component Id: " << id.z << std::endl;

		mouseDragPrevPos.Set(Fl::event_x(), Fl::event_y());
	}

	void mouseWheel() {
		camPos.z += Fl::event_dy();
		if (camPos.z < 0.0f) { camPos.z = 0.0f; }

		camPos.x += Fl::event_dx();

		updateViewUniforms();
		redraw();
	}

	void keyDown() {
		switch (Fl::event_key()) {
		case 's':
			mesh.subdivide();
			std::cout << "subdivided underlying mesh" << std::endl;
			updateRenderBuffers();
			break;
		case 't':
			mesh.tesselate();
			std::cout << "tesselated underlying mesh" << std::endl;
			updateRenderBuffers();
			break;
		case 'r':
			mesh = backup;
			updateRenderBuffers();
			break;
		case 'w':
			displayWire = !displayWire;
			break;
		case 'g':
			displayGrid = !displayGrid;
			break;
		default:
			return;
		}
		redraw();
	}

	void setPerspectiveAndViewport() {
		projection.SetPerspective(degreesToRadians(fov), float(pixel_w()) / float(pixel_h()), 0.1f, 1000.0f);
		updateProjectionUniforms();

		glViewport(0, 0, pixel_w(), pixel_h());
	}

	void updateProjectionUniforms() {
		float data[16];

		projection.Get(data);
		meshProgram.Bind();
		meshProgram.SetUniformMatrix4("projection", data);

		Matrix4f viewProj = projection * viewTransform;
		viewProj.Get(data);
		gridProgram.Bind();
		gridProgram.SetUniformMatrix4("mvp", data);

		Matrix4f mvp = viewProj * mesh.getTransform();
		mvp.Get(data);
		wireProgram.Bind();
		wireProgram.SetUniformMatrix4("mvp", data);
	}

	void initMeshModelViewUniform() {
		Vec3f camera = sphericalToCartesian(camPos);
		viewTransform.SetView(camera, ORIGIN, WORLD_Y);

		float data[16];
		Matrix4f modelView = viewTransform * mesh.getTransform();
		modelView.Get(data);
		meshProgram.Bind();
		meshProgram.SetUniformMatrix4("modelView", data);

		Matrix3f normalTransform(modelView);
		normalTransform.Invert();
		normalTransform.Transpose();
		normalTransform.Get(data);
		meshProgram.SetUniformMatrix3("normalTransform", data);
	}

	void initFramebuffer() {
		// Delete old framebuffer
		glDeleteFramebuffers(1, &fbo);
		glDeleteRenderbuffers(1, &fboDepth);

		// Create new framebuffer
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);

		// Allocate textures and render buffer
		uint texWidth = pixel_w();
		uint texHeight = pixel_h();

		glGenRenderbuffers(1, &fboDepth);
		glBindRenderbuffer(GL_RENDERBUFFER, fboDepth);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, texWidth, texHeight);

		glBindTexture(GL_TEXTURE_2D, clrTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		glBindTexture(GL_TEXTURE_2D, posTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

		glBindTexture(GL_TEXTURE_2D, idTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32UI, texWidth, texHeight, 0, GL_RGB_INTEGER, GL_UNSIGNED_INT, NULL);

		// Attach textures and depth buffer
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fboDepth);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, clrTex, 0);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, posTex, 0);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, idTex, 0);

		// Enable all color attachments for drawing
		GLenum drawBuffers[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		glDrawBuffers(3, drawBuffers);

		// Make sure framebuffer is valid
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			std::cout << "Framebuffer is borked!" << std::endl;
		}
		
		// Re-bind default framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void updateModelUniforms() {
		float data[16];

		Matrix4f modelView = viewTransform * mesh.getTransform();
		modelView.Get(data);
		meshProgram.Bind();
		meshProgram.SetUniformMatrix4("modelView", data);

		Matrix3f normalTransform(modelView);
		normalTransform.Invert();
		normalTransform.Transpose();
		normalTransform.Get(data);
		meshProgram.SetUniformMatrix3("normalTransform", data);

		Matrix4f mvp = projection * modelView;
		mvp.Get(data);
		wireProgram.Bind();
		wireProgram.SetUniformMatrix4("mvp", data);
	}

	void updateViewUniforms() {
		Vec3f camera = sphericalToCartesian(camPos);
		viewTransform.SetView(camera, ORIGIN, WORLD_Y);

		updateModelUniforms();

		float data[16];
		Matrix4f viewProj = projection * viewTransform;
		viewProj.Get(data);
		gridProgram.Bind();
		gridProgram.SetUniformMatrix4("mvp", data);
	}

	void updateRenderBuffers() {
		std::vector<uint> edgeElements;
		mesh.GetEdgeElementBuffer(edgeElements);
		wireElementCount = edgeElements.size();

		glBindVertexArray(wireVAO);

		glBindBuffer(GL_ARRAY_BUFFER, wireVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * mesh.VertexCount() * 3, mesh.VertexPositions(), GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wireElementBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * edgeElements.size(), edgeElements.data(), GL_DYNAMIC_DRAW);

		meshPosData.resize(0);
		meshNormData.resize(0);
		meshIdData.resize(0);
		mesh.getRenderBuffers(meshPosData, meshNormData, meshIdData, 1);

		size_t meshPosDataSize = sizeof(float) * 3 * meshPosData.size();
		size_t meshNormDataSize = sizeof(float) * 3 * meshNormData.size();
		size_t meshIdDataSize = sizeof(uint) * 3 * meshIdData.size();
		size_t meshFloatDataSize = meshPosDataSize + meshNormDataSize;
		size_t size = meshFloatDataSize + meshIdDataSize;

		vertCount = meshPosData.size();

		glBindVertexArray(meshVAO);

		glBindBuffer(GL_ARRAY_BUFFER, meshVBO);
		glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);

		glBufferSubData(GL_ARRAY_BUFFER, 0, meshPosDataSize, meshPosData.data());
		glBufferSubData(GL_ARRAY_BUFFER, meshPosDataSize, meshNormDataSize, meshNormData.data());
		glBufferSubData(GL_ARRAY_BUFFER, meshFloatDataSize, meshIdDataSize, meshIdData.data());

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
		
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)meshPosDataSize);

		glEnableVertexAttribArray(2);
		glVertexAttribIPointer(2, 3, GL_UNSIGNED_INT, 0, (GLvoid*)meshFloatDataSize);
	}

public:
	Viewport(int x, int y, int w, int h, const char* label = nullptr)
		: Fl_Gl_Window(x, y, w, h, label)
	{
		Fl_Gl_Window::mode(FL_RGB8 | FL_ALPHA | FL_DEPTH | FL_DOUBLE | FL_OPENGL3);
		backup.load("../assets/models/Cube.stl");

		//backup.load(PYRAMID_VERTICES, PYRAMID_FACES);
		//backup.load(CUBE_VERTICES, CUBE_FACES); 
		mesh = backup;
	}

	void setScale(int component, float scale) {
		mesh.scale[component] = scale;
		updateModelUniforms();
		redraw();
	}

	void setRot(int component, float angle) {
		mesh.eulerRot[component] = angle;
		updateModelUniforms();
		redraw();
	}

	void setPos(int component, float pos) {
		mesh.pos[component] = pos;
		updateModelUniforms();
		redraw();
	}

	bool loadObj(const char* filepath) {
		bool success = backup.load(filepath);
		if (!success) return false;
		mesh = backup;
		updateRenderBuffers();
		redraw();
		return true;
	}
};

#endif
