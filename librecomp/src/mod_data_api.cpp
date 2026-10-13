// The recompdata.h collections for mods: u32-keyed hashmaps, hashsets and slotmaps behind integer handles.
// Data elements live on the mod heap (recomp::alloc), so the pointers mods get back are ordinary game addresses.
#include <cstring>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include "librecomp/addresses.hpp"
#include "librecomp/helpers.hpp"
#include "librecomp/mods.hpp"
#include "librecomp/overlays.hpp"

namespace {
using Key = uint32_t;

struct ValueMap {
    std::unordered_map<Key, uint32_t> elements;
    Key next_key = 1;
};

// Elements are rdram offsets of element_size-byte blocks on the mod heap.
struct MemoryMap {
    uint32_t element_size = 0;
    std::unordered_map<Key, uint32_t> elements;
    Key next_key = 1;
};

template <typename T>
struct Registry {
    std::unordered_map<uint32_t, T> items;
    uint32_t next_handle = 1;

    uint32_t create(T item) {
        uint32_t handle = next_handle++;
        items.emplace(handle, std::move(item));
        return handle;
    }
    T* get(uint32_t handle) {
        auto it = items.find(handle);
        return it == items.end() ? nullptr : &it->second;
    }
};

std::mutex data_mutex;
Registry<ValueMap> value_hashmaps;
Registry<MemoryMap> memory_hashmaps;
Registry<std::unordered_set<Key>> hashsets;
Registry<ValueMap> value_slotmaps;
Registry<MemoryMap> memory_slotmaps;

uint32_t handle_arg(uint8_t* rdram, recomp_context* ctx) { return _arg<0, uint32_t>(rdram, ctx); }
Key key_arg(uint8_t* rdram, recomp_context* ctx) { return _arg<1, uint32_t>(rdram, ctx); }

void return_address(recomp_context* ctx, uint32_t rdram_offset) {
    ctx->r2 = rdram_offset == 0 ? 0 : (gpr)(int32_t)(rdram_offset + 0x80000000u);
}

uint32_t alloc_element(uint8_t* rdram, uint32_t size) {
    uint8_t* mem = reinterpret_cast<uint8_t*>(recomp::alloc(rdram, size));
    std::memset(mem, 0, size);
    return static_cast<uint32_t>(mem - rdram);
}

void free_element(uint8_t* rdram, uint32_t rdram_offset) {
    recomp::free(rdram, rdram + rdram_offset);
}

void destroy_memory_map(uint8_t* rdram, Registry<MemoryMap>& registry, uint32_t handle) {
    MemoryMap* map = registry.get(handle);
    if (map == nullptr) {
        return;
    }
    for (const auto& [key, offset] : map->elements) {
        free_element(rdram, offset);
    }
    registry.items.erase(handle);
}

int erase_memory_element(uint8_t* rdram, MemoryMap* map, Key key) {
    if (map == nullptr) {
        return 0;
    }
    auto it = map->elements.find(key);
    if (it == map->elements.end()) {
        return 0;
    }
    free_element(rdram, it->second);
    map->elements.erase(it);
    return 1;
}

template <typename Map>
void contains(uint8_t* rdram, recomp_context* ctx, Registry<Map>& registry) {
    std::lock_guard lock{ data_mutex };
    Map* map = registry.get(handle_arg(rdram, ctx));
    _return<int>(ctx, map != nullptr && map->elements.count(key_arg(rdram, ctx)) != 0);
}

template <typename Map>
void size(uint8_t* rdram, recomp_context* ctx, Registry<Map>& registry) {
    std::lock_guard lock{ data_mutex };
    Map* map = registry.get(handle_arg(rdram, ctx));
    _return<uint32_t>(ctx, map == nullptr ? 0 : static_cast<uint32_t>(map->elements.size()));
}

// u32 -> u32 hashmaps

void create_u32_value_hashmap(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ data_mutex };
    _return<uint32_t>(ctx, value_hashmaps.create({}));
}

void destroy_u32_value_hashmap(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ data_mutex };
    value_hashmaps.items.erase(handle_arg(rdram, ctx));
}

void u32_value_hashmap_contains(uint8_t* rdram, recomp_context* ctx) { contains(rdram, ctx, value_hashmaps); }

void u32_value_hashmap_insert(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ data_mutex };
    ValueMap* map = value_hashmaps.get(handle_arg(rdram, ctx));
    if (map == nullptr) {
        _return<int>(ctx, 0);
        return;
    }
    auto [it, created] = map->elements.insert_or_assign(key_arg(rdram, ctx), _arg<2, uint32_t>(rdram, ctx));
    _return<int>(ctx, created ? 1 : 0);
}

void u32_value_hashmap_get(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ data_mutex };
    ValueMap* map = value_hashmaps.get(handle_arg(rdram, ctx));
    if (map == nullptr) {
        _return<int>(ctx, 0);
        return;
    }
    auto it = map->elements.find(key_arg(rdram, ctx));
    if (it == map->elements.end()) {
        _return<int>(ctx, 0);
        return;
    }
    *_arg<2, uint32_t*>(rdram, ctx) = it->second;
    _return<int>(ctx, 1);
}

