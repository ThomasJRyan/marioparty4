#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/audio.h"
#include "game/board/window.h"
#include "game/board/ui.h"
#include "game/board/space.h"
#include "game/board/star.h"
#include "game/audio.h"
#include "game/objsub.h"
#include "game/pad.h"

#include "game/data.h"
#include "game/wipe.h"

#include "ext_math.h"

static s16 stopWin = -1;
static s16 wheelMdl = -1;
s16 lbl_801D3854 = -1;
static s16 focusMdl = -1;
static s32 messBase = -1;

static s32 hostMotTbl[][8] = {
	{ 
		DATA_MAKE_NUM(DATADIR_BLAST5, 23),
		DATA_MAKE_NUM(DATADIR_BLAST5, 24),
		DATA_MAKE_NUM(DATADIR_BLAST5, 25),
		DATA_MAKE_NUM(DATADIR_BLAST5, 26),
		DATA_MAKE_NUM(DATADIR_BLAST5, 27),
		DATA_MAKE_NUM(DATADIR_BLAST5, 28),
		DATA_MAKE_NUM(DATADIR_BLAST5, 29),
		DATA_MAKE_NUM(DATADIR_BLAST5, 30)
	},
	{ 
		DATA_MAKE_NUM(DATADIR_BLAST5, 31),
		DATA_MAKE_NUM(DATADIR_BLAST5, 32),
		DATA_MAKE_NUM(DATADIR_BLAST5, 33),
		DATA_MAKE_NUM(DATADIR_BLAST5, 34),
		DATA_MAKE_NUM(DATADIR_BLAST5, 35),
		DATA_MAKE_NUM(DATADIR_BLAST5, 36),
		DATA_MAKE_NUM(DATADIR_BLAST5, 37),
		DATA_MAKE_NUM(DATADIR_BLAST5, 38)
	},
	{ 
		DATA_MAKE_NUM(DATADIR_BLAST5, 39),
		DATA_MAKE_NUM(DATADIR_BLAST5, 40),
		DATA_MAKE_NUM(DATADIR_BLAST5, 41),
		DATA_MAKE_NUM(DATADIR_BLAST5, 42),
		DATA_MAKE_NUM(DATADIR_BLAST5, 43),
		DATA_MAKE_NUM(DATADIR_BLAST5, 44),
		DATA_MAKE_NUM(DATADIR_BLAST5, 45),
		DATA_MAKE_NUM(DATADIR_BLAST5, 46)
	},
	{ 
		DATA_MAKE_NUM(DATADIR_BLAST5, 47),
		DATA_MAKE_NUM(DATADIR_BLAST5, 48),
		DATA_MAKE_NUM(DATADIR_BLAST5, 49),
		DATA_MAKE_NUM(DATADIR_BLAST5, 50),
		DATA_MAKE_NUM(DATADIR_BLAST5, 51),
		DATA_MAKE_NUM(DATADIR_BLAST5, 52),
		DATA_MAKE_NUM(DATADIR_BLAST5, 53),
		DATA_MAKE_NUM(DATADIR_BLAST5, 54)
	},
	{ 
		DATA_MAKE_NUM(DATADIR_BLAST5, 55),
		DATA_MAKE_NUM(DATADIR_BLAST5, 56),
		DATA_MAKE_NUM(DATADIR_BLAST5, 57),
		DATA_MAKE_NUM(DATADIR_BLAST5, 58),
		DATA_MAKE_NUM(DATADIR_BLAST5, 59),
		DATA_MAKE_NUM(DATADIR_BLAST5, 60),
		DATA_MAKE_NUM(DATADIR_BLAST5, 61),
		DATA_MAKE_NUM(DATADIR_BLAST5, 62)
	},
	{ 
		DATA_MAKE_NUM(DATADIR_BLAST5, 63),
		DATA_MAKE_NUM(DATADIR_BLAST5, 64),
		DATA_MAKE_NUM(DATADIR_BLAST5, 65),
		DATA_MAKE_NUM(DATADIR_BLAST5, 66),
		DATA_MAKE_NUM(DATADIR_BLAST5, 67),
		DATA_MAKE_NUM(DATADIR_BLAST5, 68),
		DATA_MAKE_NUM(DATADIR_BLAST5, 69),
		DATA_MAKE_NUM(DATADIR_BLAST5, 70)
	},
	{ 
		-1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
	},
	{ 
		-1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
	},
	{ 
		-1,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
	},
};

static s32 messBaseTbl[] = {
	MAKE_MESSID(14, 0),
	MAKE_MESSID(14, 24),
	MAKE_MESSID(14, 48),
	MAKE_MESSID(14, 72),
	MAKE_MESSID(14, 96),
	MAKE_MESSID(14, 120),
	0,
	0,
	0,
};

static float playerOfsTbl[][2] = {
	{ -94, 0 },
	{ 0, 0 },
	{ 94, 0 }
};

static char *plateObjNameTbl[] = {
	"plate",
	"plate1",
	"plate2",
	"plate3"
};

static char *hiliteObjNameTbl[] = {
	"lite",
	"lite1",
	"lite2",
	"lite3"
};

static char *faceObjNameTbl[] = {
	"kao",
	"kao1",
	"kao2",
	"kao3"
};

static s32 resultRankFX12Tbl[] = {
	0x122,
	0x162,
	0x1A2,
	0x1E2,
	0x222,
	0x262,
	0x2A2,
	0x2E2
};

static s32 resultRankFX3Tbl[] = {
	0x12A,
	0x16A,
	0x1AA,
	0x1EA,
	0x22A,
	0x26A,
	0x2AA,
	0x2EA
};

static s32 resultRankFX4Tbl[] = {
	0x12E,
	0x16E,
	0x1AE,
	0x1EE,
	0x22E,
	0x26E,
	0x2AE,
	0x2EE
};

static s32 impactFXTbl[] = {
	0x119,
	0x159,
	0x199,
	0x1D9,
	0x219,
	0x259,
	0x299,
	0x2D9
};

static s32 lotteryWheelMdlTbl[] = {
	DATA_MAKE_NUM(DATADIR_BLAST5, 13),
	DATA_MAKE_NUM(DATADIR_BLAST5, 14),
	DATA_MAKE_NUM(DATADIR_BLAST5, 15),
	DATA_MAKE_NUM(DATADIR_BLAST5, 16),
	DATA_MAKE_NUM(DATADIR_BLAST5, 17),
	DATA_MAKE_NUM(DATADIR_BLAST5, 18),
	DATA_MAKE_NUM(DATADIR_BLAST5, 19),
	DATA_MAKE_NUM(DATADIR_BLAST5, 20),
};

static s16 teamSprPrioTbl[] = {
	1520,
	1510,
	1500,
	1500,
	1500,
	1500,
	1500,
	1500,
	1500,
	1500,
	1500,
	1490,
	1490,
	1500,
	1500
};

static s32 teamSprTbl[] = {
	DATA_MAKE_NUM(DATADIR_BOARD, 39),
	DATA_MAKE_NUM(DATADIR_BOARD, 38),
	DATA_MAKE_NUM(DATADIR_BOARD, 41),
	DATA_MAKE_NUM(DATADIR_BOARD, 40),
	DATA_MAKE_NUM(DATADIR_BOARD, 40),
	DATA_MAKE_NUM(DATADIR_BOARD, 40),
	DATA_MAKE_NUM(DATADIR_BOARD, 45),
	DATA_MAKE_NUM(DATADIR_BOARD, 40),
	DATA_MAKE_NUM(DATADIR_BOARD, 40),
	DATA_MAKE_NUM(DATADIR_BOARD, 40),
	DATA_MAKE_NUM(DATADIR_BOARD, 44),
	DATA_MAKE_NUM(DATADIR_BOARD, 42),
	DATA_MAKE_NUM(DATADIR_BOARD, 42),
};

static float teamSprPosTbl[][2] = {
	{ 0, -35 },
	{ 0, 0 },
	{ -68, -38 },
	{ 80, -22 },
	{ 64, -22 },
	{ 48, -22 },
	{ 26, -22 },
	{ 80, -48 },
	{ 64, -48 },
	{ 48, -48 },
	{ 26, -48 },
	{ -18, -40 },
	{ -18, 32 },
	{ -16, -72 },
	{ -16, 0 }
};

static s32 teamCharSprTbl[] = {
	DATA_MAKE_NUM(DATADIR_BOARD, 46),
	DATA_MAKE_NUM(DATADIR_BOARD, 47),
	DATA_MAKE_NUM(DATADIR_BOARD, 48),
	DATA_MAKE_NUM(DATADIR_BOARD, 49),
	DATA_MAKE_NUM(DATADIR_BOARD, 50),
	DATA_MAKE_NUM(DATADIR_BOARD, 51),
	DATA_MAKE_NUM(DATADIR_BOARD, 52),
	DATA_MAKE_NUM(DATADIR_BOARD, 53),
};

static s8 currTicket;
static s8 numTickets;
static s8 last5Player;
static s16 last5Space;
static s16 hostMdl;
static OMOBJ *last5RouletteObj;
static HUPROCESS *last5Proc;
static OMOBJ *teamResultObj;
static OMOBJ *hostMoveObj;
static OMOBJ *lotteryDrawObj;
static OMOBJ *lotteryDrawWheelObj;

static Vec spacePos;
static Vec focusPos;
static Vec hostPos;
static s16 hostMot[8];
static OMOBJ *lotteryTicketObj[4];

static void DestroyLast5(void);

