# Oblivion Remastered Performance Fix – Mishu's Edition

An expanded fork of [LTSharpe's Oblivion Remastered Performance Fix](https://github.com/LTSharpe/OblivionRemasteredPerformanceFix),
building on the original version 1.2.3-beta with additional performance optimizations, graphics improvements, bug
fixes, and customization options.

The original mod addresses dozens of performance issues without compromising the game's intended visuals or gameplay.
This fork builds on that foundation, with a particular focus on loading transitions, texture streaming, HDR, and
high-resolution displays.

## What's new

### Performance & Loading

- **Faster loading transitions:** Begin streaming textures behind loading screens to reduce the time spent waiting for
  the world to appear.
- **Faster texture streaming (optional):** Reduce the time it takes walls, roofs, and other streamed textures to reach
  full detail, from approximately 8 seconds to 2 seconds in testing.
- **Smoother HDR autosaves:** Move save-thumbnail processing off the main thread to eliminate the large autosave hitch
  observed during testing.
- **Shader precompilation after driver updates:** Automatically restore the shader-loading screen when graphics drivers
  change, reducing shader compilation stutter during gameplay.

### Graphics & Display

- **Improved HDR:** Correct the game's raised shadow and midtone levels, with new in-game controls for Peak Brightness,
  Paper White, Black Floor, and Mid-Tone.
- **NVIDIA DLSS Ray Reconstruction:** Enable NVIDIA's AI-powered denoiser for hardware ray-traced lighting and
  reflections.
- **Expanded resolution support:** Access all supported fullscreen resolutions, including alternative aspect ratios on
  ultrawide displays.
- **Ultrawide HUD customization:** Adjust HUD positioning independently of your display's aspect ratio.

### Additional Fixes

- Correct broken bow sneak attack damage.
- Fix inaccurate VRAM reporting and HDR save thumbnails.
- Address additional engine and gameplay issues.

## Built on the original

LTSharpe and the original contributors established the performance optimizations, benchmarking methodology, and
foundational fixes that make this project possible. This fork preserves that work while extending it with additional
improvements and features.

If the original mod improved your experience, consider
[supporting LTSharpe on GitHub Sponsors](https://github.com/sponsors/LTSharpe).

Like the original, the code and docs here were written with Claude (Anthropic's AI) in Claude Code. I set the goals,
tested every build in my own game and made the calls on what ships.

Current version: **1.2.3-beta+fork.27**, for the Steam version of the game (build 19115871).

## Every option in detail

Everything below is on by default unless it says otherwise. Each one has a line in `OblivionPerfFix.ini`, and setting it
to 0 turns it off.

### Loading screens and doors

| Option | What it does |
|---|---|
| `StreamBehindLoadingScreen` | The new area's textures load while the loading screen is still up, so the picture comes back sooner. At 5120x2160, stepping out of the Cheydinhal inn: the last loading image was held 539 ms, now 230 ms (225 to 75 ms stepping in). City gates 420-437 to 166-213 ms, fast travel 327-421 to 113-160 ms. |
| `UnfreezeBehindLoadingScreen`, `UnfreezeBehindLoadingScreenFastTravel` (off) | Lets the world run for a moment behind the loading screen after an exit, a fast travel or a save load, the way the game already does when you go indoors, so your character is standing and the area has settled when the picture comes in. The loading screen extends depending on the scenario, it adds an additional .5 second load time for going through doors and an additional 1 second for fast travel and save loads. |
| `FadeVtFlushSkip` (off) | The game empties its streamed-texture cache 1.5 s into every fade-in, so walls that were already sharp go blurry and sharpen again. This skips that. It changes what you see, so it's off. |
| `VtStreamingUploads` (off) | Lets walls, roofs and stairs reach full detail sooner after a door by allowing more texture uploads per frame (I use 64). |

### Saves

| Option | What it does |
|---|---|
| HDR autosaves (part of `SaveThumbnailAsync`) | With HDR on, every autosave froze the game for one frame of 115-175 ms at 5120x2160, about 2.5 s after you arrived. The original fix only covered SDR screens. The save's picture is now read and converted off the main thread in HDR too, with the game's own conversion, so the thumbnail is the same. Eight door autosaves in my game: no frame of 25 ms or more. |
| `AutosaveFollowsMenu` | Game bug: with all four autosave options in the Gameplay menu off, the game still autosaved after each level-up and when a quest asks for a save. Those now follow the menu too. |
| `HdrSaveThumbnail` (changes the picture) | In HDR, save thumbnails came out gray and washed out. They are now converted to look like the game did. |

### Shaders, drivers and the overlay

| Option | What it does |
|---|---|
| `ShaderScreenAfterDriverChange` | Game bug: the game only shows its "Shaders preloading" screen on the very first launch. After a graphics driver update it compiles everything in the background while you play and stutters for minutes. The fix notices the new driver and shows the screen again, once. |
| `VramCounterFix` | Game bug: "Show VRAM" in the Graphics page always said 0/0. It now shows the game's video memory use and its budget. |
| A start-up crash fix | `PsoPrecompileFast` could crash at start-up in its check of the driver's compile threads (a bug in the original 1.2.2 too). |

### HDR and display

| Option | What it does |
|---|---|
| `HdrMenuRows` | An HDR section in the game's own Options > Graphics page: HDR on/off, Peak Brightness, Paper White, Black Floor and Mid-Tone, saved with your other options. |
| `HdrMenuBackground` (changes the picture) | In HDR the menu's background looked flat and gray. It now shows the world at the brightness it had. |
| `HdrGamma22` (changes the picture) | The game's HDR shows dark areas lifted compared with SDR on a monitor. This makes shadows and midtones as deep as SDR on a gamma 2.2 monitor; highlights, peak brightness and the HUD stay the same. If you've tuned your HDR and like it as it is, set this to 0. |
| `SeparableSkinAbove4K` (changes the picture) | On ultrawides wider than 4K (5120x2160, 5120x1440) at DLSS DLAA or FSR Native AA, skin shows magenta specks. While the game renders more than 3840 pixels across, skin uses the engine's other scattering method, which doesn't have them. At 4K and below nothing changes. |
| `DlssRayReconstruction` | Adds a "DLSS - Ray Reconstruction" row under DLSS Frame Generation. The game has it but never offers it. Ray Reconstruction is NVIDIA's AI denoiser: it reduces the noise of hardware ray-traced lighting and reflections (Lumen), for about 0.4 ms of GPU time on an RTX 5090. It stays Off until you pick On (NVIDIA RTX cards only, with hardware ray tracing on). It needs NVIDIA's Ray Reconstruction file, `nvngx_dlssd.dll`, in the game's `Engine\Plugins\Marketplace\nvidia\DLSS\DLSS\Binaries\ThirdParty\Win64\` folder. The game doesn't include it, and neither does this fork (it's NVIDIA's file, not ours to share): get it from NVIDIA's DLSS SDK, or with a tool like DLSS Swapper. |
| `HudAspectRow` | A "HUD Aspect Ratio" row (Wide, 4:3, 16:9, 21:9, 32:9) that keeps the HUD inside a centered area on wide screens. Menus and full-screen effects stay full width. |
| `MonitorNativeFromWindows` | Game bug: some monitors wider than 4095 pixels report a smaller resolution first (my AW3926QW says 3440x1440), and the game took that as native. The native resolution now comes from Windows. |
| `FullscreenAllResolutions` | The fullscreen resolution list shows every resolution your monitor supports, including other aspect ratios (16:9 on an ultrawide, for example), not only the ones matching your monitor's own aspect ratio. |

### Gameplay

| Option | What it does |
|---|---|
| `BowSneakAttackFix` | Game bug: bow sneak attacks often said "Sneak attack for 1X damage" and did 1x, or hit the target twice. Each arrow now hits once, with your sneak multiplier. This changes damage, back to what the game intends. |

### For tinkerers

- `OldEngineSettings`: set the old engine's Oblivion.ini settings from `OblivionPerfFix.ini` (for example NPC-to-NPC
  subtitles), without editing the game's files.
- Diagnostics, all off: `DoorDiag` (a timeline of every loading screen), `PixGpuCapturer` (lets Microsoft PIX attach
  for GPU captures), and the original `HitchLedger`.

## How it compares

On the long benchmark flights (the Imperial City flight and the Silver Road, 5 runs each, 5120x2160, RTX 5090), the fork
and the original measure the same, and both are far smoother than the stock game. On the Imperial City flight, for
example: stock 1% low 34 fps and 10 frames of 50 ms or more per run; original 1.2.2 and the fork both about 51-53 fps
and none. Full tables: [bench/baseline/final.md](bench/baseline/final.md), and this PC's run-to-run noise:
[bench/baseline/noise.md](bench/baseline/noise.md).

The fork's own changes show up at specific moments rather than on average: arriving through a door, an autosave in
HDR, the first hour after a driver update, opening a menu in HDR.

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

- Tested on one PC: Ryzen 7 9800X3D, RTX 5090, Windows 11, Steam, 5120x2160 at 165 Hz, HDR10.
- For high resolutions like 5120x2160, and for the texture streaming that lets the world appear without textures fading
  in, a graphics card with 8 GB of video memory or more is recommended. That is advice, not a measured limit: the fork
  was only tested on a 32 GB card. The original's fixes
  have been tested much more widely; see [its README](https://github.com/LTSharpe/OblivionRemasteredPerformanceFix#compatibility).
- Use the `version.dll` package if you can. The OBSE64 package (`OblivionPerfFix-<version>-OBSE.zip`) is for setups
  where another mod already uses `version.dll`. The original's fixes work there as in his release, with the same ones
  off ([his list](https://github.com/LTSharpe/OblivionRemasteredPerformanceFix#obse64-route-what-stays-off)), but the
  fork's additions were never tested with OBSE64. The log says which fixes are on.
- Game Pass is untested.
- A game update can switch fixes off: each one checks the code it changes and logs `not found` instead of guessing.

## Problems and questions

For anything about the fork's additions, please open an issue
[here](https://github.com/Mishu117/OblivionRemasteredPerformanceFix/issues), not on the original's tracker, and attach
`OblivionPerfFix.log`. If it happens with the original fix too, it belongs there.

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
