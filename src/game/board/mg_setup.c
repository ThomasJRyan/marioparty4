#include "types.h"
#include "game/board/mg_setup.h"
#include "game/audio.h"
#include "game/objsub.h"
#include "game/wipe.h"
#include "game/flag.h"
#include "game/window.h"
#include "game/board/main.h"
#include "game/board/audio.h"
#include "game/board/player.h"
#include "game/board/space.h"
#include "game/board/ui.h"
#include "game/board/tutorial.h"
#include "game/board/window.h"

#include "ext_math.h"
#include <string.h>

// structs
typedef struct ActiveMinigame {
    s16 window_id;
    s8  minigame_index;
    s8  is_available;
    s32 name_mess;
} ActiveMinigame;

typedef struct MGSetupSprites {
    s16 handles[13];
    Vec positions[2][13];
} MGSetupSprites;

typedef struct MGSetupWork {
    struct {
        u8 field00_bit0 : 1;
        u8 field00_bit1 : 3;
        u8 field00_bit4 : 2;
        u8 field00_bit6 : 1;
        u8 field00_bit7 : 1;
    };
    struct {
        u8 field01_bit0 : 4;
    };
    u8 phase;
    s8 timer;
    s16 angle;
    u8 delay;
    u8 spin_delay;
    f32 spin_speed;
    MGSetupSprites* sprites;
} MGSetupWork;

// bss
static ActiveMinigame activeMG[4];
static s16 mgListAll[0x20];
static s16 mgPlay4P[8];

// data
static s32 mgSetupSprTbl[13] = {
    DATA_MAKE_NUM(DATADIR_BOARD, 0x4B), DATA_MAKE_NUM(DATADIR_BOARD, 0x4C), DATA_MAKE_NUM(DATADIR_BOARD, 0x4D), DATA_MAKE_NUM(DATADIR_BOARD, 0x4E),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x4F), DATA_MAKE_NUM(DATADIR_BOARD, 0x52), DATA_MAKE_NUM(DATADIR_BOARD, 0x50), DATA_MAKE_NUM(DATADIR_BOARD, 0x51),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x53), DATA_MAKE_NUM(DATADIR_BOARD, 0x54), DATA_MAKE_NUM(DATADIR_BOARD, 0x55), DATA_MAKE_NUM(DATADIR_BOARD, 0x68),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x56)
};
static Vec statusLayout4P[4] = {
    { 170.0f, 244.0f, 0.0f }, 
    { 406.0f, 244.0f, 0.0f },
    { 170.0f, 364.0f, 0.0f },
    { 406.0f, 364.0f, 0.0f },
};
static Vec statusLayout1Vs3[4] = {
    { 170.0f, 304.0f, 0.0f }, 
    { 406.0f, 224.0f, 0.0f },
    { 406.0f, 304.0f, 0.0f },
    { 406.0f, 384.0f, 0.0f },
};
static Vec statusLayout2Vs2[4] = {
    { 170.0f, 260.0f, 0.0f }, 
    { 170.0f, 348.0f, 0.0f },
    { 406.0f, 260.0f, 0.0f },
    { 406.0f, 348.0f, 0.0f },
};
static s32 luckyMessTbl[9] = {
    MAKE_MESSID(0x14, 0x00), MAKE_MESSID(0x14, 0x05), MAKE_MESSID(0x14, 0x0A), MAKE_MESSID(0x14, 0x0F),
    MAKE_MESSID(0x14, 0x14), MAKE_MESSID(0x14, 0x19), MAKE_MESSID(0x14, 0x00), MAKE_MESSID(0x14, 0x00),
    MAKE_MESSID(0x14, 0x00)
};

// sbss
static u8 mgType;
static s32 luckyF;
static f32 hilitePos;
static s16 mgNext;
static u8 mgPlay4PLen;
static u8 mgPlay1Vs3Len;
static u8 mgPlay2Vs2Len;
static u8 mgPlayBattleLen;
static s16 mgPlay1Vs3[3];
static s16 mgPlay2Vs2[3];
static s16 mgPlayBattle[2];
static u8 hiliteTimer;
static u8 hiliteBlinkLen;
static u8 hiliteShowF;
static OMOBJ* mgSetupObj;
static HUPROCESS* mgSetupProc;


// sdata
static u8 luckyValue = 0xFF;
static s8 cursorPos = 0xFF;

// determined functions
static void DestroyMGSetup(void);
static void ExecMGSetup(void);
static void DetermineMGList(MGSetupWork*);
static void SetupStatusLayout(void);
static s32 GetMGType(void);
static s32 GetMGTypeTeam(void);
static void CreateMGSetup(void);
static void UpdateMGSetup(OMOBJ*);
static void CenterStatus(MGSetupWork*, OMOBJ*);
static void SeparateStatus(MGSetupWork*, OMOBJ*);
static void PopupVS(MGSetupWork*, OMOBJ*);
static void FallMGType(MGSetupWork*, OMOBJ*);
static void LuckyMGFall(MGSetupWork*, OMOBJ*);
static void HideLuckyValue(void);
static void UpdateLuckyValue(MGSetupWork*, OMOBJ*);
static void UpdateMGList(MGSetupWork*, OMOBJ*);

// FUNCTIONS //

/**
 * @brief Runs the end-of-turn minigame selection sequence.
 *
 * @details Creates a child process for the selection presentation and waits
 * until its destructor clears the process handle.
 */
void BoardMGSetupExec(void) {
    mgSetupProc = HuPrcChildCreate(&ExecMGSetup, 0x2006, 0x3800, 0, boardMainProc);
    HuPrcDestructorSet2(mgSetupProc, &DestroyMGSetup);
    while (mgSetupProc != 0) {
        HuPrcVSleep();
    }
}

/**
 * @brief Finalizes the minigame selection process state.
 *
 * @details Restores the current-player index and clears the process handle
 * used by BoardMGSetupExec() as its completion signal.
 */
static void DestroyMGSetup(void) {
    GWSystem.player_curr = 0;
    mgSetupProc = NULL;
}

/**
 * @brief Reads the current minigame setup presentation state.
 *
 * @return The setup object's state value; state 7 is the idle/completed state
 * awaited by the controlling process.
 */
static inline s32 setupObjGet(void) {
    MGSetupWork* work = (MGSetupWork*) mgSetupObj->work;
    return work->field00_bit1;
}

/**
 * @brief Selects and loads the next board minigame.
 *
 * @details Drives the camera and UI state machine, optionally presents a
 * Lucky Minigame multiplier, starts the selected minigame's data load, records
 * it in the recent-play history, and switches to the minigame overlay.
 */
