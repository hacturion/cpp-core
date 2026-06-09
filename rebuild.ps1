<#
.SYNOPSIS
    Helper to (re)build the C++ mylib core with proper VS environment awareness.

.DESCRIPTION
    Run this from an "x64 Native Tools Command Prompt for VS 2022" (or x64 Native Tools PowerShell)
    for the simple CL path, or from a normal PowerShell for the CMake path (CMake must be on PATH).

    Supports building both the plain DLL (for ctypes / Excel-DNA P/Invoke) and the pybind11 .pyd.

.EXAMPLE
    .\rebuild.ps1 -Configuration Release -PythonVersion 3.11

.EXAMPLE
    .\rebuild.ps1 -UseCMake -PythonVersion 3.12   # for the uv 3.12 env in finsoft-sim
#>
param(
    [ValidateSet('Debug','Release')]
    [string]$Configuration = 'Release',

    [ValidateSet('3.11','3.12')]
    [string]$PythonVersion = '3.11',

    [switch]$UseCMake = $true,

    [switch]$BuildPython = $true
)

$ErrorActionPreference = 'Stop'

Write-Host "=== Rebuilding mylib (cpp-core) ===" -ForegroundColor Cyan
Write-Host "Configuration : $Configuration"
Write-Host "Python target : $PythonVersion (affects pybind11 .pyd)"
Write-Host "Use CMake     : $UseCMake"
Write-Host ""

if ($UseCMake) {
    Write-Host "Using CMake + Visual Studio generator..." -ForegroundColor Yellow

    $cmakeArgs = @(
        "-S", ".",
        "-B", "build",
        "-G", "Visual Studio 17 2022",
        "-A", "x64",
        "-DMYLIB_BUILD_SHARED=ON",
        "-DMYLIB_BUILD_PYTHON=ON"
    )

    if ($PythonVersion -eq '3.12') {
        # User may need to set Python_ROOT or let CMake find the uv Python
        Write-Host "Hint: For uv Python 3.12, make sure 'python' on PATH is the 3.12 one, or pass -DPython3_EXECUTABLE=..." -ForegroundColor DarkGray
    }

    cmake @cmakeArgs
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }

    cmake --build build --config $Configuration --parallel
    if ($LASTEXITCODE -ne 0) { throw "CMake build failed" }

    Write-Host "`nBuild complete." -ForegroundColor Green
    Write-Host "  DLL : build\$Configuration\mylib.dll"
    if ($BuildPython) {
        Write-Host "  PYD : build\$Configuration\mylib*.pyd  (or check Release/)"
    }

} else {
    Write-Host "Using legacy cl.exe path (compile_dll.bat)..." -ForegroundColor Yellow
    & .\compile_dll.bat
}

Write-Host "`nNext steps:"
Write-Host "  1. Copy the new mylib.dll (and .pyd if Python) to places that need it:"
Write-Host "     - Next to your .xll (excel-udf bin/...)"
Write-Host "     - Update PYTHONPATH for xlwings / notebooks"
Write-Host "  2. Rebuild excel-udf if you use the native path."
Write-Host "  3. Re-import UDFs in Excel (xlwings ribbon) or restart Excel for .xll."
Write-Host ""
Write-Host "Tip: For the finsoft-sim uv Python 3.12, run this from a shell where 'python' resolves to that 3.12," -ForegroundColor DarkGray
Write-Host "     then use the resulting .pyd (or fall back to ctypes + .dll)." -ForegroundColor DarkGray
