@echo off
setlocal

pushd "%~dp0"
cl /nologo /std:c++17 /EHsc /O2 /Fe:BinToTxt.exe BinToTxt.cpp
set BUILD_EXIT=%ERRORLEVEL%
popd

exit /b %BUILD_EXIT%
