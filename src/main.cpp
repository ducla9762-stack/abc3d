#include "structures.h"

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/names.h>
#include <FL/Fl_Spinner.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Menu_Button.H>
//#include <FL/Fl_Flex.H>

#include <GL/glew.h>

#include <iostream>

#include "cyGL.h"
#include "cyMatrix.h"

using namespace cy;
using std::string;
using std::vector;

typedef unsigned uint;
typedef Vec2<int> Vec2i;
typedef Vec2<uint> Vec2u;
typedef Vec3<int> Vec3i;
typedef Vec3<uint> Vec3u;
typedef Vec4<int> Vec4i;
typedef Vec4<uint> Vec4u;

// Useful constants
const Vec3f WORLD_Y = Vec3f(0.0f, 1.0f, 0.0f);
const Vec3f WORLD_X = Vec3f(1.0f, 0.0f, 0.0f);
const Vec3f WORLD_Z = Vec3f(0.0f, 0.0f, 1.0f);
const Vec3f ORIGIN = Vec3f(0.0f);

bool initGlew = true;

bool compareFloat(float x, float y, float epsilon) {
	float dist = std::abs(x - y);
	return dist < epsilon;
}

Vec3f sphericalToCartesian(const Vec3f& coords) {
	float polarAngle = degreesToRadians(coords.x);
	float azimuthalAngle = degreesToRadians(coords.y);
	float polarRadius = std::sin(azimuthalAngle) * coords.z;

	Vec3f cartesian;
	cartesian.x = std::cos(polarAngle) * polarRadius;
	cartesian.y = std::cos(azimuthalAngle) * coords.z;
	cartesian.z = -std::sin(polarAngle) * polarRadius;

	return cartesian;
}

void clamp(float& x, float left, float right) {
	if (x < left) { x = left; }
	else if (x > right) { x = right; }
}

class Viewport : public Fl_Gl_Window {
	// Shader Program
	GLSLProgram meshProgram;
	GLSLProgram wireProgram;
	GLSLProgram gridProgram;

	// Vertex Array Objects
	GLuint meshVAO;
	GLuint gridVAO;

	// Render Buffers
	GLuint vertexPositionBuffer;
	GLuint elementBuffer;
	GLuint edgeElementBuffer;

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
	uint meshElementCount = 0;
	uint meshEdgeElementCount = 0;

	float wireframeScale = 1.001f;

	bool displayWire = true;
	bool displayGrid = true;