static void ExecMGSetup(void) {
    s32 board_data_dirs[9] = {
        DATADIR_W01, DATADIR_W02, DATADIR_W03,
        DATADIR_W04, DATADIR_W05, DATADIR_W06,
        DATADIR_W10, DATADIR_W20, DATADIR_W21
    };
    Vec camera_offset;
    u16 minigame_id;
    s32 async_load_id;
    s32 data_dir;
    s32 player;
    u32 message_id;
    MGSetupWork* lucky_drop_work;
    MGSetupWork* separate_work;
    MGSetupWork* lucky_value_work;
    MGSetupWork* normal_setup_work;
    MGSetupWork* list_work;

    luckyF = 0;
    mgNext = 0;
    GWSystem.player_curr = -1;
	GWLuckyValueSet(1);
    for (player = 0; player < ARRAY_COUNT(activeMG); player++) {
        activeMG[player].window_id = -1;
        activeMG[player].name_mess = 0;
    }
    BoardPlayerMoveAwayStartCurr(GWPlayer[GWSystem.player_curr].space_curr, 1);
    BoardCameraMoveSet(0);
    BoardCameraTargetSpaceSet(BoardSpaceStarGetCurr());
    camera_offset.x = 0.0f;
    camera_offset.y = 150.0f;
    camera_offset.z = 0.0f;
    BoardCameraMotionStartEx(-1, NULL, &camera_offset, 1300.0f, -1.0f, 21);
    BoardCameraMotionWait();
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 21);
    
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    BoardFilterFadeInit(0x1E, 0xA0U);
    CreateMGSetup();
    while (setupObjGet() != 7) {
        HuPrcVSleep();
    }
    if (BoardRandMod(0x64U) < 5U) {
        HuPrcSleep(0x1E);
        luckyF = 1;
        lucky_drop_work = (MGSetupWork*) mgSetupObj->work;
        lucky_drop_work->field00_bit1 = 4;
        lucky_drop_work->phase = 0;
        lucky_drop_work->timer = 0;
        lucky_drop_work->angle = 0;
        HuAudFXPlay(0x35C);
        while (setupObjGet() != 7) {
            HuPrcVSleep();
        }
        separate_work = (MGSetupWork*) mgSetupObj->work;
        separate_work->field00_bit1 = 1;
        separate_work->phase = 0;
        separate_work->timer = 0;
        separate_work->angle = 0;
        while (setupObjGet() != 7) {
            HuPrcVSleep();
        }
        message_id = luckyMessTbl[GWBoardGet()];
        BoardWinCreate(2, message_id, BoardWinPortraitGetStar());
        BoardWinWait();
        BoardWinKill();
        lucky_value_work = (MGSetupWork*) mgSetupObj->work;
        lucky_value_work->field00_bit1 = 5;
        lucky_value_work->phase = 0;
        lucky_value_work->timer = 0;
        lucky_value_work->angle = 0;
        while (setupObjGet() != 7) {
            HuPrcVSleep();
        }
        switch (GWLuckyValueGet()) {
            case 1:
                message_id = luckyMessTbl[GWBoardGet()] + 1;
                break;
            case 2:
                message_id = luckyMessTbl[GWBoardGet()] + 2;
                break;
            case 3:
                message_id = luckyMessTbl[GWBoardGet()] + 3;
                break;
        }
        BoardWinCreate(2, message_id, BoardWinPortraitGetStar());
        BoardWinWait();
        message_id = luckyMessTbl[GWBoardGet()] + 4;
        BoardWinCreate(2, message_id, BoardWinPortraitGetStar());
        BoardWinWait();
        BoardWinKill();
        HideLuckyValue();
    } else {
        normal_setup_work = (MGSetupWork*) mgSetupObj->work;
        normal_setup_work->field00_bit1 = 1;
        normal_setup_work->phase = 0;
        normal_setup_work->timer = 0;
        normal_setup_work->angle = 0;
        while (setupObjGet() != 7) {
            HuPrcVSleep();
        }
    }
    list_work = (MGSetupWork*) mgSetupObj->work;
    list_work->field00_bit1 = 6;
    list_work->phase = 0;
    list_work->timer = 0;
    list_work->angle = 0;
    while (setupObjGet() != 7) {
        HuPrcVSleep();
    }
    HuDataDirClose(DATADIR_BOARD);
    HuDataDirClose(DATADIR_BGUEST);
    HuDataDirClose(board_data_dirs[GWBoardGet()]);
    HuPrcSleep(2);
    if (_CheckFlag(FLAG_ID_MAKE(0, 11)) != 0) {
        data_dir = 0x140000;
    } else {
        data_dir = mgInfoTbl[mgNext].data_dir;
    }
    async_load_id = BoardDataDirReadAsync(data_dir);
    BoardAudSeqFadeOut(0, 1000);
    _SetFlag(FLAG_ID_MAKE(1, 14));
    WipeColorSet(255, 255, 255);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    HuAudFXAllStop();
    _SetFlag(FLAG_ID_MAKE(1, 28));
    
    for (player = 0; player < 4; player++) {
        BoardStatusItemHideSet(player, 1);
    }
    ((MGSetupWork*) (mgSetupObj->work))->field00_bit0 = 1;
    BoardDataAsyncWait(async_load_id);
    minigame_id = mgNext + 0x191;
    BoardMGSetupPlayPush(mgType, minigame_id);
    GWMGTypeSet(-1);
    GWMGAvailSet(minigame_id);
    GWSystem.mg_next = minigame_id - 0x191;
    _SetFlag(0x10002U);
    if (GWSystem.turn == GWSystem.max_turn) {
        _SetFlag(0xAU);
    }
    BoardNextOvlSet(3);
    HuPrcEnd();
}

/**
 * @brief Builds the randomized candidate list for the current minigame type.
 *
 * @details Applies the configured minigame-list mode, custom availability,
 * recent-play exclusion, and duplicate-name rejection. Locked candidates are
 * retained but marked so the UI can conceal their names.
 *
 * @param work Setup state whose candidate-count field determines how many
 * entries are selected and whose result is written to activeMG.
 */
