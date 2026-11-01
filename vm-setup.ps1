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
$ExpectedWdkVersion = "10.0.28000.0"
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
    if (-not (Test-Command "winget")) {
        throw "winget is not available. Install/update App Installer, then rerun this script."
    }

    Write-Step "Installing Visual C++ build tools and Windows SDK"
    Invoke-External "winget" @(
        "install", "--id", "Microsoft.VisualStudio.2022.BuildTools",
        "--exact", "--accept-source-agreements", "--accept-package-agreements",
        "--override", "--wait --passive --norestart --add Microsoft.VisualStudio.Workload.VCTools;includeRecommended --add Microsoft.VisualStudio.Component.Windows10SDK.26100"
    )

    Write-Host "The WDK must be installed separately from Microsoft because its package"
    Write-Host "version and Visual Studio integration must match the SDK."
    Write-Host "Download: https://learn.microsoft.com/windows-hardware/drivers/download-the-wdk"
    Write-Host "Install the WDK, reboot if requested, then rerun this script."
}

function Get-MsBuild {
    $candidates = @(
        "$env:ProgramFiles\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe",
        "$env:ProgramFiles\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
        "D:\Visual Studio 2026\18\Community\MSBuild\Current\Bin\MSBuild.exe"
    )
    $found = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
    if ($null -eq $found) {
        $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
        if (Test-Path $vswhere) {
            $installation = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
            if ($installation) {
                $found = Join-Path $installation "MSBuild\Current\Bin\MSBuild.exe"
            }
        }
    }
    if ($null -eq $found -or -not (Test-Path $found)) {
        throw "MSBuild was not found. Install Visual Studio C++ build tools and rerun."
    }
    return $found
}

function Test-Wdk {
    $wdkRoot = "${env:ProgramFiles(x86)}\Windows Kits\10\build\$ExpectedWdkVersion"
    $required = Join-Path $wdkRoot "WindowsDriver.Common.targets"
    if (-not (Test-Path $required)) {
        throw "The required WDK target is missing: $required`nInstall WDK $ExpectedWdkVersion, or update all project WDK version properties together after choosing another installed version."
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
    $msbuild = Get-MsBuild
    Test-Wdk
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