static void ExecLast5(void);
static void CreateLast5(void);
static void KillLast5(void);

static void InitHost(void);
static void KillHost(void);

static void Last5Main(void);

static s32 CheckJump(s32 player);

static void CreateLotteryDrawWheel(void);
static void CreateLotteryDraw(Vec *pos);
static void UpdateLotteryDraw(OMOBJ *object);
static void SetLotteryDrawState(s32 state);
static s32 GetLotteryDrawState();
static void KillLotteryDrawWheel(void);
static void UpdateLotteryDrawWheel(OMOBJ *object);
static void SetLotteryDrawWheelState(s32 state);
static void ExecLotteryDraw(void);

static void KillLotteryTicket(void);
static void InitLotteryTicket(void);
static void UpdateLotteryTicket(OMOBJ *object);
static void SetLotteryTicketState(s32 player, s32 state);
static s32 GetLotteryTicketPlayer(void);
static s32 CheckLotteryTicket(void);
static void UpdateLotteryTicketMatch(s32 progress, s32 character);

static void StartHostMove(Vec *from, Vec *to, s16 time);
static void ExecHostMove(OMOBJ *object);
static s32 CheckHostMove(void);

static void CreateStopWin(void);

static void CreateLast5Roulette(void);
static void UpdateLast5Roulette(OMOBJ *object);

static void SetLast5RouletteFade(s32 flag);
static void SetLast5RouletteState(s32 state);
static s32 GetLast5RouletteState(void);
static s32 GetLast5RouletteResult(void);

static void CreateTeamResult(void);
static void GetTeamResultTarget(s32 team, Vec *pos);
static void SetTeamResultTarget(s32 team, Vec *pos);
static void KillTeamResult(void);
static void UpdateTeamResult(OMOBJ *object);

/**
 * @brief Runs and waits for the final-five-turn board event process.
 */
void BoardLast5Exec()
{
	GWSystem.player_curr = -1;
	last5Proc = HuPrcChildCreate(ExecLast5, 8200, 14336, 0, boardMainProc);
	HuPrcDestructorSet2(last5Proc, DestroyLast5);
	while(last5Proc != NULL) {
		HuPrcVSleep();
	}
	GWSystem.player_curr = 0;
}

/**
 * @brief Releases final-five-turn event resources when its process ends.
 */
static void DestroyLast5(void)
{
	KillLast5();
	HuDataDirClose(DATADIR_BLAST5);
	last5Proc = NULL;
}

/**
 * @brief Sets up, presents, and tears down the final-five-turn event scene.
 */
static void ExecLast5(void)
{
	s32 i;
	messBase = messBaseTbl[GWBoardGet()];
	CreateLast5();
	WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
	while(WipeStatGet()) {
		HuPrcVSleep();
	}
	_ClearFlag(0x1001C);
	BoardLast5GfxInit();
	BoardAudSeqFadeOut(0, 1000);
	BoardMusStart(1, 8, 127, 0);
	BoardWinCreate(2, messBase, BoardWinPortraitGetStar());
	BoardWinWait();
	BoardWinKill();
	ExecLotteryDraw();
	Last5Main();
	BoardAudSeqFadeOut(1, 1000);
	_SetFlag(0x1001C);
	_SetFlag(0x1000E);
	WipeColorSet(255, 255, 255);
	WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
	while(WipeStatGet()) {
		HuPrcVSleep();
	}
	BoardModelMotionStart(hostMdl, 1, 0x40000001);
	BoardCameraMoveSet(0);
	BoardCameraViewSet(2);
	BoardCameraMotionWait();
	BoardCameraMoveSet(1);
	for(i=0; i<4; i++) {
		BoardModelVisibilitySet(BoardPlayerModelGet(i), 1);
		BoardPlayerCornerPosSet(i);
		BoardPlayerRotYSet(i, 0.0f);
	}
	BoardStatusItemSet(0);
	HuPrcSleep(60);
	HuPrcEnd();
}

/**
 * @brief Positions the host and creates the draw wheel and lottery-ticket displays.
 */
static void CreateLast5(void)
{
	Vec rot;
	s32 i;
	last5Space = BoardSpaceFlagPosGet(0, 0x80000000, &spacePos);
	for(i=0; i<4; i++) {
		BoardModelVisibilitySet(BoardPlayerModelGet(i), 0);
	}
	BoardStatusItemSet(0);
	focusPos.x = 150.0f+spacePos.x;
	focusPos.y = 100.0f+spacePos.y;
	focusPos.z = -150.0f+spacePos.z;
	focusMdl = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 10), NULL, 0);
	BoardModelVisibilitySet(focusMdl, 0);
	BoardModelPosSetV(focusMdl, &focusPos);
	rot.x = -10.0f;
	rot.y = 0.0f;
	rot.z = 0.0f;
	BoardCameraMoveSet(0);
	BoardCameraMotionStartEx(focusMdl, &rot, NULL, 1480.0f, 25.0f, 1);
	BoardCameraMotionWait();
	BoardCameraMoveSet(1);
	InitHost();
	CreateLotteryDrawWheel();
	InitLotteryTicket();
}

/**
 * @brief Stops the final-five-turn window, effects, host motions, wheel, and tickets.
 */
static void KillLast5(void)
{
	if(stopWin != -1) {
		HuWinKill(stopWin);
		stopWin = -1;
	}
	BoardConfettiKill();
	KillHost();
	KillLotteryDrawWheel();
	KillLotteryTicket();
}

/**
 * @brief Moves the star host into the event scene and loads its event motions.
 */
static void InitHost(void)
{
	Vec pos;
	s32 i;
	s32 board = GWBoardGet();
	hostMdl = BoardStarHostMdlGet();
	BoardModelPosGet(hostMdl, &hostPos);
	for(i=0; i<8; i++) {
		hostMot[i] = BoardModelMotionCreate(hostMdl, hostMotTbl[board][i]);
	}
	BoardModelMotionStart(hostMdl, 1, 0x40000001);
	pos.x = 150.0f+spacePos.x;
	pos.y = spacePos.y;
	pos.z = (150.0f+spacePos.z)-50.0f;
	BoardModelPosSetV(hostMdl, &pos);
}

/**
 * @brief Releases the host's event motions and restores its board transform.
 */
static void KillHost(void)
{
	s32 i;
	for(i=0; i<8; i++) {
		if(hostMot[i] != -1) {
			BoardModelMotionKill(hostMdl, hostMot[i]);
			hostMot[i] = -1;
		}
	}
	BoardModelRotYSet(hostMdl, 0.0f);
	BoardModelPosSetV(hostMdl, &hostPos);
}

/**
 * @brief Presents the standings and selects a last-five-turn rule by roulette.
 */
