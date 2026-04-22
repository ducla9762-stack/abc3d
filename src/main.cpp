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

#include "customSlider.h"
#include "viewport.h"

const char* AXIS_LETTERS[3] = { "x", "y", "z" };

Viewport* viewport; 
Fl_Window* topRightPanel; 
Fl_Window* rightPanel;
customSlider* slider;
Fl_Window* scaleWidgets;

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
float yPos = 0;

void drawScaleWidgets(int x, int y, int w, int h);
void drawRotationWidgets(int x, int y, int w, int h);
void drawPositionWidgets(int x, int y, int w, int h);

/* Top Menu Callbacks */
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


/* Left Toolbar Callbacks */
void selectCallback(Fl_Widget* w, void*) {
	rightPanel->hide();
	rightPanel->clear();
	rightPanel->begin();

	drawPositionWidgets(0, 0, RIGHT_PANEL_W, 200);
	drawRotationWidgets(0, 200, RIGHT_PANEL_W, 200);
	drawScaleWidgets(0, 400, RIGHT_PANEL_W, 200);

	rightPanel->end();
	rightPanel->show();

	viewport->setActiveAction(SELECT);
}

template<Action action>
void actionChangedCallback(Fl_Widget* w) {
	viewport->setActiveAction(action);
}

void translationCallback(Fl_Widget* w, void*) {
	rightPanel->hide();
	rightPanel->clear();
	rightPanel->begin();

	viewport->setActiveAction(TRANSLATE_FREE);

	// Outward Translation Button
	Fl_Radio_Button* outwardTranslationButton = new Fl_Radio_Button(0, 0, RIGHT_PANEL_W, VIEWPORT_H / 10, "Outward");
	outwardTranslationButton->selection_color(10);
	outwardTranslationButton->box(FL_UP_BOX);
	outwardTranslationButton->callback(actionChangedCallback<TRANSLATE_OUT>);
	outwardTranslationButton->align(FL_ALIGN_INSIDE);

	// Free Translation Button
	Fl_Radio_Button* freeTranslationButton = new Fl_Radio_Button(0, VIEWPORT_H / 10, RIGHT_PANEL_W, VIEWPORT_H / 10, "Free");
	freeTranslationButton->selection_color(10);
	freeTranslationButton->box(FL_UP_BOX);
	freeTranslationButton->callback(actionChangedCallback<TRANSLATE_FREE>);
	freeTranslationButton->align(FL_ALIGN_INSIDE);
	freeTranslationButton->value(1);

	drawPositionWidgets(0, 2 * VIEWPORT_H / 10, RIGHT_PANEL_W, 200);

	rightPanel->end();
	rightPanel->show();
}

void rotateCallback(Fl_Widget* w, void*) {
	rightPanel->hide();
	rightPanel->clear();
	rightPanel->begin();

	drawRotationWidgets(0, 0, RIGHT_PANEL_W, 200);

	rightPanel->end();
	rightPanel->show();

	viewport->setActiveAction(ROTATE);
}

void scaleCallback(Fl_Widget* w, void*) {
	rightPanel->hide();
	rightPanel->clear();
	rightPanel->begin();

	drawScaleWidgets(0, 0, RIGHT_PANEL_W, 200);

	rightPanel->end();
	rightPanel->show();

	viewport->setActiveAction(SCALE);
}

// Callbacks from within the Shape button
void cubeCallback(Fl_Widget * w, void* data) {
	viewport->displayCube();
}

void UVSphereCallback(Fl_Widget* w, void* data) {
	viewport->displayUVSphere();
}

void icoSphereCallback(Fl_Widget* w, void* data) {
	viewport->displayIcoSphere();
}

void pyramidCallback(Fl_Widget* w, void* data) {
	viewport->displayPyramid();
}

void cylinderCallback(Fl_Widget* w, void* data) {
	viewport->displayCylinder();
}

void torusCallback(Fl_Widget* w, void* data) {
	viewport->displayTorus();
}

