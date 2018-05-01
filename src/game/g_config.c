/*
 * Config/Mode settings
 */

#include "g_local.h"


#define M_FFA		0x01
#define M_1V1		0x02
#define M_SP		0x04
#define M_TEAM		0x08
#define M_CTF		0x10
#define M_WOLF		0x20
#define M_WSW		0x40
#define M_WCP		0x80
#define M_WCPH		0x100

#define M_ALL		M_FFA|M_1V1|M_SP|M_SP|M_TEAM|M_CTF|M_WOLF|M_WSW|M_WCP|M_WCPH

typedef struct
{
	int modes;
	char *cvar_name;
	char *cvar_value;
} modeCvarTable_t;

static const modeCvarTable_t aCompSettings[] =
{
	{ M_ALL, "tjg_weapons", "0" },
	{ M_ALL, "tjg_SaveLoad", "0" },
	{ M_ALL, "tjg_nofatigue", "1" },
	{ M_ALL, "tjg_endround", "1" },
	{ M_ALL, "tjg_resetspeed", "0" },
	{ M_ALL, "tjg_noclip", "0" },
	{ M_ALL, "tjg_god", "0" },
	{ 0, NULL, NULL }	//end of list
};

static const modeCvarTable_t aPubSettings[] =
{
	{ M_ALL, "tjg_weapons", "1" },
	{ M_ALL, "tjg_SaveLoad", "1" },
	{ M_ALL, "tjg_nofatigue", "1" },
	{ M_ALL, "tjg_endround", "0" },
	{ M_ALL, "tjg_resetspeed", "1" },
	{ M_ALL, "tjg_noclip", "1" },
	{ M_ALL, "tjg_god", "1" },
	{ 0, NULL, NULL }	//end of list
};



// Force settings to predefined state.
void G_configSet(int dwMode, qboolean doComp)
{
	unsigned int dwGameType;
	const modeCvarTable_t *pModeCvars;

	if (dwMode < 0 || dwMode >= GT_MAX_GAME_TYPE) return;

	dwGameType = 1 << dwMode;

	G_wipeCvars();

	for (pModeCvars = ((doComp) ? aCompSettings : aPubSettings); pModeCvars->cvar_name; pModeCvars++)
	{
		if (pModeCvars->modes & dwGameType)
		{
			trap_Cvar_Set(pModeCvars->cvar_name, pModeCvars->cvar_value);
			G_Printf("set %s %s\n", pModeCvars->cvar_name, pModeCvars->cvar_value);
		}
	}

	G_UpdateCvars();
	G_Printf(">> %s settings loaded!\n", (doComp) ? "Competition" : "Public");

	if (doComp && g_gamestate.integer == GS_WARMUP_COUNTDOWN)
	{
		level.lastRestartTime = level.time;
		trap_SendConsoleCommand(EXEC_APPEND, va("map_restart 0 %i\n", GS_WARMUP));
	}
	else
	{
		trap_SendConsoleCommand(EXEC_APPEND, va("map_restart 0 %i\n", GS_WARMUP));
	}
}
