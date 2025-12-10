# STM32 烧录脚本 (PowerShell)，从 build/ 下选择 bin/hex/elf 并通过 OpenOCD 烧录
# 用法: .\upload.ps1 [-File <path-without-ext>] [-Format bin|hex|elf] [-Addr <0x...>] [-Cfg <openocd.cfg>] [-OpenOCD <path>] [-VerifyOnly] [-DryRun]

param(
    [string]$File = "Src/app",
    [ValidateSet("bin","hex","elf")][string]$Format = "bin",
    [string]$Addr = "0x08000000",
    [string]$Cfg = "config/openocd/openocd_dap.cfg",
    [string]$OpenOCD = "",
    [switch]$VerifyOnly = $false,
    [switch]$DryRun = $false
)

$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$Build = Join-Path $Root "build"

function Resolve-Target {
    param([string]$Path)
    # 相对路径自动指向 build 下（不含扩展名）
    if ([IO.Path]::IsPathRooted($Path)) { return $Path }
    return (Join-Path $Build ("{0}.{1}" -f $Path, $Format))
}

$Target = Resolve-Target $File
if (-not (Test-Path $Target)) {
    Write-Error "未找到文件: $Target"
    exit 1
}

if (-not ([IO.Path]::IsPathRooted($Cfg))) {
    $Cfg = Join-Path $Root $Cfg
}

if (-not $OpenOCD) {
    $OpenOCD = (Get-Command openocd -ErrorAction SilentlyContinue)?.Source
    if (-not $OpenOCD) {
        $OpenOCD = Join-Path $env:USERPROFILE ".platformio\packages\tool-openocd\bin\openocd.exe"
    }
}
if (-not (Test-Path $OpenOCD)) {
    Write-Error "OpenOCD 不可用: $OpenOCD"
    exit 1
}
if (-not (Test-Path $Cfg)) {
    Write-Error "OpenOCD 配置不存在: $Cfg"
    exit 1
}

$cmd = @(
    $OpenOCD, "-f", $Cfg,
    "-c", "init; halt;"
)
if ($VerifyOnly) {
    $cmd += @("-c", "verify_image $Target $Addr; reset; shutdown")
} else {
    $cmd += @("-c", "flash write_image erase $Target $Addr; verify_image $Target $Addr; reset; shutdown")
}

Write-Host "OpenOCD: $OpenOCD"
Write-Host "Config : $Cfg"
Write-Host "File   : $Target"
Write-Host "Addr   : $Addr"
Write-Host "Mode   : $(if ($VerifyOnly) { 'verify' } else { 'flash+verify' })"
if ($DryRun) {
    Write-Host "Command:" ($cmd -join ' ')
    exit 0
}

& $cmd
if ($LASTEXITCODE -ne 0) {
    Write-Error "OpenOCD 失败, code=$LASTEXITCODE"
    exit $LASTEXITCODE
}
