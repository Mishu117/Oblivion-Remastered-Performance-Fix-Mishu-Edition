# Phase 5, open-world traversal: clean re-measure and classification (2026-10-22)

## Conditions and method
- Build 1.1.0+fork.1 as installed (DLL 002018CA90E1, shipped ini; `StreamBehindLoadingScreen` does not act in the open
  world) + `HitchLedger=1`, `HitchLedgerMs=15` via -FixIni. **Clean:** SCSKiller app closed and its `d3d12.dll` proxy
  renamed (Phase 2 had it loaded), monitor on, driver unchanged, no Ghidra. bench/conditions.md 2026-10-22 10:49.
- Profile `native-nofg` (5120x2160, frame generation off, the user's resolution). 5 cells x 5 fresh launches,
  cells interleaved per round (`mytools/Run-Ledger.ps1 -Label p5tr`), 25/25 runs ok, saves identical afterwards.
- Cells: `silver-road` (Roxey Inn -> Bruma, 4.2 min), `blackwood` (forest flight there and back), `blackwood-onfoot`
  (the same route with the game's own movement), `npcburst` (Peryite's Shrine row attach), `roxey-ft` (night forest,
  teleport to Roxey Inn, road flight).
- Tables: per-frame metrics `mytools/hitch_metrics.py` (captures/bench/p5tr-metrics.md), ledger ranking
  `mytools/ledger_rank.py` (captures/bench/p5tr-ledger.md), and new **`mytools/hitch_sources.py`**: each visible hitch
  (PresentMon frame > 2x the rolling median) matched by QPC time to a slow game-thread frame in the ledger; if none,
  PresentMon's own columns say whether the GPU (MsGPUTime >= 60 % of the interval) or the CPU render side
  (MsCPUBusy >= 60 %) was long.
- Note: each run folder also holds the previous session's ledger as `OblivionPerfFix-ledger.1.csv` (Run-Ledger copies
  both). The rankings filter by the run's own scene windows, so they are unaffected; ad-hoc scripts must read
  `OblivionPerfFix-ledger.csv` only.

## Results per scene (medians of 5, range)
| scene | fps | avg ms | 1% low fps | hitches/min | frames > 15 ms | worst ms | GameThread ms | GPU ms |
|---|---|---|---|---|---|---|---|---|
| silver-road-north | 109.0 (93.8-110.7) | 9.2 | 72.1 (64.1-75.5) | 8.9 (7.7-11.8) | 154 (105-325) | 41.6 (29.0-43.1) | 7.3 | 9.1 |
| blackwood-flight | 95.7 (89.0-98.4) | 10.4 | 60.7 (56.5-62.7) | 10.9 (4.7-14.0) | 72 (65-115) | 31.9 (28.8-33.3) | 6.7 | 10.3 |
| blackwood-return | 102.1 (99.6-103.9) | 9.8 | 69.2 (65.2-71.9) | 4.8 (2.9-9.6) | 44 (24-77) | 33.4 (20.1-47.5) | 6.3 | 9.7 |
| blackwood-walk (on foot) | 90.5 (88.1-93.6) | 11.1 | 55.1 (52.4-59.1) | 13.8 (7.9-21.7) | 92 (82-137) | 31.1 (29.5-33.3) | 6.7 | 10.9 |
| peryite-flight | 88.7 (88.3-90.0) | 11.3 | 64.5 (60.0-64.8) | 4.8 (3.6-7.2) | 57 (53-103) | 30.9 (28.8-34.6) | 6.5 | 11.2 |
| roxey-sercen-night | 98.7 (96.8-100.0) | 10.1 | 67.8 (64.3-68.8) | 7.3 (4.8-8.5) | 44 (37-69) | 39.7 (31.8-46.5) | 8.2 | 10.0 |

