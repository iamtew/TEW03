# TEW03 – Justfile (Projucer + MSBuild)

set windows-shell := ["powershell.exe", "-NoLogo", "-Command"]

JUCE_PATH := env_var_or_default("JUCE_PATH", env_var("USERPROFILE") + "\\JUCE")
PROJUCER  := JUCE_PATH + "\\Projucer.exe"
PROJECT   := "TEW03.jucer"
SOLUTION  := "Builds\\VisualStudio2022\\TEW03.sln"
# Bare `msbuild` is not on PATH with Build Tools alone.
# CI sets MSBUILD to the runner's Visual Studio; this default stays the local Build Tools.
MSBUILD   := env_var_or_default("MSBUILD", "C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\MSBuild\\Current\\Bin\\MSBuild.exe")

default:
    @just --list

projucer:
    {{PROJUCER}} {{PROJECT}}

resave:
    {{PROJUCER}} --resave {{PROJECT}}

# assets/logo.png is the Affinity export. Source of truth. Never write that file.
# Derived: docs/logo.png (320, linear Lanczos EWA), Builds/.../icon.ico, then wipe .res so the next build relinks it.
# `just resave` does not refresh icon.ico when the PNG changes.
docs-logo:
    magick assets/logo.png -background none -colorspace RGB -filter Lanczos +distort Resize 320x320 -colorspace sRGB PNG32:docs/logo.png

# Debug standalone must already exist. Interactive desktop only.
docs-screenshots:
    & './scripts/docs-screenshots.ps1'

icon: docs-logo
    magick assets/logo.png -background none -colorspace RGB -filter Lanczos -define icon:auto-resize=256,128,64,48,32,16 Builds\VisualStudio2022\icon.ico
    Get-ChildItem Builds\VisualStudio2022 -Recurse -Filter resources.res | Remove-Item -Force

build-debug: resave
    & '{{MSBUILD}}' {{SOLUTION}} /p:Configuration=Debug /p:Platform=x64 /m

# Overwrites the installed VST3 (same path as the MSI). Close the DAW first. UAC for Common Files.
install-debug: build-debug
    Write-Host 'Overwrites Common Files\VST3\TEW03.vst3 (the installed plugin). Close the DAW first.'; $src = (Resolve-Path -LiteralPath 'Builds\VisualStudio2022\x64\Debug\VST3\TEW03.vst3').Path; $dest = Join-Path $env:ProgramFiles 'Common Files\VST3\TEW03.vst3'; $copy = "`$ErrorActionPreference='Stop'; if (Test-Path -LiteralPath '$dest') { Remove-Item -LiteralPath '$dest' -Recurse -Force }; Copy-Item -LiteralPath '$src' -Destination '$dest' -Recurse"; $admin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator); if ($admin) { Invoke-Expression $copy } else { $enc = [Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes($copy)); $p = Start-Process -FilePath powershell.exe -Verb RunAs -Wait -PassThru -ArgumentList @('-NoProfile','-EncodedCommand',$enc); if ($null -eq $p -or $p.ExitCode -ne 0) { throw 'install-debug: elevated copy failed' } }; Write-Host "installed $dest"

build-release: resave
    & '{{MSBUILD}}' {{SOLUTION}} /p:Configuration=Release /p:Platform=x64 /m

build-dist: build-release
    if (Test-Path dist) { Remove-Item dist -Recurse -Force }
    New-Item dist -ItemType Directory | Out-Null
    $ver = ([xml](Get-Content '{{PROJECT}}')).JUCERPROJECT.version; Compress-Archive -Path 'Builds\VisualStudio2022\x64\Release\VST3\TEW03.vst3' -DestinationPath "dist\TEW03-$ver-win-x64-vst3.zip" -Force; Compress-Archive -Path 'Builds\VisualStudio2022\x64\Release\Standalone Plugin\TEW03.exe' -DestinationPath "dist\TEW03-$ver-win-x64-standalone.zip" -Force
    just build-msi

# Release binaries must already exist. First build downloads WiX.
# `dotnet` on PATH when it has an SDK, otherwise %USERPROFILE%\.dotnet.
build-msi:
    $ver = ([xml](Get-Content '{{PROJECT}}')).JUCERPROJECT.version; $root = (Get-Location).Path; $dotnet = 'dotnet'; if (-not ((& dotnet --list-sdks) 2>$null)) { $dotnet = Join-Path $env:USERPROFILE '.dotnet\dotnet.exe'; if (-not (Test-Path $dotnet)) { throw 'build-msi needs the .NET 8 SDK on PATH, or dotnet.exe in %USERPROFILE%\.dotnet' } }; New-Item -ItemType Directory -Force -Path dist | Out-Null; & $dotnet build installer\TEW03.wixproj -c Release -p:ProductVersion=$ver -p:Vst3Dir="$root\Builds\VisualStudio2022\x64\Release\VST3\TEW03.vst3" -p:StandaloneDir="$root\Builds\VisualStudio2022\x64\Release\Standalone Plugin" -p:IconDir="$root\Builds\VisualStudio2022"; if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }; $msi = Get-ChildItem installer\bin -Recurse -Filter TEW03.msi | Select-Object -First 1; Copy-Item $msi.FullName "dist\TEW03-$ver-win-x64-installer.msi" -Force

clean:
    & '{{MSBUILD}}' {{SOLUTION}} /t:Clean /p:Configuration=Debug /p:Platform=x64
    & '{{MSBUILD}}' {{SOLUTION}} /t:Clean /p:Configuration=Release /p:Platform=x64

rebuild: clean build-debug

vs:
    start {{SOLUTION}}

check:
    cmd /c "call `"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat`" >nul && cl /nologo /EHsc /std:c++17 /I src tests\seq_check.cpp /Fo:tests\seq_check.obj /Fe:tests\seq_check.exe && tests\seq_check.exe"
