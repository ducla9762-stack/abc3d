#include "customWindow.h"	

int customWindow::keyDown() {
		switch (Fl::event_key()) {
			break;
		case 'p':
			viewport->togglePoints();
			viewport->redraw();
			return 1;
			break;
		case 'g':
			viewport->toggleGrid();
			viewport->redraw();
			return 1;
			break;
		case FL_Control_L:
			viewport->enableAdditiveSelection();
			viewport->redraw();
			return 1;
			break;
		default:
			return 0;
		}
		redraw();
	}

	int customWindow::keyUp() {
		switch (Fl::event_key()) {
			break;
		case FL_Control_L:
			viewport->disableAdditiveSelection();
			viewport->redraw();
			return 1;
			break;
		default:
			return 0;
		}
		redraw();
	}

	int customWindow::handle(int event) {
		switch (event) {
			break;
		case FL_KEYDOWN:
			return keyDown();
			break;
		case FL_KEYUP:
			return keyUp();
			break;
		default:
			return Fl_Window::handle(event);
		}
	}