mkdir tmp\

del /F /S /Q tmp\sodate\*

mkdir tmp\sodate\
copy /B /Y .\sodate\Release\sodate.exe tmp\sodate\sodate.exe
upx --best tmp\sodate\sodate.exe

copy /B /Y .\sodate_setup\Release\sodate_setup.exe tmp\sodate\sodate_setup.exe
upx --best tmp\sodate\sodate_setup.exe

copy /B /Y .\sodate\sodate_readme.txt tmp\sodate\sodate_readme.txt

del /F /S /Q tmp\sodate.zip

chdir tmp\sodate\
zip -r -9 -q ..\sodate.zip *
chdir ..\..

