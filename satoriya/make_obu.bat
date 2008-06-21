mkdir tmp\

del /F /S /Q tmp\obu\*

mkdir tmp\obu\
copy /B /Y .\obu\Release\obu.dll tmp\obu\obu.dll
upx --best tmp\obu\obu.dll

copy /B /Y .\obu\obu_readme.txt tmp\obu\obu_readme.txt

del /F /S /Q tmp\obu.zip

chdir tmp\obu\
zip -r -9 -q ..\obu.zip *
chdir ..\..