static void Last5Main(void)
{
	float jump_frame;
	s32 entry_index;
	s32 player;
	s32 frame;
	s32 rank;
	s32 ranked_count;
	s32 last_rank_count;
	s32 message;
	s32 character;
	s32 sound;
	s32 last_place_count;
	s32 roulette_result;
	s32 impact_played;
	s32 previous_rank;
	Vec status_positions[4];
	Vec host_start;
	Vec host_end;
	Vec player_position;
	Vec player_end;
	float position_steps[4];
	s8 players_by_rank[4];
	s8 player_ranks[4];
	s8 last_place_players[4];
	
	
	
	previous_rank = -1;
	for(ranked_count=rank=0; rank<=3; rank++) {
		for(last_rank_count=entry_index=0; entry_index<4; entry_index++) {
			if(GWPlayer[entry_index].rank == rank) {
				players_by_rank[ranked_count] = entry_index;
				player_ranks[ranked_count] = rank;
				ranked_count++;
				if(rank > previous_rank) {
					last_place_players[last_rank_count] = entry_index;
					last_rank_count++;
				}
			}
		}
		if(last_rank_count != 0) {
			last_place_count = last_rank_count;
		}
	}
	BoardWinCreate(2, messBase+10, BoardWinPortraitGetStar());
	BoardWinWait();
	BoardModelMotionShiftSet(hostMdl, hostMot[5], 0.0f, 4.0f, HU3D_MOTATTR_NONE);
	HuPrcVSleep(8);
	BoardModelMotionShiftSet(hostMdl, hostMot[6], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
	for(entry_index=0; entry_index<4; entry_index++) {
		player = players_by_rank[entry_index];
		rank = player_ranks[entry_index];
		character = GWPlayer[player].character;
		status_positions[entry_index].x = -122.0f;
		status_positions[entry_index].y = (entry_index*74)+80;
		status_positions[entry_index].z = 0.0f;
		BoardStatusPosSet(player, &status_positions[entry_index]);
		HuPrcVSleep();
		BoardStatusShowSet(player, 1);
		position_steps[entry_index] = (122.0f-status_positions[entry_index].x)/15.0f;
		for(frame=0; frame<15; frame++) {
			status_positions[entry_index].x += position_steps[entry_index];
			BoardStatusPosSet(player, &status_positions[entry_index]);
			HuPrcVSleep();
		}
		switch(rank) {
			case 0:
			case 1:
				sound = resultRankFX12Tbl[character];
				break;
				
			case 2:
				sound = resultRankFX3Tbl[character];
				break;
				
			case 3:
				sound = resultRankFX4Tbl[character];
				break;
		}
		HuAudFXPlay(sound);
		message = 11+messBase+rank;
		BoardWinCreate(2, message, BoardWinPortraitGetStar());
		BoardWinInsertMesSet(BoardPlayerGetCharMess(player), rank);
		BoardWinWait();
	}
	if((s32)GWSystem.team != 0) {
		CreateTeamResult();
		for(entry_index=0; entry_index<4; entry_index++) {
			position_steps[entry_index] = (-122.0f-status_positions[entry_index].x)/15.0f;
		}
		for(frame=0; frame<15; frame++) {
			for(entry_index=0; entry_index<4; entry_index++) {
				player = players_by_rank[entry_index];
				status_positions[entry_index].x += position_steps[entry_index];
				BoardStatusPosSet(player, &status_positions[entry_index]);
			}
			HuPrcVSleep();
		}
		BoardStatusItemSet(0);
		message = MAKE_MESSID(14, 144) + GWBoardGet();
		BoardWinCreate(2, message, BoardWinPortraitGetStar());
		BoardWinWait();
		for(entry_index=0; entry_index<2; entry_index++) {
			GetTeamResultTarget(entry_index, &status_positions[entry_index]);
			position_steps[entry_index] = (122.0f-status_positions[entry_index].x)/15.0f;
			for(frame=0; frame<15; frame++) {
				status_positions[entry_index].x += position_steps[entry_index];
				SetTeamResultTarget(entry_index, &status_positions[entry_index]);
				HuPrcVSleep();
			}
		}
	}
	BoardModelMotionStart(hostMdl, hostMot[5], 0x40000004);
	HuPrcVSleep(8);
	BoardModelMotionShiftSet(hostMdl, 1, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
	BoardWinCreate(2, messBase+15, BoardWinPortraitGetStar());
	BoardWinWait();
	if((s32)GWSystem.team != 0) {
		for(entry_index=0; entry_index<2; entry_index++) {
			position_steps[entry_index] = (-122.0f-status_positions[entry_index].x)/15.0f;
		}
		for(frame=0; frame<15; frame++) {
			for(entry_index=0; entry_index<2; entry_index++) {
				GetTeamResultTarget(entry_index, &status_positions[entry_index]);
				status_positions[entry_index].x += position_steps[entry_index];
				SetTeamResultTarget(entry_index, &status_positions[entry_index]);
			}
			HuPrcVSleep();
		}
		KillTeamResult();
	} else {
		for(entry_index=0; entry_index<4; entry_index++) {
			position_steps[entry_index] = (-122.0f-status_positions[entry_index].x)/15.0f;
		}
		for(frame=0; frame<15; frame++) {
			for(entry_index=0; entry_index<4; entry_index++) {
				player = players_by_rank[entry_index];
				status_positions[entry_index].x += position_steps[entry_index];
				BoardStatusPosSet(player, &status_positions[entry_index]);
			}
			HuPrcVSleep();
		}
		BoardStatusItemSet(0);
	}
	BoardModelPosGet(hostMdl, &host_start);
	host_end.x = host_start.x+100.0f;
	host_end.y = host_start.y;
	host_end.z = host_start.z;
	StartHostMove(&host_start, &host_end, 25);
	while(!CheckHostMove()) {
		HuPrcVSleep();
	}
	BoardModelMotionShiftSet(hostMdl, 1, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
	HuPrcSleep(8);
	if((s32)GWSystem.team != 0) {
		player = last5Player;
	} else {
		if(last_place_count == 1) {
			player = last_place_players[0];
		} else {
			player = last_place_players[BoardRandMod(last_place_count)];
		}
	}
	player_position.x = host_start.x-100.0f;
	player_position.y = host_start.y;
	player_position.z = host_start.z;
	player_position.y += 700.0f;
	BoardPlayerPosSetV(player, &player_position);
	BoardPlayerMotionStart(player, 6, 0x40000001);
	BoardPlayerVoiceEnableSet(player, 6, 0);
	BoardPlayerRotYSet(player, 0);
	BoardModelVisibilitySet(BoardPlayerModelGet(player), 1);
	BoardWinCreate(2, messBase+16, BoardWinPortraitGetStar());
	BoardWinInsertMesSet(BoardPlayerGetCharMess(player), 3);
	BoardWinWait();
	BoardModelMotionShiftSet(hostMdl, hostMot[5], 0.0f, 4.0f, HU3D_MOTATTR_NONE);
	HuPrcVSleep(8);
	BoardModelMotionShiftSet(hostMdl, hostMot[6], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
	impact_played = 0;
	frame = 3;
	for(jump_frame=0; frame>0; jump_frame++) {
		player_position.y += (float)(5.0+((-9.8/120)*jump_frame*jump_frame));
		if(host_start.y > player_position.y) {
			omVibrate(player, 12, 6, 6);
			if(!impact_played) {
				HuAudFXPlay(impactFXTbl[GWPlayer[player].character]);
				impact_played = 1;
			}
			player_position.y = host_start.y;
			player_position.y += frame*10;
			frame--;
			jump_frame = 0.0f;
		}
		BoardPlayerPosSetV(player, &player_position);
		HuPrcVSleep();
	}
	HuPrcSleep(20);
	BoardPlayerIdleSet(player);
	player_end = player_position;
	BoardModelMotionStart(hostMdl, hostMot[5], 0x40000004);
	HuPrcVSleep(8);
	BoardModelMotionShiftSet(hostMdl, 1, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
	CreateLast5Roulette();
	SetLast5RouletteFade(1);
	SetLast5RouletteState(2);
	CreateStopWin();
	player_position.x = host_start.x-100.0f;
	player_position.y = host_start.y;
	player_position.z = host_start.z;
	CreateLotteryDraw(&player_position);
	SetLotteryDrawState(2);
	while(GetLotteryDrawState() != 0) {
		HuPrcVSleep();
	}
	while(!CheckJump(player)) {
		HuPrcVSleep();
	}
	BoardPlayerDiceJumpStart(player);
	while(!BoardPlayerDiceJumpCheck(player)) {
		HuPrcVSleep();
	}
	SetLotteryDrawState(1);
	SetLast5RouletteState(3);
	while(GWPlayer[player].jump) {
		HuPrcVSleep();
	}
	while(GetLast5RouletteState() != 0) {
		HuPrcVSleep();
	}
	HuWinDispOff(stopWin);
	SetLotteryDrawState(3);
	roulette_result = GetLast5RouletteResult();
	GWSystem.last5_effect = roulette_result;
	if(GWSystem.last5_effect == 2) {
		BoardSpaceTypeForce(2, 3);
	}
	if(GWSystem.last5_effect == 3) {
		BoardSpaceTypeForce(2, 7);
	}
	message = 18+messBase+roulette_result;
	BoardWinCreate(2, message, BoardWinPortraitGetStar());
	BoardWinWait();
	StartHostMove(&host_end, &host_start, 25);
	BoardPlayerPosGet(player, &player_position);
	player_position = player_end;
	player_end.x = player_position.x-300.0f;
	player_end.y = player_position.y;
	player_end.z = player_position.z;
	BoardPlayerPosLerpStart(player, &player_position, &player_end, 25);
	while(GWPlayer[player].moving) {
		HuPrcVSleep();
	}
	BoardPlayerIdleSet(player);
	BoardModelVisibilitySet(BoardPlayerModelGet(player), 0);
	SetLast5RouletteFade(0);
	BoardModelMotionShiftSet(hostMdl, 1, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
	BoardWinCreate(2, messBase+23, BoardWinPortraitGetStar());
	BoardWinWait();
	BoardModelMotionShiftSet(hostMdl, hostMot[4], 0.0f, 8.0f, HU3D_MOTATTR_NONE);
	HuPrcSleep(8);
	while(!BoardModelMotionEndCheck(hostMdl)) {
		HuPrcVSleep();
	}
	BoardModelMotionShiftSet(hostMdl, hostMot[2], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
	SetLotteryDrawState(4);
}

/**
 * @brief Checks whether the selected player has requested a roulette-block jump.
 * @param player Player index whose CPU or controller input is checked.
 * @return Nonzero when the jump should begin; otherwise zero.
 */
static s32 CheckJump(s32 player)
{
	s32 unused = 0;
	if(GWPlayer[player].com) {
		if(BoardRandMod(100) >= 90) {
			return 1;
		}
	} else if(HuPadBtnDown[GWPlayer[player].port] == PAD_BUTTON_A) {
		return 1;
	}
	return 0;
}

typedef struct draw_wheel_mdl {
	s16 plate[4];
	s16 face[4];
	s16 center;
} DrawWheelMdl;

typedef struct draw_wheel_work {
	struct {
		u8 kill : 1;
	};
	s8 state;
	s8 hilite;
	u16 timer;
	u8 hilite_vel;
	u8 hilite_accel;
	DrawWheelMdl *mdl;
} DrawWheelWork;

typedef struct lottery_draw_work {
	struct {
		u8 kill : 1;
	};
	u8 state;
	s16 block_mdl;
	float min_y;
} LotteryDrawWork;

/**
 * @brief Creates the character wheel used to select the event participant.
 */
static void CreateLotteryDrawWheel(void)
{
	OMOBJ *wheel_object;
	DrawWheelWork *wheel_work;
	DrawWheelMdl *mdl;
	s32 i;
	wheel_object = omAddObjEx(boardObjMan, 257, 0, 0, -1, UpdateLotteryDrawWheel);
	lotteryDrawWheelObj = wheel_object;
	wheel_work = omObjGetWork(wheel_object, DrawWheelWork);
	wheel_work->kill = 0;
	wheel_work->state = 0;
	wheel_work->mdl = HuMemDirectMallocNum(HEAP_HEAP, sizeof(DrawWheelMdl), HU_MEMNUM_OVL);
	wheel_work->hilite = 0;
	wheel_work->hilite_vel = 2;
	wheel_work->hilite_accel = 5;
	mdl = wheel_work->mdl;
	wheel_object->trans.x = spacePos.x + -300.0f;
	wheel_object->trans.y = spacePos.y;
	wheel_object->trans.z = spacePos.z + -50.0f;
	wheelMdl = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BLAST5, 12), NULL, 0);
	BoardModelPosSet(wheelMdl, wheel_object->trans.x, wheel_object->trans.y, wheel_object->trans.z);
	mdl->center = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BLAST5, 22), NULL, 0);
	BoardModelVisibilitySet(mdl->center, 0);
	for(i=0; i<4; i++) {
		s32 character = GWPlayer[i].character;
		mdl->face[i] = BoardModelCreate(lotteryWheelMdlTbl[character], NULL, 0);
		BoardModelPosSet(mdl->face[i], 0, 0, -10);
		mdl->plate[i] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BLAST5, 21), NULL, 1);
		BoardModelHookSet(wheelMdl, faceObjNameTbl[i], mdl->face[i]);
		BoardModelHookSet(wheelMdl, plateObjNameTbl[i], mdl->plate[i]);
	}
}

/**
 * @brief Creates the falling roulette block at a board position.
 * @param pos Mutable starting position; its Y coordinate is raised by 700 world units.
 */
static void CreateLotteryDraw(Vec *pos)
{
	OMOBJ *block_object;
	LotteryDrawWork *block_work;
	block_object = omAddObjEx(boardObjMan, 257, 0, 0, -1, UpdateLotteryDraw);
	lotteryDrawObj = block_object;
	block_work = omObjGetWork(block_object, LotteryDrawWork);
	block_work->kill = 0;
	block_work->state = 0;
	block_work->block_mdl = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BLAST5, 11), NULL, 0);
	block_work->min_y = pos->y+300.0f;
	pos->y += 700.0f;
	BoardModelPosSetV(block_work->block_mdl, pos);
	block_object->trans.x = 1.0f;
	BoardModelAlphaSet(block_work->block_mdl, 255);
}

