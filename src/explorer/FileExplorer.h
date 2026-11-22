#pragma once
#include <QFrame>

class QFileSystemModel;
class QPushButton;
class QTreeView;

// Sidebar tree showing the folders and .md files inside the notes directory.
class FileExplorer : public QFrame {
    Q_OBJECT
public:
    explicit FileExplorer(QWidget* parent = nullptr);
    void setRoot(const QString& directory);
    void select(const QString& path);

signals:
    void openRequested(const QString& path);
    void newNoteRequested(const QString& directory);
    void newFolderRequested(const QString& directory);
    void renameRequested(const QString& path);
    void deleteRequested(const QString& path);

private:
    void showContextMenu(const QPoint& pos);
    QString selectedDirectory() const;
    QString selectedItem() const;

    QFileSystemModel* m_model = nullptr;
    QTreeView* m_tree = nullptr;
    QPushButton* m_title = nullptr;
    QString m_root;
};
