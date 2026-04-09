#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/names.h>
#include <FL/Fl_Spinner.H>
#include <FL/Fl_Simple_Counter.H> // Use later?
#include <FL/Fl_Box.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Radio_Button.H>
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

void helpCallback(Fl_Widget* w, void* v) {
	Fl_Menu_Bar* menu = (Fl_Menu_Bar*)w;
	const Fl_Menu_Item* helpButton = menu->mvalue();
	//helpButton->flags(FL_MENU_RADIO);
	//helpButton->label("Changed");

	//helpButton->redraw_label();
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

void lessonEditorCallback(Fl_Widget* w, void* v) {

}


// Left Toolbar Callbacks
void selectCallback(Fl_Widget* w, void*) {

}

void moveCallback(Fl_Widget* w, void*) {

}

void rotateCallback(Fl_Widget* w, void*) {
	
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
	float spinnerVal = ((Fl_Simple_Counter*)spinner)->value();
	viewport->setScale(component, spinnerVal);
}

template <int component>
void posSpinnerCallback(Fl_Widget* spinner) {
	float spinnerVal = ((Fl_Simple_Counter*)spinner)->value();
	viewport->setPos(component, spinnerVal);
}

template <int component>
void rotSpinnerCallback(Fl_Widget* spinner) {
	float spinnerVal = ((Fl_Simple_Counter*)spinner)->value();
	viewport->setRot(component, spinnerVal);
}

void fileButtonCallback(Fl_Widget* fileButton) {
	Fl_Native_File_Chooser fileChooser(Fl_Native_File_Chooser::BROWSE_FILE);
	fileChooser.directory("../assets/models/");
	if (fileChooser.show() == 0) {
		viewport->loadObj(fileChooser.filename());
	}
}

const int VIEWPORT_W = 900;
const int VIEWPORT_H = 720;
const int RIGHT_PANEL_W = 1080 - VIEWPORT_W;
const int RIGHT_PANEL_INPUTS_W = RIGHT_PANEL_W * 0.9f;
const int RIGHT_PANEL_LABELS_W = RIGHT_PANEL_W - RIGHT_PANEL_INPUTS_W;
const int LEFT_TOOLBAR_W = 50;
const int LEFT_TOOLBAR_H = 480;
const int BOTTOM_TOOLBAR_H = 50;
const int BOTTOM_TOOLBAR_W = 200;

const int TOP_TOOLBAR_W = VIEWPORT_W - RIGHT_PANEL_W;

int main(int argc, char* argv[]) {
	Fl_Window* window = new Fl_Window(0, 0, 1080, 720, "ABC3D");

	viewport = new Viewport(0, 40, VIEWPORT_W, VIEWPORT_H);
	viewport->end();


	// Top toolbar
	Fl_Window* topToolbar = new Fl_Window(0, 0, VIEWPORT_W, 40);
	topToolbar->color(12);

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

	// Help Menu Button
	topMenu->add("Help", 0, helpCallback, 0, FL_MENU_TOGGLE);

	// Standard Mode Menu Button
	topMenu->add("Standard Mode", 0, 0, standardModeCallback, FL_NORMAL_BUTTON);

	// Simple Mode Menu Button
	topMenu->add("Simple Mode", 0, 0, simpleModeCallback, FL_NORMAL_BUTTON);

	// Render Mode Menu Button
	topMenu->add("Render", 0, 0, renderModeCallback, FL_NORMAL_BUTTON);

	// Tutorial View Menu Button
	topMenu->add("Tutorials", 0, 0, tutorialModeCallback, FL_NORMAL_BUTTON);

	// Camera Modes Menu Button
	topMenu->add("Camera", 0, 0, cameraModeCallback, FL_NORMAL_BUTTON);

	// Lesson Editor Menu Button
	topMenu->add("Lesson Editor", 0, 0, lessonEditorCallback, FL_NORMAL_BUTTON);

	topToolbar->end();

	// Left Toolbar
	Fl_Window* sideToolbar = new Fl_Window(20, 70, LEFT_TOOLBAR_W, LEFT_TOOLBAR_H);

	sideToolbar->color(7);

	Fl_Radio_Button* selectButton = new Fl_Radio_Button(0, 0, LEFT_TOOLBAR_W, LEFT_TOOLBAR_H / 8, "Select");
	selectButton->selection_color(0);
	Fl_Radio_Button* moveButton = new Fl_Radio_Button(0, LEFT_TOOLBAR_H / 8, LEFT_TOOLBAR_W, LEFT_TOOLBAR_H / 8, "Move");
	moveButton->selection_color(0);
	Fl_Radio_Button* rotateButton = new Fl_Radio_Button(0, 2 * LEFT_TOOLBAR_H / 8, LEFT_TOOLBAR_W, LEFT_TOOLBAR_H / 8, "Rotate");
	rotateButton->selection_color(0);
	Fl_Radio_Button* scaleButton = new Fl_Radio_Button(0, 3 * LEFT_TOOLBAR_H / 8, LEFT_TOOLBAR_W, LEFT_TOOLBAR_H / 8, "Scale");
	scaleButton->selection_color(0);
	Fl_Radio_Button* shapeButton = new Fl_Radio_Button(0, 4 * LEFT_TOOLBAR_H / 8, LEFT_TOOLBAR_W, LEFT_TOOLBAR_H / 8, "Shape");
	shapeButton->selection_color(0);
	Fl_Radio_Button* extrudeButton = new Fl_Radio_Button(0, 5 * LEFT_TOOLBAR_H / 8, LEFT_TOOLBAR_W, LEFT_TOOLBAR_H / 8, "Extrude");
	extrudeButton->selection_color(0);

	// Subdivision button
	Fl_Radio_Button* subdivisionButton = new Fl_Radio_Button(0, 6 * LEFT_TOOLBAR_H / 8, 50, LEFT_TOOLBAR_H / 8, "Subdivision");
	subdivisionButton->selection_color(0);
	subdivisionButton->callback(subdivisionCallback);
	subdivisionButton->when(FL_WHEN_CHANGED);
		
	Fl_Radio_Button* bevelButton = new Fl_Radio_Button(0, 7 * LEFT_TOOLBAR_H / 8, 50, LEFT_TOOLBAR_H / 8, "Bevel");
	bevelButton->selection_color(0);


	sideToolbar->resizable(sideToolbar);
	sideToolbar->end();


	/* Top Right panel */
	Fl_Window* topRightPanel = new Fl_Window(VIEWPORT_W, 0, RIGHT_PANEL_W, VIEWPORT_H / 3);
	topRightPanel->resizable(nullptr); //(VIEWPORT_W, 50, 0, 50);
	topRightPanel->begin();
	topRightPanel->color(FL_DARK_RED);

	topRightPanel->end();


	/* Right panel */
	Fl_Window* rightPanel = new Fl_Window(VIEWPORT_W, VIEWPORT_H / 3 , RIGHT_PANEL_W, 650);
	rightPanel->resizable(nullptr); //(VIEWPORT_W, 50, 0, 50);
	rightPanel->begin();

	// Scale Widgets
	Fl_Window* scaleWidgets = new Fl_Window(0, 0, RIGHT_PANEL_W, 200);
	scaleWidgets->begin();
	Fl_Box* scaleLabel = new Fl_Box(0, 0, RIGHT_PANEL_W, 50);
	scaleLabel->label("Scale");

	// Scale Spinners
	Fl_Simple_Counter* scaleSpinners[3];
	float yPos = 50;
	for (int i = 0; i < 3; ++i) {
		scaleSpinners[i] = new Fl_Simple_Counter(RIGHT_PANEL_LABELS_W, yPos, RIGHT_PANEL_INPUTS_W, 15);
		scaleSpinners[i]->label(AXIS_LETTERS[i]);
		scaleSpinners[i]->type(FL_FLOAT_INPUT);
		scaleSpinners[i]->range(-1000000000, 1000000000);
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
	Fl_Simple_Counter* posSpinners[3];
	yPos = 50;
	for (int i = 0; i < 3; ++i) {
		posSpinners[i] = new Fl_Simple_Counter(RIGHT_PANEL_LABELS_W, yPos, RIGHT_PANEL_INPUTS_W, 15);
		posSpinners[i]->label(AXIS_LETTERS[i]);
		posSpinners[i]->type(FL_FLOAT_INPUT);
		posSpinners[i]->range(-1000000000, 1000000000);
		yPos += 50;
	}

	posSpinners[0]->callback(posSpinnerCallback<0>);
	posSpinners[1]->callback(posSpinnerCallback<1>);
	posSpinners[2]->callback(posSpinnerCallback<2>);

	posWidgets->end();

	// Rotation Widgets
	Fl_Window* rotWidgets = new Fl_Window(0, 400, RIGHT_PANEL_W, 200);
	rotWidgets->begin();
	Fl_Box* rotLabel = new Fl_Box(0, 0, RIGHT_PANEL_W, 25);
	rotLabel->label("Rotation");

	// Rotation Spinners
	Fl_Simple_Counter* rotSpinners[3];
	yPos = 50;
	for (int i = 0; i < 3; ++i) {
		rotSpinners[i] = new Fl_Simple_Counter(RIGHT_PANEL_LABELS_W, yPos, RIGHT_PANEL_INPUTS_W, 15);
		rotSpinners[i]->label(AXIS_LETTERS[i]);
		rotSpinners[i]->type(FL_FLOAT_INPUT);
		rotSpinners[i]->range(-1000000000, 1000000000);
		yPos += 50;
	}

	rotSpinners[0]->callback(rotSpinnerCallback<0>);
	rotSpinners[1]->callback(rotSpinnerCallback<1>);
	rotSpinners[2]->callback(rotSpinnerCallback<2>);

	rotWidgets->end();
	rightPanel->end();


	// Bottom Toolbar
	Fl_Window* bottomToolbar = new Fl_Window(150, VIEWPORT_H - 70, BOTTOM_TOOLBAR_W, BOTTOM_TOOLBAR_H);
	bottomToolbar->color(12);

	Fl_Radio_Button* vertexButton = new Fl_Radio_Button(0, 0, BOTTOM_TOOLBAR_W / 4, BOTTOM_TOOLBAR_H, "Vertex");
	vertexButton->selection_color(0);
	vertexButton->callback(vertexCallback);
	Fl_Radio_Button* edgeButton = new Fl_Radio_Button(50, 0, BOTTOM_TOOLBAR_W / 4, BOTTOM_TOOLBAR_H, "Edge");
	edgeButton->selection_color(0);
	edgeButton->callback(edgeCallback);
	Fl_Radio_Button* faceButton = new Fl_Radio_Button(100, 0, BOTTOM_TOOLBAR_W / 4, BOTTOM_TOOLBAR_H, "Face");
	faceButton->selection_color(0);
	faceButton->callback(faceCallback);
	Fl_Radio_Button* objectButton = new Fl_Radio_Button(150, 0, BOTTOM_TOOLBAR_W / 4, BOTTOM_TOOLBAR_H, "Object");
	objectButton->selection_color(0);
	objectButton->callback(objectCallback);

	bottomToolbar->resizable(bottomToolbar);
	bottomToolbar->end();

	// Window 
	window->end();
	window->resizable(viewport);
	window->show(argc, argv);
	viewport->show();
	viewport->take_focus();
	return Fl::run();
}