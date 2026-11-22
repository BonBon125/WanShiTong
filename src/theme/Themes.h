#pragma once
#include "Theme.h"
#include <QString>
#include <QVector>

const QVector<Theme>& availableThemes();
QString savedThemeId();

// Makes the theme active, applies it to the application and remembers it.
void useTheme(const QString& id);
