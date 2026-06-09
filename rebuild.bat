@echo off
setlocal

echo ========================================
echo Rebuild mylib (cpp-core) - simple wrapper
echo ========================================
echo.
echo IMPORTANT: You must run this from an
echo "x64 Native Tools Command Prompt for VS 2022"
echo (search Start menu for it, or use the shortcut
echo installed with Visual Studio Build Tools).
echo.
echo If you double-clicked this file, close Notepad
echo and run it from the correct prompt instead.
echo.

cd /d "%~dp0"

set CONFIG=Release

echo Running CMake configure + build for %CONFIG%...
echo.

cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
      -DMYLIB_BUILD_SHARED=ON ^
      -DMYLIB_BUILD_PYTHON=ON

if errorlevel 1 (
    echo.
    echo *** CMake configure failed ***
    echo Make sure you are in the x64 Native Tools prompt
    echo and that CMake is on PATH (it usually is after
    echo installing "Desktop development with C++").
    pause
    exit /b 1
)

cmake --build build --config %CONFIG% --parallel

if errorlevel 1 (
    echo.
    echo *** Build failed ***
    pause
    exit /b 1
)

echo.
echo *** BUILD SUCCESSFUL ***
echo.
echo Output files should be in:
echo   build\%CONFIG%\mylib.dll
echo   build\%CONFIG%\mylib*.pyd   (if Python module built)
echo.
echo Next:
echo   1. Copy mylib.dll next to your .xll files
echo      (excel-udf\bin\Release\... folders)
echo   2. For Python/xlwings: set PYTHONPATH to this build\%CONFIG% folder
echo   3. Rebuild the excel-udf project
echo   4. Re-import UDFs in Excel or reload the .xll
echo.

pause
endlocal
