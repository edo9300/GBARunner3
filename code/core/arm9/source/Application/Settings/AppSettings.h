#pragma once
#include "DisplaySettings.h"
#include "RunSettings.h"

class AppSettings
{
public:
    /// @brief Settings related to how the GBA game is displayed.
    DisplaySettings displaySettings;

    /// @brief Settings related to the execution of the GBA game.
    RunSettings runSettings;
};