static void DetermineMGList(MGSetupWork* work) {
    MgInfo* minigame_info;
    s32 candidate_index;
    s32 selection_index;
    s16* list_entry;
    s32 duplicate_found;
    s32 excluded;
    s32 all_count;
    s32 info_count;
    u32 candidate_name_mess;
    s32 candidate_slot;
    u32 candidate_count;
    s32 candidate_id;
    u32* name_messages;
    s16* minigame_indices;

    all_count = 0;
    minigame_info = mgInfoTbl;
    
    for (info_count = 0; minigame_info->ovl != 0xFFFF; minigame_info++, info_count++) {
        if ((minigame_info->flag & 1) == 0) {
            mgListAll[all_count] = (minigame_info - mgInfoTbl) + 0x191;
            all_count++;
        }
    }
    mgListAll[all_count] = -1;
    name_messages = HuMemDirectMallocNum(HEAP_HEAP, info_count * 4, HU_MEMNUM_OVL);
    minigame_indices = HuMemDirectMallocNum(HEAP_HEAP, info_count * 2, HU_MEMNUM_OVL);
    minigame_info = mgInfoTbl;
    
    for (candidate_slot = selection_index = 0; selection_index < info_count; selection_index++, minigame_info++) {
        if ((mgType == minigame_info->type) && (minigame_info->ovl != 0x29)) {
            if ((GWMGListGet() != 2) || (GWMGCustomGet(selection_index + 0x191) != 0)) {
                if (GWMGListGet() == 1) {
                    excluded = 0;
                    list_entry = &mgListAll[0];
                    while (*list_entry != -1) {
                        if (selection_index + 0x191 == *list_entry) {
                            excluded = 1;
                            break;
                        }
                        list_entry++;
                    }
                    if (excluded != 0) continue;
                }
                name_messages[candidate_slot] = minigame_info->name_mess;
                minigame_indices[candidate_slot] = selection_index;
                candidate_slot++;
            }
        }
    }

    candidate_count = candidate_slot;
    selection_index = 0;
    while (selection_index < work->field01_bit0) {
        candidate_slot = BoardRandMod(candidate_count);
        candidate_name_mess = name_messages[candidate_slot];
        candidate_index = minigame_indices[candidate_slot];
        candidate_id = candidate_index + 0x191;

        if (GWMGListGet() == 2 || BoardMGSetupPlaySearch(mgType, candidate_id) == 0) {
            duplicate_found = 0;
            for (candidate_slot = 0; candidate_slot < selection_index; candidate_slot++) {
                if (candidate_name_mess == activeMG[candidate_slot].name_mess) {
                    duplicate_found = 1;
                    break;
                }
            }
            if (duplicate_found == 0) {
                if (GWMGAvailGet(candidate_index + 0x191) == 0) {
                    activeMG[selection_index].is_available = 0;
                } else {
                    activeMG[selection_index].is_available = 1;
                }
                activeMG[selection_index].name_mess = candidate_name_mess;
                activeMG[selection_index].minigame_index = candidate_index;
                selection_index++;
            }
        }
    }
    HuMemDirectFree(name_messages);
    HuMemDirectFree(minigame_indices);
}

/**
 * @brief Assigns player groups and status-panel targets for team minigames.
 *
 * @details For 1-vs-3 and 2-vs-2 types, derives groups from player colors,
 * preserves each player's prior team, and positions status panels in the
 * corresponding layout. Four-player minigames require no rearrangement here.
 */
static void SetupStatusLayout(void) {
    s32 solo_player;
    s32 other_player;
    s32 red_count;
    s32 right_slot;
    s32 left_slot;
    s32 player;

    if (mgType != 0) {
        if (mgType == 1) {
            for (red_count = player = 0; player < 4; player++) {
                if (GWPlayer[player].color == 2) {
                    solo_player = player;
                    GWPlayer[player].team_backup = GWPlayerTeamGet(player);
                    GWPlayerCfg[player].group = 0;
                    red_count++;
                } else {
                    GWPlayer[player].team_backup = GWPlayerTeamGet(player);
                    GWPlayerCfg[player].group = 1;
                    other_player = player;
                }
            }
            if (red_count == 1) {
                BoardStatusTargetPosSet(solo_player, statusLayout1Vs3);
                right_slot = 1;
                
                for (player = 0; player < 4; player++) {
                    if (player != solo_player) {
                        BoardStatusTargetPosSet(player, &statusLayout1Vs3[right_slot++]);
                    }
                }
                return;
            }
            BoardStatusTargetPosSet(other_player, statusLayout1Vs3);
            left_slot = 1;
            
            for (player = 0; player < 4; player++) {
                if (player == other_player) {
                    GWPlayer[player].team_backup = GWPlayerTeamGet(player);
                    GWPlayerCfg[player].group = 0;
                } else {
                    BoardStatusTargetPosSet(player, &statusLayout1Vs3[left_slot++]);
                    GWPlayer[player].team_backup = GWPlayerTeamGet(player);
                    GWPlayerCfg[player].group = 1;
                }
            }
            return;
        }
        right_slot = 2;
        for (left_slot = player = 0; player < 4; player++) {
            if (GWPlayer[player].color == 2) {
                BoardStatusTargetPosSet(player, &statusLayout2Vs2[left_slot++]);
                GWPlayer[player].team_backup = GWPlayerTeamGet(player);
                GWPlayerCfg[player].group = 0;
            } else {
                BoardStatusTargetPosSet(player, &statusLayout2Vs2[right_slot++]);
                GWPlayer[player].team_backup = GWPlayerTeamGet(player);
                GWPlayerCfg[player].group = 1;
            }
        }
    }
}

/**
 * @brief Resolves a non-team game's minigame type from player colors.
 *
 * @details Converts neutral or unresolved colors to blue/red, using weighted
 * random choices where needed, then counts the resulting split.
 *
 * @return 0 for four-player, 1 for one-versus-three, or 2 for two-versus-two.
 */
static s32 GetMGType(void) {
    s32 neutral_count;
    s32 blue_count;
    s32 red_count;
    s32 player;
    s32 alternate_color;
    s32 preferred_color;
    s8 assigned_color;

    for (blue_count = red_count = neutral_count = player = 0; player < 4; player++) {
        if (GWPlayer[player].color == 2) {
            red_count++;
        } else if (GWPlayer[player].color == 1) {
            blue_count++;
        } else {
            neutral_count++;
        }
    }
    if ((red_count != 2) || (blue_count != 2)) {
        if (neutral_count == 4) {
            player = BoardRandMod(4);
            GWPlayer[player].color = 1;
            neutral_count = 3;
            blue_count = 1;
            red_count = 0;
        }
        preferred_color = alternate_color = -1;
        if (red_count == 3) {
            preferred_color = 2;
            alternate_color = 1;
        } else if (blue_count == 3) {
            preferred_color = 1;
            alternate_color = 2;
        }
        if (((neutral_count >= 3) || ((neutral_count == 2) && (blue_count == 1) && (red_count == 1))) && (BoardRandMod(0x64U) > 0x28U)) {
            if (red_count == 1) {
                assigned_color = 2;
            } else {
                assigned_color = 1;
            }
            do {
                player = BoardRandMod(4);
            } while (GWPlayer[player].color != 3);
            GWPlayer[player].color = assigned_color;
            if (assigned_color == 1) {
                assigned_color = 2;
            } else {
                assigned_color = 1;
            }
            for (player = 0; player < 4; player++) {
                if (GWPlayer[player].color == 3) {
                    GWPlayer[player].color = assigned_color;
                }
            }
        } else {
            
            for (player = 0; player < 4; player++) {
                if ((GWPlayer[player].color == 3) || (GWPlayer[player].color == 0)) {
                    if (preferred_color == -1) {
                        if (BoardRandMod(0x64) < 0x32) {
                            assigned_color = 2;
                        } else {
                            assigned_color = 1;
                        }
                    } else if (BoardRandMod(0x64) < 0x5A) {
                        assigned_color = preferred_color;
                    } else {
                        assigned_color = alternate_color;
                    }
                    GWPlayer[player].color = assigned_color;
                }
            }
        }
    }
    
    
    for (red_count = blue_count = player = 0; player < 4; player++) {
        if (GWPlayer[player].color == 2) {
            red_count++;
        } else if (GWPlayer[player].color == 1) {
            blue_count++;
        }
    }

    if (red_count == 1 || red_count == 3) {
        return 1;
    } else if (red_count == 2) {
        return 2;
    } else {
        return 0;
    }
}

