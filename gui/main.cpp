#include <QApplication>

#include "gui/CarveWorker.hpp"
#include "gui/MainWindow.hpp"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    qRegisterMetaType<sc::CarveResult>("sc::CarveResult");  // needed to cross threads in a signal

    MainWindow window;
    window.show();
    if (argc > 1) window.loadImage(QString::fromLocal8Bit(argv[1]));  // optional: seamcarve_app image.png
    return app.exec();
}
