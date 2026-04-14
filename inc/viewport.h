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

#include <GL/glew.h>

#include <iostream>
#include <chrono>

#include "cyGL.h"
#include "cyMatrix.h"

#include "helper.h"
#include "mesh.h"
#include "shapes.h"
#include "render.h"
#include "scene.h"

inline bool initGlew = true;

class Viewport : public Fl_Gl_Window {

	enum Mode { MODEL, SIMPLE, RENDER };
	Mode activeMode = MODEL;

	ModelingRenderer modelingRenderer;

	Renderer* activeRenderer = &modelingRenderer;
	bool modeChanged = false;

	Scene scene;

	Vec2i mouseDragPrevPos = Vec2i(0); // Input Data

	Vec3f camPos = Vec3f(0.0f, 90.0f, 5.0f); // Spherical Coordinates

	void draw();

	int handle(int event);

	void mouseDrag();

	int mousePush();

	void mouseWheel();

	void keyDown();

public:
	Viewport(int x, int y, int w, int h, const char* label = nullptr);

	void setScale(int component, float scale);

	void subdivision();

	void tesselate();

	void setRot(int component, float angle);

	void setPos(int component, float pos);

	bool loadFile(const char* filepath);

	bool exportObj(const char* filepath);

	bool exportStl(const char* filepath);
};

#endif
