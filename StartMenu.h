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

signals:
};

#pragma once
