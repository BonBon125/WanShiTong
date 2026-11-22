#pragma once
#include <QList>
#include <QMainWindow>
#include <QTimer>

class FileExplorer;
class FolderView;
class NoteEditor;
class QAction;
class QComboBox;
class QDockWidget;
class QPlainTextEdit;
class QStackedWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(const QString& rootDirectory);

protected:
    void closeEvent(QCloseEvent* e) override;
    void changeEvent(QEvent* e) override;

private slots:
    void openPath(const QString& path);
    void createNote(const QString& directory);
    void createFolder(const QString& directory);
    void renamePath(const QString& path);
    void deletePath(const QString& path);
    void switchRoot();
    void changeTheme(const QString& id);
    bool saveFile();
    void autoSave();
    void syncActions();
    void refreshPreview();
    void updateTitle();

private:
    void buildUi();
    void setRoot(const QString& directory);
    void openNote(const QString& path);
    void showFolder(const QString& path);
    bool closeNote(); // returns false if the user cancels
    bool maybeSave();
    bool writeTo(const QString& path, bool quiet = false);
    void setNoteActionsEnabled(bool enabled);
    void updateItemActions();
    QString currentItem() const; // what Rename/Delete in the menu act on
    QString targetDirectory() const; // where "New note" creates files
    QString askName(const QString& title, const QString& initial);

    NoteEditor* m_editor = nullptr;
    FileExplorer* m_explorer = nullptr;
    FolderView* m_folderView = nullptr;
    QStackedWidget* m_stack = nullptr;
    QPlainTextEdit* m_preview = nullptr;
    QDockWidget* m_dock = nullptr;
    QComboBox* m_heading = nullptr;
    QAction *m_bold = nullptr, *m_italic = nullptr, *m_strike = nullptr, *m_hl = nullptr,
            *m_code = nullptr, *m_sub = nullptr, *m_sup = nullptr;
    QAction *m_renameAction = nullptr, *m_deleteAction = nullptr;
    QList<QAction*> m_noteActions; // only usable while a note is open
    QString m_root; // the chosen notes directory
    QString m_path; // open note (empty while a folder is shown)
    QString m_folder; // folder currently shown
    bool m_autoSave = true;
    QTimer m_previewTimer;
    QTimer m_autoSaveTimer;
};
