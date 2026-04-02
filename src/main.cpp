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
#undef max

#include <GL/glew.h>

#include <iostream>

#include "cyGL.h"
#include "cyMatrix.h"

#include "Viewport.h"

const char* AXIS_LETTERS[3] = { "x", "y", "z" };

Viewport* viewport;


// Top Menu Callbacks
void fileCallback(Fl_Widget* w, void*) {
	Fl_Native_File_Chooser fileChooser(Fl_Native_File_Chooser::BROWSE_FILE);
	fileChooser.directory("../assets/models/");
	if (fileChooser.show() == 0) {
		viewport->loadObj(fileChooser.filename());
	}
}

void saveCallback(Fl_Widget* w, void*) {

}

void editCallback(Fl_Widget* w, void*) {
	
}

void helpCallback(Fl_Widget* w, void*) {

}

void standardModeCallback(Fl_Widget* w, void*) {

}

void simpleModeCallback(Fl_Widget* w, void*) {

}

void renderModeCallback(Fl_Widget* w, void*) {

}

void tutorialModeCallback(Fl_Widget* w, void*) {

}

void cameraModeCallback(Fl_Widget* w, void*) {

}

void lessonEditorCallback(Fl_Widget* w, void*) {

}


// Left Toolbar Callbacks
void selectCallback(Fl_Widget* w, void*) {

}

void moveCallback(Fl_Widget* w, void*) {

}

void rotateCallback(Fl_Widget* w, void*) {
	;
}

void scaleCallback(Fl_Widget* w, void*) {
	
}

void shapeCallback(Fl_Widget* w, void*) {
	
}

void extrudeCallback(Fl_Widget* w, void*) {

}

void subdivisionCallback(Fl_Widget* w, void*) {
	viewport->subdivision();
}

void tesselateCallback(Fl_Widget* w, void*) {
	viewport->tesselate();
}

void bevelCallback(Fl_Widget* w, void*) {

}


// Bottom Toolbar Callbacks
void vertexCallback(Fl_Widget* w, void*) {

}
void edgeCallback(Fl_Widget* w, void*) {

}

void faceCallback(Fl_Widget* w, void*) {

}

void objectCallback(Fl_Widget* w, void*) {

}


// Right Toolbar Callbacks
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

void fileButtonCallback(Fl_Widget* fileButton) {
	Fl_Native_File_Chooser fileChooser(Fl_Native_File_Chooser::BROWSE_FILE);
	fileChooser.directory("../TestModels/");
	if (fileChooser.show() == 0) {
		viewport->loadObj(fileChooser.filename());
	}
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


	// Top toolbar
	Fl_Window* topToolbar = new Fl_Window(0, 0, VIEWPORT_W, 40);
	topToolbar->color(12);
	topToolbar->border(0);

	Fl_Menu_Bar* topMenu = new Fl_Menu_Bar(0, 0, VIEWPORT_W, 40);
	topMenu->selection_color(0);
	topMenu->color(12);

	// File -> Import Menu Button
	topMenu->add("&File/&Import/&Model", "^o", fileCallback);
	topMenu->add("&File/&Import/&Scene");
	topMenu->add("&File/&Import/&Lesson");

	// File -> Export Menu Button
	topMenu->add("&File/&Export/&Model");
	topMenu->add("&File/&Export/&Scene");
	topMenu->add("&File/&Export/&Lesson");

	topMenu->add("&File/&Save", "^s", saveCallback);
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

	// Left Toolbar
	Fl_Window* sideToolbar = new Fl_Window(20, 70, 50, 500);
	sideToolbar->color(7);

	Fl_Button* selectButton = new Fl_Button(0, 0, 50, 50, "Select");
	selectButton->type(1);
	selectButton->selection_color(0);
	Fl_Button* moveButton = new Fl_Button(0, 50, 50, 50, "Move");
	moveButton->type(1);
	moveButton->selection_color(0);
	Fl_Button* rotateButton = new Fl_Button(0, 100, 50, 50, "Rotate");
	rotateButton->type(1);
	rotateButton->selection_color(0);
	Fl_Button* scaleButton = new Fl_Button(0, 150, 50, 50, "Scale");
	scaleButton->type(1);
	scaleButton->selection_color(0);
	Fl_Button* shapeButton = new Fl_Button(0, 200, 50, 50, "Shape");
	shapeButton->type(1);
	shapeButton->selection_color(0);
	Fl_Button* extrudeButton = new Fl_Button(0, 250, 50, 50, "Extrude");
	extrudeButton->type(1);
	extrudeButton->selection_color(0);

	// Subdivision button
	Fl_Button* subdivisionButton = new Fl_Button(0, 300, 50, 50, "Subdivision");
	subdivisionButton->type(1);
	subdivisionButton->selection_color(0);
	subdivisionButton->callback(subdivisionCallback);
	subdivisionButton->when(FL_WHEN_CHANGED);
	// TODO: Figure out how to make it only change when selected, not unselected
		
	Fl_Button* bevelButton = new Fl_Button(0, 350, 50, 50, "Bevel");
	bevelButton->type(1);
	bevelButton->selection_color(0);

	sideToolbar->end();


	/* Top Right panel */
	Fl_Window* topRightPanel = new Fl_Window(VIEWPORT_W, 0, RIGHT_PANEL_W, 325);
	topRightPanel->resizable(nullptr); //(VIEWPORT_W, 50, 0, 50);
	topRightPanel->begin();
	topRightPanel->color(5);

	topRightPanel->end();


	/* Right panel */
	Fl_Window* rightPanel = new Fl_Window(VIEWPORT_W, VIEWPORT_H / 2 , RIGHT_PANEL_W, 650);
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


	// Bottom Toolbar
	Fl_Window* bottomToolbar = new Fl_Window(150, VIEWPORT_H - 70, 200, 50);
	bottomToolbar->color(12);

	Fl_Button* vertexButton = new Fl_Button(0, 0, 50, 50, "Vertex");
	vertexButton->type(1);
	vertexButton->selection_color(0);
	Fl_Button* edgeButton = new Fl_Button(50, 0, 50, 50, "Edge");
	edgeButton->type(1);
	edgeButton->selection_color(0);
	Fl_Button* faceButton = new Fl_Button(100, 0, 50, 50, "Face");
	faceButton->type(1);
	faceButton->selection_color(0);
	Fl_Button* objectButton = new Fl_Button(150, 0, 50, 50, "Object");
	objectButton->type(1);
	objectButton->selection_color(0);

	bottomToolbar->end();

	// Window 
	window->end();
	window->resizable(viewport);
	window->show(argc, argv);
	viewport->show();
	viewport->take_focus();
	return Fl::run();
}