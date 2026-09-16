#ifndef _ABC3D_CUSTOMWINDOW_H_
#define _ABC3D_CUSTOMWINDOW_H_

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Tile.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/fl_ask.H>
#include <Fl/Fl_PNG_Image.H>
#include <FL/Fl_Tooltip.H>
#include <print>

#include "viewport.h"
#include "ui.h"

class customWindow : public Fl_Window {
public:

	customWindow(int x, int y, int w, int h, const char* title) : Fl_Window(x, y, w, h, title) {}

	int keyDown();

	int keyUp();

	int handle(int event);
};

#endif