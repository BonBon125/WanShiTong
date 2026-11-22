#include "app/MainWindow.h"
#include "app/WelcomeDialog.h"
#include "theme/DarkTheme.h"
#include <QApplication>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setOrganizationName("mdnote");
    app.setApplicationName("mdnote");
    applyDarkTheme(app);

    WelcomeDialog welcome;
    if (welcome.exec() != QDialog::Accepted)
        return 0;

    MainWindow w(welcome.selectedPath());
    w.show();
    return app.exec();
}
