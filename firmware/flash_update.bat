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
echo  Spool Film Processor - firmware UPDATE (v2.3)
echo  Keeps settings, touch calibration, programs and chemistry.
echo ==========================================================
echo.
echo Connect the board with a USB-C cable.
set PORT=COM3
set /p PORT=COM port [Enter = COM3]: 
echo.
esptool.exe --chip esp32c3 --port %PORT% --baud 921600 write-flash 0x10000 SpoolFilmProcessor_v2.3_update.bin
echo.
if errorlevel 1 (
  echo ERROR. Check the COM port in Device Manager. If it still fails:
  echo hold BOOT, press RESET, release BOOT and run this again.
) else (
  echo DONE. The board restarts by itself.
)
pause
