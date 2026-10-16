@echo off
cd /d "%~dp0"
if not exist esptool.exe (
  echo esptool.exe not found in this folder.
  echo Download esptool-windows-amd64.zip from https://github.com/espressif/esptool/releases
  echo and extract esptool.exe next to this .bat file.
  pause
  goto :eof
)
echo ==========================================================
echo  Spool Film Processor - FULL flash (new board) v2.3
echo  WARNING: erases settings, touch calibration and custom programs!
echo ==========================================================
echo.
set /p OK=Are you sure? Type Y and press Enter: 
if /i not "%OK%"=="Y" goto :eof
set PORT=COM3
set /p PORT=COM port [Enter = COM3]: 
echo.
esptool.exe --chip esp32c3 --port %PORT% --baud 921600 write-flash 0x0 SpoolFilmProcessor_v2.3_full.bin
echo.
if errorlevel 1 (
  echo ERROR. Hold BOOT, press RESET, release BOOT and try again.
) else (
  echo DONE. After boot, tap the logo to calibrate the touch screen.
)
pause
