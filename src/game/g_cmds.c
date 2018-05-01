/*
 * Client command processing functions.
 */

#include "g_local.h"

qboolean G_IsOnFireteam(int entityNum, fireteamData_t **teamNum);

/*
==================
G_SendScore

Sends current scoreboard information
==================
*/
void G_SendScore(gentity_t *ent)
{
	char		entry[128];
	int			i;
	gclient_t	*cl;
	int			numSorted;
	int			team, size, count;
	char		buffer[1024];
	char		startbuffer[32];

	// send the latest information on all clients
	numSorted = level.numConnectedClients;
	if (numSorted > 64)
	{
		numSorted = 64;
	}

	i = 0;
	// Gordon: team doesnt actually mean team, ignore...
	for (team = 0; team < 2; team++)
	{
		*buffer = '\0';
		*startbuffer = '\0';
		if (team == 0)
		{
			Q_strncpyz(startbuffer, va("sc0 %i %i", level.teamScores[TEAM_AXIS], level.teamScores[TEAM_ALLIES]), 32);
		}
		else
		{
			Q_strncpyz(startbuffer, "sc1", 32);
		}
		size = strlen(startbuffer) + 1;
		count = 0;

		for (; i < numSorted ; i++)
		{
			int j, totalXP;
			int		ping, playerClass, respawnsLeft;

			cl = &level.clients[level.sortedClients[i]];

			if (g_entities[level.sortedClients[i]].r.svFlags & SVF_POW)
			{
				continue;
			}

			playerClass = cl->ps.stats[STAT_PLAYER_CLASS];

			// NERVE - SMF - number of respawns left
			respawnsLeft = cl->ps.persistant[PERS_RESPAWNS_LEFT];
			if ((respawnsLeft == 0 && ((cl->ps.pm_flags & PMF_LIMBO) || ((level.intermissiontime) && g_entities[level.sortedClients[i]].health <= 0))))
			{
				respawnsLeft = -2;
			}


			if (cl->pers.connected == CON_CONNECTING)
			{
				ping = -1;
			}
			else
			{
				ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
			}

			for (totalXP = 0, j = 0; j < SK_NUM_SKILLS; j++)
			{
				totalXP += cl->sess.skillpoints[j];
			}

			Com_sprintf(entry, sizeof(entry), " %i %i %i %i %i %i %i %i",
						level.sortedClients[i],
						totalXP,
						ping,
						(level.time - cl->pers.enterTime) / 60000,
						g_entities[level.sortedClients[i]].s.powerups,
						playerClass,
						respawnsLeft,
						cl->ps.clientNum);


			if (size + strlen(entry) > 1000)
			{
				break;
			}
			size += strlen(entry);

			Q_strcat(buffer, 1024, entry);
			if (++count >= 32)
			{
				i++; // we need to redo this client in the next buffer (if we can)
				break;
			}
		}

		if (count > 0 || team == 0)
		{
			trap_SendServerCommand(ent - g_entities, va("%s %i%s", startbuffer, count, buffer));
		}
	}
}

/*
==================
Cmd_Score_f

Request current scoreboard information
==================
*/
void Cmd_Score_f(gentity_t *ent)
{
	ent->client->wantsscore = qtrue;
	//	G_SendScore( ent );
}


// Clears all rockets belonging to a certain entity
void clearRockets(gentity_t *ent)
{
	// Can't see a use for clearing world's rockets
	if (!ent->client)
		return;

	if (ent->client->hasrockets > 0)
	{
		int i;
		for (i = 0; i < MAX_GENTITIES; i++)
		{
			gentity_t *wannabbolt;

			wannabbolt = &g_entities[i];
			if (wannabbolt->methodOfDeath == MOD_PANZERFAUST && wannabbolt->owner == ent->client->ps.clientNum)
			{
				wannabbolt->takedamage	= qfalse;
				wannabbolt->think		= G_FreeEntity;
				wannabbolt->nextthink	= level.time + 10;
			}
		}
		ent->client->hasrockets = 0;
	}
}


// two functions used for initial testing in cheat functions
qboolean AliveOk(gentity_t *ent)
{
	if (ent->health <= 0)
	{
		CP("print \"You must be alive to use this command.\n\"");
		return qfalse;
	}
	return qtrue;
}

qboolean CheatsOk(gentity_t *ent)
{
	if (!g_cheats.integer)
	{
		CP("print \"Cheats are not enabled on this server.\n\"");
		return qfalse;
	}
	return qtrue;
}


/*
==================
ConcatArgs
==================
*/
char	*ConcatArgs(int start)
{
	int		i, c, tlen;
	static char	line[MAX_STRING_CHARS];
	int		len;
	char	arg[MAX_STRING_CHARS];

	len = 0;
	c = trap_Argc();
	for (i = start ; i < c ; i++)
	{
		trap_Argv(i, arg, sizeof(arg));
		tlen = strlen(arg);
		if (len + tlen >= MAX_STRING_CHARS - 1)
		{
			break;
		}
		memcpy(line + len, arg, tlen);
		len += tlen;
		if (i != c - 1)
		{
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}

/*
==================
SanitizeString

Remove case and control characters
==================
*/
void SanitizeString(char *in, char *out, qboolean fToLower)
{
	while (*in)
	{
		if (*in == 27 || *in == '^')
		{
			in++;		// skip color code
			if (*in) in++;
			continue;
		}

		if (*in < 32)
		{
			in++;
			continue;
		}

		*out++ = (fToLower) ? tolower(*in++) : *in++;
	}

	*out = 0;
}

/*
==================
ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
int ClientNumberFromString(gentity_t *to, char *s)
{
	gclient_t	*cl;
	int			idnum;
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];
	qboolean	fIsNumber = qtrue;
	int			partialMatchs = 0;
	int			partialMatchId = -1;

	// See if its a number or string
	for (idnum = 0; idnum < strlen(s) && s[idnum] != 0; idnum++)
	{
		if (s[idnum] < '0' || s[idnum] > '9')
		{
			fIsNumber = qfalse;
			break;
		}
	}

	// check for a name match
	SanitizeString(s, s2, qtrue);
	for (idnum = 0, cl = level.clients; idnum < level.maxclients; idnum++, cl++)
	{
		if (cl->pers.connected != CON_CONNECTED) continue;

		SanitizeString(cl->pers.netname, n2, qtrue);
		if (!strcmp(n2, s2)) return(idnum);

		if (strstr(n2, s2) != NULL)
		{
			partialMatchs++;
			partialMatchId = idnum;
		}
	}

	// numeric values are just slot numbers
	if (fIsNumber)
	{
		idnum = atoi(s);
		if (idnum < 0 || idnum >= level.maxclients)
		{
			CPx(to - g_entities, va("print \"Bad client slot: [lof]%i\n\"", idnum));
			return -1;
		}

		cl = &level.clients[idnum];
		if (cl->pers.connected != CON_CONNECTED)
		{
			CPx(to - g_entities, va("print \"Client[lof] %i [lon]is not active\n\"", idnum));
			return -1;
		}
		return(idnum);
	}

	if (partialMatchs > 1)
	{
		CPx(to - g_entities, "print \"Several partial matches\n\"");
		return -1;
	}
	if (partialMatchs == 1)
		return partialMatchId;

	CPx(to - g_entities, va("print \"User [lof]%s [lon]is not on the server\n\"", s));
	return(-1);
}



/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f(gentity_t *ent)
{
	char		*name, *amt;
	int			i;
	qboolean	give_all;
	int			amount;
	qboolean	hasAmount = qfalse;

	if (!AliveOk(ent) || !CheatsOk(ent))
		return;

	//----(SA)	check for an amount (like "give health 30")
	amt = ConcatArgs(2);
	if (*amt)
		hasAmount = qtrue;
	amount = atoi(amt);
	//----(SA)	end

	name = ConcatArgs(1);

	if (Q_stricmp(name, "all") == 0)
		give_all = qtrue;
	else
		give_all = qfalse;

	if (Q_stricmpn(name, "skill", 5) == 0)
	{
		if (hasAmount)
		{
			if (amount >= 0 && amount < SK_NUM_SKILLS)
			{
				G_AddSkillPoints(ent, amount, 20);
				G_DebugAddSkillPoints(ent, amount, 20, "give skill");
			}
		}
		else
		{
			// bumps all skills with 1 level
			for (i = 0; i < SK_NUM_SKILLS; i++)
			{
				G_AddSkillPoints(ent, i, 20);
				G_DebugAddSkillPoints(ent, i, 20, "give skill");
			}
		}
		return;
	}

	if (Q_stricmpn(name, "medal", 5) == 0)
	{
		for (i = 0; i < SK_NUM_SKILLS; i++)
		{
			if (!ent->client->sess.medals[i])
				ent->client->sess.medals[i] = 1;
		}
		ClientUserinfoChanged(ent - g_entities);
		return;
	}

	if (give_all || Q_stricmpn(name, "health", 6) == 0)
	{
		//----(SA)	modified
		if (amount)
			ent->health += amount;
		else
			ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
		if (!give_all)
			return;
	}

	/*if ( Q_stricmpn( name, "damage", 6) == 0)
	{
		if(amount) {
			name = ConcatArgs( 3 );

			if( *name ) {
				int client = ClientNumberFromString( ent, name );
				if( client >= 0 ) {
					G_Damage( &g_entities[client], ent, ent, NULL, NULL, amount, DAMAGE_NO_PROTECTION, MOD_UNKNOWN );
				}
			} else {
				G_Damage( ent, ent, ent, NULL, NULL, amount, DAMAGE_NO_PROTECTION, MOD_UNKNOWN );
			}
		}

	    return;
	}*/

	if (give_all || Q_stricmp(name, "weapons") == 0)
	{
		for (i = 0; i < WP_NUM_WEAPONS; i++)
		{
			if (BG_WeaponInWolfMP(i))
				COM_BitSet(ent->client->ps.weapons, i);
		}

		if (!give_all)
			return;
	}

	if (give_all || Q_stricmpn(name, "ammo", 4) == 0)
	{
		if (amount)
		{
			if (ent->client->ps.weapon
					&& ent->client->ps.weapon != WP_SATCHEL && ent->client->ps.weapon != WP_SATCHEL_DET
			   )
				Add_Ammo(ent, ent->client->ps.weapon, amount, qtrue);
		}
		else
		{
			for (i = 1 ; i < WP_NUM_WEAPONS ; i++)
			{
				if (COM_BitCheck(ent->client->ps.weapons, i) && i != WP_SATCHEL && i != WP_SATCHEL_DET)
					Add_Ammo(ent, i, 9999, qtrue);
			}
		}

		if (!give_all)
			return;
	}

	//	"give allammo <n>" allows you to give a specific amount of ammo to /all/ weapons while
	//	allowing "give ammo <n>" to only give to the selected weap.
	if (Q_stricmpn(name, "allammo", 7) == 0 && amount)
	{
		for (i = 1 ; i < WP_NUM_WEAPONS; i++)
			Add_Ammo(ent, i, amount, qtrue);

		if (!give_all)
			return;
	}
}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f(gentity_t *ent)
{
	char	*msg;
	char	*name;
	qboolean godAll = qfalse;

	// Dini, only block god if BOTH cheats are off, and tjg_god is disabled.
	// FIXED: Never set godmode for spectators.
	if (!AliveOk(ent) || (!tjg_God.integer && !CheatsOk(ent))
			|| ent->client->sess.sessionTeam == TEAM_SPECTATOR)
		return;

	if (ent->client->sess.logged)
	{
		CP("cp \"You can not use god while logged in!\n\"");
		return;
	}

	name = ConcatArgs(1);

	// are we supposed to make all our teammates gods too?
	if (Q_stricmp(name, "all") == 0)
		godAll = qtrue;

	// can only use this cheat in single player
	if (godAll && g_gametype.integer == GT_SINGLE_PLAYER)
	{
		int j;
		qboolean settingFlag = qtrue;
		gentity_t *other;

		// are we turning it on or off?
		if (ent->flags & FL_GODMODE)
			settingFlag = qfalse;

		// loop through all players
		for (j = 0; j < level.maxclients; j++)
		{
			other = &g_entities[j];
			// if they're on the same team
			if (OnSameTeam(other, ent))
			{
				// set or clear the flag
				if (settingFlag)
					other->flags |= FL_GODMODE;
				else
					other->flags &= ~FL_GODMODE;
			}
		}
		if (settingFlag)
			msg = "godmode all ON\n";
		else
			msg = "godmode all OFF\n";
	}
	else
	{
		if (!Q_stricmp(name, "on") || atoi(name))
		{
			ent->flags |= FL_GODMODE;
		}
		else if (!Q_stricmp(name, "off") || !Q_stricmp(name, "0"))
		{
			ent->flags &= ~FL_GODMODE;
		}
		else
		{
			ent->flags ^= FL_GODMODE;
		}
		if (!(ent->flags & FL_GODMODE))
			msg = "godmode OFF\n";
		else
			msg = "godmode ON\n";
	}

	CP(va("print \"%s\"", msg));
}


/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f(gentity_t *ent)
{
	char	*msg;

	if (!AliveOk(ent) || !CheatsOk(ent))
		return;

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET))
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	CP(va("print \"%s\"", msg));
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f(gentity_t *ent)
{
	char	*msg;
	char	arg[MAX_TOKEN_CHARS];

	// Dini, allow noclip if tjg_Noclip 1
	if (!AliveOk(ent) || (!tjg_Noclip.integer && !CheatsOk(ent)))
		return;

	if (!ent->client->sess.allowNoclip)
	{
		CP("cp \"You are not allowed to use noclip!\n\"");
		return;
	}

	if (ent->client->sess.logged)
	{
		CP("cp \"You can not use noclip while logged in!\n\"");
		return;
	}

	trap_Argv(1, arg, sizeof(arg));

	if (!Q_stricmp(arg, "on") || atoi(arg))
		ent->client->noclip = qtrue;
	else if (!Q_stricmp(arg, "off") || !Q_stricmp(arg, "0"))
		ent->client->noclip = qfalse;
	else
		ent->client->noclip = !ent->client->noclip;

	if (ent->client->noclip)
		msg = "noclip ON";
	else
		msg = "noclip OFF";

	CP(va("print \"%s\n\"", msg));
}

/*
=================
Cmd_Kill_f
=================
*/
// Added simple 1 sec delay betweek kills to avoid crashing & excessive spamming.
void Cmd_Kill_f(gentity_t *ent)
{
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR ||
			(ent->client->ps.pm_flags & PMF_LIMBO) ||
			ent->health <= 0 || level.match_pause != PAUSE_NONE)
	{
		return;
	}

	trap_SendServerCommand(ent - g_entities, "timerun_stop");
	ent->client->ps.curWeapHeat = 0;

	//ent->flags &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
	ent->client->ps.persistant[PERS_HWEAPON_USE] = 0; // TTimo - if using /kill while at MG42
	player_die(ent, ent, ent, (g_gamestate.integer == GS_PLAYING) ? 100000 : 135, MOD_SUICIDE);

	// Maybe make more modifications to this "die" stuff eventually, but \care
	limbo(ent, qfalse);
}

void G_TeamDataForString(const char *teamstr, int clientNum, team_t *team, spectatorState_t *sState, int *specClient)
{
	*sState = SPECTATOR_NOT;
	if (!Q_stricmp(teamstr, "follow1"))
	{
		*team =		TEAM_SPECTATOR;
		*sState =	SPECTATOR_FOLLOW;
		if (specClient)
		{
			*specClient = -1;
		}
	}
	else if (!Q_stricmp(teamstr, "follow2"))
	{
		*team =		TEAM_SPECTATOR;
		*sState =	SPECTATOR_FOLLOW;
		if (specClient)
		{
			*specClient = -2;
		}
	}
	else if (!Q_stricmp(teamstr, "spectator") || !Q_stricmp(teamstr, "s"))
	{
		*team =		TEAM_SPECTATOR;
		*sState =	SPECTATOR_FREE;
	}
	else if (!Q_stricmp(teamstr, "red") || !Q_stricmp(teamstr, "r") || !Q_stricmp(teamstr, "axis"))
	{
		*team =		TEAM_AXIS;
	}
	else if (!Q_stricmp(teamstr, "blue") || !Q_stricmp(teamstr, "b") || !Q_stricmp(teamstr, "allies"))
	{
		*team =		TEAM_ALLIES;
	}
	else
	{
		*team = PickTeam(clientNum);
		if (!G_teamJoinCheck(*team, &g_entities[clientNum]))
		{
			*team = ((TEAM_AXIS | TEAM_ALLIES) & ~*team);
		}
	}
}

