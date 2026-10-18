#include "gui/main_window.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    scapms::gui::MainWindow window;
    window.show();
    return application.exec();
}
