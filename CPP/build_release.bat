@echo off
setlocal

REM ==========================================
REM 1. Detect Architecture
REM ==========================================
if /i "%PROCESSOR_ARCHITECTURE%"=="ARM64" (
    echo Detected ARM64 architecture.
    set TRIPLET=arm64-windows-static
) else (
    echo Detected x64 architecture.
    set TRIPLET=x64-windows-static
)

REM ==========================================
REM 2. Setup vcpkg
REM ==========================================
REM Check if VCPKG_ROOT is set
if "%VCPKG_ROOT%"=="" (
    REM Check default location
    if exist "C:\vcpkg\vcpkg.exe" (
        set "VCPKG_ROOT=C:\vcpkg"
    ) else (
        REM Check local directory
        if exist "%~dp0vcpkg\vcpkg.exe" (
            set "VCPKG_ROOT=%~dp0vcpkg"
        ) else (
            echo vcpkg not found.
            echo Cloning vcpkg into "%~dp0vcpkg"...
            git clone https://github.com/microsoft/vcpkg.git "%~dp0vcpkg"
            if %ERRORLEVEL% NEQ 0 (
                echo Error: Failed to clone vcpkg. Please install git or check internet connection.
                exit /b 1
            )
            echo Bootstrapping vcpkg...
            call "%~dp0vcpkg\bootstrap-vcpkg.bat"
            set "VCPKG_ROOT=%~dp0vcpkg"
        )
    )
)

echo Using vcpkg at: %VCPKG_ROOT%
echo Target Triplet: %TRIPLET%

REM ==========================================
REM 3. Install Dependencies
REM ==========================================
echo Installing Dependencies via vcpkg...
"%VCPKG_ROOT%\vcpkg.exe" install netcdf-cxx4:%TRIPLET% gdal:%TRIPLET%
if %ERRORLEVEL% NEQ 0 (
    echo Error installing dependencies.
    exit /b %ERRORLEVEL%
)

REM ==========================================
REM 4. Configure CMake
REM ==========================================
echo Configuring CMake...

REM Workaround for LERC library linking issue in static builds
set "LERC_LIB=%VCPKG_ROOT%\installed\%TRIPLET%\lib\Lerc.lib"
set "PKG_CONFIG_PATH=%VCPKG_ROOT%\installed\%TRIPLET%\lib\pkgconfig"

cmake -B build -S . ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake" ^
    -DVCPKG_TARGET_TRIPLET=%TRIPLET% ^
    -DLERC_LIBRARY="%LERC_LIB%"

if %ERRORLEVEL% NEQ 0 (
    echo Error configuring CMake.
    exit /b %ERRORLEVEL%
)

REM ==========================================
REM 5. Build
REM ==========================================
echo Building Release...
cmake --build build --config Release

if %ERRORLEVEL% NEQ 0 (
    echo Error building project.
    exit /b %ERRORLEVEL%
)

echo ==========================================
echo Build Complete!
echo Executable is at: build\Release\swat2netcdf.exe
echo ==========================================
