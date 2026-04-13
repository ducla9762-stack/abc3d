#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/names.h>
#include <FL/Fl_Spinner.H>
#include <FL/Fl_Simple_Counter.H> // Use later?
#include <FL/Fl_Box.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Toggle_Button.H>
#include <FL/Fl_Radio_Button.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Menu_Button.H>
//#include <FL/Fl_Flex.H>
#include <FL/Fl_Scrollbar.H>
#undef max

#include <GL/glew.h>

#include <iostream>

#include "cyGL.h"
#include "cyMatrix.h"

#include "viewport.h"

const char* AXIS_LETTERS[3] = { "x", "y", "z" };

Viewport* viewport;


// Top Menu Callbacks
void importModelCallback(Fl_Widget* w, void*) {
	Fl_Native_File_Chooser fileChooser(Fl_Native_File_Chooser::BROWSE_FILE);
	fileChooser.directory("../assets/models/");
	if (fileChooser.show() == 0) {
		viewport->loadFile(fileChooser.filename());
	}
}

void importStlCallback(Fl_Widget* w, void*) {
	Fl_Native_File_Chooser fileChooser(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
	fileChooser.directory("../assets/models/");
	if (fileChooser.show() == 0) {
		viewport->exportStl(fileChooser.filename());
	}
}

void exportObjCallback(Fl_Widget* w, void*) {
	Fl_Native_File_Chooser fileChooser(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
	fileChooser.directory("../assets/models/");
	if (fileChooser.show() == 0) {
		viewport->exportObj(fileChooser.filename());
	}
}

void saveCallback(Fl_Widget* w, void*) {

}

void editCallback(Fl_Widget* w, void*) {
	
}

void helpCallback(Fl_Widget* w, void* v) {

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

const int VIEWPORT_W = 900;
const int VIEWPORT_H = 720;
const int RIGHT_PANEL_W = 1080 - VIEWPORT_W;
const int RIGHT_PANEL_INPUTS_W = RIGHT_PANEL_W * 0.9f;
const int RIGHT_PANEL_LABELS_W = RIGHT_PANEL_W - RIGHT_PANEL_INPUTS_W;
const int LEFT_TOOLBAR_W = 50;
const int LEFT_TOOLBAR_H = 480;
const int BOTTOM_TOOLBAR_H = 50;
const int BOTTOM_TOOLBAR_W = 200;

const int TOP_TOOLBAR_H = 40;
const int TOP_TOOLBAR_W = VIEWPORT_W - RIGHT_PANEL_W;

int main(int argc, char* argv[]) {
	Fl_Window* window = new Fl_Window(0, 0, 1080, 720, "ABC3D");

	viewport = new Viewport(0, 40, VIEWPORT_W, VIEWPORT_H);
	viewport->end();

	// Top toolbar
	Fl_Window* topToolbar = new Fl_Window(0, 0, VIEWPORT_W, TOP_TOOLBAR_H);
	topToolbar->color(8);

	// Menu within the top toolbar
	Fl_Menu_Bar* topMenu = new Fl_Menu_Bar(-2, 0, VIEWPORT_W / 8 - 30, TOP_TOOLBAR_H);
	topMenu->selection_color(10);
	topMenu->color(8);

	// File -> Import Menu Button
	topMenu->add("&File/&Import/&Model", "^o", importModelCallback);
	topMenu->add("&File/&Import/&Scene");
	topMenu->add("&File/&Import/&Lesson");

	// File -> Export Menu Button
	topMenu->add("&File/&Export/&Model/&.STL", 0, importStlCallback);
	topMenu->add("&File/&Export/&Model/&.OBJ", 0, exportObjCallback); //Temporary - will later be a popup menu
	topMenu->add("&File/&Export/&Scene", 0, importStlCallback);
	topMenu->add("&File/&Export/&Lesson");

	topMenu->add("&File/&Save/", "^s", saveCallback);
	topMenu->add("&File/&Settings");
	topMenu->add("&File/&Exit");

	// Edit Menu Button
	topMenu->add("&Edit/&Delete");
	topMenu->add("&Edit/&Duplicate");
	topMenu->add("&Edit/&Undo");
	topMenu->add("&Edit/&Redo");


	Fl_Toggle_Button* helpButton = new Fl_Toggle_Button(VIEWPORT_W / 8, 0, 50, TOP_TOOLBAR_H, "Help");
	helpButton->color(8);
	helpButton->selection_color(10);
	helpButton->box(FL_FLAT_BOX);
	helpButton->callback(helpCallback);
	helpButton->align(FL_ALIGN_CENTER);
	helpButton->type(FL_RADIO_BUTTON);

	// Standard Mode Menu Button
	Fl_Toggle_Button* standardButton = new Fl_Toggle_Button(2 * VIEWPORT_W / 8, 0, VIEWPORT_W / 8, TOP_TOOLBAR_H, "Standard Mode");
	standardButton->color(8);
	standardButton->selection_color(10);
	standardButton->box(FL_FLAT_BOX);
	standardButton->callback(standardModeCallback);
	standardButton->align(FL_ALIGN_INSIDE);
	standardButton->type(FL_RADIO_BUTTON);

	// Simple Mode Menu Button
	Fl_Toggle_Button* simpleButton = new Fl_Toggle_Button(3 * VIEWPORT_W / 8, 0, VIEWPORT_W / 8, TOP_TOOLBAR_H, "Simple Mode");
	simpleButton->color(8);
	simpleButton->selection_color(10);
	simpleButton->box(FL_FLAT_BOX);
	simpleButton->callback(simpleModeCallback);
	simpleButton->align(FL_ALIGN_INSIDE);
	simpleButton->type(FL_RADIO_BUTTON);

	// Render Mode Menu Button
	Fl_Toggle_Button* renderButton = new Fl_Toggle_Button(4 * VIEWPORT_W / 8, 0, VIEWPORT_W / 8, TOP_TOOLBAR_H, "Render");
	renderButton->color(8);
	renderButton->selection_color(10);
	renderButton->box(FL_FLAT_BOX);
	renderButton->callback(renderModeCallback);
	renderButton->align(FL_ALIGN_INSIDE);
	renderButton->type(FL_RADIO_BUTTON);

	// Tutorial View Menu Button
	Fl_Toggle_Button* tutorialButton = new Fl_Toggle_Button(5 * VIEWPORT_W / 8, 0, VIEWPORT_W / 8, TOP_TOOLBAR_H, "Tutorials");
	tutorialButton->color(8);
	tutorialButton->selection_color(10);
	tutorialButton->box(FL_FLAT_BOX);
	tutorialButton->callback(tutorialModeCallback);
	tutorialButton->align(FL_ALIGN_INSIDE);
	tutorialButton->type(FL_RADIO_BUTTON);

	// Camera Modes Menu Button
	Fl_Toggle_Button* cameraButton = new Fl_Toggle_Button(6 * VIEWPORT_W / 8, 0, VIEWPORT_W / 8, TOP_TOOLBAR_H, "Camera");
	cameraButton->color(8);
	cameraButton->selection_color(10);
	cameraButton->box(FL_FLAT_BOX);
	cameraButton->callback(cameraModeCallback);
	cameraButton->align(FL_ALIGN_INSIDE);
	cameraButton->type(FL_RADIO_BUTTON);

	// Lesson Editor Menu Button
	Fl_Toggle_Button* lessonEditorButton = new Fl_Toggle_Button(7 * VIEWPORT_W / 8, 0, VIEWPORT_W / 8, TOP_TOOLBAR_H, "Lesson Editor");
	lessonEditorButton->color(8);
	lessonEditorButton->selection_color(10);
	lessonEditorButton->box(FL_FLAT_BOX);
	lessonEditorButton->callback(lessonEditorCallback);
	lessonEditorButton->align(FL_ALIGN_INSIDE);
	lessonEditorButton->type(FL_RADIO_BUTTON);

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
		scaleSpinners[i]->value(1);
		scaleSpinners[i]->step(.2);
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