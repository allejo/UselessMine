## 1.1.0

**New**

- New "Bomb Defusal" flag allows players to defuse mines
- New `/minestats` command shows the amount of mines each player has
- New `/minecount` command shows the total number of mines on the field

**Changes**

- The `/reload` command by itself, which is meant to reload everything, will now reload death messages too
- Mines now contain a unique ID available at debug level 4 used intended for help with debugging

**Fixes**

- Inaccurate mine removal has been rectified; thanks a lot to [@asinck](https://github.com/asinck) for figuring out the cause
- Issues with conflicting plug-in Name() methods due to bzToolkit has been fixed

## 1.0.2

**New**

- New death messages and some corrections, courtesy of [@asinck](https://github.com/asinck)
- Allow to reload death messages via `/reload deathmessages`

**Changes**

- License changed to MIT

**Fixes**

- Changes to `_mineSafetyTime` in-game are now respected

## 1.0.1

**New**

- Support the OpenFFA game mode

**Fixes**

- Mines are now less sensitive, which fixes incorrect kill assignments

## 1.0.0

Initial release
