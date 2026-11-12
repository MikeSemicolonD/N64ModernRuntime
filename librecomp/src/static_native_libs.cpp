#include "librecomp/static_native_libs.hpp"

namespace {
    // Function-local, so registrations from other files' static initializers find it constructed.
    std::unordered_map<std::string, recomp::mods::StaticNativeLibrary>& registry() {
        static std::unordered_map<std::string, recomp::mods::StaticNativeLibrary> libs;
        return libs;
    }
}

void recomp::mods::register_static_native_library(const std::string& name, uint32_t api_version, std::unordered_map<std::string, recomp_func_t*> exports) {
    registry()[name] = StaticNativeLibrary{ api_version, std::move(exports) };
}

const recomp::mods::StaticNativeLibrary* recomp::mods::find_static_native_library(const std::string& name) {
    auto it = registry().find(name);
    return it == registry().end() ? nullptr : &it->second;
}
