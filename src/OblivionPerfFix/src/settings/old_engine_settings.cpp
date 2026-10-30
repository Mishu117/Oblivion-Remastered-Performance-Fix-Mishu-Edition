// OldEngineSettings (fork-local, 2026-10-26; nothing changes unless OblivionPerfFix.ini's [OldEngine] section lists a
// setting): the old engine's own ini settings (Oblivion.ini in the game's Content\Dev\ObvData, e.g.
// bGeneralSubtitles:GamePlay, iPostProcessMilliseconds:BackgroundLoad) set in memory, without editing the game's file.
//
// The old engine keeps each setting as a {value (8 bytes), name pointer} pair (build 19115871: uGridsToLoad:General at
// exe+0x8FAED80 holds 5, bBackgroundCellLoads:BackgroundLoad at exe+0x8FB8438 holds 1, iPreloadSizeLimit:General
// 26214400, the values of the shipped Oblivion.ini); its ini collection fills them from Oblivion.ini at start-up, and
// the code reads them where it uses them. Some pairs are static data, some are built by the exe's static initialisers.
// This finds a listed setting by its full name ("name:Section", the string the pair points to, which must exist once
// in the exe) and the one pair in .data that points to it, waits until the old engine's world object (TES) exists (so
// the game has read its ini), keeps the value it found (the player's Oblivion.ini value) and writes the listed one.
// Types by the name's prefix, as the old engine's: b (bool, one byte, 0/1), i (int32), u (uint32), f (float); other
// types are refused. Re-read every second: a changed value is written, a removed key puts back the kept value.
//
// The file stays as it was: the game writes Oblivion.ini back as it quits (INISettingCollection::WriteSetting, which
// reads the value at [setting] and the name at [setting+8]); that function is detoured and given a copy of a listed
// pair holding the kept value, so the file gets the player's value. CellBufferPacedEvict (npcs/cell_buffer.cpp)
// detours the same entry for its own setting; this is installed after it and chains to it.
//
// What changes is whatever the setting changes, and only where the game reads the setting after it is written (a
// setting read once at start-up, before the world exists, keeps the file's value). This is a tool for a player's
// choices and for measured experiments, not a fix that is identical by construction: every use is a deliberate change
// (a trade-off when it is visible or affects gameplay), documented where it is used.
#include "common.h"

#include <atomic>
#include <cstring>
#include <string>
#include <vector>

