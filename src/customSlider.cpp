#include "customSlider.h"

customSlider::customSlider(int x, int y, int w, int h) : Fl_Simple_Counter(x, y, w, h) {

}

int customSlider::handle(int event) {
	switch (event) {
    case FL_PUSH:

    case FL_DRAG:
        handleDrag(event, Fl::event_button(), Fl::event_x(), Fl::event_y());
        return 1;
    default:
        return Fl_Simple_Counter::handle(event);
	};
}

int customSlider::handleDrag(int event, int button, int x, int y) {
    return 0;
}