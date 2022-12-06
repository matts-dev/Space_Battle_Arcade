# Space Battle Arcade

A modifiable game for creating giant space battles.


<img src="./readme_media/large_battle_gif1.gif" width="100%"/>

<img src="./readme_media/large_battle_gif2.gif" width="100%"/>

<img src="./readme_media/fighter_destroying_fighters.gif" width="100%"/>

<img src="./readme_media/large_battle_gif3.gif" width="100%"/>

<img src="./readme_media/large_battle_gif4.gif" width="100%"/>


<img src="./readme_media/destroy_objectives.gif" width="100%"/>

<img src="./readme_media/menu_smallest.gif" width="100%"/>


The game is a complete standalone experience for creating large battles

![Visual](https://i.imgur.com/QvbLuze.jpg)

It comes with a campaign of hand crafted levels

![Visual](https://i.imgur.com/wzuRuWd.jpg)

Complete with tools to make new levels and new space ships based on user created models

![Visual](https://i.imgur.com/DN7lkvh.jpg)

# Running The Game

I have provided a zipped game binary in `GameBinary.zip`. Just extract that and run `Space_Battle_Arcade.exe`. Alternatively you can build it from the source. Once you have done that (see below), Be sure to copy all the dll and other files in `OPEN_GL_REQUIREMENTS\dlls` and `OPEN_AL_REQUIREMENTS\lib\OpenAL` into the same folder that the game exe is in. 

# The Game Code

The game is primarily a single threaded modern opengl application. 
This game started as a learning project, so the code should not be considered best practice; some places are better written than other places.
The repository base folders contain many opengl examples.
However, the game code can be found in `Space_Battle_Arcade\Space_Battle_Arcade\Space_Battle_Arcade\new_src\Prototypes\SpaceArcade`
The primary class to start looking at is `GameBase` which is essentially the engine. All primary systems can be found in its `GameBase.h`.
The second class to consider is `SpaceArcade`, this is the game-specific subclass of GameBase. It also contains many game specific systems, such as the modding system.
Remember, this was a learning project, so there are some inconsistencies in the way things are done, mostly because I've learned many new things while working on this project. 

# Building The Game

## New Build Method
The project has been refactored to a cross platform CMake project targeting Linux, Windows, and Mac. I recommend opening the folder containing the top-level CMakeLists.txt file in your IDE (folder: `repository/Space_Battle_Arcade`). VSCode, VisualStudio, and CLion support opening CMake projects natively. Alternatively, you can build from the command line. 
```
#change directory to the directory containing the root level CMakeLists.txt
cd path/to/repository/Space_Battle_Arcade

#make a folder to build the project in
mkdir build

#change directories to our build folder
cd build

# configure the cmake project in the build folder, this generates platform specific project files 
# (like visual studio solutions, etc.).
# We pass .. to cmake, so cmake goes up a directory to find the appropriate CMakeLists.txt file.
cmake ..

# build the platform specific projects via command line in the current directory
cmake --build .
```
Note: this used to be a windows only project, and the original `.sln` is still checked in. When building the CMake project from visual studio, make sure you target the `SpaceBattleArcade.exe` and not the `SpaceBattleArcade.sln`. The `SpaceBattleArcade.sln` is the old visual studio project and appears as a build option even when opening the folder as a CMake project. I may remove this old way of building in a later commit.

## Old Build Method
A visual studio project is provided to build the game, but Microsoft does not recommend checking in files needed to set up the linker. So, I have provided a `copy.suo` file which is a copy of my `.suo` file that defines the linker settings. 
If you don't want to set up the dependencies manually, you will need to replace the .suo file with the copy.suo file; be sure to change the name from `copy.suo` to `.suo` ( the `.suo` file is found at the generated directory `s/Space_Battle_Arcade/Space_Battle_Arcade/.vs/Space_Battle_Arcade/v15/.suo`; running the visual studio .sln a single time is required for this directory to be generated)


# Modding The Game

When at the main menu, there is a section for mods. Following these menus will let you create modifications. The entire game is json. I recommend copying the base game, which itself is a mod, as a starting point for your mod.  `Space_Battle_Arcade\GameData\mods\SpaceArcade` is the base game mod.
The `GameData` folder will need to be placed wherever your .exe is located. If debugging with visual studio, the `GameData` folder will not be in the release folder or debug folder, rather it will be in a path something like this `Space_Battle_Arcade\Space_Battle_Arcade\Space_Battle_Arcade\GameData`. You'll need to copy any changes you make here to your Release and Debug folders if you want to run the .exe standalone while developing a mod. I recommend building mods while running the game through visual studio, that way you will know the cause of any crashes you may encounter.
Check out the dev blog for videos specifically on modding as they may provide some help in understanding how this works. https://www.youtube.com/playlist?list=PL22CMuqloY0qiYlv1Lm_QtfwuFz9OB0NE For the most part, modding is done within the model editor, which lets you set up models, collision, teams, projectiles, etc. And the level editor, used to layout levels in a campaign. 

# note

I've not yet chosen on a license, but know that this game is not a commercial project, and you can not sell it.