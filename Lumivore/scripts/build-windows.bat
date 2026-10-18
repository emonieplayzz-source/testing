@echo off
REM Local build. Requires: Visual Studio 2022 (or Build Tools) with "Desktop development with C++", CMake 3.22+, Git.
setlocal
where cmake >nul 2>nul || (echo CMake not found. Install from https://cmake.org/download/ & exit /b 1)
where git   >nul 2>nul || (echo Git not found. Install from https://git-scm.com/ & exit /b 1)
cd /d "%~dp0.."
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 || exit /b 1
cmake --build build --config Release --target LumivoreCoreTests --parallel || exit /b 1
ctest --test-dir build -C Release --output-on-failure || exit /b 1
cmake --build build --config Release --target Lumivore_VST3 --parallel || exit /b 1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\package.ps1 || exit /b 1
echo.
echo Done. See Lumivore-Windows-x64.zip and the release folder.
