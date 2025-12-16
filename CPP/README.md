# SWAT+ NetCDF Converter (C++)

Part of the [SWAT+ NetCDF Converter](../README.md) project.

A C++ tool to convert SWAT+ model weather input to NetCDF format.

## Prerequisites

- CMake 3.15 or higher
- C++17 compatible compiler
- Git

## Build Instructions

### Windows

We provide a batch script that automatically handles dependency installation (via `vcpkg`) and static compilation.

1.  Ensure you have **Visual Studio 2019/2022** installed with the "Desktop development with C++" workload.
2.  Run the build script:

    ```cmd
    .\build_release.bat
    ```

    This script will:
    - Detect your architecture (x64 or ARM64).
    - Download and bootstrap `vcpkg` if not found.
    - Install static versions of `netcdf-cxx4` and `gdal`.
    - Configure and build the project in Release mode.

The executable will be located at `build\Release\swat2netcdf.exe`.

### Linux

1.  Install dependencies:

    **Arch Linux:**
    ```bash
    sudo pacman -S base-devel cmake netcdf-cxx4 gdal
    ```

    **Ubuntu/Debian:**
    ```bash
    sudo apt-get update
    sudo apt-get install build-essential cmake libnetcdf-dev libnetcdf-c++4-dev libgdal-dev
    ```

2.  Build the project:

    ```bash
    cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
    cmake --build build

    The executable will be located at `build/swat2netcdf`.

3.  Install it:

    If you specifically want it in `/usr/bin` (so the final path is `/usr/bin/swat2netcdf`), use:
    

    ```bash
    sudo cmake --install build --prefix /usr
    ```    



### macOS

1.  Install dependencies using Homebrew:

    ```bash
    brew install cmake netcdf gdal
    ```

2.  Build the project:

    ```bash
    cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
    cmake --build build
    ```

The executable will be located at `build/swat2netcdf`.

## Usage

```bash
./swat2netcdf --help
```

**Arguments:**
- `--region <RegionName>`: Name of the region.
- `--txtInOutDir <Path>`: Path to the SWAT+ TxtInOut directory.
- `--convertedDir <Path>`: Path where the NetCDF files will be saved.
- `--climateResolution <float>`: (Optional) Resolution in degrees (default: 0.25).
- `--shapePath <Path>`: (Optional) Path to shapefile.
- `--stopDate <YYYY-MM-DD>`: (Optional) Stop date (default: 2500-12-31).
