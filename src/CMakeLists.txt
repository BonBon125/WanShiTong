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
    app/WelcomeDialog.h
    app/WelcomeDialog.cpp
    editor/NoteEditor.h
    editor/NoteEditor.cpp
    editor/CodeHighlighter.h
    editor/CodeHighlighter.cpp
    explorer/FileExplorer.h
    explorer/FileExplorer.cpp
    explorer/FolderView.h
    explorer/FolderView.cpp
    theme/DarkTheme.h
    theme/DarkTheme.cpp
    ui/Icons.h
    ui/Icons.cpp)
target_link_libraries(mdnote PRIVATE mdnote_core Qt6::Widgets)
target_compile_options(mdnote PRIVATE -Wall -Wextra)
