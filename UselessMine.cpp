/*
    Copyright (C) 2013-2017 Vladimir "allejo" Jimenez

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the “Software”), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
*/

#include <algorithm>
#include <functional>
#include <memory>
#include <stdlib.h>

#include "bzfsAPI.h"
#include "bztoolkit/bzToolkitAPI.h"

const int DEBUG_VERBOSITY = 4;

// Define plugin name
const std::string PLUGIN_NAME = "Useless Mine";

// Define plugin version numbering
const int MAJOR = 1;
const int MINOR = 1;
const int REV = 0;
const int BUILD = 81;


class UselessMine : public bz_Plugin, public bz_CustomSlashCommandHandler
{
public:
    virtual const char* Name ();
    virtual void Init (const char* config);
    virtual void Event (bz_EventData *eventData);
    virtual void Cleanup (void);
    virtual bool SlashCommand (int, bz_ApiString, bz_ApiString, bz_APIStringList*);

    typedef std::multimap< int, std::string, std::greater<int> > rmap;

    // The information each mine will contain
    struct Mine
    {
        bz_ApiString uid;      // A unique ID for mine
        bool queuedRemoval;

        int owner;             // The owner of the mine
        int defuserID;         // The player who defused this mine
        int detonationID;      // The shot ID of the world weapon that exploded when this mine was defused/detonated
        float x, y, z;         // The coordinates of where the mine was placed
        bz_eTeamType team;     // The team of the mine owner
        bool defused;          // True if the mine was defused with a Bomb Defusal flag
        bool detonated;        // True if the mine was detonated by a player
        double detonationTime; // The time the mine was detonated

        Mine (int _owner, float _pos[3], bz_eTeamType _team) :
            owner(_owner),
            defuserID(-1),
            detonationID(-1),
            x(_pos[0]),
            y(_pos[1]),
            z(_pos[2]),
            team(_team),
            defused(false),
            detonated(false),
            detonationTime(-1),
            queuedRemoval(false)
        {
            uid.format("%d_%d_%d", owner, bz_getCurrentTime(), bzfrand());
        }

        // Has the mine already been detonated/defused and is ready to be removed?
        bool isStale()
        {
            return (detonationTime != -1 && detonationTime + 60 < bz_getCurrentTime());
        }

        // Should a given player trigger this mine?
        // This function checks mine ownership, team loyalty, player's location and player's alive-ness
        bool canPlayerTriggerMine(bz_BasePlayerRecord *pr, float pos[3])
        {
            if (owner != pr->playerID && (pr->team == eRogueTeam || pr->team != team || bz_getGameType() == eOpenFFAGame) && pr->spawned)
            {
                float  playerPos[3] = {pos[0], pos[1], pos[2]};
                double shockRange   = bz_getBZDBDouble("_shockOutRadius") * 0.75;

                // Check if the player is in the detonation range
                bool inDetonationRange = ((playerPos[0] > x - shockRange && playerPos[0] < x + shockRange) &&
                                          (playerPos[1] > y - shockRange && playerPos[1] < y + shockRange) &&
                                          (playerPos[2] > z - shockRange && playerPos[2] < z + shockRange));

                return inDetonationRange;
            }

            return false;
        }
        