- **No frame of 50 ms or more in any of the 25 runs** (upstream's measure).
- **GPU-limited at native:** GPU 9-11 ms per frame = the frame time; GameThread 6-8 ms. 165 fps is out of reach here,
  so CLAUDE.md's scaled targets apply: 1% low within ~30 % of the average frame time and worst frame ~2.5x it.
  1% lows are 40-60 % above the average frame time (e.g. Blackwood flight 16.5 vs 10.4 ms) and worst frames 29-42 ms
  against ~23-28 ms: **both missed in every travel scene**.
- vs Phase 2's native runs (3 per cell, SCSKiller loaded, build 1.1.0): fps 3-12 % higher, GPU ms 0.5-2.5 ms lower,
  frames > 15 ms fewer. Two things changed (SCSKiller proxy, build), so this is a lead, not a measured SCSKiller cost.
  Hitches/min are similar or a little higher: a lower median frame time lowers the 2x threshold.

## Where the visible hitches come from (hitch_sources.py, 5 runs per cell)
| scene | hitches/min | slow game-thread frame | GPU | CPU render side | worst not on the game thread |
|---|---|---|---|---|---|
| silver-road-north | 9.3 | 59 % | 26 % | 15 % | 30.5 ms |
| blackwood-flight | 9.6 | 58 % | 39 % | 3 % | 31.2 ms |
| blackwood-return | 5.4 | 61 % | 36 % | 4 % | 25.0 ms |
| blackwood-walk | 14.2 | 53 % | 39 % | 8 % | 31.7 ms |
| peryite-flight | 5.0 | 65 % | 35 % | 0 % | 28.8 ms |
| roxey-sercen-night | 7.0 | 69 % | 17 % | 14 % | 27.6 ms |

**New:** 17-39 % of the visible hitches are frames where the GPU itself took 15-25 ms (normal ~10 ms), with no slow
game-thread frame. The HitchLedger cannot see them (it records game-thread frames only). Mostly scattered; a few spots
repeat (Silver Road +175 s into the scene in 4/5 runs, +117.7 s in 3/5; Blackwood return +58 s in 3/5).

## Game-thread hitches by class (ledger_rank.py; share of hitch excess over 6.06 ms)
| cell | actor spawning | render/RHI side | GC/UI/draw/streaming tail | physics | old-engine wait | other |
|---|---|---|---|---|---|---|
| silver-road | 48 % | 13 % | 18 % | 2 % | 2 % | 17 % (mixed, tick) |
| blackwood | 31 % | 26 % | - | 15 % | 13 % | 16 % |
| blackwood-onfoot | 63 % | 12 % | 25 % | - | - | - |
| npcburst | 100 % | - | - | - | - | - |
| roxey-ft | 26 % | 50 % | 6 % | 6 % | - | 12 % |

### Repeatable spots
| spot (UE x, y) | runs | frame ms | what (ledger + pacing log) |
|---|---|---|---|
| Blackwood 119957, 137390 | 10/10 (flight 5, on foot 5) | 26-33 | 4-7 creation units, 17-20 ms of units in one frame: appearance, clothing, hair of an NPC ~17k units away; `[pacing] ... near/seen units` = on screen, so NpcCreationPacing leaves it (by design) |
| Peryite 212571, 37915 | 5/5 | 25-33 | 2 units 20-25 ms in one frame (appearance + equipment), actor **distant and paced** (lag 3-5 frames) but the two run together: the first is 10-12 ms, over the 4 ms budget, no flag in the frame log -> likely the equipment delegate completing **inside** the appearance unit (nested units run with their parent unless a backlog exists; npc_pacing.cpp `DelegateUnit`) |
| Silver Road 52770, -190220 / 53031, -166838 | 3/5 / 2/5 | 25-35 | 4-5 units 17-27 ms, near/seen |
| Blackwood return 117600, 158800 | 5/5 | 33-38 | game thread waits 26-32 ms for the render thread (`sync`) |
| Roxey road 52-54k, -146.5k | 5/5 | 26-38 | same: render-thread wait 18-28 ms |
| Silver Road 51300, -174500 | 3/5 | 29-31 | same: render-thread wait 20-25 ms |

## Mapped to upstream's "What still limits the game" (docs/research.md)
- Actor spawning: upstream's item 4 (NPC groups with a new cell row; single units unsplittable). Here the near/seen
  bursts are the bigger share; the distant ones are mostly paced.
- Render-thread waits at route spots: matches upstream's item 5 (render thread waiting for ray-tracing instance setup
  as new content streams; 40-50 ms at one route spot on their PC) and Phase 2's shortlist #4 (Blackwood return: a
  Lumen-range parallel task and D3D12 view creation in the trace). Cause not confirmed on this build.
