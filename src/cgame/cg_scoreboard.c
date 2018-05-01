// cg_scoreboard -- draw the scoreboard on top of the game screen
#include "cg_local.h"


#define	SCOREBOARD_WIDTH	(31*BIGCHAR_WIDTH)

vec4_t clrUiBack = { 0.f, 0.f, 0.f, .6f };
vec4_t clrUiBar = { .16f, .2f, .17f, .8f };

/*
=================
WM_DrawObjectives
=================
*/

#define INFO_PLAYER_WIDTH		135
#define INFO_SCORE_WIDTH		56
#define INFO_FPS_WIDTH			36
#define INFO_CLASS_WIDTH		20
#define INFO_LATENCY_WIDTH		40
#define INFO_PMOVE_WIDTH		54
#define INFO_TEAM_HEIGHT		24
#define INFO_BORDER				0
#define INFO_LINE_HEIGHT		30
#define INFO_TOTAL_WIDTH		(INFO_PLAYER_WIDTH + INFO_CLASS_WIDTH + INFO_FPS_WIDTH + INFO_SCORE_WIDTH + INFO_PMOVE_WIDTH + INFO_LATENCY_WIDTH)


int WM_DrawObjectives(int x, int y, int width, float fade)
{
	const char *s, *str;
	int tempy, rows;
	int msec, mins, seconds, tens; // JPW NERVE
	vec4_t tclr =	{ 0.6f,		0.6f,		0.6f,		1.0f };

	if (cg.snap->ps.pm_type == PM_INTERMISSION)
	{
		const char *s, *buf, *shader = NULL, *flagshader = NULL, *nameshader = NULL;

		// Moved to CG_DrawIntermission
		/*		static int doScreenshot = 0, doDemostop = 0;

				// OSP - End-of-level autoactions
				if(!cg.demoPlayback) {
					if(!cg.latchVictorySound) {
						if(cg_autoAction.integer & AA_SCREENSHOT) {
							doScreenshot = cg.time + 1000;
						}
						if(cg_autoAction.integer & AA_STATSDUMP) {
							CG_dumpStats_f();
						}
						if((cg_autoAction.integer & AA_DEMORECORD) && (cgs.gametype == GT_WOLF_STOPWATCH && cgs.currentRound != 1)) {
							doDemostop = cg.time + 5000;	// stats should show up within 5 seconds
						}
					}
					if(doScreenshot > 0 && doScreenshot < cg.time) {
						CG_autoScreenShot_f();
						doScreenshot = 0;
					}
					if(doDemostop > 0 && doDemostop < cg.time) {
						trap_SendConsoleCommand("stoprecord\n");
						doDemostop = 0;
					}
				}
		*/
		rows = 8;
		y += SMALLCHAR_HEIGHT * (rows - 1);

		s = CG_ConfigString(CS_MULTI_MAPWINNER);
		buf = Info_ValueForKey(s, "winner");

		if (atoi(buf) == -1)
			str = "ITS A TIE!";
		else if (atoi(buf))
		{
			str = "ALLIES";
			//			shader = "ui/assets/portraits/allies_win";
			flagshader = "ui/assets/portraits/allies_win_flag.tga";
			nameshader = "ui/assets/portraits/text_allies.tga";

			/*			if ( !cg.latchVictorySound ) {
							cg.latchVictorySound = qtrue;
							trap_S_StartLocalSound( trap_S_RegisterSound( "sound/music/allies_win.wav", qtrue ), CHAN_LOCAL_SOUND );	// FIXME: stream
						}*/
		}
		else
		{
			str = "AXIS";
			//			shader = "ui/assets/portraits/axis_win";
			flagshader = "ui/assets/portraits/axis_win_flag.tga";
			nameshader = "ui/assets/portraits/text_axis.tga";

			/*			if ( !cg.latchVictorySound ) {
							cg.latchVictorySound = qtrue;
							trap_S_StartLocalSound( trap_S_RegisterSound( "sound/music/axis_win.wav", qtrue ), CHAN_LOCAL_SOUND );	// FIXME: stream
						}*/
		}

		y += SMALLCHAR_HEIGHT * ((rows - 2) / 2);

		if (flagshader)
		{
			CG_DrawPic(100, 10, 210, 136, trap_R_RegisterShaderNoMip(flagshader));
			CG_DrawPic(325, 10, 210, 136, trap_R_RegisterShaderNoMip(flagshader));
		}

		if (shader)
			CG_DrawPic(229, 10, 182, 136, trap_R_RegisterShaderNoMip(shader));
		if (nameshader)
		{
			CG_DrawPic(140, 50, 127, 64, trap_R_RegisterShaderNoMip(nameshader));
			CG_DrawPic(365, 50, 127, 64, trap_R_RegisterShaderNoMip("ui/assets/portraits/text_win.tga"));
		}
		return y;
	}
	// JPW NERVE -- mission time & reinforce time
	else
	{
		tempy = y;
		rows = 1;

		CG_FillRect(x - 5, y - 2, width + 5, 21, clrUiBack);
		CG_FillRect(x - 5, y - 2, width + 5, 21, clrUiBar);
		CG_DrawRect_FixedBorder(x - 5, y - 2, width + 5, 21, 1, colorBlack);

		y += SMALLCHAR_HEIGHT * (rows - 1);
		if (cgs.timelimit > 0.0f)
		{
			msec = (cgs.timelimit * 60.f * 1000.f) - (cg.time - cgs.levelStartTime);

			seconds = msec / 1000;
			mins = seconds / 60;
			seconds -= mins * 60;
			tens = seconds / 10;
			seconds -= tens * 10;
		}
		else
		{
			msec = mins = tens = seconds = 0;
		}

		if (cgs.timelimit > 1)
		{
			s = va("%s   %2.0f:%i%i", CG_TranslateString("Map time:"), (float)mins, tens, seconds);     // float cast to line up with reinforce time
		}
		else
		{
			//s = "^9www.^zTrickjump^9.me";
			s = va("^zTJMod %s", TJMOD_VERSION);
		}


		CG_Text_Paint_Ext(x, y + 13, 0.25f, 0.25f, tclr, s, 0, 0, 0, &cgs.media.limboFont2);

		s = va(CG_TranslateString(va("^f%s", cgs.rawmapname)));
		CG_Text_Paint_Ext(640 / 2 - CG_Text_Width_Ext(s, 0.25f, 0, &cgs.media.limboFont2) / 2, y + 13, 0.25f, 0.25f, tclr, s, 0, 0, 0, &cgs.media.limboFont2);

		s = "^9www.^zTrickjump^9.me";
		CG_Text_Paint_Ext(640 - 20 - CG_Text_Width_Ext(s, 0.25f, 0, &cgs.media.limboFont2), y + 13, 0.25f, 0.25f, tclr, s, 0, 0, 0, &cgs.media.limboFont2);

		y += SMALLCHAR_HEIGHT * 2;
	}
	// jpw

	return y;
}