        // This sets the mine for defusal - the mine will trigger, but
        // will instead trigger at the location of the owner, with the
        // killer ID being that of the defuser.
        bool defuse(int playerID)
        {
            if (queuedRemoval)
            {
                return false;
            }

            bz_BasePlayerRecord *pr = bz_getPlayerByIndex(owner);

            if (!pr || pr->team == eObservers)
            {
                bz_freePlayerRecord(pr);
                return false;
            }

            bz_debugMessagef(DEBUG_VERBOSITY, "DEBUG :: Useless Mine :: Mine UID %s defused by %d", uid.c_str(), playerID);

            defused = true;
            defuserID = playerID;
            detonationTime = bz_getCurrentTime();

            bz_sendTextMessagef(BZ_SERVER, BZ_ALLUSERS, "%s successfully defused %s's mine!", bz_getPlayerCallsign(defuserID), bz_getPlayerCallsign(owner));

            queuedRemoval = bz_fireWorldWep("SW", 2.0, BZ_SERVER, pr->lastKnownState.pos, 0, 0, 0, &detonationID, 0, bz_getPlayerTeam(defuserID));

            bz_freePlayerRecord(pr);

            return true;
        }
        // This sets the mine for detonation - the mine will trigger,
        // killing the victim and setting the killer as the mine
        // owner. 
        bool detonate()
        {
            if (queuedRemoval)
            {
                return false;
            }

            bz_BasePlayerRecord *pr = bz_getPlayerByIndex(owner);

            if (!pr || pr->team == eObservers)
            {
                bz_freePlayerRecord(pr);
                return false;
            }

            float minePos[3] = {x, y, z};

            detonated = true;
            detonationTime = bz_getCurrentTime();

            bz_debugMessagef(DEBUG_VERBOSITY, "DEBUG :: Useless Mine :: Mine UID %s detonated", uid.c_str());

            queuedRemoval = bz_fireWorldWep("SW", 2.0, BZ_SERVER, minePos, 0, 0, 0, &detonationID, 0, team);

            bz_freePlayerRecord(pr);

            return true;
        }
    };

private:
    int  getMineCount ();
    void reloadDeathMessages (),
         reloadDefusalMessages (),
         removePlayerMines (int playerID),
         removeMine (Mine &mine),
         setMine (int owner, float pos[3], bz_eTeamType team);
    std::string formatMineMessage (std::string msg, std::string victim, std::string owner);

    std::vector<std::string> deathMessages; // A vector that will store all of the witty death messages
    std::vector<std::string> defusalMessages; // A vector that will store all of the witty defusal messages

    std::vector<Mine> activeMines; // A vector that will store all of the mines currently on the field
    std::string deathMessagesFile; // The path to the file containing death messages
    std::string defusalMessagesFile; // The path to the file containing defusal messages
    double playerSpawnTime[256]; // The time a player spawned last; used for _mineSafetyTime calculations
};

BZ_PLUGIN(UselessMine)

const char* UselessMine::Name (void)
{
    static std::string pluginName;

    if (pluginName.empty())
        pluginName = bztk_pluginName(PLUGIN_NAME, MAJOR, MINOR, REV, BUILD);

    return pluginName.c_str();
}

void UselessMine::Init (const char* commandLine)
{
    Register(bz_eFlagGrabbedEvent);
    Register(bz_ePlayerDieEvent);
    Register(bz_ePlayerPartEvent);
    Register(bz_ePlayerSpawnEvent);
    Register(bz_ePlayerUpdateEvent);

    bz_registerCustomSlashCommand("mine", this);
    bz_registerCustomSlashCommand("minecount", this);
    bz_registerCustomSlashCommand("minestats", this);
    bz_registerCustomSlashCommand("reload", this);

    bz_RegisterCustomFlag("BD", "Bomb Defusal", "Safely defuse enemy mines while killing the mine owners", 0, eGoodFlag);

    bztk_registerCustomDoubleBZDB("_mineSafetyTime", 5.0);

    // Save the location of the file so we can reload after
    
    // This expects two command line parameters: the death messages
    // file and the defusal messages file. 
    bz_APIStringList cmdLineParams;
    cmdLineParams.tokenize(commandLine, ",");
    
    if (cmdLineParams.size() == 2)
    {
        deathMessagesFile = cmdLineParams.get(0);
        defusalMessagesFile = cmdLineParams.get(1);
    }
    else
    {
        deathMessagesFile = "";
        defusalMessagesFile = "";
        bz_debugMessagef(DEBUG_VERBOSITY, "WARNING :: Useless Mine :: No messages loaded");
    }
    
    reloadDeathMessages();
    reloadDefusalMessages();

    if (deathMessages.empty())
    {
        bz_debugMessage(2, "WARNING :: Useless Mine :: No witty death messages were loaded");
    }
    else
    {
        bz_debugMessagef(2, "DEBUG :: Useless Mine :: %d witty death messages were loaded", deathMessages.size());
    }

    if (defusalMessages.empty())
    {
        bz_debugMessage(2, "WARNING :: Useless Mine :: No witty defusal messages were loaded");
    }
    else
    {
        bz_debugMessagef(2, "DEBUG :: Useless Mine :: %d witty defusal messages were loaded", defusalMessages.size());
    }
}