void u32_value_hashmap_erase(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ data_mutex };
    ValueMap* map = value_hashmaps.get(handle_arg(rdram, ctx));
    _return<int>(ctx, map != nullptr && map->elements.erase(key_arg(rdram, ctx)) != 0);
}

void u32_value_hashmap_size(uint8_t* rdram, recomp_context* ctx) { size(rdram, ctx, value_hashmaps); }

// u32 -> data hashmaps

void create_u32_memory_hashmap(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ data_mutex };
    MemoryMap map{};
    map.element_size = _arg<0, uint32_t>(rdram, ctx);
    _return<uint32_t>(ctx, memory_hashmaps.create(std::move(map)));
}

void destroy_u32_memory_hashmap(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ data_mutex };
    destroy_memory_map(rdram, memory_hashmaps, handle_arg(rdram, ctx));
}

void u32_memory_hashmap_contains(uint8_t* rdram, recomp_context* ctx) { contains(rdram, ctx, memory_hashmaps); }

void u32_memory_hashmap_create(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ data_mutex };
    MemoryMap* map = memory_hashmaps.get(handle_arg(rdram, ctx));
    Key key = key_arg(rdram, ctx);
    if (map == nullptr || map->elements.count(key) != 0) {
        _return<int>(ctx, 0);
        return;
    }
    map->elements.emplace(key, alloc_element(rdram, map->element_size));
    _return<int>(ctx, 1);
}

void u32_memory_hashmap_get(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ data_mutex };
    MemoryMap* map = memory_hashmaps.get(handle_arg(rdram, ctx));
    uint32_t offset = 0;
    if (map != nullptr) {
        auto it = map->elements.find(key_arg(rdram, ctx));
        offset = it == map->elements.end() ? 0 : it->second;
    }
    return_address(ctx, offset);
}

void u32_memory_hashmap_erase(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ data_mutex };
    _return<int>(ctx, erase_memory_element(rdram, memory_hashmaps.get(handle_arg(rdram, ctx)), key_arg(rdram, ctx)));
}

void u32_memory_hashmap_size(uint8_t* rdram, recomp_context* ctx) { size(rdram, ctx, memory_hashmaps); }

// u32 hashsets

void create_u32_hashset(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ data_mutex };
    _return<uint32_t>(ctx, hashsets.create({}));
}

void destroy_u32_hashset(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ data_mutex };
    hashsets.items.erase(handle_arg(rdram, ctx));
}

void u32_hashset_contains(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ data_mutex };
    auto* set = hashsets.get(handle_arg(rdram, ctx));
    _return<int>(ctx, set != nullptr && set->count(key_arg(rdram, ctx)) != 0);
}

void u32_hashset_insert(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ data_mutex };
    auto* set = hashsets.get(handle_arg(rdram, ctx));
    _return<int>(ctx, set != nullptr && set->insert(key_arg(rdram, ctx)).second);
}

void u32_hashset_erase(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ data_mutex };
    auto* set = hashsets.get(handle_arg(rdram, ctx));
    _return<int>(ctx, set != nullptr && set->erase(key_arg(rdram, ctx)) != 0);
}

void u32_hashset_size(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ data_mutex };
    auto* set = hashsets.get(handle_arg(rdram, ctx));
    _return<uint32_t>(ctx, set == nullptr ? 0 : static_cast<uint32_t>(set->size()));
}

// u32 slotmaps

void create_u32_slotmap(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ data_mutex };
    _return<uint32_t>(ctx, value_slotmaps.create({}));
}

void destroy_u32_slotmap(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ data_mutex };
    value_slotmaps.items.erase(handle_arg(rdram, ctx));
}

void u32_slotmap_contains(uint8_t* rdram, recomp_context* ctx) { contains(rdram, ctx, value_slotmaps); }

void u32_slotmap_create(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ data_mutex };
    ValueMap* map = value_slotmaps.get(handle_arg(rdram, ctx));
    if (map == nullptr) {
        _return<uint32_t>(ctx, 0);
        return;
    }
    Key key = map->next_key++;
    map->elements.emplace(key, 0);
    _return<uint32_t>(ctx, key);
}

void u32_slotmap_get(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ data_mutex };
    ValueMap* map = value_slotmaps.get(handle_arg(rdram, ctx));
    if (map == nullptr) {
        _return<int>(ctx, 0);
        return;
    }
    auto it = map->elements.find(key_arg(rdram, ctx));
    if (it == map->elements.end()) {
        _return<int>(ctx, 0);
        return;
    }
    *_arg<2, uint32_t*>(rdram, ctx) = it->second;
    _return<int>(ctx, 1);
}

void u32_slotmap_set(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ data_mutex };
    ValueMap* map = value_slotmaps.get(handle_arg(rdram, ctx));
    if (map == nullptr) {
        _return<int>(ctx, 0);
        return;
    }
    auto it = map->elements.find(key_arg(rdram, ctx));
    if (it == map->elements.end()) {
        _return<int>(ctx, 0);
        return;
    }
    it->second = _arg<2, uint32_t>(rdram, ctx);
    _return<int>(ctx, 1);
}

