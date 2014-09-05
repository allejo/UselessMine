UselessMine
===========

A BZFlag plug-in that allows players to convert the Useless flag into mines to kill other players simply by typing '/mine' while carrying a Useless flag.

The [original plug-in](http://forums.bzflag.org/viewtopic.php?f=79&t=10340&p=103683) was written by Enigma, then there was a more optimized [rewrite released](http://forums.bzflag.org/viewtopic.php?f=79&t=17630) by sigonasr2, and lastly there is this rewrite. This implementation follows sigonasr2's implementation and is very similar in regards to how it works and the messages that are used during at deaths.

Author
-------

- Vladimir "allejo" Jimenez

Compiling
---------

### Requirements

- BZFlag 2.4.0+

- [bzToolkit](https://github.com/allejo/bztoolkit/)

### How To Compile

1.  Check out the BZFlag source code.

    `git clone -b v2_4_x https://github.com/BZFlag-Dev/bzflag-import-3.git bzflag`

2.  Go into the newly checked out source code and then the plugins directory.

    `cd bzflag/plugins`

3.  Run a git clone of this repository from within the plugins directory. This should have created a new UselessMine directory within the plugins directory.

    `git clone https://github.com/allejo/UselessMine.git`

4.  Create a plugin using the `addToBuild.sh` script.

    `sh addToBuild.sh UselessMine`

5.  Now you will need to checkout the required submodules so the plugin has the proper dependencies so it can compile properly.

    `cd UselessMine; git submodule update --init`

6.  Instruct the build system to generate a Makefile and then compile and install the plugin.

    `cd ../..; ./autogen.sh; ./configure; make; make install;`
    
### Updating the Plugin

1.  Go into the UselessMine folder located in your plugins folder.

2.  Pull the changes from Git.

    `git pull origin master`

3.  (Optional) If you have made local changes to any of the files from this project, you may receive conflict errors where you may resolve the conflicts yourself or you may simply overwrite your changes with whatever is in the repository, which is recommended. *If you have a conflict every time you update because of your local change, submit a pull request and it will be accepted, provided it's a reasonable change.*

    `git reset --hard origin/master; git pull`

4.  Compile the changes.

    `make; make install;`

Server Details
--------------

### How to Use

To use this plugin after it has been compiled, simply load the plugin via the configuration file.

`-loadplugin /path/to/UselessMine.so`

### Custom BZDB Variables

    _mineSafetyTime

_mineSafetyTime

- Default: *5.0*

- Description: The number of seconds a player has to leave the mine detonation radius if they accidentally spawn in it.

### Using Custom BZDB Variables

Because this plugin utilizes custom BZDB variables, using `-set _ltsKickTime 90` in a configuration file or in an options block will cause an error; instead, `-setforced` must be used to set the value of the custom variable: `-setforced _ltsKickTime 90`. These variables can be set and changed normally in-game with the `/set` command.

### Custom Slash Commands

    /mine

/mine

- Permission Requirement: Non-observer

- Description: Place a mine

License
-------

[GNU General Public License Version 3](https://github.com/allejo/UselessMine/blob/master/LICENSE.md)