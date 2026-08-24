@echo off
setlocal

cmake -S . -B build
if errorlevel 1 exit /b %errorlevel%
cmake --build build --config Release
if errorlevel 1 exit /b %errorlevel%

if exist build\Release\csi.exe (
	echo Built binary: %cd%\build\Release\csi.exe
) else (
	echo Built binary: %cd%\build\csi.exe
)
