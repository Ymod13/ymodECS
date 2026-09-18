@echo off
cd /d "%~dp0"
set PATH=%~dp0bin;%PATH%
start "" "%~dp0ymodECS.exe"