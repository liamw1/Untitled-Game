@echo off
pushd %~dp0\..\
call lib\bin\premake\premake5.exe vs2019
popd
PAUSE