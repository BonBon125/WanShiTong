#pragma once
#include <QDialog>

// Startup chooser: open a recent notes directory, pick another, or create one.
class WorkspaceDialog : public QDialog {
    Q_OBJECT
public:
    explicit WorkspaceDialog(QWidget* parent = nullptr);
    QString selectedPath() const { return m_path; }
    static void remember(const QString& path);

private:
    void choose(const QString& path);
    void openExisting();
    void createNew();

    QString m_path;
};
