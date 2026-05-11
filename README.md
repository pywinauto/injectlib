# injectlib

injectlib is a Python package that allows you to inject DLLs into running
processes on Windows (32 or 64-bit).

This can be useful for various tasks, such as debugging or adding
functionality to existing applications.

This is initially created for desktop GUI automation purpose
(project pywinauto), because an injected DLL can access GUI widgets' text
properties with much better precision and coverage than standard OS APIs like
MS UI Automation API or Win32 API. Other use cases are potentially possible at
your own risk according to the law and the project license.

It contains:

* initial DLL to be injected which is written in C,
* server-side DLL that can exchange data between injected DLL (server) and
Python process (client),
* managed DLL which is able to get text properties of WPF applications,
* DLL which is able to get text properties of Qt applications,
* Python code to initiate the injection,
* and client Python code for data exchange with an injected DLL.

Link to PyPi: <https://pypi.org/project/injectlib/>

## How to use

    python.exe example_injector.py \<Process name\> \<path to dll\> 

## Development

 1. Clone repository: <https://github.com/pywinauto/injectlib.git>
 2. Create new features or improve exiting (for the injectlib package)
 3. After work run setup.py from project root to check standalone

        python.exe setup.py bdist_wheel

## Requirements

### For use as package

* Python >= 3.7
* pywin32 >= 306

### For development purpose

* setuptools >= 65.5.1
* wheel >= 0.38.4
* .NET Framework Targeting Pack >= 4.8
* .NET Compiler Platform
* MSBuild
* MSVC >= 143
* cmake >= 3.26.3
* Qt 5 >= 5.15.2, < 6
* Qt 6

  **NOTE**: You may use PyCharm + Visual Studio with C++/C# components for
  development

### Building Qt DLL

This package compiles C++ binaries that link against **Qt 5** (>=5.15.2, <6) and **Qt 6**. Qt is not bundled and must be installed separately before building

- **Windows** download options:
  - download via [Qt Online Installer](https://www.qt.io/download) and select the MSVC components. Registration required. Only 64-bit supported. Installation might be banned for some countries.
  - via mirrors, for example, https://mirror.yandex.ru/mirrors/qt.io.
    - Base package for Qt 5.15.2 x64 can be found [here](https://mirror.yandex.ru/mirrors/qt.io/online/qtsdkrepository/windows_x86/desktop/qt5_5152/qt.qt5.5152.win64_msvc2019_64/5.15.2-0-202011130602qtbase-Windows-Windows_10-MSVC2019-Windows-Windows_10-X86_64.7z). No registration required.
    - Base package for Qt 5.15.2 x86 can be found [here](https://mirror.yandex.ru/mirrors/qt.io/online/qtsdkrepository/windows_x86/desktop/qt5_5152/qt.qt5.5152.win32_msvc2019/5.15.2-0-202011130602qtbase-Windows-Windows_10-MSVC2019-Windows-Windows_10-X86.7z). No registration required.
    - Base package for Qt 6.9.3 can be found [here](https://mirror.yandex.ru/mirrors/qt.io/online/qtsdkrepository/windows_x86/desktop/qt6_693/qt6_693/qt.qt6.693.win64_msvc2022_64/6.9.3-0-202509261208qtbase-Windows-Windows_11_23H2-MSVC2022-Windows-Windows_11_23H2-X86_64.7z)

Once installed, point CMake to your Qt installations by setting `CMAKE_PREFIX_PATH` and add the Qt runtime directories to `PATH`.

For example:

```bash
# Windows (adjust version/compiler as needed)
set CMAKE_PREFIX_PATH=C:\Qt\5.15.2\msvc2019;C:\Qt\5.15.2\msvc2019_64;C:\Qt\6.9.1\msvc2022_64
set PATH=C:\Qt\5.15.2\msvc2019\bin;C:\Qt\5.15.2\msvc2019_64\bin;C:\Qt\6.9.1\msvc2022_64\bin;%PATH%
```

If CMake still cannot find Qt, verify the paths contain `lib/cmake/Qt5` and `lib/cmake/Qt6` subdirectories.

## License

This software is open-source under the BSD 3-Clause License.

See the LICENSE file in this repository.

Dependencies are licensed by their own licenses.
