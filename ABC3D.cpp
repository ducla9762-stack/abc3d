
#include "ABC3D.h"

ABC3D::ABC3D(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    ui.transformModes->setVisible(false);
    ui.posInput->setEnabled(false);
    ui.scaleInput->setEnabled(false);
    ui.rotInput->setEnabled(false);
    //ui.topRightPanels->setCurrentIndex(3);

    QObject::connect(this, &ABC3D::deleteKeyPressed, ui.viewport, &Viewport::deleteSelection);
    QObject::connect(this, &ABC3D::deleteKeyPressed, ui.viewport, &Viewport::deleteSelection);

    QObject::connect(ui.subdivideToolButton, &QPushButton::released, this, [=]() { 
        ui.bottomRightPanels->setCurrentIndex(2); 
        });

    QObject::connect(ui.shapeToolButton, &QPushButton::released, this, [=]() {
        ui.bottomRightPanels->setCurrentIndex(1);
        });

    // The main Move tool defaults to Free translation. Its toolbar button and
    // the translation-mode buttons live in separate exclusive groups, so keep
    // the visible mode selection synchronized when returning to Move.
    QObject::connect(ui.translateToolButton, &QPushButton::released, this, [=]() {
        ui.freeModeTranslationButton->setChecked(true);
        });


}

ABC3D::~ABC3D()
{}