/*
=================
SetTeam
=================
*/
qboolean SetTeam(gentity_t *ent, char *s, qboolean force, weapon_t w1, weapon_t w2, qboolean setweapons)
{
	team_t				team, oldTeam;
	gclient_t			*client;
	int					clientNum;
	spectatorState_t	specState;
	int					specClient;
	int					respawnsLeft;

	//
	// see what change is requested
	//
	client = ent->client;

	clientNum = client - level.clients;
	specClient = 0;

	// ydnar: preserve respawn count
	respawnsLeft = client->ps.persistant[ PERS_RESPAWNS_LEFT ];

	G_TeamDataForString(s, client - level.clients, &team, &specState, &specClient);

	if (team != TEAM_SPECTATOR)
	{
		// Ensure the player can join
		if (!G_teamJoinCheck(team, ent))
		{
			// Leave them where they were before the command was issued
			return(qfalse);
		}

		if (g_noTeamSwitching.integer && (team != ent->client->sess.sessionTeam && ent->client->sess.sessionTeam != TEAM_SPECTATOR) && g_gamestate.integer == GS_PLAYING && !force)
		{
			trap_SendServerCommand(clientNum, "cp \"You cannot switch during a match, please wait until the round ends.\n\"");
			return qfalse;	// ignore the request
		}
	}

	if (g_maxGameClients.integer > 0 && level.numNonSpectatorClients >= g_maxGameClients.integer)
	{
		team = TEAM_SPECTATOR;
	}

	//
	// decide if we will allow the change
	//
	oldTeam = client->sess.sessionTeam;
	if (team == oldTeam && team != TEAM_SPECTATOR)
	{
		return qfalse;
	}

	// NERVE - SMF - prevent players from switching to regain deployments
	if (g_gametype.integer != GT_WOLF_LMS)
	{
		if ((g_maxlives.integer > 0 ||
				(g_alliedmaxlives.integer > 0 && ent->client->sess.sessionTeam == TEAM_ALLIES) ||
				(g_axismaxlives.integer > 0 && ent->client->sess.sessionTeam == TEAM_AXIS))

				&& ent->client->ps.persistant[PERS_RESPAWNS_LEFT] == 0 && oldTeam != TEAM_SPECTATOR)
		{
			CP("cp \"You can't switch teams because you are out of lives.\n\" 3");
			return qfalse;	// ignore the request
		}
	}

	// DHM - Nerve :: Force players to wait 30 seconds before they can join a new team.
	// OSP - changed to 5 seconds
	// Gordon: disabling if in dev mode: cuz it sucks
	// Gordon: bleh, half of these variables don't mean what they used to, so this doesn't work
	/*	if ( !g_cheats.integer ) {
			 if ( team != oldTeam && level.warmupTime == 0 && !client->pers.initialSpawn && ( (level.time - client->pers.connectTime) > 10000 ) && ( (level.time - client->pers.enterTime) < 5000 ) && !force ) {
				CP(va("cp \"^3You must wait %i seconds before joining ^3a new team.\n\" 3", (int)(5 - ((level.time - client->pers.enterTime)/1000))));
				return qfalse;
			}
		}*/
	// dhm

	//
	// execute the team change
	//


	// DHM - Nerve
	// OSP
	if (team != TEAM_SPECTATOR)
	{
		client->pers.initialSpawn = qfalse;
		// no MV in-game
		if (client->pers.mvCount > 0)
		{
			G_smvRemoveInvalidClients(ent, TEAM_AXIS);
			G_smvRemoveInvalidClients(ent, TEAM_ALLIES);
		}
	}

	// he starts at 'base'
	// RF, in single player, bots always use regular spawn points
	if (!((g_gametype.integer == GT_SINGLE_PLAYER || g_gametype.integer == GT_COOP) && (ent->r.svFlags & SVF_BOT)))
	{
		client->pers.teamState.state = TEAM_BEGIN;
	}

	if (oldTeam != TEAM_SPECTATOR)
	{
		if (!(ent->client->ps.pm_flags & PMF_LIMBO))
		{
			// Kill him (makes sure he loses flags, etc)
			ent->flags &= ~FL_GODMODE;
			ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
			player_die(ent, ent, ent, 100000, MOD_SWITCHTEAM);
		}
	}
	// they go to the end of the line for tournements
	if (team == TEAM_SPECTATOR)
	{
		client->sess.spectatorTime = level.time;
		if (!client->sess.referee) client->pers.invite = 0;
		if (team != oldTeam) G_smvAllRemoveSingleClient(ent - g_entities);
	}

	G_LeaveTank(ent, qfalse);
	G_RemoveClientFromFireteams(clientNum, qtrue, qfalse);
	if (g_landminetimeout.integer)
	{
		G_ExplodeMines(ent);
	}
	G_FadeItems(ent, MOD_SATCHEL);

	// remove ourself from teamlists
	{
		int i;
		mapEntityData_t	*mEnt;
		mapEntityData_Team_t *teamList;

		for (i = 0; i < 2; i++)
		{
			teamList = &mapEntityData[i];

			if ((mEnt = G_FindMapEntityData(&mapEntityData[0], ent - g_entities)) != NULL)
			{
				G_FreeMapEntityData(teamList, mEnt);
			}

			mEnt = G_FindMapEntityDataSingleClient(teamList, NULL, ent->s.number, -1);

			while (mEnt)
			{
				mapEntityData_t	*mEntFree = mEnt;

				mEnt = G_FindMapEntityDataSingleClient(teamList, mEnt, ent->s.number, -1);

				G_FreeMapEntityData(teamList, mEntFree);
			}
		}
	}
	client->sess.spec_team = 0;
	client->sess.sessionTeam = team;
	client->sess.spectatorState = specState;
	client->sess.spectatorClient = specClient;
	client->pers.ready = qfalse;

	// (l)users will spam spec messages... honest!
	if (team != oldTeam)
	{
		gentity_t *tent = G_PopupMessage(PM_TEAM);
		tent->s.effect2Time = team;
		tent->s.effect3Time = clientNum;
		tent->s.density = 0;
	}

	if (setweapons)
	{
		G_SetClientWeapons(ent, w1, w2, qfalse);
	}

	// get and distribute relevent paramters
	G_UpdateCharacter(client);			// FIXME : doesn't ClientBegin take care of this already?
	ClientUserinfoChanged(clientNum);

	ClientBegin(clientNum);

	// ydnar: restore old respawn count (players cannot jump from team to team to regain lives)
	if (respawnsLeft >= 0 && oldTeam != TEAM_SPECTATOR)
	{
		client->ps.persistant[ PERS_RESPAWNS_LEFT ] = respawnsLeft;
	}

	G_UpdateSpawnCounts();

	if (g_gamestate.integer == GS_PLAYING && (client->sess.sessionTeam == TEAM_AXIS || client->sess.sessionTeam == TEAM_ALLIES))
	{
		int i;
		int x = client->sess.sessionTeam - TEAM_AXIS;

		for (i = 0; i < MAX_COMMANDER_TEAM_SOUNDS; i++)
		{
			if (level.commanderSounds[ x ][ i ].index)
			{
				gentity_t *tent = G_TempEntity(client->ps.origin, EV_GLOBAL_CLIENT_SOUND);
				tent->s.eventParm = level.commanderSounds[ x ][ i ].index - 1;
				tent->s.teamNum = clientNum;
			}
		}
	}

	ent->client->pers.autofireteamCreateEndTime = 0;
	ent->client->pers.autofireteamJoinEndTime = 0;

	return qtrue;
}

/*
=================
StopFollowing

If the client being followed leaves the game, or you just want to drop
to free floating spectator mode
=================
*/
void StopFollowing(gentity_t *ent)
{
	// ATVI Wolfenstein Misc #474
	// divert behaviour if TEAM_SPECTATOR, moved the code from SpectatorThink to put back into free fly correctly
	// (I am not sure this can be called in non-TEAM_SPECTATOR situation, better be safe)
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		// drop to free floating, somewhere above the current position (that's the client you were following)
		vec3_t pos, angle;
		gclient_t	*client = ent->client;
		VectorCopy(client->ps.origin, pos);
		//		pos[2] += 16; // Gordon: removing for now
		VectorCopy(client->ps.viewangles, angle);
		// Need this as it gets spec mode reset properly
		SetTeam(ent, "s", qtrue, -1, -1, qfalse);
		VectorCopy(pos, client->ps.origin);
		SetClientViewAngle(ent, angle);
	}
	else
	{
		// legacy code, FIXME: useless?
		// Gordon: no this is for limbo i'd guess
		ent->client->sess.spectatorState = SPECTATOR_FREE;
		ent->client->ps.clientNum = ent - g_entities;
	}
}

int G_NumPlayersWithWeapon(weapon_t weap, team_t team)
{
	int i, j, cnt = 0;

	for (i = 0; i < level.numConnectedClients; i++)
	{
		j = level.sortedClients[i];

		if (level.clients[j].sess.playerType != PC_SOLDIER)
		{
			continue;
		}

		if (level.clients[j].sess.sessionTeam != team)
		{
			continue;
		}

		if (level.clients[j].sess.latchPlayerWeapon != weap && level.clients[j].sess.playerWeapon != weap)
		{
			continue;
		}

		cnt++;
	}

	return cnt;
}

int G_NumPlayersOnTeam(team_t team)
{
	int i, j, cnt = 0;

	for (i = 0; i < level.numConnectedClients; i++)
	{
		j = level.sortedClients[i];

		if (level.clients[j].sess.sessionTeam != team)
		{
			continue;
		}

		cnt++;
	}

	return cnt;
}

qboolean G_IsHeavyWeapon(weapon_t weap)
{
	int i;

	for (i = 0; i < NUM_HEAVY_WEAPONS; i++)
	{
		if (bg_heavyWeapons[i] == weap)
		{
			return qtrue;
		}
	}

	return qfalse;
}

int G_TeamCount(gentity_t *ent, weapon_t weap)
{
	int i, j, cnt;

	if (weap == -1)    // we aint checking for a weapon, so always include ourselves
	{
		cnt = 1;
	}
	else     // we ARE checking for a weapon, so ignore ourselves
	{
		cnt = 0;
	}

	for (i = 0; i < level.numConnectedClients; i++)
	{
		j = level.sortedClients[i];

		if (j == ent - g_entities)
		{
			continue;
		}

		if (level.clients[j].sess.sessionTeam != ent->client->sess.sessionTeam)
		{
			continue;
		}

		if (weap != -1)
		{
			if (level.clients[j].sess.playerWeapon != weap && level.clients[j].sess.latchPlayerWeapon != weap)
			{
				continue;
			}
		}

		cnt++;
	}

	return cnt;
}

qboolean G_IsWeaponDisabled(gentity_t *ent, weapon_t weapon)
{
	int count, wcount;

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		return qtrue;
	}

	if (!G_IsHeavyWeapon(weapon))
	{
		return qfalse;
	}

	count =		G_TeamCount(ent, -1);
	wcount =	G_TeamCount(ent, weapon);

	if (wcount >= ceil(count * g_heavyWeaponRestriction.integer * 0.01f))
	{
		return qtrue;
	}

	return qfalse;
}

void G_SetClientWeapons(gentity_t *ent, weapon_t w1, weapon_t w2, qboolean updateclient)
{
	qboolean changed = qfalse;

	if (ent->client->sess.latchPlayerWeapon2 != w2)
	{
		ent->client->sess.latchPlayerWeapon2 = w2;
		changed = qtrue;
	}

	if (!G_IsWeaponDisabled(ent, w1))
	{
		if (ent->client->sess.latchPlayerWeapon != w1)
		{
			ent->client->sess.latchPlayerWeapon = w1;
			changed = qtrue;
		}
	}
	else
	{
		if (ent->client->sess.latchPlayerWeapon != 0)
		{
			ent->client->sess.latchPlayerWeapon = 0;
			changed = qtrue;
		}
	}

	if (updateclient && changed)
	{
		ClientUserinfoChanged(ent - g_entities);
	}
}

void Cmd_Team_f(gentity_t *ent)
{
	char		s[MAX_TOKEN_CHARS];
	char		ptype[4];
	char		weap[4], weap2[4];
	weapon_t	w, w2;

	if (trap_Argc() < 2)
	{
		CP("print \"usage: team <b|r|s|none>\n\"");
		return;
	}

	trap_SendServerCommand(ent - g_entities, "timerun_stop");

	trap_Argv(1, s,		sizeof(s));
	trap_Argv(2, ptype,	sizeof(ptype));
	trap_Argv(3, weap,		sizeof(weap));
	trap_Argv(4, weap2,	sizeof(weap2));

	w =		atoi(weap);
	w2 =	atoi(weap2);

	ent->client->sess.latchPlayerType =	atoi(ptype);
	if (ent->client->sess.latchPlayerType < PC_SOLDIER || ent->client->sess.latchPlayerType > PC_COVERTOPS)
		ent->client->sess.latchPlayerType = PC_SOLDIER;

	if (!SetTeam(ent, s, qfalse, w, w2, qtrue))
	{
		G_SetClientWeapons(ent, w, w2, qtrue);
	}
}

void Cmd_ResetSetup_f(gentity_t *ent)
{
	qboolean changed = qfalse;

	if (!ent || !ent->client)
	{
		return;
	}

	ent->client->sess.latchPlayerType =		ent->client->sess.playerType;

	if (ent->client->sess.latchPlayerWeapon != ent->client->sess.playerWeapon)
	{
		ent->client->sess.latchPlayerWeapon = ent->client->sess.playerWeapon;
		changed = qtrue;
	}

	if (ent->client->sess.latchPlayerWeapon2 != ent->client->sess.playerWeapon2)
	{
		ent->client->sess.latchPlayerWeapon2 =	ent->client->sess.playerWeapon2;
		changed = qtrue;
	}

	if (changed)
	{
		ClientUserinfoChanged(ent - g_entities);
	}
}

/*
 * Figures out if we are allowed to follow a given client.
 */
qboolean G_AllowFollow(gentity_t *ent, gentity_t *other)
{
	return !other->client->sess.specLocked
		   || COM_BitCheck(other->client->sess.specInvitedClients, ent - g_entities);
}

/*
 * Figures out if we are allowed & want to follow a given client.
 */
qboolean G_DesiredFollow(gentity_t *ent, gentity_t *other)
{
	return G_AllowFollow(ent, other)
		   && (ent->client->sess.spec_team == 0
			   || ent->client->sess.spec_team == other->client->sess.sessionTeam);
}

// END Mad Doc - TDF
/*
=================
Cmd_Follow_f
=================
*/
void Cmd_Follow_f(gentity_t *ent, unsigned int dwCommand, qboolean fValue)
{
	int		i;
	char	arg[MAX_TOKEN_CHARS];

	if (trap_Argc() != 2)
	{
		if (ent->client->sess.spectatorState == SPECTATOR_FOLLOW)
		{
			StopFollowing(ent);
		}
		return;
	}

	if (ent->client->ps.pm_flags & PMF_LIMBO)
	{
		CP("cpm \"You can not issue a follow command while in limbo.\n\"");
		CP("cpm \"Hit FIRE to switch between teammates.\n\"");
		return;
	}

	trap_Argv(1, arg, sizeof(arg));
	i = ClientNumberFromString(ent, arg);
	if (i == -1)
	{
		if (!Q_stricmp(arg, "allies")) i = TEAM_ALLIES;
		else if (!Q_stricmp(arg, "axis")) i = TEAM_AXIS;
		else return;

		if (!TeamCount(ent - g_entities, i))
		{
			CP(va("print \"The %s team %s empty!  Follow command ignored.\n\"", aTeams[i],
				  ((ent->client->sess.sessionTeam != i) ? "is" : "would be")));
			return;
		}

		// Allow for simple toggle
		if (ent->client->sess.spec_team != i)
		{
			ent->client->sess.spec_team = i;
			CP(va("print \"Spectator follow is now locked on the %s team.\n\"", aTeams[i]));
			Cmd_FollowCycle_f(ent, 1);
		}
		else
		{
			ent->client->sess.spec_team = 0;
			CP(va("print \"%s team spectating is now disabled.\n\"", aTeams[i]));
		}

		return;
	}

	// can't follow self
	if (&level.clients[ i ] == ent->client) return;

	// can't follow another spectator
	if (level.clients[ i ].sess.sessionTeam == TEAM_SPECTATOR) return;
	if (level.clients[ i ].ps.pm_flags & PMF_LIMBO) return;

	// can't follow a speclocked client, unless allowed
	if (!G_AllowFollow(ent, g_entities + i) && !ent->client->sess.freeSpec)
	{
		CP(va("print \"Sorry, player %s ^7is locked from spectators.\n\"", level.clients[i].pers.netname));
		return;
	}

	// first set them to spectator
	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		SetTeam(ent, "spectator", qfalse, -1, -1, qfalse);
	}

	ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
	ent->client->sess.spectatorClient = i;
}

/*
=================
Cmd_FollowCycle_f
=================
*/
void Cmd_FollowCycle_f(gentity_t *ent, int dir)
{
	int		clientNum;
	int		original;

	// first set them to spectator
	if ((ent->client->sess.spectatorState == SPECTATOR_NOT) && (!(ent->client->ps.pm_flags & PMF_LIMBO)))       // JPW NERVE for limbo state
	{
		SetTeam(ent, "spectator", qfalse, -1, -1, qfalse);
	}

	if (dir != 1 && dir != -1)
	{
		G_Error("Cmd_FollowCycle_f: bad dir %i", dir);
	}

	clientNum = ent->client->sess.spectatorClient;
	original = clientNum;
	do
	{
		clientNum += dir;
		if (clientNum >= level.maxclients)
		{
			clientNum = 0;
		}
		if (clientNum < 0)
		{
			clientNum = level.maxclients - 1;
		}

		// can only follow connected clients
		if (level.clients[ clientNum ].pers.connected != CON_CONNECTED)
		{
			continue;
		}

		// can't follow another spectator
		if (level.clients[ clientNum ].sess.sessionTeam == TEAM_SPECTATOR)
		{
			continue;
		}

		// JPW NERVE -- couple extra checks for limbo mode
		if (ent->client->ps.pm_flags & PMF_LIMBO)
		{
			if (level.clients[clientNum].ps.pm_flags & PMF_LIMBO)
				continue;
			if (level.clients[clientNum].sess.sessionTeam != ent->client->sess.sessionTeam)
				continue;
		}

		if (level.clients[clientNum].ps.pm_flags & PMF_LIMBO)
			continue;

		// OSP
		if (!G_DesiredFollow(ent, g_entities + clientNum) && !ent->client->sess.freeSpec)
			continue;

		// this is good, we can use it
		ent->client->sess.spectatorClient = clientNum;
		ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		return;
	}
	while (clientNum != original);

	// leave it where it was
}


