#pragma once

#include <QWidget>
#include "ui_StartMenu.h"

class StartMenu : public QDialog
{
	Q_OBJECT

public:
	StartMenu(QWidget* parent = nullptr);
	~StartMenu();

private:
	Ui::StartMenuClass ui;

private slots:
	// Slots that change the integer returned from exec
	void loadModelButtonClicked() { done(4); }
	void loadProjectButtonClicked() { done(5); }
	void websiteButtonClicked() { done(6);  }

signals:
};

#pragma once
