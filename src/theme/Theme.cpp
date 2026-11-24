#include "Theme.h"

namespace md {
namespace {
    int currentIndex = 0;
}

const QVector<Theme>& themes()
{
    // Rows after the name:
    //   chrome, sidebar, canvas, page
    //   surface, hover, selection, border
    //   text, muted, accent, accentText
    //   link, highlight, inlineCode, codeBlock, quote
    //   keyword, string, number, comment
    static const QVector<Theme> list {
        { QStringLiteral("midnight"), QStringLiteral("Midnight"),
            "#1e2126", "#1b1e23", "#17191d", "#2a2e35",
            "#252930", "#2d323b", "#2f3d5c", "#3a404a",
            "#dde1e6", "#8b95a1", "#4f8cff", "#ffffff",
            "#6cb6ff", "#7a6200", "#3d444e", "#1f2329", "#323844",
            "#ff7b72", "#a5d6ff", "#79c0ff", "#8b949e" },
        { QStringLiteral("gruvbox-dark"), QStringLiteral("Gruvbox Dark"),
            "#282828", "#1d2021", "#1d2021", "#282828",
            "#32302f", "#3c3836", "#504945", "#504945",
            "#ebdbb2", "#a89984", "#d79921", "#1d2021",
            "#83a598", "#7c5f12", "#3c3836", "#1d2021", "#32302f",
            "#fb4934", "#b8bb26", "#d3869b", "#928374" },
        { QStringLiteral("gruvbox-light"), QStringLiteral("Gruvbox Light"),
            "#ebdbb2", "#f2e5bc", "#ebdbb2", "#fbf1c7",
            "#f9f5d7", "#e0cfa9", "#d5c4a1", "#d5c4a1",
            "#3c3836", "#7c6f64", "#af3a03", "#fbf1c7",
            "#076678", "#fadb7a", "#ebdbb2", "#f2e5bc", "#f2e5bc",
            "#9d0006", "#79740e", "#8f3f71", "#928374" },
        { QStringLiteral("tokyo-night"), QStringLiteral("Tokyo Night"),
            "#16161e", "#16161e", "#13141c", "#1a1b26",
            "#1f2335", "#292e42", "#283457", "#2f354d",
            "#c0caf5", "#737aa2", "#7aa2f7", "#16161e",
            "#7dcfff", "#5a4a2a", "#2a2f45", "#13141c", "#222639",
            "#bb9af7", "#9ece6a", "#ff9e64", "#565f89" },
        { QStringLiteral("tokyo-night-storm"), QStringLiteral("Tokyo Night Storm"),
            "#1f2335", "#1f2335", "#1b1e2e", "#24283b",
            "#292e42", "#2f3549", "#364a82", "#3b4261",
            "#c0caf5", "#787fa6", "#7aa2f7", "#1f2335",
            "#7dcfff", "#5e4d2c", "#343a55", "#1f2335", "#2c3149",
            "#bb9af7", "#9ece6a", "#ff9e64", "#565f89" },
        { QStringLiteral("tokyo-night-day"), QStringLiteral("Tokyo Night Day"),
            "#d0d5e3", "#d5d8e4", "#cbcfdc", "#e1e2e7",
            "#e9eaef", "#c4c8da", "#b7c1e3", "#b4b9d0",
            "#343b58", "#5a67a0", "#2e7de9", "#ffffff",
            "#007197", "#f0d58c", "#c9cddd", "#d2d5e1", "#d6d9e6",
            "#9854f1", "#587539", "#b15c00", "#848cb5" },
        { QStringLiteral("catppuccin-mocha"), QStringLiteral("Catppuccin Mocha"),
            "#181825", "#181825", "#11111b", "#1e1e2e",
            "#28283a", "#313244", "#45475a", "#313244",
            "#cdd6f4", "#a6adc8", "#cba6f7", "#11111b",
            "#89b4fa", "#5c4f2e", "#313244", "#11111b", "#27273a",
            "#cba6f7", "#a6e3a1", "#fab387", "#6c7086" },
        { QStringLiteral("catppuccin-latte"), QStringLiteral("Catppuccin Latte"),
            "#e6e9ef", "#e6e9ef", "#dce0e8", "#eff1f5",
            "#f6f7f9", "#ccd0da", "#bcc0cc", "#ccd0da",
            "#4c4f69", "#6c6f85", "#8839ef", "#ffffff",
            "#1e66f5", "#f5dc9a", "#dce0e8", "#e6e9ef", "#e6e9ef",
            "#8839ef", "#40a02b", "#fe640b", "#8c8fa1" },
        { QStringLiteral("nord"), QStringLiteral("Nord"),
            "#2e3440", "#2b303b", "#272c36", "#2e3440",
            "#3b4252", "#434c5e", "#4c566a", "#434c5e",
            "#d8dee9", "#93a0b8", "#88c0d0", "#2e3440",
            "#8fbcbb", "#6b5d35", "#434c5e", "#272c36", "#353c4a",
            "#81a1c1", "#a3be8c", "#b48ead", "#616e88" },
        { QStringLiteral("dracula"), QStringLiteral("Dracula"),
            "#21222c", "#21222c", "#191a21", "#282a36",
            "#303241", "#383a4a", "#44475a", "#44475a",
            "#f8f8f2", "#8a93bd", "#bd93f9", "#21222c",
            "#8be9fd", "#6b5f2a", "#44475a", "#1e1f29", "#313342",
            "#ff79c6", "#f1fa8c", "#bd93f9", "#6272a4" },
        { QStringLiteral("solarized-dark"), QStringLiteral("Solarized Dark"),
            "#002b36", "#00252f", "#001f27", "#002b36",
            "#073642", "#0a404e", "#0f4b5a", "#11495a",
            "#93a1a1", "#657b83", "#268bd2", "#fdf6e3",
            "#2aa198", "#5c4a00", "#073642", "#00212b", "#05333f",
            "#859900", "#2aa198", "#d33682", "#586e75" },
        { QStringLiteral("solarized-light"), QStringLiteral("Solarized Light"),
            "#eee8d5", "#eee8d5", "#e6dfca", "#fdf6e3",
            "#f5eedb", "#e4ddc8", "#d9d2bd", "#d6cfba",
            "#586e75", "#839496", "#268bd2", "#fdf6e3",
            "#268bd2", "#f2dc8a", "#eee8d5", "#f5eedb", "#f5eedb",
            "#859900", "#2aa198", "#d33682", "#93a1a1" },
    };
    return list;
}

const Theme& theme() { return themes().at(currentIndex); }

void setTheme(const QString& id)
{
    currentIndex = 0;
    for (int i = 0; i < themes().size(); ++i)
        if (themes().at(i).id == id)
            currentIndex = i;
}

} // namespace md
