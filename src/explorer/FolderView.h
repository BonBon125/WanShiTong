#pragma once
#include <QFileSystemWatcher>
#include <QFrame>

class QLabel;
class QListWidget;

// Card-style preview of the folders and notes inside a directory.
class FolderView : public QFrame {
    Q_OBJECT
public:
    explicit FolderView(QWidget* parent = nullptr);
    void setRoot(const QString& directory) { m_root = directory; }
    void showFolder(const QString& directory);
    void applyTheme();

signals:
    void openRequested(const QString& path);
    void newNoteRequested(const QString& directory);
    void newFolderRequested(const QString& directory);
    void renameRequested(const QString& path);
    void deleteRequested(const QString& path);

private:
    void refresh();
    void updateBreadcrumb();
    void showContextMenu(const QPoint& pos);

    QString m_root;
    QString m_directory;
    QStringList m_crumbs; // paths behind the breadcrumb links
    QLabel* m_breadcrumb = nullptr;
    QLabel* m_title = nullptr;
    QLabel* m_summary = nullptr;
    QLabel* m_empty = nullptr;
    QListWidget* m_cards = nullptr;
    QFileSystemWatcher m_watcher;
};
