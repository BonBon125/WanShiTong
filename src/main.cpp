#include "app/MainWindow.h"
#include "app/WelcomeDialog.h"
#include "theme/Themes.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setOrganizationName("mdnote");
    app.setApplicationName("mdnote");
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    useTheme(savedThemeId());

    WelcomeDialog welcome;
    if (welcome.exec() != QDialog::Accepted)
        return 0;

    MainWindow w(welcome.selectedPath());
    w.show();
    return app.exec();
}
