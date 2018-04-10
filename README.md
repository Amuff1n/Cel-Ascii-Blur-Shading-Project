# Cel-Ascii-Blur-Shading-Project
Currently opens up a black window using SDL2.

## Mac 
1. Download SDL2 Development Library for Mac OSX from https://www.libsdl.org/download-2.0.php#source
2. Install using the downloaded SDL2 .dmg
3. Compile code using
`make -f MacMakefile`
4. Run program using
`./CAB`

## Windows
1. Download SDL2 Development Library for Windows for Visual C++ from https://www.libsdl.org/download-2.0.php#source
2. Create a directory in Cel-Ascii-Blur-Shading-Project/ called "lib" (without quotes)
3. Unzip the SDL2 folder into lib/
4. Open up Visual Studio (assuming VS 2017)
5. Navigate to File -> New -> Project from Existing Code
6. Select "Visual C++"
7. Select Cel-Ascii-Blur-Shading-Project/ as the Project File Location and click Finish
8. Open the project, select a file in the solution explorer, and navigate to Project -> "project name" Properties on the top menu bar of Visual Studio
9. Under VC++ Directories, edit Include Directories and add the FULL path for the "include" directory located in the SDL folder in the lib directory you made earlier
10. Under VC++ Directories, edit Library Directories and add the FULL path for the "x86" directory located in the lib folder in the SDL folder which is in your own lib folder you made earlier
11. Under Linker -> Input -> Additional Dependencies add the following in the textbox:
```
OpenGL32.lib
SDL2.lib
SDL2main.lib
```
12. Under Linker -> System -> SubSystem select "Console"
13. Make sure to hit Apply!
14. Copy "Cel-Ascii-Blur-Shading-Project\lib\SDL2-2.0.8\lib\x86\SDL2.dll" (ie. copy the dll file in that location) to "C:\Windows\SysWOW64" and "C:\Windows\SysWOW64"
15. You can now compile and run the program through Visual Studio by going to Debug -> Start Without Debugging
16. If you get an error asking you to retarget your solution, simply select Project -> Retarget solution and select the latest Windows SDK.

## Linux