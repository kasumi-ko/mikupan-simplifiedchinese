Unicode true
!include "MUI2.nsh"
!include "nsDialogs.nsh"
!include "LogicLib.nsh"
!include "FileFunc.nsh"

!define PRODUCT "零～zero～ [MikuPan-SimplifiedChinese]"
!define PUBLISHER "kasumi-ko"
!ifdef PREVIEW_INSTALLER
  RequestExecutionLevel user
  !define APP_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\MikuPan-SimplifiedChinese-Preview"
  !define REGISTER HKCU
  !define DISPLAY "${PRODUCT} 安装测试"
  !define SHELL_CONTEXT current
  !define SHORTCUT "${PRODUCT} 安装测试.lnk"
  InstallDir "$LOCALAPPDATA\kasumi-ko\MikuPan-SimplifiedChinese-Preview"
!else
  RequestExecutionLevel admin
  !define APP_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\MikuPan-SimplifiedChinese"
  !define REGISTER HKLM
  !define DISPLAY "${PRODUCT}"
  !define SHELL_CONTEXT all
  !define SHORTCUT "${PRODUCT}.lnk"
  InstallDir "$PROGRAMFILES32\kasumi-ko\MikuPan-SimplifiedChinese"
!endif

Name "${DISPLAY}"
OutFile "${OUTPUT_FILE}"
InstallDirRegKey ${REGISTER} "${APP_KEY}" "InstallLocation"
BrandingText "MikuPan-SimplifiedChinese"
VIProductVersion "${VERSION_NUMBER}.0"
VIAddVersionKey /LANG=2052 "ProductName" "${DISPLAY}"
VIAddVersionKey /LANG=2052 "CompanyName" "${PUBLISHER}"
VIAddVersionKey /LANG=2052 "FileDescription" "${DISPLAY} 安装程序"
VIAddVersionKey /LANG=2052 "FileVersion" "${APP_VERSION}"
VIAddVersionKey /LANG=2052 "ProductVersion" "${APP_VERSION}"
VIAddVersionKey /LANG=2052 "LegalCopyright" "MikuPan contributors, kasumi-ko"
!ifdef APP_ICON
  !define MUI_ICON "${APP_ICON}"
  !define MUI_UNICON "${APP_ICON}"
!endif
SetCompressor /SOLID lzma

Var DiscIso
Var IsoText

!define MUI_WELCOMEPAGE_TITLE "安装 ${PRODUCT}"
!define MUI_WELCOMEPAGE_TEXT "此向导会安装 Windows 移植程序。请准备非高汉化团的日版 SLPS-25074 ISO，或已有的解包数据。"
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
Page custom ShowIsoPage LeaveIsoPage
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$INSTDIR\MikuPan.exe"
!define MUI_FINISHPAGE_RUN_FUNCTION CreateGameShortcut
!define MUI_FINISHPAGE_RUN_TEXT "创建桌面快捷方式"
!define MUI_FINISHPAGE_RUN_NOTCHECKED
!define MUI_FINISHPAGE_TEXT "安装完成。游戏读取安装目录中的 binary 文件夹。若安装时跳过 ISO，请将解包后的完整游戏文件放到该文件夹。"
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "SimpChinese"

Function .onInit
    SetRegView 32
    SetShellVarContext ${SHELL_CONTEXT}
    ${GetParameters} $0
    ${GetOptions} $0 "/ISO=" $DiscIso
FunctionEnd

Function ShowIsoPage
    !insertmacro MUI_HEADER_TEXT "选择游戏 ISO" "日版简中 SLPS-25074"
    nsDialogs::Create 1018
    Pop $0
    ${If} $0 == error
        Abort
    ${EndIf}
    ${NSD_CreateLabel} 0 4u 100% 24u "选择 ISO 后，游戏文件会解包到安装目录下的 binary 文件夹。"
    Pop $0
    ${NSD_CreateText} 0 36u 75% 13u "$DiscIso"
    Pop $IsoText
    ${NSD_CreateBrowseButton} 77% 35u 23% 15u "浏览…"
    Pop $0
    ${NSD_OnClick} $0 BrowseIso
    ${NSD_CreateLabel} 0 60u 100% 34u "也可以留空并继续。启动游戏前，请将解包后的完整游戏文件放到安装目录的 binary 文件夹。"
    Pop $0
    nsDialogs::Show
