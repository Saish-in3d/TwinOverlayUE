# TwinOverlayUE

Unreal Engine 5.8 project that uses the [SynthPass](https://github.com/Saish-in3d/SynthPass) plugin to composite CG warehouse content over recorded CCTV footage in a single render pass.

The `ASynthPassController` actor (`Source/TwinOverlayUE/SynthPassController.h`) drives the compositor: it plays a movie from `Content/Movies` as the background plate, renders selected foreground actors over it, and fades the full virtual warehouse in and out on top of the footage.

---

## Requirements

| Dependency | Version / notes |
|---|---|
| Unreal Engine | 5.8 (installed from the Epic Games Launcher) |
| Visual Studio | 2022 with the components listed in `.vsconfig` |
| Git | With submodule support |
| Git LFS | Recommended for future binary assets |
| GPU | DirectX 12 capable (the project defaults to DX12) |

---

## Setup

### 1. Install Unreal Engine 5.8

Install **Unreal Engine 5.8** from the Epic Games Launcher. No source build is needed.

### 2. Install Visual Studio 2022

Install Visual Studio 2022, then import the workloads and components this project needs:

1. Open the **Visual Studio Installer**.
2. Choose **More > Import configuration**.
3. Select the `.vsconfig` file at the root of this repository.

This installs the *Game development with C++* and *Desktop development with C++* workloads, MSVC 14.44, the Windows 11 SDK (22621) and the Unreal Engine IDE support components.

### 3. Clone the repository with its submodule

SynthPass is included as a git submodule under `Plugins/SynthPass`.

```bash
git clone --recurse-submodules https://github.com/Saish-in3d/TwinOverlayUE.git
```

If you already cloned without `--recurse-submodules`:

```bash
git submodule update --init --recursive
```

### 4. Restore the Megascans assets

The Megascans content used by the warehouse scene is **not tracked** in this repository because of its size (about 7 GB). Without it the `Industrial_Warehouse` map opens with missing meshes and materials.

Place the assets back at:

```
Content/Scene_Warehouse/Assets/MS/
├── 3D/
├── Decals/
└── Surfaces/
```

Download them from [Fab](https://www.fab.com) (Quixel Megascans) and add them to the project, keeping the folder layout above so existing references resolve.

### 5. Generate project files and build

Right-click `TwinOverlayUE.uproject` and choose **Generate Visual Studio project files**, then open `TwinOverlayUE.sln` and build the `Development Editor` / `Win64` configuration.

Or build from the command line:

```bat
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" TwinOverlayUEEditor Win64 Development -Project="%CD%\TwinOverlayUE.uproject" -WaitMutex
```

Adjust the engine path if Unreal Engine is installed somewhere else.

### 6. Open the project

Open `TwinOverlayUE.uproject`. The editor starts on `/Game/Scene_Warehouse/Maps/Industrial_Warehouse`. The first launch compiles shaders, including the SynthPass compute shaders, which can take a while.

---

## Plugins

Enabled in `TwinOverlayUE.uproject`:

| Plugin | Source | Purpose |
|---|---|---|
| GPUCompositionGraphicsPipeline (SynthPass) | `Plugins/SynthPass` submodule | Single-pass composition of CG over video |
| PythonScriptPlugin | Bundled with the engine | Editor Python scripting |
| ModelingToolsEditorMode | Bundled with the engine | In-editor modeling tools |
| AIModuleToolset, AllToolsets | Bundled with the engine (experimental) | Editor toolsets |
| Terminal | Bundled with the engine (experimental) | In-editor terminal |

The game module depends on `MediaAssets` and `GPUCompositionGraphicsPipeline` (see `Source/TwinOverlayUE/TwinOverlayUE.Build.cs`).

### Project rendering settings

SynthPass relies on these settings in `Config/DefaultEngine.ini`:

```ini
r.PostProcessing.PropagateAlpha=True   ; keeps alpha through post processing for matte output
r.AntiAliasingMethod=1                 ; FXAA
```

---

## Updating SynthPass

```bash
cd Plugins/SynthPass
git pull origin main
cd ../..
git add Plugins/SynthPass
git commit -m "Update SynthPass"
```
