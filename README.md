# UselessMine

[![GitHub release](https://img.shields.io/github/release/allejo/UselessMine.svg)](https://github.com/allejo/UselessMine/releases/latest)
![Minimum BZFlag Version](https://img.shields.io/badge/BZFlag-v2.4.14+-blue.svg)
[![License](https://img.shields.io/github/license/allejo/UselessMine.svg)](/LICENSE.md)

A BZFlag plug-in that allows players to convert the Useless flag into mines to kill other players simply by typing '/mine' while carrying a Useless flag.

The [original plug-in](http://forums.bzflag.org/viewtopic.php?f=79&t=10340&p=103683) was written by Enigma, then there was a more optimized [rewrite released](http://forums.bzflag.org/viewtopic.php?f=79&t=17630) by sigonasr2, and lastly there is this rewrite. This implementation follows sigonasr2's implementation and is very similar in regards to how it works and the messages that are used during at deaths.

## Compiling Requirements

- BZFlag 2.4.14+
- C++11

This plug-in follows [my standard instructions for compiling plug-ins](https://github.com/allejo/docs.allejo.io/wiki/BZFlag-Plug-in-Distribution).

## Usage

### Loading the plug-in

This plug-in accepts the path to two text files containing [death](/UselessMine.deathMessages) and [defusal](/UselessMine.defuseMessages) messages, respectively, when loaded.

Order matters when you're loading the plug-in, it must be death messages before defuse messages. If you'd like to skip death messages and only have defuseMessages, you may use the keyword `NULL` in place of a death messages file.

```
-loadplugin UselessMine.so[,UselessMine.deathMessages][,UselessMine.defuseMessages]
```

### Custom Flags

| Name              | Abbv | Description                              |
| ----------------- | :--: | ---------------------------------------- |
| Bomb&nbsp;Defusal | `BD` | If carried over a mine's location, the mine will instead detonate at the owner's current location; points will be given to the player carrying the `+BD` flag. |

### Custom BZDB Variables

These custom BZDB variables can be configured with `-set` in configuration files and may be changed at any time in-game by using the `/set` command.

```
-set <name> <value>
```

| Name              | Type | Default | Description                              |
| ----------------- | :--: | :-----: | ---------------------------------------- |
| `_mineSafetyTime` | int  |    5    | The number of seconds a player has to leave the mine detonation radius if they accidentally spawn in it. |

> **Note**
>
> Beginning with version **1.2.0** of the plug-in, the use of `-setforced` is no longer required; in fact, it's now discouraged.

### Custom Slash Commands

| Command                   | Permission | Description                              |
| ------------------------- | :--------: | ---------------------------------------- |
| `/mine`                   |    N/A     | Lay a mine                               |
| `/minestats`              |    N/A     | Display the number of mines each player has on the field |
| `/minecount`              |    N/A     | Display the total number of mines on the field |
| `/reload`                 |   setAll   | Reload all messages                      |
| `/reload deathmessages`   |   setAll   | Reload the death messages                |
| `/reload defusalmessages` |   setAll   | Reload the defusal messages              |

### Custom Death/Defusal Messages file

These are optional files that will store all of the witty death/defusal messages announced when a player detonates or defuses a mine. If you would like death/defusal messages to be announced, you must give the plug-in the file.

In the file, each line is a separate death or defusal message. The supported placeholders are the following:

- `%victim%` - The player who got killed by the mine; only available in death messages
- `%owner%` - The player who placed the mine originally
- `%defuser%` - The player who defused the mine; only available in defusal messages
- `%minecount%` - The remaining amount of mines left on the field

The order in which you use the placeholders doesn't matter and the placeholders can be used several times in the same death message.

## License

[MIT](/LICENSE.md)
