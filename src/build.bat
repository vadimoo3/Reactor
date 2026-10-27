@echo off
rem Build ReactorLab.exe: C++ engine (MSVC) embedded into WPF app (Roslyn csc, .NET Framework 4.7.1+).
rem Usage: build.bat [output exe]   (default ..\ReactorLab.exe)
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
cd /d "%~dp0"
set OUT=%~1
if "%OUT%"=="" set OUT=..\ReactorLab.exe
if not exist obj mkdir obj
cl /nologo /O2 /EHsc /std:c++17 /utf-8 /MT /W3 /wd4244 /wd4267 /wd4018 /wd4305 /wd4996 /Iengine /Foobj\ engine\tools.cpp engine\engine.cpp /Feobj\ReactorEngine.exe /link /SUBSYSTEM:CONSOLE /MANIFEST:EMBED /MANIFESTINPUT:engine\engine.manifest || exit /b 1
set REF=C:\Program Files (x86)\Reference Assemblies\Microsoft\Framework\.NETFramework\v4.7.1
"C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\MSBuild\Current\Bin\Roslyn\csc.exe" /nologo /noconfig /nostdlib+ /target:winexe /optimize+ /langversion:latest /out:"%OUT%" ^
 /lib:"%REF%" /r:mscorlib.dll /r:System.dll /r:System.Core.dll /r:System.Xaml.dll /r:System.Xml.dll /r:WindowsBase.dll /r:PresentationCore.dll /r:PresentationFramework.dll ^
 /resource:obj\ReactorEngine.exe,ReactorEngine.exe /resource:app\Theme.xaml,Theme.xaml /recurse:app\*.cs || exit /b 1
rmdir /s /q obj
