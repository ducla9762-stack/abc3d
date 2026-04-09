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
#include <chrono>

#include "cyGL.h"
#include "cyMatrix.h"

#include "helper.h"
#include "structures.h"

// Useful constants
const Vec3f WORLD_Y = Vec3f(0.0f, 1.0f, 0.0f);
const Vec3f WORLD_X = Vec3f(1.0f, 0.0f, 0.0f);
const Vec3f WORLD_Z = Vec3f(0.0f, 0.0f, 1.0f);
const Vec3f ORIGIN = Vec3f(0.0f);

bool initGlew = true;

using std::chrono::steady_clock;
using std::chrono::milliseconds;
using std::chrono::duration_cast;

class Viewport : public Fl_Gl_Window {
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

	// Global State Data
	Vec2i mouseDragPrevPos = Vec2i(0);

	Vec3f camPos = Vec3f(0.0f, 90.0f, 5.0f); // Spherical Coordinates
	float fov = 90.0f;

	HalfEdgeMesh backup;
	HalfEdgeMesh mesh;

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

	std::vector<Vec3u> idSelectBuf;

	bool initialize() {
		// Create VAO for grid
		glGenVertexArrays(1, &gridVAO);

		// Create VAO for vertex points
		glGenVertexArrays(1, &pointVAO);
		glBindVertexArray(pointVAO);

		glGenBuffers(1, &pointVBO);
		glBindBuffer(GL_ARRAY_BUFFER, pointVBO);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

		glGenBuffers(1, &pointIdVBO);
		glBindBuffer(GL_ARRAY_BUFFER, pointIdVBO);

		glEnableVertexAttribArray(1);
		glVertexAttribIPointer(1, 2, GL_UNSIGNED_INT, 0, (GLvoid*)0);

		// Create VAO for mesh
		glGenVertexArrays(1, &meshVAO);
		glBindVertexArray(meshVAO);

		glGenBuffers(1, &meshVBO);
		glBindBuffer(GL_ARRAY_BUFFER, meshVBO);

		updateRenderBuffers();

		// Create pixel buffer object for pixel streaming
		glGenBuffers(1, &pbo);

		// Create textures
		glGenTextures(1, &idTex);
		glGenTextures(1, &clrTex);

		// Initialize Shader Programs
		bool success = gridProgram.BuildFiles("../assets/shaders/Grid.vert", "../assets/shaders/Grid.frag");
		if (!success) {
			std::cout << "Failed to compile grid shader program!" << std::endl;
			//Fl::hide_all_windows();
			return false;
		}

		success = meshProgram.BuildFiles("../assets/shaders/Mesh.vert", "../assets/shaders/Mesh.frag");
		if (!success) {
			std::cout << "Failed to compile mesh shader program!" << std::endl;
			//Fl::hide_all_windows();
			return false;
		}

		meshProgram.Bind();
		meshProgram.SetUniform("lightPosition", 0.0f, 0.0f, 0.0f);

		success = pointProgram.BuildFiles("../assets/shaders/PointDrawing.vert", "../assets/shaders/StaticColor.frag");
		if (!success) {
			std::cout << "Failed to compile wireframe shader program!" << std::endl;
			//Fl::hide_all_windows();
			return false;
		}

		pointProgram.Bind();
		pointProgram.SetUniform("scale", pointScale);

		// Initialize Model-View uniform in mesh shader program.
		// Other uniforms are initialized with setPerspectiveAndViewport callback.
		initMeshModelViewUniform();
		// Initialize the selection boolean for the mesh shader program.
		// By default nothing should be selected.
		initMeshModelSelectionBoolean();

		// Set clear color & blend function
		glClearColor(0, 0, 0, 1);
		glEnable(GL_DEPTH_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glPointSize(20.0f);

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

		glFinish();
		steady_clock::time_point start = steady_clock::now();

		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindVertexArray(meshVAO);
		meshProgram.Bind();
		glDrawArrays(GL_TRIANGLES, 0, meshVertDrawCount);

		if (displayPoints) {
			glBindVertexArray(pointVAO);
			pointProgram.Bind();
			glDrawArrays(GL_POINTS, 0, pointVertDrawCount);
		}

		glReadBuffer(GL_COLOR_ATTACHMENT1);
		glReadPixels(0, 0, pixel_w(), pixel_h(), GL_RGB_INTEGER, GL_UNSIGNED_INT, 0);
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

		void* selectBuf = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
		if (selectBuf) {
			uint selectBufSize = pixel_w() * pixel_h();
			idSelectBuf.resize(0);
			Vec3u* idSelectBuffer = (Vec3u*)selectBuf;
			idSelectBuf.insert(idSelectBuf.begin(), idSelectBuffer, idSelectBuffer + selectBufSize);
		}
		else {
			std::cout << "ERROR with mapping" << std::endl;
		}
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

		/*int selectBufSize = pixel_w() * pixel_h();

		glReadBuffer(GL_COLOR_ATTACHMENT1);
		posSelectBuf.resize(selectBufSize);
		glReadPixels(0, 0, pixel_w(), pixel_h(), GL_RGB, GL_FLOAT, posSelectBuf.data());

		glReadBuffer(GL_COLOR_ATTACHMENT2);
		idSelectBuf.resize(selectBufSize);
		glReadPixels(0, 0, pixel_w(), pixel_h(), GL_RGB_INTEGER, GL_UNSIGNED_INT, idSelectBuf.data());

		glReadBuffer(GL_COLOR_ATTACHMENT0);*/

		glFinish();
		steady_clock::time_point end = steady_clock::now();
		milliseconds ms = duration_cast<milliseconds>(end - start);
		std::cout << "Milliseconds: " << ms.count() << std::endl;
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

		Vec3u id = idSelectBuf.at(x + y * pixel_w());

		// If the object id is not 0, e.g. an object has been touched, set the selection bool.
		if (id.x != 0)
		{
			setMeshSelectionBoolean(true);
		}
		// Else the object id is 0, no object has been touched, reset the selection bool.
		else
		{
			setMeshSelectionBoolean(false);
		}

		std::cout << "Object Id: " << id.x << std::endl;
		std::cout << "Component Id: " << id.y << std::endl;
		std::cout << "Sub-Component Id: " << id.z << std::endl;

		mouseDragPrevPos.Set(Fl::event_x(), Fl::event_y());
		redraw();
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
		case 'p':
			displayPoints = !displayPoints;
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
		pointProgram.Bind();
		pointProgram.SetUniformMatrix4("mvp", data);
	}

	void setMeshSelectionBoolean(bool selected)
	{
		meshProgram.Bind();
		meshProgram.SetUniform("selected", selected);
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

	void initMeshModelSelectionBoolean() {
		meshProgram.Bind();
		meshProgram.SetUniform("selected", false);
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

		glBindTexture(GL_TEXTURE_2D, idTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32UI, texWidth, texHeight, 0, GL_RGB_INTEGER, GL_UNSIGNED_INT, NULL);

		// Allocate pixel buffer objects for streaming
		uint pboSize = texWidth * texHeight * 3 * (sizeof(float) + sizeof(uint));

		glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
		glBufferData(GL_PIXEL_PACK_BUFFER, pboSize, NULL, GL_STREAM_READ);

		// Attach textures and depth buffer
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fboDepth);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, clrTex, 0);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, idTex, 0);

