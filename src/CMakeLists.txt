add_library(WanShiTong_core STATIC
    markdown/MdFormat.h
    markdown/MarkdownParser.h
    markdown/MarkdownParser.cpp
    markdown/MarkdownWriter.h
    markdown/MarkdownWriter.cpp
    theme/Theme.h
    theme/Theme.cpp)
target_include_directories(WanShiTong_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(WanShiTong_core PUBLIC Qt6::Gui)
target_compile_options(WanShiTong_core PRIVATE -Wall -Wextra)

add_executable(WanShiTong
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
    theme/AppTheme.h
    theme/AppTheme.cpp
    ui/Icons.h
    ui/Icons.cpp)
target_link_libraries(WanShiTong PRIVATE WanShiTong_core Qt6::Widgets)
target_compile_options(WanShiTong PRIVATE -Wall -Wextra)
