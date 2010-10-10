
/**
 * @file main.cpp
 * @brief main implementation
 */

#include "Scene.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app (argc, argv);

    app.setOrganizationName("MentalDistortion");
    app.setApplicationName("FyreWare");

    Scene scene;
    scene.show();

    return app.exec();
}