void shapeCallback(Fl_Widget* w, void*) {
	rightPanel->hide();
	rightPanel->clear();
	rightPanel->begin();

	// Cube Shape Button
	Fl_Button* cubeButton = new Fl_Button(0, 0, RIGHT_PANEL_W, VIEWPORT_H / 9, "Cube");
	cubeButton->selection_color(10);
	cubeButton->box(FL_UP_BOX);
	cubeButton->callback(cubeCallback);
	cubeButton->align(FL_ALIGN_INSIDE);

	// UV Sphere Shape Button
	Fl_Button* UVSphereButton = new Fl_Button(0, VIEWPORT_H / 9, RIGHT_PANEL_W, VIEWPORT_H / 9, "UV Sphere");
	UVSphereButton->selection_color(10);
	UVSphereButton->box(FL_UP_BOX);
	UVSphereButton->callback(UVSphereCallback);
	UVSphereButton->align(FL_ALIGN_INSIDE);

	// ICO Sphere Shape Button
	Fl_Button* icoSphereButton = new Fl_Button(0, 2 * VIEWPORT_H / 9, RIGHT_PANEL_W, VIEWPORT_H / 9, "Ico Sphere");
	icoSphereButton->selection_color(10);
	icoSphereButton->box(FL_UP_BOX);
	icoSphereButton->callback(icoSphereCallback);
	icoSphereButton->align(FL_ALIGN_INSIDE);

	// Pyramid Shape Button
	Fl_Button* pyramidButton = new Fl_Button(0, 3 * VIEWPORT_H / 9, RIGHT_PANEL_W, VIEWPORT_H / 9, "Pyramid");
	pyramidButton->selection_color(10);
	pyramidButton->box(FL_UP_BOX);
	pyramidButton->callback(pyramidCallback);
	pyramidButton->align(FL_ALIGN_INSIDE);

	// Cylinder Button
	Fl_Button* cylinderButton = new Fl_Button(0, 4 * VIEWPORT_H / 9, RIGHT_PANEL_W, VIEWPORT_H / 9, "Cylinder");
	cylinderButton->selection_color(10);
	cylinderButton->box(FL_UP_BOX);
	cylinderButton->callback(cylinderCallback);
	cylinderButton->align(FL_ALIGN_INSIDE);
	
	// Torus Shape Button
	Fl_Button* torusButton = new Fl_Button(0, 5 * VIEWPORT_H / 9, RIGHT_PANEL_W, VIEWPORT_H / 9, "Torus");
	torusButton->selection_color(10);
	torusButton->box(FL_UP_BOX);
	torusButton->callback(torusCallback);
	torusButton->align(FL_ALIGN_INSIDE);


	rightPanel->end();
	rightPanel->show();
	// TODO: Add resolution
	viewport->setActiveAction(TRANSLATE_FREE);
}

void extrudeCallback(Fl_Widget* w, void* data) {
	rightPanel->hide();
	rightPanel->clear();
	rightPanel->begin();

	viewport->extrude();

	rightPanel->end();
	rightPanel->show();
}

void subdivisionCallback(Fl_Widget* w, void*) {
	viewport->subdivision();
}

void tesselationCallback(Fl_Widget*, void*) {
	viewport->tesselate();
}

void subdivisionPanelCallback(Fl_Widget* w, void*) {
	rightPanel->hide();
	rightPanel->clear();
	rightPanel->begin();

	// Subdivide Button
	Fl_Button* subdivideButton = new Fl_Button(0, 0, RIGHT_PANEL_W, VIEWPORT_H / 7, "Subdivide");
	subdivideButton->selection_color(10);
	subdivideButton->box(FL_UP_BOX);
	subdivideButton->callback(subdivisionCallback);
	subdivideButton->align(FL_ALIGN_INSIDE);

	// Tesselate Button
	Fl_Button* tesselateButton = new Fl_Button(0, 2 * VIEWPORT_H / 7, RIGHT_PANEL_W, VIEWPORT_H / 7, "Tesselation");
	tesselateButton->selection_color(10);
	tesselateButton->box(FL_UP_BOX);
	tesselateButton->callback(tesselationCallback);
	tesselateButton->align(FL_ALIGN_INSIDE);

	rightPanel->end();
	rightPanel->show();

	viewport->setActiveAction(SELECT);
}

void bevelCallback(Fl_Widget* w, void*) {
	rightPanel->hide();
	rightPanel->clear();
	rightPanel->begin();

	viewport->bevel();

	rightPanel->end();
	rightPanel->show();
}

template<Element element>
void elementSelectCallback(Fl_Widget* w) {
	viewport->setActiveElement(element);
}
	
// Bottom Right Panel Callbacks
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

