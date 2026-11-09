#pragma once
#include <QMainWindow>
#include <QTimer>

class NoteEditor;
class QAction;
class QComboBox;
class QDockWidget;
class QPlainTextEdit;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow();

protected:
    void closeEvent(QCloseEvent* e) override;

private slots:
    void newFile();
    void openFile();
    bool saveFile();
    bool saveFileAs();
    void syncActions();
    void refreshPreview();
    void updateTitle();

private:
    bool maybeSave();
    bool writeTo(const QString& path);
    void buildUi();

    NoteEditor* m_editor = nullptr;
    QPlainTextEdit* m_preview = nullptr;
    QDockWidget* m_dock = nullptr;
    QComboBox* m_heading = nullptr;
    QAction *m_bold = nullptr, *m_italic = nullptr, *m_strike = nullptr, *m_hl = nullptr,
            *m_code = nullptr, *m_sub = nullptr, *m_sup = nullptr;
    QString m_path;
    QTimer m_previewTimer;
};
