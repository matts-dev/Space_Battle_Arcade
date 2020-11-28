# Space Battle Arcade
A modifiable game for creating giant space battles.

![Visual](https://i.imgur.com/ndGVoTG.gif)

![Visual](https://imgur.com/xq0D1Kx.gif)

![Visual](https://i.imgur.com/xjBQwhA.gif)

![Visual](https://i.imgur.com/ishMFt9.gif)

![Visual](https://i.imgur.com/AWMKagm.gif)

![Visual](https://i.imgur.com/wCD2pcW.gif)


The game is a complete standalone experience for creating large battles

![Visual](https://i.imgur.com/QvbLuze.jpg)

It comes with a campaign of hand crafted levels

![Visual](https://i.imgur.com/wzuRuWd.jpg)

Complete with tools to make new levels and new space ships based on user defined models

![Visual](https://i.imgur.com/DN7lkvh.jpg)


# Building The Game

A visual studio project is provided to build the game, but microsoft does not recommend checking in files needed to set up the linker. So, I have provided a `copy.suo` file which is a copy of my `.suo` file that defines the linker settings. 
If you don't want to set up the dependencies manually, you will need to replace the .suo file with the copy.suo file; be sure to change the name from `copy.suo` to `.suo` ( the `.suo` file is found at the generated directory `s/Space_Battle_Arcade/Space_Battle_Arcade/.vs/Space_Battle_Arcade/v15/.suo`; running the visual studio .sln a single time is required for this directory to be generated)


# Modding The Game

When at the main menu, there is a section for mods. Following these menus will let you create modifications. The entire game is json. I recommend copying the base game, which itself is a mod, as a starting point for your mod.  `Space_Battle_Arcade\GameData\mods\SpaceArcade` is the base game mod.
The `GameData` folder will need to be placed wherever your .exe is located. If debugging with visual studio, the `GameData` folder will not be in the release folder or debug folder, rather it will be in a path something like this `Space_Battle_Arcade\Space_Battle_Arcade\Space_Battle_Arcade\GameData`. You'll need to copy any changes you amke here to your Relase and Debug folders if you want to run the .exe standalone while devloping a mod. I recommend building mods while running the game through visual studio, that way you will know the cause of any crashes you may encounter.
Check out the dev blog for videos specifically on modding as they may provide some help in understanding how this works. https://www.youtube.com/playlist?list=PL22CMuqloY0qiYlv1Lm_QtfwuFz9OB0NE For the most part, modding is done within the model editor, which lets you set up models, collision, teams, projectiles, etc. And the level editor, used to layout levels in a campaign. 