void drawScaleWidgets(int x, int y, int w, int h) {
	// Scale Widgets
	Fl_Window* scaleWidgets = new Fl_Window(x, y, w, h);
	scaleWidgets->begin();
	Fl_Box* scaleLabel = new Fl_Box(0, 0, RIGHT_PANEL_W, 50);
	scaleLabel->label("Scale");

	// Rotation Spinners
	Fl_Simple_Counter* scaleSpinners[3];
	yPos = 50;

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
}

void drawRotationWidgets(int x, int y, int w, int h) {
	// Rotation Widgets
	Fl_Window* rotWidgets = new Fl_Window(x, y, w, h);
	rotWidgets->begin();
	Fl_Box* rotLabel = new Fl_Box(0, 0, RIGHT_PANEL_W, 25);
	rotLabel->label("Rotation");

	// Rotation Spinners
	Fl_Simple_Counter* rotSpinners[3];
	yPos = 50;
	for (int i = 0; i < 3; ++i) {
		rotSpinners[i] = new Fl_Simple_Counter(RIGHT_PANEL_LABELS_W, yPos, RIGHT_PANEL_INPUTS_W, 15);
		rotSpinners[i]->value(1);
		rotSpinners[i]->step(2);
		rotSpinners[i]->label(AXIS_LETTERS[i]);
		rotSpinners[i]->type(FL_FLOAT_INPUT);
		rotSpinners[i]->range(-1000000000, 1000000000);
		yPos += 50;
	}

	rotSpinners[0]->callback(rotSpinnerCallback<0>);
	rotSpinners[1]->callback(rotSpinnerCallback<1>);
	rotSpinners[2]->callback(rotSpinnerCallback<2>);

	rotWidgets->end();
}

void drawPositionWidgets(int x, int y, int w, int h) {
	// Position Widgets
	Fl_Window* posWidgets = new Fl_Window(x, y, w, h);
	posWidgets->begin();
	Fl_Box* posLabel = new Fl_Box(0, 0, RIGHT_PANEL_W, 50);
	posLabel->label("Position");

	// Position Spinners
	Fl_Simple_Counter* posSpinners[3];
	yPos = 50;
	for (int i = 0; i < 3; ++i) {
		posSpinners[i] = new Fl_Simple_Counter(RIGHT_PANEL_LABELS_W, yPos, RIGHT_PANEL_INPUTS_W, 15);
		posSpinners[i]->value(1);
		posSpinners[i]->step(.2);
		posSpinners[i]->label(AXIS_LETTERS[i]);
		posSpinners[i]->type(FL_FLOAT_INPUT);
		posSpinners[i]->range(-1000000000, 1000000000);
		yPos += 50;
	}

	posSpinners[0]->callback(posSpinnerCallback<0>);
	posSpinners[1]->callback(posSpinnerCallback<1>);
	posSpinners[2]->callback(posSpinnerCallback<2>);

	posWidgets->end();
}