/**
 * @brief Advances or destroys the falling roulette block according to its state.
 * @param object Object that owns the falling roulette block state.
 */
static void UpdateLotteryDraw(OMOBJ *object)
{
	LotteryDrawWork *block_work = omObjGetWork(object, LotteryDrawWork);
	Vec pos;
	if(block_work->kill || BoardIsKill()) {
		BoardModelKill(block_work->block_mdl);
		lotteryDrawObj = NULL;
		omDelObjEx(HuPrcCurrentGet(), object);
		return;
	}
	switch(block_work->state) {
		case 1:
			BoardModelPosGet(block_work->block_mdl, &pos);
			pos.y += 50.0f;
			BoardModelPosSetV(block_work->block_mdl, &pos);
			SetLotteryDrawState(2);
			break;
			
		case 2:
			BoardModelPosGet(block_work->block_mdl, &pos);
			if(pos.y < block_work->min_y) {
				pos.y = block_work->min_y;
				SetLotteryDrawState(0);
			}
			pos.y -= 20.0f;
			BoardModelPosSetV(block_work->block_mdl, &pos);
			break;
			
		case 3:
			object->trans.x -= 0.1f;
			if(object->trans.x < 0.0f) {
				object->trans.x = 0;
				SetLotteryDrawState(0);
			}
			BoardModelAlphaSet(block_work->block_mdl, 255.0f*object->trans.x);
			break;
			
		case 0:
			break;
			
		default:
			break;
	}
}

/**
 * @brief Changes the falling roulette block's animation state.
 * @param state Requested block state; 1 raises, 2 drops, 3 fades, and 4 destroys it.
 */
static void SetLotteryDrawState(s32 state)
{
	LotteryDrawWork *block_work;
	if(!lotteryDrawObj) {
		return;
	}
	block_work = omObjGetWork(lotteryDrawObj, LotteryDrawWork);
	block_work->state = state;
	if(state == 4) {
		block_work->kill = 1;
	}
	if(state == 3) {
		lotteryDrawObj->trans.x = 1.0f;
	}
	if(state == 1) {
		HuAudFXPlay(780);
	}
}

/**
 * @brief Gets the falling roulette block's current animation state.
 * @return Current falling-block state. The block object must exist.
 */
static s32 GetLotteryDrawState(void)
{
	LotteryDrawWork *draw_work = omObjGetWork(lotteryDrawObj, LotteryDrawWork);
	return draw_work->state;
}

/**
 * @brief Marks the participant wheel and falling block for destruction.
 */
static void KillLotteryDrawWheel(void)
{
	SetLotteryDrawWheelState(5);
}

/**
 * @brief Animates the participant wheel highlight and applies its result.
 * @param object Object that owns the participant wheel models and state.
 */
static void UpdateLotteryDrawWheel(OMOBJ *object)
{
	DrawWheelWork *wheel_work = omObjGetWork(object, DrawWheelWork);
	DrawWheelMdl *mdl = wheel_work->mdl;
	s32 i;
	if(wheel_work->kill || BoardIsKill()) {
		BoardModelHookReset(wheelMdl);
		for(i=0; i<4; i++) {
			if(mdl->face[i] != -1) {
				BoardModelKill(mdl->face[i]);
			}
			if(mdl->plate[i] != -1) {
				BoardModelKill(mdl->plate[i]);
			}
		}
		if(wheelMdl != -1) {
			BoardModelKill(wheelMdl);
			wheelMdl = -1;
		}
		if(lbl_801D3854 != -1) {
			BoardModelKill(lbl_801D3854);
			lbl_801D3854 = -1;
		}
		if(mdl->center != -1) {
			BoardModelKill(mdl->center);
		}
		HuMemDirectFree(wheel_work->mdl);
		lotteryDrawWheelObj = NULL;
		omDelObjEx(HuPrcCurrentGet(), object);
		return;
	}
	switch(wheel_work->state) {
		case 4:
			if(wheel_work->timer == 0) {
				wheel_work->hilite_vel += wheel_work->hilite_accel;
				wheel_work->hilite_accel++;
				if(wheel_work->hilite_vel > 60 || (wheel_work->hilite_vel > 40 && BoardRandMod(100) > 80)) {
					BoardModelMotionStart(mdl->center, 0, 0x40000001);
					HuAudFXPlay(870);
					SetLotteryDrawWheelState(0);
					break;
				}
			}
			
		case 3:
			if(wheel_work->timer < wheel_work->hilite_vel) {
				wheel_work->timer++;
			} else {
				BoardModelHookObjReset(wheelMdl, hiliteObjNameTbl[wheel_work->hilite]);
				wheel_work->hilite = (wheel_work->hilite+1)  & 0x3;
				HuAudFXPlay(855);
				BoardModelHookSet(wheelMdl, hiliteObjNameTbl[wheel_work->hilite], mdl->center);
				wheel_work->timer = 0;
			}
			break;
		
		case 1:
		case 2:
			if(wheel_work->timer != 0) {
				object->trans.x += object->rot.x;
				object->trans.z += object->rot.z;
				wheel_work->timer--;
			} else {
				BoardModelMotionSpeedSet(wheelMdl, 0);
				wheel_work->state = 0;
			}
			break;
			
		case 0:
			break;
			
		default:
			break;
	}
	BoardModelPosSet(wheelMdl, object->trans.x, object->trans.y, object->trans.z);
}

/**
 * @brief Changes the participant wheel's animation state.
 * @param state Requested wheel state: 1 or 2 moves it, 3 starts highlighting, and 5 destroys it.
 */
static void SetLotteryDrawWheelState(s32 state)
{
	DrawWheelWork *wheel_work;
	DrawWheelMdl *mdl;
	Vec pos_wheel, pos_space, dist;
	if(!lotteryDrawWheelObj) {
		return;
	}
	wheel_work = omObjGetWork(lotteryDrawWheelObj, DrawWheelWork);
	
	wheel_work->state = state;
	mdl = wheel_work->mdl;
	switch(state) {
		case 5:
			wheel_work->kill = 1;
			break;
		
		case 3:
			wheel_work->timer = 0;
			wheel_work->hilite = 0;
			wheel_work->hilite_vel = 2;
			BoardModelVisibilitySet(mdl->center, 1);
			BoardModelMotionStart(mdl->center, 0, 0);
			BoardModelMotionSpeedSet(mdl->center, 0.0f);
			BoardModelMotionTimeSet(mdl->center, 3.0f);
			BoardModelHookSet(wheelMdl, hiliteObjNameTbl[wheel_work->hilite], mdl->center);
			break;
			
		default:
			if(state == 2 || state == 1) {
				BoardModelPosGet(wheelMdl, &pos_wheel);
				if(state == 2) {
					pos_space = spacePos;
				} else {
					pos_space.x = -300.0f+pos_wheel.x;
					pos_space.y = pos_wheel.y;
					pos_space.z = -50.0f+pos_wheel.z;
				}
				VECSubtract(&pos_space, &pos_wheel, &dist);
				lotteryDrawWheelObj->rot.x = dist.x/26.0f;
				lotteryDrawWheelObj->rot.z = dist.z/26.0f;
				BoardModelMotionStart(wheelMdl, 0, 0x40000001);
				BoardModelMotionSpeedSet(wheelMdl, 3.0f);
				wheel_work->timer = 26;
			}
			break;
	}
}

