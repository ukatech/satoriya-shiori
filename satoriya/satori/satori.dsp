# Microsoft Developer Studio Project File - Name="satori" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** 編集しないでください **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=satori - Win32 Debug
!MESSAGE これは有効なﾒｲｸﾌｧｲﾙではありません。 このﾌﾟﾛｼﾞｪｸﾄをﾋﾞﾙﾄﾞするためには NMAKE を使用してください。
!MESSAGE [ﾒｲｸﾌｧｲﾙのｴｸｽﾎﾟｰﾄ] ｺﾏﾝﾄﾞを使用して実行してください
!MESSAGE 
!MESSAGE NMAKE /f "satori.mak".
!MESSAGE 
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "satori.mak" CFG="satori - Win32 Debug"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "satori - Win32 Release" ("Win32 (x86) Dynamic-Link Library" 用)
!MESSAGE "satori - Win32 Debug" ("Win32 (x86) Dynamic-Link Library" 用)
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "satori - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SATORI_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /Oy- /Ob2 /I "..\_" /D "_WINDOWS" /D "_USRDLL" /D "SATORI_DLL" /D WINVER=0x400 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "STRICT" /FD /Zm800 /GA /QI0f /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 user32.lib /nologo /dll /machine:I386
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "satori - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SATORI_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\_" /D "_WINDOWS" /D "_USRDLL" /D "SATORI_DLL" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "STRICT" /FR /FD /Zm800 /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 user32.lib /nologo /dll /debug /machine:I386 /out:"D:\ssp_src_set\ssp\ghost\1st-innerport\ghost\master\satori.dll" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "satori - Win32 Release"
# Name "satori - Win32 Debug"
# Begin Group "Satori"

# PROP Default_Filter ""
# Begin Group "Base"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\satori.cpp
# End Source File
# Begin Source File

SOURCE=.\satori.h
# End Source File
# End Group
# Begin Group "phase0"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\satori_AnalyzeRequest.cpp
# End Source File
# Begin Source File

SOURCE=.\satori_load_dict.cpp
# End Source File
# Begin Source File

SOURCE=.\satori_load_dict.h
# End Source File
# Begin Source File

SOURCE=.\satori_load_unload.cpp
# End Source File
# End Group
# Begin Group "Phase1"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\satori_CreateResponce.cpp
# End Source File
# Begin Source File

SOURCE=.\satori_EventOperation.cpp
# End Source File
# End Group
# Begin Group "Phase2"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\satori_Kakko.cpp
# End Source File
# Begin Source File

SOURCE=.\satori_sentence.cpp
# End Source File
# Begin Source File

SOURCE=.\satori_tool.cpp
# End Source File
# Begin Source File

SOURCE=.\satoriFMO.cpp
# End Source File
# End Group
# Begin Group "phase3"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\satoriTranslate.cpp
# End Source File
# End Group
# End Group
# Begin Group "Independent"

# PROP Default_Filter ""
# Begin Group "Structure"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Families.h
# End Source File
# Begin Source File

SOURCE=.\Family.h
# End Source File
# Begin Source File

SOURCE=.\OverlapController.h
# End Source File
# Begin Source File

SOURCE=.\Selector.h
# End Source File
# End Group
# Begin Group "SakuraCS"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\console_application.h
# End Source File
# Begin Source File

SOURCE=.\SakuraClient.cpp
# End Source File
# Begin Source File

SOURCE=.\SakuraClient.h
# End Source File
# Begin Source File

SOURCE=.\SakuraCS.cpp
# End Source File
# Begin Source File

SOURCE=.\SakuraCS.h
# End Source File
# Begin Source File

SOURCE=.\SakuraDLLClient.cpp
# End Source File
# Begin Source File

SOURCE=.\SakuraDLLClient.h
# End Source File
# Begin Source File

SOURCE=.\SakuraDLLHost.cpp
# End Source File
# Begin Source File

SOURCE=.\SakuraDLLHost.h
# End Source File
# Begin Source File

SOURCE=.\SakuraFMO.cpp
# End Source File
# Begin Source File

SOURCE=.\SaoriClient.cpp
# End Source File
# Begin Source File

SOURCE=.\SaoriClient.h
# End Source File
# Begin Source File

SOURCE=.\shiori_plugin.cpp
# End Source File
# Begin Source File

SOURCE=.\shiori_plugin.h
# End Source File
# Begin Source File

SOURCE=.\ShioriClient.h
# End Source File
# End Group
# End Group
# Begin Group "_"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\_\calc.cpp
# End Source File
# Begin Source File

SOURCE=..\_\FMO.h
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

SOURCE=..\_\Sender.h
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
# Begin Source File

SOURCE=.\index.html
# End Source File
# Begin Source File

SOURCE=..\satori_license.txt
# End Source File
# End Target
# End Project
