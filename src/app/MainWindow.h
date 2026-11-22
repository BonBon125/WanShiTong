#pragma once
#include <QMainWindow>
#include <QTimer>
#include <QVector>

class FileExplorer;
class FolderView;
class NoteEditor;
class QAction;
class QComboBox;
class QDockWidget;
class QLabel;
class QPlainTextEdit;
class QSplitter;
class QStackedWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(const QString& workspace);

protected:
    void closeEvent(QCloseEvent* e) override;

private slots:
    bool openNote(const QString& path);
    bool showFolder(const QString& dir);
    void newNote(const QString& dir = QString());
    void newFolder(const QString& dir = QString());
    void openFileDialog();
    void switchWorkspace();
    bool saveFile();
    bool saveFileAs();
    void syncActions();
    void refreshPreview();
    void updateTitle();

private:
    void buildLayout();
    void buildActions();
    void setWorkspace(const QString& dir);
    void displayFolder(const QString& dir); // no "unsaved changes" check
    void setEditing(bool editing);
    bool isEditing() const;
    QString currentFolder() const;
    QString askName(const QString& title, const QString& label);
    void updatePathBar();
    bool maybeSave();
    bool writeTo(const QString& path);

    NoteEditor* m_editor = nullptr;
    FileExplorer* m_explorer = nullptr;
    FolderView* m_folderView = nullptr;
    QSplitter* m_splitter = nullptr;
    QStackedWidget* m_stack = nullptr;
    QLabel* m_pathBar = nullptr;
    QPlainTextEdit* m_preview = nullptr;
    QDockWidget* m_dock = nullptr;
    QComboBox* m_heading = nullptr;
    QAction *m_bold = nullptr, *m_italic = nullptr, *m_strike = nullptr, *m_hl = nullptr,
            *m_code = nullptr, *m_sub = nullptr, *m_sup = nullptr;
    QAction* m_sidebarAction = nullptr;
    QVector<QAction*> m_editorActions; // only enabled while a note is open

    QString m_root; // notes directory
    QString m_folder; // folder being shown / containing the open note
    QString m_path; // open note, empty in folder view
    QTimer m_previewTimer;
};
