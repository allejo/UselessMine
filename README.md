# UselessMine

[![GitHub release](https://img.shields.io/github/release/allejo/UselessMine.svg)](https://github.com/allejo/UselessMine/releases/latest)
![Minimum BZFlag Version](https://img.shields.io/badge/BZFlag-v2.4.4+-blue.svg)
[![License](https://img.shields.io/github/license/allejo/UselessMine.svg)](https://github.com/allejo/UselessMine/blob/master/LICENSE.md)

A BZFlag plug-in that allows players to convert the Useless flag into mines to kill other players simply by typing '/mine' while carrying a Useless flag.

The [original plug-in](http://forums.bzflag.org/viewtopic.php?f=79&t=10340&p=103683) was written by Enigma, then there was a more optimized [rewrite released](http://forums.bzflag.org/viewtopic.php?f=79&t=17630) by sigonasr2, and lastly there is this rewrite. This implementation follows sigonasr2's implementation and is very similar in regards to how it works and the messages that are used during at deaths.

## Compiling Requirements

- BZFlag 2.4.4+
- [bztoolkit](https://github.com/allejo/bztoolkit)
- C++11

This plug-in follows [my standard instructions for compiling plug-ins](https://github.com/allejo/docs.allejo.io/wiki/BZFlag-Plug-in-Distribution).

## Usage

### Loading the plug-in

This plug-in accepts the path to a [text file](https://github.com/allejo/UselessMine/blob/master/UselessMine.deathMessages) containing death messages when loaded. If a text file is not given when loaded, no death messages will be announced when mines are exploded.

```
-loadplugin UselessMine,/path/to/UselessMine.deathMessages
```

### Custom Flags

| Name | Abbv | Description |
| ---- | :--: | ----------- |
| Bomb&nbsp;Defusal | `BD` | If carried over a mine's location, the mine will instead detonate at the owner's current location; points will be given to the player carrying the `+BD` flag. |

### Custom BZDB Variables

These custom BZDB variables must be used with `-setforced`, which sets BZDB variable `<name>` to `<value>`, even if the variable does not exist. These variables may be changed at any time in-game by using the `/set` command.

```
-setforced <name> <value>
```

| Name | Type | Default | Description |
| ---- | :--: | :-----: | ----------- |
| `_mineSafetyTime` | int | 5 | The number of seconds a player has to leave the mine detonation radius if they accidentally spawn in it. |

### Custom Slash Commands

| Command | Permission | Description |
| ------- | :--------: | ----------- |
| `/mine` | N/A | Lay a mine |
| `/reload` | setAll | Reload the death messages |
| `/reload deathmessages` | setAll | Reload the death messages |

### Custom Death Messages file

This is an optional file that will store all of the witty death messages announced when a player detonates a mine. If you would like death messages to be announced, you must give the plug-in the file.

In the file, each line is a separate death message. The supported placeholders are the following:

- `%victim%` - The player who got killed by the mine
- `%owner%` - The player who placed the mine originally
- `%minecount%` - The remaining amount of mines left on the field

The order in which you use the placeholders doesn't matter and the placeholders can be used several times in the same death message. Both `%victim%` and `%owner%` are required for each death message; `%minecount%` is optional.

## License

[MIT](https://github.com/allejo/UselessMine/blob/master/LICENSE.md)
