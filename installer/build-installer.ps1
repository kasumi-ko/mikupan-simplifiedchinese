[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$BuildDirectory,
    [Parameter(Mandatory)][string]$OutputFile,
    [string]$MakeNSIS = 'makensis.exe',
    [string]$IconFile,
    [switch]$Preview
)

$ErrorActionPreference = 'Stop'
$source = Split-Path -Parent $PSScriptRoot
$build = (Resolve-Path -LiteralPath $BuildDirectory).ProviderPath
$runtime = Join-Path $build 'MikuPan'
$output = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputFile)
$outputDirectory = Split-Path -Parent $output
New-Item -Path $outputDirectory -ItemType Directory -Force | Out-Null

$cmake = Get-Content -LiteralPath (Join-Path $source 'CMakeLists.txt') -Raw
$version = [regex]::Match($cmake, 'project\(MikuPan-SimplifiedChinese VERSION (\d+\.\d+\.\d+)')
if (!$version.Success) {
    throw 'Cannot read the MikuPan version from CMakeLists.txt.'
}
$versionNumber = $version.Groups[1].Value

# The payload comes from the built program and its runtime assets.
$payload = @(
    foreach ($name in @('MikuPan.exe', 'libarchive-LICENSE.txt')) {
        $path = Join-Path $runtime $name
        Get-Item -LiteralPath $path
    }
    foreach ($library in @('avcodec', 'avformat', 'avutil', 'swresample', 'swscale')) {
        $dll = @(Get-ChildItem -LiteralPath $runtime -Filter "$library-*.dll" -File)
        if ($dll.Count -ne 1) {
            throw "Expected one $library DLL in $runtime."
        }
        $dll[0]
    }
    Get-ChildItem -LiteralPath (Join-Path $runtime 'resources') -File -Recurse
)

$install = [Collections.Generic.List[string]]::new()
$uninstall = [Collections.Generic.List[string]]::new()
$directories = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)

function Escape-Nsis([string]$Text) {
    $Text.Replace('$', '$$')
}

foreach ($file in $payload) {
    if ($file.PSIsContainer -or ($file.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw "Unexpected installer payload $($file.FullName)."
    }
    $relative = [IO.Path]::GetRelativePath($runtime, $file.FullName)
    if ($relative.StartsWith('..\')) {
        throw "Installer payload is outside the build output: $relative."
    }
    $parent = [IO.Path]::GetDirectoryName($relative)
    $out = '$INSTDIR'
    if ($parent) {
        $out += '\' + (Escape-Nsis $parent)
    }
    $install.Add(('SetOutPath "{0}"' -f $out))
    $install.Add(('File "{0}"' -f (Escape-Nsis $file.FullName)))
    $uninstall.Add(('Delete "$INSTDIR\{0}"' -f (Escape-Nsis $relative)))
    while ($parent) {
        [void]$directories.Add($parent)
        $parent = [IO.Path]::GetDirectoryName($parent)
    }
}
$install.Add('SetOutPath "$INSTDIR"')
$install.Add(('File "{0}"' -f (Escape-Nsis (Join-Path $source 'LICENSE'))))
$uninstall.Add('Delete "$INSTDIR\LICENSE"')
foreach ($directory in ($directories | Sort-Object Length -Descending)) {
    $uninstall.Add(('RMDir "$INSTDIR\{0}"' -f (Escape-Nsis $directory)))
}

$installList = "$output.install.nsh"
$uninstallList = "$output.uninstall.nsh"
$install | Set-Content -LiteralPath $installList -Encoding utf8BOM
$uninstall | Set-Content -LiteralPath $uninstallList -Encoding utf8BOM
$arguments = @(
    '/V3',
    '/INPUTCHARSET', 'UTF8',
    "/DAPP_VERSION=$versionNumber beta",
    "/DVERSION_NUMBER=$versionNumber",
    "/DOUTPUT_FILE=$(Escape-Nsis $output)",
    "/DINSTALL_FILES=$(Escape-Nsis $installList)",
    "/DUNINSTALL_FILES=$(Escape-Nsis $uninstallList)"
)
$icon = if ($IconFile) { $IconFile } else { Join-Path $source 'extern/mikupan.ico' }
$icon = (Resolve-Path -LiteralPath $icon).ProviderPath
$arguments += "/DAPP_ICON=$(Escape-Nsis $icon)"
if ($Preview) {
    $arguments += '/DPREVIEW_INSTALLER'
}

& $MakeNSIS @arguments (Join-Path $PSScriptRoot 'mikupan.nsi')
if ($LASTEXITCODE -ne 0) {
    throw "NSIS failed with exit code $LASTEXITCODE."
}
Get-Item -LiteralPath $output