- **GPU-side spikes: not in upstream's list.** New on this PC (GPU-bound at 5120x2160 on an RTX 5090).
- Old-engine cell attach: rare in these runs (5 frames on Silver Road, steady, none a hitch).

## Ranked shortlist
| # | target | expected impact | risk | confidence | strongest argument against | upstream tried? | first experiment |
|---|---|---|---|---|---|---|---|
| 1 | render-thread waits at fixed route spots | 13-50 % of game-thread hitch excess per cell; 26-38 ms frames -> ~10-15 ms if the wait goes | unknown until the cause is known | medium (5/5 at two spots; cause unknown) | may be a GPU fence (render thread waiting for the GPU) or real per-content work (RT instance / BLAS setup) that can only be moved, not removed | no fix; upstream lists it as open | WPR CPU + context-switch trace (user records elevated) of blackwood + roxey-ft at native, 2 runs: what the render thread runs and waits on at those spots |
| 2 | distant NPC units running together (nested completion) | Peryite 25-33 ms -> ~15-20 ms; any route where a distant NPC's appearance completes its equipment at once | low-medium (order of the pairing queue and delegates must stay the game's) | medium (frame log: 2 units, first over budget, no flag; nesting not yet proven) | the nested delegate may rely on its parent's state in the same frame; deferring it is the kind of case upstream fixed once (Silver Road 115 ms) by keeping nested units with the parent | NpcCreationPacing (shipped): queues nested units only when a backlog exists | log-only: record the nesting depth and actor key of each unit in those frames (one run of npcburst), then decide |
| 3 | GPU-side spikes (15-25 ms GPU frames) | 17-39 % of visible hitches | high if it means changing GPU work per frame (VSM / Lumen budgets change the image) | low (no GPU breakdown yet) | probably content-driven GPU work as new geometry streams in (shadow pages, Lumen cards); an identical-image fix may not exist | not in upstream's list | check whether the game build has UE's CSV profiler / GPU stats (exe strings), else an Nsight Systems GPU trace, to see which passes spike |
| 4 | near/seen NPC bursts (Blackwood 4-7 units 17-20 ms, 10/10) | Blackwood 26-33 -> ~15-20 ms | **trade-off** (a visible NPC's clothes/hair a few frames later), unless the NPC is still invisible in its dither fade-in | high for the cause | upstream's rule: on-screen work is not deferred; only behind a default-0 switch with the user's approval | NpcCreationPacing excludes near/seen on purpose | measure the NPC's fade value when its pieces arrive (if ~0, deferring may keep the image identical) |
| - | SCSKiller cost check (not a fix) | user's normal play: fps 3-12 % lower with it in Phase 2 vs clean now | none | low (two things changed) | the build changed too | - | A/B with the proxy in place vs renamed, same build, 5 + 5 on silver-road |

## Trace 1: render-thread waits (2026-10-22 12:37-12:47, captures/traces/p5-travel.etl, 76 GB, not in git)
WPR `-start CPU -start GPU` (user, elevated) around one fresh run each of blackwood and roxey-ft (p5trt-*, native-nofg,
clean). Trace time = QPC ms - 10699880.52 (DxgKrnl VSyncDPC FrameQPCTime vs its timestamp; two events agree to 0.01 ms).
Windows cut with `xperf -a dumper -stacktimeshifting -symbols -range`; mytools/etl_stacks.py, etl_waits.py.

- **Roxey road spot (54323, -146743): 155 ms frame, game thread waiting 143 ms** (untraced runs: 26-38 ms). Chain:
  game thread <- render thread (tid 38540, `RDGBuilder_Execute` in `SceneRenderer_RenderViewFamily_RT`, 148 ms in
  `TaskGraph_WaitUntilTaskCompletes` via fn_26B77C0 <- fn_26A2500 <- fn_26B2D90) <- a worker task (fn_1FBF320, the
  "Lumen-range" task of Phase 2) whose workers were idle, waiting for work <- the work is queued by render-side thread
  38584 (thread function fn_2550030 -> fn_254A3D0 -> fn_25465B0 -> fn_253B610 -> fn_2545450 -> fn_25458E0) once it
  returns from **`IDXGIAdapter3::QueryVideoMemoryInfo`, blocked 150.8 ms in the kernel on the adapter lock**
  (`dxgkrnl!DxgkQueryVideoMemoryInfo` -> `DXGADAPTER::AcquireCoreResourceShared` -> `ExAcquireResourceSharedLite`),
  readied by svchost (graphicsperfsvc), the Windows graphics performance service that WPR's GPU provider drives (its
  `GraphicsPerfMonitorSession` thread started 27 ms before the wait). The whole game was nearly idle during the wait
  (74 CPU samples in 152 ms). **This instance is mostly caused by the GPU recording itself.**