/**
 * @brief Gets the participant wheel's current animation state.
 * @return Current participant-wheel state, or -1 when no wheel exists.
 */
static s32 GetLotteryDrawWheelState()
{
	DrawWheelWork *wheel_work;
	if(!lotteryDrawWheelObj) {
		return -1;
	}
	wheel_work = omObjGetWork(lotteryDrawWheelObj, DrawWheelWork);
	return wheel_work->state;
}

/**
 * @brief Gets the character selected by the participant wheel.
 * @return Character identifier of the player under the highlighted wheel panel, or -1 when no wheel exists.
 */
static s32 GetLotteryDrawWheelResult()
{
	DrawWheelWork *wheel_work;
	if(!lotteryDrawWheelObj) {
		return -1;
	}
	wheel_work = omObjGetWork(lotteryDrawWheelObj, DrawWheelWork);
	return GWPlayer[wheel_work->hilite].character;
}

/**
 * @brief Runs the draw that chooses which player participates in the final-turn roulette.
 */
static void ExecLotteryDraw(void)
{
	s32 ticket;
	s32 valid;
	s32 character;
	s32 stream;
	s32 player;
	Vec rot;
	Vec offset;
	Vec confetti_pos;
	Vec pos;
	Vec star_pos;
	Vec host_end;
	Vec host_start;
	Vec player_end;
	Vec player_start;

	if(GWPlayer[0].draw_ticket || GWPlayer[1].draw_ticket || GWPlayer[2].draw_ticket || GWPlayer[3].draw_ticket) {
		BoardWinCreate(2, messBase+1, BoardWinPortraitGetStar());
		BoardWinWait();
		BoardModelPosGet(hostMdl, &host_end);
		250.0f+host_end.y;
		CreateLotteryDraw(&host_end);
		SetLotteryDrawState(2);
		SetLotteryDrawWheelState(2);
		for(ticket=0; ticket<4; ticket++) {
			SetLotteryTicketState(ticket, 3);
		}
		HuPrcSleep(15);
		while(GetLotteryDrawWheelState() != 0) {
			HuPrcVSleep();
		}
		offset.x = 50.0f;
		offset.z = 0.0f;
		offset.y = 100.0f;
		valid = 0;
		for(currTicket=ticket=0; ticket<3; ticket++, currTicket++) {
			BoardCameraBackup();
			SetLotteryDrawWheelState(3);
			BoardWinCreate(2, 2+messBase+ticket, BoardWinPortraitGetStar());
			BoardWinWait();
			BoardModelMotionShiftSet(hostMdl, hostMot[7], 0, 8, HU3D_MOTATTR_NONE);
			HuPrcSleep(8);
			while(BoardModelMotionTimeGet(hostMdl) < 26.0f) {
				HuPrcVSleep();
			}
			SetLotteryDrawState(1);
			HuPrcSleep(BoardRandMod(20));
			SetLotteryDrawWheelState(4);
			BoardCameraMotionStartEx(wheelMdl, NULL, &offset, 700, -1, 21);
			while(!BoardModelMotionEndCheck(hostMdl)) {
				HuPrcVSleep();
			}
			BoardModelMotionShiftSet(hostMdl, 1, 0, 8, HU3D_MOTATTR_LOOP);
			while(GetLotteryDrawWheelState() != 0) {
				HuPrcVSleep();
			}
			character = GetLotteryDrawWheelResult();
			UpdateLotteryTicketMatch(ticket, character);
			HuPrcSleep(120);
			if(CheckLotteryTicket()) {
				valid = 1;
				break;
			}
			if(ticket == 2) {
				player = GetLotteryTicketPlayer();
				BoardModelPosGet(hostMdl, &confetti_pos);
				confetti_pos.y += 400.0f;
				BoardConfettiCreate(&confetti_pos, 100, 200.0f);
				break;
			}
			BoardCameraRestore();
			BoardCameraMotionWait();
		}
		rot.x = -10.0f;
		rot.y = 0.0f;
		rot.z = 0.0f;
		BoardCameraMotionStartEx(focusMdl, &rot, NULL, 1480.0f, 25.0f, 20);
		if(valid) {
			BoardWinCreate(2, 9+messBase, BoardWinPortraitGetStar());
			BoardWinWait();
			SetLotteryDrawState(3);
			SetLotteryDrawWheelState(1);
			for(ticket=0; ticket<4; ticket++) {
				SetLotteryTicketState(ticket, 4);
			}
		} else {
			BoardAudSeqPause(1, 1, 1000);
			BoardModelMotionShiftSet(hostMdl, hostMot[3], 0, 8, HU3D_MOTATTR_LOOP);
			stream = HuAudSStreamPlay(10);
			while(HuAudSStreamStatGet(stream)) {
				HuPrcVSleep();
			}
			BoardAudSeqPause(1, 0, 1000);
			BoardWinCreate(2, 5+messBase, BoardWinPortraitGetStar());
			BoardWinWait();
			BoardCameraRestore();
			BoardCameraMotionWait();
			SetLotteryDrawState(3);
			SetLotteryDrawWheelState(1);
			for(ticket=0; ticket<4; ticket++) {
				SetLotteryTicketState(ticket, 4);
			}
			player_end.x = spacePos.x + -300.0f;
			player_end.y = spacePos.y;
			player_end.z = spacePos.z + -50.0f;
			BoardPlayerPosSetV(player, &player_end);
			BoardModelVisibilitySet(BoardPlayerModelGet(player), 1);
			BoardModelPosGet(hostMdl, &host_end);
			player_start = host_end;
			host_start.x = host_end.x+200.0f;
			host_start.y = host_end.y;
			host_start.z = host_end.z;
			StartHostMove(&host_end, &host_start, 25);
			BoardPlayerPosLerpStart(player, &player_end, &player_start, 25);
			while(GWPlayer[player].moving) {
				HuPrcVSleep();
			}
			BoardPlayerIdleSet(player);
			BoardModelMotionShiftSet(hostMdl, hostMot[0], 0, 8, HU3D_MOTATTR_LOOP);
			while(!CheckHostMove()) {
				HuPrcVSleep();
			}
			BoardModelMotionShiftSet(hostMdl, 1, 0, 8, HU3D_MOTATTR_LOOP);
			BoardStatusShowSetForce(player);
			BoardStatusShowSet(player, 1);
			BoardPlayerMotBlendSet(player, 0, 7);
			while(!BoardPlayerMotBlendCheck(player)) {
				HuPrcVSleep();
			}
			BoardWinCreate(2, 6+messBase, BoardWinPortraitGetStar());
			BoardWinWait();
			BoardAudSeqPause(1, 1, 1000);
			BoardModelMotionShiftSet(hostMdl, hostMot[1], 0, 8, HU3D_MOTATTR_NONE);
			star_pos.x = host_start.x;
			star_pos.y = 60.0f+host_start.y;
			star_pos.z = host_start.z;
			HuPrcSleep(30);
			BoardStarGive(player, &star_pos);
			BoardAudSeqPause(1, 0, 1000);
			BoardStatusShowSet(player, 0);
			StartHostMove(&host_start, &host_end, 25);
			BoardPlayerPosLerpStart(player, &player_start, &player_end, 25);
			BoardModelMotionShiftSet(hostMdl, hostMot[0], 0, 8, HU3D_MOTATTR_LOOP);
			while(!CheckHostMove()) {
				HuPrcVSleep();
			}
			BoardModelMotionShiftSet(hostMdl, 1, 0, 8, HU3D_MOTATTR_LOOP);
			while(GWPlayer[player].moving) {
				HuPrcVSleep();
			}
			BoardPlayerIdleSet(player);
			BoardModelVisibilitySet(BoardPlayerModelGet(player), 0);
			BoardConfettiStop();
			BoardModelVisibilitySet(wheelMdl, 0);
		}
		SetLotteryDrawState(4);
		BoardWinCreate(2, 7+messBase, BoardWinPortraitGetStar());
		BoardWinWait();
		BoardWinKill();
	}
}

/**
 * @brief Marks every displayed lottery ticket for destruction.
 */
static void KillLotteryTicket(void)
{
	s32 i;
	for(i=0; i<numTickets; i++) {
		SetLotteryTicketState(i, 5);
	}
}

typedef struct ticket_work {
	struct {
		u8 kill : 1;
		u8 player : 2;
		u8 done : 1;
		u8 ticket_index : 2;
	};
	s8 state;
	u8 write_only_field;
	s8 character[3];
	u16 angle;
	s16 group;
} TicketWork;

/**
 * @brief Creates ticket sprites for players carrying final-five lottery tickets.
 */
