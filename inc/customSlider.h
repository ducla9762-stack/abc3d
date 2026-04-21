#ifndef _ABC3D_CUSTOMSLIDER_H_
#define _ABC3D_CUSTOMSLIDER_H_
#include <FL/Fl_Window.H>
#include <FL/Fl.H>
#include <stdio.h>
#include <FL/Fl_Simple_Counter.h>

class customSlider : public Fl_Simple_Counter {
	private:
		int handleDrag(int event, int button, int x, int y);
	
	public:
		customSlider(int x, int y, int w, int h);
		int handle(int e);
};
#endif
