@echo off
setlocal

pushd "%~dp0"
g++ -std=c++17 -O2 -Wall -Wextra -o BinToTxt.exe BinToTxt.cpp
set BUILD_EXIT=%ERRORLEVEL%
popd

exit /b %BUILD_EXIT%