		// Enable all color attachments for drawing
		GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, drawBuffers);

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
		pointProgram.Bind();
		pointProgram.SetUniformMatrix4("mvp", data);
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
		// Unique vertex data for drawing vertex points
		std::vector<Vec2u> vertObjData;
		vertObjData.insert(vertObjData.begin(), mesh.VertexCount(), Vec2u(1, 0));
		pointVertDrawCount = mesh.VertexCount();
		glBindVertexArray(pointVAO);
		glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * pointVertDrawCount * 3, mesh.VertexPositions(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, pointIdVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(uint) * pointVertDrawCount * 2, vertObjData.data(), GL_DYNAMIC_DRAW);

		// Mesh Data
		meshPosData.resize(0);
		meshNormData.resize(0);
		meshIdData.resize(0);
		meshEdgeIdData.resize(0);
		meshDistData.resize(0);
		mesh.getRenderBuffers(meshPosData, meshNormData, meshIdData, meshDistData, meshEdgeIdData, 1);

		size_t meshPosDataSize = sizeof(float) * 3 * meshPosData.size();
		size_t meshNormDataSize = sizeof(float) * 3 * meshNormData.size();
		size_t meshIdDataSize = sizeof(uint) * 2 * meshIdData.size();
		size_t meshDistDataSize = sizeof(float) * meshDistData.size();
		size_t meshEdgeIdDataSize = sizeof(uint) * 3 * meshEdgeIdData.size();
		size_t bufOffset1 = meshPosDataSize + meshNormDataSize;
		size_t bufOffset2 = bufOffset1 + meshIdDataSize;
		size_t bufOffset3 = bufOffset2 + meshDistDataSize;
		size_t size = bufOffset3 + meshEdgeIdDataSize;

		meshVertDrawCount = meshPosData.size();

		glBindVertexArray(meshVAO);

		glBindBuffer(GL_ARRAY_BUFFER, meshVBO);
		glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);

		glBufferSubData(GL_ARRAY_BUFFER, 0, meshPosDataSize, meshPosData.data());
		glBufferSubData(GL_ARRAY_BUFFER, meshPosDataSize, meshNormDataSize, meshNormData.data());
		glBufferSubData(GL_ARRAY_BUFFER, bufOffset1, meshIdDataSize, meshIdData.data());
		glBufferSubData(GL_ARRAY_BUFFER, bufOffset2, meshDistDataSize, meshDistData.data());
		glBufferSubData(GL_ARRAY_BUFFER, bufOffset3, meshEdgeIdDataSize, meshEdgeIdData.data());

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
		
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)meshPosDataSize);

		glEnableVertexAttribArray(2);
		glVertexAttribIPointer(2, 2, GL_UNSIGNED_INT, 0, (GLvoid*)bufOffset1);

		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 0, (GLvoid*)bufOffset2);

		glEnableVertexAttribArray(4);
		glVertexAttribIPointer(4, 3, GL_UNSIGNED_INT, 0, (GLvoid*)bufOffset3);
	}

public:
	Viewport(int x, int y, int w, int h, const char* label = nullptr)
		: Fl_Gl_Window(x, y, w, h, label)
	{
		Fl_Gl_Window::mode(FL_RGB8 | FL_ALPHA | FL_DEPTH | FL_DOUBLE | FL_OPENGL3);
		backup.load("../assets/models/box.obj");
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