/**
 * @brief Preserves three otherwise-unused floating-point constants.
 *
 * @details This function has no observable runtime effect and appears to exist
 * solely to retain constants associated with the original object code.
 */
static void UnusedFloat(void)
{
	(void)288.0f;
	(void)-98.0f;
	(void)674.0f;
}

/**
 * @brief Resolves the minigame type while respecting permanent teams.
 *
 * @details Assigns unresolved player colors, then adjusts teammates' colors so
 * the result is either a four-player or two-versus-two minigame compatible
 * with the configured teams.
 *
 * @return 0 for four-player or 2 for two-versus-two.
 */
static s32 GetMGTypeTeam(void) {
    s8 red_players[4];
    s8 blue_players[4];
    s8 neutral_players[4];
    s8 team_colors[2] = { 2, 1 };
    s32 teammate;
    s32 player;
    s32 swap_color;
    s8 neutral_count;
    s8 blue_count;
    s8 red_count;
    s8* solo_players;

    neutral_players[0] = neutral_players[1] = neutral_players[2] = neutral_players[3] = -1;
    blue_players[0] = blue_players[1] = blue_players[2] = blue_players[3] = -1;
    red_players[0] = red_players[1] = red_players[2] = red_players[3] = -1;
    
    for (red_count = neutral_count = blue_count = player = 0; player < 4; player++) {
        switch (GWPlayer[player].color) {
        case 2:
            red_players[red_count] = player;
            red_count++;
            break;
        case 1:
            blue_players[blue_count] = player;
            blue_count++;
            break;
        default:
            neutral_players[neutral_count] = player;
            neutral_count++;
            break;
        }
    }
    if (neutral_count != 0) {
        
        for (player = 0; player < neutral_count; player++) {
            GWPlayer[neutral_players[player]].color = team_colors[BoardRandMod(2)];
        }
        return GetMGTypeTeam();
    }
    if ((red_count == 4) || (blue_count == 4)) {
        return 0;
    }
    if ((red_count == 1) || (blue_count == 1)) {
        if (red_count == 1) {
            solo_players = red_players;
        } else {
            solo_players = blue_players;
        }
        if (BoardRandMod(0x64U) < 0x46U) {
            teammate = BoardPlayerSameTeamFind(*solo_players);
            GWPlayer[*solo_players].color = GWPlayer[teammate].color;
            return 0;
        }
        teammate = BoardPlayerSameTeamFind(*solo_players);
        GWPlayer[teammate].color = GWPlayer[*solo_players].color;
        return 2;
    }
    teammate = BoardPlayerSameTeamFind(red_players[0]);
    if (GWPlayer[red_players[0]].color != GWPlayer[teammate].color) {
        swap_color = GWPlayer[teammate].color;
        GWPlayer[teammate].color = GWPlayer[red_players[0]].color;

        
        for (player = 0; player < 4; player++) {
            if ((player != red_players[0]) && (player != teammate)) {
                break;
            }
        }
        GWPlayer[player].color = swap_color;
        teammate = BoardPlayerSameTeamFind(player);
        GWPlayer[teammate].color = swap_color;
    }
    return 2;
}

/**
 * @brief Creates the object and sprite group used by minigame setup.
 *
 * @details Allocates per-object sprite state, loads all 13 setup sprites, and
 * initializes both current and target position tables at the screen center.
 */
static void CreateMGSetup(void) {
    s32 sprite;
    MGSetupSprites* sprites;
    MGSetupWork* work;
    MGSetupWork* state_work;

    mgSetupObj = omAddObjEx(boardObjMan, 0x200, 0U, 0U, -1, UpdateMGSetup);
    work = (MGSetupWork*) mgSetupObj->work;
    work->field00_bit0 = 0;
    state_work = (MGSetupWork*) mgSetupObj->work;
    state_work->field00_bit1 = 0;
    state_work->phase = 0;
    state_work->timer = 0;
    state_work->angle = 0;
    work->sprites = HuMemDirectMallocNum(HEAP_HEAP, sizeof(MGSetupSprites), HU_MEMNUM_OVL);
    sprites = work->sprites;
    sprites->handles[0] = HuSprGrpCreate(0xD);
    
    for (sprite = 0; sprite < 13; sprite++) {
        BoardSpriteCreate(mgSetupSprTbl[sprite], 0x5DC, NULL, &sprites->handles[sprite] + 1);
        HuSprGrpMemberSet(sprites->handles[0], sprite, *(&sprites->handles[sprite] + 1));
        HuSprAttrSet(sprites->handles[0], sprite, 4);
        sprites->positions[0][sprite].x = sprites->positions[0][sprite].y = sprites->positions[0][sprite].z = 0.0f;
        sprites->positions[1][sprite].x = sprites->positions[1][sprite].y = sprites->positions[1][sprite].z = 0.0f;
    }
    HuSprGrpPosSet(sprites->handles[0], 288.0f, 240.0f);
}

/**
 * @brief Updates or destroys the minigame setup presentation object.
 *
 * @param object Object whose work structure stores the presentation state.
 */
static void UpdateMGSetup(OMOBJ* object) {
    MGSetupWork* work;
    s32 slot;

    work = (MGSetupWork*) object->work;
    if (work->field00_bit0 != 0 || BoardIsKill() != 0) {
        for (slot = 0; slot < ARRAY_COUNT(activeMG); slot++) {
            if (activeMG[slot].window_id != -1) {
                HuWinKill(activeMG[slot].window_id);
                activeMG[slot].window_id = -1;
            }
        }
        HuSprGrpKill(work->sprites->handles[0]);
        HuMemDirectFree(work->sprites);
        mgSetupObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), object);
    } else if (work->delay != 0) {
        work->delay--;
    } else {
        switch (work->field00_bit1) {
            case 0:
                CenterStatus(work, object);
                break;
            case 1:
                SeparateStatus(work, object);
                break;
            case 2:
                PopupVS(work, object);
                break;
            case 3:
                FallMGType(work, object);
                break;
            case 4:
                LuckyMGFall(work, object);
                break;
            case 5:
                UpdateLuckyValue(work, object);
                break;
            case 6:
                UpdateMGList(work, object);
                break;
				
			case 7:
				break;
        }
    }
}