- **Blackwood return spot (117620, 158810): 20 ms frame, game thread waiting 15 ms** (untraced: 33-38 ms). No
  QueryVideoMemoryInfo wait over 1 ms in that window; the render thread (tid 2776) was busy ~17 ms of the frame
  (1 kHz samples), partly in D3D12 texture creation (`D3D12_CreateTextureInternal`); too few samples to say more.
- The fix's own memory monitor also calls QueryVideoMemoryInfo, once a second, with the same shared lock: not a holder.
- **Method caveat:** WPR's GPU provider can hold the adapter lock for >100 ms; waits in GPU-profile traces (also
  p5-door-native.etl, p4c-cold.etl) may be inflated where a QueryVideoMemoryInfo or other adapter-lock call is on the path.
- Not shown yet: whether the untraced 26-38 ms waits are the same lock (held by something else: the video memory
  manager, another process) or other work. Next: CPU-only trace (no GPU provider) of the same routes, every in-scene
  render-wait frame of the ledger cut and followed the same way.

## Trace 2: CPU only (2026-10-22 14:15-14:31, captures/traces/p5-travel-cpu.etl, 52 GB, not in git)
WPR `-start CPU` (no GPU provider), 2 fresh runs each of blackwood and roxey-ft (p5trc-*, clean, native-nofg). Trace
time = QPC ms - 16593376.7 (trace 1's VSync-derived offset + the difference of the two traces' start times, same boot).
Chains followed with new **mytools/etl_chain.py** (from the render thread: the game thread waits for it in ~1 ms slices
in fn_26BDD70, never one long wait).

| frame (ledger) | chain | root |
|---|---|---|
| Blackwood return 32.8 ms, sync 28.2 | render thread 31.8 ms in `TaskGraph_WaitUntilTaskCompletes` (fn_2315DC0 <- fn_22FCAA0 <- `DeferredShadingRenderer_Render`) <- worker idle 22.7 ms <- D3D12 thread fn_2550030 -> fn_254A120 -> fn_25465B0 -> fn_253ACD0 -> fn_2525880 | `d3d12core` -> `dxgkrnl` call blocked **31.8 ms**, readied by svchost **GraphicsPerfSvc** |
| Blackwood return end 42.9 ms, sync 36.7 | render thread 39.2 ms, same wait <- workers idle <- D3D12 thread fn_2550030 -> ... -> fn_25458E0 | `QueryVideoMemoryInfo` blocked **34.2 ms**, readied by GraphicsPerfSvc |
| Roxey road 32.5 ms, sync 23.1 | render thread 28.1 ms, same wait <- workers idle <- fn_2550030 -> ... -> fn_2525880 | `d3d12core` -> `dxgkrnl` blocked **36.5 ms**, readied by GraphicsPerfSvc |

