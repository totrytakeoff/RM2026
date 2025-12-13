# =============================================================================
# @file script/upload.ps1
# @brief STM32 OpenOCD PowerShell 烧录脚本
# @project RM2026
# @author YZ-Control/myself
# @version 1.0.0
# @date 2025-12-07
# @details 供 Windows 环境选择 build 目录下的 bin/hex/elf，并以 OpenOCD 执行烧录或校验，可配置地址与验证模式。
# =============================================================================

param(
    [Alias("Name")][string]$File = "app",
    [ValidateSet("bin","hex","elf")][string]$Format = "bin",
    [string]$Addr = "0x08000000",
    [string]$Cfg = "config/openocd/openocd_dap.cfg",
    [string]$OpenOCD = "",
    [switch]$VerifyOnly = $false,
    [switch]$DryRun = $false
)

$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$Build = Join-Path $Root "build"
$OutputDir = Join-Path $Build "output"

# 示例：
#   .\upload.ps1                       # 上传 build\output\app.bin
#   .\upload.ps1 test_motor --format elf
#   .\upload.ps1 test_remote_control_demo --verifyOnly
#   .\upload.ps1 C:\tmp\custom.elf     # 绝对路径会保留扩展名

function Resolve-Target {
    param([string]$Path)
    $expanded = $Path
    if ($expanded -eq "~") { $expanded = $HOME }
    elseif ($expanded.StartsWith("~\")) { $expanded = Join-Path $HOME $expanded.Substring(2) }

    $hasExt = [IO.Path]::HasExtension($expanded)

    if ([IO.Path]::IsPathRooted($expanded)) {
        if ($hasExt) { return $expanded }
        return ("{0}.{1}" -f $expanded, $Format)
    }

    if ($hasExt) { return (Join-Path $OutputDir $expanded) }
    return (Join-Path $OutputDir ("{0}.{1}" -f $expanded, $Format))
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
