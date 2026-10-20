@echo off
rem tests/run_tests.bat - Windows test runner for Static Code Quality Analyzer
rem Runs tests/run_tests.sh via Git Bash / sh if available.

where sh >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    sh "%~dp0run_tests.sh"
    exit /b %ERRORLEVEL%
)

where bash >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    bash "%~dp0run_tests.sh"
    exit /b %ERRORLEVEL%
)

echo [NOTE] Git Bash or a POSIX shell (MSYS2/Cygwin/WSL) is required to run the full test suite on Windows.
echo Please run: sh tests/run_tests.sh
exit /b 1