static void InitLotteryTicket(void)
{
	s32 i;
	s32 j;
	s32 player_sprite_table[] = {
		DATA_MAKE_NUM(DATADIR_BLAST5, 0),
		DATA_MAKE_NUM(DATADIR_BLAST5, 1),
		DATA_MAKE_NUM(DATADIR_BLAST5, 2),
		DATA_MAKE_NUM(DATADIR_BLAST5, 3),
		DATA_MAKE_NUM(DATADIR_BLAST5, 4),
		DATA_MAKE_NUM(DATADIR_BLAST5, 5),
		DATA_MAKE_NUM(DATADIR_BLAST5, 6),
		DATA_MAKE_NUM(DATADIR_BLAST5, 7),
	};
	s32 owner_sprite_table[] = {
		DATA_MAKE_NUM(DATADIR_BOARD, 46),
		DATA_MAKE_NUM(DATADIR_BOARD, 47),
		DATA_MAKE_NUM(DATADIR_BOARD, 48),
		DATA_MAKE_NUM(DATADIR_BOARD, 49),
		DATA_MAKE_NUM(DATADIR_BOARD, 50),
		DATA_MAKE_NUM(DATADIR_BOARD, 51),
		DATA_MAKE_NUM(DATADIR_BOARD, 52),
		DATA_MAKE_NUM(DATADIR_BOARD, 53),
	};
	s32 character;
	s16 sprite;
	s32 member;
	u8 ticket_player;
	OMOBJ *ticket_object;
	TicketWork *ticket_work;
	currTicket = 0;
	lotteryTicketObj[0] = lotteryTicketObj[1] = lotteryTicketObj[2] = lotteryTicketObj[3] = NULL;
	for(numTickets=i=0; i<4; i++) {
		if(!GWPlayer[i].draw_ticket) {
			continue;
		}
		ticket_object = omAddObjEx(boardObjMan, 257, 0, 0, -1, UpdateLotteryTicket);
		lotteryTicketObj[numTickets] = ticket_object;
		ticket_work = omObjGetWork(ticket_object, TicketWork);
		ticket_work->kill = 0;
		ticket_work->player = i;
		ticket_work->ticket_index = numTickets;
		ticket_work->angle = 0;
		ticket_work->write_only_field = 0;
		ticket_work->done = 0;
		ticket_work->group = HuSprGrpCreate(6);
		ticket_object->trans.x = 680.0f;
		ticket_object->trans.y = 64.0f+(82.0f*numTickets);
		ticket_object->rot.x = 0.0f;
		ticket_object->rot.y = 0.0f;
		HuSprGrpPosSet(ticket_work->group, ticket_object->trans.x, ticket_object->trans.y);
		HuSprGrpScaleSet(ticket_work->group, 0.4f, 0.4f);
		BoardSpriteCreate(DATA_MAKE_NUM(DATADIR_BLAST5, 9), 2000, NULL, &sprite);
		HuSprGrpMemberSet(ticket_work->group, 0, sprite);
		HuSprAttrSet(ticket_work->group, 0, HUSPR_ATTR_LINEAR);
		HuSprScaleSet(ticket_work->group, 0, 2.5f, 2.5f);
		character = GWPlayer[i].character;
		BoardSpriteCreate(owner_sprite_table[character], 1200, NULL, &sprite);
		HuSprGrpMemberSet(ticket_work->group, 5, sprite);
		HuSprAttrSet(ticket_work->group, 5, HUSPR_ATTR_LINEAR);
		HuSprScaleSet(ticket_work->group, 5, 2.5f, 2.5f);
		HuSprPosSet(ticket_work->group, 5, -200, -32);
		HuSprScaleSet(ticket_work->group, 5, 1.75f, 1.75f);
		BoardSpriteCreate(DATA_MAKE_NUM(DATADIR_BLAST5, 8), 1200, NULL, &sprite);
		HuSprGrpMemberSet(ticket_work->group, 4, sprite);
		HuSprAttrSet(ticket_work->group, 4, HUSPR_ATTR_LINEAR);
		HuSprAttrSet(ticket_work->group, 4, HUSPR_ATTR_DISPOFF);
		HuSprAttrSet(ticket_work->group, 4, HUSPR_ATTR_ADDCOL);
		HuSprScaleSet(ticket_work->group, 4, 1.3f, 1.3f);
		ticket_player = GWPlayer[i].ticket_player;
		j=3;
		while(j>=0) {
			s32 player_spr;
			
			j--;
			ticket_work->character[j] = GWPlayer[ticket_player & 0x3].character;
			player_spr = player_sprite_table[ticket_work->character[j]];
			member = j+1;
			BoardSpriteCreate(player_spr, 1500, NULL, &sprite);
			HuSprGrpMemberSet(ticket_work->group, member, sprite);
			HuSprAttrSet(ticket_work->group, member, HUSPR_ATTR_LINEAR);
			HuSprPosSet(ticket_work->group, member, playerOfsTbl[j][0], playerOfsTbl[j][1]);
			ticket_player >>= 2;
		}
		numTickets++;
	}
}

/**
 * @brief Animates or destroys one final-five lottery-ticket display.
 * @param object Object that owns one displayed ticket and its sprites.
 */
static void UpdateLotteryTicket(OMOBJ *object)
{
	TicketWork *ticket_work = omObjGetWork(object, TicketWork);
	if(ticket_work->kill || BoardIsKill()) {
		HuSprGrpKill(ticket_work->group);
		omDelObjEx(HuPrcCurrentGet(), object);
		lotteryTicketObj[ticket_work->ticket_index] = NULL;
		return;
	}
	switch(ticket_work->state) {
		case 2:
			if(ticket_work->angle < 540) {
				float scale = 0.4f+(0.1f*sind((float)(ticket_work->angle%180)));
				HuSprGrpScaleSet(ticket_work->group, scale, scale);
				ticket_work->angle += 9;
			} else {
				ticket_work->angle = 0;
				ticket_work->state = 0;
				HuSprGrpScaleSet(ticket_work->group, 0.4f, 0.4f);
			}
			break;
			
		case 1:
			if(ticket_work->angle > 96) {
				u16 color = ticket_work->angle;
				HuSprColorSet(ticket_work->group, 0, color, color, color);
				HuSprColorSet(ticket_work->group, 1, color, color, color);
				HuSprColorSet(ticket_work->group, 2, color, color, color);
				HuSprColorSet(ticket_work->group, 3, color, color, color);
				HuSprColorSet(ticket_work->group, 5, color, color, color);
				ticket_work->angle -= 3;
			} else {
				ticket_work->angle = 0;
				ticket_work->state = 0;
			}
			break;
			
		case 3:
		case 4:
			if(ticket_work->angle != 0) {
				object->trans.x += object->rot.x;
				
				HuSprGrpPosSet(ticket_work->group, object->trans.x, object->trans.y);
				ticket_work->angle--;
			} else {
				if(ticket_work->state == 4) {
					BoardModelVisibilitySet(wheelMdl, 0);
				}
				SetLotteryTicketState(ticket_work->player, 0);
			}
			break;
			
		case 0:
			break;
		
		default:
			break;
			
			
	}
}

/**
 * @brief Changes one displayed lottery ticket's animation state.
 * @param player Board-player index whose displayed ticket should change state.
 * @param state Requested ticket animation state.
 */
static void SetLotteryTicketState(s32 player, s32 state)
{
	TicketWork *ticket_work;
	OMOBJ *ticket_object;
	s32 i;
	for(i=0; i<numTickets; i++) {
		ticket_object = lotteryTicketObj[i];
		if(!ticket_object) {
			continue;
		}
		ticket_work = omObjGetWork(ticket_object, TicketWork);
		if(ticket_work->player == player) {
			ticket_work->state = state;
			ticket_work->angle = 0;
			switch(state) {
				case 2:
					HuSprAttrReset(ticket_work->group, 4, HUSPR_ATTR_DISPOFF);
					HuSprPosSet(ticket_work->group, 4, playerOfsTbl[currTicket][0], playerOfsTbl[currTicket][1]);
					break;
					
				case 1:
					ticket_work->angle = 255;
					ticket_work->done = 1;
					HuSprAttrSet(ticket_work->group, 4, HUSPR_ATTR_DISPOFF);
					break;
					
				case 3:
					ticket_object->rot.x = 472.0f-ticket_object->trans.x;
					ticket_object->rot.x /= 15.0f;
					ticket_work->angle = 15;
					break;
					
				case 4:
					ticket_object->rot.x = 680.0f-ticket_object->trans.x;
					ticket_object->rot.x /= 15.0f;
					ticket_work->angle = 15;
					break;
					
				case 5:
					ticket_work->kill = 1;
					
				default:
					break;
			}
			return;
		}
		
	}
}

/**
 * @brief Gets the player represented by the currently selected ticket.
 * @return Board player index represented by the selected ticket.
 */
static s32 GetLotteryTicketPlayer(void)
{
	s32 i;
	for(i=0; i<numTickets; i++) {
		OMOBJ *ticket_object = lotteryTicketObj[i];
		TicketWork *ticket_work = omObjGetWork(ticket_object, TicketWork);
		if(!ticket_work->done) {
			return ticket_work->player;
		}
	}
	return -1;
}

/**
 * @brief Checks whether the ticket wheel has stopped on a matching ticket.
 * @return Nonzero when the selected ticket matches all required portraits; otherwise zero.
 */
static s32 CheckLotteryTicket(void)
{
	TicketWork *ticket_work;
	OMOBJ *ticket_object;
	
	s32 i;
	for(i=0; i<numTickets; i++) {
		ticket_object = lotteryTicketObj[i];
		ticket_work = omObjGetWork(ticket_object, TicketWork);
		if(!ticket_work->done) {
			return 0;
		}
	}
	return 1;
}

/**
 * @brief Reveals matching character portraits on the selected ticket.
 * @param progress Number of ticket portrait positions to reveal.
 * @param character Character identifier to compare with each ticket portrait.
 */
static void UpdateLotteryTicketMatch(s32 progress, s32 character)
{
	s32 i;
	TicketWork *ticket_work;
	OMOBJ *ticket_object;
	s32 match_state;
	
	for(i=0; i<numTickets; i++) {
		ticket_object = lotteryTicketObj[i];
		ticket_work = omObjGetWork(ticket_object, TicketWork);
		if(!ticket_work->done) {
			if(ticket_work->character[progress] == character) {
				match_state = 2;
			} else {
				match_state = 1;
			}
			SetLotteryTicketState(ticket_work->player, match_state);
		}
	}
}

