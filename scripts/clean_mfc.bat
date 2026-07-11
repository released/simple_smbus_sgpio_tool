@echo off
setlocal

if /I "%~1"=="/?" goto usage
if /I "%~1"=="-h" goto usage
if /I "%~1"=="--help" goto usage

pushd "%~dp0\.."

if not exist build mkdir build

if exist build\obj rd /s /q build\obj
del /q build\SmbusSgpioTool.lib >nul 2>nul
del /q build\SmbusSgpioTool.exp >nul 2>nul
del /q build\*.pdb >nul 2>nul
del /q build\*.ilk >nul 2>nul
del /q build\*.idb >nul 2>nul
del /q build\*.iobj >nul 2>nul
del /q build\*.ipdb >nul 2>nul

echo Clean complete. Preserved build\SmbusSgpioTool.exe, build\smbus_sgpio_tool.ini, and build\test_log.

popd
endlocal
goto eof

:usage
echo Usage: scripts\clean_mfc.bat
echo Removes PC build intermediates and debug artifacts.
echo Preserves build\SmbusSgpioTool.exe, build\smbus_sgpio_tool.ini, and build\test_log.
endlocal

:eof

