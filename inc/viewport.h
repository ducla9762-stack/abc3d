#ifndef _ABC3D_VIEWPORT_H_
#define _ABC3D_VIEWPORT_H_

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/names.h>

#include <GL/glew.h>

#include <iostream>

#include "mesh.h"
#include "shapes.h"
#include "render.h"
#include "scene.h"
#include "helper.h"

inline bool initGlew = true;

enum Mode { MODEL, SIMPLE, RENDER };
enum Element { OBJECT, FACE, EDGE, VERTEX };
enum Action { CAMERA, SELECT, TRANSLATE_OUT, TRANSLATE_FREE, TRANSLATE_AXIS, TRANSLATE_EDGE, SCALE, ROTATE, EXTRUDE, BEVEL };

class Viewport : public Fl_Gl_Window {

	Mode activeMode = MODEL;
	Element activeElement = OBJECT;
	Action activeAction = SELECT;

	ModelingRenderer modelingRenderer;

	Renderer* activeRenderer = &modelingRenderer;
	bool modeChanged = false;

	bool singleSelection = true;

	Scene scene;

	Vec2f mouseDragStartPosUV = Vec2f(0.0f);
	Vec2f initialDragDir = Vec2f(0.0f);
	Vec2i mouseDragPrevPos = Vec2i(0);
	Vec2f mouseDragPrevPosUV = Vec2f(0.0f);
	Vec2i mouseDragStartPos = Vec2i(0);
	Vec3u selectionIds = Vec3u(uint(0));
	Vec3f actionDir = Vec3f(0.0f);
	Vec3f initialActionState = Vec3f(0.0f);
	bool firstDragCallback = false;

	Vec3f camPos = Vec3f(0.0f, 90.0f, 5.0f); // Spherical Coordinates

	void draw();

	int handle(int event);

	void mouseDrag();

	bool translateOutDrag(const Vec2f& pos, const Vec2f& uv);
	bool translateOutVertexDrag(const Vec2f& uv);

	bool translateFreeDrag(const Vec2f& pos, const Vec2f& uv);

	int mousePush();

	void mouseWheel();

	void keyDown();

	void keyUp();

public:
	Viewport(int x, int y, int w, int h, const char* label = nullptr);

	void setScale(int component, float scale);

	float getScale(int component, float scale);

	void subdivision();

	void tesselate();

	void bevel();
	
	void extrude();

	void displayCube();

	void displayPyramid();

	void displayCylinder();

	void displaySphere();

	void displayTorus();

	float getRot(int compontent, float angle);

	void deleteMesh();

	void selectMesh(uint id);

	void deselectMeshes();

	void setRot(int component, float angle);

	float getPos(int compontent, float angle);

	void setPos(int component, float pos);

	bool loadFile(const char* filepath);

	bool exportObj(const char* filepath);

	bool exportStl(const char* filepath);

	Mode getActiveMode();

	Element getActiveElement();

	Action getActiveAction();

	void setActiveMode(Mode mode);

	void setActiveElement(Element element);

	void setActiveAction(Action action);
};

#endif
