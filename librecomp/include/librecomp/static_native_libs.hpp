#ifndef __RECOMP_STATIC_NATIVE_LIBS_HPP__
#define __RECOMP_STATIC_NATIVE_LIBS_HPP__

#include <cstdint>
#include <string>
#include <unordered_map>

#include "recomp.h"

namespace recomp {
    namespace mods {
        // A mod's native library linked into the executable, for builds that can't load library files (the browser).
        struct StaticNativeLibrary {
            uint32_t api_version;
            std::unordered_map<std::string, recomp_func_t*> exports;
        };

        // Registers a library under the name a mod.json's native_libraries entry uses; load_native_library then takes it instead of a file.
        void register_static_native_library(const std::string& name, uint32_t api_version, std::unordered_map<std::string, recomp_func_t*> exports);
        const StaticNativeLibrary* find_static_native_library(const std::string& name);
    }
}

#endif
