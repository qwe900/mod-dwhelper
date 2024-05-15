# ![logo](https://raw.githubusercontent.com/azerothcore/azerothcore.github.io/master/images/logo-github.png) AzerothCore

## mod-dwhelper

### This is a module for [AzerothCore](http://www.azerothcore.org)

#### Features:

- Adds an NPC to the Entrance in the room of Dreamwalker in Icecrowncitadel 
- You can choose whoch type of healer u want to help in the Bossfight Healing the Friendly Boss

### How to install

1. Simply place the module under the `modules` folder of your AzerothCore source folder.
2. Re-run cmake and launch a clean build of AzerothCore
3. Run the Sql file into your database.
4. Ready.

### Usage

- Select the NPC choose the type of healer u want (Only the Paladin working as POC)

### Info

This module is a WIP as is not working in Heroic mode correctly as the Portal Spawns are not correctly recognized by the NPC

The healer heals himself, you and the boss if necessary and use normal paladin healing rotation.
on portal spawn it uses them and collecting the voids for heal buffs.

##Note 

AGAIN ITS A WIP and not finished
