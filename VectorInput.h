#pragma once

#include <QWidget>
#include "ui_VectorInput.h"
#include "helper.h"

class VectorInput : public QWidget
{
	Q_OBJECT

public:
	VectorInput(QWidget *parent = nullptr);
	~VectorInput();

private:
	Ui::VectorInputClass ui;

private slots:
	void emitValChanged() {
		Vec3f val;
		val.x = ui.xSpinner->value();
		val.y = ui.ySpinner->value();
		val.z = ui.zSpinner->value();
		emit valChanged(val);
	}

	void setEnabled(bool enabled) {
		ui.xSpinner->setEnabled(enabled);
		ui.ySpinner->setEnabled(enabled);
		ui.zSpinner->setEnabled(enabled);
	}

public slots:
	void setValue(const Vec3f& val) {
		ui.xSpinner->setValue(val.x);
		ui.ySpinner->setValue(val.y);
		ui.zSpinner->setValue(val.z);
	}

signals:
	void valChanged(const Vec3f& val);
};