/*======================
G_EntitySound
	Mad Doc xkan, 11/06/2002 -

	Plays a sound (wav file or sound script) on this entity

	Note that calling G_AddEvent(..., EV_GENERAL_SOUND, ...) has the danger of
	the event never getting through to the client because the entity might not
	be visible (unless it has the SVF_BROADCAST flag), so if you want to make sure
	the sound is heard, call this function instead.
======================*/
void G_EntitySound(
	gentity_t *ent,			// entity to play the sound on
	const char *soundId,	// sound file name or sound script ID
	int volume)				// sound volume, only applies to sound file name call
//   for sound script, volume is currently always 127.
{
	trap_SendServerCommand(-1, va("entitySound %d %s %d %i %i %i normal", ent->s.number, soundId, volume,
								  (int)ent->s.pos.trBase[0], (int)ent->s.pos.trBase[1], (int)ent->s.pos.trBase[2]));
}

/*======================
G_EntitySoundNoCut
	Mad Doc xkan, 1/16/2003 -

	Similar to G_EntitySound, but do not cut this sound off

======================*/
void G_EntitySoundNoCut(
	gentity_t *ent,			// entity to play the sound on
	const char *soundId,	// sound file name or sound script ID
	int volume)				// sound volume, only applies to sound file name call
//   for sound script, volume is currently always 127.
{
	trap_SendServerCommand(-1, va("entitySound %d %s %d %i %i %i noCut", ent->s.number, soundId, volume,
								  (int)ent->s.pos.trBase[0], (int)ent->s.pos.trBase[1], (int)ent->s.pos.trBase[2]));
}




/*
Admin system crap
*/
void G_AdminStuff(gentity_t *ent, const char *text)
{
	gclient_t *client = ent->client;
	char cmd[MAX_TOKEN_CHARS], arg1[MAX_TOKEN_CHARS], arg2[MAX_TOKEN_CHARS], full[MAX_TOKEN_CHARS];
	int i, levels, minlev = 0, len;
	char cleancmd[MAX_TOKEN_CHARS];
	char *levelname;

	if (!tjg_admin.integer)
		return;

	// Will be e.g.
	// 0   cmd		 arg1    arg2
	// say !setlevel someguy 5
	trap_Argv(1, cmd, sizeof(cmd));		// !setlevel
	trap_Argv(2, arg1, sizeof(arg1));	// someguy
	trap_Argv(3, arg2, sizeof(arg2));	// 5
	// Full: !setlevel someguy 5
	Com_sprintf(full, sizeof(full), "%s", ConcatArgs(2));

	levels = level.admin.lvl.count;
	minlev = -1;

	// This is shitty made I guess, but I can't seem to find a goddamn replacement for it :s
	len = strlen(cmd);
	for (i = 0; i < len; i++)
	{
		cleancmd[i] = cmd[1 + i];	// Cut off first char
	}

	for (i = 1; i <= levels; i++)
	{
		if (strstr(level.admin.lvl.cmds[i], cleancmd) != NULL)
		{
			minlev = i;
			continue;
		}
	}

	// Level 0 stuff
	if (!strcmp(cleancmd, "admintest") && !strlen(arg1))
	{
		minlev = 0;
		if (client->pers.admin)
			levelname = level.admin.lvl.name[client->pers.admin];
		else
			levelname = level.admin.lvl.name[0];

		AP(va("print \"^3Admintest:^7 %s^7 %s (level %i)\n\"", client->pers.netname, levelname, client->pers.admin));
		return;
	}
	else if (!strcmp(cleancmd, "listcmds") || !strcmp(cleancmd, "cmds") || !strcmp(cleancmd, "commands") || !strcmp(cleancmd, "list") || !strcmp(cleancmd, "cmdlist"))
	{
		minlev = 0;

		for (i = 1; i <= levels; i++)
		{
			CP(va("print \"Level %i (%s^7): %s\n\"", i, level.admin.lvl.name[i], level.admin.lvl.cmds[i]));
		}
		return;
	}
	else if (!strcmp(cleancmd, "help"))
	{
		minlev = 0;

		CP("print \"Use ^3/help^7 for help and information.\n\"");
		return;
	}

	// Add any others which will be sent on if > X arguments etc
	if (!strcmp(cleancmd, "admintest"))
		minlev = 0;

	// If it's an unknown command, act like nothing happened
	if (minlev == -1)
	{
		CP(va("print \"^1Unknown admin command:^7 %s\n\"", cleancmd));
		return;
	}

	Q_strncpyz(full, va("%s %s", cleancmd, full), sizeof(full));

	if (client->pers.admin < minlev)
	{
		CP(va("print \"^1Insufficient Level:^7 You need to be at least a level ^3%i^7 admin to use the ^3%s^7 command.\n\"", minlev, cmd));
		return;
	}
	// Run a check to see if intended player has higher\same admin level already.
	else if (!strcmp(cleancmd, "setlevel"))
	{
		gentity_t *other;
		int clientNum;

		clientNum = ClientNumberFromStringR(arg1);
		if (clientNum == -1)
		{
			CP("print \"^1Setlevel:^7 Invalid player specified.\n^3Usage:^7 setlevel <player> <level>\n\"");
			return;
		}
		other = g_entities + clientNum;

		if (other->client->pers.admin >= client->pers.admin && !(other == ent))
		{
			CP("print \"^1Setlevel:^7 Can't setlevel players of equal or higher level.\n\"");
			return;
		}
	}

	if (strchr(full, ';'))
	{
		CP("print \"Invalid Command: semicolons are not allowed.\n\"");
		return;
	}

	CP(va("print \"Sending: %s\n\"", full));
	trap_SendConsoleCommand(EXEC_APPEND, va("%s", full));
}



/*
==================
G_Say
==================
*/
#define	MAX_SAY_TEXT	150

void G_SayTo(gentity_t *ent, gentity_t *other, int mode, int color,
			 const char *name, const char *message, qboolean localize, qboolean encoded)
{
	char *cmd;

	if (!other || !other->inuse || !other->client)
	{
		return;
	}
	if ((mode == SAY_TEAM || mode == SAY_TEAMNL) && !OnSameTeam(ent, other))
	{
		return;
	}

	// NERVE - SMF - if spectator, no chatting to players in WolfMP
	if (match_mutespecs.integer > 0 && ent->client->sess.referee == 0 &&	// OSP
			((ent->client->sess.sessionTeam == TEAM_FREE && other->client->sess.sessionTeam != TEAM_FREE) ||
			 (ent->client->sess.sessionTeam == TEAM_SPECTATOR && other->client->sess.sessionTeam != TEAM_SPECTATOR)))
	{
		return;
	}
	else
	{
		if (mode == SAY_BUDDY)   	// send only to people who have the sender on their buddy list
		{
			if (ent->s.clientNum != other->s.clientNum)
			{
				fireteamData_t *ft1, *ft2;
				if (!G_IsOnFireteam(other - g_entities, &ft1))
				{
					return;
				}
				if (!G_IsOnFireteam(ent - g_entities, &ft2))
				{
					return;
				}
				if (ft1 != ft2)
				{
					return;
				}
			}
		}

		if (encoded)
			cmd = mode == SAY_TEAM || mode == SAY_BUDDY ? "enc_tchat" : "enc_chat";
		else
			cmd = mode == SAY_TEAM || mode == SAY_BUDDY ? "tchat" : "chat";

		trap_SendServerCommand(other - g_entities, va("%s \"%s%c%c%s\" %i %i", cmd, name, Q_COLOR_ESCAPE, color, message, ent - g_entities, localize));
	}
}