static void WM_DrawClientScore(int x, int y, score_t *score, float *color, float fade)
{
	int offset;
	float tempx;
	vec4_t hcolor;
	clientInfo_t *ci;
	int	min, sec, mil, w;
	char *s;

	if (y + SMALLCHAR_HEIGHT >= 470)
		return;

	ci = &cgs.clientinfo[score->client];

	// Dini, Note, highlights yourself
	if (score->client == cg.snap->ps.clientNum)
	{
		tempx = x;

		hcolor[3] = fade * 0.3;
		VectorSet(hcolor, .5f, .5f, .2f);			// DARK-RED

		CG_FillRect(tempx - 4, y + 1, INFO_PLAYER_WIDTH - INFO_BORDER + 4, SMALLCHAR_HEIGHT - 1, hcolor);
		tempx += INFO_PLAYER_WIDTH;

		if (ci->team == TEAM_SPECTATOR)
		{
			int width;
			width = INFO_CLASS_WIDTH + INFO_SCORE_WIDTH + INFO_LATENCY_WIDTH * 2 - 6;

			CG_FillRect(tempx, y + 1, width - INFO_BORDER, SMALLCHAR_HEIGHT - 1, hcolor);
			tempx += width;
		}
		else
		{
			CG_FillRect(tempx, y + 1, INFO_CLASS_WIDTH - INFO_BORDER, SMALLCHAR_HEIGHT - 1, hcolor);
			tempx += INFO_CLASS_WIDTH;


			CG_FillRect(tempx, y + 1, INFO_FPS_WIDTH - INFO_BORDER, SMALLCHAR_HEIGHT - 1, hcolor);
			tempx += INFO_FPS_WIDTH;


			// Shows pmove now, cba to rename everything :P
			CG_FillRect(tempx, y + 1, INFO_PMOVE_WIDTH, SMALLCHAR_HEIGHT - 1, hcolor);
			tempx += INFO_PMOVE_WIDTH;


			CG_FillRect(tempx, y + 1, INFO_LATENCY_WIDTH - 1, SMALLCHAR_HEIGHT - 1, hcolor);
			tempx += INFO_LATENCY_WIDTH;

		}
	}

	tempx = x;

	// DHM - Nerve
	VectorSet(hcolor, 1, 1, 1);
	hcolor[3] = fade;

	offset = 0;

	// Dini, Note, starts line with player infos etc
	// CG_DrawSmallString( tempx, y, va("%i", cg.ps.clientnum), fade );
	// draw name
	CG_DrawStringExt(tempx, y, ci->name, hcolor, qfalse, qfalse, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 16);

	tempx += INFO_PLAYER_WIDTH - offset;

	if (ci->team == TEAM_SPECTATOR)
	{
		const char *s;
		int totalwidth;

		totalwidth = INFO_CLASS_WIDTH + INFO_SCORE_WIDTH + INFO_LATENCY_WIDTH - 8;

		if (score->ping == -1)
		{
			s = CG_TranslateString("^3CONNECTING");
			w = CG_DrawStrlen(s) * SMALLCHAR_WIDTH;
			CG_DrawSmallString(tempx + 10, y, s, fade);
		}
		else if (ci->clientNum == score->followedClient)
		{
			s = CG_TranslateString("^3SPECTATOR");
			w = CG_DrawStrlen(s) * SMALLCHAR_WIDTH;
			CG_DrawSmallString(tempx + 12, y, s, fade);
		}
		else
		{
			s = cgs.clientinfo[score->followedClient].name;

			// Draws max 12 chars of the name
			CG_DrawStringExt(tempx + 3, y, s, hcolor, qfalse, qfalse, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 12);

			trap_R_SetColor(colorYellow);
			CG_DrawPic(tempx - 8, y + 2, 12, 12, cgs.media.specArrow);
			trap_R_SetColor(NULL);
		}

		// Dini, show ping for specs too..
		CG_DrawSmallString(tempx + 10 + INFO_PMOVE_WIDTH + INFO_FPS_WIDTH, y, va("^z%5i", score->ping), fade);
		return;
	}
	// Dini, Draw small image \ icon for class
	else
	{
		//CG_DrawSmallString( tempx, y, CG_TranslateString( BG_ShortClassnameForNumber( score->playerClass ) ), fade );
		// cg_drawpic, x, y, w, h, pic

		int playerType;
		playerType = ci->cls;

		if (playerType == PC_MEDIC)
		{
			CG_DrawPic(tempx + 1, y + 2, 12, 12, cgs.media.skillPics[SK_FIRST_AID]);
		}
		else if (playerType == PC_ENGINEER)
		{
			CG_DrawPic(tempx + 1, y + 2, 12, 12, cgs.media.skillPics[SK_EXPLOSIVES_AND_CONSTRUCTION]);
		}
		else if (playerType == PC_FIELDOPS)
		{
			CG_DrawPic(tempx + 1, y + 2, 12, 12, cgs.media.skillPics[SK_SIGNALS]);
		}
		else if (playerType == PC_COVERTOPS)
		{
			CG_DrawPic(tempx + 1, y + 2, 12, 12, cgs.media.skillPics[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS]);
		}
		else
		{
			CG_DrawPic(tempx + 1, y + 2, 12, 12, cgs.media.skillPics[SK_HEAVY_WEAPONS]);
		}
	}

	tempx += INFO_CLASS_WIDTH;

	if (isTimerun.integer)
	{
		if (ci->timerunBT)
		{
			mil = ci->timerunBT;
			min = mil / 60000;
			mil -= min * 60000;
			sec = mil / 1000;
			mil -= sec * 1000;

			s = va("^7%02d:%02d.%03d", min, sec, mil);
		}
		else
			s = "^zNo Time";

		tempx += 10;
		CG_DrawSmallString(tempx, y, s, fade);

		// Dini, shows current Pmove..
		//CG_DrawSmallString( tempx+15, y, va( "%4i", ci->pmoveFixed  ), fade );
		tempx += 25;
	}
	else
	{
		// Dini, shows current FPS..
		CG_DrawSmallString(tempx, y, va("%3i", ci->maxFPS), fade);
		tempx += (INFO_FPS_WIDTH / 2) - 5;

		// Dini, shows current Pmove..
		CG_DrawSmallString(tempx + 15, y, va("%4i", ci->pmoveFixed), fade);
		tempx += (INFO_PMOVE_WIDTH / 2) - 5;
	}

	// Ping
	CG_DrawSmallString(tempx + 45, y, va("%5i", score->ping), fade);

}/*
	if (isTimerun.integer)
	{
		tempx += 25;
		CG_DrawSmallString( tempx, y, CG_TranslateString( "TIME" ), fade );
		tempx += 66;
	}
	else
	{
		CG_DrawSmallString( tempx, y, CG_TranslateString( "FPS" ), fade );
		tempx += INFO_FPS_WIDTH;

		CG_DrawSmallString( tempx, y, CG_TranslateString( "Pmove" ), fade );
		tempx += INFO_PMOVE_WIDTH;
	}*/