void u32_slotmap_erase(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ data_mutex };
    ValueMap* map = value_slotmaps.get(handle_arg(rdram, ctx));
    _return<int>(ctx, map != nullptr && map->elements.erase(key_arg(rdram, ctx)) != 0);
}

void u32_slotmap_size(uint8_t* rdram, recomp_context* ctx) { size(rdram, ctx, value_slotmaps); }

// data slotmaps

void create_memory_slotmap(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ data_mutex };
    MemoryMap map{};
    map.element_size = _arg<0, uint32_t>(rdram, ctx);
    _return<uint32_t>(ctx, memory_slotmaps.create(std::move(map)));
}

void destroy_memory_slotmap(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ data_mutex };
    destroy_memory_map(rdram, memory_slotmaps, handle_arg(rdram, ctx));
}

void memory_slotmap_contains(uint8_t* rdram, recomp_context* ctx) { contains(rdram, ctx, memory_slotmaps); }

void memory_slotmap_create(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ data_mutex };
    MemoryMap* map = memory_slotmaps.get(handle_arg(rdram, ctx));
    if (map == nullptr) {
        _return<uint32_t>(ctx, 0);
        return;
    }
    Key key = map->next_key++;
    map->elements.emplace(key, alloc_element(rdram, map->element_size));
    _return<uint32_t>(ctx, key);
}

void memory_slotmap_get(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ data_mutex };
    MemoryMap* map = memory_slotmaps.get(handle_arg(rdram, ctx));
    if (map == nullptr) {
        _return<int>(ctx, 0);
        return;
    }
    auto it = map->elements.find(key_arg(rdram, ctx));
    if (it == map->elements.end()) {
        _return<int>(ctx, 0);
        return;
    }
    *_arg<2, uint32_t*>(rdram, ctx) = it->second + 0x80000000u;
    _return<int>(ctx, 1);
}

void memory_slotmap_erase(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard lock{ data_mutex };
    _return<int>(ctx, erase_memory_element(rdram, memory_slotmaps.get(handle_arg(rdram, ctx)), key_arg(rdram, ctx)));
}

void memory_slotmap_size(uint8_t* rdram, recomp_context* ctx) { size(rdram, ctx, memory_slotmaps); }
}

void recomp::mods::register_data_exports() {
#define REGISTER_DATA_FUNC(name) recomp::overlays::register_base_export("recomputil_" #name, name)
    REGISTER_DATA_FUNC(create_u32_value_hashmap);
    REGISTER_DATA_FUNC(destroy_u32_value_hashmap);
    REGISTER_DATA_FUNC(u32_value_hashmap_contains);
    REGISTER_DATA_FUNC(u32_value_hashmap_insert);
    REGISTER_DATA_FUNC(u32_value_hashmap_get);
    REGISTER_DATA_FUNC(u32_value_hashmap_erase);
    REGISTER_DATA_FUNC(u32_value_hashmap_size);
    REGISTER_DATA_FUNC(create_u32_memory_hashmap);
    REGISTER_DATA_FUNC(destroy_u32_memory_hashmap);
    REGISTER_DATA_FUNC(u32_memory_hashmap_contains);
    REGISTER_DATA_FUNC(u32_memory_hashmap_create);
    REGISTER_DATA_FUNC(u32_memory_hashmap_get);
    REGISTER_DATA_FUNC(u32_memory_hashmap_erase);
    REGISTER_DATA_FUNC(u32_memory_hashmap_size);
    REGISTER_DATA_FUNC(create_u32_hashset);
    REGISTER_DATA_FUNC(destroy_u32_hashset);
    REGISTER_DATA_FUNC(u32_hashset_contains);
    REGISTER_DATA_FUNC(u32_hashset_insert);
    REGISTER_DATA_FUNC(u32_hashset_erase);
    REGISTER_DATA_FUNC(u32_hashset_size);
    REGISTER_DATA_FUNC(create_u32_slotmap);
    REGISTER_DATA_FUNC(destroy_u32_slotmap);
    REGISTER_DATA_FUNC(u32_slotmap_contains);
    REGISTER_DATA_FUNC(u32_slotmap_create);
    REGISTER_DATA_FUNC(u32_slotmap_get);
    REGISTER_DATA_FUNC(u32_slotmap_set);
    REGISTER_DATA_FUNC(u32_slotmap_erase);
    REGISTER_DATA_FUNC(u32_slotmap_size);
    REGISTER_DATA_FUNC(create_memory_slotmap);
    REGISTER_DATA_FUNC(destroy_memory_slotmap);
    REGISTER_DATA_FUNC(memory_slotmap_contains);
    REGISTER_DATA_FUNC(memory_slotmap_create);
    REGISTER_DATA_FUNC(memory_slotmap_get);
    REGISTER_DATA_FUNC(memory_slotmap_erase);
    REGISTER_DATA_FUNC(memory_slotmap_size);
#undef REGISTER_DATA_FUNC
}