void UselessMine::Cleanup (void)
{
    Flush();

    bz_removeCustomSlashCommand("mine");
    bz_removeCustomSlashCommand("minecount");
    bz_removeCustomSlashCommand("minestats");
    bz_removeCustomSlashCommand("reload");
}

void UselessMine::Event (bz_EventData *eventData)
{
    switch (eventData->eventType)
    {
        case bz_eFlagGrabbedEvent:
        {
            bz_FlagGrabbedEventData_V1* flagGrabData = (bz_FlagGrabbedEventData_V1*)eventData;

            // If the user grabbed the Useless flag, let them know they can place a mine
            if (strcmp(flagGrabData->flagType, "US") == 0)
            {
                bz_sendTextMessage(BZ_SERVER, flagGrabData->playerID, "You grabbed a Useless flag! Type /mine at any time to set a useless mine!");
            }
        }
        break;

        case bz_ePlayerDieEvent:
        {
            bz_PlayerDieEventData_V1* dieData = (bz_PlayerDieEventData_V1*)eventData;

            int playerID = dieData->playerID;

            for (Mine &mine : activeMines)
            {
                // We delete the mine after it's considered stale. This way, we can correctly allow for multiple players to be killed by the mine
                if (mine.isStale())
                {
                    removeMine(mine);
                    continue;
                }

                // This isn't the mine we're looking for since it hasn't been touched
                if (!mine.detonated && !mine.defused)
                {
                    continue;
                }

                // If the shot IDs don't match, this mine wasn't the killer
                if (dieData->shotID != mine.detonationID)
                {
                    continue;
                }


                // This is the mine that killed our player, so handle it

                const char* owner  = bz_getPlayerCallsign(mine.owner);
                const char* victim = bz_getPlayerCallsign(playerID);

                if (mine.detonated)
                {
                    dieData->killerID = mine.owner;

                    if (playerID != mine.owner)
                    {
                        if (!deathMessages.empty())
                        {
                            // The random number used to fetch a random taunting death message
                            int randomNumber = rand() % deathMessages.size();
                            std::string deathMessage = deathMessages.at(randomNumber);

                            bz_sendTextMessage(BZ_SERVER, BZ_ALLUSERS, formatMineMessage(deathMessage, victim, owner).c_str());
                        }
                        else
                        {
                            // If there are no death messages, explain to the user that it was a mine that killed them
                            bz_sendTextMessagef(BZ_SERVER, playerID, "You were killed by %s's mine", owner);
                        }
                    }
                }
                else if (mine.defused)
                {
                    dieData->killerID = mine.defuserID;

                    if (playerID != mine.owner)
                    {
                        if (!defusalMessages.empty())
                        {
                            // The random number used to fetch a random taunting defusal message
                            int randomNumber = rand() % defusalMessages.size();

                            // Get a random defusal message
                            std::string defusalMessage = defusalMessages.at(randomNumber);
                            bz_sendTextMessage(BZ_SERVER, BZ_ALLUSERS, formatMineMessage(defusalMessage, owner, bz_getPlayerCallsign(mine.defuserID)).c_str());
                        }
                        else
                        {
                            // Let the BD player know that they killed the owner
                            bz_sendTextMessagef(BZ_SERVER, mine.defuserID, "You defused %s's mine", owner);

                            // Let the owner know that they killed the BD player
                            bz_sendTextMessagef(BZ_SERVER, mine.owner, "You were killed by %s's mine defusal", bz_getPlayerCallsign(mine.defuserID));
                            
                        }
                    }
                }

                break;
            }
        }
        break;

        case bz_ePlayerPartEvent:
        {
            bz_PlayerJoinPartEventData_V1* partData = (bz_PlayerJoinPartEventData_V1*)eventData;

            int playerID = partData->playerID;

            // Remove all the mines belonging to the player who just left
            removePlayerMines(playerID);
            playerSpawnTime[playerID] = -1;
        }
        break;

        case bz_ePlayerSpawnEvent:
        {
            bz_PlayerSpawnEventData_V1* spawnData = (bz_PlayerSpawnEventData_V1*)eventData;

            int playerID = spawnData->playerID;

            // Save the time the player spawned last
            playerSpawnTime[playerID] = bz_getCurrentTime();
        }
        break;

        case bz_ePlayerUpdateEvent:
        {
            bz_PlayerUpdateEventData_V1* updateData = (bz_PlayerUpdateEventData_V1*)eventData;

            int playerID = updateData->playerID;
            bz_BasePlayerRecord *pr = bz_getPlayerByIndex(playerID);

            for (Mine &mine : activeMines)
            {
                bool bypassSafetyTime = (playerSpawnTime[pr->playerID] + bz_getBZDBDouble("_mineSafetyTime") <= bz_getCurrentTime());

                if (mine.canPlayerTriggerMine(pr, updateData->state.pos) && bypassSafetyTime)
                {
                    (pr->currentFlag == "Bomb Defusal (+BD)") ? mine.defuse(playerID) : mine.detonate();
                    break;
                }
            }

            bz_freePlayerRecord(pr);
        }
        break;

        default:
            break;
    }
}