const char *WM_TimeToString(float msec)
{
	int mins, seconds, tens;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	return va("%i:%i%i", mins, tens, seconds);
}



static void WM_DrawClientScore_Small(int x, int y, score_t *score, float *color, float fade)
{

}

static int WM_DrawInfoLine(int x, int y, float fade)
{
	int w, defender, winner;
	const char *s;
	vec4_t tclr =	{ 0.6f,		0.6f,		0.6f,		1.0f };

	if (cg.snap->ps.pm_type != PM_INTERMISSION)
	{
		return y;
	}

	w = 360;
	//	CG_DrawPic( 320 - w/2, y, w, INFO_LINE_HEIGHT, trap_R_RegisterShaderNoMip( "ui/assets/mp_line_strip.tga" ) );

	s = CG_ConfigString(CS_MULTI_INFO);
	defender = atoi(Info_ValueForKey(s, "defender"));

	s = CG_ConfigString(CS_MULTI_MAPWINNER);
	winner = atoi(Info_ValueForKey(s, "winner"));

	if (cgs.currentRound)
	{
		// first round
		s = va(CG_TranslateString("CLOCK IS NOW SET TO %s!"), WM_TimeToString(cgs.nextTimeLimit * 60.f * 1000.f));
	}
	else
	{
		// second round
		if (!defender)
		{
			if (winner != defender)
				s = "ALLIES SUCCESSFULLY BEAT THE CLOCK!";
			else
				s = "ALLIES COULDN'T BEAT THE CLOCK!";
		}
		else
		{
			if (winner != defender)
				s = "AXIS SUCCESSFULLY BEAT THE CLOCK!";
			else
				s = "AXIS COULDN'T BEAT THE CLOCK!";
		}

		s = CG_TranslateString(s);
	}

	CG_FillRect(320 - w / 2, y, w, 20, clrUiBar);
	CG_DrawRect_FixedBorder(320 - w / 2, y, w, 20, 1, colorBlack);

	w = CG_Text_Width_Ext(s, 0.25f, 0, &cgs.media.limboFont1);

	CG_Text_Paint_Ext(320 - w * 0.5f, y + 15, 0.25f, 0.25f, tclr, s, 0, 0, 0, &cgs.media.limboFont1);
	//	CG_DrawSmallString( 320 - w/2, ( y + INFO_LINE_HEIGHT / 2 ) - SMALLCHAR_HEIGHT / 2, s, fade );
	return y + INFO_LINE_HEIGHT + 6;
}


