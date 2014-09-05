#include "bzfsAPI.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class uselessmine : public bz_Plugin, bz_CustomSlashCommandHandler
{
	virtual const char* Name (){return "Useless Mine";}
	virtual void Init (const char* /*config*/);
	virtual void Event(bz_EventData *eventData);
	virtual bool SlashCommand (int playerID,bz_ApiString command,bz_ApiString message,bz_APIStringList* params);
	virtual void Cleanup ();
	virtual int CountMines();
	virtual void SetMine(int playerID,float pos1,float pos2,float pos3);
	virtual void RemoveMine(int id);
	virtual void RemoveAllMines(int playerID);
	public:
		double spawnsafetime; //This variable can be modified to reflect how much time players have before being hit by mines after spawning
		bool mine[255]; //Whether or not the mine has been set.
		signed int mineplayer[255]; //Which player set up the mine.
		float minepos[255][3]; //The position of a mine.
		signed int mineteam[255]; //The team the mine was set on.
		double safetytime[255]; //The safe time limit that players have when they spawn. To prevent instant deaths.
		bool minenotify[255]; //Tells the player they have useless.
};

BZ_PLUGIN(uselessmine);

void uselessmine::Init(const char* /*commandline*/)
{
	spawnsafetime=5.0; //Tells the plugin to allow 5 seconds of grace time before activating mines on a player.
	
	bz_debugMessage(1,"Useless Mine plugin loaded");
	bz_registerCustomSlashCommand("mine",this);
	Register(bz_eFlagGrabbedEvent);
	Register(bz_ePlayerPartEvent);
	Register(bz_ePlayerUpdateEvent);
	Register(bz_ePlayerDieEvent);
	Register(bz_ePlayerSpawnEvent);
	int i;
	for (i=0;i<255;i++) {
		mine[i]=0;
		mineplayer[i]=-1;
		minepos[i][0]=0;
		minepos[i][1]=0;
		minepos[i][2]=0;
		mineteam[i]=-1;
		minenotify[i]=0;
	}
}

int uselessmine::CountMines()
{
	int i,mines;
	mines=0;
	for (i=0;i<255;i++) {
		if (mine[i]) {mines++;}
	}
	return mines;
}

void uselessmine::Cleanup(void)
{
	Flush();
	bz_removeCustomSlashCommand("mine");
	bz_debugMessage(1,"Useless Mine plugin unloaded");
}

void uselessmine::SetMine(int playerID,float pos1,float pos2,float pos3)
{
	int i;
	for (i=0;i<255;i++) {
		if (!mine[i]) {break;}
	}
	mine[i]=1;
	mineplayer[i]=playerID;
	minepos[i][0]=pos1;
	minepos[i][1]=pos2;
	minepos[i][2]=pos3;
	bz_BasePlayerRecord *pr = bz_getPlayerByIndex(playerID);
	if (pr) {
		mineteam[i]=pr->team;
		bz_freePlayerRecord(pr);
	}
}


void uselessmine::RemoveMine(int id)
{
	mine[id]=0;
}

void uselessmine::RemoveAllMines(int playerID)
{
	int i;
	for (i=0;i<255;i++) {
		if (mine[i] && mineplayer[i]==playerID) {
			RemoveMine(i);
		}
	}
}

bool uselessmine::SlashCommand(int playerID,bz_ApiString command,bz_ApiString message,bz_APIStringList* params)
{
	if (!strcmp("mine",command.c_str())) {
		bz_BasePlayerRecord *pr = bz_getPlayerByIndex(playerID);
		if (pr) {
			if (pr->currentFlag=="USeless (+US)") {
				bz_removePlayerFlag(playerID);
				SetMine(playerID,pr->lastKnownState.pos[0],pr->lastKnownState.pos[1],pr->lastKnownState.pos[2]);
			}
			bz_freePlayerRecord(pr);
		}
		return 1;
	}
}

