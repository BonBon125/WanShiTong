#include "app/MainWindow.h"
#include "app/WelcomeDialog.h"
#include "theme/AppTheme.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setOrganizationName("WanShiTong");
    app.setApplicationName("WanShiTong");
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    applyTheme(savedThemeId());

    WelcomeDialog welcome;
    if (welcome.exec() != QDialog::Accepted)
        return 0;

    MainWindow w(welcome.selectedPath());
    w.show();
    return app.exec();
}
