add_library(mdnote_core STATIC
    markdown/MdFormat.h
    markdown/MarkdownParser.h
    markdown/MarkdownParser.cpp
    markdown/MarkdownWriter.h
    markdown/MarkdownWriter.cpp)
target_include_directories(mdnote_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mdnote_core PUBLIC Qt6::Gui)
target_compile_options(mdnote_core PRIVATE -Wall -Wextra)

add_executable(mdnote
    main.cpp
    app/MainWindow.h
    app/MainWindow.cpp
    app/WorkspaceDialog.h
    app/WorkspaceDialog.cpp
    app/FileExplorer.h
    app/FileExplorer.cpp
    app/FolderView.h
    app/FolderView.cpp
    editor/NoteEditor.h
    editor/NoteEditor.cpp
    editor/CodeHighlighter.h
    editor/CodeHighlighter.cpp
    theme/DarkTheme.h
    theme/DarkTheme.cpp
    theme/Icons.h
    theme/Icons.cpp)
target_link_libraries(mdnote PRIVATE mdnote_core Qt6::Widgets)
target_compile_options(mdnote PRIVATE -Wall -Wextra)