	bool initialize() {
		// Create VAO for grid
		glGenVertexArrays(1, &gridVAO);
		glBindVertexArray(gridVAO);

		// Create VAO for mesh
		glGenVertexArrays(1, &meshVAO);
		glBindVertexArray(meshVAO);

		glGenBuffers(1, &vertexPositionBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertexPositionBuffer);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

		glGenBuffers(1, &elementBuffer);
		glGenBuffers(1, &edgeElementBuffer);

		updateRenderBuffers();

		// Initialize Shader Programs
		bool success = gridProgram.BuildFiles("../assets/shaders/Grid.vert", "../assets/shaders/Grid.frag");
		if (!success) {
			std::cout << "Failed to compile grid shader program!" << std::endl;
			//Fl::hide_all_windows();
			return false;
		}

		success = meshProgram.BuildFiles("../assets/shaders/Mesh.vert", "../assets/shaders/BlinnPhong.frag", "../assets/shaders/GeomNormals.geom");
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
		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindVertexArray(meshVAO);
		meshProgram.Bind();
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
		glDrawElements(GL_TRIANGLES, meshElementCount, GL_UNSIGNED_INT, 0);

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
			glBindVertexArray(meshVAO);
			wireProgram.Bind();
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, edgeElementBuffer);
			glDrawElements(GL_LINES, meshEdgeElementCount, GL_UNSIGNED_INT, 0);
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
		mouseDragPrevPos.Set(Fl::event_x(), Fl::event_y());
		updateViewUniforms();
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
	}

	void updateModelUniforms() {
		float data[16];

		Matrix4f modelView = viewTransform * mesh.getTransform();
		modelView.Get(data);
		meshProgram.Bind();
		meshProgram.SetUniformMatrix4("modelView", data);

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
		std::vector<uint> elements;
		std::vector<uint> edgeElements;

		mesh.GetElementBuffer(elements);
		mesh.GetEdgeElementBuffer(edgeElements);

		meshElementCount = elements.size();
		meshEdgeElementCount = edgeElements.size();

		glBindVertexArray(meshVAO);

		glBindBuffer(GL_ARRAY_BUFFER, vertexPositionBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * mesh.VertexCount() * 3, mesh.VertexPositions(), GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * elements.size(), elements.data(), GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, edgeElementBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * edgeElements.size(), edgeElements.data(), GL_DYNAMIC_DRAW);
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

const char* AXIS_LETTERS[3] = { "x", "y", "z" };

Viewport* viewport;

template <int component>
void scaleSpinnerCallback(Fl_Widget* spinner) {
	float spinnerVal = ((Fl_Spinner*)spinner)->value();
	viewport->setScale(component, spinnerVal);
}

template <int component>
void posSpinnerCallback(Fl_Widget* spinner) {
	float spinnerVal = ((Fl_Spinner*)spinner)->value();
	viewport->setPos(component, spinnerVal);
}

template <int component>
void rotSpinnerCallback(Fl_Widget* spinner) {
	float spinnerVal = ((Fl_Spinner*)spinner)->value();
	viewport->setRot(component, spinnerVal);
}

void fileCallback(Fl_Widget* fileButton) {
	Fl_Native_File_Chooser fileChooser(Fl_Native_File_Chooser::BROWSE_FILE);
	fileChooser.directory("../assets/models/");
	if (fileChooser.show() == 0) {
		viewport->loadObj(fileChooser.filename());
	}
}

void menuCallback(Fl_Menu_Bar topMenu) {
	
}

const int VIEWPORT_W = 900;
const int VIEWPORT_H = 720;
const int RIGHT_PANEL_W = 1080 - VIEWPORT_W;
const int RIGHT_PANEL_INPUTS_W = RIGHT_PANEL_W * 0.9f;
const int RIGHT_PANEL_LABELS_W = RIGHT_PANEL_W - RIGHT_PANEL_INPUTS_W;

const int TOP_TOOLBAR_W = VIEWPORT_W - RIGHT_PANEL_W;

int main(int argc, char* argv[]) {
	Fl_Window* window = new Fl_Window(0, 0, 1080, 720, "ABC3D");

	viewport = new Viewport(0, 40, VIEWPORT_W, VIEWPORT_H);
	viewport->end();

	// Side Toolbar
	Fl_Window* sideToolbar = new Fl_Window(20, 70, 50, 500);
	sideToolbar->color(7);

	sideToolbar->end();

	// Side Toolbar
	Fl_Window* bottomToolbar = new Fl_Window(250, VIEWPORT_H - 60, VIEWPORT_W / 2, 50);
	bottomToolbar->color(12);
	bottomToolbar->end();


	// Top toolbar
	Fl_Window* topToolbar = new Fl_Window(0, 0, VIEWPORT_W,40);
	topToolbar->color(30);
	topToolbar->border(0);

	Fl_Button* fileButton = new Fl_Button(0, 600, 30, 50, "Pick File");
	fileButton->callback(fileCallback);		// Currently doesn't do anything

	Fl_Menu_Bar* topMenu = new Fl_Menu_Bar(0, 0, VIEWPORT_W, 40);
	topMenu->color(3);

	// File Menu Button
	topMenu->add("&File/&Import/&Model");
	topMenu->add("&File/&Import/&Scene");
	topMenu->add("&File/&Import/&Lesson");

	topMenu->add("&File/&Export/&Model");
	topMenu->add("&File/&Export/&Scene");
	topMenu->add("&File/&Export/&Lesson");

	topMenu->add("&File/&Save");
	topMenu->add("&File/&Settings");
	topMenu->add("&File/&Exit");
	
	// Edit Menu Button
	topMenu->add("&Edit/&Delete");
	topMenu->add("&Edit/&Duplicate");
	topMenu->add("&Edit/&Undo");
	topMenu->add("&Edit/&Redo");

	//topMenu->add("|");

	// Help Menu Button
	topMenu->add("Help");

	// Standard Mode Menu Button
	topMenu->add("Standard Mode");

	// Simple Mode Menu Button
	topMenu->add("Simple Mode");

	// Render Mode Menu Button
	topMenu->add("Render");

	// Tutorial View Menu Button
	topMenu->add("Tutorials");

	// Camera Modes Menu Button
	topMenu->add("Camera");

	// Lesson Editor Menu Button
	topMenu->add("Lesson Editor");
	
	topToolbar->end();

	// Right panel
	Fl_Window* rightPanel = new Fl_Window(VIEWPORT_W, 0, RIGHT_PANEL_W, 650);
	rightPanel->resizable(nullptr); //(VIEWPORT_W, 50, 0, 50);
	rightPanel->begin();

	// Scale Widgets
	Fl_Window* scaleWidgets = new Fl_Window(0, 0, RIGHT_PANEL_W, 200);
	scaleWidgets->begin();
	Fl_Box* scaleLabel = new Fl_Box(0, 0, RIGHT_PANEL_W, 50);
	scaleLabel->label("Scale");

	// Scale Spinners
	Fl_Spinner* scaleSpinners[3];
	float yPos = 50;
	for (int i = 0; i < 3; ++i) {
		scaleSpinners[i] = new Fl_Spinner(RIGHT_PANEL_LABELS_W, yPos, RIGHT_PANEL_INPUTS_W, 50);
		scaleSpinners[i]->label(AXIS_LETTERS[i]);
		scaleSpinners[i]->type(FL_FLOAT_INPUT);
		scaleSpinners[i]->range(-1000000000, 1000000000);
		scaleSpinners[i]->end();
		yPos += 50;
	}

	scaleSpinners[0]->callback(scaleSpinnerCallback<0>);
	scaleSpinners[1]->callback(scaleSpinnerCallback<1>);
	scaleSpinners[2]->callback(scaleSpinnerCallback<2>);

	scaleWidgets->end();

	// Position Widgets
	Fl_Window* posWidgets = new Fl_Window(0, 200, RIGHT_PANEL_W, 200);
	posWidgets->begin();
	Fl_Box* posLabel = new Fl_Box(0, 0, RIGHT_PANEL_W, 50);
	posLabel->label("Position");

	// Position Spinners
	Fl_Spinner* posSpinners[3];
	yPos = 50;
	for (int i = 0; i < 3; ++i) {
		posSpinners[i] = new Fl_Spinner(RIGHT_PANEL_LABELS_W, yPos, RIGHT_PANEL_INPUTS_W, 50);
		posSpinners[i]->label(AXIS_LETTERS[i]);
		posSpinners[i]->type(FL_FLOAT_INPUT);
		posSpinners[i]->range(-1000000000, 1000000000);
		posSpinners[i]->end();
		yPos += 50;
	}

	posSpinners[0]->callback(posSpinnerCallback<0>);
	posSpinners[1]->callback(posSpinnerCallback<1>);
	posSpinners[2]->callback(posSpinnerCallback<2>);

	posWidgets->end();

	// Rotation Widgets
	Fl_Window* rotWidgets = new Fl_Window(0, 400, RIGHT_PANEL_W, 200);
	rotWidgets->begin();
	Fl_Box* rotLabel = new Fl_Box(0, 0, RIGHT_PANEL_W, 50);
	rotLabel->label("Rotation");

	// Rotation Spinners
	Fl_Spinner* rotSpinners[3];
	yPos = 50;
	for (int i = 0; i < 3; ++i) {
		rotSpinners[i] = new Fl_Spinner(RIGHT_PANEL_LABELS_W, yPos, RIGHT_PANEL_INPUTS_W, 50);
		rotSpinners[i]->label(AXIS_LETTERS[i]);
		rotSpinners[i]->type(FL_FLOAT_INPUT);
		rotSpinners[i]->range(-1000000000, 1000000000);
		rotSpinners[i]->end();
		yPos += 50;
	}

	rotSpinners[0]->callback(rotSpinnerCallback<0>);
	rotSpinners[1]->callback(rotSpinnerCallback<1>);
	rotSpinners[2]->callback(rotSpinnerCallback<2>);

	rotWidgets->end();
	rightPanel->end();

	window->end();
	window->resizable(viewport);
	window->show(argc, argv);
	viewport->show();
	viewport->take_focus();
	return Fl::run();
}