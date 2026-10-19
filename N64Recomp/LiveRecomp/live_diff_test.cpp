// Runs single-instruction functions through LiveRecomp with seeded random registers and memory and prints one hash per case.
// Build it for a 64-bit and a 32-bit host and diff the outputs: the 32-bit pair lowering must match the 64-bit JIT.
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>
#include "recompiler/live_recompiler.h"
#include "recomp.h"

namespace {

constexpr uint32_t text_vram = 0x80000400;
constexpr uint32_t data_vram = 0x80100000;
constexpr uint32_t data_size = 0x2000;
constexpr int A = 8, B = 9, C = 10, D = 11;

uint64_t rng_state = 0x9E3779B97F4A7C15ull;
uint64_t rng() {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return rng_state;
}

uint32_t swap32(uint32_t v) {
    return (v >> 24) | ((v >> 8) & 0xFF00) | ((v << 8) & 0xFF0000) | (v << 24);
}

uint32_t r_type(uint32_t funct, int rs, int rt, int rd, int sa = 0) {
    return (uint32_t(rs) << 21) | (uint32_t(rt) << 16) | (uint32_t(rd) << 11) | (uint32_t(sa) << 6) | funct;
}

uint32_t i_type(uint32_t op, int rs, int rt, uint16_t imm) {
    return (op << 26) | (uint32_t(rs) << 21) | (uint32_t(rt) << 16) | imm;
}

uint32_t cop1(uint32_t fmt, int ft, int fs, int fd, uint32_t funct) {
    return (0x11u << 26) | (fmt << 21) | (uint32_t(ft) << 16) | (uint32_t(fs) << 11) | (uint32_t(fd) << 6) | funct;
}

constexpr uint32_t jr_ra = 0x03E00008;
constexpr uint32_t nop = 0;

enum class BaseKind { None, Aligned, Unaligned };

struct Case {
    std::string name;
    std::vector<uint32_t> words;
    BaseKind base;
};

std::vector<Case> make_cases() {
    std::vector<Case> cases;
    auto add = [&](std::string name, std::vector<uint32_t> body, BaseKind base = BaseKind::None) {
        body.push_back(jr_ra);
        body.push_back(nop);
        cases.push_back({ std::move(name), std::move(body), base });
    };
    struct RFunct { const char* name; uint32_t funct; };
    for (RFunct f : std::initializer_list<RFunct>{ {"addu", 0x21}, {"subu", 0x23}, {"daddu", 0x2D}, {"dsubu", 0x2F}, {"and", 0x24}, {"or", 0x25},
        {"xor", 0x26}, {"nor", 0x27}, {"slt", 0x2A}, {"sltu", 0x2B}, {"sllv", 0x04}, {"srlv", 0x06}, {"srav", 0x07}, {"dsllv", 0x14},
        {"dsrlv", 0x16}, {"dsrav", 0x17} }) {
        add(f.name, { r_type(f.funct, A, B, C) });
        add(std::string(f.name) + "_same", { r_type(f.funct, A, A, A) });
    }
    struct Shift { const char* name; uint32_t funct; };
    for (Shift s : std::initializer_list<Shift>{ {"sll", 0x00}, {"srl", 0x02}, {"sra", 0x03}, {"dsll", 0x38}, {"dsrl", 0x3A}, {"dsra", 0x3B},
        {"dsll32", 0x3C}, {"dsrl32", 0x3E}, {"dsra32", 0x3F} }) {
        for (int sa : { 0, 1, 7, 16, 31 }) {
            add(std::string(s.name) + "_" + std::to_string(sa), { r_type(s.funct, 0, B, C, sa) });
        }
    }
    struct MulDiv { const char* name; uint32_t funct; };
    for (MulDiv m : std::initializer_list<MulDiv>{ {"mult", 0x18}, {"multu", 0x19}, {"div", 0x1A}, {"divu", 0x1B}, {"dmult", 0x1C},
        {"dmultu", 0x1D}, {"ddiv", 0x1E}, {"ddivu", 0x1F} }) {
        add(m.name, { r_type(m.funct, A, B, 0), r_type(0x10, 0, 0, C), r_type(0x12, 0, 0, D) });
        add(std::string(m.name) + "_zero", { r_type(m.funct, A, 0, 0), r_type(0x10, 0, 0, C), r_type(0x12, 0, 0, D) });
    }
    add("mthi_mtlo", { r_type(0x11, A, 0, 0), r_type(0x13, B, 0, 0), r_type(0x10, 0, 0, C), r_type(0x12, 0, 0, D) });
    struct Imm { const char* name; uint32_t op; };
    for (Imm i : std::initializer_list<Imm>{ {"addiu", 0x09}, {"daddiu", 0x19}, {"slti", 0x0A}, {"sltiu", 0x0B}, {"andi", 0x0C}, {"ori", 0x0D}, {"xori", 0x0E} }) {
        for (uint16_t imm : { uint16_t(0), uint16_t(1), uint16_t(0x7FFF), uint16_t(0x8000), uint16_t(0xFFFF), uint16_t(0x1234) }) {
            add(std::string(i.name) + "_" + std::to_string(imm), { i_type(i.op, A, C, imm) });
        }
    }
    add("lui", { i_type(0x0F, 0, C, 0x8123), i_type(0x0F, 0, D, 0x7123) });
    struct Mem { const char* name; uint32_t op; BaseKind base; };
    for (Mem m : std::initializer_list<Mem>{ {"lb", 0x20, BaseKind::Unaligned}, {"lbu", 0x24, BaseKind::Unaligned}, {"lh", 0x21, BaseKind::Aligned},
        {"lhu", 0x25, BaseKind::Aligned}, {"lw", 0x23, BaseKind::Aligned}, {"lwu", 0x27, BaseKind::Aligned}, {"ld", 0x37, BaseKind::Aligned},
        {"lwl", 0x22, BaseKind::Unaligned}, {"lwr", 0x26, BaseKind::Unaligned}, {"ldl", 0x1A, BaseKind::Unaligned}, {"ldr", 0x1B, BaseKind::Unaligned},
        {"sb", 0x28, BaseKind::Unaligned}, {"sh", 0x29, BaseKind::Aligned}, {"sw", 0x2B, BaseKind::Aligned}, {"sd", 0x3F, BaseKind::Aligned},
        {"swl", 0x2A, BaseKind::Unaligned}, {"swr", 0x2E, BaseKind::Unaligned}, {"sdl", 0x2C, BaseKind::Unaligned}, {"sdr", 0x2D, BaseKind::Unaligned} }) {
        for (uint16_t imm : { uint16_t(0), uint16_t(8), uint16_t(0xFFF8) }) {
            add(std::string(m.name) + "_" + std::to_string(imm), { i_type(m.op, A, C, imm) }, m.base);
        }
    }
    add("lwc1_swc1", { i_type(0x31, A, 2, 0), i_type(0x39, A, 4, 16) }, BaseKind::Aligned);
    add("ldc1_sdc1", { i_type(0x35, A, 2, 8), i_type(0x3D, A, 4, 24) }, BaseKind::Aligned);
    add("mtc1_mfc1", { cop1(4, A, 2, 0, 0), cop1(0, C, 4, 0, 0) });
    add("dmtc1_dmfc1", { cop1(5, A, 2, 0, 0), cop1(1, C, 4, 0, 0) });
    add("mov_s_d", { cop1(0x10, 0, 4, 2, 0x06), cop1(0x11, 0, 6, 8, 0x06) });
    add("cvt_d_l", { cop1(0x15, 0, 4, 2, 0x21) });
    add("cvt_s_l", { cop1(0x15, 0, 4, 2, 0x20) });
    struct ToL { const char* name; uint32_t funct; };
    for (ToL t : std::initializer_list<ToL>{ {"cvt_l", 0x25}, {"cvt_w", 0x24}, {"trunc_w", 0x0D}, {"add", 0x00}, {"sqrt", 0x04} }) {
        add(std::string(t.name) + "_s", { cop1(0x10, 6, 4, 2, t.funct) });
        add(std::string(t.name) + "_d", { cop1(0x11, 6, 4, 2, t.funct) });
    }
    add("c_lt_s_bc1t", { cop1(0x10, 6, 4, 0, 0x3C), (0x11u << 26) | (8u << 21) | (1u << 16) | 2u, i_type(0x0D, 0, C, 1), i_type(0x0D, 0, D, 1) });
    // Branches skip the next instruction when taken; C is set in the delay slot, D only when not taken.
    struct Branch { const char* name; uint32_t word; };
    for (Branch b : std::initializer_list<Branch>{ {"beq", i_type(0x04, A, B, 2)}, {"bne", i_type(0x05, A, B, 2)}, {"blez", i_type(0x06, A, 0, 2)},
        {"bgtz", i_type(0x07, A, 0, 2)}, {"bltz", i_type(0x01, A, 0, 2)}, {"bgez", i_type(0x01, A, 1, 2)}, {"beq_same", i_type(0x04, A, A, 2)} }) {
        add(b.name, { b.word, i_type(0x0D, 0, C, 1), i_type(0x0D, 0, D, 1) });
    }
    return cases;
}

uint64_t random_gpr(int trial) {
    // Mix canonical (sign extended) values, small values, and arbitrary 64-bit patterns.
    switch (trial % 4) {
        case 0: return uint64_t(int64_t(int32_t(uint32_t(rng()))));
        case 1: return uint64_t(int64_t(int8_t(rng())));
        case 2: return rng();
        default: return (rng() & 1) ? 0x8000000000000000ull : uint64_t(int64_t(-1)) * (rng() & 1);
    }
}

uint64_t fnv(uint64_t h, const void* data, size_t size) {
    const uint8_t* p = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < size; i++) {
        h = (h ^ p[i]) * 0x100000001B3ull;
    }
    return h;
}