int main(int argc, char* argv[]) {
	Fl_Window* window = new Fl_Window(0, 0, 1080, 720, "ABC3D");

	viewport = new Viewport(0, 40, VIEWPORT_W, VIEWPORT_H);
	viewport->end();

	// Top toolbar
	Fl_Window* topToolbar = new Fl_Window(0, 0, VIEWPORT_W, TOP_TOOLBAR_H);
	topToolbar->color(8);

	// Menu within the top toolbar
	Fl_Menu_Bar* topMenu = new Fl_Menu_Bar(-2, 0, VIEWPORT_W / 8, TOP_TOOLBAR_H);
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

	topMenu->add("&File/&Save", "^s", saveCallback);
	topMenu->add("&File/&Settings");
	topMenu->add("&File/&Exit");

	// Edit Menu Button
	topMenu->add("&Edit/&Delete");
	topMenu->add("&Edit/&Duplicate");
	topMenu->add("&Edit/&Undo");
	topMenu->add("&Edit/&Redo");


	Fl_Toggle_Button* helpButton = new Fl_Toggle_Button(VIEWPORT_W / 8, 0, VIEWPORT_W / 8, TOP_TOOLBAR_H, "Help");
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
	selectButton->callback(selectCallback);

	Fl_Radio_Button* translationButton = new Fl_Radio_Button(0, LEFT_TOOLBAR_H / 8, LEFT_TOOLBAR_W, LEFT_TOOLBAR_H / 8, "Move");
	translationButton->selection_color(0);
	translationButton->callback(translationCallback);
	
	Fl_Radio_Button* rotateButton = new Fl_Radio_Button(0, 2 * LEFT_TOOLBAR_H / 8, LEFT_TOOLBAR_W, LEFT_TOOLBAR_H / 8, "Rotate");
	rotateButton->selection_color(0);
	rotateButton->callback(rotateCallback);

	Fl_Radio_Button* scaleButton = new Fl_Radio_Button(0, 3 * LEFT_TOOLBAR_H / 8, LEFT_TOOLBAR_W, LEFT_TOOLBAR_H / 8, "Scale");
	scaleButton->selection_color(0);
	scaleButton->callback(scaleCallback);

	Fl_Radio_Button* shapeButton = new Fl_Radio_Button(0, 4 * LEFT_TOOLBAR_H / 8, LEFT_TOOLBAR_W, LEFT_TOOLBAR_H / 8, "Shape");
	shapeButton->selection_color(0);
	shapeButton->callback(shapeCallback);
	// buttons for resolutions?

	Fl_Radio_Button* extrudeButton = new Fl_Radio_Button(0, 5 * LEFT_TOOLBAR_H / 8, LEFT_TOOLBAR_W, LEFT_TOOLBAR_H / 8, "Extrude");
	extrudeButton->selection_color(0);
	extrudeButton->callback(extrudeCallback, rightPanel);

	// Subdivision button
	Fl_Radio_Button* subdivisionButton = new Fl_Radio_Button(0, 6 * LEFT_TOOLBAR_H / 8, 50, LEFT_TOOLBAR_H / 8, "Subdivide");
	subdivisionButton->selection_color(0);
	subdivisionButton->callback(subdivisionPanelCallback);
		
	Fl_Radio_Button* bevelButton = new Fl_Radio_Button(0, 7 * LEFT_TOOLBAR_H / 8, 50, LEFT_TOOLBAR_H / 8, "Bevel");
	bevelButton->selection_color(0);
	bevelButton->callback(bevelCallback);

	sideToolbar->resizable(sideToolbar);
	sideToolbar->end();


	/* Top Right panel */
	topRightPanel = new Fl_Window(VIEWPORT_W, 0, RIGHT_PANEL_W, VIEWPORT_H / 3);
	topRightPanel->resizable(nullptr);
	topRightPanel->color(FL_DARK_RED);
	topRightPanel->begin();
	topRightPanel->end();

	/* Right panel */
	rightPanel = new Fl_Window(VIEWPORT_W, VIEWPORT_H / 3 , RIGHT_PANEL_W, 650);
	rightPanel->resizable(nullptr);
	rightPanel->begin();

	drawRotationWidgets(0, 0, RIGHT_PANEL_W, 200);
	drawPositionWidgets(0, 200, RIGHT_PANEL_W, 200);
	drawScaleWidgets(0, 400, RIGHT_PANEL_W, 200);

	rightPanel->end();


	/* Bottom Toolbar */
	Fl_Window* bottomToolbar = new Fl_Window(150, VIEWPORT_H - 70, BOTTOM_TOOLBAR_W, BOTTOM_TOOLBAR_H);
	bottomToolbar->color(12);

	Fl_Radio_Button* vertexButton = new Fl_Radio_Button(0, 0, BOTTOM_TOOLBAR_W / 4, BOTTOM_TOOLBAR_H, "Vertex");
	vertexButton->callback(elementSelectCallback<VERTEX>);
	vertexButton->selection_color(0);

	Fl_Radio_Button* edgeButton = new Fl_Radio_Button(50, 0, BOTTOM_TOOLBAR_W / 4, BOTTOM_TOOLBAR_H, "Edge");
	edgeButton->callback(elementSelectCallback<EDGE>);
	edgeButton->selection_color(0);

	Fl_Radio_Button* faceButton = new Fl_Radio_Button(100, 0, BOTTOM_TOOLBAR_W / 4, BOTTOM_TOOLBAR_H, "Face");
	faceButton->callback(elementSelectCallback<FACE>);
	faceButton->selection_color(0);

	Fl_Radio_Button* objectButton = new Fl_Radio_Button(150, 0, BOTTOM_TOOLBAR_W / 4, BOTTOM_TOOLBAR_H, "Object");
	objectButton->callback(elementSelectCallback<OBJECT>);
	objectButton->selection_color(0);
	objectButton->value(1);

	bottomToolbar->resizable(bottomToolbar);
	bottomToolbar->end();

	// Window 
	window->end();
	window->resizable(viewport);
	window->show(argc, argv);
	viewport->show();
	viewport->take_focus();
	window->fullscreen();
	return Fl::run();
}