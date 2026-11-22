#pragma once
#include <QDialog>

class QListWidget;

// Shown at launch: open an existing notes directory or create a new one.
class WelcomeDialog : public QDialog {
    Q_OBJECT
public:
    explicit WelcomeDialog(QWidget* parent = nullptr);
    QString selectedPath() const { return m_path; }

private:
    void openExisting();
    void createNew();
    void choose(const QString& path);

    QListWidget* m_recent = nullptr;
    QString m_path;
};