void uselessmine::Event(bz_EventData *eventData)
{
	int playerID = -1;
	switch (eventData->eventType) {
		case bz_eFlagGrabbedEvent: {
			playerID=((bz_FlagGrabbedEventData_V1*)eventData)->playerID;
			/*bz_BasePlayerRecord *pr = bz_getPlayerByIndex(playerID);
			//bz_debugMessage(0,bz_getName(((bz_FlagGrabbedEventData_V1*)eventData)->flagID).c_str());
			if (pr) {
			if (!strcmp(pr->currentFlag.c_str(),"US")) {
				bz_sendTextMessage(BZ_SERVER,playerID,"You grabbed a Useless flag! Type /mine at any time to set a useless mine!");
			}
			bz_freePlayerRecord(pr);
			}*/
			minenotify[playerID]=1;
		}break;
		case bz_ePlayerPartEvent: {
			playerID=((bz_PlayerJoinPartEventData_V1*)eventData)->playerID;
			RemoveAllMines(playerID);
		}break;
		case bz_ePlayerUpdateEvent: {
			int i;
			playerID=((bz_PlayerUpdateEventData_V1*)eventData)->playerID;
			bz_BasePlayerRecord *pr = bz_getPlayerByIndex(playerID);
			if (pr) {
			if (minenotify[playerID] && pr->currentFlag=="USeless (+US)") {bz_sendTextMessage(BZ_SERVER,playerID,"You grabbed a Useless flag! Type /mine at any time to set a useless mine!");
			minenotify[playerID]=0;}
			for (i=0;i<255;i++) {
					if (mine[i] && mineplayer[i]!=playerID && (pr->team==eRogueTeam || pr->team!=mineteam[i])) {
						if (pr->lastKnownState.pos[0]>minepos[i][0]-((bz_getBZDBDouble("_shockOutRadius")*0.75))
						&& pr->lastKnownState.pos[0]<minepos[i][0]+((bz_getBZDBDouble("_shockOutRadius")*0.75))
						&& pr->lastKnownState.pos[1]>minepos[i][1]-((bz_getBZDBDouble("_shockOutRadius")*0.75))
						&& pr->lastKnownState.pos[1]<minepos[i][1]+((bz_getBZDBDouble("_shockOutRadius")*0.75))
						&& pr->lastKnownState.pos[2]>minepos[i][2]-((bz_getBZDBDouble("_shockOutRadius")*0.75))
						&& pr->lastKnownState.pos[2]<minepos[i][2]+((bz_getBZDBDouble("_shockOutRadius")*0.75))
						&& safetytime[playerID]<=bz_getCurrentTime()
						) {
							bz_fireWorldWep("SW",2.0,BZ_SERVER,minepos[i],0,0,i+1000,0);
							RemoveMine(i);
						}
					}
			}
			bz_freePlayerRecord(pr);
			}
		}break;
		case bz_ePlayerDieEvent:{
			playerID=((bz_PlayerDieEventData_V1*)eventData)->playerID;
			int i;
			for (i=0;i<255;i++) {
				if (((bz_PlayerDieEventData_V1*)eventData)->shotID==i+1000) {
					((bz_PlayerDieEventData_V1*)eventData)->killerID=mineplayer[i];
					bz_BasePlayerRecord *pr = bz_getPlayerByIndex(playerID);
					bz_BasePlayerRecord *kr = bz_getPlayerByIndex(mineplayer[i]);
					if (pr || kr) {
						if (pr && kr) {
						switch (rand() % 20) {
							case 0:{
								bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"%s was killed by %s's mine. [%d]",pr->callsign.c_str(),kr->callsign.c_str(),CountMines());
							}break;
							case 1:{
								bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"%s was owned by %s's mine. [%d]",pr->callsign.c_str(),kr->callsign.c_str(),CountMines());
							}break;
							case 2:{
								bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"%s was obliterated by %s's mine. [%d]",pr->callsign.c_str(),kr->callsign.c_str(),CountMines());
							}break;
							case 3:{
								bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"%s's tank disintegrated from %s's mine. [%d]",pr->callsign.c_str(),kr->callsign.c_str(),CountMines());
							}break;
							case 4:{
								bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"%s was permanently blinded by the bright light from %s's mine. [%d]",pr->callsign.c_str(),kr->callsign.c_str(),CountMines());
							}break;
							case 5:{
								bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"%s was sent shooting into the stars by %s's mine. [%d]",pr->callsign.c_str(),kr->callsign.c_str(),CountMines());
							}break;
							case 6:{
								bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"%s was never heard from again thanks to %s's mine. [%d]",pr->callsign.c_str(),kr->callsign.c_str(),CountMines());
							}break;
							case 7:{
								bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"We salute %s for taking an atrocious hit from %s's mine. [%d]",pr->callsign.c_str(),kr->callsign.c_str(),CountMines());
							}break;
							case 8:{
								bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"%s's mine left %s's tank parts scattered all over. [%d]",kr->callsign.c_str(),pr->callsign.c_str(),CountMines());
							}break;
							case 9:{
								bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"%s thought %s's mine was a shiny brand new car. [%d]",pr->callsign.c_str(),kr->callsign.c_str(),CountMines());
							}break;
							case 10:{
								bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"%s was bombarded by many concussive attacks from %s's mine. [%d]",pr->callsign.c_str(),kr->callsign.c_str(),CountMines());
							}break;
							case 11:{
								bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"%s was ignited by %s's mine. [%d]",pr->callsign.c_str(),kr->callsign.c_str(),CountMines());
							}break;
							case 12:{
								bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"%s's mine bursted %s's tank to bite-size flaming pieces. [%d]",kr->callsign.c_str(),pr->callsign.c_str(),CountMines());
							}break;
							case 13:{
								bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"%s took a nosedive into %s's mine. [%d]",pr->callsign.c_str(),kr->callsign.c_str(),CountMines());
							}break;
							case 14:{
								bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"%s fell face first into %s's mine. [%d]",pr->callsign.c_str(),kr->callsign.c_str(),CountMines());
							}break;
							case 15:{
								bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"I knew %s would be clumsy enough to run into %s's mine. [%d]",pr->callsign.c_str(),kr->callsign.c_str(),CountMines());
							}break;
							case 16:{
								bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"%s killed %s with a mine. No surprise there. [%d]",kr->callsign.c_str(),pr->callsign.c_str(),CountMines());
							}break;
							case 17:{
								bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"DID YOU SEE THAT? %s did total carnage to %s's tank with that one little mine. [%d]",kr->callsign.c_str(),pr->callsign.c_str(),CountMines());
							}break;
							case 18:{
								bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"%s purposely ran into %s's mine. [%d]",pr->callsign.c_str(),kr->callsign.c_str(),CountMines());
							}break;
							case 19:{
								bz_sendTextMessagef(BZ_SERVER,BZ_ALLUSERS,"I ascertain that %s has been ruptured by a mine created from the heavens with the name dubbed %s. [%d]",pr->callsign.c_str(),kr->callsign.c_str(),CountMines());
							}break;
						}
						}
						if (pr) {
					bz_freePlayerRecord(pr);}
					if (kr) {
					bz_freePlayerRecord(kr);
					}
					}
				}
			}
		}break;
		case bz_ePlayerSpawnEvent: {
			playerID=((bz_PlayerSpawnEventData_V1*)eventData)->playerID;
			safetytime[playerID]=bz_getCurrentTime()+spawnsafetime; //Give a grace time on spawn.
		}break;
	}
}