**Cause: Windows' Graphics Performance service starting.** In each window a new `svchost -k GraphicsPerfSvcGroup`
process starts (`P-Start`) 30-50 ms before the game's call blocks. Its samples: `GraphicsPerfSvc.dll!ServiceEtwThreadProc`
-> `PresentTraceConsumerCore::OpenTrace` -> `EnableProviders` -> `CaptureState` -> `sechost!EnableTraceEx2` ->
`dxgkrnl!DxgkEtwEnableCallback` -> `DXGGLOBAL::ReportState` / `DxgkReportGlobalState` -> `ADAPTER_RENDER::FlushScheduler`
-> `dxgmms2!VidSchSuspendResumeDevice` (suspends and resumes every GPU context) -> nvlddmkm. While the graphics kernel
reports its state, the game's kernel graphics calls wait; the D3D12 submission thread holds back the work the render
thread's task needs, and the game thread waits for the render thread.

**It is periodic, not a place:** the service runs ~150 s, stops, and is restarted 1-5 s later (trace 2: starts at
37.3, 189.0, 414.6?, 636.9, 844.0 s). Measured from each game launch: 169 / 174 / 170 / 170 s. Every benchmark run has
the same timeline, so the stall lands at the same route spot. In the 25 untraced runs (p5tr) plus the traced ones,
**16 of 21 in-scene render waits >= 15 ms fall 145-157 s after the first ledger frame**, in every cell (Silver Road's
"spot" included). The first start comes ~17 s after the game starts, before the benchmark's PresentMon (started ~44 s
after the game), so the benchmark's PresentMon is not the trigger; something reacts to the game starting.

Service: `GraphicsPerfSvc`, manual start, trigger = a WNF state change (sc qtriggerinfo: custom system state change
2d7a2816-0c5e-45fc-9ce7-570e5ecde9c9). Running on this PC throughout: Intel **PresentMonSharedService**
(`PresentMonService.exe`, ETW session `PMService`), the **NVIDIA App** (`NVIDIA-NVTOPPS-*` sessions), GamingServices,
Steam's tracing. Which one asks Windows for present statistics is not known yet.

### Consequences
- Not a game or engine bottleneck; not fixable in the game's code without changing system behaviour. In normal play it
  should recur every ~2.5 min while the trigger is active: a 20-40 ms hitch (more with a GPU trace running).
- Trace 1's 155 ms instance was the same service start, stretched by the GPU provider.
- The render/RHI-side class of the traversal ranking (13-50 % of game-thread hitch excess) is mostly this. The real
  per-route content waits are smaller than the ranking suggested (the remaining in-scene waits >= 15 ms: 5 of 21,
  15-18 ms, at 50-80 s after launch, not yet traced).
