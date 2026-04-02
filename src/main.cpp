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

#include "Viewport.h"

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

int main(int argc, char* argv[]) {
	Fl_Window* window = new Fl_Window(0, 0, 1080, 720, "ABC3D");

	viewport = new Viewport(0, 0, VIEWPORT_W, VIEWPORT_H);
	viewport->end();

	Fl_Window* rightPanel = new Fl_Window(VIEWPORT_W, 0, RIGHT_PANEL_W, 650);
	rightPanel->resizable(nullptr);
	rightPanel->begin();

	Fl_Window* scaleWidgets = new Fl_Window(0, 0, RIGHT_PANEL_W, 200);
	scaleWidgets->begin();
	Fl_Box* scaleLabel = new Fl_Box(0, 0, RIGHT_PANEL_W, 50);
	scaleLabel->label("Scale");

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

	Fl_Window* posWidgets = new Fl_Window(0, 200, RIGHT_PANEL_W, 200);
	posWidgets->begin();
	Fl_Box* posLabel = new Fl_Box(0, 0, RIGHT_PANEL_W, 50);
	posLabel->label("Position");

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

	Fl_Window* rotWidgets = new Fl_Window(0, 400, RIGHT_PANEL_W, 200);
	rotWidgets->begin();
	Fl_Box* rotLabel = new Fl_Box(0, 0, RIGHT_PANEL_W, 50);
	rotLabel->label("Rotation");

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

	Fl_Button* fileButton = new Fl_Button(0, 600, RIGHT_PANEL_W, 50, "Pick File");
	fileButton->callback(fileButtonCallback);

	rightPanel->end();
	
	window->end();
	window->resizable(viewport);
	window->show(argc, argv);
	viewport->show();
	viewport->take_focus();
	return Fl::run();
}