/**
 * @brief Centers player status panels and prepares candidate selection.
 *
 * @details Waits for all panels to stop, resolves the minigame type, builds
 * the candidate list, and advances the presentation to the VS popup.
 *
 * @param work Mutable setup state for this presentation.
 * @param object Owning object; not directly accessed by this state handler.
 */
static void CenterStatus(MGSetupWork* work, OMOBJ* object) {
    s32 player;
    MGSetupWork* state_work;

    if (work->phase == 0) {
        for (player = 0; player < 4; player++) {
            BoardStatusItemHideSet(player, 0);
            BoardStatusTargetPosSet(player, &statusLayout4P[player]);
        }
        work->phase = 1;
        HuAudFXPlay(0x354);
        return;
    }
    
    for (player = 0; player < 4; player++) {
        if (!BoardStatusStopCheck(player)) {
            return;
        }
    }
    if ((s32) GWSystem.team != 0) {
        mgType = GetMGTypeTeam();
    } else {
        mgType = GetMGType();
    }
    if (mgType == 0) {
        work->field01_bit0 = 4;
    } else {
        work->field01_bit0 = 3;
    }
    SetupStatusLayout();
    DetermineMGList(work);
    if (_CheckFlag(FLAG_ID_MAKE(1, 11)) != 0) {
        BoardTutorialHookExec(0x1A, 0);
    }
    state_work = (MGSetupWork*) mgSetupObj->work;
    state_work->field00_bit1 = 2;
    state_work->phase = 0;
    state_work->timer = 0;
    state_work->angle = 0;
}

/**
 * @brief Moves status panels offscreen and dismisses the VS sprite.
 *
 * @param work Mutable setup state and animation counters.
 * @param object Owning object; not directly accessed by this state handler.
 */
static void SeparateStatus(MGSetupWork* work, OMOBJ* object) {
    Vec status_pos;
    f32 scale;
    f32 angle;
    s32 player;
    MGSetupWork* state_work;
    MGSetupSprites* sprites;

    sprites = work->sprites;
    switch (work->phase) {
        case 0:
            for (player = 0; player < 4; player++) {
                BoardStatusPosGet(player, &status_pos);
                if (status_pos.x < 288.0f) {
                    status_pos.x = -98.0f;
                } else {
                    status_pos.x = 674.0f;
                }
                BoardStatusTargetPosSet(player, &status_pos);
            }
            work->phase = 1;
            work->timer = 0;
            return;
    case 1:
        if ((s8) work->timer >= 0x5A) {
            work->timer = 0;
            state_work = (MGSetupWork*) mgSetupObj->work;
            state_work->field00_bit1 = 7;
            state_work->phase = 0;
            state_work->timer = 0;
            state_work->angle = 0;
            return;
        }
        OSu8tof32((u8*)&work->timer, &angle);
        scale = sind(angle) * 4.0 + 1.0;
        work->timer += 6;
        if (work->timer > 0x5A) {
            work->timer = 0x5A;
        }
        HuSprScaleSet(sprites->handles[0], 0, scale, scale);
        if (work->timer > 0x3C) {
            HuSprTPLvlSet(sprites->handles[0], 0, (90.0f - work->timer) / 30.0f);
        }
    }
}

/**
 * @brief Animates the VS sprite into view.
 *
 * @details Scales the sprite through a sine-based overshoot, pauses before the
 * next state, and invokes the corresponding tutorial hook when enabled.
 *
 * @param work Mutable setup state and animation angle.
 * @param object Owning object; not directly accessed by this state handler.
 */
static void PopupVS(MGSetupWork* work, OMOBJ* object) {
    f32 scale;
    f32 angle;
    MGSetupWork* state_work;
    MGSetupSprites* sprites;

    sprites = work->sprites;
    if (work->phase == 0) {
        HuSprAttrReset(sprites->handles[0], 0, HUSPR_ATTR_DISPOFF);
        HuSprAttrSet(sprites->handles[0], 0, 8);
        HuSprScaleSet(sprites->handles[0], 0, 0.001f, 0.001f);
        HuSprPosSet(sprites->handles[0], 0, 0.0f, 64.0f);
        work->phase = 1;
        HuAudFXPlay(0x355);
        return;
    }
    OSs16tof32(&work->angle, &angle);
    if (work->angle < 0x5A) {
        scale = 2.0 * sind(angle);
    } else {
        scale = 1.0 + sind(angle);
        if (work->angle >= 0xB4) {
            scale = 1.0f;
            state_work = (MGSetupWork*) mgSetupObj->work;
            state_work->field00_bit1 = 3;
            state_work->phase = 0;
            state_work->timer = 0;
            state_work->angle = 0;
            work->delay = 0x3C;
        }
    }
    HuSprScaleSet(sprites->handles[0], 0, scale, scale);
    work->angle += 9;
    if ((_CheckFlag(FLAG_ID_MAKE(1, 11)) != 0) && work->field00_bit1 == 3) {
        BoardTutorialHookExec(0x1B, 0);
        if (*boardTutorialData == 1) {
            work->field00_bit1 = 0;
        }
    }
}

/**
 * @brief Drops the resolved minigame-type sprite to its target position.
 *
 * @param work Mutable setup state and sprite-position storage.
 * @param object Owning object; not directly accessed by this state handler.
 */
static void FallMGType(MGSetupWork* work, OMOBJ* object) {
    Vec offset;
    s16 type_sprite_ids[3] = { 3, 1, 2 };
    Vec* unused_pos_a;
    Vec* unused_pos_b;
    s16 sprite;
    MGSetupWork* state_work;
    MGSetupSprites* sprites;

    sprites = work->sprites;
    sprite = type_sprite_ids[mgType];
    if (work->phase == 0) {
        HuSprAttrReset(sprites->handles[0], sprite, HUSPR_ATTR_DISPOFF);
        sprites->positions[0][sprite].y = -284.0f;
        sprites->positions[1][sprite].y = -128.0f;
        work->phase = 1;
    }
    VECSubtract(&sprites->positions[1][sprite], &sprites->positions[0][sprite], &offset);
    if (VECMag(&offset) <= 1.0f) {
        sprites->positions[0][sprite] = sprites->positions[1][sprite];
        state_work = (MGSetupWork*) mgSetupObj->work;
        state_work->field00_bit1 = 7;
        state_work->phase = 0;
        state_work->timer = 0;
        state_work->angle = 0;
    } else {
        VECScale(&offset, &offset, 0.1f);
        VECAdd(&offset, &sprites->positions[0][sprite], &sprites->positions[0][sprite]);
    }
    HuSprPosSet(sprites->handles[0], sprite, sprites->positions[0][sprite].x, sprites->positions[0][sprite].y);
}