void G_Say(gentity_t *ent, gentity_t *target, int mode, qboolean encoded, const char *chatText)
{
	int			j;
	gentity_t	*other;
	int			color;
	char		name[64];
	// don't let text be too long for malicious reasons
	char		text[MAX_SAY_TEXT];
	qboolean	localize = qfalse;
	char		*loc;

	switch (mode)
	{
		default:
		case SAY_ALL:
			G_LogPrintf("say: %s: %s\n", ent->client->pers.netname, chatText);
			Com_sprintf(name, sizeof(name), "%s%c%c: ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE);
			color = COLOR_GREEN;
			break;
		case SAY_BUDDY:
			localize = qtrue;
			G_LogPrintf("saybuddy: %s: %s\n", ent->client->pers.netname, chatText);
			loc = BG_GetLocationString(ent->r.currentOrigin);
			Com_sprintf(name, sizeof(name), "[lof](%s%c%c) (%s): ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, loc);
			color = COLOR_YELLOW;
			break;
		case SAY_TEAM:
			localize = qtrue;
			G_LogPrintf("sayteam: %s: %s\n", ent->client->pers.netname, chatText);
			loc = BG_GetLocationString(ent->r.currentOrigin);
			Com_sprintf(name, sizeof(name), "[lof](%s%c%c) (%s): ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, loc);
			color = COLOR_CYAN;
			break;
		case SAY_TEAMNL:
			G_LogPrintf("sayteamnl: %s: %s\n", ent->client->pers.netname, chatText);
			Com_sprintf(name, sizeof(name), "(%s^7): ", ent->client->pers.netname);
			color = COLOR_CYAN;
			break;
	}

	Q_strncpyz(text, chatText, sizeof(text));

	if (target)
	{
		if (!COM_BitCheck(target->client->sess.ignoreClients, ent - g_entities))
		{
			G_SayTo(ent, target, mode, color, name, text, localize, encoded);
		}
		return;
	}

	// echo the text to the console
	if (g_dedicated.integer)
	{
		G_Printf("%s%s\n", name, text);
	}

	// send it to all the apropriate clients
	for (j = 0; j < level.numConnectedClients; j++)
	{
		other = &g_entities[level.sortedClients[j]];
		if (!COM_BitCheck(other->client->sess.ignoreClients, ent - g_entities))
		{
			G_SayTo(ent, other, mode, color, name, text, localize, encoded);
		}
	}

	if (tjg_admin.integer)
		if (text[0] == '!')
			G_AdminStuff(ent, text);
}

/*
==================
Cmd_Say_f
==================
*/
void Cmd_Say_f(gentity_t *ent, int mode, qboolean arg0, qboolean encoded)
{
	if (trap_Argc() < 2 && !arg0) return;
	G_Say(ent, NULL, mode, encoded, ConcatArgs(((arg0) ? 0 : 1)));
}


// NERVE - SMF
void G_VoiceTo(gentity_t *ent, gentity_t *other, int mode, const char *id, qboolean voiceonly)
{
	int color;
	char *cmd;

	if (!other)
	{
		return;
	}
	if (!other->inuse)
	{
		return;
	}
	if (!other->client)
	{
		return;
	}
	if (mode == SAY_TEAM && !OnSameTeam(ent, other))
	{
		return;
	}

	// OSP - spec vchat rules follow the same as normal chatting rules
	if (match_mutespecs.integer > 0 && ent->client->sess.referee == 0 &&
			ent->client->sess.sessionTeam == TEAM_SPECTATOR && other->client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		return;
	}

	// send only to people who have the sender on their buddy list
	if (mode == SAY_BUDDY)
	{
		if (ent->s.clientNum != other->s.clientNum)
		{
			fireteamData_t *ft1, *ft2;
			if (!G_IsOnFireteam(other - g_entities, &ft1))
			{
				return;
			}
			if (!G_IsOnFireteam(ent - g_entities, &ft2))
			{
				return;
			}
			if (ft1 != ft2)
			{
				return;
			}
		}
	}

	if (mode == SAY_TEAM)
	{
		color = COLOR_CYAN;
		cmd = "vtchat";
	}
	else if (mode == SAY_BUDDY)
	{
		color = COLOR_YELLOW;
		cmd = "vbchat";
	}
	else
	{
		color = COLOR_GREEN;
		cmd = "vchat";
	}

	if (voiceonly == 2)
	{
		voiceonly = qfalse;
	}

	if (mode == SAY_TEAM || mode == SAY_BUDDY)
	{
		CPx(other - g_entities, va("%s %d %d %d %s %i %i %i", cmd, voiceonly, ent - g_entities, color, id, (int)ent->s.pos.trBase[0], (int)ent->s.pos.trBase[1], (int)ent->s.pos.trBase[2]));
	}
	else
	{
		CPx(other - g_entities, va("%s %d %d %d %s", cmd, voiceonly, ent - g_entities, color, id));
	}
}









void G_Voice(gentity_t *ent, gentity_t *target, int mode, const char *id, qboolean voiceonly)
{
	int			j;

	// DHM - Nerve :: Don't allow excessive spamming of voice chats
	ent->voiceChatSquelch -= (level.time - ent->voiceChatPreviousTime);
	ent->voiceChatPreviousTime = level.time;

	if (ent->voiceChatSquelch < 0)
		ent->voiceChatSquelch = 0;

	// Only do the spam check for MP
	if (ent->voiceChatSquelch >= 30000)
	{
		trap_SendServerCommand(ent - g_entities, "cpm \"^1Spam Protection^7: VoiceChat ignored\n\"");
		return;
	}

	if (g_voiceChatsAllowed.integer)
		ent->voiceChatSquelch += (34000 / g_voiceChatsAllowed.integer);
	else
		return;
	// dhm

	// OSP - Charge for the lame spam!
	/*if(mode == SAY_ALL && (!Q_stricmp(id, "DynamiteDefused") || !Q_stricmp(id, "DynamitePlanted"))) {
		return;
	}*/

	if (target)
	{
		G_VoiceTo(ent, target, mode, id, voiceonly);
		return;
	}

	// echo the text to the console
	if (g_dedicated.integer)
	{
		G_Printf("voice: %s %s\n", ent->client->pers.netname, id);
	}

	if (mode == SAY_BUDDY)
	{
		char buffer[32];
		int	cls = -1, i, cnt, num;
		qboolean allowclients[MAX_CLIENTS];

		memset(allowclients, 0, sizeof(allowclients));

		trap_Argv(1, buffer, 32);

		cls = atoi(buffer);

		trap_Argv(2, buffer, 32);
		cnt = atoi(buffer);
		if (cnt > MAX_CLIENTS)
		{
			cnt = MAX_CLIENTS;
		}

		for (i = 0; i < cnt; i++)
		{
			trap_Argv(3 + i, buffer, 32);

			num = atoi(buffer);
			if (num < 0)
			{
				continue;
			}
			if (num >= MAX_CLIENTS)
			{
				continue;
			}

			allowclients[ num ] = qtrue;
		}

		for (j = 0; j < level.numConnectedClients; j++)
		{

			if (level.sortedClients[j] != ent->s.clientNum)
			{
				if (cls != -1 && cls != level.clients[level.sortedClients[j]].sess.playerType)
				{
					continue;
				}
			}

			if (cnt)
			{
				if (!allowclients[ level.sortedClients[j] ])
				{
					continue;
				}
			}

			G_VoiceTo(ent, &g_entities[level.sortedClients[j]], mode, id, voiceonly);
		}
	}
	else
	{

		// send it to all the apropriate clients
		for (j = 0; j < level.numConnectedClients; j++)
		{
			G_VoiceTo(ent, &g_entities[level.sortedClients[j]], mode, id, voiceonly);
		}
	}
}

/*
==================
Cmd_Voice_f
==================
*/
static void Cmd_Voice_f(gentity_t *ent, int mode, qboolean arg0, qboolean voiceonly)
{
	if (mode != SAY_BUDDY)
	{
		if (trap_Argc() < 2 && !arg0)
		{
			return;
		}
		G_Voice(ent, NULL, mode, ConcatArgs(((arg0) ? 0 : 1)), voiceonly);
	}
	else
	{
		char buffer[16];
		int index;

		trap_Argv(2, buffer, sizeof(buffer));
		index = atoi(buffer);
		if (index < 0)
		{
			index = 0;
		}

		if (trap_Argc() < 3 + index && !arg0)
		{
			return;
		}
		G_Voice(ent, NULL, mode, ConcatArgs(((arg0) ? 2 + index : 3 + index)), voiceonly);
	}
}

/*
==================
Cmd_Where_f
==================
*/
void Cmd_Where_f(gentity_t *ent)
{
	trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", vtos(ent->s.origin)));
}

/*
==================
Cmd_CallVote_f
==================
*/
qboolean Cmd_CallVote_f(gentity_t *ent, unsigned int dwCommand, qboolean fRefCommand)
{
	int		i;
	char	arg0[MAX_STRING_TOKENS];
	char	arg1[MAX_STRING_TOKENS];
	char	arg2[MAX_STRING_TOKENS];

	// Normal checks, if its not being issued as a referee command
	if (!fRefCommand)
	{
		if (level.voteInfo.voteTime)
		{
			CP("cpm \"A vote is already in progress.\n\"");
			return qfalse;
		}
		else if (level.voteInfo.vote_passtime > 0)
		{
			CP("cpm \"A vote is already in queue to be performed.\n\"");
			return qfalse;
		}
		else if (level.intermissiontime)
		{
			CP("cpm \"Cannot callvote during intermission.\n\"");
			return qfalse;
		}
		else if (!ent->client->sess.referee)
		{
			if (voteFlags.integer == VOTING_DISABLED)
			{
				CP("cpm \"Voting is not enabled on this server.\n\"");
				return qfalse;
			}
			else if (vote_limit.integer > 0 && ent->client->pers.voteCount >= vote_limit.integer)
			{
				CP(va("cpm \"You have already called the maximum number of votes (%d).\n\"", vote_limit.integer));
				return qfalse;
			}
			else if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
			{
				CP("cpm \"You're not allowed to call a vote as a spectator.\n\"");
				return qfalse;
			}
		}
	}

	// make sure it is a valid command to vote on
	trap_Argv(0, arg0, sizeof(arg0));
	trap_Argv(1, arg1, sizeof(arg1));
	trap_Argv(2, arg2, sizeof(arg2));

	if (strchr(arg1, ';') || strchr(arg2, ';')
			|| strchr(arg1, '\r') || strchr(arg2, '\r')
			|| strchr(arg1, '\n') || strchr(arg2, '\n'))
	{
		char *strCmdBase = (!fRefCommand) ? "vote" : "ref command";

		G_refPrintf(ent, "Invalid %s string.", strCmdBase);
		return(qfalse);
	}

	// Check if the map exists.
	if (!Q_stricmp(arg1, "map"))
	{
		char			mapfile[MAX_QPATH];
		fileHandle_t    f;
		int				len;

		if (arg2[0] == '\0' || trap_Argc() == 1)
		{
			CP("print \"^1Callvote:^7 No map specified.\n\"");
			return qfalse;
		}

		Com_sprintf(mapfile, sizeof(mapfile), "maps/%s.bsp", arg2);

		len = trap_FS_FOpenFile(mapfile, &f, FS_READ);

		trap_FS_FCloseFile(f);

		if (!f)
		{
			CP(va("print \"^1Callvote:^7 The map ^3%s^7 is not on the server.\n\"", arg2));
			return qfalse;
		}

		if (strstr(Q_strlwr(tjg_blockedMaps.string), Q_strlwr(arg2)) != NULL)
		{
			CP(va("print \"^1Callvote:^7 This server doesn't allow voting for ^3%s^7\n\"", arg2));
			return qfalse;
		}
	}


	if (trap_Argc() > 1 && (i = G_voteCmdCheck(ent, arg1, arg2, fRefCommand)) != G_NOTFOUND)  	//  --OSP
	{
		if (i != G_OK)
		{
			if (i == G_NOTFOUND) return(qfalse);	// Command error
			else return(qtrue);
		}
	}
	else
	{
		if (!fRefCommand)
		{
			CP(va("print \"\n^3>>> Unknown vote command: ^7%s %s\n\"", arg1, arg2));
			G_voteHelp(ent, qtrue);
		}
		return(qfalse);
	}

	Com_sprintf(level.voteInfo.voteString, sizeof(level.voteInfo.voteString), "%s %s", arg1, arg2);

	// start the voting, the caller automatically votes yes
	// If a referee, vote automatically passes.	// OSP
	if (fRefCommand)
	{
		//		level.voteInfo.voteYes = level.voteInfo.numVotingClients + 10;	// JIC :)
		// Don't announce some votes, as in comp mode, it is generally a ref
		// who is policing people who shouldn't be joining and players don't want
		// this sort of spam in the console
		if (level.voteInfo.vote_fn != G_Kick_v && level.voteInfo.vote_fn != G_Mute_v)
		{
			AP("cp \"^1** Referee Server Setting Change **\n\"");
		}

		// Gordon: just call the stupid thing.... don't bother with the voting faff
		level.voteInfo.vote_fn(NULL, 0, NULL, NULL, qfalse);

		G_globalSound("sound/misc/referee.wav");
	}
	else
	{
		level.voteInfo.voteYes = 1;
		AP(va("print \"[lof]%s^7 [lon]called a vote.[lof]  Voting for: %s\n\"", ent->client->pers.netname, level.voteInfo.voteString));
		AP(va("cp \"[lof]%s\n^7[lon]called a vote.\n\"", ent->client->pers.netname));
		G_globalSound("sound/misc/vote.wav");
	}

	level.voteInfo.voter_team = ent->client->sess.sessionTeam;
	level.voteInfo.voter_cn = ent - g_entities;
	level.voteInfo.voteTime = level.time;
	level.voteInfo.voteNo = 0;

	// Don't send the vote info if a ref initiates (as it will automatically pass)
	if (!fRefCommand)
	{
		for (i = 0; i < level.numConnectedClients; i++)
		{
			level.clients[level.sortedClients[i]].ps.eFlags &= ~EF_VOTED;
		}

		if (!ent->client->sess.noVoteLimit)
			ent->client->pers.voteCount++;
		ent->client->ps.eFlags |= EF_VOTED;

		trap_SetConfigstring(CS_VOTE_YES,	 va("%i", level.voteInfo.voteYes));
		trap_SetConfigstring(CS_VOTE_NO,	 va("%i", level.voteInfo.voteNo));
		trap_SetConfigstring(CS_VOTE_STRING, level.voteInfo.voteString);
		trap_SetConfigstring(CS_VOTE_TIME,	 va("%i", level.voteInfo.voteTime));
	}

	return(qtrue);
}


qboolean Cmd_RefCommandShitIsCrap_f(gentity_t *ent, unsigned int dwCommand, qboolean fRefCommand)
{
	int		i;
	char	arg0[MAX_STRING_TOKENS];
	char	arg1[MAX_STRING_TOKENS];
	char	arg2[MAX_STRING_TOKENS];

	// make sure it is a valid command to vote on
	trap_Argv(0, arg0, sizeof(arg0));
	trap_Argv(1, arg1, sizeof(arg1));
	trap_Argv(2, arg2, sizeof(arg2));

	if (strchr(arg1, ';') || strchr(arg2, ';')
			|| strchr(arg1, '\r') || strchr(arg2, '\r')
			|| strchr(arg1, '\n') || strchr(arg2, '\n'))
	{
		char *strCmdBase = (!fRefCommand) ? "vote" : "ref command";

		G_refPrintf(ent, "Invalid %s string.", strCmdBase);
		return(qfalse);
	}

	if (trap_Argc() > 1 && (i = G_voteCmdCheck(ent, arg1, arg2, fRefCommand)) != G_NOTFOUND)  	//  --OSP
	{
		if (i != G_OK)
		{
			if (i == G_NOTFOUND) return(qfalse);	// Command error
			else return(qtrue);
		}
	}
	else
	{
		return(qfalse);
	}

	Com_sprintf(level.voteInfo.voteString, sizeof(level.voteInfo.voteString), "%s %s", arg1, arg2);

	if (level.voteInfo.vote_fn != G_Kick_v && level.voteInfo.vote_fn != G_Mute_v)
	{
		AP("cp \"^1** Referee Server Setting Change **\n\"");
	}

	// Gordon: just call the stupid thing.... don't bother with the voting faff
	level.voteInfo.vote_fn(NULL, 0, NULL, NULL, qfalse);

	G_globalSound("sound/misc/referee.wav");

	level.voteInfo.voter_team = ent->client->sess.sessionTeam;
	level.voteInfo.voter_cn = ent - g_entities;
	level.voteInfo.voteTime = level.time;
	level.voteInfo.voteNo = 0;

	return(qtrue);
}

qboolean StringToFilter(const char *s, ipFilter_t *f);

qboolean G_FindFreeComplainIP(gclient_t *cl, ipFilter_t *ip)
{
	int i = 0;

	if (!g_ipcomplaintlimit.integer)
	{
		return qtrue;
	}

	for (i = 0; i < MAX_COMPLAINTIPS && i < g_ipcomplaintlimit.integer; i++)
	{
		if (!cl->pers.complaintips[i].compare && !cl->pers.complaintips[i].mask)
		{
			cl->pers.complaintips[i].compare = ip->compare;
			cl->pers.complaintips[i].mask = ip->mask;
			return qtrue;
		}
		if ((cl->pers.complaintips[i].compare & cl->pers.complaintips[i].mask) == (ip->compare & ip->mask))
		{
			return qtrue;
		}
	}
	return qfalse;
}

/*
==================
Cmd_Vote_f
==================
*/
void Cmd_Vote_f(gentity_t *ent)
{
	char		msg[64];
	int			num;

	if (ent->client->pers.applicationEndTime > level.time)
	{

		gclient_t *cl = g_entities[ ent->client->pers.applicationClient ].client;
		if (!cl)
			return;
		if (cl->pers.connected != CON_CONNECTED)
			return;

		trap_Argv(1, msg, sizeof(msg));

		if (msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1')
		{
			trap_SendServerCommand(ent - g_entities, "application -4");
			trap_SendServerCommand(ent->client->pers.applicationClient, "application -3");

			G_AddClientToFireteam(ent->client->pers.applicationClient, ent - g_entities);
		}
		else
		{
			trap_SendServerCommand(ent - g_entities, "application -4");
			trap_SendServerCommand(ent->client->pers.applicationClient, "application -2");
		}

		ent->client->pers.applicationEndTime = 0;
		ent->client->pers.applicationClient = -1;

		return;
	}

	ent->client->pers.applicationEndTime = 0;
	ent->client->pers.applicationClient = -1;

	if (ent->client->pers.invitationEndTime > level.time)
	{

		gclient_t *cl = g_entities[ ent->client->pers.invitationClient ].client;
		if (!cl)
			return;
		if (cl->pers.connected != CON_CONNECTED)
			return;

		trap_Argv(1, msg, sizeof(msg));

		if (msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1')
		{
			trap_SendServerCommand(ent - g_entities, "invitation -4");
			trap_SendServerCommand(ent->client->pers.invitationClient, "invitation -3");

			G_AddClientToFireteam(ent - g_entities, ent->client->pers.invitationClient);
		}
		else
		{
			trap_SendServerCommand(ent - g_entities, "invitation -4");
			trap_SendServerCommand(ent->client->pers.invitationClient, "invitation -2");
		}

		ent->client->pers.invitationEndTime = 0;
		ent->client->pers.invitationClient = -1;

		return;
	}

	ent->client->pers.invitationEndTime = 0;
	ent->client->pers.invitationClient = -1;

	if (ent->client->pers.propositionEndTime > level.time)
	{
		gclient_t *cl = g_entities[ ent->client->pers.propositionClient ].client;
		if (!cl)
			return;
		if (cl->pers.connected != CON_CONNECTED)
			return;

		trap_Argv(1, msg, sizeof(msg));

		if (msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1')
		{
			trap_SendServerCommand(ent - g_entities, "proposition -4");
			trap_SendServerCommand(ent->client->pers.propositionClient2, "proposition -3");

			G_InviteToFireTeam(ent - g_entities, ent->client->pers.propositionClient);
		}
		else
		{
			trap_SendServerCommand(ent - g_entities, "proposition -4");
			trap_SendServerCommand(ent->client->pers.propositionClient2, "proposition -2");
		}

		ent->client->pers.propositionEndTime = 0;
		ent->client->pers.propositionClient = -1;
		ent->client->pers.propositionClient2 = -1;

		return;
	}

	if (ent->client->pers.autofireteamEndTime > level.time)
	{
		fireteamData_t *ft;

		trap_Argv(1, msg, sizeof(msg));

		if (msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1')
		{
			trap_SendServerCommand(ent - g_entities, "aft -2");

			if (G_IsFireteamLeader(ent - g_entities, &ft))
			{
				ft->priv = qtrue;
			}
		}
		else
		{
			trap_SendServerCommand(ent - g_entities, "aft -2");
		}

		ent->client->pers.autofireteamEndTime = 0;

		return;
	}

	if (ent->client->pers.autofireteamCreateEndTime > level.time)
	{
		trap_Argv(1, msg, sizeof(msg));

		if (msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1')
		{
			trap_SendServerCommand(ent - g_entities, "aftc -2");

			G_RegisterFireteam(ent - g_entities);
		}
		else
		{
			trap_SendServerCommand(ent - g_entities, "aftc -2");
		}

		ent->client->pers.autofireteamCreateEndTime = 0;

		return;
	}

	if (ent->client->pers.autofireteamJoinEndTime > level.time)
	{
		trap_Argv(1, msg, sizeof(msg));

		if (msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1')
		{
			fireteamData_t *ft;

			trap_SendServerCommand(ent - g_entities, "aftj -2");


			ft = G_FindFreePublicFireteam(ent->client->sess.sessionTeam);
			if (ft)
			{
				G_AddClientToFireteam(ent - g_entities, ft->joinOrder[0]);
			}
		}
		else
		{
			trap_SendServerCommand(ent - g_entities, "aftj -2");
		}

		ent->client->pers.autofireteamCreateEndTime = 0;

		return;
	}

	ent->client->pers.propositionEndTime = 0;
	ent->client->pers.propositionClient = -1;
	ent->client->pers.propositionClient2 = -1;

	// dhm
	// Reset this ent's complainEndTime so they can't send multiple complaints
	ent->client->pers.complaintEndTime = -1;
	ent->client->pers.complaintClient = -1;

	if (!level.voteInfo.voteTime)
	{
		trap_SendServerCommand(ent - g_entities, "print \"No vote in progress.\n\"");
		return;
	}
	if (ent->client->ps.eFlags & EF_VOTED)
	{
		trap_SendServerCommand(ent - g_entities, "print \"Vote already cast.\n\"");
		return;
	}
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		trap_SendServerCommand(ent - g_entities, "print \"Not allowed to vote as spectator.\n\"");
		return;
	}

	if (level.voteInfo.vote_fn == G_Kick_v)
	{
		int pid = atoi(level.voteInfo.vote_value);
		if (!g_entities[ pid ].client)
		{
			return;
		}
	}

	trap_SendServerCommand(ent - g_entities, "print \"Vote cast.\n\"");

	ent->client->ps.eFlags |= EF_VOTED;

	trap_Argv(1, msg, sizeof(msg));

	if (msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1')
	{
		level.voteInfo.voteYes++;
		trap_SetConfigstring(CS_VOTE_YES, va("%i", level.voteInfo.voteYes));
	}
	else
	{
		level.voteInfo.voteNo++;
		trap_SetConfigstring(CS_VOTE_NO, va("%i", level.voteInfo.voteNo));
	}

	// a majority will be determined in G_CheckVote, which will also account
	// for players entering or leaving
}


qboolean G_canPickupMelee(gentity_t *ent)
{
	// JPW NERVE -- no "melee" weapons in net play
	return qfalse;
}
// jpw




/*
=================
Cmd_SetViewpos_f
=================
*/
void Cmd_SetViewpos_f(gentity_t *ent)
{
	vec3_t		origin, angles;
	char		buffer[MAX_TOKEN_CHARS];
	int			i;

	if (!g_cheats.integer)
	{
		trap_SendServerCommand(ent - g_entities, va("print \"Cheats are not enabled on this server.\n\""));
		return;
	}
	if (trap_Argc() != 5)
	{
		trap_SendServerCommand(ent - g_entities, va("print \"usage: setviewpos x y z yaw\n\""));
		return;
	}

	VectorClear(angles);
	for (i = 0 ; i < 3 ; i++)
	{
		trap_Argv(i + 1, buffer, sizeof(buffer));
		origin[i] = atof(buffer);
	}

	trap_Argv(4, buffer, sizeof(buffer));
	angles[YAW] = atof(buffer);

	TeleportPlayer(ent, origin, angles);
}

/*
=================
Cmd_StartCamera_f
=================
*/
void Cmd_StartCamera_f(gentity_t *ent)
{

	if (ent->client->cameraPortal)
		G_FreeEntity(ent->client->cameraPortal);
	ent->client->cameraPortal = G_Spawn();

	ent->client->cameraPortal->s.eType = ET_CAMERA;
	ent->client->cameraPortal->s.apos.trType = TR_STATIONARY;
	ent->client->cameraPortal->s.apos.trTime = 0;
	ent->client->cameraPortal->s.apos.trDuration = 0;
	VectorClear(ent->client->cameraPortal->s.angles);
	VectorClear(ent->client->cameraPortal->s.apos.trDelta);
	G_SetOrigin(ent->client->cameraPortal, ent->r.currentOrigin);
	VectorCopy(ent->r.currentOrigin, ent->client->cameraPortal->s.origin2);

	ent->client->cameraPortal->s.frame = 0;

	ent->client->cameraPortal->r.svFlags |= (SVF_PORTAL | SVF_SINGLECLIENT);
	ent->client->cameraPortal->r.singleClient = ent->client->ps.clientNum;

	ent->client->ps.eFlags |= EF_VIEWING_CAMERA;
	ent->s.eFlags |= EF_VIEWING_CAMERA;

	VectorCopy(ent->r.currentOrigin, ent->client->cameraOrigin);	// backup our origin

	// (SA) trying this in client to avoid 1 frame of player drawing
	//	ent->client->ps.eFlags |= EF_NODRAW;
	//	ent->s.eFlags |= EF_NODRAW;
}

/*
=================
Cmd_StopCamera_f
=================
*/
void Cmd_StopCamera_f(gentity_t *ent)
{
	//	gentity_t *sp;

	if (ent->client->cameraPortal && (ent->client->ps.eFlags & EF_VIEWING_CAMERA))
	{
		// send a script event
		//		G_Script_ScriptEvent( ent->client->cameraPortal, "stopcam", "" );

		// go back into noclient mode
		G_FreeEntity(ent->client->cameraPortal);
		ent->client->cameraPortal = NULL;

		ent->s.eFlags &= ~EF_VIEWING_CAMERA;
		ent->client->ps.eFlags &= ~EF_VIEWING_CAMERA;

		//G_SetOrigin( ent, ent->client->cameraOrigin );	// restore our origin
		//VectorCopy( ent->client->cameraOrigin, ent->client->ps.origin );

		// (SA) trying this in client to avoid 1 frame of player drawing
		//		ent->s.eFlags &= ~EF_NODRAW;
		//		ent->client->ps.eFlags &= ~EF_NODRAW;

		// RF, if we are near the spawn point, save the "current" game, for reloading after death
		//		sp = NULL;
		// gcc: suggests () around assignment used as truth value
		//		while ((sp = G_Find( sp, FOFS(classname), "info_player_deathmatch" ))) {	// info_player_start becomes info_player_deathmatch in it's spawn functon
		//			if (Distance( ent->s.pos.trBase, sp->s.origin ) < 256 && trap_InPVS( ent->s.pos.trBase, sp->s.origin )) {
		//				G_SaveGame( NULL );
		//				break;
		//			}
		//		}
	}
}

/*
=================
Cmd_SetCameraOrigin_f
=================
*/
void Cmd_SetCameraOrigin_f(gentity_t *ent)
{
	char		buffer[MAX_TOKEN_CHARS];
	int i;
	vec3_t		origin;

	if (trap_Argc() != 4)
	{
		return;
	}

	for (i = 0 ; i < 3 ; i++)
	{
		trap_Argv(i + 1, buffer, sizeof(buffer));
		origin[i] = atof(buffer);
	}

	if (ent->client->cameraPortal)
	{
		//G_SetOrigin( ent->client->cameraPortal, origin );	// set our origin
		VectorCopy(origin, ent->client->cameraPortal->s.origin2);
		trap_LinkEntity(ent->client->cameraPortal);
		//	G_SetOrigin( ent, origin );	// set our origin
		//	VectorCopy( origin, ent->client->ps.origin );
	}
}

extern vec3_t playerMins;
extern vec3_t playerMaxs;

qboolean G_TankIsOccupied(gentity_t *ent)
{
	if (!ent->tankLink)
	{
		return qfalse;
	}

	return qtrue;
}

qboolean G_TankIsMountable(gentity_t *ent, gentity_t *other)
{
	if (!(ent->spawnflags & 128))
	{
		return qfalse;
	}

	if (level.disableTankEnter)
	{
		return qfalse;
	}

	if (G_TankIsOccupied(ent))
	{
		return qfalse;
	}

	if (ent->health <= 0)
	{
		return qfalse;
	}

	if (other->client->ps.weaponDelay)
	{
		return qfalse;
	}

	return qtrue;
}

// Rafael
/*
==================
Cmd_Activate_f
==================
*/
qboolean Do_Activate2_f(gentity_t *ent, gentity_t *traceEnt)
{
	qboolean found = qfalse;

	if (ent->client->sess.playerType == PC_COVERTOPS && !ent->client->ps.powerups[PW_OPS_DISGUISED] && ent->health > 0)
	{
		if (!ent->client->ps.powerups[PW_BLUEFLAG] && !ent->client->ps.powerups[PW_REDFLAG])
		{
			if (traceEnt->s.eType == ET_CORPSE)
			{
				if (BODY_TEAM(traceEnt) < 4 && BODY_TEAM(traceEnt) != ent->client->sess.sessionTeam)
				{
					found = qtrue;

					if (BODY_VALUE(traceEnt) >= 250)
					{

						traceEnt->nextthink = traceEnt->timestamp + BODY_TIME(BODY_TEAM(traceEnt));

						//						BG_AnimScriptEvent( &ent->client->ps, ent->client->pers.character->animModelInfo, ANIM_ET_PICKUPGRENADE, qfalse, qtrue );
						//						ent->client->ps.pm_flags |= PMF_TIME_LOCKPLAYER;
						//						ent->client->ps.pm_time = 2100;

						ent->client->ps.powerups[PW_OPS_DISGUISED] = 1;
						ent->client->ps.powerups[PW_OPS_CLASS_1] = BODY_CLASS(traceEnt) & 1;
						ent->client->ps.powerups[PW_OPS_CLASS_2] = BODY_CLASS(traceEnt) & 2;
						ent->client->ps.powerups[PW_OPS_CLASS_3] = BODY_CLASS(traceEnt) & 4;

						BODY_TEAM(traceEnt) += 4;
						traceEnt->activator = ent;

						traceEnt->s.time2 =	1;

						// sound effect
						G_AddEvent(ent, EV_DISGUISE_SOUND, 0);

						G_AddSkillPoints(ent, SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 5.f);
						G_DebugAddSkillPoints(ent, SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 5, "stealing uniform");

						Q_strncpyz(ent->client->disguiseNetname, g_entities[traceEnt->s.clientNum].client->pers.netname, sizeof(ent->client->disguiseNetname));
						ent->client->disguiseRank = g_entities[traceEnt->s.clientNum].client ? g_entities[traceEnt->s.clientNum].client->sess.rank : 0;

						ClientUserinfoChanged(ent->s.clientNum);
					}
					else
					{
						BODY_VALUE(traceEnt) += 5;
					}
				}
			}
		}
	}

	return found;
}

// TAT 1/14/2003 - extracted out the functionality of Cmd_Activate_f from finding the object to use
//		so we can force bots to use items, without worrying that they are looking EXACTLY at the target
qboolean Do_Activate_f(gentity_t *ent, gentity_t *traceEnt)
{
	qboolean found = qfalse;
	qboolean	walking = qfalse;
	vec3_t		forward;	//, offset, end;
	//trace_t		tr;

	// Arnout: invisible entities can't be used

	if (traceEnt->entstate == STATE_INVISIBLE || traceEnt->entstate == STATE_UNDERCONSTRUCTION)
	{
		return qfalse;
	}

	if (ent->client->pers.cmd.buttons & BUTTON_WALKING)
		walking = qtrue;

	if (traceEnt->classname)
	{
		traceEnt->flags &= ~FL_SOFTACTIVATE;	// FL_SOFTACTIVATE will be set if the user is holding 'walk' key

		if (traceEnt->s.eType == ET_ALARMBOX)
		{
			trace_t		trace;

			if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
				return qfalse;

			memset(&trace, 0, sizeof(trace));

			if (traceEnt->use)
				G_UseEntity(traceEnt, ent, 0);
			found = qtrue;
		}
		else if (traceEnt->s.eType == ET_ITEM)
		{
			trace_t		trace;

			if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
				return qfalse;

			memset(&trace, 0, sizeof(trace));

			if (traceEnt->touch)
			{
				if (ent->client->pers.autoActivate == PICKUP_ACTIVATE)
					ent->client->pers.autoActivate = PICKUP_FORCE;		//----(SA) force pickup
				traceEnt->active = qtrue;
				traceEnt->touch(traceEnt, ent, &trace);
			}

			found = qtrue;
		}
		else if (traceEnt->s.eType == ET_MOVER && G_TankIsMountable(traceEnt, ent))
		{
			G_Script_ScriptEvent(traceEnt, "mg42", "mount");
			ent->tagParent = traceEnt->nextTrain;
			Q_strncpyz(ent->tagName, "tag_player", MAX_QPATH);
			ent->backupWeaponTime = ent->client->ps.weaponTime;
			ent->client->ps.weaponTime = traceEnt->backupWeaponTime;
			ent->client->ps.weapHeat[WP_DUMMY_MG42] = traceEnt->mg42weapHeat;

			ent->tankLink = traceEnt;
			traceEnt->tankLink = ent;

			G_ProcessTagConnect(ent, qtrue);
			found = qtrue;
		}
		else if (G_EmplacedGunIsMountable(traceEnt, ent))
		{
			gclient_t *cl = &level.clients[ ent->s.clientNum ];
			vec3_t	point;

			AngleVectors(traceEnt->s.apos.trBase, forward, NULL, NULL);
			VectorMA(traceEnt->r.currentOrigin, -36, forward, point);
			point[2] = ent->r.currentOrigin[2];

			// Save initial position
			VectorCopy(point, ent->TargetAngles);

			// Zero out velocity
			VectorCopy(vec3_origin, ent->client->ps.velocity);
			VectorCopy(vec3_origin, ent->s.pos.trDelta);

			traceEnt->active = qtrue;
			ent->active = qtrue;
			traceEnt->r.ownerNum = ent->s.number;
			VectorCopy(traceEnt->s.angles, traceEnt->TargetAngles);
			traceEnt->s.otherEntityNum = ent->s.number;

			cl->pmext.harc = traceEnt->harc;
			cl->pmext.varc = traceEnt->varc;
			VectorCopy(traceEnt->s.angles, cl->pmext.centerangles);
			cl->pmext.centerangles[PITCH] = AngleNormalize180(cl->pmext.centerangles[PITCH]);
			cl->pmext.centerangles[YAW] = AngleNormalize180(cl->pmext.centerangles[YAW]);
			cl->pmext.centerangles[ROLL] = AngleNormalize180(cl->pmext.centerangles[ROLL]);

			ent->backupWeaponTime = ent->client->ps.weaponTime;
			ent->client->ps.weaponTime = traceEnt->backupWeaponTime;
			ent->client->ps.weapHeat[WP_DUMMY_MG42] = traceEnt->mg42weapHeat;

			G_UseTargets(traceEnt, ent);	//----(SA)	added for Mike so mounting an MG42 can be a trigger event (let me know if there's any issues with this)
			found = qtrue;
		}
		else if (((Q_stricmp(traceEnt->classname, "func_door") == 0) || (Q_stricmp(traceEnt->classname, "func_door_rotating") == 0)))
		{
			if (walking)
			{
				traceEnt->flags |= FL_SOFTACTIVATE;		// no noise
			}
			G_TryDoor(traceEnt, ent, ent);		// (door,other,activator)
			found = qtrue;
		}
		else if ((Q_stricmp(traceEnt->classname, "team_WOLF_checkpoint") == 0))
		{
			if (traceEnt->count != ent->client->sess.sessionTeam)
			{
				traceEnt->health++;
			}
			found = qtrue;
		}
		else if ((Q_stricmp(traceEnt->classname, "func_button") == 0) && (traceEnt->s.apos.trType == TR_STATIONARY && traceEnt->s.pos.trType == TR_STATIONARY) && traceEnt->active == qfalse)
		{
			Use_BinaryMover(traceEnt, ent, ent);
			traceEnt->active = qtrue;
			found = qtrue;
		}
		else if (!Q_stricmp(traceEnt->classname, "func_invisible_user"))
		{
			if (walking)
			{
				traceEnt->flags |= FL_SOFTACTIVATE;		// no noise
			}
			G_UseEntity(traceEnt, ent, ent);
			found = qtrue;
		}
		else if (!Q_stricmp(traceEnt->classname, "props_footlocker"))
		{
			G_UseEntity(traceEnt, ent, ent);
			found = qtrue;
		}
	}

	return found;
}

void G_LeaveTank(gentity_t *ent, qboolean position)
{
	gentity_t *tank;

	// found our tank (or whatever)
	vec3_t axis[3];
	vec3_t pos;
	trace_t tr;

	tank = ent->tankLink;
	if (!tank)
	{
		return;
	}

	if (position)
	{

		AnglesToAxis(tank->s.angles, axis);

		VectorMA(ent->client->ps.origin, 128, axis[1], pos);
		trap_Trace(&tr, pos, playerMins, playerMaxs, pos, -1, CONTENTS_SOLID);

		if (tr.startsolid)
		{
			// try right
			VectorMA(ent->client->ps.origin, -128, axis[1], pos);
			trap_Trace(&tr, pos, playerMins, playerMaxs, pos, -1, CONTENTS_SOLID);

			if (tr.startsolid)
			{
				// try back
				VectorMA(ent->client->ps.origin, -224, axis[0], pos);
				trap_Trace(&tr, pos, playerMins, playerMaxs, pos, -1, CONTENTS_SOLID);

				if (tr.startsolid)
				{
					// try front
					VectorMA(ent->client->ps.origin, 224, axis[0], pos);
					trap_Trace(&tr, pos, playerMins, playerMaxs, pos, -1, CONTENTS_SOLID);

					if (tr.startsolid)
					{
						// give up
						return;
					}
				}
			}
		}

		VectorClear(ent->client->ps.velocity);   // Gordon: dont want them to fly away ;D
		TeleportPlayer(ent, pos, ent->client->ps.viewangles);
	}


	tank->mg42weapHeat = ent->client->ps.weapHeat[WP_DUMMY_MG42];
	tank->backupWeaponTime = ent->client->ps.weaponTime;
	ent->client->ps.weaponTime = ent->backupWeaponTime;

	G_Script_ScriptEvent(tank, "mg42", "unmount");
	ent->tagParent = NULL;
	*ent->tagName = '\0';
	ent->s.eFlags &= ~EF_MOUNTEDTANK;
	ent->client->ps.eFlags &= ~EF_MOUNTEDTANK;
	tank->s.powerups = -1;

	tank->tankLink = NULL;
	ent->tankLink = NULL;
}

void Cmd_Activate_f(gentity_t *ent)
{
	trace_t		tr;
	vec3_t		end;
	gentity_t	*traceEnt;
	vec3_t		forward, right, up, offset;
	//	int			activatetime = level.time;
	qboolean	found = qfalse;
	qboolean	pass2 = qfalse;
	int			i;

	if (ent->health <= 0)
	{
		return;
	}

	if (ent->s.weapon == WP_MORTAR_SET || ent->s.weapon == WP_MOBILE_MG42_SET)
	{
		return;
	}

	if (ent->active)
	{
		if (ent->client->ps.persistant[PERS_HWEAPON_USE])
		{
			// DHM - Nerve :: Restore original position if current position is bad
			trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, ent->s.number, MASK_PLAYERSOLID);
			if (tr.startsolid)
			{
				VectorCopy(ent->TargetAngles, ent->client->ps.origin);
				VectorCopy(ent->TargetAngles, ent->r.currentOrigin);
				ent->r.contents = CONTENTS_CORPSE;		// DHM - this will correct itself in ClientEndFrame
			}

			ent->client->ps.eFlags &= ~EF_MG42_ACTIVE;			// DHM - Nerve :: unset flag
			ent->client->ps.eFlags &= ~EF_AAGUN_ACTIVE;

			ent->client->ps.persistant[PERS_HWEAPON_USE] = 0;
			ent->active = qfalse;

			for (i = 0; i < level.num_entities; i++)
			{
				if (g_entities[i].s.eType == ET_MG42_BARREL && g_entities[i].r.ownerNum == ent->s.number)
				{
					g_entities[i].mg42weapHeat = ent->client->ps.weapHeat[WP_DUMMY_MG42];
					g_entities[i].backupWeaponTime = ent->client->ps.weaponTime;
					break;
				}
			}
			ent->client->ps.weaponTime = ent->backupWeaponTime;
		}
		else
		{
			ent->active = qfalse;
		}
		return;
	}
	else if (ent->client->ps.eFlags & EF_MOUNTEDTANK && ent->s.eFlags & EF_MOUNTEDTANK && !level.disableTankExit)
	{
		G_LeaveTank(ent, qtrue);
		return;
	}

	AngleVectors(ent->client->ps.viewangles, forward, right, up);

	VectorCopy(ent->client->ps.origin, offset);
	offset[2] += ent->client->ps.viewheight;

	// lean
	if (ent->client->ps.leanf)
	{
		VectorMA(offset, ent->client->ps.leanf, right, offset);
	}

	//VectorMA( offset, 256, forward, end );
	VectorMA(offset, 96, forward, end);

	trap_Trace(&tr, offset, NULL, NULL, end, ent->s.number, (CONTENTS_SOLID | CONTENTS_MISSILECLIP | CONTENTS_BODY | CONTENTS_CORPSE));

	if (tr.surfaceFlags &SURF_NOIMPACT || tr.entityNum == ENTITYNUM_WORLD)
	{
		trap_Trace(&tr, offset, NULL, NULL, end, ent->s.number, (CONTENTS_SOLID | CONTENTS_MISSILECLIP | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_TRIGGER));
		pass2 = qtrue;
	}

tryagain:

	if (tr.surfaceFlags &SURF_NOIMPACT || tr.entityNum == ENTITYNUM_WORLD)
	{
		return;
	}

	traceEnt = &g_entities[ tr.entityNum ];

	found = Do_Activate_f(ent, traceEnt);

	if (!found && !pass2)
	{
		pass2 = qtrue;
		trap_Trace(&tr, offset, NULL, NULL, end, ent->s.number, (CONTENTS_SOLID | CONTENTS_MISSILECLIP | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_TRIGGER));
		goto tryagain;
	}
}

/*
========================
Dini, Push, ETPub
========================
*/
qboolean G_PushPlayer(gentity_t *ent, gentity_t *victim)
{
	vec3_t	dir, push;

	if (!tjg_shove.integer)
		return qfalse;

	if (ent->health <= 0)
		return qfalse;

	if ((level.time - ent->client->pmext.shoveTime) < 500)
	{
		return qfalse;
	}

	ent->client->pmext.shoveTime = level.time;

	VectorSubtract(victim->r.currentOrigin, ent->r.currentOrigin, dir);

	VectorNormalizeFast(dir);

	VectorScale(dir, (tjg_shove.value * 5.0f), push);

	if ((abs(push[2]) > abs(push[0])) &&
			(abs(push[2]) > abs(push[1])))
	{

		push[2] = tjg_shove.value;
		push[0] = push[1] = 0;
	}
	else
	{
		// give them a little hop
		push[2] = 24;
	}

	VectorAdd(victim->s.pos.trDelta, push, victim->s.pos.trDelta);
	VectorAdd(victim->client->ps.velocity, push,
			  victim->client->ps.velocity);

	/*	// Care @ sounds for now
		if(tjg_shoveSound.string != '\0') {
			G_AddEvent(victim, EV_GENERAL_SOUND,
				G_SoundIndex(tjg_shoveSound.string));
		}*/

	return qtrue;
}

void Cmd_Activate2_f(gentity_t *ent)
{
	trace_t		tr;
	vec3_t		end;
	gentity_t	*traceEnt;
	vec3_t		forward, right, up, offset;
	//	int			activatetime = level.time;
	qboolean	found = qfalse;
	qboolean	pass2 = qfalse;

	AngleVectors(ent->client->ps.viewangles, forward, right, up);
	CalcMuzzlePointForActivate(ent, forward, right, up, offset);
	VectorMA(offset, 96, forward, end);

	trap_Trace(&tr, offset, NULL, NULL, end, ent->s.number, CONTENTS_BODY);
	if (tr.entityNum >= 0)
	{
		traceEnt = &g_entities[tr.entityNum];
		if (traceEnt->client)
		{
			G_PushPlayer(ent, traceEnt);
			return;
		}
	}

	if (ent->client->sess.playerType != PC_COVERTOPS)
	{
		return;
	}

	trap_Trace(&tr, offset, NULL, NULL, end, ent->s.number, (CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE));

	if (tr.surfaceFlags &SURF_NOIMPACT || tr.entityNum == ENTITYNUM_WORLD)
	{
		trap_Trace(&tr, offset, NULL, NULL, end, ent->s.number, (CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_TRIGGER));
		pass2 = qtrue;
	}

tryagain:

	if (tr.surfaceFlags &SURF_NOIMPACT || tr.entityNum == ENTITYNUM_WORLD)
	{
		return;
	}

	traceEnt = &g_entities[ tr.entityNum ];

	found = Do_Activate2_f(ent, traceEnt);

	if (!found && !pass2)
	{
		pass2 = qtrue;
		trap_Trace(&tr, offset, NULL, NULL, end, ent->s.number, (CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_TRIGGER));
		goto tryagain;
	}
}

void G_UpdateSpawnCounts(void)
{
	int		i, j;
	char	cs[MAX_STRING_CHARS];
	int		current, count, team;

	for (i = 0; i < level.numspawntargets; i++)
	{
		trap_GetConfigstring(CS_MULTI_SPAWNTARGETS + i, cs, sizeof(cs));

		current = atoi(Info_ValueForKey(cs, "c"));
		team = atoi(Info_ValueForKey(cs, "t")) & ~256;

		count = 0;
		for (j = 0; j < level.numConnectedClients; j++)
		{
			gclient_t *client = &level.clients[ level.sortedClients[ j ] ];

			if (client->sess.sessionTeam != TEAM_AXIS && client->sess.sessionTeam != TEAM_ALLIES)
			{
				continue;
			}

			if (client->sess.sessionTeam == team && client->sess.spawnObjectiveIndex == i + 1)
			{
				count++;
				continue;
			}

			if (client->sess.spawnObjectiveIndex == 0)
			{
				if (client->sess.sessionTeam == TEAM_AXIS)
				{
					if (level.axisAutoSpawn == i)
					{
						count++;
						continue;
					}
				}
				else
				{
					if (level.alliesAutoSpawn == i)
					{
						count++;
						continue;
					}
				}
			}
		}

		if (count == current)
		{
			continue;
		}

		Info_SetValueForKey(cs, "c", va("%i", count));
		trap_SetConfigstring(CS_MULTI_SPAWNTARGETS + i, cs);
	}
}

/*
============
Cmd_SetSpawnPoint_f
============
*/
void SetPlayerSpawn(gentity_t *ent, int spawn, qboolean update)
{
	ent->client->sess.spawnObjectiveIndex = spawn;
	if (ent->client->sess.spawnObjectiveIndex >= MAX_MULTI_SPAWNTARGETS || ent->client->sess.spawnObjectiveIndex < 0)
	{
		ent->client->sess.spawnObjectiveIndex = 0;
	}

	if (update)
	{
		G_UpdateSpawnCounts();
	}
}

void Cmd_SetSpawnPoint_f(gentity_t *ent)
{
	char arg[MAX_TOKEN_CHARS];
	int val, i;

	if (trap_Argc() != 2)
	{
		return;
	}

	trap_Argv(1, arg, sizeof(arg));
	val = atoi(arg);

	if (ent->client)
	{
		SetPlayerSpawn(ent, val, qtrue);
	}

	//	if( ent->client->sess.sessionTeam != TEAM_SPECTATOR && !(ent->client->ps.pm_flags & PMF_LIMBO) ) {
	//		return;
	//	}

	for (i = 0; i < level.numLimboCams; i++)
	{
		int x = (g_entities[level.limboCams[i].targetEnt].count - CS_MULTI_SPAWNTARGETS) + 1;
		if (level.limboCams[i].spawn && x == val)
		{
			VectorCopy(level.limboCams[i].origin, ent->s.origin2);
			ent->r.svFlags |= SVF_SELF_PORTAL_EXCLUSIVE;
			trap_SendServerCommand(ent - g_entities, va("portalcampos %i %i %i %i %i %i %i %i", val - 1, (int)level.limboCams[i].origin[0], (int)level.limboCams[i].origin[1], (int)level.limboCams[i].origin[2], (int)level.limboCams[i].angles[0], (int)level.limboCams[i].angles[1], (int)level.limboCams[i].angles[2], level.limboCams[i].hasEnt ? level.limboCams[i].targetEnt : -1));
			break;
		}
	}
}

// Dini, wsfix
void Cmd_WeaponStat_f(gentity_t *ent)
{
	char buffer[16];
	extWeaponStats_t stat;

	if (!ent || !ent->client)
	{
		return;
	}

	if (trap_Argc() != 2)
	{
		return;
	}
	trap_Argv(1, buffer, 16);
	stat = atoi(buffer);

	if (stat >= 0 && stat <= WS_MAX)
	{
		trap_SendServerCommand(ent - g_entities, va("rws %i %i", ent->client->sess.aWeaponStats[stat].atts, ent->client->sess.aWeaponStats[stat].hits));
	}
	else
	{
		G_LogPrintf("Invalid client command ws received: ws %d\n", stat);
	}
}


void Cmd_IntermissionWeaponStats_f(gentity_t *ent)
{
	char buffer[1024];
	int i, clientNum;

	if (!ent || !ent->client)
	{
		return;
	}

	trap_Argv(1, buffer, sizeof(buffer));

	clientNum = atoi(buffer);
	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		return;
	}

	Q_strncpyz(buffer, "imws ", sizeof(buffer));
	for (i = 0; i < WS_MAX; i++)
	{
		Q_strcat(buffer, sizeof(buffer), va("%i %i %i ", level.clients[clientNum].sess.aWeaponStats[i].atts, level.clients[clientNum].sess.aWeaponStats[i].hits, level.clients[clientNum].sess.aWeaponStats[i].kills));
	}

	trap_SendServerCommand(ent - g_entities, buffer);
}

void G_MakeReady(gentity_t *ent)
{
	ent->client->ps.eFlags |= EF_READY;
	ent->s.eFlags |= EF_READY;
	// rain - #105 - moved this set here
	ent->client->pers.ready = qtrue;
}

void G_MakeUnready(gentity_t *ent)
{
	ent->client->ps.eFlags &= ~EF_READY;
	ent->s.eFlags &= ~EF_READY;
	// rain - #105 - moved this set here
	ent->client->pers.ready = qfalse;
}

void Cmd_IntermissionReady_f(gentity_t *ent)
{
	if (!ent || !ent->client)
	{
		return;
	}

	G_MakeReady(ent);
}

void Cmd_IntermissionPlayerKillsDeaths_f(gentity_t *ent)
{
	char buffer[1024];
	int i;

	if (!ent || !ent->client)
	{
		return;
	}

	Q_strncpyz(buffer, "impkd ", sizeof(buffer));
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (g_entities[i].inuse)
		{
			Q_strcat(buffer, sizeof(buffer), va("%i %i ", level.clients[i].sess.kills, level.clients[i].sess.deaths));
		}
		else
		{
			Q_strcat(buffer, sizeof(buffer), "0 0 ");
		}
	}

	trap_SendServerCommand(ent - g_entities, buffer);
}

void G_CalcClientAccuracies(void)
{
	int i, j;
	int shots, hits;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		shots = 0;
		hits = 0;

		if (g_entities[i].inuse)
		{
			for (j = 0; j < WS_MAX; j++)
			{
				shots += level.clients[i].sess.aWeaponStats[j].atts;
				hits += level.clients[i].sess.aWeaponStats[j].hits;
			}

			level.clients[ i ].acc = shots ? (100 * hits) / (float)shots : 0;
		}
		else
		{
			level.clients[ i ].acc = 0;
		}
	}
}


void Cmd_IntermissionWeaponAccuracies_f(gentity_t *ent)
{
	char buffer[1024];
	int i;

	if (!ent || !ent->client)
	{
		return;
	}

	G_CalcClientAccuracies();

	Q_strncpyz(buffer, "imwa ", sizeof(buffer));
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		Q_strcat(buffer, sizeof(buffer), va("%i ", (int)level.clients[ i ].acc));
	}

	trap_SendServerCommand(ent - g_entities, buffer);
}

void Cmd_SelectedObjective_f(gentity_t *ent)
{
	int i, val;
	char buffer[16];
	vec_t dist, neardist = 0;
	int nearest = -1;


	if (!ent || !ent->client)
	{
		return;
	}

	//	if( ent->client->sess.sessionTeam != TEAM_SPECTATOR && !(ent->client->ps.pm_flags & PMF_LIMBO) ) {
	//		return;
	//	}

	if (trap_Argc() != 2)
	{
		return;
	}
	trap_Argv(1, buffer, 16);
	val = atoi(buffer) + 1;


	for (i = 0; i < level.numLimboCams; i++)
	{
		if (!level.limboCams[i].spawn && level.limboCams[i].info == val)
		{
			if (!level.limboCams[i].hasEnt)
			{
				VectorCopy(level.limboCams[i].origin, ent->s.origin2);
				ent->r.svFlags |= SVF_SELF_PORTAL_EXCLUSIVE;
				trap_SendServerCommand(ent - g_entities, va("portalcampos %i %i %i %i %i %i %i %i", val - 1, (int)level.limboCams[i].origin[0], (int)level.limboCams[i].origin[1], (int)level.limboCams[i].origin[2], (int)level.limboCams[i].angles[0], (int)level.limboCams[i].angles[1], (int)level.limboCams[i].angles[2], level.limboCams[i].hasEnt ? level.limboCams[i].targetEnt : -1));

				break;
			}
			else
			{
				dist = VectorDistanceSquared(level.limboCams[i].origin, g_entities[level.limboCams[i].targetEnt].r.currentOrigin);
				if (nearest == -1 || dist < neardist)
				{
					nearest = i;
					neardist = dist;
				}
			}
		}
	}

	if (nearest != -1)
	{
		i = nearest;

		VectorCopy(level.limboCams[i].origin, ent->s.origin2);
		ent->r.svFlags |= SVF_SELF_PORTAL_EXCLUSIVE;
		trap_SendServerCommand(ent - g_entities, va("portalcampos %i %i %i %i %i %i %i %i", val - 1, (int)level.limboCams[i].origin[0], (int)level.limboCams[i].origin[1], (int)level.limboCams[i].origin[2], (int)level.limboCams[i].angles[0], (int)level.limboCams[i].angles[1], (int)level.limboCams[i].angles[2], level.limboCams[i].hasEnt ? level.limboCams[i].targetEnt : -1));
	}
}

void Cmd_Ignore_f(gentity_t *ent)
{
	char	cmd[MAX_TOKEN_CHARS];
	int		cnum;

	trap_Argv(1, cmd, sizeof(cmd));

	if (!*cmd)
	{
		trap_SendServerCommand(ent - g_entities, "print \"usage: Ignore <clientname>.\n\"\n");
		return;
	}

	cnum = G_refClientnumForName(ent, cmd);

	if (cnum != MAX_CLIENTS)
	{
		COM_BitSet(ent->client->sess.ignoreClients, cnum);
	}
}

void Cmd_TicketTape_f(void)
{
	/*	char	cmd[MAX_TOKEN_CHARS];

		trap_Argv( 1, cmd, sizeof( cmd ) );

		trap_SendServerCommand( -1, va( "tt \"LANDMINES SPOTTED <STOP> CHECK COMMAND MAP FOR DETAILS <STOP>\"\n", cmd ));*/
}

void Cmd_UnIgnore_f(gentity_t *ent)
{
	char	cmd[MAX_TOKEN_CHARS];
	int		cnum;

	trap_Argv(1, cmd, sizeof(cmd));

	if (!*cmd)
	{
		trap_SendServerCommand(ent - g_entities, "print \"usage: Unignore <clientname>.\n\"\n");
		return;
	}

	cnum = G_refClientnumForName(ent, cmd);

	if (cnum != MAX_CLIENTS)
	{
		COM_BitClear(ent->client->sess.ignoreClients, cnum);
	}
}



void Cmd_Load_f(gentity_t *ent)
{
	char cmd[MAX_TOKEN_CHARS];
	int argc;
	int posNum;
	save_position_t *pos;
	trace_t trace;

	if (!tjg_SaveLoad.integer)
	{
		CP("print \"Save/load is not enabled on this server.\n\"");
		return;
	}

	// get save slot (do this first so players can get usage message even if
	// they are not allowed to use this command)
	argc = trap_Argc();
	if (argc == 1)
		posNum = 0;
	else if (argc == 2)
	{
		trap_Argv(1, cmd, sizeof(cmd));
		if ((posNum = atoi(cmd)) < 0 || posNum >= MAX_SAVED_POSITIONS)
		{
			CP("print \"Invalid position!\n\"");
			return;
		}
	}
	else
	{
		CP("print \"usage: load [position]\n\"");
		return;
	}

	if (!ent->client->sess.allowSL)
	{
		CP("cp \"^7You are not allowed to load!\n\"");
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		CP("cp \"^5You can not load as a spectator!\n\"");
		return;
	}

	if (ent->client->sess.logged && ((tjg_strictSL.integer || (tjg_recordMode.integer && physics.integer)) && ent->client->ps.eFlags & EF_PRONE))
	{
		CP("cp \"You can not load while proning!\n\"");
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_ALLIES)
		pos = ent->client->sess.alliesSaves + posNum;
	else
		pos = ent->client->sess.axisSaves + posNum;
	if (pos->valid)
	{
		if (ent->client->timerunActive)
		{
			trap_TraceCapsule(&trace, pos->origin, ent->r.mins, ent->r.maxs, pos->origin, ent->s.number, CONTENTS_NOSAVERESET);

			if (trace.fraction != 1.0f || tjg_altSL.integer)
			{
				ent->client->timerunActive = qfalse;
				trap_SendServerCommand(ent - g_entities, "timerun_stop");
				ent->client->ps.curWeapHeat = 0;
			}
		}

		ent->client->nextTimerunStartAllowed = ent->client->ps.commandTime;
		VectorCopy(pos->origin, ent->client->ps.origin);

		if (ent->client->pers.loadViewAngles)
			SetClientViewAngle(ent, pos->vangles);

		if (ent->client->pers.resetVelocityOnLoad || ent->client->sess.logged)
			VectorClear(ent->client->ps.velocity);

		if (ent->client->ps.stats[STAT_HEALTH] < 100 && ent->client->ps.stats[STAT_HEALTH] > 0)
			ent->health = 100;

		if (ent->client->ps.weapon == WP_PANZERFAUST)
			ent->client->ps.ammoclip[WP_PANZERFAUST] = level.rocketRun ? level.rocketRun : 999;

		clearRockets(ent);

		// Start recording a new temp demo.
		trap_SendServerCommand(ent - g_entities, "tempDemoStart");
	}
	else
		CP("cp \"^5Use save first!\n\"");
}

void Cmd_Save_f(gentity_t *ent)
{
	char cmd[MAX_TOKEN_CHARS];
	int argc;
	int posNum;
	save_position_t *pos;
	trace_t trace;

	if (!tjg_SaveLoad.integer)
	{
		CP("print \"" GV_PREFIX " ^5Save/load is not enabled on this server!\n\"");
		return;
	}

	// get save slot (do this first so players can get usage message even if
	// they are not allowed to use this command)
	argc = trap_Argc();
	if (argc == 1)
		posNum = 0;
	else if (argc == 2)
	{
		trap_Argv(1, cmd, sizeof(cmd));
		if ((posNum = atoi(cmd)) < 0 || posNum >= MAX_SAVED_POSITIONS)
		{
			CP("print \"Invalid position!\n\"");
			return;
		}
	}
	else
	{
		CP("print \"usage: save [position]\n\"");
		return;
	}

	if (!ent->client->sess.allowSL)
	{
		CP("cp \"^7You are not allowed to save!\n\"");
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		CP("cp \"^5You can not save as a spectator!\n\"");
		return;
	}

	trap_TraceCapsule(&trace, ent->client->ps.origin, ent->r.mins, ent->r.maxs, ent->client->ps.origin, ent->s.number, CONTENTS_NOSAVE);
	if (trace.fraction != 1.0f)
	{
		CP("cp \"You can not save inside this area!\n\"");
		return;
	}

	if (ent->client->sess.logged && (tjg_strictSL.integer || tjg_recordMode.integer))
	{
		if (ent->client->ps.groundEntityNum == ENTITYNUM_NONE)
		{
			CP("cp \"You can not save while in the air!\n\"");
			return;
		}

		if (tjg_strictSL.integer == 2 && VectorLengthSquared(ent->client->ps.velocity) > 0)
		{
			CP("cp \"You can not save while moving!\n\"");
			return;
		}

		if (ent->client->ps.eFlags & EF_PRONE || ent->client->ps.eFlags & EF_PRONE_MOVING)
		{
			CP("cp \"You can not save while proning!\n\"");
			return;
		}

		if (ent->client->ps.eFlags & EF_CROUCHING)
		{
			CP("cp \"You can not save while crouching!\n\"");
			return;
		}
	}

	if (ent->client->sess.sessionTeam == TEAM_ALLIES)
		pos = ent->client->sess.alliesSaves + posNum;
	else
		pos = ent->client->sess.axisSaves + posNum;

	VectorCopy(ent->client->ps.origin, pos->origin);
	VectorCopy(ent->client->ps.viewangles, pos->vangles);
	pos->valid = qtrue;

	if (posNum == 0)
		CP("cp \"^fSaved\n\"");
	else
		CP(va("cp \"^fSaved ^z%d\n\"", posNum));
}

void Cmd_ResetSpeed_f(gentity_t *ent)
{
	if (!tjg_ResetSpeed.integer)
	{
		CP("print \"" GV_PREFIX " ^5Resetting speed is not enabled on this server!\n\"");
		return;
	}

	if (ent->client->sess.logged)
	{
		CP("cp \"" GV_PREFIX " You can not use resetspeed while logged in!\n\"");
		return;
	}

	VectorClear(ent->client->ps.velocity);
	CP("cpm \"" GV_PREFIX " Speed Reset!\n\"");
	return;
}

/*
 * Resets all client best timeruns and all checkpoints.
 * TODO allow reseting only one timerun, not all
 */
static void Cmd_ResetTimeruns_f(gentity_t *ent)
{
	memset(ent->client->sess.timerunBestTime, 0,
		   sizeof(ent->client->sess.timerunBestTime));
	memset(ent->client->sess.timerunBestCheckpointTimes, 0,
		   sizeof(ent->client->sess.timerunBestCheckpointTimes));

	trap_SendServerCommand(ent - g_entities, "resettimeruns");

	CP("cpm \"" GV_PREFIX " Timeruns reset!\n\"");
	return;
}

/*
 * Performs client login.
 */
void G_Login(gentity_t *ent)
{
	int				i, j;
	gclient_t		*client = ent->client;
	save_position_t	*saves[] = {client->sess.alliesSaves, client->sess.axisSaves};

	if (!tjg_recordMode.integer)
	{
		CP("print \"" GV_PREFIX " ^5Timerun record mode is not enabled on this server!\n\"");
		return;
	}

	if (ent->client->sess.logged)
	{
		CP("cpm \"" GV_PREFIX " You have already logged in!\n\"");
		return;
	}

	if (client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		if (client->ps.groundEntityNum == ENTITYNUM_NONE)
		{
			CP("cpm \"" GV_PREFIX " You can not log in while in air!\n\"");
			return;
		}

		if (VectorLengthSquared(client->ps.velocity) > 0)
		{
			CP("cpm \"" GV_PREFIX " You can not log in while moving!\n\"");
			return;
		}
	}

	// reset all saves (except goback save)
	for (i = 0; i < 2; i++)
		for (j = 0; j < MAX_SAVED_POSITIONS; j++)
			saves[i][j].valid = qfalse;

	if (client->sess.sessionTeam != TEAM_SPECTATOR)
		player_die(ent, ent, ent, 100000, MOD_SUICIDE);
	client->sess.logged = qtrue;
	trap_SendServerCommand(ent - g_entities, "recordmode_login");
	// Start recording a new temp demo.
	trap_SendServerCommand(ent - g_entities, "tempDemoStart");

	CP("cpm \"" GV_PREFIX " You are now logged in!\n\"");
}

static void Cmd_Login_f(gentity_t *ent)
{
	G_Login(ent);
	trap_SendServerCommand(ent - g_entities, "timerun_stop");
}

void Cmd_Logout_f(gentity_t *ent)
{
	if (!tjg_recordMode.integer)
	{
		CP("print \"" GV_PREFIX " ^5Timerun record mode is not enabled on this server!\n\"");
		return;
	}

	if (!ent->client->sess.logged)
	{
		CP("cpm \"" GV_PREFIX " You are not logged in!\n\"");
		return;
	}

	ent->client->timerunActive = qfalse;
	trap_SendServerCommand(ent - g_entities, "timerun_stop");
	ent->client->sess.logged = qfalse;
	trap_SendServerCommand(ent - g_entities, "recordmode_logout");

	CP("cpm \"" GV_PREFIX " You are now logged out!\n\"");
	return;
}

void Cmd_Goto_f(gentity_t *ent)
{
	int clientNum;
	char cmd[MAX_TOKEN_CHARS];
	gentity_t *other;

	if (trap_Argc() != 2)
	{
		CP("print \"usage: goto <PID | partname | name>\n\"");
		return;
	}

	if (!ent->client->sess.allowGoto)
	{
		CP("cpm \"" GV_PREFIX " ^5You are not allowed to use goto.\n\"");
		return;
	}

	if (ent->client->sess.logged)
	{
		CP("cp \"You can not use goto while logged in!\n\"");
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		CP("cpm \"" GV_PREFIX " ^5You can not goto as spectator!\n\"");
		return;
	}

	trap_Argv(1, cmd, sizeof(cmd));
	if ((clientNum = ClientNumberFromString(ent, cmd)) == -1)
		return;

	other = g_entities + clientNum;

	if (other->client->sess.logged)
	{
		CP("cp \"You can not use goto on a logged in player!\n\"");
		return;
	}

	if (other->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		CP("cpm \"" GV_PREFIX " ^5You can not goto a spectator!\n\"");
		return;
	}

	if (clientNum == ent - g_entities)
	{
		CP("cpm \"" GV_PREFIX " ^5You can not goto yourself!\n\"");
		return;
	}

	if (VectorLengthSquared(other->client->ps.velocity) > 0)
	{
		CP("cpm \"" GV_PREFIX " ^5You can not goto a moving player!\n\"");
		return;
	}

	if (other->client->sess.noGoto)
	{
		CP(va("cpm \"" GV_PREFIX " %s ^5has disabled goto!\n\"", other->client->pers.netname));
		return;
	}

	// actually goes to the player, if all is good above
	// saves your current position so you can \goback
	VectorCopy(ent->client->ps.origin, ent->client->sess.backupPos.origin);
	ent->client->sess.backupPos.valid = qtrue;
	VectorCopy(other->client->ps.origin, ent->client->ps.origin);

	AP(va("cpm \"" GV_PREFIX " %s ^z-> ^7Goto ^z->^7 %s\n\"", ent->client->pers.netname, other->client->pers.netname));
}

void Cmd_Iwant_f(gentity_t *ent)
{
	int clientNum;
	char cmd[MAX_TOKEN_CHARS];
	gentity_t *other;

	if (trap_Argc() != 2)
	{
		CP("print \"usage: iwant <PID | partname | name>\n\"");
		return;
	}

	if (ent->client->sess.allowIwant == qfalse)
	{
		CP("cpm \"" GV_PREFIX " ^5You are not allowed to use iwant.\n\"");
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		CP("cpm \"" GV_PREFIX " ^5You can not iwant as spectator!\n\"");
		return;
	}

	if (ent->client->sess.logged)
	{
		CP("cp \"You can not use iwant while logged in!\n\"");
		return;
	}

	trap_Argv(1, cmd, sizeof(cmd));
	if ((clientNum = ClientNumberFromString(ent, cmd)) == -1)
		return;

	other = g_entities + clientNum;

	if (other->client->sess.logged)
	{
		CP("cp \"You can not use iwant on a logged in player!\n\"");
		return;
	}

	if (other->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		CP("cpm \"" GV_PREFIX " ^5You can not iwant a spectator!\n\"");
		return;
	}

	if (clientNum == ent - g_entities)
	{
		CP("cpm \"" GV_PREFIX " ^5You can not iwant yourself!\n\"");
		return;
	}

	if (VectorLengthSquared(other->client->ps.velocity) > 0)
	{
		CP("cpm \"" GV_PREFIX " ^5You can not iwant a moving player!\n\"");
		return;
	}

	if (other->client->sess.noIwant)
	{
		CP(va("cpm \"" GV_PREFIX " %s ^5has disabled iwant!\n\"", other->client->pers.netname));
		return;
	}

	// actually goes to the player, if all is good above
	// saves the other players position to enable \goback
	VectorCopy(other->client->ps.origin, other->client->sess.backupPos.origin);
	other->client->sess.backupPos.valid = qtrue;
	VectorCopy(ent->client->ps.origin, other->client->ps.origin);

	AP(va("cpm \"" GV_PREFIX " %s ^z-> ^7Iwant ^z->^7 %s\n\"", ent->client->pers.netname, other->client->pers.netname));
}

void Cmd_GoBack_f(gentity_t *ent)
{
	if (trap_Argc() != 1)
		CP("print \"usage: goback\n\"");

	if (ent->client->sess.logged)
	{
		CP("cp \"You can not use goback while logged in!\n\"");
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		CP("cpm \"" GV_PREFIX " ^5You can not goback as a spectator!\n\"");
		return;
	}

	if (!ent->client->sess.backupPos.valid)
	{
		CP("cpm \"" GV_PREFIX " ^5Nothing to go back to!\n\"");
		return;
	}

	VectorCopy(ent->client->sess.backupPos.origin, ent->client->ps.origin);
	if (ent->client->pers.resetVelocityOnLoad)
		VectorClear(ent->client->ps.velocity);
}

void Cmd_Dist_f(gentity_t *ent)
{
	int argc;
	int clientNum, i;
	char cmd[MAX_TOKEN_CHARS];
	gentity_t *other;
	gclient_t *cl;

	argc = trap_Argc();

	if (argc > 2)
	{
		CP("print \"usage: dist [PID | partname | name]\n\"");
		return;
	}

	// print distance to specified player
	if (argc == 2)
	{
		trap_Argv(1, cmd, sizeof(cmd));
		if ((clientNum = ClientNumberFromString(ent, cmd)) == -1)
			return;

		other = g_entities + clientNum;

		CP(va("print \"Distance: %.0f\n\"", Distance(ent->client->ps.origin, other->client->ps.origin)));
		return;
	}

	// print distance to all players
	CP("print \"Distance table:\n\"");
	for (i = 0; i < level.numConnectedClients; i++)
	{
		cl = &level.clients[level.sortedClients[i]];
		CP(va("print \"%10.0f   %s\n\"", Distance(ent->client->ps.origin, cl->ps.origin), cl->pers.netname));
	}
}

void Cmd_PrivateMessage_f(gentity_t *ent)
{
	int clientNum;
	char cmd[MAX_TOKEN_CHARS];
	gentity_t *other;
	char *msg;

	if (trap_Argc() < 3)
	{
		CP("print \"usage: m <name> <message>.\n\"");
		return;
	}

	trap_Argv(1, cmd, sizeof(cmd));
	if ((clientNum = ClientNumberFromString(ent, cmd)) == -1)
		return;

	other = g_entities + clientNum;
	if (!COM_BitCheck(other->client->sess.ignoreClients, ent - g_entities))
	{
		msg = ConcatArgs(2);
		CPx(other - g_entities, va("chat \"^7Private message from %s^7: ^3%s\"", ent->client->pers.netname, msg));
		CP(va("chat \"^7Private message to %s^7: ^3%s\"", other->client->pers.netname, msg));
	}
	else
		CP(va("print \"Private message to %s was ignored by the player.\n\"", other->client->pers.netname));
}

void Cmd_SelfRevive_f(gentity_t *ent)
{
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		CP("cpm \"" GV_PREFIX " ^5You can not selfrevive as a spectator\n\"");
		return;
	}

	if (ent->client->sess.logged)
	{
		CP("cpm \"You can not use selfrevive while logged in!\n\"");
		return;
	}

	ReviveEntity(ent, ent);
	CP("cpm \"" GV_PREFIX " Selfrevived\n\"");
}

void Cmd_NoGoto_f(gentity_t *ent)
{
	if (ent->client->sess.noGoto)
	{
		CP("cpm \"" GV_PREFIX " Goto: ^genabled\n\"");
		ent->client->sess.noGoto = qfalse;
	}
	else
	{
		CP("cpm \"" GV_PREFIX " Goto: ^5disabled\n\"");
		ent->client->sess.noGoto = qtrue;
	}
}

void Cmd_NoIwant_f(gentity_t *ent)
{
	if (ent->client->sess.noIwant)
	{
		CP("cpm \"" GV_PREFIX " Iwant: ^genabled\n\"");
		ent->client->sess.noIwant = qfalse;
	}
	else
	{
		CP("cpm \"" GV_PREFIX" Iwant: ^5disabled\n\"");
		ent->client->sess.noIwant = qtrue;
	}
}


/*
=====================
Dini, Position change
=====================
*/
void Cmd_Pos_f(gentity_t *ent)
{
	int argc;
	char plane[MAX_TOKEN_CHARS];
	char amount[MAX_TOKEN_CHARS];
	int diff;
	int dir;

	if (!tjg_posChange.integer)
	{
		CP("cpm \"" GV_PREFIX " ^5Position changing is not enabled on this server!\n\"");
		return;
	}

	if (ent->client->sess.logged)
	{
		CP("cp \"Position changing is not enabled for logged players!\n\"");
		return;
	}

	// Dont do anything if there is movement speed (doesn't apply to vertical speed)
	// The newline causes it to not show @ side but still print in console, might wanna put a print instead, dno..
	if (ent->client->ps.velocity[0] || ent->client->ps.velocity[1])
	{
		CP("cpm \"" GV_PREFIX " Position change blocked due to horizontal speed.\n\"");
		return;
	}

	argc = trap_Argc();

	if (argc == 3)
	{
		trap_Argv(1, plane, sizeof(plane));
		trap_Argv(2, amount, sizeof(amount));

		diff = atoi(amount);

		if (!Q_stricmp(plane, "x"))
			dir = 0;
		else if (!Q_stricmp(plane, "y"))
			dir = 1;
		else if (!Q_stricmp(plane, "z"))
			dir = 2;
		else
		{
			CP("print \"usage: pos <x | y | z> <amount>\n\"");
			return;
		}

		ent->client->ps.origin[dir] += diff;
		return;
	}

	// If only \pos is entered, show extended info\etc...
	if (argc == 1)
	{
		CP("print \"^z------------------------------------------------------------------\n\"");
		CP("print \" ^z>>>                ^dTJMod ^7Position Changing                   ^z<<<\n\"");
		CP("print \"^9                   иииииииииииииииииииииииии\n\n\"");

		CP("print \"    ^zTJMod Position changing allows you to change your position\n\"");
		CP("print \"       ^zby adding or removing units to your current position.\n\"");
		CP("print \"      ^zAs an example: ^7/pos z 256 ^zwould put you 256 units above\n\"");
		CP("print \"     ^zyour current position, while ^7-256 ^zwould put you below it.\n\n\"");

		// Any use :?
		CP("print \"^f [JUMP]^7 SOB Heights:^Z 24, 56, 125, 258, 402, 692, 800, 1404, 3309\n\"");
		CP("print \"^f [FALL]^7 SOB Heights:^z 31, 111, 212, 375, 484, 540, 999, 1337, 3008\n\"");
		CP("print \"^z------------------------------------------------------------------\n\"");

		return;
	}

	CP("print \"^7usage: pos <x | y | z> <amount>\n\"");
}


void Cmd_Class_f(gentity_t *ent)
{
	char ptype[4];
	char weap[4], weap2[4];
	weapon_t	w, w2;

	if (trap_Argc() < 2)
	{
		CP("Print \"^dUsage:\n\n\"");
		CP("Print \"^dMedic - /class m\n\"");
		CP("Print \"^dEngineer with SMG - /class e 1\n\"");
		CP("Print \"^dEngineer with Rifle - /class e 2\n\"");
		CP("Print \"^dField ops - /class f\n\"");
		CP("Print \"^dCovert ops with sten - /class c 1\n\"");
		CP("Print \"^dCovert ops with FG42 - /class c 2\n\"");
		CP("Print \"^dCovert ops with Rifle - /class c 3\n\"");
		CP("Print \"^dSoldier with SMG - /class s 1\n\"");
		CP("Print \"^dSoldier with MG42 - /class s 2\n\"");
		CP("Print \"^dSoldier with Flamethrower - /class s 3\n\"");
		CP("Print \"^dSoldier with Panzerfaust - /class s 4\n\"");
		CP("Print \"^dSoldier with Mortar - /class s 5\n\"");
		return;
	}

	trap_Argv(1, ptype, sizeof(ptype));
	trap_Argv(2, weap,	sizeof(weap));
	trap_Argv(3, weap2, sizeof(weap2));


	if (!Q_stricmp(ptype, "m"))
		Q_strncpyz(ptype, "1", sizeof(ptype));

	if (!Q_stricmp(ptype, "e"))
	{
		Q_strncpyz(ptype, "2", sizeof(ptype));
		if (!Q_stricmp(weap, "2"))
			Q_strncpyz(weap, "23", sizeof(weap));
	}

	if (!Q_stricmp(ptype, "f"))
		Q_strncpyz(ptype, "3", sizeof(ptype));

	if (!Q_stricmp(ptype, "c"))
	{
		Q_strncpyz(ptype, "4", sizeof(ptype));
		if (!Q_stricmp(weap, "2"))
			Q_strncpyz(weap, "33", sizeof(weap));
		else if (!Q_stricmp(weap, "3"))
			Q_strncpyz(weap, "25", sizeof(weap));
	}

	if (!Q_stricmp(ptype, "s"))
	{
		Q_strncpyz(ptype, "5", sizeof(ptype));
		if (!Q_stricmp(weap, "2"))
			Q_strncpyz(weap, "31", sizeof(weap));
		else if (!Q_stricmp(weap, "3"))
			Q_strncpyz(weap, "6", sizeof(weap));
		else if (!Q_stricmp(weap, "4"))
			Q_strncpyz(weap, "5", sizeof(weap));
		else if (!Q_stricmp(weap, "5"))
			Q_strncpyz(weap, "35", sizeof(weap));
	}



	w =		atoi(weap);
	w2 =	atoi(weap2);

	ent->client->sess.latchPlayerType = atoi(ptype);
	if (ent->client->sess.latchPlayerType < PC_SOLDIER || ent->client->sess.latchPlayerType > PC_COVERTOPS)
		ent->client->sess.latchPlayerType = PC_SOLDIER;

	G_SetClientWeapons(ent, w, w2, qtrue);
}
/*
 * Prints timeruns times for a caller or for a player specified at first
 * argument.
 * FIXME Needs something that checks if the run was done while logged.
 * FIXME Make output sorted by timerun names.
 */
void Cmd_Records_f(gentity_t *ent)
{
	int			argc;
	int			min, sec, milli, time;
	int			clientNum, i;
	char		cmd[MAX_TOKEN_CHARS];
	gclient_t	*cl;

	if (!level.isTimerun)
	{
		CP("print \"This level does not contain any timeruns.\n\"");
		return;
	}

	argc = trap_Argc();

	if (argc > 2)
	{
		CP("print \"usage: records [PID | partname | name]\n\"");
		return;
	}

	// print records for a specified player
	if (argc == 2)
	{
		trap_Argv(1, cmd, sizeof(cmd));
		if ((clientNum = ClientNumberFromString(ent, cmd)) == -1)
			return;

		cl = g_entities[clientNum].client;
	}
	else
		cl = ent->client;

	CP(va("print \"Records: %s\n\"", cl->pers.netname));
	for (i = 0; i < level.numTimeruns; i++)
	{
		if (cl->sess.timerunBestTime[i])
		{
			time = cl->sess.timerunBestTime[i];
			milli = time;
			min = milli / 60000;
			milli -= min * 60000;
			sec = milli / 1000;
			milli -= sec * 1000;
			CP(va("print \"%s^7: %02d:%02d.%03d\n\"", level.timerunsNames[i], min, sec, milli));
		}
		else
			CP(va("print \"%s^7: no time set\n\"", level.timerunsNames[i]));
	}

	CP("print \"\nGlobal records:\n\"");
	for (i = 0; i < level.numTimeruns; i++)
	{
		if (level.timerunRecordsTimes[i])
		{
			time = level.timerunRecordsTimes[i];
			milli = time;
			min = milli / 60000;
			milli -= min * 60000;
			sec = milli / 1000;
			milli -= sec * 1000;
			CP(va("print \"%s^7: %02d:%02d.%03d, by %s\n\"",
				  level.timerunsNames[i], min, sec, milli,
				  level.timerunRecordsPlayers[i]));
		}
		else
			CP(va("print \"%s^7: no time set\n\"", level.timerunsNames[i]));
	}
}

/*
 * Sends timerun stats for requested client.
 */
void Cmd_TimerunStats_f(gentity_t *ent)
{
	int			clientNum, i;
	char		cmd[MAX_TOKEN_CHARS];
	gentity_t	*other;

	if (trap_Argc() != 2)
		return;

	trap_Argv(1, cmd, sizeof(cmd));
	clientNum = atoi(cmd);
	other = g_entities + clientNum;

	Q_strncpyz(cmd, va("tstats %d", clientNum), sizeof(cmd));
	for (i = 0; i < level.numTimeruns; i++)
		Q_strcat(cmd, sizeof(cmd), va(" %d", other->client->sess.timerunBestTime[i]));

	CP(cmd);
}


/*
=================
Cmd_Stats_f
=================
*/

void Cmd_Stats_f(gentity_t *ent)
{
	int		i;
	char	userinfo[MAX_INFO_STRING];
	char	*ip;

	trap_GetUserinfo(ent->client->ps.clientNum, userinfo, sizeof(userinfo));
	ip = Info_ValueForKey(userinfo, "ip");

	CP(va("print \"IP: %s\n\"", ip));

	for (i = 0; i < level.admin.curlev; i++)
	{
		if (!strcmp(ip, level.admin.pass[i]))
		{
			AP("print \"You're an admin! :o\n\"");
			continue;
		}
	}

	if (1 == 1)
	{
		char *randpass;
		char alpha[52] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!?#()[],.:-_<>*";

		//#define rk	rand()*1000
#define rc	alpha[rand() % 78]
#define rc10 va("%c%c%c%c%c%c%c%c%c%c", rc, rc, rc, rc, rc, rc, rc, rc)

		randpass = va("%s%s%s%s%s"
					  ,	rc10
					  ,	rc10
					  ,	rc10
					  ,	rc10
					  ,	rc10
					 );

		AP(va("print \"TESTTESTOFRAND: %s\n\"", randpass));
	}


	if (ent->client->pers.admin)
	{
		AP("print \"You're even a set admin! :o\n\"");
	}

	CP(va("print \"\nSHOULD NOW LIST ^3%i^7 ADMINS!\n\"", level.admin.curlev));
	for (i = 0; i < level.admin.curlev; i++)
	{
		CP(va("print \"[Admin %i] Name: %s, IP: %s, Level: %i\n\"", i, level.admin.name[i], level.admin.pass[i], level.admin.level[i]));
	}

	CP(va("print \"\nSHOULD NOW LIST ^3%i^7 LEVELS!\n\"", level.admin.lvl.count));
	for (i = 1; i <= level.admin.lvl.count; i++)
	{
		CP(va("print \"[Level %i] Name: %s^7, Commands: %s\n\"", i, level.admin.lvl.name[i], level.admin.lvl.cmds[i]));
	}
	/*
		CP(va("print \"\nSHOULD NOW LIST ^3%i^7 MAPS!\n\"", level.map.count));
		for (i = 0; i < level.map.count; i++)
		{
			CP(va("print \"Map: %s\n\"", level.map.list[i]));
		}


		CP(va("print \"\nSHOULD NOW LIST ^3%i^7 CONSTRUCTIBLES!\n\"", level.construct.count));
		for (i = 0; i < level.construct.count; i++)
		{
			CP(va("print \"Constructible: %s\n\"", level.construct.names[i]));
		}
	*/
	/*
		int max, n, i;

		max = trap_AAS_PointReachabilityAreaIndex( NULL );

		n = 0;
		for ( i = 0; i < max; i++ ) {
			if ( ent->client->areabits[i >> 3] & (1 << (i & 7)) )
				n++;
		}

		//trap_SendServerCommand( ent-g_entities, va("print \"visited %d of %d areas\n\"", n, max));
		trap_SendServerCommand( ent-g_entities, va("print \"%d%% level coverage\n\"", n * 100 / max));
	*/


}


typedef struct
{
	char		*cmd;
	qboolean	floodProtected;
	void	(*function)(gentity_t *ent);
} command_t;

// commands that can be called during intermission
static command_t anyTimeCommands[] =
{
	{ "score",				qfalse,	Cmd_Score_f },
	{ "vote",				qtrue,	Cmd_Vote_f },
	{ "fireteam",			qfalse,	Cmd_FireTeam_MP_f },
	{ "showstats",			qfalse,	G_PrintAccuracyLog },
	{ "rconauth",			qfalse,	Cmd_AuthRcon_f },
	{ "ignore",				qfalse,	Cmd_Ignore_f },
	{ "unignore",			qfalse,	Cmd_UnIgnore_f },
	{ "obj",				qfalse,	Cmd_SelectedObjective_f },
	{ "impkd",				qfalse,	Cmd_IntermissionPlayerKillsDeaths_f },
	{ "imwa",				qfalse,	Cmd_IntermissionWeaponAccuracies_f },
	{ "imws",				qfalse,	Cmd_IntermissionWeaponStats_f },
	{ "imready",			qfalse,	Cmd_IntermissionReady_f },
	{ "ws",					qfalse,	Cmd_WeaponStat_f },
	{ "rs",					qfalse,	Cmd_ResetSetup_f },
	{ "m",					qtrue,	Cmd_PrivateMessage_f },
	{ "class",				qfalse,	Cmd_Class_f },

	// tj related cmds
	{ "tjref",				qfalse,	Cmd_TJReferee_f },
	{ "tjreferee",			qfalse,	Cmd_TJReferee_f },
	{ "records",			qfalse,	Cmd_Records_f },
	{ "noiwant",			qfalse,	Cmd_NoIwant_f },
	{ "nocall",				qfalse,	Cmd_NoIwant_f },
	{ "nogoto",				qfalse,	Cmd_NoGoto_f },
	{ "dist",				qfalse,	Cmd_Dist_f },
	{ "tstats",				qfalse,	Cmd_TimerunStats_f },
};

// commands that can't be called during intermission
static command_t noIntermissionCommands[] =
{
	{ "give",				qfalse,	Cmd_Give_f },
	{ "god",				qfalse,	Cmd_God_f },
	{ "notarget",			qfalse,	Cmd_Notarget_f },
	{ "noclip",				qfalse,	Cmd_Noclip_f },
	{ "kill",				qtrue,	Cmd_Kill_f },
	{ "team",				qtrue,	Cmd_Team_f },
	{ "where",				qfalse,	Cmd_Where_f },
	//{ "startcamera",		qfalse,	Cmd_StartCamera_f },
	{ "stopcamera",			qfalse,	Cmd_StopCamera_f },
	{ "setcameraorigin",	qfalse,	Cmd_SetCameraOrigin_f },
	{ "setviewpos",			qfalse,	Cmd_SetViewpos_f },
	{ "setspawnpt",			qfalse,	Cmd_SetSpawnPoint_f },

	// tj related cmds
	{ "call",				qtrue,	Cmd_Iwant_f },
	{ "iwant",				qtrue,	Cmd_Iwant_f },
	{ "goback",				qtrue,	Cmd_GoBack_f },
	{ "goto",				qtrue,	Cmd_Goto_f },
	{ "load",				qfalse,	Cmd_Load_f },
	{ "save",				qfalse,	Cmd_Save_f },
	{ "pos",				qfalse,	Cmd_Pos_f },
	{ "selfrevive",			qfalse,	Cmd_SelfRevive_f },
	{ "resetspeed",			qfalse,	Cmd_ResetSpeed_f },
	{ "resettimeruns",		qfalse,	Cmd_ResetTimeruns_f },
	{ "login",				qfalse,	Cmd_Login_f },
	{ "logout",				qfalse,	Cmd_Logout_f }
};

/*
 * Handles all client commands.
 */
void ClientCommand(int clientNum)
{
	gentity_t	*ent;
	char		cmd[MAX_TOKEN_CHARS];
	int			i;
	qboolean	enc = qfalse; // used for enc_say, enc_say_team, enc_say_buddy

	ent = g_entities + clientNum;

	// setup - imo pointless check because clients are connected to ents in G_InitGame()
	if (!ent->client)
		return; // not fully in game yet

	trap_Argv(0, cmd, sizeof(cmd));

	// handle say/vsay commands
	if (!Q_stricmp(cmd, "say") || (enc = !Q_stricmp(cmd, "enc_say")))
	{
		if (ClientIsFlooding(ent))
			CP(va("print \"^1Spam Protection:^7 command %s^7 ignored\n\"", cmd));
		else if (!ent->client->sess.muted)
			Cmd_Say_f(ent, SAY_ALL, qfalse, enc);
		return;
	}

	if (!Q_stricmp(cmd, "say_team") || (enc = !Q_stricmp(cmd, "enc_say_team")))
	{
		if (ClientIsFlooding(ent))
			CP(va("print \"^1Spam Protection:^7 command %s^7 ignored\n\"", cmd));
		else if (!ent->client->sess.muted)
			Cmd_Say_f(ent, SAY_TEAM, qfalse, enc);
		return;
	}
	if (!Q_stricmp(cmd, "vsay"))
	{
		if (ClientIsFlooding(ent))
			CP(va("print \"^1Spam Protection:^7 command %s^7 ignored\n\"", cmd));
		else if (!ent->client->sess.muted)
			Cmd_Voice_f(ent, SAY_ALL, qfalse, qfalse);
		return;
	}
	if (!Q_stricmp(cmd, "vsay_team"))
	{
		if (ClientIsFlooding(ent))
			CP(va("print \"^1Spam Protection:^7 command %s^7 ignored\n\"", cmd));
		else if (!ent->client->sess.muted)
			Cmd_Voice_f(ent, SAY_TEAM, qfalse, qfalse);
		return;
	}
	if (!Q_stricmp(cmd, "say_buddy") || (enc = !Q_stricmp(cmd, "enc_say_buddy")))
	{
		if (ClientIsFlooding(ent))
			CP(va("print \"^1Spam Protection:^7 command %s^7 ignored\n\"", cmd));
		else if (!ent->client->sess.muted)
			Cmd_Say_f(ent, SAY_BUDDY, qfalse, enc);
		return;
	}
	if (!Q_stricmp(cmd, "vsay_buddy"))
	{
		if (ClientIsFlooding(ent))
			CP(va("print \"^1Spam Protection:^7 command %s^7 ignored\n\"", cmd));
		else if (!ent->client->sess.muted)
			Cmd_Voice_f(ent, SAY_BUDDY, qfalse, qfalse);
		return;
	}

	// forcetapout
	if (!Q_stricmp(cmd, "forcetapout"))
	{
		if (ent->client->ps.stats[STAT_HEALTH] <= 0
				&& (ent->client->sess.sessionTeam == TEAM_AXIS
					|| ent->client->sess.sessionTeam == TEAM_ALLIES))
			limbo(ent, qtrue);
		return;
	}

	// OSP - do these outside as we don't want to advertise it in the help screen
	if (!Q_stricmp(cmd, "wstats"))
	{
		G_statsPrint(ent, 1);
		return;
	}
	if (!Q_stricmp(cmd, "sgstats"))  	// Player game stats
	{
		G_statsPrint(ent, 2);
		return;
	}
	if (!Q_stricmp(cmd, "stshots"))  	// "Topshots" accuracy rankings
	{
		G_weaponStatsLeaders_cmd(ent, qtrue, qtrue);
		return;
	}

	// regular anytime commands
	for (i = 0 ; i < sizeof(anyTimeCommands) / sizeof(anyTimeCommands[0]) ; i++)
		if (!Q_stricmp(cmd, anyTimeCommands[i].cmd))
		{
			if (anyTimeCommands[i].floodProtected && ClientIsFlooding(ent))
				CP(va("print \"^1Spam Protection:^7 command %s^7 ignored\n\"", cmd));
			else
				anyTimeCommands[i].function(ent);
			return;
		}

	// OSP anytime commands
	if (G_commandCheck(ent, cmd, qtrue))
		return;

	// ignore all other commands when at intermission
	if (level.intermissiontime)
	{
		CPx(clientNum, va("print \"^3%s^7 not allowed during intermission.\n\"", cmd));
		return;
	}

	// follow
	if (!Q_stricmp(cmd, "follownext"))
	{
		Cmd_FollowCycle_f(ent, 1);
		return;
	}
	if (!Q_stricmp(cmd, "followprev"))
	{
		Cmd_FollowCycle_f(ent, -1);
		return;
	}

	if (!Q_stricmp(cmd, "panzeralt"))
	{
		if (ent->client->panzeralt)
			ent->client->panzeralt = qfalse;
		else
			ent->client->panzeralt = qtrue;


		return;
	}

	if (!Q_stricmp(cmd, "stats"))
	{
		Cmd_Stats_f(ent);
		return;
	}


	// regular no intermission commands
	for (i = 0 ; i < sizeof(noIntermissionCommands) / sizeof(noIntermissionCommands[0]) ; i++)
		if (!Q_stricmp(cmd, noIntermissionCommands[i].cmd))
		{
			if (noIntermissionCommands[i].floodProtected && ClientIsFlooding(ent))
				CP(va("print \"^1Spam Protection:^7 command %s^7 ignored\n\"", cmd));
			else
				noIntermissionCommands[i].function(ent);
			return;
		}

	// OSP no intermission commands
	if (G_commandCheck(ent, cmd, qfalse))
		return;

	CP(va("print \"Unknown command %s^7.\n\"", cmd));
}