static int WM_TeamScoreboard(int x, int y, team_t team, float fade, int maxrows)
{
	vec4_t hcolor;
	float tempx, tempy;
	int height, width;
	int i;
	int count = 0;
	vec4_t tclr =	{ 0.6f,		0.6f,		0.6f,		1.0f };

	height = SMALLCHAR_HEIGHT * maxrows;
	width = INFO_PLAYER_WIDTH + INFO_CLASS_WIDTH + INFO_PMOVE_WIDTH + INFO_FPS_WIDTH + INFO_LATENCY_WIDTH;

	CG_FillRect(x - 5, y - 2, width + 5, 21, clrUiBack);
	CG_FillRect(x - 5, y - 2, width + 5, 21, clrUiBar);

	Vector4Set(hcolor, 0, 0, 0, fade);
	CG_DrawRect_FixedBorder(x - 5, y - 2, width + 5, 21, 1, colorBlack);

	// draw header
	if (cg_gameType.integer == GT_WOLF_LMS)
	{
		char *s;
		if (team == TEAM_AXIS)
		{
			s = va("%s [%d] (%d %s)", CG_TranslateString("AXIS"), cg.teamScores[0], cg.teamPlayers[team], CG_TranslateString("PLAYERS"));
			s = va("%s ^3%s", s, cg.teamFirstBlood == TEAM_AXIS ? CG_TranslateString("FIRST BLOOD") : "");

			CG_Text_Paint_Ext(x, y + 13, 0.25f, 0.25f, tclr, s, 0, 0, 0, &cgs.media.limboFont1);
		}
		else if (team == TEAM_ALLIES)
		{
			s = va("%s [%d] (%d %s)", CG_TranslateString("ALLIES"), cg.teamScores[1], cg.teamPlayers[team], CG_TranslateString("PLAYERS"));
			s = va("%s ^3%s", s, cg.teamFirstBlood == TEAM_ALLIES ? CG_TranslateString("FIRST BLOOD") : "");

			CG_Text_Paint_Ext(x, y + 13, 0.25f, 0.25f, tclr, s, 0, 0, 0, &cgs.media.limboFont1);
		}
	}
	else
	{
		if (team == TEAM_AXIS)
		{
			CG_Text_Paint_Ext(x, y + 13, 0.25f, 0.25f, tclr, va("%s [%d] (%d %s)", CG_TranslateString("AXIS"), cg.teamScores[0], cg.teamPlayers[team], CG_TranslateString("PLAYERS")), 0, 0, 0, &cgs.media.limboFont1);
		}
		else if (team == TEAM_ALLIES)
		{
			CG_Text_Paint_Ext(x, y + 13, 0.25f, 0.25f, tclr, va("%s [%d] (%d %s)", CG_TranslateString("ALLIES"), cg.teamScores[1], cg.teamPlayers[team], CG_TranslateString("PLAYERS")), 0, 0, 0, &cgs.media.limboFont1);
		}
	}


	y += SMALLCHAR_HEIGHT + 3;

	// save off y val
	tempy = y;

	// draw color bands
	for (i = 0; i <= maxrows; i++)
	{
		if (i % 2 == 0)
			VectorSet(hcolor, (80.f / 255.f), (80.f / 255.f), (80.f / 255.f));			// LIGHT BLUE
		else
			VectorSet(hcolor, (0.f / 255.f), (0.f / 255.f), (0.f / 255.f));			// DARK BLUE
		hcolor[3] = fade * 0.3;

		CG_FillRect(x - 5, y, width + 5, SMALLCHAR_HEIGHT + 1, hcolor);
		trap_R_SetColor(colorBlack);
		CG_DrawTopBottom(x - 5, y, width + 5, SMALLCHAR_HEIGHT + 1, 1);
		trap_R_SetColor(NULL);

		y += SMALLCHAR_HEIGHT;
	}
	hcolor[3] = 1;

	y = tempy;

	tempx = x;

	CG_FillRect(x - 5, y - 1, width + 5, 18, clrUiBack);
	//CG_FillRect( x-5, y-1, width+5, 18, clrUiBar );
	trap_R_SetColor(colorBlack);
	CG_DrawTopBottom(x - 5, y - 1, width + 5, 18, 1);
	trap_R_SetColor(NULL);

	// draw player info headings
	CG_DrawSmallString(tempx, y, CG_TranslateString("Name"), fade);
	tempx += INFO_PLAYER_WIDTH;

	CG_DrawSmallString(tempx + 2, y, CG_TranslateString("C"), fade);
	tempx += INFO_CLASS_WIDTH;

	if (isTimerun.integer)
	{
		tempx += 25;
		CG_DrawSmallString(tempx, y, CG_TranslateString("TIME"), fade);
		tempx += 66;
	}
	else
	{
		CG_DrawSmallString(tempx, y, CG_TranslateString("FPS"), fade);
		tempx += INFO_FPS_WIDTH;

		CG_DrawSmallString(tempx, y, CG_TranslateString("Pmove"), fade);
		tempx += INFO_PMOVE_WIDTH;
	}

	CG_DrawSmallString(tempx, y, CG_TranslateString("Ping"), fade);
	tempx += INFO_LATENCY_WIDTH;


	y += SMALLCHAR_HEIGHT;

	// draw player info
	VectorSet(hcolor, 1, 1, 1);
	hcolor[3] = fade;

	cg.teamPlayers[team] = 0; // JPW NERVE
	for (i = 0; i < cg.numScores; i++)
	{
		if (team != cgs.clientinfo[ cg.scores[i].client ].team)
			continue;

		cg.teamPlayers[team]++;
	}

	count = 0;
	for (i = 0; i < cg.numScores && count < maxrows; i++)
	{
		if (team != cgs.clientinfo[ cg.scores[i].client ].team)
		{
			continue;
		}

		WM_DrawClientScore(x, y, &cg.scores[i], hcolor, fade);
		y += SMALLCHAR_HEIGHT;
		count++;
	}

	// draw spectators
	y += SMALLCHAR_HEIGHT;

	for (i = 0; i < cg.numScores; i++)
	{
		if (cgs.clientinfo[ cg.scores[i].client ].team != TEAM_SPECTATOR)
			continue;
		if (team == TEAM_AXIS && (i % 2))
			continue;
		if (team == TEAM_ALLIES && ((i + 1) % 2))
			continue;

		WM_DrawClientScore(x, y, &cg.scores[i], hcolor, fade);
		y += SMALLCHAR_HEIGHT;
	}

	return y;
}
// -NERVE - SMF

