# ROB RUN C++ Launcher

A native Windows launcher written in C++17 using the Win32 API.

## Features
- Select the ROB RUN executable with a Windows file picker.
- Remember the selected path in `%APPDATA%\\RobRunLauncher\\config.ini`.
- Launch the selected executable normally using `CreateProcessW`.
- Write activity logs to `%APPDATA%\\RobRunLauncher\\launcher.log`.

This is a normal launcher. It does not inject DLLs, modify game memory/files, disable anti-cheat, or bypass anti-cheat protections.

## Build automatically with GitHub Actions

1. Sign in to GitHub and create a **new empty repository**.
2. Upload all files and folders from this project, including the hidden `.github` folder.
3. Open the repository's **Actions** tab.
4. Select **Build ROB RUN Launcher**.
5. Click **Run workflow** (or push to `main` / `master` to trigger a build).
6. When the run finishes, open it and find **Artifacts** at the bottom.
7. Download `RobRunLauncher-Windows-x64`. Inside is `RobRunLauncher.exe`.

No Visual Studio installation is required on your own PC; GitHub's Windows runner compiles it.

## Build locally on Windows

Install Visual Studio 2022 with the **Desktop development with C++** workload, open **x64 Native Tools Command Prompt for VS 2022**, then run:

```bat
build_windows.bat
```
