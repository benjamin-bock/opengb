#pragma once

#include <optional>
#include <string>

struct LauncherConfig {
    std::string romPath;
    float volume = 0.8f;
};

// Shows the launcher window. Returns nullopt if the user closed it,
// otherwise the chosen ROM path and volume.
std::optional<LauncherConfig> runLauncher();
