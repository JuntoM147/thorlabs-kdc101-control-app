#requires -Version 5.1
param([string]$QtDir)

$ErrorActionPreference = 'Stop'
if (-not [Environment]::Is64BitProcess) {
    Write-Host '[FAIL] Run the checker in 64-bit Windows PowerShell.'
    exit 1
}

function Test-BuildConfiguration {
    if (-not (Get-Command cmake.exe -ErrorAction SilentlyContinue)) {
        Write-Host '[FAIL] Install CMake 3.21 or newer and add it to PATH.'
        return $false
    }

    $tempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    $buildDir = Join-Path $tempRoot ("confo-check-" + [guid]::NewGuid())
    $arguments = @('--preset', 'app', '-B', $buildDir)
    if ($QtDir) {
        $arguments += "-DCMAKE_PREFIX_PATH=$([IO.Path]::GetFullPath($QtDir))"
    }
    Push-Location (Split-Path -Parent $PSScriptRoot)
    try {
        cmake.exe @arguments | Out-Host
        if ($LASTEXITCODE -ne 0) {
            Write-Host '[FAIL] Build configuration failed. See the CMake diagnostic above.'
            return $false
        }
        Write-Host '[OK] Hardware build configuration.'
        return $true
    } finally {
        Pop-Location
        if (Test-Path -LiteralPath $buildDir) {
            $resolved = (Resolve-Path -LiteralPath $buildDir).Path
            if ($resolved -eq $buildDir) {
                Remove-Item -LiteralPath $resolved -Recurse -Force
            }
        }
    }
}

# Probe DLL exports without opening a connection to either device.
Add-Type -Namespace DependencyCheck -Name Native -MemberDefinition @'
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern IntPtr LoadLibraryEx(string path, IntPtr file, uint flags);
    [DllImport("kernel32.dll", ExactSpelling = true, SetLastError = true)]
    public static extern IntPtr GetProcAddress(IntPtr module, string name);
    [DllImport("kernel32.dll")]
    public static extern bool FreeLibrary(IntPtr module);
'@

function Test-Runtime($Name, $Symbol) {
    $folders = @([Environment]::SystemDirectory) + ($env:Path -split ';')
    foreach ($folder in $folders) {
        if (-not $folder) { continue }
        $dll = [IO.Path]::Combine($folder.Trim('"'), $Name)
        if (-not [IO.File]::Exists($dll)) { continue }
        # LOAD_WITH_ALTERED_SEARCH_PATH includes the vendor DLL's own directory.
        $module = [DependencyCheck.Native]::LoadLibraryEx([IO.Path]::GetFullPath($dll), [IntPtr]::Zero, 8)
        if ($module -eq [IntPtr]::Zero) {
            $reason = [ComponentModel.Win32Exception]::new([Runtime.InteropServices.Marshal]::GetLastWin32Error())
            Write-Host "[FAIL] Cannot load $dll. $($reason.Message)"
            return $false
        }
        try {
            if ([DependencyCheck.Native]::GetProcAddress($module, $Symbol) -eq [IntPtr]::Zero) {
                Write-Host "[FAIL] $dll is missing $Symbol."
                return $false
            }
            Write-Host "[OK] $dll"
            return $true
        } finally {
            [void][DependencyCheck.Native]::FreeLibrary($module)
        }
    }
    Write-Host "[FAIL] Cannot find $Name. Install its x64 runtime and check PATH."
    return $false
}

$buildOk = Test-BuildConfiguration
$motorOk = Test-Runtime 'Thorlabs.MotionControl.Benchtop.StepperMotor.dll' 'SBC_Open'
$daqOk = Test-Runtime 'nicaiu.dll' 'DAQmxCreateTask'
if ($buildOk -and $motorOk -and $daqOk) { exit 0 }
exit 1