typedef struct host_move_work {
	struct {
		u8 kill : 1;
		u8 finish : 1;
	};
	u8 state;
	s16 time;
	s16 angle;
	s16 angle_end;
} HostMoveWork;

/**
 * @brief Starts a timed linear movement of the final-five-turn host.
 * @param from Host movement start position in world coordinates.
 * @param to Host movement destination in world coordinates.
 * @param time Movement duration in video frames.
 */
static void StartHostMove(Vec *from, Vec *to, s16 time)
{
	HostMoveWork *move_work;
	OMOBJ *move_object;
	float duration;
	float angle;
	if(time <= 0) {
		time = 1;
	}
	move_object = omAddObjEx(boardObjMan, 257, 0, 0, -1, ExecHostMove);
	hostMoveObj = move_object;
	move_work = omObjGetWork(move_object, HostMoveWork);
	move_work->kill = 0;
	move_work->finish = 0;
	move_work->time = time;
	move_work->state = 0;
	OSs16tof32(&time, &duration);
	omSetTra(move_object, from->x, from->y, from->z);
	omSetRot(move_object, (to->x-from->x)/duration, 0.0f, (to->z-from->z)/duration);
	omSetSca(move_object, to->x, to->y, to->z);
	angle = atan2d(move_object->rot.x, move_object->rot.z);
	move_work->angle = 0;
	OSf32tos16(&angle, &move_work->angle_end);
	BoardModelMotionShiftSet(hostMdl, hostMot[0], 0, 8, HU3D_MOTATTR_LOOP);
	BoardModelMotionSpeedSet(hostMdl, 3.0f);
}

/**
 * @brief Updates the host's linear movement and deletes it when complete.
 * @param object Object that stores the host movement endpoints and timer.
 */
static void ExecHostMove(OMOBJ *object)
{
	HostMoveWork *move_work = omObjGetWork(object, HostMoveWork);
	float angle;
	float angle_end;
	if(move_work->kill || BoardIsKill()) {
		hostMoveObj = NULL;
		omDelObjEx(HuPrcCurrentGet(), object);
		return;
	}
	OSs16tof32(&move_work->angle, &angle);
	OSs16tof32(&move_work->angle_end, &angle_end);
	switch(move_work->state) {
		case 0:
			if(BoardDAngleCalcRange(&angle, angle_end, 10.0f)) {
				if(move_work->finish) {
					move_work->kill = 1;
				} else {
					move_work->state = 1;
				}
				BoardModelMotionSpeedSet(hostMdl, 1.0f);
			}
			OSf32tos16(&angle, &move_work->angle);
			BoardModelRotYSet(hostMdl, angle);
			break;
			
		case 1:
			if(move_work->time) {
				move_work->time--;
				object->trans.x += object->rot.x;
				object->trans.y += object->rot.y;
				object->trans.z += object->rot.z;
			} else {
				object->trans.x = object->scale.x;
				object->trans.y = object->scale.y;
				object->trans.z = object->scale.z;
				move_work->angle_end = 0;
				move_work->finish = 1;
				move_work->state = 0;
				BoardModelMotionSpeedSet(hostMdl, 3.0f);
			}
			break;
			
		default:
			break;
	}
	BoardModelPosSet(hostMdl, object->trans.x, object->trans.y, object->trans.z);
}

/**
 * @brief Checks whether the host movement object has finished.
 * @return Nonzero when no host movement object remains; otherwise zero.
 */
static s32 CheckHostMove(void)
{
	if(hostMoveObj) {
		return 0;
	} else {
		return 1;
	}
}

/**
 * @brief Creates the on-screen prompt for stopping the roulette.
 */
static void CreateStopWin(void)
{
	s32 mess = MAKE_MESSID(0x12, 0x11);
	float size[2];
	float pos_x, pos_y;
	HuWinMesMaxSizeGet(1, size, mess);
	pos_x = -10000;
	pos_y = 352;
	stopWin = HuWinCreate(pos_x, pos_y, size[0], size[1], 0);
	HuWinBGTPLvlSet(stopWin, 0.0f);
	HuWinMesSpeedSet(stopWin, 0);
	HuWinMesSet(stopWin, mess);
}

typedef struct last5_roulette_work {
	struct {
		u8 kill : 1;
		u8 state : 3;
	};
	s8 choice;
	s8 fade_speed;
	s8 choices[4];
	s16 alpha;
	u16 switch_timer;
	u8 switch_vel;
	u8 switch_accel;
	s16 roulette_model;
} Last5RouletteWork;

/**
 * @brief Creates the roulette that selects the final-five-turn rule.
 */
static void CreateLast5Roulette(void)
{
	Last5RouletteWork *roulette_work;
	OMOBJ *roulette_object;
	s32 choice1, choice2;
	Vec pos;
	roulette_object = omAddObjEx(boardObjMan, 257, 0, 0, -1, UpdateLast5Roulette);
	last5RouletteObj = roulette_object;
	roulette_work = omObjGetWork(roulette_object, Last5RouletteWork);
	roulette_work->kill = 0;
	roulette_work->choice = 0;
	roulette_work->switch_timer = 0;
	roulette_work->switch_vel = 4;
	roulette_work->state = 2;
	roulette_work->alpha = 0;
	roulette_work->fade_speed = 0;
	roulette_work->switch_accel = 5;
	choice1 = BoardRandMod(3)+2;
	do {
		choice2 = BoardRandMod(3)+2;
	} while(choice1 == choice2);
	roulette_work->choices[0] = 1;
	roulette_work->choices[1] = 1;
	roulette_work->choices[2] = choice1;
	roulette_work->choices[3] = choice2;
	roulette_work->roulette_model = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BLAST5, 10), NULL, 0);
	pos.x = 150.0f+spacePos.x;
	pos.y = 177.0f+spacePos.y;
	pos.z = -135.0f+spacePos.z;
	BoardModelPosSetV(roulette_work->roulette_model, &pos);
	BoardModelMotionStart(roulette_work->roulette_model, 0, 0);
	BoardModelMotionSpeedSet(roulette_work->roulette_model, 0);
}

/**
 * @brief Animates the final-rule roulette and records the stopped result.
 * @param object Object that owns the final-rule roulette sprites and state.
 */
static void UpdateLast5Roulette(OMOBJ *object)
{
	Last5RouletteWork *roulette_work = omObjGetWork(object, Last5RouletteWork);
	if(roulette_work->kill || BoardIsKill()) {
		BoardModelHookReset(wheelMdl);
		if(roulette_work->roulette_model != -1) {
			BoardModelKill(roulette_work->roulette_model);
		}
		last5RouletteObj = NULL;
		omDelObjEx(HuPrcCurrentGet(), object);
		return;
	}
	
	switch(roulette_work->state) {
		case 3:
			if(roulette_work->switch_timer == 0) {
				roulette_work->switch_vel += roulette_work->switch_accel;
				roulette_work->switch_accel++;
				if(roulette_work->switch_vel > 60 || (roulette_work->switch_vel > 40 && BoardRandMod(100) > 80)) {
					if((GWBoardGet() == BOARD_ID_MAIN1 || GWBoardGet() == BOARD_ID_MAIN3) && roulette_work->choices[roulette_work->choice] == 2) {
						roulette_work->switch_timer++;
						return;
					} else {
						HuAudFXPlay(870);
						SetLast5RouletteState(0);
						break;
					}
				}
			}
			
		case 2:
			if(roulette_work->switch_timer < roulette_work->switch_vel) {
				roulette_work->switch_timer++;
			} else {
				s32 choice = roulette_work->choices[roulette_work->choice];
				float time;
				do {
					roulette_work->choice = BoardRandMod(4);
				} while(roulette_work->choices[roulette_work->choice] == choice);
				OSs8tof32(&roulette_work->choices[roulette_work->choice], &time);
				HuAudFXPlay(855);
				BoardModelMotionTimeSet(roulette_work->roulette_model, 0.1f+time);
				roulette_work->switch_timer = 0;
			}
			break;
			
		case 0:
			break;
			
		default:
			break;
	}
	if(roulette_work->fade_speed) {
		roulette_work->alpha += roulette_work->fade_speed;
		if(roulette_work->alpha >= 255) {
			roulette_work->alpha = 255;
			roulette_work->fade_speed = 0;
		}
		if(roulette_work->alpha < 0) {
			roulette_work->alpha = 0;
			roulette_work->fade_speed = 0;
		}
		BoardModelAlphaSet(roulette_work->roulette_model, roulette_work->alpha);
	}
}

/**
 * @brief Enables or disables fading of unselected roulette entries.
 * @param flag Nonzero to fade unselected entries; zero to restore full opacity.
 */
static void SetLast5RouletteFade(s32 flag)
{
	Last5RouletteWork *roulette_work;
	if(!last5RouletteObj) {
		return;
	}
	roulette_work = omObjGetWork(last5RouletteObj, Last5RouletteWork);
	if(flag) {
		roulette_work->fade_speed = 4;
	} else {
		roulette_work->fade_speed = -4;
	}
}

/**
 * @brief Changes the final-rule roulette's animation state.
 * @param state Requested roulette state; 2 starts spinning and 3 begins stopping.
 */
static void SetLast5RouletteState(s32 state)
{
	Last5RouletteWork *roulette_work;
	if(!last5RouletteObj) {
		return;
	}
	roulette_work = omObjGetWork(last5RouletteObj, Last5RouletteWork);
	roulette_work->state = state;
}

