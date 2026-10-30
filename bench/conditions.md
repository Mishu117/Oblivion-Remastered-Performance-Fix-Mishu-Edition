# Test conditions

Record any change here before the next benchmark run, and flag runs that straddle a change.

## Current (2026-10-19, Phase 0)

| Item | Value | How checked |
|---|---|---|
| Game build | Steam build 19115871 (same as upstream) | `steamapps/appmanifest_2623190.acf` buildid / TargetBuildID; `tools\GamePaths.ps1` |
| Exe | `OblivionRemastered-Win64-Shipping.exe`, 160,654,648 bytes, TimeDateStamp 0xF19077A4, SizeOfImage 0x9E1E000 | `find_patterns.py` header |
| Exe SHA-256 | `B7BE7E6EBE9424F6FDF274F5E7A59372DE103E043AB5B66C60E021C0C89DF457` | `Get-FileHash`; backup in `backups/2026-10-19-pre-phase0/` (not in git) |
| Engine | UE 5.3 (patch level unverified, likely 5.3.2) | UE4SS log |
| Upstream commit | `6deebc2` (main = origin/main) | `git log` |
| Fix build | OblivionPerfFix built from `work` @ upstream 6deebc2, unmodified source (built with `-OutDir %TEMP%\opfb`, see status.md) | |
| CPU | AMD Ryzen 7 9800X3D, 8C/16T | CIM |
| RAM | 64 GB (2 x 32 GB) DDR5-6000 | CIM |
| GPU | NVIDIA GeForce RTX 5090, 32 GB | nvidia-smi |
| GPU driver | 617.70 (Windows 32.0.16.1770, dated 2026-10-11) | nvidia-smi / CIM |
| Other display adapters | SudoMaker Virtual Display Adapter 1.10.9.289, Sunshine Virtual Display Driver 1.6.3.0 (virtual displays; may affect monitor detection for `-Profile native`) | CIM |
| Display | 5120x2160 @ 165 Hz | CIM (current mode) |
| OS | Windows 11 Pro (registry ProductName still says "Windows 10 Pro") 26H2, build 26300.9711 | registry |
| Game settings (player's own) | 5120x2160 fullscreen, VSync on, all scalability groups 3, AA group 0, no frame cap, HDR off | `GameUserSettings.ini` |
| Engine.ini | only the game's generated `[Core.System] Paths=` list; no CVar tweaks | read 2026-10-19 |
| Benchmark tools | PresentMon 2.6.0, UE4SS 3.0.1-1161-oblvfix1 (both SHA-256 pinned by `Setup-Bench.ps1`) | Setup-Bench |
| Python (tools) | CPython 3.12.15 via uv (`%APPDATA%\uv\python\cpython-3.12.15-windows-x86_64-none\python.exe`) | |
| Background software seen on PATH | Process Lasso, Sunshine/Apollo streaming hosts (check they are idle / not adjusting affinity during runs) | PATH |

## Notes
- **Third-party software in the game process (found 2026-10-20):**
  - **SCSKiller recorder** (BlueHeisenberg, GPL-3.0, https://github.com/BlueHeisenberg/SCSKiller): `d3d12.dll` proxy +
    `scskiller.*` in `Win64`, placed there by the user 2026-10-20 10:04:18 (file date 2026-10-18), `mode=record`. Hooks
    the D3D12 device vtable, DXGI present, NVAPI, Aftermath; records every pipeline/shader create (810k rows by
    13:38). **Loaded in every Phase 2 run (p2*, p2rep*, p2t*, p2t2*); not loaded in Phase 0/1 (2026-10-19) or the
    user's play session 2026-10-19 23:55-00:24.** From 2026-10-20 ~14:20, disabled for benchmark sessions by renaming
    `d3d12.dll` -> `d3d12.dll.scskiller-off` (user's choice), renamed back at the end of each session.
    **Caveat found 14:27:** the SCSKiller app (running since 10:03) re-installs its recorder by itself: a fresh copy of
    `d3d12.dll` appeared at 14:27:46 while the original was renamed. Renaming is not enough while the app runs; for clean
    runs the app must be closed or the recorder uninstalled in its UI.
  - **MacType** (`MacType64.dll`, font renderer, injects system-wide): loaded in the game process 2026-10-20 (per the
    SCSKiller module list); probably also in Phase 0/1. Not removed; note it in shared results.
- `GameUserSettings.ini` and `AltarGameUserSetting.ini` are normally read-only on this PC (player's choice). The flag is
  cleared for benchmark sessions and set again at the end of each session.
- Pipeline cache present before any runs: `Saved\OblivionRemastered_PCD3D_SM6.upipelinecache` (warm cache).

## Change log
| Date | Change | Runs affected |
|---|---|---|
| 2026-10-19 | initial record | - |
| 2026-10-19 18:39-22:12 | none (noise floor + 40 Phase 1 runs under the conditions above) | noise-cpu-*, p1-* |
| 2026-10-19 22:16-23:58 | none (20 variant runs, 3 crash-check launches, 9 visual runs) | p1v-*, vis-* |
| 2026-10-20 10:13-13:38 | **SCSKiller recorder loaded (see next row)**; otherwise unchanged (30 ledger runs, 9 repeat passes, 6 traced runs incl. WPR; Ghidra stopped before measuring) | p2*, p2rep*, p2t*, p2t2* |
| 2026-10-20 10:04 | SCSKiller recorder (d3d12.dll proxy) installed by the user; active in all Phase 2 runs | p2*, p2rep*, p2t*, p2t2* |
| 2026-10-20 14:16-14:27 | SCSKiller proxy renamed away; Ghidra suspended | p3noscs-chey-p1..p3 (clean, one process from 14:17) |
| 2026-10-20 14:27:46 | SCSKiller app re-installed the proxy by itself | p3noscs-icm-p1..p3 NOT clean (process 14:27:43, recorder log written) |
| 2026-10-20 14:49-15:05 | monitor off/standby: 13 start-up crashes (incl. stock); no measurements kept | probe-door (x2), crash-* |
| 2026-10-20 16:14-17:20 | SCSKiller app closed + proxy renamed (clean); Ghidra idle; monitor on | probe-door (16:14, 16:18), p5door-* |
| 2026-10-20 17:21 | SCSKiller proxy renamed back (user setup); app still closed | - |
| 2026-10-20 16:02 | PC rebooted (found later: boot 16:02:24); QPC base changed, so traces/ledgers before and after it do not share a clock | all runs after 16:02 |
| 2026-10-20 21:19 | Phase 4 session: SCSKiller app closed + proxy renamed (clean); settings files writable; Ghidra not running; user pipeline cache backed up (backups/2026-10-20-pre-phase4) | p4w-*, p4val-*, probe-door (21:36, 22:12) |
| 2026-10-20 21:20-21:21 | a single-thread static scan (python) ran ~1 min during p4w-silver-p1's start-up (stopped) | p4w-silver-p1 start-up only |
| 2026-10-20 22:12 | user Engine.ini temporarily given `[ConsoleVariables] r.PSOPrecache.Validation=0` for one probe, restored (same SHA-256) | probe-door 22:12 |
| 2026-10-20 ~23:10 | **NVIDIA Control Panel Shader Cache Size = Disabled (global, set by the user): every launch driver-cold** | p4c-*, p4ct-*, p4cu-* |
| 2026-10-21 00:11-00:24 | WPR CPU profile (sampling + context switches, filemode) recording, started/stopped by the user | p4ct-silver-r1, -r2 (both: flight did not start) |
| 2026-10-21 ~00:55 | NVCP Shader Cache Size back to Driver Default (user); SCSKiller proxy renamed back, settings read-only again | - |
| 2026-10-21 19:58-20:10 | clean session again (SCSKiller app closed + proxy renamed, settings writable, monitor on); driver 617.70 unchanged | p4w2-silver-r1, -r2 |
| 2026-10-21 20:20 | backup backups/2026-10-21-pre-phase5 (exe SHA-256 unchanged since Phase 0, saves, config) | - |
| 2026-10-21 20:25-20:31 | WPR CPU profile recording (user) | p5t-door-r1 |
| 2026-10-21 20:43-22:59 | clean; nvidia-smi sampler every 2 s during p5np2, p5npn, p5np8 (negligible load); Ghidra headless runs only between benchmark runs | p5flush-*, p5np-*, p5np2-*, vis-p5-*, p5npn-*, p5np8-* |
| 2026-10-21 23:16-23:32 | WPR CPU + GPU profiles recording (user); native-nofg door runs | p5tn-F-r1, p5tn-NP-r1 |
| 2026-10-22 00:06 | native door probe (read-only CVar read-back) | probe-door 00:06 |
| 2026-10-22 00:31-00:43 | **diagnostic build installed** (exp/door-diag: upstream fixes unchanged + DoorDiag, enabled per run via -FixIni); original version.dll + ini backed up and restored after (SHA-256 identical) | p5dd-A-r1, p5dd-N-r1, p5dd-E-r1 |
| 2026-10-22 01:02-04:00 | **experiment builds installed** (exp/stream-behind-loading: upstream fixes + DoorDiag + StreamBehindLoadingScreen, enabled per run via -FixIni); original version.dll + ini restored after (SHA-256 identical); nvidia-smi sampler during the A/Bs | p5sbl-*, p5sbln-*, p5sbl4-*, p5sbl4c-*, vis-sbl-*, legs-sbl-* |
| 2026-10-22 09:41-10:20 | experiment build installed (mode-4 DLL B8041E7B3C9A, ini via -FixIni); SCSKiller app found running at 09:41, run stopped and discarded (20261022-094844-p5sblt-F-r1, no result.json), user closed it; then clean (proxy renamed, settings writable, monitor on); nvidia-smi sampler | p5sblt-F-r1..r3, p5sblt-S4-r1..r3 |
| 2026-10-22 10:22-10:26 | final build installed (1.1.0+fork.1, DLL 002018CA90E1, shipped ini); single run parked at the menu, closed with -Close; user's DLL/ini/proxy/read-only flags restored after (SHA-256 identical), saves = backups/2026-10-21-pre-phase5; user pipeline cache grew 315 -> 332 KB (game's own recording, now 61CA78FDD691) | p5sbl-final-smoke |
| 2026-10-22 | **merged build installed for normal play** (1.1.0+fork.1, DLL 002018CA90E1, shipped ini 122EFA60DC4B); previous DLL 288081BE111F + ini C97DBBF41875 in backups/2026-10-22-pre-install-fork1. Benchmark runs from now on start from this installed build (use -FixIni StreamBehindLoadingScreen=0 for the old behaviour) | - |
| 2026-10-22 10:49- | Phase 5 traversal re-measure: clean (SCSKiller app closed, proxy renamed, settings writable, monitor on 5120x2160, driver 617.70 / 32.0.16.1770 unchanged); installed build 1.1.0+fork.1 (002018CA90E1) + HitchLedger via -FixIni | p5tr-* |
| 2026-10-22 23:50 | **prototype installed for the user's check** (exp/fade-hold, DLL 4C72E41D75DC, shipped ini + DrawBehindLoadingScreen=1500); previous DLL 002018CA90E1 + ini 122EFA60DC4B in backups/2026-10-22-pre-fadediag; Altar.ini FadeInDurationUntilUnfreeze=0.01 | - |
| 2026-10-23 00:16 | **user's Engine.ini gets [SystemSettings] streaming lines** (PoolSize 6000, Boost 1.5, FramesForFullUpdate 2, Nanite pool 1024); original BC207AE7E6EB in backups/2026-10-23-pre-streaming. Runs from now on start with these unless swapped | streamcfg-*, later runs |
| 2026-10-23 00:45 | **user's Engine.ini restored** to the original BC207AE7E6EB (streaming lines removed) | unfrz-* on |
| 2026-10-23 01:15 | **UnfreezeBehindLoadingScreen prototype v2 installed for the user's check** (exp/fade-hold 59c80cc, DLL ADEF15B0EC6B, ini F95C26BD2EBC = shipped + UnfreezeBehindLoadingScreen=1000, DrawBehindLoadingScreen=0); user's DLL 002018CA90E1 + ini 122EFA60DC4B still in backups/2026-10-22-pre-fadediag; Altar.ini FadeInDurationUntilUnfreeze=0.01 | - |
| 2026-10-23 01:25 | **merged build installed for normal play** (work fb6662f, 1.1.0+fork.2, DLL A3340C690F96, ini 9A61E719B56B = shipped + UnfreezeBehindLoadingScreen=1000); the pre-fade-work DLL 002018CA90E1 + ini 122EFA60DC4B stay in backups/2026-10-22-pre-fadediag; Altar.ini FadeInDurationUntilUnfreeze=0.01 still in place. Benchmark runs from now on start from this build (UnfreezeBehindLoadingScreen=1000 in the installed ini: pass -FixIni UnfreezeBehindLoadingScreen=0 for the old behaviour) | - |
| 2026-10-23 01:30 | **Altar.ini restored** to the original (one empty line): FadeInDurationUntilUnfreeze back to the game default 0.5 s; the override copy in backups/2026-10-22-unfreeze-config/Altar.ini.with-override | - |
| 2026-10-23 | installed ini: UnfreezeBehindLoadingScreen 1000 -> 500 for the user's play check (rest unchanged) | - |
| 2026-10-23 | user: 500 still settled on exits; installed ini UnfreezeBehindLoadingScreen 500 -> 200 for the next check | - |
| 2026-10-23 | installed ini UnfreezeBehindLoadingScreen 200 -> 300 (user's request; no report on 200) | - |
| 2026-10-23 | installed ini UnfreezeBehindLoadingScreen 300 -> 400 (user's request; no report on 300) | - |
| 2026-10-23 | installed ini UnfreezeBehindLoadingScreen 400 -> 500 (user chose 500 after trying 200/300/400; ini E61C717FE542) | - |
| 2026-10-23 02:51 | **user's Engine.ini gets [SystemSettings] r.Streaming.FullyLoadUsedTextures=1, r.Streaming.UseAllMips=1** (CC82E63C5CC6; original BC207AE7E6EB in backups/2026-10-23-pre-fullmips) for the play test; runs from now on start with these unless swapped | fullmips-*, later runs |
| 2026-10-23 03:53 | user's Engine.ini now BE48AAC3EEC5 = original + [SystemSettings] r.VT.MaxUploadsPerFrame=64 (takes) and r.VT.MaxUploadsPerFrame.Streaming=64 (does not take, stays 4); full-quality texture lines removed; DeviceProfiles.ini back to empty; backups in backups/2026-10-23-pre-vtfast | vtfast-*, vtts-* |
| 2026-10-23 | **user's Engine.ini restored to the original** BC207AE7E6EB (no [SystemSettings]); DeviceProfiles.ini empty; the user will send screenshots of a bad door case | - |
| 2026-10-23 12:36-13:13 | user-door runs from the user quicksave (lighthouse exit); session cvars only (Lumen off x2, VT limits); user config unchanged; quicksave backed up in backups/2026-10-23-user-door | userdoor1-5, userdoor-refloff/gioff, userdoor-vt64a/b, userdoor-vts64a/b |
| 2026-10-23 13:35 | **build 1.1.0+fork.3 installed** (DLL 6B231685FF3C; ini 643F962367C3 = shipped + Unfreeze 500, VtStreamingUploads 0); previous in backups/2026-10-23-pre-vtup | userdoor-vtup64, vtup-silver-road-*, vtup-blackwood-* (-FixIni VtStreamingUploads 0/64) |
| 2026-10-23 15:15 | user's ini VtStreamingUploads=64 for the play check (7105C65BDCD2); read-only flags set again | - |
| 2026-10-23 17:58 | build with diagnostic FadeVtFlushSkip installed (DLL 32BFB3D5C330, branch exp/fade-vt-flush); user ini unchanged (7105C65BDCD2: VT 64, Unfreeze 500, no FadeVtFlushSkip = off); previous DLL in backups/2026-10-23-pre-vtflush | lhf-skip1a/b, vfx-*, vft-* |
| 2026-10-23 | **user ini FadeVtFlushSkip=1** for the play check (207123CC5DC2 = 7105C65BDCD2 + the key and 3 comment lines after VtStreamingUploads); previous ini in backups/2026-10-23-pre-vtflush-ini | - |
| 2026-10-23 | **build 1.1.0+fork.4 installed** (DLL 4B4CA511D4C9; FadeVtFlushSkip documented, shipped ini default 0); user ini 7593383C9136 = shipped + UnfreezeBehindLoadingScreen 500, VtStreamingUploads 64, FadeVtFlushSkip 1 (same keys as before); previous DLL/ini in backups/2026-10-23-pre-fork4 | - |
| 2026-10-23 21:11-23:12 | test builds of exp/pacing-equipment-resolve installed for the runs (DLL 75BBF08DEF1F); user play DLL 4B4CA511D4C9 restored after | p5n-resolve-check, pres-* |
| 2026-10-23 23:30 | **build 1.1.0+fork.5 installed for play** (DLL FAC1C64C330D; PacingResolvePerLevel on by default; ini unchanged 7593383C9136); previous in backups/2026-10-23-pre-fork5 | - |

- 2026-10-25 20:20-21:45: NVIDIA Control Panel Shader Cache Size = Disabled (set by the user) for the option B cold block
  (p7b-cold-*); back to Driver Default after (confirmed by the user). Runs outside that window used the driver cache.