bool UselessMine::SlashCommand(int playerID, bz_ApiString command, bz_ApiString /*message*/, bz_APIStringList *params)
{
    if (command == "mine")
    {
        bz_BasePlayerRecord *pr = bz_getPlayerByIndex(playerID);

        if (pr->team != eObservers)
        {
            // Check if the player has the Useless flag
            if (pr->currentFlag == "USeless (+US)")
            {
                // Store their current position
                float currentPosition[3] = {pr->lastKnownState.pos[0], pr->lastKnownState.pos[1], pr->lastKnownState.pos[2]};

                setMine(playerID, currentPosition, pr->team);
            }
            else
            {
                bz_sendTextMessage(BZ_SERVER, playerID, "You can't place a mine without the Useless flag!");
            }
        }
        else
        {
            bz_sendTextMessage(BZ_SERVER, playerID, "Silly observer, you can't place a mine.");
        }

        bz_freePlayerRecord(pr);

        return true;
    }
    else if (command == "minecount")
    {
        bz_sendTextMessagef(BZ_SERVER, playerID, "There are currently %d active mines on the field", getMineCount());

        return true;
    }
    else if (command == "minestats")
    {
        bz_sendTextMessagef(BZ_SERVER, playerID, "Player Mines");
        bz_sendTextMessagef(BZ_SERVER, playerID, "------------");

        std::map<std::string, int> mineCount;

        for (Mine &mine : activeMines)
        {
            if (mine.queuedRemoval)
            {
                continue;
            }

            mineCount[bz_getPlayerCallsign(mine.owner)]++;
        }

        rmap mineList;

        for (auto i : mineCount)
        {
            mineList.insert(rmap::value_type(i.second, i.first));
        }

        for (auto i : mineList)
        {
            bz_sendTextMessagef(BZ_SERVER, playerID, "%-32s %d", i.second.c_str(), i.first);
        }

        return true;
    }
    else if (command == "reload" && bz_hasPerm(playerID, "setAll"))
    {
        if (params->size() == 0)
        {
            reloadDeathMessages();
            reloadDefusalMessages();
        }

        else if (params->get(0) == "deathmessages")
        {
            reloadDeathMessages();
            bz_sendTextMessage(BZ_SERVER, playerID, "Death messages reloaded");
            return true;
        }

        else if (params->get(0) == "defusalmessages")
        {
            reloadDefusalMessages();
            bz_sendTextMessage(BZ_SERVER, playerID, "Defusal messages reloaded");
            return true;
        }
    }

    return false;
}