/**
 * @brief Animates the Lucky Minigame banner replacing the type sprite.
 *
 * @details Drops the Lucky sprite, moves the original type sprite downward,
 * and applies a short rotation before returning the state machine to idle.
 *
 * @param work Mutable setup state and sprite-position storage.
 * @param object Owning object; not directly accessed by this state handler.
 */
static void LuckyMGFall(MGSetupWork* work, OMOBJ* object) {
    u16 unused_angle_a;
    u16 unused_angle_b;
    s16 type_sprite_ids[3] = { 3, 1, 2 };
    Vec* lucky_target;
    Vec* type_pos;
    Vec* lucky_pos;
    s16 type_sprite;
    MGSetupWork* state_work;
    u8 unused_value;
    MGSetupSprites* sprites;
    f32 rotation;
    
    sprites = work->sprites;
    type_sprite = type_sprite_ids[mgType];
    lucky_pos = &sprites->positions[0][4];
    lucky_target = &sprites->positions[1][4];
    type_pos = &sprites->positions[0][type_sprite];
    switch (work->phase) {
        case 0:
            HuSprAttrReset(sprites->handles[0], 4, HUSPR_ATTR_DISPOFF);
            lucky_pos->y = -284.0f;
            lucky_target->y = -128.0f;
            work->phase = 1;
            break;
        case 1:
            lucky_pos->y += 10.0f;
            if (lucky_target->y < lucky_pos->y) {
                work->phase = 2;
                work->timer = 0;
                lucky_pos->y = lucky_target->y;
                HuAudFXPlay(0x35D);
            }
            break;
        case 2:
            type_pos->y += 10.0f;
            if (328.0f < type_pos->y) {
                HuSprAttrReset(sprites->handles[0], type_sprite, HUSPR_ATTR_DISPOFF);
                state_work = (MGSetupWork*) mgSetupObj->work;
                state_work->field00_bit1 = 7;
                state_work->phase = 0;
                state_work->timer = 0;
                state_work->angle = 0;
            } else if (work->timer < 0x1E) {
                OSs8tof32(&work->timer, &rotation);
                HuSprZRotSet(sprites->handles[0], type_sprite, rotation);
                work->timer++;
            }
            break;
    }
    HuSprPosSet(sprites->handles[0], 4, lucky_pos->x, lucky_pos->y);
    HuSprPosSet(sprites->handles[0], type_sprite, type_pos->x, type_pos->y);
}

/**
 * @brief Fades out and hides the Lucky Minigame multiplier sprites.
 */
static void HideLuckyValue(void) {
    f32 alpha;
    MGSetupWork* work;
    MGSetupSprites* sprites;

    work = (MGSetupWork*) mgSetupObj->work;
    sprites = work->sprites;
    
    for (alpha = 1.0f; alpha > 0.1f; alpha -= 0.1f) {
        HuSprTPLvlSet(sprites->handles[0], 8, alpha);
        HuSprTPLvlSet(sprites->handles[0], 9, alpha);
        HuPrcVSleep();
    }
    HuSprAttrSet(sprites->handles[0], 8, 4);
    HuSprAttrSet(sprites->handles[0], 9, 4);
}

/**
 * @brief Chooses and animates the Lucky Minigame multiplier.
 *
 * @details Randomly leaves the configured value at 1 or changes it to 2 or 3,
 * heavily weighting 2, then spins the displayed choice until it reaches that
 * value and pulses the result.
 * The displayed bank is zero-based while GWLuckyValueGet() is one-based.
 *
 * @param work Mutable setup state, timers, and highlight speed.
 * @param object Owning object; not directly accessed by this state handler.
 */
static void UpdateLuckyValue(MGSetupWork* work, OMOBJ* object) {
    s32 unused_index;
    f32 scale;
    s32 random_value;
    MGSetupWork* state_work;
    MGSetupSprites* sprites;
    f32 angle;
    u8 highlight_index;

    sprites = work->sprites;
    switch (work->phase) {
        case 0:
            HuSprAttrReset(sprites->handles[0], 8, HUSPR_ATTR_DISPOFF);
            HuSprAttrReset(sprites->handles[0], 9, HUSPR_ATTR_DISPOFF);
            HuSprAttrSet(sprites->handles[0], 8, 8);
            HuSprAttrSet(sprites->handles[0], 9, 8);
            HuSprPosSet(sprites->handles[0], 8, 0.0f, 0.0f);
            HuSprPosSet(sprites->handles[0], 9, 0.0f, 0.0f);
            HuSprScaleSet(sprites->handles[0], 8, 0.001f, 0.001f);
            HuSprScaleSet(sprites->handles[0], 9, 0.001f, 0.001f);
            HuSprBankSet(sprites->handles[0], 9, 1);
            work->angle = 0;
            work->phase = 1;
            work->timer = 0;
            random_value = BoardRandMod(0x64U);
            if (random_value < 0x55) {
				GWLuckyValueSet(2);
                return;
            }
            if (random_value >= 0x5F) return;
			GWLuckyValueSet(3);
            return;
        case 1:
            if (work->timer < 0x5A) {
                OSu8tof32((u8*) &work->timer, &angle);
                scale = sind(angle);
                HuSprScaleSet(sprites->handles[0], 8, scale, scale);
                HuSprScaleSet(sprites->handles[0], 9, scale, scale);
                work->timer += 2;
                return;
            }
            work->phase = 2;
            work->delay = BoardRandMod(0xCU) + 0x12;
            work->timer = 0;
            work->spin_speed = 0.8f;
            return;
        case 2:
            work->spin_speed *= 0.97f;
            hilitePos += work->spin_speed;
            if (hilitePos >= 3.0f) {
                hilitePos = 0.0f;
            }
            OSf32tos8(&hilitePos, (s8*)&highlight_index);
            if (highlight_index != luckyValue) {
                HuAudFXPlay(0x304);
                luckyValue = highlight_index;
            }
            HuSprBankSet(sprites->handles[0], 9, luckyValue);
            if (work->spin_speed < 0.05f) {
                if (luckyValue != GWLuckyValueGet() - 1) {
                    work->spin_speed *= 1.0309278f;
                    return;
                }
                HuAudFXPlay(0x305);
                HuAudFXPlay(0x357);
                HuAudFXPlay(0x358);
                work->phase = 3;
                work->timer = 0;
                work->angle = 0;
                return;
            }
            break;
        case 3:
            work->angle += 8;
            if (work->angle >= 0x438) {
                state_work = (MGSetupWork*) mgSetupObj->work;
                state_work->field00_bit1 = 7;
                state_work->phase = 0;
                state_work->timer = 0;
                state_work->angle = 0;
                return;
            }
            angle = (work->angle % 360);
            scale = (1.0 + (0.5 * sind(angle)));
            HuSprScaleSet(sprites->handles[0], 8, scale, scale);
            HuSprScaleSet(sprites->handles[0], 9, scale, scale);
            break;
    }
}