FunctionEnd

Function BrowseIso
    nsDialogs::SelectFileDialog open "$DiscIso" "光盘镜像 (*.iso)|*.iso"
    Pop $0
    ${If} $0 != ""
    ${AndIf} $0 != error
        ${NSD_SetText} $IsoText "$0"
    ${EndIf}
FunctionEnd

Function LeaveIsoPage
    ${NSD_GetText} $IsoText $DiscIso
    ${If} $DiscIso != ""
        IfFileExists "$DiscIso" valid
        MessageBox MB_OK|MB_ICONEXCLAMATION "找不到选择的 ISO 文件。"
        Abort
        valid:
    ${EndIf}
FunctionEnd

Section "MikuPan" Main
    !include "${INSTALL_FILES}"
    SetOutPath "$INSTDIR"
    CreateDirectory "$INSTDIR\binary"
    WriteUninstaller "$INSTDIR\uninstall.exe"
    WriteRegStr ${REGISTER} "${APP_KEY}" "DisplayName" "${DISPLAY}"
    WriteRegStr ${REGISTER} "${APP_KEY}" "DisplayVersion" "${APP_VERSION}"
    WriteRegStr ${REGISTER} "${APP_KEY}" "Publisher" "${PUBLISHER}"
    WriteRegStr ${REGISTER} "${APP_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr ${REGISTER} "${APP_KEY}" "DisplayIcon" "$INSTDIR\MikuPan.exe"
    WriteRegStr ${REGISTER} "${APP_KEY}" "UninstallString" '$\"$INSTDIR\uninstall.exe$\"'
    WriteRegDWORD ${REGISTER} "${APP_KEY}" "NoModify" 1
    WriteRegDWORD ${REGISTER} "${APP_KEY}" "NoRepair" 1

    ${If} $DiscIso != ""
        DetailPrint "正在校验并导入 ISO…"
        ClearErrors
        ExecWait '"$INSTDIR\MikuPan.exe" --import-iso "$DiscIso"' $0
        ${If} ${Errors}
        ${OrIf} $0 != 0
            SetErrorLevel 2
            IfSilent +2
            MessageBox MB_OK|MB_ICONEXCLAMATION "ISO 导入未完成。请检查日版镜像，或将解包后的游戏文件放到安装目录的 binary 文件夹。"
        ${EndIf}
    ${EndIf}
!ifndef PREVIEW_INSTALLER
    ; Allow standard users to copy extracted game data into binary.
    nsExec::ExecToLog '"$SYSDIR\icacls.exe" "$INSTDIR\binary" /grant "*S-1-5-32-545:(OI)(CI)M"'
    Pop $0
    ${If} $0 != 0
        IfSilent +2
        MessageBox MB_OK|MB_ICONEXCLAMATION "无法设置 binary 的写入权限。可以在安装时选 ISO，或使用管理员权限将游戏文件复制到 binary。"
    ${EndIf}
!endif
SectionEnd

Function CreateGameShortcut
    CreateShortCut "$DESKTOP\${SHORTCUT}" "$INSTDIR\MikuPan.exe" "" "$INSTDIR\MikuPan.exe" 0
    IfErrors done
    WriteRegDWORD ${REGISTER} "${APP_KEY}" "DesktopShortcut" 1
    done:
FunctionEnd

Function un.onInit
    SetRegView 32
    SetShellVarContext ${SHELL_CONTEXT}
FunctionEnd

Section "Uninstall"
    ReadRegStr $0 ${REGISTER} "${APP_KEY}" "InstallLocation"
    ${If} $0 == "$INSTDIR"
        ReadRegDWORD $0 ${REGISTER} "${APP_KEY}" "DesktopShortcut"
        ${If} $0 == 1
            Delete "$DESKTOP\${SHORTCUT}"
        ${EndIf}
        DeleteRegKey ${REGISTER} "${APP_KEY}"
    ${EndIf}
    !include "${UNINSTALL_FILES}"
    RMDir "$INSTDIR\binary"
    Delete "$INSTDIR\uninstall.exe"
    RMDir "$INSTDIR"
SectionEnd
