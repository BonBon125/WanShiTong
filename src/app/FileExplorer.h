#pragma once
#include <QWidget>

class QFileSystemModel;
class QModelIndex;
class QToolButton;
class QTreeView;

// Sidebar tree showing folders and .md files in the notes directory.
class FileExplorer : public QWidget {
    Q_OBJECT
public:
    explicit FileExplorer(QWidget* parent = nullptr);
    void setRootPath(const QString& path);
    void select(const QString& path);

signals:
    void noteActivated(const QString& path);
    void folderActivated(const QString& path);
    void newNoteRequested(const QString& dir);
    void newFolderRequested(const QString& dir);

private:
    void activate(const QModelIndex& index);
    void showContextMenu(const QPoint& pos);

    QFileSystemModel* m_model = nullptr;
    QTreeView* m_tree = nullptr;
    QToolButton* m_title = nullptr;
    QString m_root;
};
