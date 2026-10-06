# TEW03 – Justfile (Projucer + MSBuild)

set windows-shell := ["powershell.exe", "-NoLogo", "-Command"]

JUCE_PATH := env_var_or_default("JUCE_PATH", env_var("USERPROFILE") + "\\JUCE")
PROJUCER  := JUCE_PATH + "\\Projucer.exe"
PROJECT   := "TEW03.jucer"
SOLUTION  := "Builds\\VisualStudio2022\\TEW03.sln"
# Bare `msbuild` is not on PATH with Build Tools alone.
MSBUILD   := "C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\MSBuild\\Current\\Bin\\MSBuild.exe"

default:
    @just --list

projucer:
    {{PROJUCER}} {{PROJECT}}

resave:
    {{PROJUCER}} --resave {{PROJECT}}

# PNG → Builds/.../icon.ico, then wipe compiled .res so the next build relinks it.
# `just resave` does not refresh icon.ico when the PNG changes.
icon:
    magick assets/icon.png -define icon:auto-resize=256,128,64,48,32,16 Builds\VisualStudio2022\icon.ico
    Get-ChildItem Builds\VisualStudio2022 -Recurse -Filter resources.res | Remove-Item -Force

build-debug:
    & '{{MSBUILD}}' {{SOLUTION}} /p:Configuration=Debug /p:Platform=x64 /m

build-release:
    & '{{MSBUILD}}' {{SOLUTION}} /p:Configuration=Release /p:Platform=x64 /m

build-dist: build-release
    if (Test-Path dist) { Remove-Item dist -Recurse -Force }
    New-Item dist -ItemType Directory | Out-Null
    Copy-Item -Recurse 'Builds\VisualStudio2022\x64\Release\VST3\TEW03.vst3' dist
    New-Item dist\Standalone -ItemType Directory | Out-Null
    Copy-Item 'Builds\VisualStudio2022\x64\Release\Standalone Plugin\TEW03.exe' dist\Standalone
    $ver = ([xml](Get-Content '{{PROJECT}}')).JUCERPROJECT.version; Compress-Archive -Path dist\TEW03.vst3, dist\Standalone -DestinationPath "dist\TEW03-$ver-win-x64.zip" -Force

clean:
    & '{{MSBUILD}}' {{SOLUTION}} /t:Clean /p:Configuration=Debug /p:Platform=x64
    & '{{MSBUILD}}' {{SOLUTION}} /t:Clean /p:Configuration=Release /p:Platform=x64

rebuild: clean build-debug

vs:
    start {{SOLUTION}}

check:
    cmd /c "call `"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat`" >nul && cl /nologo /EHsc /std:c++17 /I src tests\seq_check.cpp /Fo:tests\seq_check.obj /Fe:tests\seq_check.exe && tests\seq_check.exe"
