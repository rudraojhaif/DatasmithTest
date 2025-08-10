# DatasmithTest Project

## Overview

This project demonstrates live synchronization from CAD software to Unreal Engine using the Datasmith Direct Link plugin. It provides a custom UI to control import settings, manage sources, and trigger updates for real-time visualization.

Additionally, it includes a **DSLightSyncer** utility that receives and processes light data from Rhino through **real-time TCP communication** using the [LightSyncPlugin](https://github.com/rudraojhaif/LightSyncPlugin). This provides automatic, live synchronization of lights between Rhino and Unreal Engine with superior stability compared to Datasmith's built-in light sync.

## Prerequisites

Before proceeding, ensure you have the following installed:

- **Unreal Engine 5.4 or later**
- **Visual Studio 2022** (with *Desktop development with C++* workload)
- **Datasmith plugin** enabled in Unreal Engine
- **Datasmith Exporter plugin** installed in your CAD application (e.g., Rhino, Revit, SketchUp, etc.)
- *(Recommended)* **[LightSyncPlugin for Rhino](https://github.com/rudraojhaif/LightSyncPlugin)** for real-time light synchronization

## Installation and Setup

### 1. Extract the Project

Download and extract the ZIP archive of the project to a suitable location on your computer (e.g., `D:\Projects\DatasmithTest`).

### 2. Generate Visual Studio Solution

1. Locate the `DatasmithTest.uproject` file in the root folder.
2. Right-click on the `.uproject` file.
3. Select **"Generate Visual Studio project files"** from the context menu.
   - This will create a `.sln` file and generate intermediate project files.

### 3. Open and Build in Visual Studio

1. Open the generated `.sln` (solution) file in **Visual Studio 2022**.
2. Set the configuration to **Development Editor** and **Win64** platform.
3. Build the solution via **Build → Build Solution (Ctrl+Shift+B)**.
   - Wait for the build to complete without errors.

### 4. Open Unreal Project

Once the build is successful:

1. Double-click the `DatasmithTest.uproject` file to launch the project in Unreal Editor.
2. Wait for shaders and assets to compile if it's your first time opening.

## Preparing for Live Sync

### CAD Geometry Sync

1. **Open your CAD application** (e.g., Rhino).
2. **Ensure the Datasmith Exporter plugin is installed and enabled**.
3. **Load the CAD file** you want to sync.

### Light Sync Setup (Rhino)

For enhanced light synchronization with Rhino:

1. **Install the [LightSyncPlugin](https://github.com/rudraojhaif/LightSyncPlugin)** in Rhino
2. **Load the plugin** (drag and drop the `.rhp` file)
3. **Ensure both Rhino and Unreal are running** for real-time TCP communication

## Using Live Sync in Unreal

### CAD Geometry Sync

Once both Unreal and your CAD software are running:

1. In your CAD software, open the **Datasmith plugin panel**.
2. Click **"Auto Sync"** to begin live linking with Unreal Engine.

In Unreal:

1. **Run the default map** in Play mode.
2. Press the **"1" key** to bring up the Datasmith runtime widget.
3. In the widget, select the appropriate **source** from the list.
4. Click **"Update Direct Link"** to manually fetch updates from CAD.

### Real-Time Light Sync from Rhino

With the [LightSyncPlugin](https://github.com/rudraojhaif/LightSyncPlugin) installed:

1. **Start Play Mode** in Unreal Engine
2. Press **"1"** to open the runtime widget
3. Click **"Sync Lights"** to establish TCP connection with Rhino
4. **Automatic Synchronization**: Any changes to lights in Rhino (add, delete, modify, undelete) will now automatically update in Unreal Engine in real-time

#### Supported Light Operations

- **Add Light**: New lights appear instantly in Unreal
- **Delete Light**: Lights are removed and blacklisted to prevent ghost lights
- **Modify Light**: Position, rotation, intensity, and color changes sync immediately
- **Undelete Light**: Restored lights reappear in Unreal

#### Legacy Manual Light Sync

For backward compatibility, you can still manually export lights:

1. Export lights from Rhino using the `ListLights` command
2. This creates: `C:/ProgramData/RhinoLightSync/Lights.txt`
3. Click **"Sync Lights"** in the Unreal widget to import

## Technical Features

### Light Synchronization

- **TCP Communication**: Real-time data exchange on port 5173
- **JSON Protocol**: Structured light data with position, rotation, intensity, color
- **Unit Conversion**: Automatic conversion from Rhino units to meters
- **Smart State Management**: Blacklist system prevents deleted light artifacts
- **Background Processing**: Non-blocking communication maintains UI responsiveness

### Supported Light Types

- **Point Lights**: Omnidirectional with position and intensity
- **Directional Lights**: Parallel rays for distant light sources
- **Spot Lights**: Cone-shaped with inner/outer angles
- **Ambient Lights**: *(Planned for future release)*

## Folder Structure Summary

```
DatasmithTest/
│
├───Actors              // CAD sync logic (DSRuntimeManager), 
│                       // Real-time Rhino light sync (DSLightSyncer)
├───Controllers         // Custom Player Controller (DSPlayerController)
├───Modes               // Game Mode class (DSGameMode)
├───Pawns               // Default Pawn used in the scene (DSPawn)
├───Widgets             // UI for import, light, and graphics settings (DSRuntimeWidget)
│
├───DatasmithTest.uproject
├───DatasmithTest.sln (after generation)
└───Build and Config Files
```

## Advantages Over Standard Datasmith Light Sync

While Datasmith provides built-in light synchronization, this implementation offers:

- **Enhanced Stability**: More reliable light syncing specifically optimized for Rhino
- **Real-time Updates**: Immediate synchronization without manual refresh
- **Better Error Handling**: Robust TCP communication with timeout protection
- **Smart State Management**: Prevents deleted light artifacts through blacklisting
- **Optimized Performance**: Background processing maintains UI responsiveness

## Troubleshooting

### General Issues

- For live CAD updates, both Unreal and the CAD application must be running simultaneously
- If Unreal doesn't appear in the CAD sync targets:
  - Ensure both apps are on the same network or machine
  - Enable Auto Sync in the CAD app
  - Enable the **Datasmith Runtime** plugin in Unreal under **Edit → Plugins**

### Light Sync Issues

- **Connection Problems**: Ensure both Rhino plugin and Unreal project are running
- **Port Conflicts**: Check if port 5173 is available
- **Missing Updates**: Verify you're in Play Mode in Unreal Engine
- **File Permissions**: Ensure write access to `C:/ProgramData/RhinoLightSync/`
- **Unit Scaling**: Check that your Rhino model units are properly set

### Build Issues

- Ensure your Visual Studio installation includes **Unreal Engine C++ toolchain**
- Double-check that the **Datasmith Runtime** and related plugins are enabled in Unreal under **Edit → Plugins**

## Related Projects

- **[LightSyncPlugin for Rhino](https://github.com/rudraojhaif/LightSyncPlugin)**: The companion Rhino plugin that enables real-time light synchronization

## License

Copyright (c) 2025 Rudra Ojha. All rights reserved.

This source code is provided for educational and reference purposes only. Redistribution, modification, or use of this code in any commercial or private product is strictly prohibited without explicit written permission from the author.

## Developer

**Rudra Ojha**  
Email: [rudraojhaif@gmail.com](mailto:rudraojhaif@gmail.com)

## Support

For technical support or contributions, please visit the respective GitHub repositories or contact the developers through the project pages.