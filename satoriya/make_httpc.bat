mkdir tmp\

del /F /S /Q tmp\httpc\*

mkdir tmp\httpc\
copy /B /Y .\httpc\Release\httpc.dll tmp\httpc\httpc.dll
upx --best tmp\httpc\httpc.dll

copy /B /Y .\httpc\httpc.txt tmp\httpc\httpc_readme.txt

del /F /S /Q tmp\httpc.zip

chdir tmp\httpc\
zip -r -9 -q ..\httpc.zip *
chdir ..\..

