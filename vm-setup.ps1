[CmdletBinding()]
param(
    [string]$RepositoryUrl = "https://github.com/MuraleeT/chatpad-super-driver-win10.git",
    [string]$Branch = "win10-port",
    [string]$Workspace = $PSScriptRoot,
    [switch]$InstallPrerequisites,
    [switch]$EnableTestSigning,
    [switch]$CreateTestCertificate
)

$ErrorActionPreference = "Stop"
$WdkVersion = $null
$SdkVersion = $null
$WdkContentRoot = $null
$RepoRoot = $Workspace
$SourceRoot = Join-Path $RepoRoot "source_release_0_0_4a\source_release_0_0_4a"

function Write-Step([string]$Message) {
    Write-Host "`n== $Message ==" -ForegroundColor Cyan
}

function Require-Administrator {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = [Security.Principal.WindowsPrincipal]::new($identity)
    if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        throw "Run this script from an Administrator PowerShell window."
    }
}

function Invoke-External([string]$File, [string[]]$Arguments) {
    Write-Host "$File $($Arguments -join ' ')"
    & $File @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$File failed with exit code $LASTEXITCODE."
    }
}

function Test-Command([string]$Name) {
    return $null -ne (Get-Command $Name -ErrorAction SilentlyContinue)
}

function Install-DevelopmentTools {
    Write-Step "Installing Visual Studio 2026 and Windows SDK"
    $vsInstaller = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\setup.exe"
    if (Test-Path $vsInstaller) {
        Invoke-External $vsInstaller @(
            "modify",
            "--installPath", "$env:ProgramFiles\Microsoft Visual Studio\18\Community",
            "--add", "Microsoft.VisualStudio.Workload.VCTools",
            "--includeRecommended",
            "--add", "Microsoft.VisualStudio.Component.Windows10SDK.28000",
            "--quiet",
            "--norestart"
        )
    }
    else {
        if (-not (Test-Command "winget")) {
            throw "Neither the Visual Studio Installer nor winget is available."
        }
        Invoke-External "winget" @("source", "update")
        Invoke-External "winget" @(
            "install", "--id", "Microsoft.VisualStudio.Community",
            "--exact", "--accept-source-agreements", "--accept-package-agreements",
            "--override", "--wait --passive --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended --add Microsoft.VisualStudio.Component.Windows10SDK.28000"
        )
    }

    Write-Host "The WDK must be installed separately from Microsoft because its package"
    Write-Host "version and Visual Studio integration must match the SDK."
    Write-Host "Download: https://learn.microsoft.com/windows-hardware/drivers/download-the-wdk"
    Write-Host "Install the WDK, reboot if requested, then rerun this script."
}

function Get-MsBuild {
    $vswhereCandidates = @(
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe",
        "$env:ProgramFiles\Microsoft Visual Studio\Installer\vswhere.exe"
    )
    $installationCandidates = @(
        "$env:ProgramFiles\Microsoft Visual Studio\18\Community",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\18\Community",
        "D:\Visual Studio 2026\18\Community"
    )
    $vswhere = $vswhereCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
    $installationPath = $null
    if (Test-Path $vswhere) {
        $installationPath = & $vswhere -version "[18.0,19.0)" -latest -products * -property installationPath |
            Where-Object { $_ -and (Test-Path $_.Trim()) } |
            Select-Object -First 1
        if ($installationPath) {
            $installationPath = $installationPath.Trim()
        }
    }
    if ($null -eq $installationPath -or -not (Test-Path $installationPath)) {
        $installationPath = $installationCandidates |
            Where-Object { Test-Path (Join-Path $_ "MSBuild\Current\Bin\MSBuild.exe") } |
            Select-Object -First 1
    }
    $found = if ($installationPath) {
        Join-Path $installationPath "MSBuild\Current\Bin\MSBuild.exe"
    }
    if ($null -eq $found -or -not (Test-Path $found)) {
        throw "MSBuild was not found. Install Visual Studio 2026 with the Desktop C++ workload and rerun."
    }
    $cppProps = Join-Path $installationPath "MSBuild\Microsoft\VC\v180\Microsoft.Cpp.Default.props"
    if (-not (Test-Path $cppProps)) {
        throw "Visual Studio 2026 was found at $installationPath, but Microsoft.Cpp.Default.props is missing. Install the Desktop C++ workload."
    }
    return @{
        Path = $found
        InstallationPath = $installationPath
        VcTargetsPath = (Join-Path $installationPath "MSBuild\Microsoft\VC\v180")
    }
}

