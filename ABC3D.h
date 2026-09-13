#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_ABC3D.h"

class ABC3D : public QMainWindow
{
    Q_OBJECT

public:
    ABC3D(QWidget *parent = nullptr);
    ~ABC3D();

private:
    Ui::ABC3DClass ui;
};