void dummy_switch_error(const char*, uint32_t, uint32_t) {}
recomp_func_t* dummy_get_function(int32_t) { return nullptr; }

}

int main() {
    // Unbuffered so an sljit assert's message survives the abort; no CRT abort dialog.
    setvbuf(stdout, nullptr, _IONBF, 0);
#ifdef _WIN32
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
    std::vector<Case> cases = make_cases();
    N64Recomp::Context context{};
    context.sections.resize(1);
    context.sections[0].ram_addr = text_vram;
    context.sections[0].rom_addr = 0;
    context.sections[0].name = ".text";
    context.sections[0].executable = true;
    context.section_functions.resize(1);
    uint32_t vram = text_vram;
    for (const Case& c : cases) {
        std::vector<uint32_t> words;
        for (uint32_t w : c.words) {
            words.push_back(swap32(w));
        }
        context.functions_by_vram[vram].push_back(context.functions.size());
        context.section_functions[0].push_back(context.functions.size());
        context.sections[0].function_addrs.push_back(vram);
        context.functions.emplace_back(vram, vram - text_vram, std::move(words), "f_" + c.name, 0);
        vram += uint32_t(c.words.size() * 4);
    }
    context.sections[0].size = vram - text_vram;

    int32_t section_addresses[] = { int32_t(text_vram) };
    N64Recomp::LiveGeneratorInputs inputs{
        .switch_error = dummy_switch_error,
        .get_function = dummy_get_function,
        .reference_section_addresses = nullptr,
        .local_section_addresses = section_addresses,
    };
    N64Recomp::live_recompiler_init();
    N64Recomp::LiveGenerator generator{ context.functions.size(), inputs };
    std::vector<std::vector<uint32_t>> static_funcs{};
    for (size_t i = 0; i < context.functions.size(); i++) {
        std::ostringstream unused{};
        if (!N64Recomp::recompile_function_live(generator, context, i, unused, static_funcs, true)) {
            printf("recompile failed: %s\n", cases[i].name.c_str());
            return 1;
        }
    }
    N64Recomp::LiveGeneratorOutput output = generator.finish();
    if (!output.good) {
        printf("generation failed\n");
        return 1;
    }

    std::vector<uint8_t> rdram(16 * 1024 * 1024);
    for (size_t i = 0; i < cases.size(); i++) {
        for (int trial = 0; trial < 64; trial++) {
            recomp_context ctx{};
            uint64_t* gprs = &ctx.r0;
            for (int r = 1; r < 32; r++) {
                gprs[r] = random_gpr(trial + r);
            }
            for (int f = 0; f < 32; f++) {
                uint64_t bits = rng();
                if (trial % 3 == 0) {
                    double d = double(int64_t(rng() >> (rng() % 40))) / 7.0;
                    memcpy(&bits, &d, 8);
                }
                else if (trial % 3 == 1) {
                    float lo = float(int32_t(rng() % 2000000) - 1000000) / 3.0f;
                    uint32_t lo_bits;
                    memcpy(&lo_bits, &lo, 4);
                    bits = (bits & 0xFFFFFFFF00000000ull) | lo_bits;
                }
                memcpy(&(&ctx.f0)[f], &bits, 8);
            }
            uint32_t base = data_vram + 0x800 + uint32_t(rng() % 0x800);
            if (cases[i].base == BaseKind::Aligned) {
                base &= ~7u;
            }
            if (cases[i].base != BaseKind::None) {
                ctx.r8 = uint64_t(int64_t(int32_t(base)));
            }
            for (uint32_t off = 0; off < data_size; off++) {
                rdram[data_vram - 0x80000000 + off] = uint8_t(rng());
            }
            output.functions[i](rdram.data(), &ctx);
            uint64_t h = 0xCBF29CE484222325ull;
            h = fnv(h, gprs, 32 * sizeof(uint64_t));
            h = fnv(h, &ctx.f0, 32 * sizeof(fpr));
            h = fnv(h, &rdram[data_vram - 0x80000000], data_size);
            printf("%s %d %016llx\n", cases[i].name.c_str(), trial, (unsigned long long)h);
        }
    }
    return 0;
}