function Get-WdkInfo {
    $buildRoot = "${env:ProgramFiles(x86)}\Windows Kits\10\build"
    if (-not (Test-Path $buildRoot)) {
        throw "The WDK build directory was not found: $buildRoot"
    }

    $versions = @(Get-ChildItem $buildRoot -Directory |
        Sort-Object Name -Descending |
        Where-Object {
            (Test-Path (Join-Path $_.FullName "WindowsDriver.KernelMode.props")) -and
            (Test-Path (Join-Path $_.FullName "WindowsDriver.Common.targets"))
        })
    if ($null -eq $versions -or $versions.Count -eq 0) {
        throw "No complete WDK installation was found under $buildRoot. Install the WDK with Visual Studio integration."
    }
    $libRoot = Join-Path "${env:ProgramFiles(x86)}\Windows Kits\10" "Lib"
    $sdkPrefix = $versions[0].Name -replace "\.[^.]+$", ""
    $sdkVersions = @(Get-ChildItem $libRoot -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -like "$sdkPrefix.*" } |
        Sort-Object Name -Descending)
    if ($sdkVersions.Count -eq 0) {
        $sdkVersions = @(Get-ChildItem $libRoot -Directory -ErrorAction SilentlyContinue |
            Sort-Object Name -Descending)
    }
    if ($sdkVersions.Count -eq 0) {
        throw "No Windows SDK version was found under $libRoot."
    }
    return @{
        Version = $versions[0].Name
        SdkVersion = $sdkVersions[0].Name
        ContentRoot = ("${env:ProgramFiles(x86)}\Windows Kits\10\" -replace "\\", "/")
    }
}

function Sync-Repository {
    Write-Step "Pulling repository"
    if (-not (Test-Path (Join-Path $RepoRoot ".git"))) {
        $parent = Split-Path $RepoRoot -Parent
        New-Item -ItemType Directory -Force -Path $parent | Out-Null
        Invoke-External "git" @("clone", "--branch", $Branch, $RepositoryUrl, $RepoRoot)
    }
    else {
        Push-Location $RepoRoot
        try {
            Invoke-External "git" @("fetch", "--prune", "origin")
            Invoke-External "git" @("checkout", $Branch)
            Invoke-External "git" @("pull", "--ff-only", "origin", $Branch)
        }
        finally {
            Pop-Location
        }
    }
}

function Build-Drivers {
    Write-Step "Building drivers (no installation or loading)"
    $msbuildInfo = Get-MsBuild
    $msbuild = $msbuildInfo.Path
    if ($msbuild -match "\\2022\\") {
        throw "WDK 10.0.28000 requires Visual Studio 2026, but the selected MSBuild is from Visual Studio 2022: $msbuild"
    }
    $wdk = Get-WdkInfo
    $WdkVersion = $wdk.Version
    $SdkVersion = $wdk.SdkVersion
    $WdkContentRoot = $wdk.ContentRoot
    Write-Host "Using installed WDK build folder: $WdkVersion"
    Write-Host "Using Windows SDK version: $SdkVersion"
    Write-Host "Using WDK content root: $WdkContentRoot"
    $projects = @(
        "filter\chatpad_filter.vcxproj",
        "chatpad_keyboard_kmdf\chatpad_keyboard_kmdf.vcxproj",
        "hid_keyboard_interface\chatpad_keyboard.vcxproj",
        "chatpad_mouse_kmdf\chatpad_mouse_kmdf.vcxproj",
        "hid_mouse_interface\chatpad_mouse.vcxproj"
    )
    Push-Location $SourceRoot
    try {
        foreach ($project in $projects) {
            Invoke-External $msbuild @(
                $project, "/m", "/t:Build",
                "/p:Configuration=Debug", "/p:Platform=x64",
                "/p:WDKBuildFolder=$WdkVersion",
                "/p:WindowsTargetPlatformVersion=$SdkVersion",
                "/p:WDKContentRoot=$WdkContentRoot",
                "/p:VCTargetsPath=$($msbuildInfo.VcTargetsPath)",
                "/nologo"
            )
        }
    }
    finally {
        Pop-Location
    }
}

function Enable-TestMode {
    if (-not $EnableTestSigning) {
        return
    }
    Write-Step "Enabling Windows test signing"
    Write-Warning "This changes VM boot configuration and requires a reboot. It does not install or load a driver."
    Invoke-External "bcdedit.exe" @("/set", "testsigning", "on")
    Write-Host "Reboot the VM before installing any test-signed package."
}

function New-TestCertificate {
    if (-not $CreateTestCertificate) {
        return
    }
    Write-Step "Creating a VM-only test certificate"
    $certificate = New-SelfSignedCertificate `
        -Type CodeSigningCert `
        -Subject "CN=Chatpad VM Test Certificate" `
        -CertStoreLocation "Cert:\LocalMachine\My"
    $cerPath = Join-Path $env:TEMP "chatpad-vm-test.cer"
    Export-Certificate -Cert $certificate -FilePath $cerPath | Out-Null
    Write-Host "Certificate exported to $cerPath"
    Write-Host "Trust it in the VM's Trusted Root and Trusted Publishers stores before testing."
    Write-Host "This script does not sign or install drivers."
}

Require-Administrator

if ($InstallPrerequisites) {
    Install-DevelopmentTools
}
if (-not (Test-Command "git")) {
    throw "Git is required. Install Git for Windows and rerun this script."
}
Sync-Repository
Build-Drivers
New-TestCertificate
Enable-TestMode

Write-Step "Complete"
Write-Host "Build succeeded. Outputs are under $RepoRoot\build\Debug\x64"
Write-Host "No driver was installed, registered, started, or loaded."
Write-Host "Next: snapshot the VM, then deliberately install only the filter package for testing."
