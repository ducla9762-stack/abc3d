#include "StartMenu.h"

StartMenu::StartMenu(QWidget* parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	this->setWindowFlag(Qt::FramelessWindowHint, true);
	
}

StartMenu::~StartMenu() {
}

