#pragma once

#include <QPushButton>
#include <QColor>
#include <QColorDialog>

#include <cyVector.h>
#include "helper.h"

class ColorButton  : public QPushButton
{
	Q_OBJECT

	Q_PROPERTY(QColor color READ GetColor WRITE SetColor)

	QColor clr;

signals:
	void ColorSelected(const Vec3f& selectedClr);

public:
	ColorButton(QWidget* parent);
	~ColorButton();
	QColor GetColor() { return clr; }
	Vec3f GetColorF() { return Vec3f(clr.redF(), clr.greenF(), clr.blueF()); }
	void SetColor(QColor newColor) { 
		clr = newColor;
		setStyleSheet("ColorButton { background-color: " + clr.name() + "; }");
		Vec3f selectedClr(clr.redF(), clr.greenF(), clr.blueF());
		emit ColorSelected(selectedClr);
	}

private:
	void mousePressEvent(QMouseEvent* event) override {
		QPushButton::mousePressEvent(event);

		QColor newClr = QColorDialog::getColor(clr, this, "Pick a color");

		if (newClr.isValid()) {
			clr = newClr;
			setStyleSheet("ColorButton { background-color: " + clr.name() + "; }");

			cy::Vec3f selectedClr(clr.redF(), clr.greenF(), clr.blueF());
			emit ColorSelected(selectedClr);
		}
	}
};

