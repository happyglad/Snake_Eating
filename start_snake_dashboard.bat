@echo off
setlocal

cd /d "%~dp0"

set "DEFAULT_PORT=COM3"
set "DEFAULT_BAUD=115200"
set "DEFAULT_HTTP_PORT=8080"
set "HOST=127.0.0.1"

echo.
echo Snake Dashboard one-click launcher
echo ----------------------------------
echo.

where python >nul 2>nul
if not errorlevel 1 (
    set "PYTHON_CMD=python"
) else (
    where py >nul 2>nul
    if not errorlevel 1 (
        set "PYTHON_CMD=py"
    ) else (
        echo Python was not found. Please install Python and try again.
        pause
        exit /b 1
    )
)

%PYTHON_CMD% -c "import serial" >nul 2>nul
if errorlevel 1 (
    echo pyserial is missing. Installing it now...
    %PYTHON_CMD% -m pip install pyserial
    if errorlevel 1 (
        echo Failed to install pyserial. Try running: %PYTHON_CMD% -m pip install pyserial
        pause
        exit /b 1
    )
)

echo Available serial ports:
%PYTHON_CMD% -m serial.tools.list_ports
echo.

set /p SNAKE_PORT=Serial port, COM9 or 9 [%DEFAULT_PORT%]: 
if "%SNAKE_PORT%"=="" set "SNAKE_PORT=%DEFAULT_PORT%"
set "HAS_NON_DIGIT="
for /f "delims=0123456789" %%A in ("%SNAKE_PORT%") do set "HAS_NON_DIGIT=1"
if not defined HAS_NON_DIGIT set "SNAKE_PORT=COM%SNAKE_PORT%"
set "HAS_NON_DIGIT="

set /p SNAKE_BAUD=Baud rate [%DEFAULT_BAUD%]: 
if "%SNAKE_BAUD%"=="" set "SNAKE_BAUD=%DEFAULT_BAUD%"

set /p SNAKE_HTTP_PORT=Web port [%DEFAULT_HTTP_PORT%]: 
if "%SNAKE_HTTP_PORT%"=="" set "SNAKE_HTTP_PORT=%DEFAULT_HTTP_PORT%"

echo.
echo Starting dashboard...
echo Web page: http://%HOST%:%SNAKE_HTTP_PORT%
echo Serial:   %SNAKE_PORT% @ %SNAKE_BAUD%
echo.
echo Controls: arrow keys move, Enter starts/restarts, B/M switches mode, Esc quits.
echo Keep this window focused when using keyboard controls.
echo.

start "" "http://%HOST%:%SNAKE_HTTP_PORT%"
%PYTHON_CMD% ".\Tools\snake_web_dashboard.py" "%SNAKE_PORT%" --baud %SNAKE_BAUD% --host %HOST% --http-port %SNAKE_HTTP_PORT%

echo.
echo Dashboard stopped.
pause
