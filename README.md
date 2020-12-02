# Space Battle Arcade

A modifiable game for creating giant space battles.


![Visual](./readme_media/large_battle_gif1.gif)

![Visual](./readme_media/large_battle_gif2.gif)

![Visual](./readme_media/fighter_destroying_fighters.gif)

![Visual](./readme_media/large_battle_gif3.gif)

![Visual](./readme_media/large_battle_gif4.gif)

![Visual](./readme_media/destroy_objectives.gif)

![Visual](./readme_media/menu_smallest.gif)


The game is a complete standalone experience for creating large battles

![Visual](https://i.imgur.com/QvbLuze.jpg)

It comes with a campaign of hand crafted levels

![Visual](https://i.imgur.com/wzuRuWd.jpg)

Complete with tools to make new levels and new space ships based on user defined models

![Visual](https://i.imgur.com/DN7lkvh.jpg)

# Running The Game

I have provided a zipped game binary in `GameBinary.zip`. Just extract that and run `Space_Battle_Arcade.exe`. Alternativel you can build it from the source. Once you have done that (see below), Be sure to copy all the dll and other files in `OPEN_GL_REQUIREMENTS\dlls` and `OPEN_AL_REQUIREMENTS\lib\OpenAL` into the same folder that the game exe is in. 

# The Game Code

The game is primarily a single threaded modern opengl application. 
This game started as a learning project, so the code should not be considered best practice; some places are better written than other places.
The repository base folders contain many opengl examples.
However, the game code can be found in `Space_Battle_Arcade\Space_Battle_Arcade\Space_Battle_Arcade\new_src\Prototypes\SpaceArcade`
The primary class to start looking at is `GameBase` which is essentially the engine. All primary systems can be found in its `GameBase.h`.
The second class to consider is `SpaceArcade`, this is the game-specific subclass of GameBase. It also contains many game specific systems, such as the modding system.
Remember, this was a learning project, so there are some inconsistencies in the way things are done, mostly because I've learned many new things while working on this project. 

# Building The Game

A visual studio project is provided to build the game, but microsoft does not recommend checking in files needed to set up the linker. So, I have provided a `copy.suo` file which is a copy of my `.suo` file that defines the linker settings. 
If you don't want to set up the dependencies manually, you will need to replace the .suo file with the copy.suo file; be sure to change the name from `copy.suo` to `.suo` ( the `.suo` file is found at the generated directory `s/Space_Battle_Arcade/Space_Battle_Arcade/.vs/Space_Battle_Arcade/v15/.suo`; running the visual studio .sln a single time is required for this directory to be generated)


# Modding The Game

When at the main menu, there is a section for mods. Following these menus will let you create modifications. The entire game is json. I recommend copying the base game, which itself is a mod, as a starting point for your mod.  `Space_Battle_Arcade\GameData\mods\SpaceArcade` is the base game mod.
The `GameData` folder will need to be placed wherever your .exe is located. If debugging with visual studio, the `GameData` folder will not be in the release folder or debug folder, rather it will be in a path something like this `Space_Battle_Arcade\Space_Battle_Arcade\Space_Battle_Arcade\GameData`. You'll need to copy any changes you amke here to your Relase and Debug folders if you want to run the .exe standalone while devloping a mod. I recommend building mods while running the game through visual studio, that way you will know the cause of any crashes you may encounter.
Check out the dev blog for videos specifically on modding as they may provide some help in understanding how this works. https://www.youtube.com/playlist?list=PL22CMuqloY0qiYlv1Lm_QtfwuFz9OB0NE For the most part, modding is done within the model editor, which lets you set up models, collision, teams, projectiles, etc. And the level editor, used to layout levels in a campaign. 

# note

I"ve not chosen on a license yet, but know that this game is not a commerical project, and you can not sell it.