/**
 * @brief Gets the final-rule roulette's current animation state.
 * @return Current roulette state, or -1 when no roulette exists.
 */
static s32 GetLast5RouletteState(void)
{
	Last5RouletteWork *roulette_work;
	if(!last5RouletteObj) {
		return -1;
	}
	roulette_work = omObjGetWork(last5RouletteObj, Last5RouletteWork);
	return roulette_work->state;
}

/**
 * @brief Gets the rule selected by the final-rule roulette.
 * @return Selected last-five-turn rule index.
 */
static s32 GetLast5RouletteResult(void)
{
	Last5RouletteWork *roulette_work;
	if(!last5RouletteObj) {
		return -1;
	}
	roulette_work = omObjGetWork(last5RouletteObj, Last5RouletteWork);
	return roulette_work->choices[roulette_work->choice];
}

typedef struct team_result_work {
	struct {
		u8 kill : 1;
	};
	u8 delay;
	u8 player[2][2];
	s16 group[2];
} TeamResultWork;

static void CreateTeamResultWork(TeamResultWork *work);

/**
 * @brief Creates the two team-standing sprite groups.
 */
static void CreateTeamResult(void)
{
	TeamResultWork *team_work;
	OMOBJ *team_object;
	s32 i;
	s32 j;
	s32 coins[2];
	s32 stars[2];
	u32 score[2];
	s32 team, player;
	s32 rank1, rank2;
	team_object = omAddObjEx(boardObjMan, 268, 0, 0, -1, UpdateTeamResult);
	teamResultObj = team_object;
	team_work = omObjGetWork(team_object, TeamResultWork);
	CreateTeamResultWork(team_work);
	coins[0] = coins[1] = stars[0] = stars[1] = 0;
	for(i=0; i<2; i++) {
		for(j=0; j<2; j++) {
			coins[i] += BoardPlayerCoinsGet(team_work->player[i][j]);
			stars[i] += GWStarsGet(team_work->player[i][j]);
		}
	}
	for(i=0; i<2; i++) {
		score[i] = coins[i]+(stars[i] << 11);
	}
	team = BoardRandMod(2);
	player = BoardRandMod(2);
	if(score[0] >= score[1]) {
		team_object->trans.x = -108.0f;
		team_object->trans.y = 152.0f;
		team_object->rot.x = -108.0f;
		team_object->rot.y = 302.0f;
		if(score[0] == score[1]) {
			last5Player = team_work->player[team][player];
		} else {
			rank1 = GWPlayer[team_work->player[1][0]].rank;
			rank2 = GWPlayer[team_work->player[1][1]].rank;
			if(rank1 == rank2) {
				last5Player = team_work->player[1][player];
			} else {
				if(rank1 < rank2) {
					last5Player = team_work->player[1][1];
				} else {
					last5Player = team_work->player[1][0];
				}
			}
		}
	} else {
		team_object->rot.x = -108.0f;
		team_object->rot.y = 152.0f;
		team_object->trans.x = -108.0f;
		team_object->trans.y = 302.0f;
		rank1 = GWPlayer[team_work->player[0][0]].rank;
		rank2 = GWPlayer[team_work->player[0][1]].rank;
		if(rank1 == rank2) {
			last5Player = team_work->player[0][player];
		} else {
			if(rank1 < rank2) {
				last5Player = team_work->player[0][1];
			} else {
				last5Player = team_work->player[0][0];
			}
		}
	}
}

/**
 * @brief Populates one team-standing sprite group with its player portraits.
 * @param work Team-result work whose two sprite groups are populated.
 */
static void CreateTeamResultWork(TeamResultWork *work)
{
	GXColor color = { 128, 128, 128, 0 };
	s32 i;
	s32 character;
	s16 group;
	s32 j;
	s32 team1_idx, team2_idx;
	s16 sprite;
	s32 spr_file;
	s16 prio;
	float x, y;
	for(team1_idx=team2_idx=i=0; i<4; i++) {
		s32 team = GWPlayer[i].team;
		if(team == 0) {
			work->player[0][team1_idx] = i;
			team1_idx++;
		} else {
			work->player[1][team2_idx] = i;
			team2_idx++;
		}
	}
	for(j=0; j<2; j++) {
		group = HuSprGrpCreate(15);
		work->group[j] = group;
		for(i=0; i<13; i++) {
			spr_file = teamSprTbl[i];
			prio = teamSprPrioTbl[i];
			
			BoardSpriteCreate(spr_file, prio, NULL, &sprite);
			HuSprGrpMemberSet(group, i, sprite);
			HuSprAttrSet(group, i, HUSPR_ATTR_NOANIM);
			HuSprAttrSet(group, i, HUSPR_ATTR_LINEAR);
			HuSprPosSet(group, i, teamSprPosTbl[i][0], teamSprPosTbl[i][1]);
		}
		HuSprTPLvlSet(group, 0, 0.7f);
		HuSprScaleSet(group, 0, 1.0f, 2.5f);
		HuSprColorSet(group, 0, color.r, color.g, color.b);
		if(!GWPlayer[work->player[j][0]].com) {
			HuSprAttrSet(group, 11, HUSPR_ATTR_DISPOFF);
		}
		if(!GWPlayer[work->player[j][1]].com) {
			HuSprAttrSet(group, 12, HUSPR_ATTR_DISPOFF);
		}
		if(j == 0) {
			HuSprBankSet(group, 1, 2);
		} else {
			HuSprBankSet(group, 1, 1);
		}
		for(i=0; i<2; i++) {
			character = GWPlayer[work->player[j][i]].character;
			BoardSpriteCreate(teamCharSprTbl[character], 1500, NULL, &sprite);
			HuSprGrpMemberSet(group, i+13, sprite);
			HuSprAttrSet(group, i+13, HUSPR_ATTR_LINEAR);
			x = teamSprPosTbl[i+13][0];
			y = teamSprPosTbl[i+13][1];
			HuSprPosSet(group, i+13, x, y);
		}
	}
}

/**
 * @brief Sets a team-standing group's screen position.
 * @param team Team index, either 0 or 1.
 * @param pos Screen position applied to the team's sprite group.
 */
static void SetTeamResultTarget(s32 team, Vec *pos)
{
	if(!teamResultObj || !pos) {
		return;
	}
	if(team) {
		teamResultObj->rot.x = pos->x;
		teamResultObj->rot.y = pos->y;
	} else {
		teamResultObj->trans.x = pos->x;
		teamResultObj->trans.y = pos->y;
	}
}

/**
 * @brief Gets a team-standing group's screen position.
 * @param team Team index, either 0 or 1.
 * @param pos Receives the sprite group's screen position.
 */
static void GetTeamResultTarget(s32 team, Vec *pos)
{
	if(!teamResultObj || !pos) {
		return;
	}
	if(team) {
		pos->x = teamResultObj->rot.x;
		pos->y = teamResultObj->rot.y;
	} else {
		pos->x = teamResultObj->trans.x;
		pos->y = teamResultObj->trans.y;
	}
}

/**
 * @brief Marks the team-standing display for destruction.
 */
static void KillTeamResult(void)
{
	TeamResultWork *team_work;
	if(!teamResultObj) {
		return;
	}
	team_work = omObjGetWork(teamResultObj, TeamResultWork);
	team_work->kill = 1;
}

/**
 * @brief Destroys the team-standing display when requested or the board exits.
 * @param object Object controlling the two team-standing sprite groups.
 */
static void UpdateTeamResult(OMOBJ *object)
{
	TeamResultWork *team_work;
	s32 i, j;
	team_work = omObjGetWork(object, TeamResultWork);
	if(team_work->kill || BoardIsKill()) {
		HuSprGrpKill(team_work->group[0]);
		HuSprGrpKill(team_work->group[1]);
		teamResultObj = NULL;
		omDelObjEx(HuPrcCurrentGet(), object);
		return;
	}
	if(team_work->delay) {
		team_work->delay--;
		return;
	}
	{
		s8 rank[2];
		u32 score[2];
		s32 coins[2];
		s32 stars[2];
		
		
		
		coins[0] = coins[1] = stars[0] = stars[1] = 0;
		for(i=0; i<2; i++) {
			for(j=0; j<2; j++) {
				coins[i] += BoardPlayerCoinsGet(team_work->player[i][j]);
				stars[i] += GWStarsGet(team_work->player[i][j]);
			}
		}
		for(i=0; i<2; i++) {
			s32 star_val;
			s32 coin_val;
			
			score[i] = coins[i]+(stars[i] << 11);
			coin_val = coins[i];
			if(coin_val > 999) {
				coin_val = 999;
			}
			star_val = stars[i];
			if(star_val > 999) {
				star_val = 999;
			}
			BoardSpriteDigitUpdate(team_work->group[i], 3, coin_val);
			BoardSpriteDigitUpdate(team_work->group[i], 7, star_val);
		}
		if(score[0] == score[1]) {
			rank[0] = rank[1] = 0;
		} else if(score[0] > score[1]) {
			rank[0] = 0;
			rank[1] = 1;
		} else {
			rank[1] = 0;
			rank[0] = 1;
		}
		HuSprBankSet(team_work->group[0], 2, rank[0]);
		HuSprBankSet(team_work->group[1], 2, rank[1]);
		HuSprGrpPosSet(team_work->group[0], object->trans.x, object->trans.y);
		HuSprGrpPosSet(team_work->group[1], object->rot.x, object->rot.y);
	}
}
