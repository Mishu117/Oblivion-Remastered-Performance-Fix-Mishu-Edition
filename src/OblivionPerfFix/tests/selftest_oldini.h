// OldEngineSettings (fork-local) self-test, included by selftest.cpp after CellBufferPacedEvict's test (which detours
// the same ini writer in the mapped copy, so the chained install is what runs here): the {value, name*} pairs of
// static settings with the shipped Oblivion.ini's values, a name that does not exist, the writer found and detoured.
#pragma once

namespace oini {

inline void Run(const opf::GameModule& game)
{
    using namespace opf;
    uint64_t* const grids = FindOldEngineSetting("uGridsToLoad:General");
    Check(grids != nullptr, "old engine settings: uGridsToLoad:General's pair found");
    if (grids) {
        Check(static_cast<uint32_t>(grids[0]) == 5, "old engine settings: uGridsToLoad holds 5 (static default)");
        Check(strcmp(reinterpret_cast<const char*>(grids[1]), "uGridsToLoad:General") == 0, "old engine settings: the pair's name pointer");
        Check(reinterpret_cast<uint8_t*>(grids) == game.base + 0x8FAED80, "old engine settings: uGridsToLoad pair at exe+0x8FAED80");
    }
    uint64_t* const bg = FindOldEngineSetting("bBackgroundCellLoads:BackgroundLoad");
    Check(bg && static_cast<uint8_t>(bg[0]) == 1, "old engine settings: bBackgroundCellLoads:BackgroundLoad holds 1");
    uint64_t* const pre = FindOldEngineSetting("iPreloadSizeLimit:General");
    Check(pre && static_cast<uint32_t>(pre[0]) == 26214400u, "old engine settings: iPreloadSizeLimit:General holds 26214400");
    Check(FindOldEngineSetting("bNoSuchSetting:General") == nullptr, "old engine settings: an unknown name finds nothing");
    Check(FindOldEngineSetting("GridsToLoad:General") == nullptr, "old engine settings: a name's tail is not a match");
    if (!Installed(FindOldEngineSettings(), "OldEngineSettings")) return;
    uint8_t* const w = CellBufferIniWriter();
    Check(w != nullptr, "old engine settings: the ini writer's entry");
    Check(InstallOldEngineSettings(), "old engine settings: installed (ini writer detoured)");
    if (w) {
        Check(w[0] == 0xE9, "old engine settings: the writer starts with a jump");
        int32_t rel;
        memcpy(&rel, w + 1, 4);
        Check(!game.text.Contains(w + 5 + rel), "old engine settings: the jump goes to a stub outside the game's code");
    }
}

}  // namespace oini
