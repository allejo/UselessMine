/*
UselessMine
    Copyright (C) 2014 Vladimir "allejo" Jimenez

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <fstream>
#include <memory>
#include <stdlib.h>
#include <string>

#include "bzfsAPI.h"
#include "bztoolkit/bzToolkitAPI.h"

// Define plugin name
const std::string PLUGIN_NAME = "Useless Mine";

// Define plugin version numbering
const int MAJOR = 1;
const int MINOR = 0;
const int REV = 0;
const int BUILD = 21;

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

class UselessMine : public bz_Plugin, public bz_CustomSlashCommandHandler
{
public:
    virtual const char* Name ();
    virtual void Init (const char* config);
    virtual void Event (bz_EventData *eventData);
    virtual void Cleanup (void);

    virtual bool SlashCommand (int playerID, bz_ApiString, bz_ApiString, bz_APIStringList*);

    virtual int         getMineCount ();
    virtual void        removeAllMines (int playerID),
                        removeMine (int mineIndex),
                        setMine (int owner, float pos[3], bz_eTeamType team);
    virtual std::string formatDeathMessage (std::string msg, std::string victim, std::string owner);

    // The information each mine will contain
    struct Mine
    {
        int owner;             // The owner of the mine and the victim, respectively

        float x, y, z;         // The coordinates of where the mine was placed

        bz_eTeamType team;     // The team of the owner

        bool detonated;        // Whether or not the mine has been detonated; to prepare it for removal from play

        double detonationTime; // The time the mine was detonated

        Mine (int _owner, float _pos[3], bz_eTeamType _team) :
            owner(_owner),
            x(_pos[0]),
            y(_pos[1]),
            z(_pos[2]),
            team(_team),
            detonated(false)
        {}
    };

    std::vector<Mine>        activeMines;   // A vector that will store all of the mines that are in play
    std::vector<std::string> deathMessages; // A vector that will store all of the witty death messages

    std::string deathMessagesFile;

    double bzdb_SpawnSafetyTime, // The BZDB variable that will store the amount of seconds a player has before a mine is detonated
           playerSpawnTime[256]; // The time of a player's last spawn time used to calculate their safety from detonation
};

BZ_PLUGIN(UselessMine)

const char* UselessMine::Name (void)
{
    static std::string pluginBuild = "";

    if (!pluginBuild.size())
    {
        std::ostringstream pluginBuildStream;

        pluginBuildStream << PLUGIN_NAME << " " << MAJOR << "." << MINOR << "." << REV << " (" << BUILD << ")";
        pluginBuild = pluginBuildStream.str();
    }

    return pluginBuild.c_str();
}

void UselessMine::Init (const char* commandLine)
{
    // Register our events with Register()
    Register(bz_eFlagGrabbedEvent);
    Register(bz_ePlayerDieEvent);
    Register(bz_ePlayerPartEvent);
    Register(bz_ePlayerSpawnEvent);
    Register(bz_ePlayerUpdateEvent);

    // Register our custom slash commands
    bz_registerCustomSlashCommand("mine", this);

    // Set some custom BZDB variables
    bzdb_SpawnSafetyTime = bztk_registerCustomDoubleBZDB("_mineSafetyTime", 5.0);

    // Save the location of the file so we can reload after
    deathMessagesFile = commandLine;

    // Open the file of witty death messages
    std::ifstream file(commandLine);
    std::string currentLine;

    // If the file exists, read each line
    if (file)
    {
        // Push each line into the deathMessages vector
        while (std::getline(file, currentLine))
        {
            deathMessages.push_back(currentLine);
        }

        bz_debugMessagef(2, "DEBUG :: Useless Mine :: %d witty messages were loaded", deathMessages.size());
    }
    else
    {
        bz_debugMessage(2, "WARNING :: Useless Mine :: No witty death messages were loaded");
    }
}

void UselessMine::Cleanup (void)
{
    Flush(); // Clean up all the events

    // Clean up our custom slash commands
    bz_removeCustomSlashCommand("mine");
}

void UselessMine::Event (bz_EventData *eventData)
{
    switch (eventData->eventType)
    {
        case bz_eFlagGrabbedEvent: // This event is called each time a flag is grabbed by a player
        {
            bz_FlagGrabbedEventData_V1* flagGrabData = (bz_FlagGrabbedEventData_V1*)eventData;

            // Data
            // ---
            //    (int)           playerID  - The player that grabbed the flag
            //    (int)           flagID    - The flag ID that was grabbed
            //    (bz_ApiString)  flagType  - The flag abbreviation of the flag that was grabbed
            //    (float[3])      pos       - The position at which the flag was grabbed
            //    (double)        eventTime - This value is the local server time of the event.

            // If the user grabbed the Useless flag, let them know they can place a mine
            if (strcmp(flagGrabData->flagType, "US") == 0)
            {
                bz_sendTextMessage(BZ_SERVER, flagGrabData->playerID, "You grabbed a Useless flag! Type /mine at any time to set a useless mine!");
            }
        }
        break;


        case bz_ePlayerDieEvent: // This event is called each time a tank is killed.
        {
            bz_PlayerDieEventData_V1* dieData = (bz_PlayerDieEventData_V1*)eventData;

            // Data
            // ---
            //   (int)                   playerID       - ID of the player who was killed.
            //   (bz_eTeamType)          team           - The team the killed player was on.
            //   (int)                   killerID       - The owner of the shot that killed the player, or BZ_SERVER for server side kills
            //   (bz_eTeamType)          killerTeam     - The team the owner of the shot was on.
            //   (bz_ApiString)          flagKilledWith - The flag name the owner of the shot had when the shot was fired.
            //   (int)                   shotID         - The shot ID that killed the player, if the player was not killed by a shot, the id will be -1.
            //   (bz_PlayerUpdateState)  state          - The state record for the killed player at the time of the event
            //   (double)                eventTime      - Time of the event on the server.

            int playerID = dieData->playerID;

            // Loop through all the mines in play
            for (int i = 0; i < getMineCount(); i++)
            {
                // Create a local variable for easy access
                Mine &detonatedMine = activeMines.at(i);

                // Check if the mine has already been detonated
                if (detonatedMine.detonated)
                {
                    // If they were killed within the time the shockwave explodes, then we can safely say they were killed by the mine
                    if (detonatedMine.detonationTime + bz_getBZDBDouble("_shockAdLife") < bz_getCurrentTime())
                    {
                        // Check if the player who just died was killed by the server
                        if (dieData->killerID == 253)
                        {
                            float  deathPos[3] = {dieData->state.pos[0], dieData->state.pos[1], dieData->state.pos[2]};
                            double shockRange = bz_getBZDBDouble("_shockOutRadius") * 0.75;

                            if ((deathPos[0] > detonatedMine.x - shockRange && deathPos[0] < detonatedMine.x + shockRange) &&
                                (deathPos[1] > detonatedMine.y - shockRange && deathPos[1] < detonatedMine.y + shockRange) &&
                                (deathPos[2] > detonatedMine.z - shockRange && deathPos[2] < detonatedMine.z + shockRange))
                            {
                                // Attribute the kill to the mine owner
                                dieData->killerID = detonatedMine.owner;

                                // Only get a death messages if death messages exist and the player who died is now also the mine owner
                                if (!deathMessages.empty() && playerID != detonatedMine.owner)
                                {
                                    // The random number used to fetch a random taunting death message
                                    int randomNumber = rand() % deathMessages.size();

                                    // Get the callsigns of the players
                                    const char* owner  = bz_getPlayerCallsign(detonatedMine.owner);
                                    const char* victim = bz_getPlayerCallsign(playerID);

                                    // Get a random death message
                                    std::string deathMessage = deathMessages.at(randomNumber);
                                    bz_sendTextMessage(BZ_SERVER, BZ_ALLUSERS, formatDeathMessage(deathMessage, victim, owner).c_str());
                                }

                                break;
                            }
                        }
                    }
                    else
                    {
                        removeMine(i);
                    }
                }
            }
        }
        break;

        case bz_ePlayerPartEvent: // This event is called each time a player leaves a game
        {
            bz_PlayerJoinPartEventData_V1* partData = (bz_PlayerJoinPartEventData_V1*)eventData;

            // Data
            // ---
            //    (int)                   playerID  - The player ID that is leaving
            //    (bz_BasePlayerRecord*)  record    - The player record for the leaving player
            //    (bz_ApiString)          reason    - The reason for leaving, such as a kick or a ban
            //    (double)                eventTime - Time of event.

            int playerID = partData->playerID;

            // Remove all the mines belonging to the player who just left
            removeAllMines(playerID);
        }
        break;

        case bz_ePlayerSpawnEvent: // This event is called each time a playing tank is being spawned into the world
        {
            bz_PlayerSpawnEventData_V1* spawnData = (bz_PlayerSpawnEventData_V1*)eventData;

            // Data
            // ---
            //    (int)                   playerID  - ID of the player who was added to the world.
            //    (bz_eTeamType)          team      - The team the player is a member of.
            //    (bz_PlayerUpdateState)  state     - The state record for the spawning player
            //    (double)                eventTime - Time local server time for the event.

            int playerID = spawnData->playerID;

            // Save the time the player spawned last
            playerSpawnTime[playerID] = bz_getCurrentTime();
        }
        break;

        case bz_ePlayerUpdateEvent: // This event is called each time a player sends an update to the server
        {
            bz_PlayerUpdateEventData_V1* updateData = (bz_PlayerUpdateEventData_V1*)eventData;

            // Data
            // ---
            //   (int)                   playerID  - ID of the player that sent the update
            //   (bz_PlayerUpdateState)  state     - The original state the tank was in
            //   (bz_PlayerUpdateState)  lastState - The second state the tank is currently in to show there was an update
            //   (double)                stateTime - The time the state was updated
            //   (double)                eventTime - The current server time

            int playerID = updateData->playerID;

            // Loop through all of the players
            for (int i = 0; i < getMineCount(); i++)
            {
                // Make an easy access mine
                Mine &currentMine = activeMines.at(i);

                std::shared_ptr<bz_BasePlayerRecord> pr(bz_getPlayerByIndex(playerID));

                // If the mine owner is not the player triggering the mine (so no self kills) and the player is a rogue or does is an enemy team relative to the mine owner
                if (currentMine.owner != playerID && (pr->team == eRogueTeam || pr->team != currentMine.team) && pr->spawned)
                {
                    // Make easy to access variables
                    float  playerPos[3] = {updateData->state.pos[0], updateData->state.pos[1], updateData->state.pos[2]};
                    double shockRange   = bz_getBZDBDouble("_shockOutRadius") * 0.75;

                    // Check if the player is in the detonation range
                    if ((playerPos[0] > currentMine.x - shockRange && playerPos[0] < currentMine.x + shockRange) &&
                        (playerPos[1] > currentMine.y - shockRange && playerPos[1] < currentMine.y + shockRange) &&
                        (playerPos[2] > currentMine.z - shockRange && playerPos[2] < currentMine.z + shockRange) &&
                        playerSpawnTime[playerID] + bzdb_SpawnSafetyTime <= bz_getCurrentTime())
                    {
                        // Check that the mine owner exists and is not an observer
                        if (bztk_isValidPlayerID(currentMine.owner) && bz_getPlayerTeam(currentMine.owner) != eObservers)
                        {
                            // Get the current mine position
                            float minePos[3] = {currentMine.x, currentMine.y, currentMine.z};

                            // Only detonate a mine once
                            if (!currentMine.detonated)
                            {
                                currentMine.detonated = true;
                                currentMine.detonationTime = bz_getCurrentTime();

                                // BOOM!
                                bz_fireWorldWep("SW", 2.0, BZ_SERVER, minePos, 0, 0, 0, currentMine.team);
                            }
                        }
                        // Just in case the player doesn't exist or is an observer, then remove the mine because it shouldn't be there
                        else
                        {
                            removeMine(i);
                        }

                        break;
                    }
                }
            }
        }
        break;

        default: break;
    }
}

bool UselessMine::SlashCommand(int playerID, bz_ApiString command, bz_ApiString /*message*/, bz_APIStringList *params)
{
    if (command == "mine")
    {
        std::shared_ptr<bz_BasePlayerRecord> playerRecord(bz_getPlayerByIndex(playerID));

        // If the player is not an observer, they let them proceed to the next check
        if (playerRecord->team != eObservers)
        {
            // Check if the player has the Useless flag
            if (playerRecord->currentFlag == "USeless (+US)")
            {
                // Store their current position
                float currentPosition[3] = {playerRecord->lastKnownState.pos[0], playerRecord->lastKnownState.pos[1], playerRecord->lastKnownState.pos[2]};

                setMine(playerID, currentPosition, playerRecord->team);
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

        return true;
    }

    return true;
}

// A function to format death messages in order to replace placeholders with callsigns and values
std::string UselessMine::formatDeathMessage(std::string msg, std::string victim, std::string owner)
{
    // Replace the %victim% and %owner% placeholders
    std::string formattedMessage = ReplaceString(ReplaceString(msg, "%victim%", victim), "%owner%", owner);

    // If the message has a %minecount%, then replace it
    if (formattedMessage.find("%minecount%") != std::string::npos)
    {
        formattedMessage = ReplaceString(formattedMessage, "%minecount%", std::to_string(getMineCount() - 1));
    }

    return formattedMessage;
}


// A shortcut to get the amount of mines that are in play
int UselessMine::getMineCount()
{
    return activeMines.size();
}

// Remove all of the mines belonging to a player
void UselessMine::removeAllMines(int playerID)
{
    // Go through all of the mines
    for (int i = 0; i < getMineCount(); i++)
    {
        // Quick access mine
        Mine &currentMine = activeMines.at(i);

        // If the mine belongs to the player, remove it
        if (currentMine.owner == playerID)
        {
            removeMine(i);
        }
    }
}

// A shortcut to remove a mine from play
void UselessMine::removeMine(int mineIndex)
{
    activeMines.erase(activeMines.begin() + mineIndex);
}

// A shortcut to set a mine
void UselessMine::setMine(int owner, float pos[3], bz_eTeamType team)
{
    // Remove their flag because it's going to be a US flag
    bz_removePlayerFlag(owner);

    // Push the new mine
    Mine newMine(owner, pos, team);
    activeMines.push_back(newMine);
}