// A function to replace substrings in a string with another substring
std::string ReplaceString(std::string subject, const std::string& search, const std::string& replace)
{
    size_t pos = 0;

    while ((pos = subject.find(search, pos)) != std::string::npos)
    {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }

    return subject;
}

// A function to format death messages in order to replace placeholders with callsigns and values
std::string UselessMine::formatMineMessage(std::string msg, std::string victim, std::string owner)
{
    // Replace the %victim% and %owner% placeholders
    std::string formattedMessage = ReplaceString(ReplaceString(msg, "%victim%", victim), "%owner%", owner);

    // If the message has a %minecount%, then replace it
    if (formattedMessage.find("%minecount%") != std::string::npos)
    {
        formattedMessage = ReplaceString(formattedMessage, "%minecount%", std::to_string(getMineCount()));
    }

    return formattedMessage;
}

// Reload the death messages
void UselessMine::reloadDeathMessages()
{
    deathMessages.clear();

    if (!deathMessagesFile.empty())
    {
        bztk_fileToVector(deathMessagesFile.c_str(), deathMessages);
    }
}

// Reload the defusal messages
void UselessMine::reloadDefusalMessages()
{
    defusalMessages.clear();

    if (!defusalMessagesFile.empty())
    {
        bztk_fileToVector(defusalMessagesFile.c_str(), defusalMessages);
    }
}

// Get the amount of active mines that exist
int UselessMine::getMineCount()
{
    int count = 0;

    for (Mine &m : activeMines)
    {
        if (!m.queuedRemoval)
        {
            count++;
        }
    }

    return count;
}

// Remove a specific mine
void UselessMine::removeMine(Mine &mine)
{
    const char* uid = mine.uid.c_str();

    bz_debugMessagef(DEBUG_VERBOSITY, "DEBUG :: Useless Mine :: Removing mine UID: %s", mine.uid.c_str());
    bz_debugMessagef(DEBUG_VERBOSITY, "DEBUG :: Useless Mine ::   mine count: %d", getMineCount());

    activeMines.erase(
        std::remove_if(
            activeMines.begin(),
            activeMines.end(),
            [uid](Mine &m) {
                bool result = (m.uid == uid);

                if (result)
                {
                    bz_debugMessagef(DEBUG_VERBOSITY, "DEBUG :: Useless Mine :: Mine %s removed", uid);
                }

                return result;
            }
        ),
        activeMines.end()
    );

    bz_debugMessagef(DEBUG_VERBOSITY, "DEBUG :: Useless Mine ::   new mine count: %d", getMineCount());
}

// Remove all of the mines of a specific player
void UselessMine::removePlayerMines(int playerID)
{
    bz_debugMessagef(DEBUG_VERBOSITY, "DEBUG :: Useless Mine :: Removing all mines for player %d", playerID);

    activeMines.erase(
        std::remove_if(
            activeMines.begin(),
            activeMines.end(),
            [playerID](Mine &m) { return m.owner == playerID; }
        ),
        activeMines.end()
    );
}

// A shortcut to set a mine
void UselessMine::setMine(int owner, float pos[3], bz_eTeamType team)
{
    // Remove their flag because they "converted" it into a mine
    bz_removePlayerFlag(owner);

    Mine newMine(owner, pos, team);

    bz_debugMessagef(DEBUG_VERBOSITY, "DEBUG :: Useless Mine :: Mine UID %s created by %d", newMine.uid.c_str(), owner);
    bz_debugMessagef(DEBUG_VERBOSITY, "DEBUG :: Useless Mine ::   x, y, z => %0.2f, %0.2f, %0.2f", pos[0], pos[1], pos[2]);

    activeMines.push_back(newMine);
}
