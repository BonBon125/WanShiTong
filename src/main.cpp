#include "app/MainWindow.h"
#include "app/WorkspaceDialog.h"
#include "theme/DarkTheme.h"

#include <QApplication>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setOrganizationName("mdnote"); // used by QSettings
    app.setApplicationName("mdnote");
    applyDarkTheme(app);

    WorkspaceDialog chooser;
    if (chooser.exec() != QDialog::Accepted)
        return 0;

    MainWindow w(chooser.selectedPath());
    w.show();
    return app.exec();
}