static f32 cursorYTbl[4] = {
    64.0f, 88.0f, 112.0f, 136.0f
};
static f32 hiliteYTbl[4] = {
    60.0f, 84.0f, 108.0f, 132.0f
};
static GXColor hiliteColTbl[3] = {
    { 0xF8, 0xF2, 0x13, 0x00 },
    { 0xF8, 0xF2, 0x13, 0x00 },
    { 0xF8, 0xF2, 0x13, 0x00 }
};

/**
 * @brief Presents the roulette of candidate minigames and selects one.
 *
 * @details Creates candidate-name windows, spins and decelerates the cursor,
 * reveals the selected name, blinks its highlight, and dismisses the list.
 * The selected table index is stored in mgNext.
 *
 * @param work Mutable setup state, timers, candidate count, and spin speed.
 * @param object Owning object; not directly accessed by this state handler.
 */
static void UpdateMGList(MGSetupWork* work, OMOBJ* object) {
    f32 message_size[2];
    s32 message_id;
    f32 angle;
    f32 window_x;
    f32 window_y;
    f32 scale;
    s32 type_sprite;
    s16 window_id;
    s8 highlight_index;
    GXColor* highlight_color;
    s32 slot;
    MGSetupWork* state_work;
    MGSetupSprites* sprites;
    
    sprites = work->sprites;
    type_sprite = mgType + 5;
    switch (work->phase) {
        case 0:
            HuSprAttrSet(sprites->handles[0], type_sprite, 8);
            HuSprAttrReset(sprites->handles[0], type_sprite, HUSPR_ATTR_DISPOFF);
            HuSprPosSet(sprites->handles[0], type_sprite, 0.0f, 32.0f);
            HuSprScaleSet(sprites->handles[0], type_sprite, 0.001f, 0.001f);
            work->angle = 0;
            work->phase = 1;
            work->timer = 0;
            hilitePos = 0.0f;
            
            for (slot = 0; slot < work->field01_bit0; slot++) {
                HuWinMesSizeCancelCRSet(1);
                HuWinMesMaxSizeGet(1, &message_size[0], activeMG[slot].name_mess);
                window_x = 152.0f;
                window_y = 280.0f + (slot * 0x18);
                window_id = HuWinCreate(window_x, window_y, message_size[0], message_size[1], 0);
                HuWinBGTPLvlSet(window_id, 0.0f);
                HuWinMesSpeedSet(window_id, 0);
                HuWinDispOff(window_id);
                HuWinAttrSet(window_id, 0x100);
                if (activeMG[slot].is_available != 0) {
                    message_id = activeMG[slot].name_mess;
                } else {
                    message_id = MAKE_MESSID(9, 1);
                }
                HuWinMesSet(window_id, message_id);
                activeMG[slot].window_id = window_id;
            }
            return;
        case 1:
            if (work->timer < 0x5A) {
                OSu8tof32((u8*) &work->timer, &angle);
                scale = sind(angle);
                work->timer += 3;
                HuSprScaleSet(sprites->handles[0], type_sprite, scale, scale);
                return;
            }
            
            for (slot = 0; slot < work->field01_bit0; slot++) {
                HuWinDispOn(activeMG[slot].window_id);
            }
            HuSprAttrReset(sprites->handles[0], 0xA, HUSPR_ATTR_DISPOFF);
            HuSprAttrSet(sprites->handles[0], 0xA, 8);
            HuSprPosSet(sprites->handles[0], 0xA, -148.0f, cursorYTbl[0]);
            highlight_color = &hiliteColTbl[mgType];
            HuSprAttrReset(sprites->handles[0], 0xC, HUSPR_ATTR_DISPOFF);
            HuSprPosSet(sprites->handles[0], 0xC, 0.0f, hiliteYTbl[0]);
            HuSprTPLvlSet(sprites->handles[0], 0xC, 0.7f);
            HuSprScaleSet(sprites->handles[0], 0xC, 32.0f, 3.5f);
            HuSprColorSet(sprites->handles[0], 0xC, highlight_color->r, highlight_color->g, highlight_color->b);
            work->phase = 2;
            work->timer = 0;
            work->spin_speed = 0.7f;
            work->spin_delay = 0x23 - BoardRandMod(0x1EU);
            return;
        case 2:
            hilitePos += work->spin_speed;
            if (hilitePos >= work->field01_bit0) {
                hilitePos = 0.0f;
            }
            OSf32tos8(&hilitePos, &highlight_index);
            if (highlight_index != cursorPos) {
                HuAudFXPlay(0x304);
                cursorPos = highlight_index;
            }
            HuSprPosSet(sprites->handles[0], 0xA, -148.0f, cursorYTbl[cursorPos]);
            HuSprPosSet(sprites->handles[0], 0xC, 0.0f, hiliteYTbl[cursorPos]);
            if (work->spin_delay != 0) {
                work->spin_delay--;
                return;
            }
            work->phase = 3;
            return;
        case 3:
            work->spin_speed *= 0.97f;
            hilitePos += work->spin_speed;
            if (hilitePos >= work->field01_bit0) {
                hilitePos = 0.0f;
            }
            OSf32tos8(&hilitePos, &highlight_index);
            if (highlight_index != cursorPos) {
                HuAudFXPlay(0x304);
                cursorPos = highlight_index;
            }
            HuSprPosSet(sprites->handles[0], 0xA, -148.0f, cursorYTbl[cursorPos]);
            HuSprPosSet(sprites->handles[0], 0xC, 0.0f, hiliteYTbl[cursorPos]);
            if (!(work->spin_speed < (0.02f + (0.02f * BoardRandFloat())))) return; // weird hacky garbage
            HuAudFXPlay(0x305);
            work->phase = 4;
            work->timer = 0;
            hiliteTimer = 0;
            hiliteShowF = 1;
            hiliteBlinkLen = 5;
            mgNext = activeMG[cursorPos].minigame_index;
            HuWinMesSet(activeMG[cursorPos].window_id, activeMG[cursorPos].name_mess);
            return;
        case 4:
            if (work->timer++ > 0x78U) {
                HuSprAttrSet(sprites->handles[0], 0xA, 4);
                HuSprAttrSet(sprites->handles[0], 0xC, 4);
                
                for (slot = 0; slot < work->field01_bit0; slot++) {
                    HuWinDispOff(activeMG[slot].window_id);
                }
                work->phase = 5;
                work->timer = 0;
                return;
            }
            if (hiliteTimer < hiliteBlinkLen) {
                hiliteTimer++;
            } else {
                hiliteShowF ^= 1;
                hiliteTimer = 0;
            }
            if (hiliteShowF != 0) {
                HuSprAttrReset(sprites->handles[0], 0xC, HUSPR_ATTR_DISPOFF);
                return;
            }
            HuSprAttrSet(sprites->handles[0], 0xC, HUSPR_ATTR_DISPOFF);
            return;
        case 5:
            if (work->timer < 0x5A) {
                OSu8tof32((u8*) &work->timer, &angle);
                scale = (1.0 + (2.0 * sind(angle)));
                work->timer += 4;
                if (work->timer > 0x5A) {
                    work->timer = 0x5A;
                }
                HuSprScaleSet(sprites->handles[0], type_sprite, scale, scale);
                if (work->timer > 0x3C) {
                    HuSprTPLvlSet(sprites->handles[0], type_sprite, (90.0f - work->timer) / 30.0f);
                    return;
                }
            } else {
                HuSprAttrSet(sprites->handles[0], type_sprite, 4);
                
                for (slot = 0; slot < work->field01_bit0; slot++) {
                    HuWinDispOff(activeMG[slot].window_id);
                }
                work->timer = 0;
                work->phase = 6;
                state_work = (MGSetupWork*) mgSetupObj->work;
                state_work->field00_bit1 = 7;
                state_work->phase = 0;
                state_work->timer = 0;
                state_work->angle = 0;
            }
            break;
    }
}

