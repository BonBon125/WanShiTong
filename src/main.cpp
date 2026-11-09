#include "app/MainWindow.h"
#include "theme/DarkTheme.h"
#include <QApplication>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setApplicationName("mdnote");
    applyDarkTheme(app);

    MainWindow w;
    w.show();
    return app.exec();
}
