# Microsoft Developer Studio Project File - Name="ssu" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** 編集しないでください **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ssu - Win32 Debug
!MESSAGE これは有効なﾒｲｸﾌｧｲﾙではありません。 このﾌﾟﾛｼﾞｪｸﾄをﾋﾞﾙﾄﾞするためには NMAKE を使用してください。
!MESSAGE [ﾒｲｸﾌｧｲﾙのｴｸｽﾎﾟｰﾄ] ｺﾏﾝﾄﾞを使用して実行してください
!MESSAGE 
!MESSAGE NMAKE /f "ssu.mak".
!MESSAGE 
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "ssu.mak" CFG="ssu - Win32 Debug"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "ssu - Win32 Release" ("Win32 (x86) Dynamic-Link Library" 用)
!MESSAGE "ssu - Win32 Debug" ("Win32 (x86) Dynamic-Link Library" 用)
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ssu - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_SU"
# PROP BASE Intermediate_Dir "Release_SU"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_SU"
# PROP Intermediate_Dir "Release_SU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ssu_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /Oy- /Ob2 /I "..\_" /D "_WINDOWS" /D "_USRDLL" /D "ssu_EXPORTS" /D WINVER=0x400 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "STRICT" /D "SSU_SAORI_CALL_INTERFACE" /FD /Zm800 /GA /QI0f /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wininet.lib /nologo /dll /machine:I386

!ELSEIF  "$(CFG)" == "ssu - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug_SU"
# PROP BASE Intermediate_Dir "Debug_SU"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_SU"
# PROP Intermediate_Dir "Debug_SU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ssu_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\_" /D "_WINDOWS" /D "_USRDLL" /D "ssu_EXPORTS" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "STRICT" /D "SSU_SAORI_CALL_INTERFACE" /FR /FD /Zm800 /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wininet.lib /nologo /dll /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "ssu - Win32 Release"
# Name "ssu - Win32 Debug"
# Begin Group "_"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\_\calc.cpp
# End Source File
# Begin Source File

SOURCE=..\_\calc_float.cpp
# End Source File
# Begin Source File

SOURCE=..\_\charset.cpp
# End Source File
# Begin Source File

SOURCE=..\_\mt19937ar.cpp
# End Source File
# Begin Source File

SOURCE=..\_\mt19937ar.h
# End Source File
# Begin Source File

SOURCE=..\_\random.cpp
# End Source File
# Begin Source File

SOURCE=..\_\random.h
# End Source File
# Begin Source File

SOURCE=..\_\Sender.cpp
# End Source File
# Begin Source File

SOURCE=..\_\simple_stack.h
# End Source File
# Begin Source File

SOURCE=..\_\stltool.cpp
# End Source File
# Begin Source File

SOURCE=..\_\stltool.h
# End Source File
# Begin Source File

SOURCE=..\_\Utilities.cpp
# End Source File
# Begin Source File

SOURCE=..\_\Utilities.h
# End Source File
# End Group
# Begin Group "SakuraCS"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\SakuraCS.cpp
# End Source File
# Begin Source File

SOURCE=.\SakuraCS.h
# End Source File
# Begin Source File

SOURCE=.\SakuraDLLHost.cpp
# End Source File
# Begin Source File

SOURCE=.\SakuraDLLHost.h
# End Source File
# Begin Source File

SOURCE=.\SaoriHost.cpp
# End Source File
# Begin Source File

SOURCE=.\SaoriHost.h
# End Source File
# End Group
# Begin Group "Resource"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\resource.rc
# End Source File
# End Group
# Begin Source File

SOURCE=.\ssu.cpp
# End Source File
# End Target
# End Project
