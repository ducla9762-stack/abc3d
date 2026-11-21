#pragma once

#include <QWidget>
#include <QSignalBlocker>
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

public slots:
	// Programmatic refresh (the viewport reporting a drag, undo/redo): emits nothing, so the
	// value is not applied back to the selection, which would record extra undo steps.
	void setValue(const Vec3f& val) {
		const QSignalBlocker blockX(ui.xSpinner);
		const QSignalBlocker blockY(ui.ySpinner);
		const QSignalBlocker blockZ(ui.zSpinner);
		ui.xSpinner->setValue(val.x);
		ui.ySpinner->setValue(val.y);
		ui.zSpinner->setValue(val.z);
	}

	void setEnabled(bool enabled) {
		ui.xSpinner->setEnabled(enabled);
		ui.ySpinner->setEnabled(enabled);
		ui.zSpinner->setEnabled(enabled);
	}

signals:
	void valChanged(const Vec3f& val);
};

