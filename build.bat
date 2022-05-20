@echo off

set ObjOutDir=build\obj\
set BinOutDir=build\
set OutName=scan

set LibClangIncludeDir=ext/libclang/include
set LibClangLibraries=ext/libclang/lib/clang_static_bundled.lib

IF NOT EXIST %BinOutDir% mkdir %BinOutDir%
IF NOT EXIST %ObjOutDir% mkdir %ObjOutDir%

ctime -begin ctime.ctm

set CommonDefines=/D_CRT_SECURE_NO_WARNINGS /D_CINDEX_LIB_
set CommonCompilerFlags=/Gm- /fp:fast /GR- /EHsc /nologo /diagnostics:classic /WX /std:c++17 /Zi /Od /GL /MT /Fd%BinOutDir% /I%LibClangIncludeDir%
set CommonLinkerFlags=/INCREMENTAL:NO /OPT:REF /MACHINE:X64 /OUT:%BinOutDir%\%OutName%.exe /PDB:%BinOutDir%\%OutName%.pdb %LibClangLibraries% version.lib

cl /MP /W3 /Fo%ObjOutDir% %CommonDefines% /I%ClReflectIncludeDirectory% %CommonCompilerFlags% src/Main.cpp /link %CommonLinkerFlags%

cl /MP /W3 /Fo%ObjOutDir% %CommonCompilerFlags% src/Metaprogram.cpp /link /INCREMENTAL:NO /OPT:REF /MACHINE:X64 /DLL /EXPORT:Metaprogram /OUT:%BinOutDir%\Metaprogram.dll /PDB:%BinOutDir%\Metaprogram.pdb

rmdir /S /Q %ObjOutDir%
del build\%OutName%.exp >NUL 2>&1
del build\%OutName%.lib >NUL 2>&1

ctime -end ctime.ctm
