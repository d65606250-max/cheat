@echo off
setlocal
where cl >nul 2>nul
if errorlevel 1 (
  echo Microsoft C++ compiler (cl.exe) not found.
  echo Open "x64 Native Tools Command Prompt for VS 2022" and run this file there.
  pause
  exit /b 1
)
cl /nologo /std:c++17 /EHsc /DUNICODE /D_UNICODE main.cpp /link /SUBSYSTEM:WINDOWS comdlg32.lib
if errorlevel 1 (
  echo Build failed.
  pause
  exit /b 1
)
echo.
echo Build successful: main.exe
pause
