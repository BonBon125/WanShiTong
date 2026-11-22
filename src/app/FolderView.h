#pragma once
#include <QFileSystemWatcher>
#include <QWidget>

class QLabel;
class QListWidget;
class QListWidgetItem;

// Card grid previewing the notes and sub-folders of one directory.
class FolderView : public QWidget {
    Q_OBJECT
public:
    explicit FolderView(QWidget* parent = nullptr);
    void showFolder(const QString& dir);

signals:
    void noteActivated(const QString& path);
    void folderActivated(const QString& path);
    void newNoteRequested(const QString& dir);
    void newFolderRequested(const QString& dir);

private:
    void reload();
    void activate(const QListWidgetItem* item);

    QString m_dir;
    QLabel* m_title = nullptr;
    QLabel* m_summary = nullptr;
    QLabel* m_empty = nullptr;
    QListWidget* m_cards = nullptr;
    QFileSystemWatcher m_watcher;
};
