# Qt-Based C++ Calculator

A fully functional desktop calculator application built with **C++17** and the **Qt6 GUI framework**. The project features a clean user interface and supports standard arithmetic operations along with advanced mathematical calculations.

## Features
* **Standard Arithmetic:** Addition, subtraction, multiplication, and division.
* **Advanced Math:** Support for square roots, Pi (π), fractions, and complex calculations.
* **Full Unicode Support:** Clean rendering of mathematical symbols (`×`, `÷`, `√`, `π`) across different operating systems.
* **Cross-Platform Build System:** Configured entirely via CMake for easy setup and compilation.

---

## Technical Stack & Requirements

To build and run this project from the source code, you will need the following components installed on your system:

* **C++ Compiler:** Any compiler supporting **C++17** standard or higher (e.g., MSVC 2022, GCC, or Clang).
* **CMake:** Version **3.16** or higher.
* **Qt Framework:** **Qt6 (specifically Qt6::Widgets module)** must be installed on your computer.

---

## How to Build and Run

### Option 1: Via Visual Studio (Recommended for Windows)
1. Ensure you have the **Qt Visual Studio Tools** extension installed, or your `CMAKE_PREFIX_PATH` env variable points to your Qt6 installation folder.
2. Launch Visual Studio and choose **"Open a local folder"**.
3. Select the root directory containing the `CMakeLists.txt` file.
4. Wait for Visual Studio to finish the CMake cache generation.
5. In the top toolbar, select **`QtCalculator.exe`** as the startup target.
6. Press **Ctrl + F5** to compile and launch the application.

### Option 2: Via Command Line (CMake)
If you prefer building outside of an IDE, run the following commands in your terminal from the root folder:

```bash
# Create a build directory and navigate into it
mkdir build && cd build

# Configure the project (replace the path with your actual Qt6 installation path if needed)
cmake -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2022_64" ..

# Build the executable
cmake --build . --config Release
```

---

## Deployment Note (For End Users)
If you want to share the compiled `.exe` file with someone who **does not have Qt installed**, make sure to run the `windeployqt` utility on your Release binary to bundle all necessary `.dll` dependencies into the application folder before archiving it into a ZIP file.
parser
expr   := term (('+'|'−') term)*
term   := unary (('×'|'÷'|implicit multiplication) unary)*
unary  := ('−'|'+') unary | power
power  := postfix ('^' unary)?
postfix:= frac ('!' | '%')*
frac   := primary ('⁄' primary)*          <- fraction bar, binds tighter than ×÷
primary:= number | '(' expr ')' | π | e | √ frac | func '(' expr ')' 