namespace opf {
namespace {

// INISettingCollection::WriteSetting(collection, setting) (exe+0x66E0340, as npcs/cell_buffer.cpp): push rbp; push rbx;
// push rdi; push r14 (6 bytes, position-independent); lea rbp,[rsp-0C8h]; sub rsp,1C8h; ...; cmp qword [rdx+8],0; ...
constexpr char kWriteSetting[] =
    "40 55 53 57 41 56 48 8D AC 24 38 FF FF FF 48 81 EC C8 01 00 00 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 85 B0 00 00 00 "
    "32 DB 4C 8B F2 48 83 7A 08 00 48 8B F9 0F 84";
constexpr size_t kPrologue = 6;
constexpr size_t kMaxEntries = 64;

using WriteFn = uint64_t (*)(uint64_t, uint64_t, uint64_t, uint64_t);

struct Entry {
    char name[96] = {};
    char type = 0;               // 'b', 'i', 'u', 'f'
    uint64_t* pair = nullptr;    // {value, name*}
    uint32_t kept = 0;           // the value found before the first write (the player's Oblivion.ini value)
    uint32_t written = 0;        // the value last written
    bool applied = false;
    bool listed = false;         // in the ini's section on the last read
    bool failed = false;         // not found / unsupported (logged once)
};

struct State {
    bool found = false, installed = false;
    uint8_t* writeSetting = nullptr;
    bool chained = false;        // the entry was already detoured (CellBufferPacedEvict)
    WriteFn next = nullptr;
    Entry entries[kMaxEntries];
    std::atomic<int> count{0};
    std::atomic<long> iniWrites{0};
} s;

bool Fail(const char* why)
{
    Log("[oldini] %s: OldEngineSettings not installed", why);
    return false;
}

uint32_t ReadValue(const Entry& e)
{
    if (e.type == 'b') return *reinterpret_cast<volatile uint8_t*>(e.pair);
    return *reinterpret_cast<volatile uint32_t*>(e.pair);
}

void WriteValue(Entry& e, uint32_t v)
{
    if (e.type == 'b') InterlockedExchange8(reinterpret_cast<volatile char*>(e.pair), static_cast<char>(v ? 1 : 0));
    else InterlockedExchange(reinterpret_cast<volatile LONG*>(e.pair), static_cast<LONG>(v));
}

void Format(const Entry& e, uint32_t v, char* out, size_t n)
{
    if (e.type == 'f') {
        float f;
        memcpy(&f, &v, 4);
        snprintf(out, n, "%g", static_cast<double>(f));
    } else if (e.type == 'i') snprintf(out, n, "%d", static_cast<int32_t>(v));
    else snprintf(out, n, "%u", v);
}

bool Parse(char type, const char* text, uint32_t* out)
{
    char* end = nullptr;
    if (type == 'f') {
        const float f = strtof(text, &end);
        if (end == text) return false;
        memcpy(out, &f, 4);
        return true;
    }
    const long long v = strtoll(text, &end, 0);
    if (end == text) return false;
    if (type == 'b') { if (v != 0 && v != 1) return false; }
    else if (type == 'i') { if (v < INT32_MIN || v > INT32_MAX) return false; }
    else if (v < 0 || v > UINT32_MAX) return false;
    *out = static_cast<uint32_t>(v);
    return true;
}

// The game writes the file: a listed, applied setting is written from a copy holding the kept value.
uint64_t WriteSettingHook(uint64_t coll, uint64_t setting, uint64_t r8, uint64_t r9)
{
    const int n = s.count.load(std::memory_order_acquire);
    for (int i = 0; i < n; ++i) {
        Entry& e = s.entries[i];
        if (!e.applied || reinterpret_cast<uint64_t>(e.pair) != setting) continue;
        uint64_t copy[2] = {e.pair[0], e.pair[1]};
        if (e.type == 'b') copy[0] = (copy[0] & ~0xFFull) | (e.kept & 0xFF);
        else copy[0] = (copy[0] & ~0xFFFFFFFFull) | e.kept;
        s.iniWrites.fetch_add(1, std::memory_order_relaxed);
        Log("[oldini] Oblivion.ini written with %s's own value (not the one set from OblivionPerfFix.ini)", e.name);
        return s.next(coll, reinterpret_cast<uint64_t>(copy), r8, r9);
    }
    return s.next(coll, setting, r8, r9);
}

const uint8_t* FindString(const GameModule& game, const char* name)
{
    const size_t len = strlen(name) + 1;  // with its terminating zero
    const uint8_t* hit = nullptr;
    for (const Section* sec : {&game.rdata, &game.data}) {
        for (const uint8_t* p = sec->begin; p + len <= sec->end; ++p) {
            if (*p != static_cast<uint8_t>(name[0]) || memcmp(p, name, len) != 0) continue;
            if (p > sec->begin && p[-1] != 0) continue;  // a longer name ending in this one
            if (hit) return nullptr;                       // more than once
            hit = p;
        }
    }
    return hit;
}

}  // namespace

// A setting's {value, name*} pair: the one 8-aligned qword in .data that points to the setting's full name.
uint64_t* FindOldEngineSetting(const char* fullName)
{
    const GameModule& game = Game();
    const uint8_t* const str = FindString(game, fullName);
    if (!str) return nullptr;
    const uint64_t want = reinterpret_cast<uint64_t>(str);
    uint64_t* hit = nullptr;
    const uintptr_t b = (reinterpret_cast<uintptr_t>(game.data.begin) + 7) & ~uintptr_t(7);
    for (auto* q = reinterpret_cast<uint64_t*>(b); reinterpret_cast<uint8_t*>(q + 1) <= game.data.end; ++q) {
        if (*q != want) continue;
        if (hit) return nullptr;
        hit = q;
    }
    return hit ? hit - 1 : nullptr;
}

bool FindOldEngineSettings()
{
    if (s.found) return true;
    const GameModule& game = Game();
    const Pattern p = ParsePattern(kWriteSetting);
    auto hits = FindAll(game.text, p);
    uint8_t* ws = nullptr;
    bool chained = false;
    if (hits.size() == 1) ws = hits[0];
    else if (hits.empty() && CellBufferIniWriter() && CellBufferIniWriter()[0] == 0xE9 &&
             MatchesAt(CellBufferIniWriter() + kPrologue, ParsePattern(kWriteSetting + 18))) {
        ws = CellBufferIniWriter();  // already detoured by CellBufferPacedEvict: chain to its stub
        chained = true;
    }
    if (!ws) return Fail(hits.size() > 1 ? "the ini collection's WriteSetting is ambiguous" : "the ini collection's WriteSetting not found");
    if (!CellBufferTesSlot()) return Fail("the old engine's world object (TES) slot not found (CellBufferPacedEvict's search)");
    s.writeSetting = ws;
    s.chained = chained;
    s.found = true;
    Log("[oldini] found: WriteSetting exe+0x%X%s, TES slot exe+0x%X", static_cast<unsigned>(ws - game.base),
        chained ? " (after CellBufferPacedEvict's detour)" : "",
        static_cast<unsigned>(reinterpret_cast<uint8_t*>(CellBufferTesSlot()) - game.base));
    return true;
}

bool InstallOldEngineSettings()
{
    if (s.installed) return true;
    if (!FindOldEngineSettings()) return false;
    uint8_t* const t = AllocNearGame(4096);
    if (!t) return Fail("no free memory near the game's code");
    const uint8_t jmpAbs[6] = {0xFF, 0x25, 0, 0, 0, 0};
    uint8_t* const w = s.writeSetting;
    if (s.chained) {   // the earlier detour's target, then the rest of its chain
        int32_t rel;
        memcpy(&rel, w + 1, 4);
        uint8_t* const prev = w + 5 + rel;
        memcpy(t, jmpAbs, 6);
        memcpy(t + 6, &prev, 8);
    } else {
        uint8_t* const resume = w + kPrologue;
        memcpy(t, w, kPrologue);
        memcpy(t + kPrologue, jmpAbs, 6);
        memcpy(t + kPrologue + 6, &resume, 8);
    }
    void* const hook = reinterpret_cast<void*>(&WriteSettingHook);
    memcpy(t + 64, jmpAbs, 6);
    memcpy(t + 70, &hook, 8);
    DWORD old;
    VirtualProtect(t, 4096, PAGE_EXECUTE_READ, &old);
    FlushInstructionCache(GetCurrentProcess(), t, 4096);
    s.next = reinterpret_cast<WriteFn>(t);
    const int64_t rel = (t + 64) - (w + 5);
    if (rel != static_cast<int32_t>(rel)) return Fail("the stub is out of reach");
    uint8_t code[kPrologue] = {0xE9, 0, 0, 0, 0, 0x90};
    const int32_t r32 = static_cast<int32_t>(rel);
    memcpy(code + 1, &r32, 4);
    if (!WriteCode(w, std::span<const uint8_t>(code, s.chained ? 5 : kPrologue))) return Fail("could not patch WriteSetting");
    s.installed = true;
    Log("[oldini] installed: Oblivion.ini is written with the player's values of the settings set here");
    return true;
}

// Monitor (every second): read [OldEngine], apply once the world object exists.
void TickOldEngineSettings(const wchar_t* iniPath)
{
    if (!s.installed) return;
    wchar_t buf[8192];
    const DWORD n = GetPrivateProfileSectionW(L"OldEngine", buf, static_cast<DWORD>(std::size(buf)), iniPath);
    const int count = s.count.load(std::memory_order_relaxed);
    for (int i = 0; i < count; ++i) s.entries[i].listed = false;
    const bool world = *CellBufferTesSlot() != 0;
    for (const wchar_t* p = buf; p < buf + n && *p; p += wcslen(p) + 1) {
        char line[160];
        if (WideCharToMultiByte(CP_UTF8, 0, p, -1, line, sizeof line, nullptr, nullptr) <= 0) continue;
        char* eq = strchr(line, '=');
        if (!eq || line[0] == ';') continue;
        *eq = 0;
        const char* name = line;
        const char* value = eq + 1;
        while (*value == ' ') ++value;
        int idx = -1;
        for (int i = 0; i < s.count.load(std::memory_order_relaxed); ++i)
            if (strcmp(s.entries[i].name, name) == 0) { idx = i; break; }
        if (idx < 0) {
            const int k = s.count.load(std::memory_order_relaxed);
            if (k >= static_cast<int>(kMaxEntries) || strlen(name) >= sizeof(Entry::name)) continue;
            Entry& e = s.entries[k];
            strcpy_s(e.name, name);
            e.type = name[0];
            if (!strchr("biuf", e.type) || !strchr(name, ':')) {
                Log("[oldini] %s: only b/i/u/f settings named \"name:Section\" can be set; ignored", name);
                e.failed = true;
            } else if (_stricmp(name, "uGridsToLoad:General") == 0 || _stricmp(name, "uExterior Cell Buffer:General") == 0) {
                // set here (after the world exists) the grid's arrays would keep their old size; and even set in time, a
                // bigger grid crashed every launch
                Log("[oldini] %s: sized when the world is made and not safe to change in this game; ignored", name);
                e.failed = true;
            } else if (!(e.pair = FindOldEngineSetting(name))) {
                Log("[oldini] %s: no such setting in the game (its name once, one pair pointing to it); ignored", name);
                e.failed = true;
            }
            s.count.store(k + 1, std::memory_order_release);
            idx = k;
        }
        Entry& e = s.entries[idx];
        e.listed = true;
        if (e.failed || !world) continue;
        uint32_t want;
        if (!Parse(e.type, value, &want)) {
            Log("[oldini] %s=%s: not a valid value for this type; ignored", name, value);
            e.failed = true;
            continue;
        }
        if (!e.applied) {
            e.kept = ReadValue(e);
            e.applied = true;
        } else if (e.written == want && ReadValue(e) == want) continue;  // as set (a changed value is written again)
        WriteValue(e, want);
        e.written = want;
        char a[32], b[32];
        Format(e, e.kept, a, sizeof a);
        Format(e, want, b, sizeof b);
        Log("[oldini] %s = %s (Oblivion.ini's value %s)", e.name, b, a);
    }
    for (int i = 0; i < s.count.load(std::memory_order_relaxed); ++i) {
        Entry& e = s.entries[i];
        if (e.listed || !e.applied) continue;
        WriteValue(e, e.kept);
        e.applied = false;
        e.written = 0;
        char a[32];
        Format(e, e.kept, a, sizeof a);
        Log("[oldini] %s back to Oblivion.ini's value %s (removed from OblivionPerfFix.ini)", e.name, a);
    }
}

bool OldEngineSettingsListed(const wchar_t* iniPath)
{
    wchar_t buf[256];
    return GetPrivateProfileSectionW(L"OldEngine", buf, static_cast<DWORD>(std::size(buf)), iniPath) > 0;
}

}  // namespace opf
