/*
 * General game functions including vmMain (only entry point for the engine)
 * and cvars handling.
 */


#include "g_local.h"

level_locals_t	level;

typedef struct
{
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int			cvarFlags;
	int			modificationCount;	// for tracking changes
	qboolean	trackChange;		// track this variable, and announce if changed
	qboolean	fConfigReset;		// OSP: set this var to the default on a config reset
} cvarTable_t;

gentity_t		g_entities[MAX_GENTITIES];
gclient_t		g_clients[MAX_CLIENTS];

g_campaignInfo_t g_campaigns[MAX_CAMPAIGNS];
int				saveGamePending;	// 0 = no, 1 = check, 2 = loading

mapEntityData_Team_t mapEntityData[2];

vmCvar_t	g_gametype;
vmCvar_t	g_timelimit;
vmCvar_t	g_friendlyFire;
vmCvar_t	g_password;
vmCvar_t	sv_privatepassword;
vmCvar_t	g_maxclients;
vmCvar_t	g_maxGameClients;
vmCvar_t	g_minGameClients;		// NERVE - SMF
vmCvar_t	g_dedicated;
vmCvar_t	g_cheats;
vmCvar_t	g_knockback;
vmCvar_t	g_quadfactor;
vmCvar_t	g_forcerespawn;
vmCvar_t	g_inactivity;
vmCvar_t	g_debugMove;
vmCvar_t	g_debugDamage;
vmCvar_t	g_debugAlloc;
vmCvar_t	g_debugBullets;	//----(SA)	added
vmCvar_t	g_motd;
#ifdef ALLOW_GSYNC
vmCvar_t	g_synchronousClients;
#endif // ALLOW_GSYNC

// NERVE - SMF
vmCvar_t	g_nextTimeLimit;
vmCvar_t	g_showHeadshotRatio;
vmCvar_t	g_userTimeLimit;
vmCvar_t	g_userAlliedRespawnTime;
vmCvar_t	g_userAxisRespawnTime;
vmCvar_t	g_currentRound;
vmCvar_t	g_noTeamSwitching;
vmCvar_t	g_altStopwatchMode;
vmCvar_t	g_gamestate;
vmCvar_t	g_swapteams;
// -NERVE - SMF

vmCvar_t	g_restarted;
vmCvar_t	g_log;
vmCvar_t	g_logSync;
vmCvar_t	g_podiumDist;
vmCvar_t	g_podiumDrop;
vmCvar_t	voteFlags;
vmCvar_t	g_complaintlimit;		// DHM - Nerve
vmCvar_t	g_ipcomplaintlimit;
vmCvar_t	g_filtercams;
vmCvar_t	g_maxlives;				// DHM - Nerve
vmCvar_t	g_maxlivesRespawnPenalty;
vmCvar_t	g_voiceChatsAllowed;	// DHM - Nerve
vmCvar_t	g_alliedmaxlives;		// Xian
vmCvar_t	g_axismaxlives;			// Xian
vmCvar_t	g_fastres;				// Xian
vmCvar_t	g_knifeonly;			// Xian
vmCvar_t	g_enforcemaxlives;		// Xian

vmCvar_t	g_needpass;
vmCvar_t	g_balancedteams;
vmCvar_t	g_teamAutoJoin;
vmCvar_t	g_teamForceBalance;
vmCvar_t	g_banIPs;
vmCvar_t	g_filterBan;
vmCvar_t	g_rankings;
vmCvar_t	g_smoothClients;
vmCvar_t	pmove_fixed;
vmCvar_t	pmove_msec;

// Rafael
vmCvar_t	g_scriptName;		// name of script file to run (instead of default for that map)

vmCvar_t	g_developer;

vmCvar_t	g_userAim;

vmCvar_t	g_footstepAudibleRange;
// JPW NERVE multiplayer reinforcement times
vmCvar_t		g_redlimbotime;
vmCvar_t		g_bluelimbotime;
// charge times for character class special weapons
vmCvar_t		g_medicChargeTime;
vmCvar_t		g_engineerChargeTime;
vmCvar_t		g_LTChargeTime;
vmCvar_t		g_soldierChargeTime;
// screen shakey magnitude multiplier

// Gordon
vmCvar_t		g_antilag;

// OSP
vmCvar_t		g_spectatorInactivity;
vmCvar_t		match_latejoin;
vmCvar_t		match_minplayers;
vmCvar_t		match_mutespecs;
vmCvar_t		match_readypercent;
vmCvar_t		match_timeoutcount;
vmCvar_t		match_timeoutlength;
vmCvar_t		server_autoconfig;
vmCvar_t		team_maxPanzers;
vmCvar_t		team_maxplayers;
vmCvar_t		team_nocontrols;
vmCvar_t		server_motd0;
vmCvar_t		server_motd1;
vmCvar_t		server_motd2;
vmCvar_t		server_motd3;
vmCvar_t		server_motd4;
vmCvar_t		server_motd5;
vmCvar_t		vote_allow_comp;
vmCvar_t		vote_allow_gametype;
vmCvar_t		vote_allow_kick;
vmCvar_t		vote_allow_map;
vmCvar_t		vote_allow_matchreset;
vmCvar_t		vote_allow_mutespecs;
vmCvar_t		vote_allow_nextmap;
vmCvar_t		vote_allow_pub;
vmCvar_t		vote_allow_referee;
vmCvar_t		vote_allow_shuffleteamsxp;
vmCvar_t		vote_allow_swapteams;
vmCvar_t		vote_allow_friendlyfire;
vmCvar_t		vote_allow_timelimit;
vmCvar_t		vote_allow_antilag;
vmCvar_t		vote_allow_balancedteams;
vmCvar_t		vote_allow_muting;
vmCvar_t		vote_limit;
vmCvar_t		vote_percent;
vmCvar_t		z_serverflags;


vmCvar_t		g_covertopsChargeTime;
vmCvar_t		refereePassword;
vmCvar_t		tjrefereePassword;
vmCvar_t		g_debugConstruct;
vmCvar_t		g_landminetimeout;

// Variable for setting the current level of debug printing/logging
// enabled in bot scripts and regular scripts.
// Added by Mad Doctor I, 8/23/2002
vmCvar_t		g_scriptDebugLevel;
vmCvar_t		g_movespeed;

vmCvar_t g_axismapxp;
vmCvar_t g_alliedmapxp;
vmCvar_t g_oldCampaign;
vmCvar_t g_currentCampaign;
vmCvar_t g_currentCampaignMap;

// Arnout: for LMS
vmCvar_t g_axiswins;
vmCvar_t g_alliedwins;
vmCvar_t g_lms_teamForceBalance;
vmCvar_t g_lms_roundlimit;
vmCvar_t g_lms_matchlimit;
vmCvar_t g_lms_currentMatch;
vmCvar_t g_lms_lockTeams;
vmCvar_t g_lms_followTeamOnly;

#ifdef SAVEGAME_SUPPORT
vmCvar_t		g_reloading;
#endif // SAVEGAME_SUPPORT

vmCvar_t		mod_url;
vmCvar_t		url;

vmCvar_t		g_letterbox;
vmCvar_t		bot_enable;

vmCvar_t		g_debugSkills;
vmCvar_t		g_heavyWeaponRestriction;

vmCvar_t		g_nextmap;
vmCvar_t		g_nextcampaign;

vmCvar_t		tjg_SaveLoad;
vmCvar_t		tjg_strictSL;
vmCvar_t		tjg_altSL;

vmCvar_t		tjg_Nofatigue;

// Dini - wm_endround disabled\enabled
vmCvar_t		tjg_EndRound;

// Nolla - DisableResetSpeed
vmCvar_t		tjg_ResetSpeed;

// Dini, extra setting for noclip
// Allows noclip to work without cheats being enabled!
vmCvar_t		tjg_Noclip;

// Dini, extra setting for god
vmCvar_t		tjg_God;

// Dini, mapscript support
vmCvar_t		tjg_mapScriptDirectory;

// Dini, Disable all weapons
vmCvar_t		tjg_maxConnections;

// Dini, push
vmCvar_t		tjg_shove;

// Dini, Spamprotection ala ETPub
vmCvar_t		tjg_floodprotection;
vmCvar_t		tjg_floodlimit;
vmCvar_t		tjg_floodwait;

// Dini, allow position change
vmCvar_t		tjg_posChange;

// Nolla, MaxWarnCount
vmCvar_t		tjg_maxWarnCount;


// TJMOD WEAPON RELATED
// Dini, Disabled G_Damage
vmCvar_t		tjg_damage;

// Dini, Disable all weapons
vmCvar_t		tjg_weapons;

// Different logfile timestamps
vmCvar_t		tjg_logTime;

vmCvar_t		tjg_dailyLogs;

// Record stuff
vmCvar_t		tjg_recordMode;
vmCvar_t		tjg_disabled_ob;
vmCvar_t		tjg_recordFile;
vmCvar_t		tjg_localrecords;

vmCvar_t		tjg_trigger_savereset;
vmCvar_t		tjg_mapExplosiveToggle;
vmCvar_t		tjg_mapKillEntities;
vmCvar_t		tjg_holdDoorsOpen;
vmCvar_t		tjg_blockedMaps;

vmCvar_t		isTimerun;
vmCvar_t		physics;

vmCvar_t		tjg_q3teles;
vmCvar_t		tjg_timerwait;

vmCvar_t		tjg_admin;
vmCvar_t		tjg_missilepassthrough;
vmCvar_t		tjg_votewait;