- Next: identify the consumer by elimination (the user stops one candidate at a time; one short run each with the
  service's state polled), then re-measure without it.

## Elimination test and normal play (2026-10-22 15:28-16:31)
mytools/Check-GraphicsPerfSvc.ps1: one roxey-ft benchmark run while GraphicsPerfSvc is polled every 0.5 s (state change
times from the game process start) + mytools/sync_times.py (in-scene render waits >= 15 ms).

| run | changed (by the user) | service starts (s after launch) | render waits >= 15 ms (s after first ledger frame) |
|---|---|---|---|
| gps-base | nothing | 16.1, 170.2 (stopped 166.6) | 49.8 (16.5 ms), **148.5 (28.6 ms)** |
| gps-nopm | Intel PresentMonSharedService stopped | 14.5, 171.8 | 116.4 (21.7 ms), **153.7 (20.9 ms)** |
| - | NVIDIA overlay: already off in every run (screenshot) | - | - |
| gps-nogamebar | + Game Bar off and terminated | 14.3, 165.9 | **148.0 (24.7 ms)** |
| gps-nonvapp | + NvContainerLocalSystem stopped (the NVIDIA App's container; its user-session child pid 7460 had talked to services.exe 120-260 ms before each traced start: a coincidence) | 14.7, 176.2 | **158.2 (25.6 ms)** |

**Normal play** (user launched from Steam, played 10.8 min, 16:20:01-16:30:48; the three candidates still off; SCSKiller
proxy in place as usual): the service started once at **13.7 s** (start-up, behind menus) and **stayed running the whole
session: no stop, no restart.** In the benchmark it idles out after ~150 s and is restarted, and each restart is the
stall. Leading hypothesis: something keeps the service in use for a visible, focused game window; the benchmark keeps the
game window off-screen and unfocused. **So the periodic render-thread stall is a benchmark artifact**, not a hitch in
the user's play (to confirm with all candidates back on).

### What it changes
- Every benchmark run on this PC has a 20-40 ms stall ~165-176 s after launch (and every ~152 s after, if the run is
  long enough). It inflated: the render/RHI-side class of the traversal ranking, the "repeatable spots" at Blackwood
  return / Roxey road / Silver Road, and worst frames of those cells. Remaining in-scene render waits >= 15 ms not from
  the service: 5 of 21 in the 25-run set (15-18 ms, 50-80 s after launch) + 1 of 21.7 ms (gps-nopm, +116 s).
- Shortlist #1 is closed as "not a game bottleneck". For later analyses: drop frames within +-1 s of a GraphicsPerfSvc
  start (poll the service, or a ledger frame at 145-160 s / +152 s after the first ledger frame with sync >= 15 ms).
- Probably applies to upstream's benchmark too (same hidden window), on Windows builds where GraphicsPerfSvc behaves
  like this (this PC: Windows 11 build 26300). Unverified elsewhere.

## Shortlist #2: the Peryite pair is not a nested completion (2026-10-23 20:11, run p5n-unitlog, branch exp/pacing-nesting-log)
Diagnostic `PacingUnitLog=1` (fork-local, log only): for every frame with > 12 ms of pacing units, each unit's type,
nesting depth (0 = top level, >= 1 = completed inside another unit), path (run / replay / nested-inline /
nested-queued / queued / deferred), actor (object, distance) and time. One npcburst run, native-nofg, build
1.1.0+fork.4 + the log.
- The Peryite frame (6807): `appearance depth 0 replay 9.62 ms, actor 0x...C4450 at 16638 (distant)`, then
  `equipment depth 0 replay 9.93 ms, actor unknown (near/seen)` = 19.6 ms. **Both top level, no nesting** (0 nested
  records in the whole run).
- Actor not resolved for 9 of 10 equipment units and 8 of 8 hair units (appearance: 3 of 6). npc_pacing.cpp
  `ResolveDelegate` walks the delegate's owner weak pointer and up to 4 outers looking for an actor with a location;
  for these delegates it finds none, so the unit counts as near (the safe default). With a backlog such a unit queues
  behind it (unknown key keeps the order), and `ReplayBudget` runs non-distant entries whatever the budget: the
  equipment (and hair) of a distant NPC replays in the same frame as that NPC's appearance unit.
- So the hypothesis "equipment completes inside the appearance unit" is wrong; the cause is the unresolved owner of the
  equipment / hair delegates. Next (log only): what the equipment / hair delegate's owner weak pointer points to (class,
  outer chain, attach parent) so it can be traced to its NPC; if it can, those units are paced like the NPC's other
  distant units (within NpcCreationPacing's existing bounds: unseen, > 4096 units, a few frames).

### Step 2: whose equipment and hair? (2026-10-23 20:50, run p5n-owner2, same diagnostic + owner chains)
For every equipment / hair unit whose actor was not resolved, the delegate's owner weak pointer and up to 6 outers,
with names and classes (decoded from the name pool, found at run time by its first two entries) and each level
guarded separately:
- **Equipment:** owner = `Character Body Pairing Component` (class `VCharacterBodyPairingComponent`); its outer is the
  NPC actor itself (e.g. `BP_Kewan_C_2147472548`, 16,591 units from the player at Peryite; `BP_InventoryPlayer_C`
  for the player's own). Traceable, one level up.
- **Hair / beard / moustache:** owner = the shared hair-piece asset (`VCharacterHairPiece_Hair` / `_Beard` /
  `_Mustache`, e.g. `HP_Nord_HR_BaldPony`), outer = its package. Not traceable through outers: the owner is the
  hairstyle every NPC with it shares. (Hair units were 0.15-1.4 ms in these runs.)
- **Why `ResolveDelegate` fails on equipment although the NPC is one level up:** it reads the actor's root-component
  slot (+0x1A0) on every object it walks, inside one `__try`; on the component that slot is not a pointer, the read
  behind it faults, and the guard ends the whole walk before it reaches the NPC. Guarding each level (as the
  diagnostic does) would resolve it.
- Consequence for the fix: with equipment resolved, the Peryite equipment unit (9.9 ms) is distant and unseen and waits
  for the next frame's budget like the NPC's other units, instead of replaying in the same frame as its appearance
  unit (frame 19.6 ms -> ~10 + ~10 ms). Hair would need the delegate's captured data instead of its owner.

### Step 3: the fix (PacingResolvePerLevel, branch exp/pacing-equipment-resolve; notes/design-pacing-equipment-resolve.md)
Each level of the owner walk guarded: equipment resolves to its NPC. ABBA x5: Peryite hitches/min 6.0 -> 1.2 (ranges
do not overlap), worst frame 30.6 -> 25.4 ms (median); Silver Road and Blackwood within noise; no lag-bound pile-ups.
Confirmed improvement at Peryite.

## Shortlist #3: GPU-side spikes, first checks (2026-10-23 23:30)
- Recap: 17-39 % of visible hitches on the travel routes are frames where the GPU itself took 15-25 ms (normal
  ~10-11.5 ms at native-nofg), with no slow game-thread frame (hitch_sources.py); repeat spots: Silver Road +175 s
  (4/5), +117.7 s (3/5), Blackwood return +58 s (3/5). The user plays with hardware ray tracing on (recorded
  `Altar.GraphicsOptions.EnableHardwareRaytracing` 1), DLSS Quality.
- Exe strings (shipping build): `ProfileGPU` / `r.ProfileGPU.*` (14), `r.RDG.Events`, `DumpGPU`,
  `RHI.GPUHitchThreshold`, `GPUFrameTime` are present as names; the CSV profiler only as 4 ASCII references (likely
  compiled out). Names do not prove the tools work in shipping (their output usually goes to the log, which shipping
  does not write). Nsight Systems 2026.5.1 is installed (target-windows-x64\nsys.exe).
- Candidate causes to tell apart: ray-tracing acceleration-structure builds for newly streamed meshes (BLAS / TLAS),
  virtual shadow map page rendering for new geometry, Lumen surface-cache card captures, Nanite streaming / instance
  culling, or texture uploads.
- First experiment (proposed): Nsight Systems trace of silver-road (2 runs, profile native-nofg) with D3D12 API and GPU
  queue tracing: per frame the GPU time per queue (graphics / async compute / copy) and the D3D12 calls behind it
  (e.g. BuildRaytracingAccelerationStructure counts), matched to the spike frames by time. Then, if a class stands
  out, a diagnostic A/B with that feature's budget (image-changing, diagnostic only) to confirm it.

### Classification of the GPU-side spikes (2026-10-24 00:00, Nsight Systems, 2 silver-road runs)
Recorded with the benchmark launching the game under Nsight Systems (tools\BenchBridge.ps1 `OBLV_NSYS_OUT`, no
administrator rights): `--trace=dx12 --dx12-gpu-workload=individual` (each command list's GPU start/end and queue, every
D3D12 call), reports captures/traces/nsys-silver-1/2 (~850 MB each, not in git). Tracing cost ~7 % fps (93-94 vs ~100).
Analysis: mytools/nsys_gpu_frames.py (frames cut at IDXGISwapChain::Present; per frame the GPU time of the command lists
submitted in it per queue, and the count of selected calls; a command list belongs to the ExecuteCommandLists call with
the largest correlation id <= its own).
- Normal frames: 10.5-10.6 ms, direct-queue GPU 9.2 ms, compute and copy queues ~0 (all work on the direct queue).
  Per frame ~75 BuildRaytracingAccelerationStructure (1.9 M in a run), 0 pipeline creations (17,860 CreatePipelineState
  and 1,203 CreateStateObject over a whole run, mostly at start-up).
- GPU-heavy frames (direct GPU >= 1.3 x the local median and >= 12 ms): 6 and 7 per run, at the same route spots in
  both runs (~90, ~151-155, ~191-196, ~205-209, ~232-236 s), 12-18 ms of GPU work vs 8-11 ms around them.
- Dose-response over all frames (share of frames that are GPU-heavy, both runs):

  | in the frame | < 10 | 10-49 | 50-99 | 100-149 | >= 150 |
  |---|---|---|---|---|---|
  | CopyTextureRegion | 0.00-0.01 % | 0.1 % | 1.1-1.2 % | **17 %** | **50-75 %** |

  BuildRaytracingAccelerationStructure: no dose-response (100-130 builds: 0-0.6 %); AddToStateObject (a ray-tracing
  shader added): 8-11 % of the 35-39 frames that have one (confounded: new materials arrive with their textures);
  CreatePlacedResource: >= 100 in a frame 17-20 % (new textures).
- **Class: texture streaming upload bursts** (100-200 texture regions copied to the GPU in one frame, on the direct
  queue). Not ray-tracing structure builds, not pipeline compiles. Not in upstream's list.
- Lever (exists in the exe, not tried upstream, live value 0 = unlimited): `r.Streaming.AmortizeCPUToGPUCopy` 1 with
  `r.Streaming.MaxNumTexturesToStreamPerFrame` N spreads the CPU-to-GPU texture copies over frames. Timing change
  (mips a few frames later in a burst); risk: slower texture arrival after doors (StreamBehindLoadingScreen streams
  behind the loading screen) and at the burst spots themselves.
- **Lever test (2026-10-24, trace nsys-silver-cap20; notes/design-texture-upload-cap.md):** `r.Streaming.AmortizeCPUToGPUCopy 1`
  + `MaxNumTexturesToStreamPerFrame 20` leaves the bursts exactly as they were (same copy counts at the same spots in
  all three runs): fixed content uploads when new cells load, not streamed mips. Proven wrong for this lever. 6-8 such
  frames per 5-minute flight (12-18 ms vs ~10.5 ms).

## Shortlist #4: the near/seen Blackwood bursts arrive while the NPC is still faded out (2026-10-24 01:00-01:54, branch exp/pacing-fade-check)
5 runs of `blackwood` (native-nofg, fresh launches, fork.5 + PacingUnitLog with a fade recorder). Every in-play burst:
two NPCs (12.0-12.3k and 16.6-16.7k units), 4-5 units, 15.3-18.8 ms of units in one frame, 7/7 at fade value 5.00 (the
game's start value). The fade started 7-80 ms before the last unit, left 5 after +22-200 ms and reached 0 at
+1.0-1.2 s. Pacing counts these NPCs as seen because their components are drawn at value 5. Details, table and
the proposed rule (PacingFadeUnseen, not built): notes/design-pacing-fade-unseen.md. Frame times of these runs
(single runs, not a comparison): blackwood-flight 85-92 fps, worst 29-40 ms; blackwood-return 83-96 fps, worst 21-32 ms.

## Shortlist #4 measured: PacingFadeUnseen ABBA x5 (2026-10-24 02:58-04:56, branch exp/pacing-fade-unseen)
Image test first: fade value 5 draws nothing while the game's fade runs (FadeHoldTest time series; stock fades show a
ghost at ~0.6 s). Then base (rule off) vs rule on, blackwood / npcburst / silver-road, 5 per side:
- blackwood-flight 10.9 -> 4.7 hitches/min, ranges overlapping;
- worst frames unchanged everywhere;
- the other routes within noise.
The rule acted on 4-6 units per run. Of the far-NPC bursts, 3 of 5 runs had them split; one was unchanged (17.6 ms);
one piled up at the deadline (23.2 ms). **Within noise.** Details and the possible refinement:
notes/design-pacing-fade-unseen.md.
- Refined (hold to value 3, soft/hard deadlines; 2026-10-24 12:43-14:42): bursts split in every run, no deadline runs.
  Worst frame at the 25.4 s NPC spot 30.3 -> 23.1 ms median (ranges separate, **Confirmed** at the spot). Scene
  metrics and other routes within noise. Details: notes/design-pacing-fade-unseen.md.
