# Doomenstein
A 2.5D First Person Shooter based off the Doom and Wolfenstein games combined. Created using my custom game engine. Features drawing maps from images, simple ai and actor behavior, sprite animations, and more!
![Runner Banner](https://github.com/jswilkinSMU/Doomenstein/blob/main/DoomensteinHeroImg.png)

### What was Implemented:
	- Landscape and Shader changes: 
		- Stormy clouds skybox
		- New 64x64 DoomMap
			- Layout and enemy placement emulates a level.
		- Added Point lights onto plasma projectiles 
			- Gave a light color and toned down the distance so the point lights don't clash with the skybox.

	- 4 New Enemies:
		- Imp
			- Created Imp spritesheet
			- Melee like pinky demon, but faster and deals less damage. Aim was to make them the "nuisance" horde enemy.
		- Cacodemon
			- Created Cacodemon spritesheet
			- Ranged stationary enemy using CacodemonPlasma. Aim was to make them the "stationary tower" enemy.
		- Lost Soul
			- Created Lost Soul spritesheet
			- Fast moving enemy that explodes upon impacting the player and deals a large chunk of damage. Aim was to make them the "kamikaze" enemy.
		- Enemy Spawner
			- Spawns an imp every 30 seconds. Enemy type spawned and time interval is data driven and can be manipulated within the xml.

	- Game Feel: 
		- Added Objective:
			- Kill all the enemies within the level and destroy their spawners to win.
		- Player Lives:
			- Added limited player lives for a more challenging experience.
		- Victory Condition:
			- After killing all enemies, victory screen and sound plays and then returns user to Attract Mode.
		- Game Over Condition:
			- If player lives equals zero, then game over screen and sound plays and then returns user to Attract Mode.

	- Weapon changes:
		- Decreased pistol refire time.
		- New weapon - CacodemonPlasma

	- Simple dynamic combat system:
		- Only for Pistol Vs Imp
		- Includes layered height cylinders: legs, body, and head
		- Headshots deal double the original damage
		- Bodyshots deal normal damage
		- Legshots deal little damage but slow down the Imp's speed.


### Build and Use:

	1. Download and Extract the zip folder.
	2. Open the Run folder.
	3. Double-click Doomenstein_Release_x64.exe to start the program.