cvarTable_t		gameCvarTable[] =
{
	// don't override the cheat state set by the system
	{ &g_cheats, "sv_cheats", "", 0, qfalse },

	// noset vars
	// Dini, the modname shown for ... everything, serverbrowsers, ingame serverinfo, etcetc..
	{ NULL, "gamename", GAMEVERSION" "TJMOD_VERSION, CVAR_SERVERINFO | CVAR_ROM, 0, qfalse  },
	{ NULL, "gamedate", __DATE__ , CVAR_ROM, 0, qfalse  },
	{ &g_restarted, "g_restarted", "0", CVAR_ROM, 0, qfalse  },
	{ NULL, "sv_mapname", "", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse  },

	// latched vars
	{ &g_gametype, "g_gametype", "2", CVAR_SERVERINFO | CVAR_LATCH, 0, qfalse  },		// Arnout: default to GT_WOLF_CAMPAIGN

	// JPW NERVE multiplayer stuffs
	{ &g_redlimbotime, "g_redlimbotime", "1000", CVAR_SERVERINFO | CVAR_LATCH, 0, qfalse },
	{ &g_bluelimbotime, "g_bluelimbotime", "1000", CVAR_SERVERINFO | CVAR_LATCH, 0, qfalse },
	{ &g_medicChargeTime, "g_medicChargeTime", "100", CVAR_SERVERINFO | CVAR_LATCH, 0, qfalse, qtrue },
	{ &g_engineerChargeTime, "g_engineerChargeTime", "100", CVAR_SERVERINFO | CVAR_LATCH, 0, qfalse, qtrue },
	{ &g_LTChargeTime, "g_LTChargeTime", "100", CVAR_SERVERINFO | CVAR_LATCH, 0, qfalse, qtrue },
	{ &g_soldierChargeTime, "g_soldierChargeTime", "100", CVAR_SERVERINFO | CVAR_LATCH, 0, qfalse, qtrue },
	// jpw

	{ &g_covertopsChargeTime, "g_covertopsChargeTime", "100", CVAR_SERVERINFO | CVAR_LATCH, 0, qfalse, qtrue },
	{ &g_landminetimeout, "g_landminetimeout", "1", CVAR_ARCHIVE, 0, qfalse, qtrue },

	{ &g_maxclients, "sv_maxclients", "20", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qfalse  },			// NERVE - SMF - made 20 from 8
	{ &g_maxGameClients, "g_maxGameClients", "0", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qfalse  },
	{ &g_minGameClients, "g_minGameClients", "8", CVAR_SERVERINFO, 0, qfalse  },								// NERVE - SMF

	// change anytime vars
	{ &g_timelimit, "timelimit", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },

#ifdef ALLOW_GSYNC
	{ &g_synchronousClients, "g_synchronousClients", "0", CVAR_SYSTEMINFO | CVAR_CHEAT, 0, qfalse  },
#endif // ALLOW_GSYNC

	{ &g_friendlyFire, "g_friendlyFire", "1", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue, qtrue },

	{ &g_teamForceBalance, "g_teamForceBalance", "0", CVAR_ARCHIVE  },							// NERVE - SMF - merge from team arena

	{ &g_nextTimeLimit, "g_nextTimeLimit", "0", CVAR_WOLFINFO, 0, qfalse  },
	{ &g_currentRound, "g_currentRound", "0", CVAR_WOLFINFO, 0, qfalse, qtrue },
	{ &g_altStopwatchMode, "g_altStopwatchMode", "0", CVAR_ARCHIVE, 0, qtrue, qtrue },
	{ &g_gamestate, "gamestate", "-1", CVAR_WOLFINFO | CVAR_ROM, 0, qfalse  },

	{ &g_noTeamSwitching, "g_noTeamSwitching", "0", CVAR_ARCHIVE, 0, qtrue  },

	{ &g_showHeadshotRatio, "g_showHeadshotRatio", "0", 0, 0, qfalse  },

	{ &g_userTimeLimit, "g_userTimeLimit", "0", 0, 0, qfalse, qtrue },
	{ &g_userAlliedRespawnTime, "g_userAlliedRespawnTime", "0", 0, 0, qfalse, qtrue },
	{ &g_userAxisRespawnTime, "g_userAxisRespawnTime", "0", 0, 0, qfalse, qtrue },

	{ &g_swapteams, "g_swapteams", "0", CVAR_ROM, 0, qfalse, qtrue },
	// -NERVE - SMF

	{ &g_log, "g_log", "", CVAR_ARCHIVE, 0, qfalse },
	{ &g_logSync, "g_logSync", "0", CVAR_ARCHIVE, 0, qfalse },

	{ &g_password, "g_password", "none", CVAR_USERINFO, 0, qfalse },
	{ &sv_privatepassword, "sv_privatepassword", "", CVAR_TEMP, 0, qfalse },
	{ &g_banIPs, "g_banIPs", "", CVAR_ARCHIVE, 0, qfalse },
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=500
	{ &g_filterBan, "g_filterBan", "1", CVAR_ARCHIVE, 0, qfalse },

	{ &g_dedicated, "dedicated", "0", 0, 0, qfalse },

	{ &g_knockback, "g_knockback", "500", CVAR_CHEAT, 0, qtrue, qtrue },
	{ &g_quadfactor, "g_quadfactor", "3", 0, 0, qtrue },

	{ &g_needpass, "g_needpass", "0", CVAR_SERVERINFO | CVAR_ROM, 0, qtrue },
	{ &g_balancedteams, "g_balancedteams", "0", CVAR_SERVERINFO | CVAR_ROM, 0, qtrue },
	{ &g_forcerespawn, "g_forcerespawn", "0", 0, 0, qtrue },
	{ &g_forcerespawn, "g_forcerespawn", "0", 0, 0, qtrue },
	{ &g_inactivity, "g_inactivity", "0", 0, 0, qtrue },
	{ &g_debugMove, "g_debugMove", "0", 0, 0, qfalse },
	{ &g_debugDamage, "g_debugDamage", "0", CVAR_CHEAT, 0, qfalse },
	{ &g_debugAlloc, "g_debugAlloc", "0", 0, 0, qfalse },
	{ &g_debugBullets, "g_debugBullets", "0", CVAR_ARCHIVE, 0, qfalse},	//----(SA)	added
	{ &g_motd, "g_motd", "", CVAR_ARCHIVE, 0, qfalse },

	{ &g_podiumDist, "g_podiumDist", "80", 0, 0, qfalse },
	{ &g_podiumDrop, "g_podiumDrop", "70", 0, 0, qfalse },

	{ &voteFlags, "voteFlags", "0", CVAR_TEMP | CVAR_ROM | CVAR_SERVERINFO, 0, qfalse },

	{ &g_complaintlimit, "g_complaintlimit", "6", CVAR_ARCHIVE, 0, qtrue },						// DHM - Nerve
	{ &g_ipcomplaintlimit, "g_ipcomplaintlimit", "3", CVAR_ARCHIVE, 0, qtrue },
	{ &g_filtercams, "g_filtercams", "0", CVAR_ARCHIVE, 0, qfalse },
	{ &g_maxlives, "g_maxlives", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_SERVERINFO, 0, qtrue },		// DHM - Nerve
	{ &g_maxlivesRespawnPenalty, "g_maxlivesRespawnPenalty", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_SERVERINFO, 0, qtrue },
	{ &g_voiceChatsAllowed, "g_voiceChatsAllowed", "4", CVAR_ARCHIVE, 0, qfalse },				// DHM - Nerve

	{ &g_alliedmaxlives, "g_alliedmaxlives", "0", CVAR_LATCH | CVAR_SERVERINFO, 0, qtrue },		// Xian
	{ &g_axismaxlives, "g_axismaxlives", "0", CVAR_LATCH | CVAR_SERVERINFO, 0, qtrue },			// Xian
	{ &g_fastres, "g_fastres", "0", CVAR_ARCHIVE, 0, qtrue, qtrue },							// Xian - Fast Medic Resing
	{ &g_knifeonly, "g_knifeonly", "0", 0, 0, qtrue },											// Xian - Fast Medic Resing
	{ &g_enforcemaxlives, "g_enforcemaxlives", "1", CVAR_ARCHIVE, 0, qtrue },					// Xian - Gestapo enforce maxlives stuff by temp banning

	{ &g_developer, "developer", "0", CVAR_TEMP, 0, qfalse },
	{ &g_rankings, "g_rankings", "0", 0, 0, qfalse },
	{ &g_userAim, "g_userAim", "1", CVAR_CHEAT, 0, qfalse },

	{ &g_smoothClients, "g_smoothClients", "1", 0, 0, qfalse },
	{ &pmove_fixed, "pmove_fixed", "0", 0, 0, qfalse },
	{ &pmove_msec, "pmove_msec", "8", CVAR_SYSTEMINFO, 0, qfalse },

	{ &g_footstepAudibleRange, "g_footstepAudibleRange", "256", CVAR_CHEAT, 0, qfalse },

	{ &g_scriptName, "g_scriptName", "", CVAR_TEMP, 0, qfalse },

	{ &g_antilag, "g_antilag", "1", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qfalse },

	//bani - #184
	{ NULL, "P", "", CVAR_SERVERINFO_NOUPDATE, 0, qfalse, qfalse },

	{ &refereePassword, "refereePassword", "none", 0, 0, qfalse},
	{ &tjrefereePassword, "tjrefereePassword", "none", 0, 0, qfalse},
	{ &g_spectatorInactivity, "g_spectatorInactivity", "0", 0, 0, qfalse, qfalse },
	{ &match_latejoin,		"match_latejoin", "1", 0, 0, qfalse, qfalse },
	{ &match_minplayers,	"match_minplayers", MATCH_MINPLAYERS, 0, 0, qfalse, qfalse },
	{ &match_mutespecs,		"match_mutespecs", "0", 0, 0, qfalse, qtrue },
	{ &match_readypercent,	"match_readypercent", "50", 0, 0, qfalse, qtrue },
	{ &match_timeoutcount,	"match_timeoutcount", "3", 0, 0, qfalse, qtrue },
	{ &match_timeoutlength,	"match_timeoutlength", "180", 0, 0, qfalse, qtrue },
	{ &server_autoconfig, "server_autoconfig", "0", 0, 0, qfalse, qfalse },
	{ &server_motd0,	"server_motd0", "^7Enemy Territory", 0, 0, qfalse, qfalse },
	{ &server_motd1,	"server_motd1", "", 0, 0, qfalse, qfalse },
	{ &server_motd2,	"server_motd2", "^dTJMod Server", 0, 0, qfalse, qfalse },
	{ &server_motd3,	"server_motd3", "", 0, 0, qfalse, qfalse },
	{ &server_motd4,	"server_motd4", "", 0, 0, qfalse, qfalse },
	{ &server_motd5,	"server_motd5", "^zwww.^7Trickjump^z.me", 0, 0, qfalse, qfalse },
	{ &team_maxPanzers, "team_maxPanzers", "-1", 0, 0, qfalse, qfalse },
	{ &team_maxplayers, "team_maxplayers", "0", 0, 0, qfalse, qfalse },
	{ &team_nocontrols, "team_nocontrols", "1", 0, 0, qfalse, qfalse },
	{ &vote_allow_comp,			"vote_allow_comp", "0", 0, 0, qfalse, qfalse },
	{ &vote_allow_gametype,		"vote_allow_gametype", "0", 0, 0, qfalse, qfalse },
	{ &vote_allow_kick,			"vote_allow_kick", "1", 0, 0, qfalse, qfalse },
	{ &vote_allow_map,			"vote_allow_map", "1", 0, 0, qfalse, qfalse },
	{ &vote_allow_matchreset,	"vote_allow_matchreset", "1", 0, 0, qfalse, qfalse },
	{ &vote_allow_mutespecs,	"vote_allow_mutespecs", "1", 0, 0, qfalse, qfalse },
	{ &vote_allow_nextmap,		"vote_allow_nextmap", "1", 0, 0, qfalse, qfalse },
	{ &vote_allow_pub,			"vote_allow_pub", "0", 0, 0, qfalse, qfalse },
	{ &vote_allow_referee,		"vote_allow_referee", "0", 0, 0, qfalse, qfalse },
	{ &vote_allow_shuffleteamsxp,	"vote_allow_shuffleteamsxp", "0", 0, 0, qfalse, qfalse },
	{ &vote_allow_swapteams,	"vote_allow_swapteams", "0", 0, 0, qfalse, qfalse },
	{ &vote_allow_friendlyfire,	"vote_allow_friendlyfire", "1", 0, 0, qfalse, qfalse },
	{ &vote_allow_timelimit,	"vote_allow_timelimit", "0", 0, 0, qfalse, qfalse },
	{ &vote_allow_antilag,		"vote_allow_antilag", "0", 0, 0, qfalse, qfalse },
	{ &vote_allow_balancedteams, "vote_allow_balancedteams", "0", 0, 0, qfalse, qfalse },
	{ &vote_allow_muting,		"vote_allow_muting", "1", 0, 0, qfalse, qfalse },
	{ &vote_limit,		"vote_limit", "5", 0, 0, qfalse, qfalse },
	{ &vote_percent,	"vote_percent", "51", 0, 0, qfalse, qfalse },

	// state vars
	{ &z_serverflags, "z_serverflags", "0", 0, 0, qfalse, qfalse },

	{ &g_debugConstruct, "g_debugConstruct", "0", CVAR_CHEAT, 0, qfalse },

	{ &g_scriptDebug, "g_scriptDebug", "0", CVAR_CHEAT, 0, qfalse },

	// What level of detail do we want script printing to go to.
	{ &g_scriptDebugLevel, "g_scriptDebugLevel", "0", CVAR_CHEAT, 0, qfalse },

	// How fast do we want Allied single player movement?
	//	{ &g_movespeed, "g_movespeed", "127", CVAR_CHEAT, 0, qfalse },
	{ &g_movespeed, "g_movespeed", "76", CVAR_CHEAT, 0, qfalse },

	// Arnout: LMS
	{ &g_lms_teamForceBalance,	"g_lms_teamForceBalance",	"1",	CVAR_ARCHIVE },
	{ &g_lms_roundlimit,		"g_lms_roundlimit",			"3",	CVAR_ARCHIVE },
	{ &g_lms_matchlimit,		"g_lms_matchlimit",			"2",	CVAR_ARCHIVE },
	{ &g_lms_currentMatch,		"g_lms_currentMatch",		"0",	CVAR_ROM, 0, qfalse, qtrue },
	{ &g_lms_lockTeams,			"g_lms_lockTeams",			"0",	CVAR_ARCHIVE },
	{ &g_lms_followTeamOnly,	"g_lms_followTeamOnly",		"1",	CVAR_ARCHIVE },
	{ &g_axiswins,				"g_axiswins",				"0",	CVAR_ROM, 0, qfalse, qtrue },
	{ &g_alliedwins,			"g_alliedwins",				"0",	CVAR_ROM, 0, qfalse, qtrue },
	{ &g_axismapxp,				"g_axismapxp",				"0",	CVAR_ROM, 0, qfalse, qtrue },
	{ &g_alliedmapxp,			"g_alliedmapxp",			"0",	CVAR_ROM, 0, qfalse, qtrue },

	{ &g_oldCampaign,			"g_oldCampaign",			"",		CVAR_ROM, 0, },
	{ &g_currentCampaign,		"g_currentCampaign",		"",		CVAR_WOLFINFO | CVAR_ROM, 0, },
	{ &g_currentCampaignMap,	"g_currentCampaignMap",		"0",	CVAR_WOLFINFO | CVAR_ROM, 0, },


#ifdef SAVEGAME_SUPPORT
	{ &g_reloading, "g_reloading", "0", CVAR_ROM },
#endif // SAVEGAME_SUPPORT

	// points to the URL for mod information, should not be modified by server admin
	{ &mod_url, "mod_url", "http://www.trickjump.me", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse },
	// configured by the server admin, points to the web pages for the server
	{ &url, "URL", "http://www.trickjump.me", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qfalse },

	{ &g_letterbox, "cg_letterbox", "0", CVAR_TEMP	},
	{ &bot_enable,	"bot_enable",	"0", 0			},

	{ &g_debugSkills,	"g_debugSkills", "0", 0		},

	{ &g_heavyWeaponRestriction, "g_heavyWeaponRestriction", "100", CVAR_ARCHIVE | CVAR_SERVERINFO },

	{ &g_nextmap, "nextmap", "", CVAR_TEMP },
	{ &g_nextcampaign, "nextcampaign", "", CVAR_TEMP },

	{ &tjg_SaveLoad, "tjg_SaveLoad", "1", CVAR_ARCHIVE },
	{ &tjg_strictSL, "tjg_strictSL", "0", CVAR_ARCHIVE | CVAR_LATCH },
	{ &tjg_altSL, "tjg_altSL", "0", CVAR_ARCHIVE },

	{ &tjg_Nofatigue, "tjg_Nofatigue", "1", CVAR_ARCHIVE },

	// Dini - wm_endround disabled\enabled
	{ &tjg_EndRound, "tjg_EndRound", "0", CVAR_ARCHIVE },

	//Nolla - Resetmaxspeed
	{ &tjg_ResetSpeed, "tjg_ResetSpeed", "0", CVAR_ARCHIVE },

	{ &tjg_maxWarnCount, "tjg_maxWarnCount", "3", CVAR_ARCHIVE },

	// Dini, extra setting for noclip
	// Allows noclip to work without cheats being enabled!
	{ &tjg_Noclip, "tjg_Noclip", "0", CVAR_ARCHIVE },

	// Dini, extra setting for god
	{ &tjg_God, "tjg_God", "1", CVAR_ARCHIVE },

	// Dini, mapscript support
	{ &tjg_mapScriptDirectory, "tjg_mapScriptDirectory", "tjmapscripts", CVAR_ARCHIVE },

	// Dini, max IP connections
	{ &tjg_maxConnections, "tjg_maxConnections", "2", CVAR_ARCHIVE },

	// Dini, push
	{ &tjg_shove, "tjg_shove", "0", CVAR_ARCHIVE },

	// Dini, Spamprotection ala ETPub
	{ &tjg_floodprotection, "tjg_floodprotection", "1", CVAR_ARCHIVE },
	{ &tjg_floodlimit, "tjg_floodlimit", "24", CVAR_ARCHIVE },
	{ &tjg_floodwait, "tjg_floodwait", "768", CVAR_ARCHIVE },

	// Dini, allow position change
	{ &tjg_posChange, "tjg_posChange", "1", CVAR_ARCHIVE },

	// TJMOD WEAPON RELATED
	// Dini, Disables players taking damage from eachother.
	{ &tjg_damage, "tjg_damage", "0", CVAR_ARCHIVE },

	// Dini, Disable all weapons
	{ &tjg_weapons, "tjg_weapons", "1", CVAR_ARCHIVE },

	// Different logfile timestamps
	{ &tjg_logTime, "tjg_logTime", "1", CVAR_ARCHIVE },
	{ &tjg_dailyLogs, "tjg_dailyLogs", "0", CVAR_ARCHIVE },

	{ &tjg_recordMode, "tjg_recordMode", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_SYSTEMINFO },
	{ &tjg_disabled_ob, "tjg_disabled_ob", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_SYSTEMINFO },

	{ &tjg_recordFile, "tjg_recordFile", "recordtimes.log", CVAR_ARCHIVE },
	{ &tjg_localrecords, "tjg_localrecords", "1", CVAR_ARCHIVE },

	{ &tjg_trigger_savereset, "tjg_trigger_savereset", "1", CVAR_ARCHIVE },
	{ &tjg_mapExplosiveToggle, "tjg_mapExplosiveToggle", "1", CVAR_ARCHIVE },
	{ &tjg_mapKillEntities, "tjg_mapKillEntities", "1", CVAR_ARCHIVE },
	{ &tjg_holdDoorsOpen, "tjg_holdDoorsOpen", "0", CVAR_ARCHIVE },
	{ &tjg_blockedMaps, "tjg_blockedMaps", "", CVAR_ARCHIVE },

	{ &tjg_q3teles, "tjg_q3teles", "0", CVAR_ARCHIVE | CVAR_LATCH },
	{ &tjg_timerwait, "tjg_timerwait", "0", CVAR_ARCHIVE},

	{ &tjg_votewait, "tjg_votewait", "30", CVAR_ARCHIVE},

	{ &tjg_missilepassthrough, "tjg_missilepassthrough", "0", CVAR_ARCHIVE },

	// Stuff set serverside and sent to client. For prediction @ both sides.
	{ &physics, "physics", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_SYSTEMINFO },
	{ &isTimerun, "isTimerun", "0", CVAR_ROM | CVAR_SYSTEMINFO },
	{ &tjg_admin, "tjg_admin", "1", CVAR_ARCHIVE, 0, qfalse },
};

// bk001129 - made static to avoid aliasing
static int gameCvarTableSize = sizeof(gameCvarTable) / sizeof(gameCvarTable[0]);


void G_InitGame(int levelTime, int randomSeed, int restart);

void G_RunFrame(int levelTime);
void G_ShutdownGame(int restart);
void CheckExitRules(void);

qboolean G_SnapshotCallback(int entityNum, int clientNum)
{
	gentity_t *ent = &g_entities[ entityNum ];

	if (ent->s.eType == ET_MISSILE)
	{
		if (ent->s.weapon == WP_LANDMINE)
		{
			return G_LandmineSnapshotCallback(entityNum, clientNum);
		}
	}

	return qtrue;
}

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
#if defined(__MACOS__)
#ifndef __GNUC__
#pragma export on
#endif
#endif
int vmMain(int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6)
{
#if defined(__MACOS__)
#ifndef __GNUC__
#pragma export off
#endif
#endif
	switch (command)
	{
		case GAME_INIT:
			G_InitGame(arg0, arg1, arg2);
			return 0;
		case GAME_SHUTDOWN:
			G_ShutdownGame(arg0);
			return 0;
		case GAME_CLIENT_CONNECT:
			return (int)ClientConnect(arg0, arg1, arg2);
		case GAME_CLIENT_THINK:
			ClientThink(arg0);
			return 0;
		case GAME_CLIENT_USERINFO_CHANGED:
			ClientUserinfoChanged(arg0);
			return 0;
		case GAME_CLIENT_DISCONNECT:
			ClientDisconnect(arg0);
			return 0;
		case GAME_CLIENT_BEGIN:
			ClientBegin(arg0);
			return 0;
		case GAME_CLIENT_COMMAND:
			ClientCommand(arg0);
			return 0;
		case GAME_RUN_FRAME:
			G_RunFrame(arg0);
			return 0;
		case GAME_CONSOLE_COMMAND:
			return ConsoleCommand();
		case GAME_SNAPSHOT_CALLBACK:
			return G_SnapshotCallback(arg0, arg1);
		case GAME_MESSAGERECEIVED:
			return -1;
	}

	return -1;
}

void QDECL G_Printf(const char *fmt, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	trap_Printf(text);
}
//bani
void QDECL G_Printf(const char *fmt, ...)_attribute((format(printf, 1, 2)));

void QDECL G_DPrintf(const char *fmt, ...)
{
	va_list		argptr;
	char		text[1024];

	if (!g_developer.integer)
		return;

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	trap_Printf(text);
}
//bani
void QDECL G_DPrintf(const char *fmt, ...)_attribute((format(printf, 1, 2)));

void QDECL G_Error(const char *fmt, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	trap_Error(text);
}
//bani
void QDECL G_Error(const char *fmt, ...)_attribute((format(printf, 1, 2)));


#define CH_KNIFE_DIST		48	// from g_weapon.c
#define CH_LADDER_DIST		100
#define CH_WATER_DIST		100
#define CH_BREAKABLE_DIST	64
#define CH_DOOR_DIST		96
#define CH_ACTIVATE_DIST	96
#define CH_EXIT_DIST		256
#define CH_FRIENDLY_DIST	1024

#define CH_MAX_DIST			1024	// use the largest value from above
#define CH_MAX_DIST_ZOOM	8192	// max dist for zooming hints

/*
==============
G_CursorHintIgnoreEnt: returns whether the ent should be ignored
for cursor hint purpose (because the ent may have the designed content type
but nevertheless should not display any cursor hint)
==============
*/
static qboolean G_CursorHintIgnoreEnt(gentity_t *traceEnt, gentity_t *clientEnt)
{
	return (traceEnt->s.eType == ET_OID_TRIGGER || traceEnt->s.eType == ET_TRIGGER_MULTIPLE) ? qtrue : qfalse;
}

/*
==============
G_CheckForCursorHints
	non-AI's check for cursor hint contacts

	server-side because there's info we want to show that the client
	just doesn't know about.  (health or other info of an explosive,invisible_users,items,etc.)

	traceEnt is the ent hit by the trace, checkEnt is the ent that is being
	checked against (in case the traceent was an invisible_user or something)

==============
*/

