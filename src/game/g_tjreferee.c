/*
 * Trickjump referee commands handling.
 */

#include "g_local.h"

/*
======================
Nolla - TJReferee
======================
*/

void Cmd_TJReferee_f(gentity_t *ent)
{
	char arg[MAX_TOKEN_CHARS];
	int argc;

	trap_Argv(1, arg, sizeof(arg));
	argc = trap_Argc();

	if (ent->client->sess.tjreferee == qtrue && argc == 1)
	{
		CP("print \"^5Commands:\n\"");
		CP("print \"^5togglesl        togglenoclip\n\"");
		CP("print \"^5togglegoto      togglecall:\n\"");
		CP("print \"^5toggleiwant     clearsaves:\n\"");
	}

	if (ent->client->sess.tjreferee == qtrue)
	{
		G_TJRefereeCommandCheck(ent, arg);
		return;
	}


	if (Q_stricmp(arg, tjrefereePassword.string))
	{
		CP("print \"^5Invalid TJ Referee password!\n\"");
		return;
	}

	ent->client->sess.tjreferee = qtrue;
	AP(va("cp \"%s ^5has become a TJ Referee.\n\"", ent->client->pers.netname));
}

qboolean G_TJRefereeCommandCheck(gentity_t *ent, char *cmd)
{
	if (!Q_stricmp(cmd, "toggleNoclip"))	G_toggleNoclip_cmd(ent);
	else if (!Q_stricmp(cmd, "toggleSL"))		G_toggleSL_cmd(ent);
	else if (!Q_stricmp(cmd, "clearSaves"))		G_clearSaves_cmd(ent);
	else if (!Q_stricmp(cmd, "toggleGoto"))		G_toggleGoto_cmd(ent);
	else if (!Q_stricmp(cmd, "toggleCall"))		G_toggleIwant_cmd(ent);
	else if (!Q_stricmp(cmd, "toggleIwant"))		G_toggleIwant_cmd(ent);
	else return(qfalse);
	return(qtrue);
}