/**
 * @brief Clears all recent-minigame history rings.
 */
void BoardMGSetupPlayClear(void) {
    mgPlay4PLen = mgPlay1Vs3Len = mgPlay2Vs2Len = mgPlayBattleLen = 0;
    memset(mgPlay4P, 0, sizeof(mgPlay4P));
    memset(mgPlay1Vs3, 0, sizeof(mgPlay1Vs3));
    memset(mgPlay2Vs2, 0, sizeof(mgPlay2Vs2));
    memset(mgPlayBattle, 0, sizeof(mgPlayBattle));
}

/**
 * @brief Checks whether a minigame appears in its type's recent-play history.
 *
 * @param minigame_type Minigame category: 0 for four-player, 1 for
 * one-versus-three, 2 for two-versus-two, or 4 for Battle.
 * @param minigame_id Global minigame ID to find.
 *
 * @return 1 if the ID is present, otherwise 0. Unsupported types also return 0.
 */
s32 BoardMGSetupPlaySearch(s32 minigame_type, s16 minigame_id) {
    s16* history;
    s16 slot;
    s16 history_length;

    switch (minigame_type) {
        case 0:
            history_length = ARRAY_COUNT(mgPlay4P);
            history = mgPlay4P;
            break;
        case 1:
            history_length = ARRAY_COUNT(mgPlay1Vs3);
            history = mgPlay1Vs3;
            break;
        case 2:
            history_length = ARRAY_COUNT(mgPlay2Vs2);
            history = mgPlay2Vs2;
            break;
        case 4:
            history_length = ARRAY_COUNT(mgPlayBattle);;
            history = mgPlayBattle;
            break;
        default:
            return 0;
    }
    
    slot = 0;
    while (slot < history_length) {
        if (minigame_id == history[slot]) {
            return 1;
        }
        slot++;
    }
    return 0;
}

/**
 * @brief Adds a minigame to its type's circular recent-play history.
 *
 * @param minigame_type Minigame category: 0 for four-player, 1 for
 * one-versus-three, 2 for two-versus-two, or 4 for Battle. Other values are ignored.
 * @param minigame_id Global minigame ID to record.
 */
void BoardMGSetupPlayPush(s32 minigame_type, s16 minigame_id) {
    s16 history_capacity;
    s16 write_index;
    u8* history_length;
    s16* history;

    switch (minigame_type) {
    case 0:
        history_length = &mgPlay4PLen;
        history_capacity = ARRAY_COUNT(mgPlay4P);
        history = mgPlay4P;
        break;
    case 1:
        history_length = &mgPlay1Vs3Len;
        history_capacity = ARRAY_COUNT(mgPlay1Vs3);
        history = mgPlay1Vs3;
        break;
    case 2:
        history_length = &mgPlay2Vs2Len;
        history_capacity = ARRAY_COUNT(mgPlay2Vs2);
        history = mgPlay2Vs2;
        break;
    case 4:
        history_length = &mgPlayBattleLen;
        history_capacity = ARRAY_COUNT(mgPlayBattle);
        history = mgPlayBattle;
        break;
    default:
        return;
    }
    write_index = *history_length;
    history[write_index] = minigame_id;
    *history_length = write_index + 1;
    if (*history_length >= history_capacity) {
        *history_length = 0;
    }
    return;
}

/**
 * @brief Runs the tutorial version of the minigame setup presentation.
 *
 * @details Creates the same status and roulette UI used by normal selection,
 * waits for each requested animation state, then destroys the object and fades
 * the board filter. It does not load or launch a minigame overlay.
 */
void BoardMGSetupTutorialExec(void) {
    s32 unused_value_a;
    s32 unused_value_b;
    s16 unused_id;
    s16* unused_history;
    s32 unused_result;
    s32 slot;
    MGSetupWork* destroy_work;
    MGSetupWork* unused_work_a;
    MGSetupWork* unused_work_b;
    MGSetupWork* separate_work;
    MGSetupWork* list_work;

    *boardTutorialData = 1;
    BoardFilterFadeInit(30, 0xA0);
    
    for (slot = 0; slot < ARRAY_COUNT(activeMG); slot++) {
        activeMG[slot].window_id = -1;
        activeMG[slot].name_mess = 0;
    }
    
    CreateMGSetup();
    while (setupObjGet() != 7) {
        HuPrcVSleep();
    }
    separate_work = (MGSetupWork*) mgSetupObj->work;
    separate_work->field00_bit1 = 1;
    separate_work->phase = 0;
    separate_work->timer = 0;
    separate_work->angle = 0;
    while (setupObjGet() != 7) {
        HuPrcVSleep();
    }
    list_work = (MGSetupWork*) mgSetupObj->work;
    list_work->field00_bit1 = 6;
    list_work->phase = 0;
    list_work->timer = 0;
    list_work->angle = 0;
    while (setupObjGet() != 7) {
        HuPrcVSleep();
    }
    destroy_work = (MGSetupWork*) mgSetupObj->work;
    destroy_work->field00_bit0 = 1;
    BoardFilterFadeOut(30);
    HuPrcSleep(30);
}