qboolean G_EmplacedGunIsMountable(gentity_t *ent, gentity_t *other)
{
	if (Q_stricmp(ent->classname, "misc_mg42") && Q_stricmp(ent->classname, "misc_aagun"))
	{
		return qfalse;
	}

	if (!other->client)
	{
		return qfalse;
	}

	if (BG_IsScopedWeapon(other->client->ps.weapon))
	{
		return qfalse;
	}

	if (other->client->ps.pm_flags & PMF_DUCKED)
	{
		return qfalse;
	}

	if (other->client->ps.persistant[PERS_HWEAPON_USE])
	{
		return qfalse;
	}

	if (ent->r.currentOrigin[2] - other->r.currentOrigin[2] >= 40)
	{
		return qfalse;
	}

	if (ent->r.currentOrigin[2] - other->r.currentOrigin[2] < 0)
	{
		return qfalse;
	}

	if (ent->s.frame != 0)
	{
		return qfalse;
	}

	if (ent->active)
	{
		return qfalse;
	}

	if (other->client->ps.grenadeTimeLeft)
	{
		return qfalse;
	}

	if (infront(ent, other))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean G_EmplacedGunIsRepairable(gentity_t *ent, gentity_t *other)
{
	if (Q_stricmp(ent->classname, "misc_mg42") && Q_stricmp(ent->classname, "misc_aagun"))
	{
		return qfalse;
	}

	if (!other->client)
	{
		return qfalse;
	}

	if (BG_IsScopedWeapon(other->client->ps.weapon))
	{
		return qfalse;
	}

	if (other->client->ps.persistant[PERS_HWEAPON_USE])
	{
		return qfalse;
	}

	if (ent->s.frame == 0)
	{
		return qfalse;
	}

	return qtrue;
}

// setup /begin - moved from botai, maybe move to another file ?
// Gordon: adding some support functions
// returns qtrue if a construction is under way on this ent, even before it hits any stages
qboolean G_ConstructionBegun(gentity_t *ent)
{
	if (G_ConstructionIsPartlyBuilt(ent))
	{
		return qtrue;
	}

	if (ent->s.angles2[0])
	{
		return qtrue;
	}

	return qfalse;
}

// returns qtrue if all stage are built
qboolean G_ConstructionIsFullyBuilt(gentity_t *ent)
{
	if (ent->s.angles2[1] != 1)
	{
		return qfalse;
	}
	return qtrue;
}

// returns qtrue if 1 stage or more is built
qboolean G_ConstructionIsPartlyBuilt(gentity_t *ent)
{
	if (G_ConstructionIsFullyBuilt(ent))
	{
		return qtrue;
	}

	if (ent->count2)
	{
		if (!ent->grenadeFired)
		{
			return qfalse;
		}
		else
		{
			return qtrue;
		}
	}

	return qfalse;
}

// returns the constructible for this team that is attached to this toi
gentity_t *G_ConstructionForTeam(gentity_t *toi, team_t team)
{
	gentity_t *targ = toi->target_ent;
	if (!targ || targ->s.eType != ET_CONSTRUCTIBLE)
	{
		return NULL;
	}

	if (targ->spawnflags & 4)
	{
		if (team == TEAM_ALLIES)
		{
			return targ->chain;
		}
	}
	else if (targ->spawnflags & 8)
	{
		if (team == TEAM_AXIS)
		{
			return targ->chain;
		}
	}

	return targ;
}

gentity_t *G_IsConstructible(team_t team, gentity_t *toi)
{
	gentity_t *ent;

	if (!toi || toi->s.eType != ET_OID_TRIGGER)
	{
		return NULL;
	}

	if (!(ent = G_ConstructionForTeam(toi, team)))
	{
		return NULL;
	}

	if (G_ConstructionIsFullyBuilt(ent))
	{
		return NULL;
	}

	if (ent->chain && G_ConstructionBegun(ent->chain))
	{
		return NULL;
	}

	return ent;
}
// setup /end

void G_CheckForCursorHints(gentity_t *ent)
{
	vec3_t		forward, right, up, offset, end;
	trace_t		*tr;
	float		dist;
	float		chMaxDist = CH_MAX_DIST;
	gentity_t	*checkEnt, *traceEnt = 0;
	playerState_t *ps;
	int			hintType, hintDist, hintVal;
	qboolean	zooming, indirectHit;	// indirectHit means the checkent was not the ent hit by the trace (checkEnt!=traceEnt)
	int			trace_contents;			// DHM - Nerve
	int			numOfIgnoredEnts = 0;

	if (!ent->client)
	{
		return;
	}

	ps = &ent->client->ps;

#ifdef SAVEGAME_SUPPORT
	// don't change anything if reloading.  just set the exit hint
	if ((g_gametype.integer == GT_SINGLE_PLAYER || g_gametype.integer == GT_COOP) && g_reloading.integer == RELOAD_NEXTMAP_WAITING)
	{
		ps->serverCursorHint = HINT_EXIT;
		ps->serverCursorHintVal = 0;
		return;
	}
#endif // SAVEGAME_SUPPORT

	indirectHit = qfalse;

	zooming = (qboolean)(ps->eFlags & EF_ZOOMING);

	AngleVectors(ps->viewangles, forward, right, up);

	VectorCopy(ps->origin, offset);
	offset[2] += ps->viewheight;

	// lean
	if (ps->leanf)
	{
		VectorMA(offset, ps->leanf, right, offset);
	}

	if (zooming)
	{
		VectorMA(offset, CH_MAX_DIST_ZOOM, forward, end);
	}
	else
	{
		VectorMA(offset, chMaxDist, forward, end);
	}

	tr = &ps->serverCursorHintTrace;

	trace_contents = (CONTENTS_TRIGGER | CONTENTS_SOLID | CONTENTS_MISSILECLIP | CONTENTS_BODY | CONTENTS_CORPSE);
	trap_Trace(tr, offset, NULL, NULL, end, ps->clientNum, trace_contents);

	// reset all
	hintType	= ps->serverCursorHint		= HINT_NONE;
	hintVal		= ps->serverCursorHintVal	= 0;

	dist = VectorDistanceSquared(offset, tr->endpos);

	if (zooming)
	{
		hintDist	= CH_MAX_DIST_ZOOM;
	}
	else
	{
		hintDist	= chMaxDist;
	}

	// Arnout: building something - add this here because we don't have anything solid to trace to - quite ugly-ish
	if (ent->client->touchingTOI && ps->stats[ STAT_PLAYER_CLASS ] == PC_ENGINEER)
	{
		gentity_t *constructible;
		if ((constructible = G_IsConstructible(ent->client->sess.sessionTeam, ent->client->touchingTOI)))
		{
			ps->serverCursorHint = HINT_CONSTRUCTIBLE;
			ps->serverCursorHintVal = (int)constructible->s.angles2[0];
			return;
		}
	}

	if (ps->stats[ STAT_PLAYER_CLASS ] == PC_COVERTOPS)
	{
		if (ent->client->landmineSpottedTime && level.time - ent->client->landmineSpottedTime < 500)
		{
			ps->serverCursorHint = HINT_LANDMINE;
			ps->serverCursorHintVal	= ent->client->landmineSpotted ? ent->client->landmineSpotted->count2 : 0;
			return;
		}
	}

	if (tr->fraction == 1)
	{
		return;
	}

	traceEnt = &g_entities[tr->entityNum];
	while (G_CursorHintIgnoreEnt(traceEnt, ent) && numOfIgnoredEnts < 10)
	{
		// xkan, 1/9/2003 - we may hit multiple invalid ents at the same point
		// count them to prevent too many loops
		numOfIgnoredEnts++;

		// xkan, 1/8/2003 - advance offset (start point) past the entity to ignore
		VectorMA(tr->endpos, 0.1, forward, offset);

		trap_Trace(tr, offset, NULL, NULL, end, traceEnt->s.number, trace_contents);

		// xkan, 1/8/2003 - (hintDist - dist) is the actual distance in the above
		// trap_Trace call. update dist accordingly.
		dist += VectorDistanceSquared(offset, tr->endpos);
		if (tr->fraction == 1)
		{
			return;
		}
		traceEnt = &g_entities[tr->entityNum];
	}

	if (tr->entityNum == ENTITYNUM_WORLD)
	{
		if ((tr->contents & CONTENTS_WATER) && !(ps->powerups[PW_BREATHER]))
		{
			hintDist = CH_WATER_DIST;
			hintType = HINT_WATER;
		}
		else if ((tr->surfaceFlags & SURF_LADDER) && !(ps->pm_flags & PMF_LADDER))      // ladder
		{
			hintDist = CH_LADDER_DIST;
			hintType = HINT_LADDER;
		}
	}
	else if (tr->entityNum < MAX_CLIENTS)
	{
		// Show medics a syringe if they can revive someone

		if (traceEnt->client && traceEnt->client->sess.sessionTeam == ent->client->sess.sessionTeam)
		{
			if (ps->stats[ STAT_PLAYER_CLASS ] == PC_MEDIC && traceEnt->client->ps.pm_type == PM_DEAD && !(traceEnt->client->ps.pm_flags & PMF_LIMBO))
			{
				hintDist	= 48; // JPW NERVE matches weapon_syringe in g_weapon.c
				hintType	= HINT_REVIVE;
			}
		}
		else if (traceEnt->client && traceEnt->client->isCivilian)
		{
			// xkan, 1/6/2003 - check for civilian, show neutral cursor (no matter which team)
			hintType = HINT_PLYR_NEUTRAL;
			hintDist = CH_FRIENDLY_DIST;	// far, since this will be used to determine whether to shoot bullet weaps or not
		}
	}
	else
	{
		checkEnt = traceEnt;

		// Arnout: invisible entities don't show hints
		if (traceEnt->entstate == STATE_INVISIBLE || traceEnt->entstate == STATE_UNDERCONSTRUCTION)
		{
			return;
		}

		// check invisible_users first since you don't want to draw a hint based
		// on that ent, but rather on what they are targeting.
		// so find the target and set checkEnt to that to show the proper hint.
		if (traceEnt->s.eType == ET_GENERAL)
		{

			// ignore trigger_aidoor.  can't just not trace for triggers, since I need invisible_users...
			// damn, I would like to ignore some of these triggers though.

			if (!Q_stricmp(traceEnt->classname, "trigger_aidoor"))
			{
				return;
			}

			if (!Q_stricmp(traceEnt->classname, "func_invisible_user"))
			{
				indirectHit = qtrue;

				// DHM - Nerve :: Put this back in only in multiplayer
				if (traceEnt->s.dmgFlags)  	// hint icon specified in entity
				{
					hintType = traceEnt->s.dmgFlags;
					hintDist = CH_ACTIVATE_DIST;
					checkEnt = 0;
				}
				else     // use target for hint icon
				{
					checkEnt = G_FindByTargetname(NULL, traceEnt->target);
					if (!checkEnt)  		// no target found
					{
						hintType = HINT_BAD_USER;
						hintDist = CH_MAX_DIST_ZOOM;	// show this one from super far for debugging
					}
				}
			}
		}


		if (checkEnt)
		{

			// TDF This entire function could be the poster boy for converting to OO programming!!!
			// I'm making this into a switch in a vain attempt to make this readable so I can find which
			// brackets don't match!!!

			switch (checkEnt->s.eType)
			{
				case ET_CORPSE:
					if (!ent->client->ps.powerups[PW_BLUEFLAG] && !ent->client->ps.powerups[PW_REDFLAG] && !ent->client->ps.powerups[PW_OPS_DISGUISED])
					{
						if (BODY_TEAM(traceEnt) < 4 && BODY_TEAM(traceEnt) != ent->client->sess.sessionTeam && traceEnt->nextthink == traceEnt->timestamp + BODY_TIME(BODY_TEAM(traceEnt)))
						{
							if (ent->client->ps.stats[STAT_PLAYER_CLASS] == PC_COVERTOPS)
							{
								hintDist	= 48;
								hintType	= HINT_UNIFORM;
								hintVal		= BODY_VALUE(traceEnt);
								if (hintVal > 255)
								{
									hintVal = 255;
								}
							}
						}
					}
					break;
				case ET_GENERAL:
				case ET_MG42_BARREL:
				case ET_AAGUN:
					hintType = HINT_FORCENONE;

					if (G_EmplacedGunIsMountable(traceEnt, ent))
					{
						hintDist = CH_ACTIVATE_DIST;
						hintType = HINT_MG42;
						hintVal = 0;
					}
					else
					{
						if (ps->stats[ STAT_PLAYER_CLASS ] == PC_ENGINEER && G_EmplacedGunIsRepairable(traceEnt, ent))
						{
							hintType = HINT_BUILD;
							hintDist = CH_BREAKABLE_DIST;
							hintVal = traceEnt->health;
							if (hintVal > 255)
							{
								hintVal = 255;
							}
						}
						else
						{
							hintDist = 0;
							hintType = ps->serverCursorHint = HINT_FORCENONE;
							hintVal = ps->serverCursorHintVal = 0;
						}
					}
					break;
				case ET_EXPLOSIVE:
				{
					if (checkEnt->spawnflags & EXPLOSIVE_TANK)
					{
						hintDist = CH_BREAKABLE_DIST * 2;
						hintType = HINT_TANK;
						hintVal	 = ps->serverCursorHintVal	= 0;	// no health for tank destructibles
					}
					else
					{
						switch (checkEnt->constructibleStats.weaponclass)
						{
							case 0:
								hintDist = CH_BREAKABLE_DIST;
								hintType = HINT_BREAKABLE;
								hintVal  = checkEnt->health;		// also send health to client for visualization
								break;
							case 1:
								hintDist = CH_BREAKABLE_DIST * 2;
								hintType = HINT_SATCHELCHARGE;
								hintVal	 = ps->serverCursorHintVal = 0;	// no health for satchel charges
								break;
							case 2:
								hintDist = 0;
								hintType = ps->serverCursorHint = HINT_FORCENONE;
								hintVal = ps->serverCursorHintVal = 0;

								if (checkEnt->parent && checkEnt->parent->s.eType == ET_OID_TRIGGER)
								{
									if (((ent->client->sess.sessionTeam == TEAM_AXIS) && (checkEnt->parent->spawnflags & ALLIED_OBJECTIVE)) ||
											((ent->client->sess.sessionTeam == TEAM_ALLIES) && (checkEnt->parent->spawnflags & AXIS_OBJECTIVE)))
									{
										hintDist = CH_BREAKABLE_DIST * 2;
										hintType = HINT_BREAKABLE_DYNAMITE;
										hintVal	 = ps->serverCursorHintVal	= 0;	// no health for dynamite
									}
								}
								break;
							default:
								if (checkEnt->health > 0)
								{
									hintDist = CH_BREAKABLE_DIST;
									hintType = HINT_BREAKABLE;
									hintVal  = checkEnt->health;		// also send health to client for visualization
								}
								else
								{
									hintDist = 0;
									hintType = ps->serverCursorHint		= HINT_FORCENONE;
									hintVal	 = ps->serverCursorHintVal	= 0;
								}
								break;
						}
					}

					break;
				}
				case ET_CONSTRUCTIBLE:
					if (G_ConstructionIsPartlyBuilt(checkEnt) && !(checkEnt->spawnflags & CONSTRUCTIBLE_INVULNERABLE))
					{
						// only show hint for players who can blow it up
						if (checkEnt->s.teamNum != ent->client->sess.sessionTeam)
						{
							switch (checkEnt->constructibleStats.weaponclass)
							{
								case 0:
									hintDist = CH_BREAKABLE_DIST;
									hintType = HINT_BREAKABLE;
									hintVal  = checkEnt->health;		// also send health to client for visualization
									break;
								case 1:
									hintDist = CH_BREAKABLE_DIST * 2;
									hintType = HINT_SATCHELCHARGE;
									hintVal	 = ps->serverCursorHintVal	= 0;	// no health for satchel charges
									break;
								case 2:
									hintDist = CH_BREAKABLE_DIST * 2;
									hintType = HINT_BREAKABLE_DYNAMITE;
									hintVal	 = ps->serverCursorHintVal	= 0;	// no health for dynamite
									break;
								default:
									hintDist = 0;
									hintType = ps->serverCursorHint		= HINT_FORCENONE;
									hintVal	 = ps->serverCursorHintVal	= 0;
									break;
							}
						}
						else
						{
							hintDist = 0;
							hintType = ps->serverCursorHint		= HINT_FORCENONE;
							hintVal	 = ps->serverCursorHintVal	= 0;
							return;
						}
					}

					break;
				case ET_ALARMBOX:
					if (checkEnt->health > 0)
					{
						hintType = HINT_ACTIVATE;
					}
					break;

				case ET_ITEM:
				{
					gitem_t *it = &bg_itemlist[checkEnt->item - bg_itemlist];

					hintDist = CH_ACTIVATE_DIST;

					switch (it->giType)
					{
						case IT_HEALTH:
							hintType = HINT_HEALTH;
							break;
						case IT_TREASURE:
							hintType = HINT_TREASURE;
							break;
						case IT_WEAPON:
						{
							qboolean canPickup = COM_BitCheck(ent->client->ps.weapons, it->giTag);

							if (!canPickup)
							{
								if (it->giTag == WP_AMMO)
								{
									canPickup = qtrue;
								}
							}

							if (!canPickup)
							{
								canPickup = G_CanPickupWeapon(it->giTag, ent);
							}

							if (canPickup)
							{
								hintType = HINT_WEAPON;
							}
							break;
						}
						case IT_AMMO:
							hintType = HINT_AMMO;
							break;
						case IT_ARMOR:
							hintType = HINT_ARMOR;
							break;
						case IT_HOLDABLE:
							hintType = HINT_HOLDABLE;
							break;
						case IT_KEY:
							hintType = HINT_INVENTORY;
							break;
						case IT_TEAM:
							if (!Q_stricmp(traceEnt->classname, "team_CTF_redflag") && ent->client->sess.sessionTeam == TEAM_ALLIES)
								hintType = HINT_POWERUP;
							else if (!Q_stricmp(traceEnt->classname, "team_CTF_blueflag") && ent->client->sess.sessionTeam == TEAM_AXIS)
								hintType = HINT_POWERUP;
							break;
						case IT_BAD:
						default:
							break;
					}

					break;
				}
				case ET_MOVER:
					if (!Q_stricmp(checkEnt->classname, "script_mover"))
					{
						if (G_TankIsMountable(checkEnt, ent))
						{
							hintDist = CH_ACTIVATE_DIST;
							hintType = HINT_ACTIVATE;
						}
					}
					else if (!Q_stricmp(checkEnt->classname, "func_door_rotating"))
					{
						if (checkEnt->moverState == MOVER_POS1ROTATE)    // stationary/closed
						{
							hintDist = CH_DOOR_DIST;
							hintType = HINT_DOOR_ROTATING;
							if (!G_AllowTeamsAllowed(checkEnt, ent))      // locked
							{
								hintType = HINT_DOOR_ROTATING_LOCKED;
							}
						}
					}
					else if (!Q_stricmp(checkEnt->classname, "func_door"))
					{
						if (checkEnt->moverState == MOVER_POS1)  // stationary/closed
						{
							hintDist = CH_DOOR_DIST;
							hintType = HINT_DOOR;

							if (!G_AllowTeamsAllowed(checkEnt, ent))      // locked
							{
								hintType = HINT_DOOR_LOCKED;
							}
						}
					}
					else if (!Q_stricmp(checkEnt->classname, "func_button"))
					{
						hintDist = CH_ACTIVATE_DIST;
						hintType = HINT_BUTTON;
					}
					else if (!Q_stricmp(checkEnt->classname, "props_flamebarrel"))
					{
						hintDist = CH_BREAKABLE_DIST * 2;
						hintType = HINT_BREAKABLE;
					}
					else if (!Q_stricmp(checkEnt->classname, "props_statue"))
					{
						hintDist = CH_BREAKABLE_DIST * 2;
						hintType = HINT_BREAKABLE;
					}

					break;
				case ET_MISSILE:
				case ET_BOMB:
					if (ps->stats[ STAT_PLAYER_CLASS ] == PC_ENGINEER)
					{
						hintDist	= CH_BREAKABLE_DIST;
						hintType	= HINT_DISARM;
						hintVal		= checkEnt->health;		// also send health to client for visualization
						if (hintVal > 255)
							hintVal = 255;
					}


					// hint icon specified in entity (and proper contact was made, so hintType was set)
					// first try the checkent...
					if (checkEnt->s.dmgFlags && hintType)
					{
						hintType = checkEnt->s.dmgFlags;
					}

					// then the traceent
					if (traceEnt->s.dmgFlags && hintType)
					{
						hintType = traceEnt->s.dmgFlags;
					}

					break;
				default:
					break;
			}

			if (zooming)
			{
				hintDist = CH_MAX_DIST_ZOOM;

				// zooming can eat a lot of potential hints
				switch (hintType)
				{

						// allow while zooming
					case HINT_PLAYER:
					case HINT_TREASURE:
					case HINT_LADDER:
					case HINT_EXIT:
					case HINT_NOEXIT:
					case HINT_PLYR_FRIEND:
					case HINT_PLYR_NEUTRAL:
					case HINT_PLYR_ENEMY:
					case HINT_PLYR_UNKNOWN:
						break;

					default:
						return;
				}
			}
		}
	}

	if (dist <= Square(hintDist))
	{
		ps->serverCursorHint = hintType;
		ps->serverCursorHintVal = hintVal;
	}

}

void G_SetTargetName(gentity_t *ent, char *targetname)
{
	if (targetname && *targetname)
	{
		ent->targetname = targetname;
		ent->targetnamehash = BG_StringHashValue(targetname);
	}
	else
	{
		ent->targetnamehash = -1;
	}
}

/*
================
G_FindTeams

Chain together all entities with a matching team field.
Entity teams are used for item groups and multi-entity mover groups.

All but the first will have the FL_TEAMSLAVE flag set and teammaster field set
All but the last will have the teamchain field set to the next one
================
*/
void G_FindTeams(void)
{
	gentity_t	*e, *e2;
	int		i, j;
	int		c, c2;

	c = 0;
	c2 = 0;
	for (i = 1, e = g_entities + i ; i < level.num_entities ; i++, e++)
	{
		if (!e->inuse)
			continue;

		if (!e->team)
			continue;

		if (e->flags & FL_TEAMSLAVE)
			continue;

		if (!Q_stricmp(e->classname, "func_tramcar"))
		{
			if (e->spawnflags & 8) // leader
			{
				e->teammaster = e;
			}
			else
			{
				continue;
			}
		}

		c++;
		c2++;
		for (j = i + 1, e2 = e + 1 ; j < level.num_entities ; j++, e2++)
		{
			if (!e2->inuse)
				continue;
			if (!e2->team)
				continue;
			if (e2->flags & FL_TEAMSLAVE)
				continue;
			if (!strcmp(e->team, e2->team))
			{
				c2++;
				e2->teamchain = e->teamchain;
				e->teamchain = e2;
				e2->teammaster = e;
				//				e2->key = e->key;	// (SA) I can't set the key here since the master door hasn't finished spawning yet and therefore has a key of -1
				e2->flags |= FL_TEAMSLAVE;

				if (!Q_stricmp(e2->classname, "func_tramcar"))
				{
					trap_UnlinkEntity(e2);
				}

				// make sure that targets only point at the master
				if (e2->targetname)
				{
					G_SetTargetName(e, e2->targetname);

					// Rafael
					// note to self: added this because of problems
					// pertaining to keys and double doors
					if (Q_stricmp(e2->classname, "func_door_rotating"))
						e2->targetname = NULL;
				}
			}
		}
	}

	G_Printf("%i teams with %i entities\n", c, c2);
}

/*
=================
G_RegisterCvars
=================
*/
void G_RegisterCvars(void)
{
	int i;
	cvarTable_t	*cv;

	level.server_settings = 0;

	for (i = 0, cv = gameCvarTable; i < gameCvarTableSize; i++, cv++)
	{
		trap_Cvar_Register(cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags);
		if (cv->vmCvar)
		{
			cv->modificationCount = cv->vmCvar->modificationCount;
			// OSP - Update vote info for clients, if necessary
			if (!G_IsSinglePlayerGame())
			{
				G_checkServerToggle(cv->vmCvar);
			}
		}
	}

	// check some things
	// DHM - Gametype is currently restricted to supported types only
	if ((g_gametype.integer < GT_WOLF || g_gametype.integer >= GT_MAX_GAME_TYPE))
	{
		G_Printf("g_gametype %i is out of range, defaulting to GT_WOLF(%i)\n", g_gametype.integer, GT_WOLF);
		trap_Cvar_Set("g_gametype", va("%i", GT_WOLF));
		trap_Cvar_Update(&g_gametype);
	}

	// OSP
	if (!G_IsSinglePlayerGame())
	{
		trap_SetConfigstring(CS_SERVERTOGGLES, va("%d", level.server_settings));
		if (match_readypercent.integer < 1) trap_Cvar_Set("match_readypercent", "1");
	}

	if (pmove_msec.integer < 8)
	{
		trap_Cvar_Set("pmove_msec", "8");
	}
	else if (pmove_msec.integer > 33)
	{
		trap_Cvar_Set("pmove_msec", "33");
	}

}

/*
=================
G_UpdateCvars
=================
*/
void G_UpdateCvars(void)
{
	int i;
	cvarTable_t	*cv;
	qboolean fToggles = qfalse;
	qboolean fVoteFlags = qfalse;
	qboolean chargetimechanged = qfalse;

	for (i = 0, cv = gameCvarTable ; i < gameCvarTableSize ; i++, cv++)
	{
		if (cv->vmCvar)
		{
			trap_Cvar_Update(cv->vmCvar);

			if (cv->modificationCount != cv->vmCvar->modificationCount)
			{
				cv->modificationCount = cv->vmCvar->modificationCount;

				if (cv->trackChange && !(cv->cvarFlags & CVAR_LATCH))
				{
					trap_SendServerCommand(-1, va("print \"Server:[lof] %s [lon]changed to[lof] %s\n\"", cv->cvarName, cv->vmCvar->string));
				}

				if (cv->vmCvar == &tjg_strictSL)
				{
					trap_SendServerCommand(-1, va("print \"Server: tjg_strictSL changed to %s\n\"", cv->vmCvar->string));
				}

				if (cv->vmCvar == &g_filtercams)
				{
					trap_SetConfigstring(CS_FILTERCAMS, va("%i", g_filtercams.integer));
				}

				if (cv->vmCvar == &g_soldierChargeTime)
				{
					level.soldierChargeTime[0] = g_soldierChargeTime.integer * level.soldierChargeTimeModifier[0];
					level.soldierChargeTime[1] = g_soldierChargeTime.integer * level.soldierChargeTimeModifier[1];
					chargetimechanged = qtrue;
				}
				else if (cv->vmCvar == &g_medicChargeTime)
				{
					level.medicChargeTime[0] = g_medicChargeTime.integer * level.medicChargeTimeModifier[0];
					level.medicChargeTime[1] = g_medicChargeTime.integer * level.medicChargeTimeModifier[1];
					chargetimechanged = qtrue;
				}
				else if (cv->vmCvar == &g_engineerChargeTime)
				{
					level.engineerChargeTime[0] = g_engineerChargeTime.integer * level.engineerChargeTimeModifier[0];
					level.engineerChargeTime[1] = g_engineerChargeTime.integer * level.engineerChargeTimeModifier[1];
					chargetimechanged = qtrue;
				}
				else if (cv->vmCvar == &g_LTChargeTime)
				{
					level.lieutenantChargeTime[0] = g_LTChargeTime.integer * level.lieutenantChargeTimeModifier[0];
					level.lieutenantChargeTime[1] = g_LTChargeTime.integer * level.lieutenantChargeTimeModifier[1];
					chargetimechanged = qtrue;
				}
				else if (cv->vmCvar == &g_covertopsChargeTime)
				{
					level.covertopsChargeTime[0] = g_covertopsChargeTime.integer * level.covertopsChargeTimeModifier[0];
					level.covertopsChargeTime[1] = g_covertopsChargeTime.integer * level.covertopsChargeTimeModifier[1];
					chargetimechanged = qtrue;
				}
				else if (cv->vmCvar == &match_readypercent)
				{
					if (match_readypercent.integer < 1) trap_Cvar_Set(cv->cvarName, "1");
					else if (match_readypercent.integer > 100) trap_Cvar_Set(cv->cvarName, "100");
				}
				// Moved this check out of the main world think loop
				else if (cv->vmCvar == &pmove_msec)
				{
					if (pmove_msec.integer < 8)
					{
						trap_Cvar_Set(cv->cvarName, "8");
					}
					else if (pmove_msec.integer > 33)
					{
						trap_Cvar_Set(cv->cvarName, "33");
					}
				}


				// OSP - Update vote info for clients, if necessary
				else if (!G_IsSinglePlayerGame())
				{
					if (cv->vmCvar == &vote_allow_comp			|| cv->vmCvar == &vote_allow_gametype		||
							cv->vmCvar == &vote_allow_kick			|| cv->vmCvar == &vote_allow_map			||
							cv->vmCvar == &vote_allow_matchreset	||
							cv->vmCvar == &vote_allow_mutespecs		|| cv->vmCvar == &vote_allow_nextmap		||
							cv->vmCvar == &vote_allow_pub			|| cv->vmCvar == &vote_allow_referee		||
							cv->vmCvar == &vote_allow_shuffleteamsxp	|| cv->vmCvar == &vote_allow_swapteams		||
							cv->vmCvar == &vote_allow_friendlyfire	|| cv->vmCvar == &vote_allow_timelimit		||
							cv->vmCvar == &vote_allow_antilag		||
							cv->vmCvar == &vote_allow_balancedteams	|| cv->vmCvar == &vote_allow_muting
					   )
					{
						fVoteFlags = qtrue;
					}
					else
					{
						fToggles = (G_checkServerToggle(cv->vmCvar) || fToggles);
					}
				}

			}
		}
	}

	if (fVoteFlags)
	{
		G_voteFlags();
	}

	if (fToggles)
	{
		trap_SetConfigstring(CS_SERVERTOGGLES, va("%d", level.server_settings));
	}

	if (chargetimechanged)
	{
		char cs[MAX_INFO_STRING];
		cs[0] = '\0';
		Info_SetValueForKey(cs, "axs_sld", va("%i", level.soldierChargeTime[0]));
		Info_SetValueForKey(cs, "ald_sld", va("%i", level.soldierChargeTime[1]));
		Info_SetValueForKey(cs, "axs_mdc", va("%i", level.medicChargeTime[0]));
		Info_SetValueForKey(cs, "ald_mdc", va("%i", level.medicChargeTime[1]));
		Info_SetValueForKey(cs, "axs_eng", va("%i", level.engineerChargeTime[0]));
		Info_SetValueForKey(cs, "ald_eng", va("%i", level.engineerChargeTime[1]));
		Info_SetValueForKey(cs, "axs_lnt", va("%i", level.lieutenantChargeTime[0]));
		Info_SetValueForKey(cs, "ald_lnt", va("%i", level.lieutenantChargeTime[1]));
		Info_SetValueForKey(cs, "axs_cvo", va("%i", level.covertopsChargeTime[0]));
		Info_SetValueForKey(cs, "ald_cvo", va("%i", level.covertopsChargeTime[1]));
		trap_SetConfigstring(CS_CHARGETIMES, cs);
	}
}

// Reset particular server variables back to defaults if a config is voted in.
void G_wipeCvars(void)
{
	int			i;
	cvarTable_t	*pCvars;

	for (i = 0, pCvars = gameCvarTable; i < gameCvarTableSize; i++, pCvars++)
	{
		if (pCvars->vmCvar && pCvars->fConfigReset)
		{
			G_Printf("set %s %s\n", pCvars->cvarName, pCvars->defaultString);
			trap_Cvar_Set(pCvars->cvarName, pCvars->defaultString);
		}
	}

	G_UpdateCvars();
}

//bani - #113
#define SNIPSIZE 250

//copies max num chars from beginning of dest into src and returns pointer to new src
char *strcut(char *dest, char *src, int num)
{
	int i;

	if (!dest || !src || !num)
		return NULL;
	for (i = 0 ; i < num ; i++)
	{
		if ((char)*src)
		{
			*dest = *src;
			dest++;
			src++;
		}
		else
		{
			break;
		}
	}
	*dest = (char)0;
	return src;
}

//g_{axies,allies}mapxp overflows and crashes the server
void bani_clearmapxp(void)
{
	trap_SetConfigstring(CS_AXIS_MAPS_XP, "");
	trap_SetConfigstring(CS_ALLIED_MAPS_XP, "");

	trap_Cvar_Set(va("%s_axismapxp0", GAMEVERSION), "");
	trap_Cvar_Set(va("%s_alliedmapxp0", GAMEVERSION), "");
}

void bani_storemapxp(void)
{
	char cs[MAX_STRING_CHARS];
	char u[MAX_STRING_CHARS];
	char *k;
	int i, j;

	//axis
	trap_GetConfigstring(CS_AXIS_MAPS_XP, cs, sizeof(cs));
	for (i = 0; i < SK_NUM_SKILLS; i++)
	{
		Q_strcat(cs, sizeof(cs), va(" %i", (int)level.teamXP[ i ][ 0 ]));
	}
	trap_SetConfigstring(CS_AXIS_MAPS_XP, cs);

	j = 0;
	k = strcut(u, cs, SNIPSIZE);
	while (strlen(u))
	{
		//"to be continued..."
		if (strlen(u) == SNIPSIZE)
		{
			strcat(u, "+");
		}
		trap_Cvar_Set(va("%s_axismapxp%i", GAMEVERSION, j), u);
		j++;
		k = strcut(u, k, SNIPSIZE);
	}

	//allies
	trap_GetConfigstring(CS_ALLIED_MAPS_XP, cs, sizeof(cs));
	for (i = 0; i < SK_NUM_SKILLS; i++)
	{
		Q_strcat(cs, sizeof(cs), va(" %i", (int)level.teamXP[ i ][ 1 ]));
	}
	trap_SetConfigstring(CS_ALLIED_MAPS_XP, cs);

	j = 0;
	k = strcut(u, cs, SNIPSIZE);
	while (strlen(u))
	{
		//"to be continued..."
		if (strlen(u) == SNIPSIZE)
		{
			strcat(u, "+");
		}
		trap_Cvar_Set(va("%s_alliedmapxp%i", GAMEVERSION, j), u);
		j++;
		k = strcut(u, k, SNIPSIZE);
	}
}

void bani_getmapxp(void)
{
	int j;
	char s[MAX_STRING_CHARS];
	char t[MAX_STRING_CHARS];

	j = 0;
	trap_Cvar_VariableStringBuffer(va("%s_axismapxp%i", GAMEVERSION, j), s, sizeof(s));
	//reassemble string...
	while (strrchr(s, '+'))
	{
		j++;
		*strrchr(s, '+') = (char)0;
		trap_Cvar_VariableStringBuffer(va("%s_axismapxp%i", GAMEVERSION, j), t, sizeof(t));
		strcat(s, t);
	}
	trap_SetConfigstring(CS_AXIS_MAPS_XP, s);

	j = 0;
	trap_Cvar_VariableStringBuffer(va("%s_alliedmapxp%i", GAMEVERSION, j), s, sizeof(s));
	//reassemble string...
	while (strrchr(s, '+'))
	{
		j++;
		*strrchr(s, '+') = (char)0;
		trap_Cvar_VariableStringBuffer(va("%s_alliedmapxp%i", GAMEVERSION, j), t, sizeof(t));
		strcat(s, t);
	}
	trap_SetConfigstring(CS_ALLIED_MAPS_XP, s);
}







/*
Admin System, ala shrub.
*/
void G_LoadAdminConfig(void)
{
	fileHandle_t f = 0;
	char		*filename;
	int			len = 0;
	char		*data;
	char		*token;

	int			admin_level;
	char		admin_name[MAX_NETNAME];
	char		admin_pass[MAX_NETNAME];

	int			level_num;
	char		level_name[MAX_NETNAME];
	char		level_cmds[MAX_STRING_CHARS];

	int			i;

	/*
		char			timerunName[MAX_TOKEN_CHARS];
		int				timerunTime;
		char			timerunPlayer[MAX_NETNAME];
	*/
	if (!tjg_admin.integer)
		return;

	// load file content into data buffer
	filename = "admin.cfg";
	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if (len < 0)
	{
		G_Printf("G_LoadAdminConfig: File was empty (%s)\n", filename);
		return;
	}

	G_Printf("\n------------------------------------------------------------\nG_LoadAdminConfig: Starting loading of admin config (%s) ...\n", filename);

	data = G_Alloc(len + 1);
	trap_FS_Read(data, len, f);
	data[len] = '\0';

	trap_FS_FCloseFile(f);

	level.admin.curlev = 0;

	// parse file content
	COM_BeginParseSession("G_LoadAdminConfig");
	while (qtrue)
	{
		token = COM_Parse(&data);

		// bail out
		if (!token[0])
			return;

		if (!strcmp(token, "admin"))
		{
			token = COM_Parse(&data);
			if (strcmp(token, "{"))
			{
				G_Printf("G_LoadAdminConfig: Error (line %d): '{' expected.\n",	COM_GetCurrentParseLine());
				return;
			}

			admin_name[0] = '\0';
			admin_pass[0] = '\0';
			admin_level = -99999;
			while (qtrue)
			{
				token = COM_Parse(&data);
				if (!strcmp(token, "level")) // ADD ADMIN LEVEL
				{
					token = COM_Parse(&data);
					if (!token[0])
					{
						G_Printf("G_LoadAdminConfig: Error (line %d): Admin level excepted, end of script found.\n", COM_GetCurrentParseLine());
						return;
					}
					admin_level = atoi(token);
				}
				else if (!strcmp(token, "name")) // ADD ADMIN NAME
				{
					token = COM_Parse(&data);
					if (!token[0])
					{
						G_Printf("G_LoadAdminConfig: Error (line %d): Admin name excepted, end of script found.\n", COM_GetCurrentParseLine());
						return;
					}
					if (strlen(token) >= sizeof(admin_name))
					{
						G_Printf("G_LoadAdminConfig: Error (line %d): Player name too long.\n",	COM_GetCurrentParseLine());
						return;
					}
					Q_strncpyz(admin_name, token, sizeof(admin_name));
					/*if (sscanf(token, "%d:%d.%d", &mins, &secs, &mil) != 3)
					{
						G_Printf("G_LoadAdminConfig: Error (line %d): Malformed time found.\n",	COM_GetCurrentParseLine());
						return;
					}*/
				}
				else if (!strcmp(token, "pass")) // ADD ADMIN IP ADRESS
				{
					token = COM_Parse(&data);
					if (!token[0])
					{
						G_Printf("G_LoadAdminConfig: Error (line %d): Admin IP excepted, end of config found.\n", COM_GetCurrentParseLine());
						return;
					}
					if (strlen(token) >= sizeof(admin_pass))
					{
						G_Printf("G_LoadAdminConfig: Error (line %d): Admin IP Adress too long.\n",	COM_GetCurrentParseLine());
						return;
					}
					Q_strncpyz(admin_pass, token, sizeof(admin_pass));
				}
				else if (!strcmp(token, "}"))
					break;
				else
				{
					G_Printf("G_LoadAdminConfig: Error (line %d): 'admin_level', 'admin_name', 'admin_pass' or '}' expected.\n", COM_GetCurrentParseLine());
					return;
				}
			}

			// check parsed values
			if (!admin_name[0])
			{
				G_Printf("G_LoadTimerunsRecords, Error (line %d): Admin name missing.\n",	COM_GetCurrentParseLine());
				continue;
			}
			if (admin_level == -99999)
			{
				G_Printf("G_LoadTimerunsRecords, Error (line %d): Admin level missing.\n",	COM_GetCurrentParseLine());
				continue;
			}
			if (!admin_pass[0])
			{
				G_Printf("G_LoadTimerunsRecords, Error (line %d): Admin IP missing.\n",	COM_GetCurrentParseLine());
				continue;
			}

			// Add as an admin
			if (strlen(admin_pass) > 0 && strlen(admin_name) > 0 && admin_level != 0)
			{
				Q_strncpyz(level.admin.name[level.admin.curlev], admin_name, sizeof(level.admin.name[level.admin.curlev]));
				Q_strncpyz(level.admin.pass[level.admin.curlev], admin_pass, sizeof(level.admin.pass[level.admin.curlev]));
				level.admin.level[level.admin.curlev] = admin_level;
				level.admin.curlev++;
			}
		}
		else if (!strcmp(token, "level"))
		{
			token = COM_Parse(&data);
			if (strcmp(token, "{"))
			{
				G_Printf("G_LoadAdminConfig: Error (line %d): '{' expected.\n",	COM_GetCurrentParseLine());
				return;
			}

			level_name[0] = '\0';
			level_cmds[0] = '\0';
			level_num = -99999;
			while (qtrue)
			{
				token = COM_Parse(&data);
				if (!strcmp(token, "level"))
				{
					token = COM_Parse(&data);
					if (!token[0])
					{
						G_Printf("G_LoadAdminConfig: Error (line %d): Level number excepted, end of script found.\n", COM_GetCurrentParseLine());
						return;
					}
					level_num = atoi(token);
				}
				else if (!strcmp(token, "name"))
				{
					token = COM_Parse(&data);
					if (!token[0])
					{
						G_Printf("G_LoadAdminConfig: Error (line %d): Level name excepted, end of script found.\n", COM_GetCurrentParseLine());
						return;
					}
					if (strlen(token) >= sizeof(level_name))
					{
						G_Printf("G_LoadAdminConfig: Error (line %d): Level name too long.\n",	COM_GetCurrentParseLine());
						return;
					}
					Q_strncpyz(level_name, token, sizeof(level_name));
				}
				else if (!strcmp(token, "cmds"))
				{
					token = COM_Parse(&data);
					if (!token[0])
					{
						G_Printf("G_LoadAdminConfig: Error (line %d): Level commands excepted, end of config found.\n", COM_GetCurrentParseLine());
						return;
					}
					if (strlen(token) >= sizeof(level_cmds))
					{
						G_Printf("G_LoadAdminConfig: Error (line %d): Level commands list too long.\n",	COM_GetCurrentParseLine());
						return;
					}
					Q_strncpyz(level_cmds, token, sizeof(level_cmds));
				}
				else if (!strcmp(token, "}"))
					break;
				else
				{
					G_Printf("G_LoadAdminConfig: Error (line %d): 'admin_level', 'admin_name', 'admin_pass' or '}' expected.\n", COM_GetCurrentParseLine());
					return;
				}
			}

			// check parsed values
			if (!level_name[0])
			{
				G_Printf("G_LoadTimerunsRecords, Error (line %d): Level name missing.\n",	COM_GetCurrentParseLine());
				continue;
			}
			if (level_num == -99999)
			{
				G_Printf("G_LoadTimerunsRecords, Error (line %d): Level number missing.\n",	COM_GetCurrentParseLine());
				continue;
			}
			if (!level_cmds[0])
			{
				G_Printf("G_LoadTimerunsRecords, Error (line %d): Level commands missing.\n",	COM_GetCurrentParseLine());
				continue;
			}

			// Add as an admin
			if (strlen(level_name) > 0 && strlen(level_cmds) > 0)
			{
				Q_strncpyz(level.admin.lvl.name[level_num], level_name, sizeof(level.admin.lvl.name[level_num]));
				Q_strncpyz(level.admin.lvl.cmds[level_num], level_cmds, sizeof(level.admin.lvl.cmds[level_num]));
				level.admin.lvl.count++;
			}
		}
		else
		{
			G_Printf("G_LoadAdminConfig: Error (line %d): 'admin' or 'level' expected.\n",	COM_GetCurrentParseLine());
			return;
		}
	}
}

// Basic trim that removes X chars from beginning of string, not char match just removes first char, dont need anything else atm.
char *trim (char *in, int n)
{
	char out[MAX_NAME_LENGTH];
	int i, len;

	if (!in[0])
		return;

	len = strlen(in);

	for (i = 0; i < len - n; i++)
	{
		out[i] = in[n + i];
	}

	// Clears newline or whatnot
	out[len - n] = '\0';

	return out;
}


// Note to self: // lines are ignored before this, aka they're ignored in the fileread function or sth
// 2nd note to self: Maps, pk3s, etc, can't have spaces. Tokens r
// separated on whitespace.. Maybe something should be done eventually..
void LoadCFSConfig (void)
{
	fileHandle_t f = 0;
	char		*filename;
	int			len = 0;
	char		*data;
	char		*token;
	char		*curpak;
	int			paknum;
	int			bspnum;

	// load file content into data buffer
	filename = "cfs.cfg";
	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if (len < 0)
	{
		G_Printf("LoadCFSConfig: File was empty (%s)\n", filename);
		return;
	}


	G_Printf("\n------------------------------------------------------------\nLoadCFSConfig: Starting loading of admin config (%s) ...\n", filename);

	data = G_Alloc(len + 1);
	trap_FS_Read(data, len, f);
	data[len] = '\0';

	trap_FS_FCloseFile(f);

	curpak = '\0';

	// parse file content
	COM_BeginParseSession("LoadCFSConfig");
	while (qtrue)
	{
		int cur_permission;
		token = COM_Parse(&data);

		// EOF
		if (!token[0])
		{
			G_Printf("\nLoadCFSConfig: FINISHED ------------------------------------------------------------\n");
			return;
		}

		// Found a new PK3 declaration
		if (token[0] == ':')
		{
			curpak = trim(token, 1);
			G_Printf("Loading from %s:\n", curpak);

			// Add pk3 to the overall paknames list
			level.cfs.paknames[paknum] = curpak;

			// Add to overall pak count
			level.cfs.total_paks++;

			// Add the pk3 name to the current pak's cfs entry
			level.cfs.pak[paknum].pk3name = curpak;

			// Set to 0 since starting on a new pak
			bspnum = 0;

			paknum++;
			continue;
		}

		// Below, will only run when it doesn't find a pk3 entry, hence it must be a bsp entry.
		// it MUST be in format <permission> <bspname> in the cfsfile
		// Hence, first token will always be the number.. Maybe make this a bit more "safe" sometime..
		cur_permission = atoi(token);

		// Get the next token, which with a valid cfs config will unavoidably be the bspname
		token = COM_Parse(&data);

		// A small safety check, in case the bsp entry isn't there, then there's no point to keep going; there's a prob with cfs file
		if (!token[0])
		{
			G_Printf("Critical CFS error while loading from %s\n", curpak);
			return;
		}

		// Increase the individual pak entry's number of bsps
		level.cfs.pak[paknum].num_bsps++;

		// Also increase the overall bsp count.
		level.cfs.total_bsps++;

		// Set the permissions for the bsp number
		level.cfs.pak[paknum].permissions[bspnum] = cur_permission;

		// Set the bsp name in the pak entry.
		Q_strncpyz(level.cfs.pak[paknum].bsp[bspnum], token, sizeof(level.cfs.pak[paknum].bsp[bspnum]));

		// - At this point, we've got a bsp entry with a permission - all we need for voting etc.
		G_Printf("	%i %s\n", cur_permission, token);

		// Increase bspnum for the parsin of the next bsp, if any..
		bspnum++;
	}
}




/*
============
G_InitGame

============
*/

void G_InitGame(int levelTime, int randomSeed, int restart)
{
	int					i;
	char				cs[MAX_INFO_STRING];
	const char *aMonths[12] =
	{
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	qtime_t ct;
	trap_RealTime(&ct);

	G_Printf("------- Game Initialization -------\n");
	G_Printf("gamename: %s\n", GAMEVERSION);
	G_Printf("gamedate: %s\n", __DATE__);

	srand(randomSeed);

	//bani - make sure pak2.pk3 gets referenced on server so pure checks pass
	trap_FS_FOpenFile("pak2.dat", &i, FS_READ);
	trap_FS_FCloseFile(i);

	G_RegisterCvars();

	// Xian enforcemaxlives stuff
	/*
	we need to clear the list even if enforce maxlives is not active
	in case the g_maxlives was changed, and a map_restart happened
	*/
	ClearMaxLivesBans();

	// just for verbosity
	if (g_enforcemaxlives.integer &&
			(g_maxlives.integer > 0 || g_axismaxlives.integer > 0 || g_alliedmaxlives.integer > 0))
	{
		G_Printf("EnforceMaxLives-Cleared GUID List\n");
	}

	G_ProcessIPBans();

	G_InitMemory();

	// NERVE - SMF - intialize gamestate
	if (g_gamestate.integer == GS_INITIALIZE)
	{
		// OSP
		trap_Cvar_Set("gamestate", va("%i", GS_PLAYING));
	}

	// set some level globals
	i = level.server_settings;
	{
		qboolean oldspawning = level.spawning;
		voteInfo_t votedata;

		memcpy(&votedata, &level.voteInfo, sizeof(voteInfo_t));

		memset(&level, 0, sizeof(level));

		memcpy(&level.voteInfo, &votedata, sizeof(voteInfo_t));

		level.spawning = oldspawning;
	}
	level.time = levelTime;
	level.startTime = levelTime;
	level.server_settings = i;

	for (i = 0; i < level.numConnectedClients; i++)
	{
		level.clients[ level.sortedClients[ i ] ].sess.spawnObjectiveIndex = 0;
	}

	// RF, init the anim scripting
	level.animScriptData.soundIndex = G_SoundIndex;
	level.animScriptData.playSound = G_AnimScriptSound;

	level.soldierChargeTime[0] = level.soldierChargeTime[1] = g_soldierChargeTime.integer;
	level.medicChargeTime[0] = level.medicChargeTime[1] = g_medicChargeTime.integer;
	level.engineerChargeTime[0] = level.engineerChargeTime[1] = g_engineerChargeTime.integer;
	level.lieutenantChargeTime[0] = level.lieutenantChargeTime[1] = g_LTChargeTime.integer;

	level.covertopsChargeTime[0] = level.covertopsChargeTime[1] = g_covertopsChargeTime.integer;

	level.soldierChargeTimeModifier[0] = level.soldierChargeTimeModifier[1] = 1.f;
	level.medicChargeTimeModifier[0] = level.medicChargeTimeModifier[1] = 1.f;
	level.engineerChargeTimeModifier[0] = level.engineerChargeTimeModifier[1] = 1.f;
	level.lieutenantChargeTimeModifier[0] = level.lieutenantChargeTimeModifier[1] = 1.f;
	level.covertopsChargeTimeModifier[0] = level.covertopsChargeTimeModifier[1] = 1.f;

	cs[0] = '\0';
	Info_SetValueForKey(cs, "axs_sld", va("%i", level.soldierChargeTime[0]));
	Info_SetValueForKey(cs, "ald_sld", va("%i", level.soldierChargeTime[1]));
	Info_SetValueForKey(cs, "axs_mdc", va("%i", level.medicChargeTime[0]));
	Info_SetValueForKey(cs, "ald_mdc", va("%i", level.medicChargeTime[1]));
	Info_SetValueForKey(cs, "axs_eng", va("%i", level.engineerChargeTime[0]));
	Info_SetValueForKey(cs, "ald_eng", va("%i", level.engineerChargeTime[1]));
	Info_SetValueForKey(cs, "axs_lnt", va("%i", level.lieutenantChargeTime[0]));
	Info_SetValueForKey(cs, "ald_lnt", va("%i", level.lieutenantChargeTime[1]));
	Info_SetValueForKey(cs, "axs_cvo", va("%i", level.covertopsChargeTime[0]));
	Info_SetValueForKey(cs, "ald_cvo", va("%i", level.covertopsChargeTime[1]));
	trap_SetConfigstring(CS_CHARGETIMES, cs);
	trap_SetConfigstring(CS_FILTERCAMS, va("%i", g_filtercams.integer));

	G_SoundIndex("sound/misc/referee.wav");
	G_SoundIndex("sound/misc/vote.wav");
	G_SoundIndex("sound/player/gurp1.wav");
	G_SoundIndex("sound/player/gurp2.wav");


	if (g_gametype.integer == GT_WOLF)
	{
		//bani - #113
		bani_clearmapxp();
	}

	if (g_gametype.integer == GT_WOLF_STOPWATCH)
	{
		//bani - #113
		bani_clearmapxp();
	}


	trap_GetServerinfo(cs, sizeof(cs));
	Q_strncpyz(level.rawmapname, Info_ValueForKey(cs, "mapname"), sizeof(level.rawmapname));

	G_ParseCampaigns();
	if (g_gametype.integer == GT_WOLF_CAMPAIGN)
	{
		if (g_campaigns[level.currentCampaign].current == 0 || level.newCampaign)
		{
			trap_Cvar_Set("g_axiswins", "0");
			trap_Cvar_Set("g_alliedwins", "0");

			//bani - #113
			bani_clearmapxp();

			trap_Cvar_Update(&g_axiswins);
			trap_Cvar_Update(&g_alliedwins);
		}
		else
		{
			//bani - #113
			bani_getmapxp();
		}
	}

	trap_SetConfigstring(CS_SCRIPT_MOVER_NAMES, "");	// clear out

	G_DebugOpenSkillLog();

	if (tjg_dailyLogs.integer)
		trap_Cvar_Set("g_log", va("%s-%02d-%02d.log", aMonths[ct.tm_mon], ct.tm_mday, 1900 + ct.tm_year));

	if (g_log.string[0])
	{
		if (g_logSync.integer)
		{
			trap_FS_FOpenFile(g_log.string, &level.logFile, FS_APPEND_SYNC);
		}
		else
		{
			trap_FS_FOpenFile(g_log.string, &level.logFile, FS_APPEND);
		}
		if (!level.logFile)
		{
			G_Printf("WARNING: Couldn't open logfile: %s\n", g_log.string);
		}
		else
		{
			G_LogPrintf("------------------------------------------------------------\n");
			G_LogPrintf("InitGame: %s\n", cs);
		}
	}
	else
	{
		G_Printf("Not logging to disk.\n");
	}

	if (tjg_admin.integer)
	{
		G_LoadAdminConfig();
		G_Printf("\n... G_LoadAdminConfig: Finished loading admin config.\n------------------------------------------------------------\n\n");
	}


	LoadCFSConfig();

	if (tjg_recordMode.integer)
	{
		trap_FS_FOpenFile(tjg_recordFile.string, &level.recordFile, FS_APPEND_SYNC);
		if (!level.logFile)
			G_Printf("WARNING: Couldn't open recordfile: %s\n", tjg_recordFile.string);
	}

	G_InitWorldSession();

	// DHM - Nerve :: Clear out spawn target config strings
	trap_GetConfigstring(CS_MULTI_INFO, cs, sizeof(cs));
	Info_SetValueForKey(cs, "numspawntargets", "0");
	trap_SetConfigstring(CS_MULTI_INFO, cs);

	for (i = CS_MULTI_SPAWNTARGETS; i < CS_MULTI_SPAWNTARGETS + MAX_MULTI_SPAWNTARGETS; i++)
	{
		trap_SetConfigstring(i, "");
	}

	G_ResetTeamMapData();

	// initialize all entities for this game
	memset(g_entities, 0, MAX_GENTITIES * sizeof(g_entities[0]));
	level.gentities = g_entities;

	// initialize all clients for this game
	level.maxclients = g_maxclients.integer;
	memset(g_clients, 0, MAX_CLIENTS * sizeof(g_clients[0]));
	level.clients = g_clients;

	// set client fields on player ents
	for (i = 0 ; i < level.maxclients ; i++)
	{
		g_entities[i].client = level.clients + i;
	}

	// always leave room for the max number of clients,
	// even if they aren't all used, so numbers inside that
	// range are NEVER anything but clients
	level.num_entities = MAX_CLIENTS;

	// let the server system know where the entites are
	trap_LocateGameData(level.gentities, level.num_entities, sizeof(gentity_t),
						&level.clients[0].ps, sizeof(level.clients[0]));

	if (g_gametype.integer == GT_SINGLE_PLAYER)
	{
		char loading[4];

		trap_Cvar_VariableStringBuffer("savegame_loading", loading, sizeof(loading));

		if (atoi(loading))
			saveGamePending = 2;
		else
			saveGamePending = 1;
	}
	else
	{
		saveGamePending = 0;
	}

	G_Script_ScriptLoad();	// Load level script, regardless

	// reserve some spots for dead player bodies
	InitBodyQue();

	numSplinePaths = 0 ;
	numPathCorners = 0;

#ifdef USEXPSTORAGE
	G_ClearXPBackup();
	if (g_gametype.integer == GT_WOLF_CAMPAIGN && !level.newCampaign)
	{
		G_ReadXPBackup();
	}
#endif // USEXPSTORAGE

	// START	Mad Doctor I changes, 8/21/2002
	// This needs to be called before G_SpawnEntitiesFromString, or the
	// bot entities get trashed.
	//initialize the bot game entities
	//	BotInitBotGameEntities();
	// END		Mad Doctor I changes, 8/21/2002

	// TAT 11/13/2002
	//		similarly set up the Server entities
	InitServerEntities();

	// parse the key/value pairs and spawn gentities
	G_SpawnEntitiesFromString();

	// TAT 11/13/2002 - entities are spawned, so now we can do setup
	InitialServerEntitySetup();

	// Gordon: debris test
	G_LinkDebris();

	// Gordon: link up damage parents
	G_LinkDamageParents();

	BG_ClearScriptSpeakerPool();

	BG_LoadSpeakerScript(va("sound/maps/%s.sps", level.rawmapname));

	// ===================

	if (!level.gameManager)
	{
		G_Printf("^1ERROR No 'script_multiplayer' found in map\n");
	}

	level.tracemapLoaded = qfalse;
	if (!BG_LoadTraceMap(level.rawmapname, level.mapcoordsMins, level.mapcoordsMaxs))
	{
		G_Printf("^1ERROR No tracemap found for map\n");
	}
	else
	{
		level.tracemapLoaded = qtrue;
	}

	// Link all the splines up
	BG_BuildSplinePaths();

	// create the camera entity that will communicate with the scripts
	//	G_SpawnScriptCamera();

	// general initialization
	G_FindTeams();

	G_Printf("---------------------------------------------\n");

	trap_PbStat(-1 , "INIT" , "GAME") ;

	BG_ClearAnimationPool();

	BG_ClearCharacterPool();

	BG_InitWeaponStrings();

	G_RegisterPlayerClasses();

	// Match init work
	G_loadMatchGame();

	G_LoadTimerunsRecords();

	// Reinstate any MV views for clients -- need to do this after all init is complete
	// --- maybe not the best place to do this... seems to be some race conditions on map_restart
	G_spawnPrintf(DP_MVSPAWN, level.time + 2000, NULL);
	/*

		if (1 == 1)
		{
			CURL *curl;
			CURLcode res;

			G_Printf ("\n\n\nInitcurling..\n", res);
			curl = curl_easy_init();
			if(curl)
			{
				curl_easy_setopt(curl, CURLOPT_URL, "http://trickjump.me");
				res = curl_easy_perform(curl);

				curl_easy_cleanup(curl);
			}
		}
	*/
	// Dini, these values should always be set as below
	G_Printf("\n---------------------------------------------\n");
	trap_Cvar_Set("sv_floodprotect", "0");
	G_Printf(va("%s: setting sv_floodprotect 0\n", GAMEVERSION));
	if (!level.isTimerun && tjg_recordMode.integer)
	{
		trap_Cvar_Set("isTimerun", "0");
		G_Printf(va("%s: No timerun found in map:\n- Disabled recordmode physics limitations.\n", GAMEVERSION));
	}
	else if (level.isTimerun)
		trap_Cvar_Set("isTimerun", "1");
	// Add more informative spam here, + Allprints?
	G_Printf("---------------------------------------------\n");
	G_Printf("CFS Status\nTotal PK3s: %i\nTotal BSPs: %i\nAverage BSP per PK3: %.2f\n", level.cfs.total_paks, level.cfs.total_bsps, (float)level.cfs.total_bsps / level.cfs.total_paks);
	G_Printf("---------------------------------------------\n");
}



/*
=================
G_ShutdownGame
=================
*/
void G_ShutdownGame(int restart)
{

	G_Printf("==== ShutdownGame ====\n");

	if (level.recordFile)
	{
		trap_FS_FCloseFile(level.recordFile);
		level.recordFile = 0;
	}

	if (level.recFile)
	{
		trap_FS_FCloseFile(level.recFile);
		level.recFile = 0;
	}

	G_DebugCloseSkillLog();

	if (level.logFile)
	{
		G_LogPrintf("ShutdownGame:\n");
		G_LogPrintf("------------------------------------------------------------\n");
		trap_FS_FCloseFile(level.logFile);
		level.logFile = 0;
	}

	// write all the client session data so we can get it back
	G_WriteSessionData(restart);
}



//===================================================================

#ifndef GAME_HARD_LINKED
// this is only here so the functions in q_shared.c and bg_*.c can link

void QDECL Com_Error(int level, const char *error, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start(argptr, error);
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	G_Error("%s", text);
}
//bani
void QDECL Com_Error(int level, const char *error, ...)_attribute((format(printf, 2, 3)));

void QDECL Com_Printf(const char *msg, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	G_Printf("%s", text);
}
//bani
void QDECL Com_Printf(const char *msg, ...)_attribute((format(printf, 1, 2)));

#endif

/*
========================================================================

PLAYER COUNTING / SCORE SORTING

========================================================================
*/

/*
=============
SortRanks

=============
*/
int QDECL SortRanks(const void *a, const void *b)
{
	gclient_t	*ca, *cb;
	int i, totalXP[2];

	ca = &level.clients[*(int *)a];
	cb = &level.clients[*(int *)b];

	// sort special clients last
	if (/*ca->sess.spectatorState == SPECTATOR_SCOREBOARD ||*/ ca->sess.spectatorClient < 0)
	{
		return 1;
	}
	if (/*cb->sess.spectatorState == SPECTATOR_SCOREBOARD ||*/ cb->sess.spectatorClient < 0)
	{
		return -1;
	}

	// then connecting clients
	if (ca->pers.connected == CON_CONNECTING)
	{
		return 1;
	}
	if (cb->pers.connected == CON_CONNECTING)
	{
		return -1;
	}


	// then spectators
	if (ca->sess.sessionTeam == TEAM_SPECTATOR && cb->sess.sessionTeam == TEAM_SPECTATOR)
	{
		if (ca->sess.spectatorTime < cb->sess.spectatorTime)
		{
			return -1;
		}
		if (ca->sess.spectatorTime > cb->sess.spectatorTime)
		{
			return 1;
		}
		return 0;
	}
	if (ca->sess.sessionTeam == TEAM_SPECTATOR)
	{
		return 1;
	}
	if (cb->sess.sessionTeam == TEAM_SPECTATOR)
	{
		return -1;
	}

	for (totalXP[0] = totalXP[1] = 0, i = 0; i < SK_NUM_SKILLS; i++)
	{
		totalXP[0] += ca->sess.skillpoints[i];
		totalXP[1] += cb->sess.skillpoints[i];
	}

	// then sort by xp
	if (totalXP[0] > totalXP[1])
	{
		return -1;
	}
	if (totalXP[0] < totalXP[1])
	{
		return 1;
	}

	return 0;
}

//bani - #184
//(relatively) sane replacement for OSP's Players_Axis/Players_Allies
void etpro_PlayerInfo(void)
{
	//128 bits
	char playerinfo[MAX_CLIENTS + 1];
	gentity_t *e;
	team_t playerteam;
	int i;
	int lastclient;

	memset(playerinfo, 0, sizeof(playerinfo));

	lastclient = -1;
	e = &g_entities[0];
	for (i = 0; i < MAX_CLIENTS; i++, e++)
	{
		if (e->client == NULL || e->client->pers.connected == CON_DISCONNECTED)
		{
			playerinfo[i] = '-';
			continue;
		}

		//keep track of highest connected/connecting client
		lastclient = i;

		if (e->inuse == qfalse)
		{
			playerteam = 0;
		}
		else
		{
			playerteam = e->client->sess.sessionTeam;
		}
		playerinfo[i] = (char)'0' + playerteam;
	}
	//terminate the string, if we have any non-0 clients
	if (lastclient != -1)
	{
		playerinfo[lastclient + 1] = (char)0;
	}
	else
	{
		playerinfo[0] = (char)0;
	}

	trap_Cvar_Set("P", playerinfo);
}

/*
============
CalculateRanks

Recalculates the score ranks of all players
This will be called on every client connect, begin, disconnect, death,
and team change.
============
*/
void CalculateRanks(void)
{
	int		i;
	//	int		rank;
	//	int		score;
	//	int		newScore;
	char	teaminfo[TEAM_NUM_TEAMS][256];	// OSP
	gclient_t	*cl;

	level.follow1 = -1;
	level.follow2 = -1;
	level.numConnectedClients = 0;
	level.numNonSpectatorClients = 0;
	level.numPlayingClients = 0;
	level.voteInfo.numVotingClients = 0;		// don't count bots

	level.numFinalDead[0] = 0;		// NERVE - SMF
	level.numFinalDead[1] = 0;		// NERVE - SMF

	level.voteInfo.numVotingTeamClients[ 0 ] = 0;
	level.voteInfo.numVotingTeamClients[ 1 ] = 0;

	for (i = 0; i < TEAM_NUM_TEAMS; i++)
	{
		if (i < 2)
		{
			level.numTeamClients[i] = 0;
		}
		teaminfo[i][0] = 0;			// OSP
	}

	for (i = 0 ; i < level.maxclients ; i++)
	{
		if (level.clients[i].pers.connected != CON_DISCONNECTED)
		{
			int team = level.clients[i].sess.sessionTeam;

			level.sortedClients[level.numConnectedClients] = i;
			level.numConnectedClients++;

			if (team != TEAM_SPECTATOR)
			{
				level.numNonSpectatorClients++;

				// OSP
				Q_strcat(teaminfo[team], sizeof(teaminfo[team]) - 1, va("%d ", level.numConnectedClients));

				// decide if this should be auto-followed
				if (level.clients[i].pers.connected == CON_CONNECTED)
				{
					int teamIndex = level.clients[i].sess.sessionTeam == TEAM_AXIS ? 0 : 1;
					level.numPlayingClients++;
					if (!(g_entities[i].r.svFlags & SVF_BOT))
					{
						level.voteInfo.numVotingClients++;
					}

					if (level.clients[i].sess.sessionTeam == TEAM_AXIS ||
							level.clients[i].sess.sessionTeam == TEAM_ALLIES)
					{

						if (level.clients[i].ps.persistant[PERS_RESPAWNS_LEFT] == 0 && g_entities[i].health <= 0)
						{
							level.numFinalDead[teamIndex]++;
						}

						level.numTeamClients[teamIndex]++;
						if (!(g_entities[i].r.svFlags & SVF_BOT))
						{
							level.voteInfo.numVotingTeamClients[ teamIndex ]++;
						}
					}

					if (level.follow1 == -1)
					{
						level.follow1 = i;
					}
					else if (level.follow2 == -1)
					{
						level.follow2 = i;
					}
				}
			}
		}
	}

	// OSP
	for (i = 0; i < TEAM_NUM_TEAMS; i++)
	{
		if (0 == teaminfo[i][0]) Q_strncpyz(teaminfo[i], "(None)", sizeof(teaminfo[i]));
	}

	qsort(level.sortedClients, level.numConnectedClients,
		  sizeof(level.sortedClients[0]), SortRanks);

	// set the rank value for all clients that are connected and not spectators
	// in team games, rank is just the order of the teams, 0=red, 1=blue, 2=tied
	for (i = 0;  i < level.numConnectedClients; i++)
	{
		cl = &level.clients[ level.sortedClients[i] ];
		if (level.teamScores[TEAM_AXIS] == level.teamScores[TEAM_ALLIES])
		{
			cl->ps.persistant[PERS_RANK] = 2;
		}
		else if (level.teamScores[TEAM_AXIS] > level.teamScores[TEAM_ALLIES])
		{
			cl->ps.persistant[PERS_RANK] = 0;
		}
		else
		{
			cl->ps.persistant[PERS_RANK] = 1;
		}
	}

	// set the CS_SCORES1/2 configstrings, which will be visible to everyone
	//	trap_SetConfigstring( CS_SCORES1, va("%i", level.teamScores[TEAM_AXIS] ) );
	//	trap_SetConfigstring( CS_SCORES2, va("%i", level.teamScores[TEAM_ALLIES] ) );

	trap_SetConfigstring(CS_FIRSTBLOOD, va("%i", level.firstbloodTeam));
	trap_SetConfigstring(CS_ROUNDSCORES1, va("%i", g_axiswins.integer));
	trap_SetConfigstring(CS_ROUNDSCORES2, va("%i", g_alliedwins.integer));

	//bani - #184
	etpro_PlayerInfo();

	// if we are at the intermission, send the new info to everyone
	if (g_gamestate.integer == GS_INTERMISSION)
	{
		SendScoreboardMessageToAllClients();
	}
	else
	{
		// see if it is time to end the level
		CheckExitRules();
	}
}


/*
========================================================================

MAP CHANGING

========================================================================
*/

/*
========================
SendScoreboardMessageToAllClients

Do this at BeginIntermission time and whenever ranks are recalculated
due to enters/exits/forced team changes
========================
*/
void SendScoreboardMessageToAllClients(void)
{
	int		i;

	for (i = 0; i < level.numConnectedClients; i++)
	{
		if (level.clients[level.sortedClients[i]].pers.connected == CON_CONNECTED)
		{
			level.clients[level.sortedClients[i]].wantsscore = qtrue;
			//			G_SendScore(g_entities + level.sortedClients[i]);
		}
	}
}

/*
========================
MoveClientToIntermission

When the intermission starts, this will be called for all players.
If a new client connects, this will be called after the spawn function.
========================
*/
void MoveClientToIntermission(gentity_t *ent)
{
	//	float			timeLived;

	// take out of follow mode if needed
	if (ent->client->sess.spectatorState == SPECTATOR_FOLLOW)
	{
		StopFollowing(ent);
	}

	/*if ( ent->client->sess.sessionTeam == TEAM_AXIS || ent->client->sess.sessionTeam == TEAM_ALLIES ) {
		timeLived = (level.time - ent->client->pers.lastSpawnTime) * 0.001f;

		G_AddExperience( ent, min((timeLived * timeLived) * 0.00005f, 5) );
	}*/

	// move to the spot
	VectorCopy(level.intermission_origin, ent->s.origin);
	VectorCopy(level.intermission_origin, ent->client->ps.origin);
	VectorCopy(level.intermission_angle, ent->client->ps.viewangles);
	ent->client->ps.pm_type = PM_INTERMISSION;

	// clean up powerup info
	// memset( ent->client->ps.powerups, 0, sizeof(ent->client->ps.powerups) );

	ent->client->ps.eFlags = 0;
	ent->s.eFlags = 0;
	ent->s.eType = ET_GENERAL;
	ent->s.modelindex = 0;
	ent->s.loopSound = 0;
	ent->s.event = 0;
	ent->s.events[0] = ent->s.events[1] = ent->s.events[2] = ent->s.events[3] = 0;		// DHM - Nerve
	ent->r.contents = 0;
}

/*
==================
FindIntermissionPoint

This is also used for spectator spawns
==================
*/
void FindIntermissionPoint(void)
{
	gentity_t	*ent = NULL, *target;
	vec3_t		dir;
	char		cs[MAX_STRING_CHARS];		// DHM - Nerve
	char		*buf;						// DHM - Nerve
	int			winner;						// DHM - Nerve

	// NERVE - SMF - if the match hasn't ended yet, and we're just a spectator
	if (!level.intermissiontime)
	{
		// try to find the intermission spawnpoint with no team flags set
		ent = G_Find(NULL, FOFS(classname), "info_player_intermission");

		for (; ent; ent = G_Find(ent, FOFS(classname), "info_player_intermission"))
		{
			if (!ent->spawnflags)
				break;
		}
	}

	trap_GetConfigstring(CS_MULTI_MAPWINNER, cs, sizeof(cs));
	buf = Info_ValueForKey(cs, "winner");
	winner = atoi(buf);

	// Change from scripting value for winner (0==AXIS, 1==ALLIES) to spawnflag value
	if (winner == 0)
	{
		winner = TEAM_AXIS;
	}
	else
	{
		winner = TEAM_ALLIES;
	}


	if (!ent)
	{
		ent = G_Find(NULL, FOFS(classname), "info_player_intermission");
		while (ent)
		{
			if (ent->spawnflags & winner)
			{
				break;
			}

			ent = G_Find(ent, FOFS(classname), "info_player_intermission");
		}
	}

	if (!ent)  	// the map creator forgot to put in an intermission point...
	{
		SelectSpawnPoint(vec3_origin, level.intermission_origin, level.intermission_angle);
	}
	else
	{
		VectorCopy(ent->s.origin, level.intermission_origin);
		VectorCopy(ent->s.angles, level.intermission_angle);
		// if it has a target, look towards it
		if (ent->target)
		{
			target = G_PickTarget(ent->target);
			if (target)
			{
				VectorSubtract(target->s.origin, level.intermission_origin, dir);
				vectoangles(dir, level.intermission_angle);
			}
		}
	}

}

/*
==================
BeginIntermission
==================
*/
void BeginIntermission(void)
{
	int			i;
	gentity_t	*client;

	if (g_gamestate.integer == GS_INTERMISSION)
	{
		return;		// already active
	}

	level.intermissiontime = level.time;
	trap_SetConfigstring(CS_INTERMISSION_START_TIME, va("%i", level.intermissiontime));
	trap_Cvar_Set("gamestate", va("%i", GS_INTERMISSION));
	trap_Cvar_Update(&g_gamestate);

	FindIntermissionPoint();

	// move all clients to the intermission point
	for (i = 0 ; i < level.maxclients ; i++)
	{
		client = g_entities + i;
		if (!client->inuse)
			continue;
		MoveClientToIntermission(client);
	}

	// send the current scoring to all clients
	SendScoreboardMessageToAllClients();

}


/*
=============
ExitLevel

When the intermission has been exited, the server is either killed
or moved to a new level based on the "nextmap" cvar

=============
*/
void ExitLevel(void)
{
	int		i;
	gclient_t *cl;

	if (g_gametype.integer == GT_WOLF_CAMPAIGN)
	{
		g_campaignInfo_t *campaign = &g_campaigns[level.currentCampaign];

		if (campaign->current + 1 < campaign->mapCount)
		{
			trap_Cvar_Set("g_currentCampaignMap", va("%i", campaign->current + 1));
#if 0
			if (g_developer.integer)
				trap_SendConsoleCommand(EXEC_APPEND, va("devmap %s\n", campaign->mapnames[campaign->current + 1]));
			else
#endif
				trap_SendConsoleCommand(EXEC_APPEND, va("map %s\n", campaign->mapnames[campaign->current + 1]));
		}
		else
		{
			char s[MAX_STRING_CHARS];
			trap_Cvar_VariableStringBuffer("nextcampaign", s, sizeof(s));

			if (*s)
			{
				trap_SendConsoleCommand(EXEC_APPEND, "vstr nextcampaign\n");
			}
			else
			{
				// restart the campaign
				trap_Cvar_Set("g_currentCampaignMap", "0");
#if 0
				if (g_developer.integer)
					trap_SendConsoleCommand(EXEC_APPEND, va("devmap %s\n", campaign->mapnames[0]));
				else
#endif
					trap_SendConsoleCommand(EXEC_APPEND, va("map %s\n", campaign->mapnames[0]));
			}

			// FIXME: do we want to do something else here?
			//trap_SendConsoleCommand( EXEC_APPEND, "vstr nextmap\n" );
		}
	}
	else
	{
		trap_SendConsoleCommand(EXEC_APPEND, "vstr nextmap\n");
	}
	level.changemap = NULL;
	level.intermissiontime = 0;

	// reset all the scores so we don't enter the intermission again
	level.teamScores[TEAM_AXIS] = 0;
	level.teamScores[TEAM_ALLIES] = 0;
	if (g_gametype.integer != GT_WOLF_CAMPAIGN)
	{
		for (i = 0 ; i < g_maxclients.integer ; i++)
		{
			cl = level.clients + i;
			if (cl->pers.connected != CON_CONNECTED)
			{
				continue;
			}
			cl->ps.persistant[PERS_SCORE] = 0;
		}
	}

	// we need to do this here before chaning to CON_CONNECTING
	G_WriteSessionData(qfalse);

	// change all client states to connecting, so the early players into the
	// next level will know the others aren't done reconnecting
	for (i = 0 ; i < g_maxclients.integer ; i++)
	{

		if (level.clients[i].pers.connected == CON_CONNECTED)
		{
			level.clients[i].pers.connected = CON_CONNECTING;
			trap_UnlinkEntity(&g_entities[i]);
		}
	}

	G_LogPrintf("ExitLevel: executed\n");
}

/*
=================
G_LogPrintf

Print to the logfile with a time stamp if it is open
=================
*/
void QDECL G_LogPrintf(const char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];
	int			min, tens, sec, l;
	const char *aMonths[12] =
	{
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	qtime_t ct;
	trap_RealTime(&ct);

	sec = level.time / 1000;

	min = sec / 60;
	sec -= min * 60;
	tens = sec / 10;
	sec -= tens * 10;

	if (tjg_logTime.integer == 1)
		Com_sprintf(string, sizeof(string), "[%s%02d-%02d %02d:%02d:%02d] ", aMonths[ct.tm_mon], ct.tm_mday, 1900 + ct.tm_year, ct.tm_hour, ct.tm_min, ct.tm_sec);
	else if (tjg_logTime.integer == 2)
		Com_sprintf(string, sizeof(string), "[%02d.%02d.%02d %02d:%02d:%02d] ", ct.tm_mday, ct.tm_mon, 1900 + ct.tm_year, ct.tm_hour, ct.tm_min, ct.tm_sec);
	else if (tjg_logTime.integer == 3)
		Com_sprintf(string, sizeof(string), "[%02d:%02d:%02d] ", ct.tm_hour, ct.tm_min, ct.tm_sec);
	else
		Com_sprintf(string, sizeof(string), "%i:%i%i ", min, tens, sec);

	l = strlen(string);

	va_start(argptr, fmt);
	Q_vsnprintf(string + l, sizeof(string) - l, fmt, argptr);
	va_end(argptr);

	if (g_dedicated.integer)
	{
		G_Printf("%s", string + l);
	}

	if (!level.logFile)
	{
		return;
	}

	trap_FS_Write(string, strlen(string), level.logFile);
}

void QDECL TJLogPrint(const char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];
	int			l;
	const char *aMonths[12] =
	{
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	qtime_t ct;
	trap_RealTime(&ct);

	Com_sprintf(string, sizeof(string), "[%s%02d-%02d %02d:%02d:%02d] ", aMonths[ct.tm_mon], ct.tm_mday, 1900 + ct.tm_year, ct.tm_hour, ct.tm_min, ct.tm_sec);
	l = strlen(string);

	va_start(argptr, fmt);
	Q_vsnprintf(string + l, sizeof(string) - l, fmt, argptr);
	va_end(argptr);

	if (g_dedicated.integer)
	{
		G_Printf("%s", string + l);
	}

	if (!level.recordFile)
	{
		return;
	}

	trap_FS_Write(string, strlen(string), level.recordFile);
}


//bani
void QDECL G_LogPrintf(const char *fmt, ...)_attribute((format(printf, 1, 2)));

/*
================
LogExit

Append information about this game to the log file
================
*/
void LogExit(const char *string)
{
	int				i;
	gclient_t		*cl;
	char			cs[MAX_STRING_CHARS];

	// NERVE - SMF - do not allow LogExit to be called in non-playing gamestate
	if (g_gamestate.integer != GS_PLAYING)
		return;

	G_LogPrintf("Exit: %s\n", string);

	level.intermissionQueued = level.time;

	// this will keep the clients from playing any voice sounds
	// that will get cut off when the queued intermission starts
	trap_SetConfigstring(CS_INTERMISSION, "1");

	for (i = 0; i < level.numConnectedClients; i++)
	{
		int		ping;

		cl = &level.clients[level.sortedClients[i]];

		// rain - #105 - use G_MakeUnready instead
		G_MakeUnready(&g_entities[level.sortedClients[i]]);

		if (cl->sess.sessionTeam == TEAM_SPECTATOR)
		{
			continue;
		}
		if (cl->pers.connected == CON_CONNECTING)
		{
			continue;
		}
		// CHRUKER: b016 - Make sure all the stats are recalculated and accurate
		G_CalcRank(cl);

		ping = cl->ps.ping < 999 ? cl->ps.ping : 999;

		G_LogPrintf("score: %i  ping: %i  client: %i %s\n", cl->ps.persistant[PERS_SCORE], ping, level.sortedClients[i], cl->pers.netname);
	}

	// CHRUKER: b016 - Moved here because we needed the stats to be up-to-date before sending
	G_LogPrintf("red:%i  blue:%i\n", level.teamScores[TEAM_AXIS], level.teamScores[TEAM_ALLIES]);

	// NERVE - SMF - send gameCompleteStatus message to master servers
	trap_SendConsoleCommand(EXEC_APPEND, "gameCompleteStatus\n");
	// b016


	// NERVE - SMF
	if (g_gametype.integer == GT_WOLF_STOPWATCH)
	{
		char	cs[MAX_STRING_CHARS];
		int		winner, defender;

		trap_GetConfigstring(CS_MULTI_INFO, cs, sizeof(cs));
		defender = atoi(Info_ValueForKey(cs, "defender"));

		trap_GetConfigstring(CS_MULTI_MAPWINNER, cs, sizeof(cs));
		winner = atoi(Info_ValueForKey(cs, "winner"));

		// NERVE - SMF
		if (!g_currentRound.integer)
		{
			if (winner == defender)
			{
				// if the defenders won, use default timelimit
				trap_Cvar_Set("g_nextTimeLimit", va("%f", g_timelimit.value));
			}
			else
			{
				// use remaining time as next timer
				trap_Cvar_Set("g_nextTimeLimit", va("%f", (level.timeCurrent - level.startTime) / 60000.f));
			}
		}
		else
		{
			// reset timer
			trap_Cvar_Set("g_nextTimeLimit", "0");
		}

		trap_Cvar_Set("g_currentRound", va("%i", !g_currentRound.integer));

		//bani - #113
		bani_storemapxp();
	}
	// -NERVE - SMF

	else if (g_gametype.integer == GT_WOLF_CAMPAIGN)
	{
		char	cs[MAX_STRING_CHARS];
		int	winner;
		int	highestskillpoints, highestskillpointsclient, j, teamNum;
		int	highestskillpointsincrease; // CHRUKER: b017 - Preventing medals from being handed out left and right

		trap_GetConfigstring(CS_MULTI_MAPWINNER, cs, sizeof(cs));
		winner = atoi(Info_ValueForKey(cs, "winner"));

		if (winner == 0)
		{
			g_axiswins.integer |= (1 << g_campaigns[level.currentCampaign].current);
			trap_Cvar_Set("g_axiswins", va("%i", g_axiswins.integer));
			trap_Cvar_Update(&g_axiswins);
		}
		else if (winner == 1)
		{
			g_alliedwins.integer |= (1 << g_campaigns[level.currentCampaign].current);
			trap_Cvar_Set("g_alliedwins", va("%i", g_alliedwins.integer));
			trap_Cvar_Update(&g_alliedwins);
		}

		trap_SetConfigstring(CS_ROUNDSCORES1, va("%i", g_axiswins.integer));
		trap_SetConfigstring(CS_ROUNDSCORES2, va("%i", g_alliedwins.integer));

		//bani - #113
		bani_storemapxp();

		// award medals
		for (teamNum = TEAM_AXIS; teamNum <= TEAM_ALLIES; teamNum++)
		{
			for (i = 0; i < SK_NUM_SKILLS; i++)
			{
				highestskillpoints = 0;
				highestskillpointsincrease = 0; // CHRUKER: b017 - Preventing medals from being handed out left and right
				highestskillpointsclient = -1;
				for (j = 0; j < level.numConnectedClients; j++)
				{
					cl = &level.clients[level.sortedClients[j]];

					if (cl->sess.sessionTeam != teamNum)
						continue;

					// CHRUKER: b017 - Make sure the player got some skills
					if (cl->sess.skill[i] < 1)
						continue;

					// CHRUKER: b017 - Only battlesense and light weapons medals are awarded to the highest score. Class medals get awarded to best ones.
					if (i == SK_BATTLE_SENSE || i == SK_LIGHT_WEAPONS)
					{
						if (cl->sess.skillpoints[i] > highestskillpoints)
						{
							highestskillpoints = cl->sess.skillpoints[i];
							highestskillpointsclient = j;
						}
					}
					else
					{
						if ((cl->sess.skillpoints[i] - cl->sess.startskillpoints[i]) > highestskillpointsincrease)
						{
							highestskillpointsincrease = (cl->sess.skillpoints[i] - cl->sess.startskillpoints[i]);
							highestskillpointsclient = j;
						}
					} // b017

				}

				if (highestskillpointsclient >= 0)
				{
					// highestskillpointsclient is the first client that has this highest
					// score. See if there are more clients with this same score. If so,
					// give them medals too
					for (j = highestskillpointsclient; j < level.numConnectedClients; j++)
					{
						cl = &level.clients[level.sortedClients[j]];

						if (cl->sess.sessionTeam != teamNum)
							continue;

						// CHRUKER: b017 - Make sure the player got some skills
						if (cl->sess.skill[i] < 1)
							continue;

						// CHRUKER: b017 - Only battlesense and light weapons medals are awarded to the highest score. Class medals get awarded to best ones.
						if (i == SK_BATTLE_SENSE || i == SK_LIGHT_WEAPONS)
						{
							if (cl->sess.skillpoints[i] == highestskillpoints)
							{
								cl->sess.medals[i]++;

								ClientUserinfoChanged(level.sortedClients[j]);
							}
						}
						else
						{
							if ((cl->sess.skillpoints[i] - cl->sess.startskillpoints[i]) == highestskillpointsincrease)
							{
								cl->sess.medals[i]++;
								ClientUserinfoChanged(level.sortedClients[j]);
							}
						} // b017

					}
				}
			}
		}
	}
	else if (g_gametype.integer == GT_WOLF)
	{

		//bani - #113
		bani_storemapxp();
	}

	G_BuildEndgameStats();
}


/*
=================
CheckIntermissionExit

The level will stay at the intermission for a minimum of 5 seconds
If all players wish to continue, the level will then exit.
If one or more players have not acknowledged the continue, the game will
wait 10 seconds before going on.
=================
*/
void CheckIntermissionExit(void)
{
	static int fActions = 0;
	qboolean exit = qtrue;
	int i;
	// rain - for #105
	gclient_t *cl;
	int ready = 0, notReady = 0;

	// OSP - end-of-level auto-actions
	//		  maybe make the weapon stats dump available to single player?
	if (!(fActions & EOM_WEAPONSTATS) && level.time - level.intermissiontime > 300)
	{
		G_matchInfoDump(EOM_WEAPONSTATS);
		fActions |= EOM_WEAPONSTATS;
	}
	if (!(fActions & EOM_MATCHINFO) && level.time - level.intermissiontime > 800)
	{
		G_matchInfoDump(EOM_MATCHINFO);
		fActions |= EOM_MATCHINFO;
	}

	for (i = 0; i < level.numConnectedClients; i++)
	{
		// rain - #105 - spectators and people who are still loading
		// don't have to be ready at the end of the round.
		// additionally, make readypercent apply here.

		cl = level.clients + level.sortedClients[i];

		if (cl->pers.connected != CON_CONNECTED || cl->sess.sessionTeam == TEAM_SPECTATOR)
		{
			continue;
		}
		else if (cl->pers.ready || (g_entities[level.sortedClients[i]].r.svFlags & SVF_BOT))
		{
			ready++;
		}
		else
		{
			notReady++;
		}
	}


	// rain - #105 - use the same code as the beginning of round ready to
	// decide whether enough players are ready to exceed
	// match_readypercent
	if ((ready + notReady > 0) && 100 * ready / (ready + notReady) >= match_readypercent.integer)
	{
		exit = qtrue;
	}
	else
	{
		exit = qfalse;
	}

	// Gordon: changing this to a minute for now
	if (!exit && (level.time < level.intermissiontime + 60000))
	{
		return;
	}

	ExitLevel();
}

/*
=============
ScoreIsTied
=============
*/
qboolean ScoreIsTied(void)
{
	int		a/*, b*/;
	char	cs[MAX_STRING_CHARS];
	char	*buf;

	// DHM - Nerve :: GT_WOLF checks the current value of
	trap_GetConfigstring(CS_MULTI_MAPWINNER, cs, sizeof(cs));

	buf = Info_ValueForKey(cs, "winner");
	a = atoi(buf);

	return a == -1;
}

qboolean G_ScriptAction_SetWinner(gentity_t *ent, char *params);

/*
=================
CheckExitRules

There will be a delay between the time the exit is qualified for
and the time everyone is moved to the intermission spot, so you
can see the last frag.
=================
*/
void CheckExitRules(void)
{
	char	cs[MAX_STRING_CHARS];

	// if at the intermission, wait for all non-bots to
	// signal ready, then go to next level
	if (g_gamestate.integer == GS_INTERMISSION)
	{
		CheckIntermissionExit();
		return;
	}

	if (level.intermissionQueued)
	{
		level.intermissionQueued = 0;
		BeginIntermission();
		return;
	}

	if (g_timelimit.value)
	{
		// OSP
		if ((level.timeCurrent - level.startTime) >= (g_timelimit.value * 60000))
		{
			// OSP

			// NERVE - SMF - do not allow LogExit to be called in non-playing gamestate
			// - This already happens in LogExit, but we need it for the print command
			if (g_gamestate.integer != GS_PLAYING)
				return;

			trap_SendServerCommand(-1, "print \"Timelimit hit.\n\"");
			LogExit("Timelimit hit.");

			return;
		}
	}

	if (level.numPlayingClients < 2)
	{
		return;
	}

	if (g_maxlives.integer > 0 || g_axismaxlives.integer > 0 || g_alliedmaxlives.integer > 0)
	{
		if (level.numFinalDead[0] >= level.numTeamClients[0] && level.numTeamClients[0] > 0)
		{
			trap_GetConfigstring(CS_MULTI_MAPWINNER, cs, sizeof(cs));
			Info_SetValueForKey(cs, "winner", "1");
			trap_SetConfigstring(CS_MULTI_MAPWINNER, cs);
			LogExit("Axis team eliminated.");
		}
		else if (level.numFinalDead[1] >= level.numTeamClients[1] && level.numTeamClients[1] > 0)
		{
			trap_GetConfigstring(CS_MULTI_MAPWINNER, cs, sizeof(cs));
			Info_SetValueForKey(cs, "winner", "0");
			trap_SetConfigstring(CS_MULTI_MAPWINNER, cs);
			LogExit("Allied team eliminated.");
		}
	}

}



void CheckWolfMP()
{
	// check because we run 6 game frames before calling Connect and/or ClientBegin
	// for clients on a map_restart
	if (g_gametype.integer >= GT_WOLF)
	{
		if (g_gamestate.integer == GS_PLAYING || g_gamestate.integer == GS_INTERMISSION)
		{
			if (level.intermissiontime && g_gamestate.integer != GS_INTERMISSION) trap_Cvar_Set("gamestate", va("%i", GS_INTERMISSION));
			return;
		}
	}
}

/*
==================
CheckVote
==================
*/
void CheckVote(void)
{
	int votewait;
	gentity_t *other;

	if ((!level.voteInfo.voteTime && !level.voteInfo.vote_passtime) || level.voteInfo.vote_fn == NULL || level.time - level.voteInfo.voteTime < 1000) return;

	votewait = tjg_votewait.integer;
	if (votewait < 0)
		votewait = 0;

	votewait *= 1000;

	// Dini, if the voter switches teams.
	other = g_entities + level.voteInfo.voter_cn;

	if (level.voteInfo.vote_passtime > 0)
	{
		int countdown, passtime;
		passtime = level.voteInfo.vote_passtime;

		// Cuntdown finished
		if (passtime + votewait < level.time)
		{
			level.voteInfo.vote_fn(NULL, 0, NULL, NULL, qfalse);
			level.voteInfo.vote_passtime = passtime = 0;
		}
		else
		{
			static int lasttime = 0;

			if (!lasttime)
				lasttime = votewait;

			countdown = votewait - (level.time - passtime);
			countdown = floor(countdown / 1000) + 1;

			if (countdown < lasttime)
			{
				char *sornot = "s";

				if (countdown == 1)
					sornot = "";

				if (countdown == 30 || countdown == 20 || countdown == 15 || countdown == 10 || countdown == 5 || countdown <= 3)
					AP(va("cpm \"^7'^3%s^7' will happen in ^3%i^7 second%s\n\"", level.voteInfo.voteString, countdown, sornot));
				lasttime = countdown;
			}

			return;
		}
	}
	else if (level.voteInfo.voter_team != other->client->sess.sessionTeam)
	{
		AP("cpm \"^5Vote canceled^z: voter switched teams\n\"");
		G_LogPrintf("Vote Failed: %s (voter %s switched teams)\n", level.voteInfo.voteString, other->client->pers.netname);
	}
	else if (level.time - level.voteInfo.voteTime >= VOTE_TIME)
	{
		AP(va("cpm \"^1Vote FAILED! ^3(%s)\n\"", level.voteInfo.voteString));
		G_LogPrintf("Vote Failed: %s\n", level.voteInfo.voteString);
	}
	else
	{
		int pcnt = vote_percent.integer;
		int total;

		if (pcnt > 99)
		{
			pcnt = 99;
		}
		if (pcnt < 1)
		{
			pcnt = 1;
		}

		// No need for per-team balanced kicking in tjmod..
		total = level.voteInfo.numVotingClients;

		// Ehm.. Lets say 51% for passing vote, and 6 players,
		// then you'd get a required > 3.06 players for passing the vote..
		// In other words, it should be rounded down to 3.0.
		if (level.voteInfo.voteYes > floor(pcnt * total / 100))
		{
			AP(va("cpm \"^2Vote passed: ^7(%s)\n\"", level.voteInfo.voteString));
			G_LogPrintf("Vote Passed: %s\n", level.voteInfo.voteString);

			// Queue the vote to be performed
			level.voteInfo.vote_passtime = level.time;
			level.voteInfo.voteTime = 0;
			trap_SetConfigstring(CS_VOTE_TIME, "");
			return;
		}
		else if (level.voteInfo.voteNo && level.voteInfo.voteNo >= floor((100 - pcnt)*total / 100))
		{
			// same behavior as a no response vote
			AP(va("cpm \"^1Vote FAILED: ^7(%s)\n\"", level.voteInfo.voteString));
			G_LogPrintf("Vote Failed: %s\n", level.voteInfo.voteString);
		}
		else
		{
			// still waiting for a majority
			return;
		}
	}

	level.voteInfo.voteTime = 0;
	trap_SetConfigstring(CS_VOTE_TIME, "");
}

/*
==================
CheckCvars
==================
*/
void CheckCvars(void)
{
	static int g_password_lastMod = -1;
	static int g_teamForceBalance_lastMod = -1;
	static int g_lms_teamForceBalance_lastMod = -1;

	if (g_password.modificationCount != g_password_lastMod)
	{
		g_password_lastMod = g_password.modificationCount;
		if (*g_password.string && Q_stricmp(g_password.string, "none"))
		{
			trap_Cvar_Set("g_needpass", "1");
		}
		else
		{
			trap_Cvar_Set("g_needpass", "0");
		}
	}

	if (g_teamForceBalance.modificationCount != g_teamForceBalance_lastMod)
	{
		g_teamForceBalance_lastMod = g_teamForceBalance.modificationCount;
		if (g_teamForceBalance.integer)
		{
			trap_Cvar_Set("g_balancedteams", "1");
		}
		else
		{
			trap_Cvar_Set("g_balancedteams", "0");
		}
	}

}

/*
=============
G_RunThink

Runs thinking code for this frame if necessary
=============
*/
void G_RunThink(gentity_t *ent)
{
	float	thinktime;

	// OSP - If paused, push nextthink
	if (level.match_pause != PAUSE_NONE && (ent - g_entities) >= g_maxclients.integer &&
			ent->nextthink > level.time && strstr(ent->classname, "DPRINTF_") == NULL)
	{
		ent->nextthink += level.time - level.previousTime;
	}

	// RF, run scripting
	if (ent->s.number >= MAX_CLIENTS)
	{
		G_Script_ScriptRun(ent);
	}

	thinktime = ent->nextthink;
	if (thinktime <= 0)
	{
		return;
	}
	if (thinktime > level.time)
	{
		return;
	}

	ent->nextthink = 0;
	if (!ent->think)
	{
		G_Error("NULL ent->think");
	}
	ent->think(ent);
}

void G_RunEntity(gentity_t *ent, int msec);

/*
======================
G_PositionEntityOnTag
======================
*/
qboolean G_PositionEntityOnTag(gentity_t *entity, gentity_t *parent, char *tagName)
{
	int				i;
	orientation_t	tag;
	vec3_t			axis[3];
	AnglesToAxis(parent->r.currentAngles, axis);

	VectorCopy(parent->r.currentOrigin, entity->r.currentOrigin);

	if (!trap_GetTag(-1, parent->tagNumber, tagName, &tag))
	{
		return qfalse;
	}

	for (i = 0 ; i < 3 ; i++)
	{
		VectorMA(entity->r.currentOrigin, tag.origin[i], axis[i], entity->r.currentOrigin);
	}

	if (entity->client && entity->s.eFlags & EF_MOUNTEDTANK)
	{
		// zinx - moved tank hack to here
		// bani - fix tank bb
		// zinx - figured out real values, only tag_player is applied,
		// so there are two left:
		// mg42upper attaches to tag_mg42nest[mg42base] at:
		// 0.03125, -1.171875, 27.984375
		// player attaches to tag_playerpo[mg42upper] at:
		// 3.265625, -1.359375, 2.96875
		// this is a hack, by the way.
		entity->r.currentOrigin[0] += 0.03125 + 3.265625;
		entity->r.currentOrigin[1] += -1.171875 + -1.359375;
		entity->r.currentOrigin[2] += 27.984375 + 2.96875;
	}

	G_SetOrigin(entity, entity->r.currentOrigin);

	if (entity->r.linked && !entity->client)
	{
		if (!VectorCompare(entity->oldOrigin, entity->r.currentOrigin))
		{
			trap_LinkEntity(entity);
		}
	}

	return qtrue;
}

void G_TagLinkEntity(gentity_t *ent, int msec)
{
	gentity_t *parent = &g_entities[ent->s.torsoAnim];
	vec3_t move, amove;
	gentity_t *obstacle;
	vec3_t origin, angles;
	vec3_t v;

	if (ent->linkTagTime >= level.time)
	{
		return;
	}

	G_RunEntity(parent, msec);

	if (!(parent->s.eFlags & EF_PATH_LINK))
	{
		if (parent->s.pos.trType == TR_LINEAR_PATH)
		{
			int pos;
			float frac;

			if ((ent->backspline = BG_GetSplineData(parent->s.effect2Time, &ent->back)) == NULL)
			{
				return;
			}

			ent->backdelta = parent->s.pos.trDuration ? (level.time - parent->s.pos.trTime) / ((float)parent->s.pos.trDuration) : 0;

			if (ent->backdelta < 0.f)
			{
				ent->backdelta = 0.f;
			}
			else if (ent->backdelta > 1.f)
			{
				ent->backdelta = 1.f;
			}

			if (ent->back)
			{
				ent->backdelta = 1 - ent->backdelta;
			}

			pos = floor(ent->backdelta * (MAX_SPLINE_SEGMENTS));
			if (pos >= MAX_SPLINE_SEGMENTS)
			{
				pos = MAX_SPLINE_SEGMENTS - 1;
				frac = ent->backspline->segments[pos].length;
			}
			else
			{
				frac = ((ent->backdelta * (MAX_SPLINE_SEGMENTS)) - pos) * ent->backspline->segments[pos].length;
			}


			VectorMA(ent->backspline->segments[pos].start, frac, ent->backspline->segments[pos].v_norm, v);
			if (parent->s.apos.trBase[0])
			{
				BG_LinearPathOrigin2(parent->s.apos.trBase[0], &ent->backspline, &ent->backdelta, v, ent->back);
			}

			VectorCopy(v, origin);

			if (ent->s.angles2[0])
			{
				BG_LinearPathOrigin2(ent->s.angles2[0], &ent->backspline, &ent->backdelta, v, ent->back);
			}

			VectorCopy(v, ent->backorigin);

			if (ent->s.angles2[0] < 0)
			{
				VectorSubtract(v, origin, v);
				vectoangles(v, angles);
			}
			else if (ent->s.angles2[0] > 0)
			{
				VectorSubtract(origin, v, v);
				vectoangles(v, angles);
			}
			else
			{
				VectorCopy(vec3_origin, origin);
			}

			ent->moving = qtrue;
		}
		else
		{
			ent->moving = qfalse;
		}
	}
	else
	{
		if (parent->moving)
		{
			VectorCopy(parent->backorigin, v);

			ent->back =			parent->back;
			ent->backdelta =	parent->backdelta;
			ent->backspline =	parent->backspline;

			VectorCopy(v, origin);

			if (ent->s.angles2[0])
			{
				BG_LinearPathOrigin2(ent->s.angles2[0], &ent->backspline, &ent->backdelta, v, ent->back);
			}

			VectorCopy(v, ent->backorigin);

			if (ent->s.angles2[0] < 0)
			{
				VectorSubtract(v, origin, v);
				vectoangles(v, angles);
			}
			else if (ent->s.angles2[0] > 0)
			{
				VectorSubtract(origin, v, v);
				vectoangles(v, angles);
			}
			else
			{
				VectorCopy(vec3_origin, origin);
			}

			ent->moving = qtrue;
		}
		else
		{
			ent->moving = qfalse;
		}
	}

	if (ent->moving)
	{
		VectorSubtract(origin, ent->r.currentOrigin, move);
		VectorSubtract(angles, ent->r.currentAngles, amove);

		if (!G_MoverPush(ent, move, amove, &obstacle))
		{
			script_mover_blocked(ent, obstacle);
		}

		VectorCopy(origin, ent->s.pos.trBase);
		VectorCopy(angles, ent->s.apos.trBase);
	}
	else
	{
		memset(&ent->s.pos, 0, sizeof(ent->s.pos));
		memset(&ent->s.apos, 0, sizeof(ent->s.apos));

		VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);
		VectorCopy(ent->r.currentAngles, ent->s.apos.trBase);
	}

	ent->linkTagTime = level.time;
}

void G_RunEntity(gentity_t *ent, int msec)
{
	if (ent->runthisframe)
	{
		return;
	}

	ent->runthisframe = qtrue;

	if (!ent->inuse)
	{
		return;
	}

	if (ent->tagParent)
	{

		G_RunEntity(ent->tagParent, msec);

		if (ent->tagParent)
		{
			if (G_PositionEntityOnTag(ent, ent->tagParent, ent->tagName))
			{
				if (!ent->client)
				{
					if (!ent->s.density)
					{
						BG_EvaluateTrajectory(&ent->s.apos, level.time, ent->r.currentAngles, qtrue, ent->s.effect2Time);
						VectorAdd(ent->tagParent->r.currentAngles, ent->r.currentAngles, ent->r.currentAngles);
					}
					else
					{
						BG_EvaluateTrajectory(&ent->s.apos, level.time, ent->r.currentAngles, qtrue, ent->s.effect2Time);
					}
				}
			}
		}
	}
	else if (ent->s.eFlags & EF_PATH_LINK)
	{

		G_TagLinkEntity(ent, msec);
	}

	// ydnar: hack for instantaneous velocity
	VectorCopy(ent->r.currentOrigin, ent->oldOrigin);

	// check EF_NODRAW status for non-clients
	if (ent - g_entities > level.maxclients)
	{
		if (ent->flags & FL_NODRAW)
		{
			ent->s.eFlags |= EF_NODRAW;
		}
		else
		{
			ent->s.eFlags &= ~EF_NODRAW;
		}
	}


	// clear events that are too old
	if (level.time - ent->eventTime > EVENT_VALID_MSEC)
	{
		if (ent->s.event)
		{
			ent->s.event = 0;
		}
		if (ent->freeAfterEvent)
		{
			// tempEntities or dropped items completely go away after their event
			G_FreeEntity(ent);
			return;
		}
		else if (ent->unlinkAfterEvent)
		{
			// items that will respawn will hide themselves after their pickup event
			ent->unlinkAfterEvent = qfalse;
			trap_UnlinkEntity(ent);
		}
	}

	// temporary entities don't think
	if (ent->freeAfterEvent)
	{
		return;
	}

	// Arnout: invisible entities don't think
	// NOTE: hack - constructible one does
	if (ent->s.eType != ET_CONSTRUCTIBLE && (ent->entstate == STATE_INVISIBLE || ent->entstate == STATE_UNDERCONSTRUCTION))
	{
		// Gordon: we want them still to run scripts tho :p
		if (ent->s.number >= MAX_CLIENTS)
		{
			G_Script_ScriptRun(ent);
		}
		return;
	}

	if (!ent->r.linked && ent->neverFree)
	{
		return;
	}

	if (ent->s.eType == ET_MISSILE
			|| ent->s.eType == ET_FLAMEBARREL
			|| ent->s.eType == ET_FP_PARTS
			|| ent->s.eType == ET_FIRE_COLUMN
			|| ent->s.eType == ET_FIRE_COLUMN_SMOKE
			|| ent->s.eType == ET_EXPLO_PART
			|| ent->s.eType == ET_RAMJET)
	{

		// OSP - pausing
		if (level.match_pause == PAUSE_NONE)
		{
			G_RunMissile(ent);
		}
		else
		{
			// During a pause, gotta keep track of stuff in the air
			ent->s.pos.trTime += level.time - level.previousTime;
			// Keep pulsing right for dynmamite
			if (ent->methodOfDeath == MOD_DYNAMITE)
			{
				ent->s.effect1Time += level.time - level.previousTime;
			}
			G_RunThink(ent);
		}
		// OSP

		return;
	}

	// DHM - Nerve :: Server-side collision for flamethrower
	if (ent->s.eType == ET_FLAMETHROWER_CHUNK)
	{
		G_RunFlamechunk(ent);

		// ydnar: hack for instantaneous velocity
		VectorSubtract(ent->r.currentOrigin, ent->oldOrigin, ent->instantVelocity);
		VectorScale(ent->instantVelocity, 1000.0f / msec, ent->instantVelocity);

		return;
	}

	if (ent->s.eType == ET_ITEM || ent->physicsObject)
	{
		G_RunItem(ent);

		// ydnar: hack for instantaneous velocity
		VectorSubtract(ent->r.currentOrigin, ent->oldOrigin, ent->instantVelocity);
		VectorScale(ent->instantVelocity, 1000.0f / msec, ent->instantVelocity);

		return;
	}

	if (ent->s.eType == ET_MOVER || ent->s.eType == ET_PROP)
	{
		G_RunMover(ent);

		// ydnar: hack for instantaneous velocity
		VectorSubtract(ent->r.currentOrigin, ent->oldOrigin, ent->instantVelocity);
		VectorScale(ent->instantVelocity, 1000.0f / msec, ent->instantVelocity);

		return;
	}

	if (ent - g_entities < MAX_CLIENTS)
	{
		G_RunClient(ent);

		// ydnar: hack for instantaneous velocity
		VectorSubtract(ent->r.currentOrigin, ent->oldOrigin, ent->instantVelocity);
		VectorScale(ent->instantVelocity, 1000.0f / msec, ent->instantVelocity);

		return;
	}

	// OSP - multiview
	if (ent->s.eType == ET_PORTAL && G_smvRunCamera(ent))
	{
		return;
	}

	if ((ent->s.eType == ET_HEALER || ent->s.eType == ET_SUPPLIER) && ent->target_ent)
	{
		ent->target_ent->s.onFireStart =	ent->health;
		ent->target_ent->s.onFireEnd =		ent->count;
	}

	G_RunThink(ent);

	// ydnar: hack for instantaneous velocity
	VectorSubtract(ent->r.currentOrigin, ent->oldOrigin, ent->instantVelocity);
	VectorScale(ent->instantVelocity, 1000.0f / msec, ent->instantVelocity);
}

/*
================
G_RunFrame

Advances the non-player objects in the world
================
*/
void G_RunFrame(int levelTime)
{
	int			i, msec;
	//	int			pass = 0;

	// if we are waiting for the level to restart, do nothing
	if (level.restarted)
	{
		return;
	}

	// Handling of pause offsets
	if (level.match_pause == PAUSE_NONE)
	{
		level.timeCurrent = levelTime - level.timeDelta;
	}
	else
	{
		level.timeDelta = levelTime - level.timeCurrent;
		if ((level.time % 500) == 0)
		{
			// FIXME: set a PAUSE cs and let the client adjust their local starttimes
			//        instead of this spam
			trap_SetConfigstring(CS_LEVEL_START_TIME, va("%i", level.startTime + level.timeDelta));
		}
	}

	level.frameTime = trap_Milliseconds();

	level.framenum++;
	level.previousTime = level.time;
	level.time = levelTime;

	msec = level.time - level.previousTime;

	level.axisBombCounter -= msec;
	level.alliedBombCounter -= msec;

	if (level.axisBombCounter < 0)
	{
		level.axisBombCounter = 0;
	}
	if (level.alliedBombCounter < 0)
	{
		level.alliedBombCounter = 0;
	}

#if 0
	if (trap_Cvar_VariableIntegerValue("dbg_spam"))
	{
		trap_SetConfigstring(CS_VOTE_STRING, va(
								 "vvvvvvvvvvvvvvvvvvvwiubgfiwebxqbwigb3qir4gviqrbegiuertbgiuoeyvqrugyveru\
qogyjvuqeyrvguqoehvrguorevqguoveruygqueorvguoqeyrvguyervguverougvequryvg\
uoerqvgouqevrguoerqvguoerqvguoyqevrguoyvreuogvqeuogiyvureoyvnguoeqvguoqe\
rvguoeqrvuoeqvrguoyvqeruogverquogvqoeurvgouqervguerqvgqiertbgiqerubgipqe\
rbgipqebgierqviqrviertqyvbgyerqvgieqrbgipqebrgpiqbergibepbreqgupqruiperq\
ubgipqeurbgpiqjefgpkeiueripgberipgubreugqeirpqgbipeqygbibgpibqpebiqgefpi\
mgbqepigjbriqpirbgipvbiqpgvbpqiegvbiepqbgqiebgipqgjebiperqbgpiqebpireqbg\
ipqbgipjqfebzipjgbqipqervbgiyreqvbgipqertvgbiprqbgipgbipertqjgbipubriuqi\
pjgpifjbqzpiebgipuerqbgpibuergpijfebgqiepgbiupreqbgpqegjfebzpigu4ebrigpq\
uebrgpiebrpgibqeripgubeqrpigubqifejbgipegbrtibgurepqgbn%i", level.time)
							);
	}
#endif

#ifdef SAVEGAME_SUPPORT
	G_CheckLoadGame();
#endif // SAVEGAME_SUPPORT

	// get any cvar changes
	G_UpdateCvars();

	for (i = 0; i < level.num_entities; i++)
	{
		g_entities[i].runthisframe = qfalse;
	}

	// go through all allocated objects
	for (i = 0; i < level.num_entities; i++)
	{
		G_RunEntity(&g_entities[ i ], msec);
	}


	for (i = 0; i < level.numConnectedClients; i++)
	{
		ClientEndFrame(&g_entities[level.sortedClients[i]]);
	}

	// NERVE - SMF
	CheckWolfMP();

	// see if it is time to end the level
	CheckExitRules();

	// update to team status?
	CheckTeamStatus();

	// cancel vote if timed out
	CheckVote();

	// for tracking changes
	CheckCvars();

	G_UpdateTeamMapData();

	if (level.gameManager)
	{
		level.gameManager->s.otherEntityNum = MAX_TEAM_LANDMINES - G_CountTeamLandmines(TEAM_AXIS);
		level.gameManager->s.otherEntityNum2 = MAX_TEAM_LANDMINES - G_CountTeamLandmines(TEAM_ALLIES);
	}

#ifdef SAVEGAME_SUPPORT
	// Check if we are reloading, and times have expired
	G_CheckReloadStatus();
#endif // SAVEGAME_SUPPORT
}

// Is this a single player type game - sp or coop?
qboolean G_IsSinglePlayerGame()
{
	if (g_gametype.integer == GT_SINGLE_PLAYER || g_gametype.integer == GT_COOP)
		return qtrue;

	return qfalse;
}
