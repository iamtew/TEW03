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

build-debug:
    & '{{MSBUILD}}' {{SOLUTION}} /p:Configuration=Debug /p:Platform=x64 /m

build-release:
    & '{{MSBUILD}}' {{SOLUTION}} /p:Configuration=Release /p:Platform=x64 /m

clean:
    & '{{MSBUILD}}' {{SOLUTION}} /t:Clean /p:Configuration=Debug /p:Platform=x64
    & '{{MSBUILD}}' {{SOLUTION}} /t:Clean /p:Configuration=Release /p:Platform=x64

rebuild: clean build-debug

vs:
    start {{SOLUTION}}
