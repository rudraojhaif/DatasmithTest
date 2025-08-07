---

# DatasmithTest Project

## Overview

This project demonstrates live synchronization from CAD software to Unreal Engine using the Datasmith Direct Link plugin. It provides a custom UI to control import settings, manage sources, and trigger updates for real-time visualization.
Additionally, it includes a **DSLightSyncer** utility to import and sync light data exported from Rhino using the [LightSyncPlugin](#).

---

## Prerequisites

Before proceeding, ensure you have the following installed:

* **Unreal Engine 5.4 or later**
* **Visual Studio 2022** (with *Desktop development with C++* workload)
* **Datasmith plugin** enabled in Unreal Engine
* **Datasmith Exporter plugin** installed in your CAD application (e.g., Rhino, Revit, SketchUp, etc.)
* *(Optional)* **LightSyncPlugin for Rhino**, if you're syncing lights from Rhino

---

## Installation and Setup

### 1. Extract the Project

Download and extract the ZIP archive of the project to a suitable location on your computer (e.g., `D:\Projects\DatasmithTest`).

### 2. Generate Visual Studio Solution

1. Locate the `DatasmithTest.uproject` file in the root folder.
2. Right-click on the `.uproject` file.
3. Select **"Generate Visual Studio project files"** from the context menu.

   * This will create a `.sln` file and generate intermediate project files.

### 3. Open and Build in Visual Studio

1. Open the generated `.sln` (solution) file in **Visual Studio 2022**.
2. Set the configuration to **Development Editor** and **Win64** platform.
3. Build the solution via **Build → Build Solution (Ctrl+Shift+B)**.

   * Wait for the build to complete without errors.

### 4. Open Unreal Project

Once the build is successful:

1. Double-click the `DatasmithTest.uproject` file to launch the project in Unreal Editor.
2. Wait for shaders and assets to compile if it's your first time opening.

---

## Preparing for Live Sync

1. **Open your CAD application** (e.g., Rhino).
2. **Ensure the Datasmith Exporter plugin is installed and enabled**.
3. **Load the CAD file** you want to sync.

*(Optional)*: If using Rhino, export lights using the `ListLights` command from the LightSyncPlugin. This will write data to:

```
C:/ProgramData/RhinoLightSync/Lights.txt
```

---

## Using Live Sync in Unreal

Once both Unreal and your CAD software are running:

1. In your CAD software, open the **Datasmith plugin panel**.
2. Click **"Auto Sync"** to begin live linking with Unreal Engine.

In Unreal:

1. **Run the default map** in Play mode.
2. Press the **"1" key** to bring up the Datasmith runtime widget.
3. In the widget, select the appropriate **source** from the list.
4. Click **"Update Direct Link"** to manually fetch updates from CAD.

**Syncing Lights from Rhino**:

1. Export lights from Rhino using the `ListLights` command.
2. While in Play Mode in Unreal, click **"Sync Lights"** in the UI.
   This invokes the `DSLightSyncer`, which parses the file from `ProgramData` and spawns matching Unreal lights.

---

## Folder Structure Summary

```
DatasmithTest/
│
├───Actors              // CAD sync logic (DSRuntimeManager), Rhino light sync (DSLightSyncer)
├───Controllers         // Custom Player Controller (DSPlayerController)
├───Modes               // Game Mode class (DSGameMode)
├───Pawns               // Default Pawn used in the scene (DSPawn)
├───Widgets             // UI for import, light, and graphics settings (DSRuntimeWidget)
│
├───DatasmithTest.uproject
├───DatasmithTest.sln (after generation)
└───Build and Config Files
```

---

## Notes

* For live CAD updates, both Unreal and the CAD application must be running simultaneously.
* For light sync from Rhino:

  * Ensure the `Lights.txt` export file exists.
  * Make sure `DSLightSyncer` is properly set up and mapped in the project UI.
* If Unreal doesn't appear in the CAD sync targets:

  * Ensure both apps are on the same network or machine.
  * Enable Auto Sync in the CAD app.
  * Enable the **Datasmith Runtime** plugin in Unreal.

---

## Support

If you encounter build issues:

* Ensure your Visual Studio installation includes **Unreal Engine C++ toolchain**.
* Double-check that the **Datasmith Runtime** and related plugins are enabled in Unreal under **Edit → Plugins**.

---