/*
=================
CG_DrawScoreboard

Draw the normal in-game scoreboard
=================
*/
qboolean CG_DrawScoreboard(void)
{
	int		x = 0, y = 0, x_right;
	float	fade;
	float	*fadeColor;

	x = 20;
	y = 10;

	x_right = 640 - x - (INFO_TOTAL_WIDTH - 5);

	// don't draw anything if the menu or console is up
	if (cg_paused.integer)
	{
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	// OSP - also for pesky scoreboards in demos
	if ((cg.demoPlayback && cg.snap->ps.pm_type != PM_INTERMISSION) && !cg.showScores)
	{
		return qfalse;
	}

	// don't draw if in cameramode
	if (cg.cameraMode)
	{
		return qtrue;
	}

	if (cg.showScores || cg.ps.pm_type == PM_INTERMISSION)
	{
		fade = 1.0;
		fadeColor = colorWhite;
	}
	else
	{
		fadeColor = CG_FadeColor(cg.scoreFadeTime, FADE_TIME);

		if (!fadeColor)
		{
			// next time scoreboard comes up, don't print killer
			*cg.killerName = 0;
			return qfalse;
		}
		fade = fadeColor[3];
	}

	y = WM_DrawObjectives(x, y, 640 - 2 * x + 5, fade);

	// Dini, Note, positions \ draws the team container thingies etc
	if (cgs.gametype == GT_WOLF_STOPWATCH && (cg.snap->ps.pm_type == PM_INTERMISSION))
	{
		y = WM_DrawInfoLine(x, 155, fade);

		WM_TeamScoreboard(x, y, TEAM_AXIS, fade, 8);
		x = x_right;
		WM_TeamScoreboard(x, y, TEAM_ALLIES, fade, 8);
	}
	else
	{
		if (cg.snap->ps.pm_type == PM_INTERMISSION)
		{
			WM_TeamScoreboard(x, y, TEAM_AXIS, fade, 9);
			x = x_right;
			WM_TeamScoreboard(x, y, TEAM_ALLIES, fade, 9);
		}
		else
		{
			WM_TeamScoreboard(x, y, TEAM_AXIS, fade, 25);
			x = x_right;
			WM_TeamScoreboard(x + 56, y, TEAM_ALLIES, fade, 25);
		}
	}

	/*	if(!CG_IsSinglePlayer()) {
			qtime_t ct;

			G_showWindowMessages();
			trap_RealTime(&ct);
			s = va("^3%02d:%02d:%02d - %02d %s %d",
								ct.tm_hour, ct.tm_min, ct.tm_sec,
								ct.tm_mday, aMonths[ct.tm_mon], 1900 + ct.tm_year);
			CG_DrawStringExt(444, 12, s, colorWhite, qfalse, qtrue, 8, 8, 0);
		}
	*/
	return qtrue;
}
