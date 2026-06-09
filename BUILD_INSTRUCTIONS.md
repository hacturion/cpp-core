# Building the C++ Core Library (for Excel XLL)

This C++ library exposes a stable **C ABI** (`mylib.dll`) so it can be called from:
- C# / Excel-DNA (via P/Invoke) → this is the recommended path for a native-feeling XLL
- ctypes from Python (optional)
- Other languages

**This guide focuses on the pure C++ + Excel XLL workflow (no Python required).**

## Prerequisites (Windows)

You need a C++ compiler. The recommended option is:

### Option 1: Visual Studio Build Tools (Free, Recommended)

1. Download **Visual Studio Build Tools**:
   - https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022

2. During installation, select:
   - **Desktop development with C++** workload
   - Under "Individual components", make sure these are selected:
     - MSVC v143 - VS 2022 C++ x64/x86 build tools (or latest)
     - Windows 10/11 SDK (latest)
     - CMake (optional but nice)

3. After installation, open a **"x64 Native Tools Command Prompt for VS 2022"** (important — not a normal cmd/PowerShell).

**Note**: You do **not** need Python at all for the XLL. Python support is completely optional and off by default.

### Option 2: Full Visual Studio 2022 Community
Same as above, just install the full IDE if you want it.

---

## Building the DLL (Simplest Way)

Once you have the C++ tools installed:

### Using the provided batch file (easiest)

```powershell
cd cpp-core
.\compile_dll.bat
```

This will produce `build\mylib.dll` (and `.lib` + `.pdb`).

### Manual build using Developer Command Prompt

```cmd
cd cpp-core
mkdir build
cd build

cl /nologo /LD /EHsc /O2 /D MYLIB_EXPORTS ^
   /I ..\include ^
   ..\src\mylib.cpp ^
   /Fe:mylib.dll ^
   /link /OUT:mylib.dll
```

The key flag is `/LD` (create DLL).

---

## Verifying the Build

After building, you should have:

```
cpp-core/
└── build/
    └── Release/
        ├── mylib.dll      ← The important file (copy next to your .xll)
        ├── mylib.lib
        └── mylib.pdb
```

Copy `mylib.dll` into the same folder as your compiled `.xll` (see `excel-udf` project).

---

## Using from Python (ctypes)

See `examples/python/use_from_python.py`

As soon as you have a real Python installation (not the Microsoft Store stub), run:

```powershell
python examples/python/use_from_python.py
```

Make sure `mylib.dll` is either:
- In the current directory, or
- In a directory on your `PATH`, or
- You pass the full path to `ctypes.CDLL`

---

## Using from Excel (Recommended: C++ DLL + Excel-DNA XLL)

See the sibling `excel-udf` project.

1. Build `mylib.dll` (above) with `-DMYLIB_BUILD_PYTHON=OFF`.
2. Copy `mylib.dll` into the same folder as your compiled `.xll`.
3. Build the `excel-udf` project:
   ```powershell
   cd excel-udf
   dotnet build -c Release
   ```
4. The `NATIVE.*` functions become available in Excel. All the expensive work (Kahan summation, Sobol, basic stats, Monte Carlo, etc.) runs in your C++ code.

This is the clean "C++ for the XLL" experience with no Python.

---

# Modern Build with CMake (Recommended)

For serious numerical methods work, the raw `cl.exe` approach is limited.

We strongly recommend using **CMake**. This produces the `mylib.dll` used by the Excel XLL. 

Python support is **optional** (disabled by default for pure C++ / XLL users).

## 1. Install Prerequisites

You still need the **Visual Studio Build Tools** with the C++ workload (see top of this file).

You also need a real Python with development headers:

- Recommended: Use **uv**, **conda**, or the official Python installer (not the Microsoft Store version).
- Make sure you can run `python -c "import sysconfig; print(sysconfig.get_config_var('INCLUDEPY'))"` successfully.

## 2. Configure and Build with CMake

From a regular PowerShell / cmd (CMake from VS Build Tools should be on PATH after installation):

```powershell
cd cpp-core

# Create build directory
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
      -DMYLIB_BUILD_SHARED=ON

# Build
cmake --build build --config Release
```

This will produce:

- `build/Release/mylib.dll` (the native library for Excel-DNA XLL and ctypes)

If you later want Python bindings, add `-DMYLIB_BUILD_PYTHON=ON`.

```python
import numpy as np
import mylib

x = np.linspace(0, 10, 1000)
result = mylib.moving_average(x, window=50)
```

## 4. Using from VSCode (Recommended)

1. Install the **C/C++** and **CMake Tools** extensions in VSCode.
2. Open the `cpp-core` folder.
3. CMake Tools should detect the project.
4. Select the "Visual Studio Community 2022 Release - x64" kit (or whatever MSVC kit you have).
5. Configure + Build from the CMake Tools sidebar.

This gives you excellent IntelliSense, debugging, etc.

---

## Adding Eigen (Highly Recommended for Numerical Methods)

Many numerical methods benefit enormously from a good linear algebra library.

Uncomment the Eigen section in `CMakeLists.txt` and use:

```powershell
# Option A: vcpkg (cleanest long-term)
vcpkg install eigen3

# Then pass to CMake:
cmake -S . -B build ... -DCMAKE_TOOLCHAIN_FILE=.../vcpkg.cmake
```

Or use FetchContent to pull Eigen (header-only, easy).

---

## Strategy for Your Legacy Code

Since you mentioned legacy C++ code for numerical methods, here is the suggested migration/wrapping path:

1. **Keep the legacy code compiling** inside this CMake project (add its source files).
2. **Expose the most valuable functions** via the C ABI (`mylib.h`) for quick Excel access.
3. **Create rich pybind11 bindings** for the parts you use heavily from notebooks.
4. Gradually modernize / refactor the legacy code behind a clean interface (the bindings protect you from internal changes).
5. For very old code with raw arrays, pybind11's `py::array_t` + `py::buffer` makes wrapping much less painful than it used to be.

Would you like me to help structure a specific legacy module?

---

## Future Enhancements We Can Add

- Proper Python packaging (`scikit-build-core` or `pyproject.toml` + CMake)
- nanobind instead of / in addition to pybind11 (lower overhead)
- Automatic generation of stubs (`pybind11-stubgen` or `nanobind`)
- Support for 2D arrays / matrices
- Threading / parallel execution from C++ (OpenMP, std::execution, TBB)
- GPU acceleration hooks later (CUDA / oneAPI) if needed

Let me know how you'd like to proceed.


---

## Next Level: Better Python Bindings

When you want nicer Python experience (NumPy arrays, exceptions, classes, etc.), we can add **pybind11** or **nanobind**.

This will require CMake + a Python development environment.

---

## Troubleshooting

- **"cl is not recognized"** → You did not open a **Developer Command Prompt / x64 Native Tools Command Prompt**.
- **Architecture mismatch** → Build 64-bit DLL if your Python/Excel is 64-bit (almost always the case today).
- **DLL not found at runtime** → Put `mylib.dll` next to the .exe/.py/.xll or add its folder to PATH.

Let me know if you hit any issues and I'll help debug.
