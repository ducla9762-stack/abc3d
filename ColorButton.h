#pragma once

#include <QPushButton>
#include <QColor>
#include <QColorDialog>

#include <cyVector.h>
#include "helper.h"

class ColorButton  : public QPushButton
{
	Q_OBJECT

	QColor clr;

public:
	ColorButton(QWidget* parent);
	~ColorButton();

signals:
	void ColorSelected(const Vec3f& selectedClr);

private:
	void mousePressEvent(QMouseEvent* event) override {
		QPushButton::mousePressEvent(event);

		clr = QColorDialog::getColor(clr, this, "Pick a color");

		setStyleSheet("background-color: " + clr.name() + ";");

		cy::Vec3f selectedClr(clr.redF(), clr.greenF(), clr.blueF());
		emit ColorSelected(selectedClr);
	}
};

