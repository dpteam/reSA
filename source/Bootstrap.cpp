#include "StdInc.h"

#include "Bootstrap.h"

#include <extensions/Configuration.hpp>
#include <extensions/Configs/FastLoader.hpp>
#include <extensions/Configs/Miscellaneous.hpp>

namespace {
constexpr auto DEFAULT_INI_FILENAME = "gta-reversed.ini";
}

void notsa::bootstrap::LoadConfigurations() {
    // Firstly load the INI into the memory.
    g_ConfigurationMgr.Load(DEFAULT_INI_FILENAME);

    // Then load all specific configurations.
    g_FastLoaderConfig.Load();
    g_MiscConfig.Load();
    // ...
}
