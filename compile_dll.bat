@echo off
setlocal

echo ========================================
echo Building mylib.dll (C++ core library)
echo ========================================

if not exist build mkdir build
cd build

:: Use x64 compiler (change to x86 if you really need 32-bit)
cl /nologo /LD /EHsc /O2 /D MYLIB_EXPORTS ^
   /I ..\include ^
   ..\src\mylib.cpp ^
   /Fe:mylib.dll ^
   /link /OUT:mylib.dll /IMPLIB:mylib.lib

if %ERRORLEVEL% neq 0 (
    echo.
    echo *** BUILD FAILED ***
    echo Make sure you are running this from a "x64 Native Tools Command Prompt for VS"
    cd ..
    exit /b 1
)

echo.
echo *** BUILD SUCCESSFUL ***
echo.
echo Output:
echo   %CD%\mylib.dll
echo   %CD%\mylib.lib
echo.
echo Copy mylib.dll next to your Python scripts or Excel add-in.

cd ..
endlocal
