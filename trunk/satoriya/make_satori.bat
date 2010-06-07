mkdir tmp\

del /F /S /Q tmp\satori\*

mkdir tmp\satori\
copy /B /Y .\satori\Release\satori.dll tmp\satori\satori.dll
upx --best tmp\satori\satori.dll
copy /B /Y .\satori\Release\satorite.exe tmp\satori\satorite.exe
upx --best tmp\satori\satorite.exe

mkdir tmp\satori\saori\
copy /B /Y .\satori\Release\ssu.dll tmp\satori\saori\ssu.dll
upx --best tmp\satori\saori\ssu.dll

copy /B /Y .\satori\satori_license.txt tmp\satori\satori_license.txt

del /F /S /Q tmp\satori.zip

chdir tmp\satori\
7z a -y -tzip -mx=9 -mmt=on "..\satori.zip" *
chdir ..\..

