#pragma once
#include <QString>

// Activates a theme for the whole application and remembers the choice.
void applyTheme(const QString& id);
QString savedThemeId();
