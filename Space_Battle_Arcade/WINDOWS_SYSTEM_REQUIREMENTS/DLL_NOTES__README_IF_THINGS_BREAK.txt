this game aims to not require installation of microsoft redist(tributable)

Some libraries were built (glfw) in debug configuration.

this means it uses dlls not included in the redist install (a user may already have this installed)

I've copied dll files over, that do not come with redist. 

it is important you copy the 32 bit versions of these dlls, system32 will contain these dll files but on my machine these are 64 bit dlls
    trying to load 64bit dll when 32bit application this will probably result in some strange errors, like `The application was unable to start correctly (0xc000007b)`
    see https://stackoverflow.com/questions/10492037/the-application-was-unable-to-start-correctly-0xc000007b

their current file paths are:
vcruntime140d.dll
    C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Redist\MSVC\14.16.27012\onecore\debug_nonredist\x86\Microsoft.VC141.DebugCRT
msvcp140d.dll
    C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Redist\MSVC\14.16.27012\onecore\debug_nonredist\x86\Microsoft.VC141.DebugCRT
ucrtbased.dll
    C:\Program Files (x86)\Windows Kits\10\bin\10.0.17134.0\x86\ucrt
    NOTE: it is probably improtant you match the windows SDK version, eg 10.0.17134.0, in that path. this means these dlls will change if compiled on another computer!

note: you probably need to make sure all libraries are compiled with the same computer and same windows sdk version!