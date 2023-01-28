mkdir build
cd build
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
cmake .. -DV8NPCSERVER=TRUE -G "Visual Studio 17 2022" -A x64 -DV8_DIR="../dependencies/v8" -DV8_LIBRARY=""
nuget install v8-v142-x64 -OutputDirectory ..\dependencies
cmake .. -DV8NPCSERVER=TRUE -G "Visual Studio 17 2022" -A x64 -DV8_DIR="../dependencies/v8-v142-x64.9.1.269.9" -DV8_LIBRARY=""
pause
