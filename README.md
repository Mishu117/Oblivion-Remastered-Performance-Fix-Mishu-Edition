# Oblivion Remastered Performance Fix – Mishu's Edition

A fork of [LTSharpe's Oblivion Remastered Performance Fix](https://github.com/LTSharpe/OblivionRemasteredPerformanceFix).

Since version 1.2.4-beta, the original includes almost everything this fork added: faster loading transitions, smoother
HDR autosaves, the shader screen after a driver update, the HDR menu rows and HDR picture fixes, NVIDIA DLSS Ray
Reconstruction, the ultrawide HUD option, the wide-monitor resolution fixes and the bow sneak attack fix (pull requests
#33 to #36). Mishu's Edition is now the original 1.2.4-beta with two differences:

- **My choice of defaults.** Eight options the original ships off are on here (below). Everything else is the same.
- **`OldEngineSettings`**, which the original does not have: set the old engine's own Oblivion.ini settings from
  `OblivionPerfFix.ini`, without editing the game's files.

If you prefer the original's defaults, use [the original](https://github.com/LTSharpe/OblivionRemasteredPerformanceFix),
or set the options below to 0 in `OblivionPerfFix.ini`.

Current version: **1.2.4-beta+fork.28**, for the Steam version of the game (build 19115871).

## On by default here

| Option | What it does |
|---|---|
| `HdrMenuRows` | An HDR section in the game's own Options > Graphics page: HDR on/off, Peak Brightness, Paper White, Black Floor and Mid-Tone, saved with your other options. |
| `HdrGamma22` (changes the picture) | The game's HDR shows dark areas lifted compared with SDR on a monitor. This makes shadows and midtones as deep as SDR on a gamma 2.2 monitor; highlights, peak brightness and the HUD stay the same. If you've tuned your HDR and like it as it is, set this to 0. |
| `HdrMenuBackground` (changes the picture) | In HDR the menu's background looked flat and gray. It now shows the world at the brightness it had. |
| `HdrSaveThumbnail` (changes the picture) | In HDR, save thumbnails came out gray and washed out. They are now converted to look like the game did. |
| `DlssRayReconstruction` | Adds a "DLSS - Ray Reconstruction" row under DLSS Frame Generation. The game has it but never offers it. Ray Reconstruction is NVIDIA's AI denoiser: it reduces the noise of hardware ray-traced lighting and reflections (Lumen), for about 0.4 ms of GPU time on an RTX 5090. It stays Off until you pick On (NVIDIA RTX cards only, with hardware ray tracing on). It needs NVIDIA's Ray Reconstruction file, `nvngx_dlssd.dll`, in the game's `Engine\Plugins\Marketplace\nvidia\DLSS\DLSS\Binaries\ThirdParty\Win64\` folder. The game doesn't include it, and neither does this fork (it's NVIDIA's file, not ours to share): get it from NVIDIA's DLSS SDK, or with a tool like DLSS Swapper. |
| `HudAspectRow` | A "HUD Aspect Ratio" row (Wide, 4:3, 16:9, 21:9, 32:9) that keeps the HUD inside a centered area on wide screens. Menus and full-screen effects stay full width. |
| `AutosaveFollowsMenu` | Game bug: with all four autosave options in the Gameplay menu off, the game still autosaved after each level-up and when a quest asks for a save. Those now follow the menu too. |
| `FullscreenAllResolutions` | The fullscreen resolution list shows every resolution your monitor supports, including other aspect ratios (16:9 on an ultrawide, for example), not only the ones matching your monitor's own aspect ratio. |

Each one is explained in plain words in [docs/how-it-works.md](docs/how-it-works.md) and in its line of `OblivionPerfFix.ini`.

## OldEngineSettings

The old engine still reads its own Oblivion.ini with the 2006 settings, and the game writes that file back every time
it quits. List a setting in the `[OldEngine]` section of `OblivionPerfFix.ini` as `name:Section=value`, and it is set in
memory once the old engine's world exists; Oblivion.ini itself stays as it was. Nothing is set unless you list it. For
example, NPC-to-NPC chatter subtitles, which the remaster's subtitle option does not switch on:

```ini
[OldEngine]
bGeneralSubtitles:GamePlay=1
```

## Built on the original

LTSharpe and the original contributors established the performance optimizations, benchmarking methodology, and
foundational fixes that make this project possible, and reviewed and took in this fork's work.

If the original mod improved your experience, consider
[supporting LTSharpe on GitHub Sponsors](https://github.com/sponsors/LTSharpe).

Like the original, the code and docs here were written with Claude (Anthropic's AI) in Claude Code. I set the goals,
tested every build in my own game and made the calls on what ships.

## Install

1. Close the game.
2. Copy `version.dll` and `OblivionPerfFix.ini` into
   `...\steamapps\common\Oblivion Remastered\OblivionRemastered\Binaries\Win64\` (next to
   `OblivionRemastered-Win64-Shipping.exe`).
3. Start the game. The fix writes `OblivionPerfFix.log` next to itself; its first line names the version.

Download `OblivionPerfFix-<version>.zip` from the [Releases](https://github.com/Mishu117/OblivionRemasteredPerformanceFix/releases)
page and extract it into the game's folder (it has the game's folder layout), or copy the two files by hand as above.
With OBSE64, or another mod that already uses `version.dll`, use `OblivionPerfFix-<version>-OBSE.zip` instead (see
Compatibility).

**Use it on its own.** If you already use the original fix, this replaces it: use one or the other, not both. Also
don't combine it with other performance or optimization mods (Engine.ini tweak packs, other stutter or performance DLL
fixes): they change the same engine settings and code, and can interfere with this mod.

**Uninstall:** delete `version.dll` and `OblivionPerfFix.ini`. The fix never changes the game's files.

## Compatibility

- Everything but `OldEngineSettings` is the original's code: see [its README](https://github.com/LTSharpe/OblivionRemasteredPerformanceFix#compatibility)
  for what has been tested where.
- This edition's defaults and `OldEngineSettings` were tested on one PC: Ryzen 7 9800X3D, RTX 5090, Windows 11, Steam,
  5120x2160 at 165 Hz, HDR10.
- Use the `version.dll` package if you can. The OBSE64 package (`OblivionPerfFix-<version>-OBSE.zip`) is for setups
  where another mod already uses `version.dll`; the same fixes stay off there as in the original
  ([its list](https://github.com/LTSharpe/OblivionRemasteredPerformanceFix#obse64-route-what-stays-off)). The log says
  which fixes are on.
- Game Pass is untested.
- A game update can switch fixes off: each one checks the code it changes and logs `not found` instead of guessing.

## Problems and questions

For this edition's defaults or `OldEngineSettings`, please open an issue
[here](https://github.com/Mishu117/OblivionRemasteredPerformanceFix/issues) and attach `OblivionPerfFix.log`. Everything
else is the original's code: if it happens with the original fix too, it belongs on
[its tracker](https://github.com/LTSharpe/OblivionRemasteredPerformanceFix/issues).

## Build from source

Windows, Visual Studio 2022 or later with the C++ x64 tools (the free Build Tools are enough), PowerShell 5.1+, Python 3.

```powershell
src\OblivionPerfFix\build.ps1 -SelfTest
```

The DLL lands in `src\OblivionPerfFix\build\`. The self-test checks every fix against your installed game.

How each fix works, in plain words: [docs/how-it-works.md](docs/how-it-works.md). The evidence and the ideas that were
tried and dropped: [docs/research.md](docs/research.md). What changed in each version: [CHANGELOG.md](CHANGELOG.md).
The benchmark: [docs/benchmark.md](docs/benchmark.md).

## License

MIT, as the original ([LICENSE](LICENSE)). Copyright LTSharpe and contributors, and Mishu117 for the fork's changes.
