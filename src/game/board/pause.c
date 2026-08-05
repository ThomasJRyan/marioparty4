#include "game/board/pause.h"
#include "game/audio.h"
#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/roll.h"
#include "game/board/star.h"
#include "game/board/ui.h"
#include "game/data.h"
#include "game/flag.h"
#include "game/gamework_data.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/process.h"
#include "game/saveload.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"

#include "game/board/audio.h"
#include "game/hsfex.h"

typedef struct {
    struct {
        u8 destroy_requested : 1;
        u8 option_active : 1;
        u8 pad_config_closing : 1;
        u8 original_cpu_difficulty : 3;
        u8 save_unavailable : 1;
    };
    s8 state;
    s8 cursor_column;
    s8 cursor_row;
    s8 selection;
    s8 pending_difficulty;
    u8 input_delay;
    u8 transition_frames;
    s8 original_selection;
    u8 repeat_delay;
    s16 write_only_s16;
    u32 previous_input;
} ConfigWork;


static void PauseExit(void);
static void PauseProcess(void);
static void CreatePauseScreen(void);
static void DeletePauseScreen(void);
static void SetBoxVisible(s32 visible);
static void PauseConfigObjFunc(OMOBJ *object);
static void UpdatePauseText(s32 option);
static void InitPauseQuit(OMOBJ *object, ConfigWork *work);
static void PauseQuitProcess(void);
static void DeletePauseQuit(void);
static s32 UpdatePadConfig(OMOBJ *object, ConfigWork *work);
static void CreatePadConfig(OMOBJ *object, ConfigWork *work);
static void ScrollInPadConfig(OMOBJ *object, ConfigWork *work);
static void CursorMovePadConfig(OMOBJ *object, ConfigWork *work);
static void ChangeDiffPadConfig(OMOBJ *object, ConfigWork *work);
static void ScrollOutPadConfig(OMOBJ *object, ConfigWork *work);
static s32 UpdatePauseBox(OMOBJ *object, ConfigWork *work, s32 option);
static s32 UpdatePauseBoxExt(OMOBJ *object, ConfigWork *work, s32 option);
static s32 ExecPauseConfig(OMOBJ *object, ConfigWork *work);
static void CreatePauseControlWin(void);
static void DeletePauseControlWin(void);
static s32 WaitPauseInput(void);
static void ShowPauseConfig(void);
static void CreatePadConfigSprite(void);
static void ShowPadConfigSprite(s32 show);
static void PauseCreateNumber(s32 number_type, s8 value);
static s32 CheckPort(s32 port);
static void RotateBox(float x_rotation, float y_rotation, Mtx matrix);

static float boxRot[8];
static Vec padBoxPos;

static s8 pauseQuitF;
static s8 mainScreenF;
static u8 hostOldLayer;
static s16 hostMdl;
static s8 cursorPos;
static s8 boxState[8];
static OMOBJ *configObj;
static HUPROCESS *quitProcess;
static HUPROCESS *mainProcess;

static s8 pausePad = -1;
static s16 pauseSprGrp = -1;
static s16 padConfigSprGrp = -1;
static s16 pauseCursorPos = -1;
static s16 settingsControlWin = -1;
static s16 controlWin = -1;
static s16 quitWin = -1;
static s16 settingsWin = -1;

static s16 boxModelID[8] = {
    -1, -1, -1, -1, -1, -1, -1, -1
};

static float boxPosTbl[8][2] = {
    { 120.0f, 120.0f },
    { 232.0f, 120.0f },
    { 344.0f, 120.0f },
    { 456.0f, 120.0f },
    { 120.0f, 232.0f },
    { 232.0f, 232.0f },
    { 344.0f, 232.0f },
    { 456.0f, 232.0f }
};

static float padConfigPosTbl[4][2] = {
    { 170.0f, 160.0f },
    { 266.0f, 160.0f },
    { 362.0f, 160.0f },
    { 458.0f, 160.0f }
};

static s32 boardLogoTbl[] = {
    DATA_MAKE_NUM(DATADIR_BOARD, 87),
    DATA_MAKE_NUM(DATADIR_BOARD, 88),
    DATA_MAKE_NUM(DATADIR_BOARD, 89),
    DATA_MAKE_NUM(DATADIR_BOARD, 90),
    DATA_MAKE_NUM(DATADIR_BOARD, 91),
    DATA_MAKE_NUM(DATADIR_BOARD, 92),
    DATA_MAKE_NUM(DATADIR_BOARD, 87),
    DATA_MAKE_NUM(DATADIR_BOARD, 93),
    DATA_MAKE_NUM(DATADIR_BOARD, 94)
};

static float turnDigitPosTbl[] = {
    145.0f, 188.0f, 285.0f, 328.0f
};

static s32 boxMdlTbl[] = {
    DATA_MAKE_NUM(DATADIR_BPAUSE, 8),
    DATA_MAKE_NUM(DATADIR_BPAUSE, 9),
    DATA_MAKE_NUM(DATADIR_BPAUSE, 10),
    DATA_MAKE_NUM(DATADIR_BPAUSE, 11),
    DATA_MAKE_NUM(DATADIR_BPAUSE, 12),
    DATA_MAKE_NUM(DATADIR_BPAUSE, 13),
    DATA_MAKE_NUM(DATADIR_BPAUSE, 14),
    DATA_MAKE_NUM(DATADIR_BPAUSE, 15)
};

/** @brief Starts the board pause menu process and pauses board object updates. */
void BoardPauseStart(void) {
    mainProcess = HuPrcChildCreate(PauseProcess, 0x2001, 0x3800, 0, boardMainProc);
    hostMdl = BoardStarHostMdlGet();
    BoardModelMotionStart(hostMdl, 1, 0x40000001);
    HuPrcVSleep();
    omSysPauseCtrl(1);
    omSysPauseEnable(1);
    HuPrcSetStat(mainProcess, 0xC);
    HuPrcDestructorSet2(mainProcess, PauseExit);
}

/**
 * @brief Restores board state and persists pause-menu settings on exit.
 *
 * @details When quitting the board, audio and confetti are stopped instead of
 * restoring the roll and status displays.
 */
static void PauseExit(void) {
    if (pauseQuitF == 0) {
        BoardRollWinDispSet(1);
        BoardRollDispSet(1);
        BoardStatusItemSet(1);
        BoardRollUPauseSet(1);
        BoardLast5GfxShowSet(1);
        HuAudFXPauseAll(0);
        HuAudSeqPauseAll(0);
    } else {
        BoardConfettiKill();
        BoardAudSeqFadeOutAll();
        HuAudAllStop();
    }
    DeletePauseScreen();
    DeletePauseControlWin();
    BoardEventFlagReset();
    HuDataDirClose(DATADIR_BPAUSE);
    pausePad = -1;
    omSysPauseCtrl(0);
    if (GWMGExplainGet()) {
        _SetFlag(FLAG_ID_MAKE(0, 11));
    } else {
        _ClearFlag(FLAG_ID_MAKE(0, 11));
    }
    if (GWPartyGet() == 0) {
        GWGameStat.story_pause.explain_mg = GWMGExplainGet();
        GWGameStat.story_pause.show_com_mg = GWMGShowComGet();
        GWGameStat.story_pause.mg_list = GWMGListGet();
        GWGameStat.story_pause.mess_speed = GWMessSpeedGet();
        GWGameStat.story_pause.save_mode = GWSaveModeGet();
    } else {
        GWGameStat.party_pause.explain_mg = GWMGExplainGet();
        GWGameStat.party_pause.show_com_mg = GWMGShowComGet();
        GWGameStat.party_pause.mg_list = GWMGListGet();
        GWGameStat.party_pause.mess_speed = GWMessSpeedGet();
        GWGameStat.party_pause.save_mode = GWSaveModeGet();
    }
    mainProcess = NULL;
}

/** @brief Loads pause resources and runs the main pause-menu flow. */
static void PauseProcess(void) {
    s32 data_read_id;

    pauseQuitF = 0;
    mainScreenF = 1;
    BoardFilterFadeInit(30, 0xA0);
    data_read_id = BoardDataDirReadAsync(DATADIR_BPAUSE);
    BoardRollWinDispSet(0);
    BoardRollDispSet(0);
    BoardStatusItemSet(0);
    BoardRollUPauseSet(0);
    BoardLast5GfxShowSet(0);
    HuPadRumbleAllStop();
    HuPrcSleep(1);
    HuAudFXPlay(5);
    while (!BoardFilterFadePauseCheck()) {
        HuPrcVSleep();
    }
    BoardDataAsyncWait(data_read_id);
    CreatePauseScreen();
    CreatePauseControlWin();
    if (WaitPauseInput() == 1) {
        ShowPauseConfig();
    }
    DeletePauseScreen();
    DeletePauseControlWin();
    if (pauseQuitF != 0) {
        HuPrcSleep(20);
        WipeColorSet(0, 0, 0);
        BoardKill();
    } else {
        BoardFilterFadeOut(30);
        HuPrcSleep(30);
    }
    HuPrcEnd();
}

/** @brief Creates the pause-menu sprites, option models, cursor, and host model. */
void CreatePauseScreen(void) {
    Mtx model_matrix;
    Vec screen_pos;
    Vec world_pos;
    float model_scale;
    float option_rotation;
    void *object_manager_data;
    OMOBJ *object;
    ConfigWork *work;
    s16 sprite;
    s32 option_state;
    s32 i;

    object_manager_data = boardObjMan->user_data;
    object = omAddObjEx(boardObjMan, 0x108, 0, 0, -1, PauseConfigObjFunc);
    configObj = object;
    work = omObjGetWork(object, ConfigWork);
    work->destroy_requested = 0;
    work->cursor_column = 0;
    work->cursor_row = 0;
    work->input_delay = 0;
    work->state = 0;
    work->option_active = 0;
    work->pad_config_closing = 0;
    work->write_only_s16 = 0;
    work->save_unavailable = (SLSaveFlagGet() != 0) ? 0 : 1;
    work->repeat_delay = 0;
    work->previous_input = 0;
    cursorPos = work->cursor_column + work->cursor_row * 4;
    object->trans.x = -40.0f + boxPosTbl[cursorPos][0];
    object->trans.y = -32.0f + boxPosTbl[cursorPos][1];
    pauseCursorPos = HuSprGrpCreate(2);
    HuSprGrpPosSet(pauseCursorPos, 0.0f, 0.0f);
    BoardSpriteCreate(DATA_MAKE_NUM(DATADIR_BOARD, 85), 700, 0, &sprite);
    HuSprGrpMemberSet(pauseCursorPos, 0, sprite);
    HuSprZRotSet(pauseCursorPos, 0, 45.0f);
    HuSprAttrSet(pauseCursorPos, 0, 4);
    HuSprAttrSet(pauseCursorPos, 0, 8);
    HuSprPosSet(pauseCursorPos, 0, object->trans.x, object->trans.y);
    BoardSpriteCreate(DATA_MAKE_NUM(DATADIR_BPAUSE, 7), 750, 0, &sprite);
    HuSprGrpMemberSet(pauseCursorPos, 1, sprite);
    HuSprAttrSet(pauseCursorPos, 1, 4);
    HuSprAttrSet(pauseCursorPos, 1, 8);
    pauseSprGrp = HuSprGrpCreate(7);
    HuSprGrpPosSet(pauseSprGrp, 0.0f, 0.0f);
    BoardSpriteCreate(boardLogoTbl[GWBoardGet()], 900, 0, &sprite);
    HuSprGrpMemberSet(pauseSprGrp, 0, sprite);
    HuSprPosSet(pauseSprGrp, 0, 288.0f, 120.0f);
    HuSprAttrSet(pauseSprGrp, 0, 4);
    HuSprAttrSet(pauseSprGrp, 0, 8);
    BoardSpriteCreate(DATA_MAKE_NUM(DATADIR_BPAUSE, 5), 900, 0, &sprite);
    HuSprGrpMemberSet(pauseSprGrp, 1, sprite);
    HuSprPosSet(pauseSprGrp, 1, 288.0f, 302.0f);
    HuSprAttrSet(pauseSprGrp, 1, 8);
    BoardSpriteCreate(DATA_MAKE_NUM(DATADIR_BPAUSE, 4), 800, 0, &sprite);
    HuSprGrpMemberSet(pauseSprGrp, 2, sprite);
    HuSprZRotSet(pauseSprGrp, 2, -30.0f);
    HuSprPosSet(pauseSprGrp, 2, 87.0f, 245.0f);
    HuSprAttrSet(pauseSprGrp, 2, 8);
    if (GWBoardGet() == 7 || GWBoardGet() == 8) {
        HuSprAttrSet(pauseSprGrp, 2, 4);
    }
    if (GWPartyGet() == 1) {
        if (GWTeamGet()) {
            HuSprBankSet(pauseSprGrp, 2, 2);
        } else {
            HuSprBankSet(pauseSprGrp, 2, 0);
        }
    } else {
        HuSprBankSet(pauseSprGrp, 2, 1);
    }
    for (i = 0; i < 4; i++) {
        BoardSpriteCreate(DATA_MAKE_NUM(DATADIR_BPAUSE, 6), 800, 0, &sprite);
        HuSprGrpMemberSet(pauseSprGrp, i + 3, sprite);
        HuSprPosSet(pauseSprGrp, i + 3, turnDigitPosTbl[i], 301.0f);
        HuSprAttrSet(pauseSprGrp, i + 3, 8);
        HuSprAttrSet(pauseSprGrp, i + 3, 1);
    }
    padConfigSprGrp = HuSprGrpCreate(13);
    HuSprGrpPosSet(padConfigSprGrp, 0.0f, 0.0f);
    for (i = 0; i < 4; i++) {
        BoardSpriteCreate(DATA_MAKE_NUM(DATADIR_BPAUSE, 0), 910, 0, &sprite);
        HuSprGrpMemberSet(padConfigSprGrp, i, sprite);
        HuSprAttrSet(padConfigSprGrp, i, 8);
        HuSprAttrSet(padConfigSprGrp, i, 1);
        BoardSpriteCreate(DATA_MAKE_NUM(DATADIR_BPAUSE, 1), 700, 0, &sprite);
        HuSprGrpMemberSet(padConfigSprGrp, i + 4, sprite);
        HuSprAttrSet(padConfigSprGrp, i + 4, 8);
        HuSprAttrSet(padConfigSprGrp, i + 4, 1);
        BoardSpriteCreate(DATA_MAKE_NUM(DATADIR_BPAUSE, 2), 800, 0, &sprite);
        HuSprGrpMemberSet(padConfigSprGrp, i + 8, sprite);
        HuSprAttrSet(padConfigSprGrp, i + 8, 8);
        HuSprAttrSet(padConfigSprGrp, i + 8, 1);
    }
    BoardSpriteCreate(DATA_MAKE_NUM(DATADIR_BPAUSE, 3), 1000, 0, &sprite);
    HuSprGrpMemberSet(padConfigSprGrp, 12, sprite);
    HuSprAttrSet(padConfigSprGrp, 12, 4);
    HuSprDrawNoSet(padConfigSprGrp, 12, 0x40);
    CreatePadConfigSprite();
    ShowPadConfigSprite(0);
    hostOldLayer = BoardModelLayerGet(hostMdl);
    BoardModelLayerSet(hostMdl, 6);
    screen_pos.x = 68.0f;
    if (GWBoardGet() == 3) {
        screen_pos.y = 464.0f;
    } else {
        screen_pos.y = 434.0f;
    }
    screen_pos.z = 1100.0f;
    Hu3D2Dto3D(&screen_pos, 1, &world_pos);
    RotateBox(0.0f, 15.0f, model_matrix);
    BoardModelMtxSet(hostMdl, &model_matrix);
    BoardModelPosSetV(hostMdl, &world_pos);
    BoardModelVisibilitySet(hostMdl, 0);
    BoardModelRotSet(hostMdl, 0.0f, 0.0f, 0.0f);
    for (i = 0; i < 8; i++) {
        boxModelID[i] = BoardModelCreate(boxMdlTbl[i], NULL, 0);
        screen_pos.x = boxPosTbl[i][0];
        screen_pos.y = boxPosTbl[i][1];
        screen_pos.z = 700.0f;
        Hu3D2Dto3D(&screen_pos, 1, &world_pos);
        model_scale = 0.5f;
        BoardModelPosSetV(boxModelID[i], &world_pos);
        BoardModelLayerSet(boxModelID[i], 6);
        BoardModelScaleSet(boxModelID[i], model_scale, model_scale, model_scale);
        switch (i) {
            case 0:
            case 7:
                option_state = 0;
                option_rotation = 0.0f;
                break;
            case 1:
                option_state = GWMGExplainGet();
                option_rotation = (option_state != 0) ? 0.0f : -90.0f;
                break;
            case 2:
                option_state = GWMGShowComGet();
                option_rotation = (option_state != 0) ? 0.0f : -90.0f;
                break;
            case 3:
                option_state = GWMGListGet();
                switch (option_state) {
                    case 1:
                        option_state = 0;
                        option_rotation = 90.0f;
                        break;
                    case 0:
                        option_state = 1;
                        option_rotation = 0.0f;
                        break;
                    case 2:
                        option_state = 2;
                        option_rotation = -90.0f;
                        break;
                }
                break;
            case 4:
                option_state = GWRumbleGet();
                option_rotation = (option_state != 0) ? 0.0f : -90.0f;
                break;
            case 5:
                option_state = GWMessSpeedGet();
                switch (option_state) {
                    case 2:
                        option_state = 0;
                        option_rotation = 90.0f;
                        break;
                    case 1:
                        option_state = 1;
                        option_rotation = 0.0f;
                        break;
                    case 0:
                        option_state = 2;
                        option_rotation = -90.0f;
                        break;
                }
                break;
            case 6:
                if (work->save_unavailable != 0) {
                    GWSystem.save_mode = 1;
                }
                option_state = GWSaveModeGet();
                switch (option_state) {
                    case 1:
                        option_state = 0;
                        option_rotation = 90.0f;
                        break;
                    case 0:
                        option_state = 1;
                        option_rotation = 0.0f;
                        break;
                    case 2:
                        option_state = 2;
                        option_rotation = -90.0f;
                        break;
                }
                break;
        }
        boxState[i] = option_state;
        boxRot[i] = option_rotation;
        RotateBox(3.5f, option_rotation, model_matrix);
        BoardModelMtxSet(boxModelID[i], &model_matrix);
        BoardModelRotSet(boxModelID[i], 0.0f, 0.0f, 0.0f);
    }
    SetBoxVisible(0);
}

/** @brief Destroys pause-screen objects and restores the host model. */
static void DeletePauseScreen(void) {
    ConfigWork *work;
    Mtx identity_matrix;
    s32 i;

    if (configObj) {
        work = omObjGetWork(configObj, ConfigWork);
        work->destroy_requested = 1;
    }
    for (i = 0; i < 8; i++) {
        BoardModelKill(boxModelID[i]);
        boxModelID[i] = -1;
    }
    if (pauseSprGrp != -1) {
        HuSprGrpKill(pauseSprGrp);
        pauseSprGrp = -1;
    }
    if (padConfigSprGrp != -1) {
        HuSprGrpKill(padConfigSprGrp);
        padConfigSprGrp = -1;
    }
    if (pauseCursorPos != -1) {
        HuSprGrpKill(pauseCursorPos);
        pauseCursorPos = -1;
    }
    PSMTXIdentity(identity_matrix);
    BoardModelLayerSet(hostMdl, hostOldLayer);
    BoardModelVisibilitySet(hostMdl, 1);
    BoardModelMtxSet(hostMdl, &identity_matrix);
    BoardStarHostSet(hostMdl);
    if (GWBoardGet() == 7 || GWBoardGet() == 8) {
        BoardModelVisibilitySet(hostMdl, 0);
    }
}

/**
 * @brief Sets the visibility of every pause option-box model.
 *
 * @param visible Nonzero to show the models; zero to hide them.
 */
static void SetBoxVisible(s32 visible) {
    s32 i;

    for (i = 0; i < 8; i++) {
        BoardModelVisibilitySet(boxModelID[i], visible);
    }
}

/**
 * @brief Updates the settings cursor and dispatches the selected option editor.
 *
 * @param object Pause configuration object whose work tracks menu state.
 */
static void PauseConfigObjFunc(OMOBJ *object) {
    ConfigWork *work;
    s16 previous_cursor_pos;
    s32 unused_config_value;

    work = omObjGetWork(object, ConfigWork);
    if (work->destroy_requested != 0 || BoardIsKill()) {
        configObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), object);
        return;
    }
    if (mainScreenF == 0) {
        if (work->input_delay != 0) {
            work->input_delay--;
            return;
        }
        cursorPos = work->cursor_column + work->cursor_row * 4;
        previous_cursor_pos = cursorPos;
        if (work->option_active != 0) {
            switch (cursorPos) {
                case 0:
                    if (UpdatePadConfig(object, work) != 0) {
                        UpdatePauseText(cursorPos);
                    }
                    break;
                case 1:
                    if (UpdatePauseBox(object, work, 1) != 0) {
                        GWMGExplainSet(boxState[1]);
                    }
                    UpdatePauseText(cursorPos);
                    break;
                case 2:
                    if (UpdatePauseBox(object, work, 2) != 0) {
                        GWMGShowComSet(boxState[2]);
                    }
                    UpdatePauseText(cursorPos);
                    break;
                case 3:
                    if (UpdatePauseBoxExt(object, work, 3) != 0) {
                        switch (boxState[3]) {
                            case 0:
                                GWMGListSet(1);
                                break;
                            case 1:
                                GWMGListSet(0);
                                break;
                            case 2:
                                GWMGListSet(2);
                                break;
                        }
                    }
                    UpdatePauseText(cursorPos);
                    break;
                case 4:
                    if (UpdatePauseBox(object, work, 4) != 0) {
                        GWRumbleSet(boxState[4]);
                    }
                    UpdatePauseText(cursorPos);
                    break;
                case 5:
                    if (UpdatePauseBoxExt(object, work, 5) != 0) {
                        switch (boxState[5]) {
                            case 0:
                                GWMessSpeedSet(2);
                                break;
                            case 1:
                                GWMessSpeedSet(1);
                                break;
                            case 2:
                                GWMessSpeedSet(0);
                                break;
                        }
                    }
                    UpdatePauseText(cursorPos);
                    break;
                case 6:
                    if (work->save_unavailable != 0) {
                        work->option_active = 0;
                        break;
                    }
                    if (UpdatePauseBoxExt(object, work, 6) != 0) {
                        switch (boxState[6]) {
                            case 0:
                                GWSaveModeSet(1);
                                break;
                            case 1:
                                GWSaveModeSet(0);
                                break;
                            case 2:
                                GWSaveModeSet(2);
                                break;
                        }
                    }
                    UpdatePauseText(cursorPos);
                    break;
                case 7:
                    InitPauseQuit(object, work);
                    break;
            }
        } else {
            cursorPos = ExecPauseConfig(object, work);
            if (previous_cursor_pos != cursorPos) {
                object->trans.x = -40.0f + boxPosTbl[cursorPos][0];
                object->trans.y = -32.0f + boxPosTbl[cursorPos][1];
            }
            UpdatePauseText(cursorPos);
            HuSprPosSet(pauseCursorPos, 0, object->trans.x, object->trans.y);
        }
    }
}

/**
 * @brief Updates the settings description for a pause-menu option.
 *
 * @param option Option index in the two-row pause settings grid.
 */
static void UpdatePauseText(s32 option) {
    s32 player_type_message;
    s32 j;
    s32 i;

    switch (option) {
        case 0:
            HuWinMesSet(settingsWin, MAKE_MESSID(16, 28));
            for (i = 0; i < 4; i++) {
                for (j = 0; j < 4; j++) {
                    if (i == GWPlayer[j].port) {
                        if (GWPlayer[j].com) {
                            player_type_message = MAKE_MESSID(16, 30);
                        } else {
                            player_type_message = MAKE_MESSID(16, 29);
                        }
                        HuWinInsertMesSet(settingsWin, player_type_message, i);
                        break;
                    }
                }
            }
            break;
        case 1:
            if (boxState[option] != 0) {
                HuWinMesSet(settingsWin, MAKE_MESSID(16, 33));
            } else {
                HuWinMesSet(settingsWin, MAKE_MESSID(16, 34));
            }
            break;
        case 2:
            if (boxState[option] != 0) {
                HuWinMesSet(settingsWin, MAKE_MESSID(16, 35));
            } else {
                HuWinMesSet(settingsWin, MAKE_MESSID(16, 36));
            }
            break;
        case 3:
            switch (boxState[option]) {
                case 0:
                    HuWinMesSet(settingsWin, MAKE_MESSID(16, 38));
                    break;
                case 1:
                    HuWinMesSet(settingsWin, MAKE_MESSID(16, 37));
                    break;
                case 2:
                    HuWinMesSet(settingsWin, MAKE_MESSID(16, 39));
                    break;
            }
            break;
        case 4:
            if (boxState[option] != 0) {
                HuWinMesSet(settingsWin, MAKE_MESSID(16, 40));
            } else {
                HuWinMesSet(settingsWin, MAKE_MESSID(16, 41));
            }
            break;
        case 5:
            switch (boxState[option]) {
                case 0:
                    HuWinMesSet(settingsWin, MAKE_MESSID(16, 44));
                    break;
                case 1:
                    HuWinMesSet(settingsWin, MAKE_MESSID(16, 43));
                    break;
                case 2:
                    HuWinMesSet(settingsWin, MAKE_MESSID(16, 42));
                    break;
            }
            break;
        case 6:
            switch (boxState[option]) {
                case 0:
                    HuWinMesSet(settingsWin, MAKE_MESSID(16, 47));
                    break;
                case 1:
                    HuWinMesSet(settingsWin, MAKE_MESSID(16, 45));
                    break;
                case 2:
                    HuWinMesSet(settingsWin, MAKE_MESSID(16, 46));
                    break;
            }
            break;
        case 7:
            HuWinMesSet(settingsWin, MAKE_MESSID(16, 48));
            break;
    }
}

/**
 * @brief Advances entry to and return from the quit-confirmation dialog.
 *
 * @param object Configuration object retained while the dialog runs.
 * @param work Work state for the active pause-menu option.
 */
static void InitPauseQuit(OMOBJ *object, ConfigWork *work) {
    float model_scale;

    switch (work->state) {
        case 0:
            SetBoxVisible(0);
            quitProcess = HuPrcChildCreate(PauseQuitProcess, 0x2001, 0x3800, 0, boardMainProc);
            HuPrcSetStat(quitProcess, 0xC);
            HuPrcDestructorSet2(quitProcess, DeletePauseQuit);
            work->state = 1;
            work->input_delay = 10;
            HuSprAttrSet(pauseCursorPos, 0, 4);
            HuSprAttrSet(pauseCursorPos, 1, 4);
            break;
        case 1:
            if (quitWin != -1 && HuWinStatGet(quitWin) == 3) {
                if (HuWinChoiceNowGet(quitWin) != 0) {
                    HuWinMesSet(settingsWin, MAKE_MESSID(16, 51));
                } else {
                    HuWinMesSet(settingsWin, MAKE_MESSID(16, 50));
                }
            }
            if (!quitProcess) {
                work->state = 2;
                if (pauseQuitF != 0) {
                    work->destroy_requested = 1;
                    break;
                }
            }
            break;
        case 2:
            work->option_active = 0;
            work->state = 0;
            work->selection = 0;
            work->pending_difficulty = 0;
            work->input_delay = 0;
            SetBoxVisible(1);
            model_scale = 0.5f;
            BoardModelScaleSet(boxModelID[7], model_scale, model_scale, model_scale);
            HuSprAttrReset(pauseCursorPos, 0, HUSPR_ATTR_DISPOFF);
            if (GWPartyGet() == 0) {
                HuSprAttrReset(pauseCursorPos, 1, HUSPR_ATTR_DISPOFF);
            }
            break;
    }
}

/** @brief Displays the board-quit confirmation and records an accepted quit. */
static void PauseQuitProcess(void) {
    WindowData *window_data;
    float window_pos[2];
    float message_size[2];

    HuWinMesMaxSizeGet(1, message_size, MAKE_MESSID(16, 49));
    window_pos[0] = -10000.0f;
    window_pos[1] = 140.0f;
    quitWin = HuWinExCreate(window_pos[0], window_pos[1], message_size[0], message_size[1], -1);
    window_data = &winData[quitWin];
    window_data->active_pad = (1 << pausePad);
    HuWinExAnimIn(quitWin);
    HuWinMesSpeedSet(quitWin, 0);
    HuWinMesSet(quitWin, MAKE_MESSID(16, 49));
    HuWinMesWait(quitWin);
    if (HuWinChoiceGet(quitWin, 0) == 1) {
        HuPrcSleep(60);
        pauseQuitF = 1;
        _SetFlag(0x1001B);
    }
    HuPrcEnd();
}

/** @brief Cleans up the quit dialog process and its window. */
static void DeletePauseQuit(void) {
    if (quitWin != -1) {
        HuWinExCleanup(quitWin);
        quitWin = -1;
    }
    HuDataDirClose(DATADIR_BPAUSE);
    quitProcess = NULL;
}

/**
 * @brief Advances the controller-to-player configuration state machine.
 *
 * @param object Configuration object used for transition positions.
 * @param work Work state for the controller configuration screen.
 * @return `1` after the controller editor closes; otherwise `0`.
 */
static s32 UpdatePadConfig(OMOBJ *object, ConfigWork *work) {
    switch (work->state) {
        case 0:
            CreatePadConfig(object, work);
            break;
        case 1:
            ScrollInPadConfig(object, work);
            break;
        case 2:
            CursorMovePadConfig(object, work);
            break;
        case 3:
            ChangeDiffPadConfig(object, work);
            break;
        case 4:
            ScrollOutPadConfig(object, work);
            break;
    }
    if (work->option_active != 0) {
        return 0;
    } else {
        return 1;
    }
}

/**
 * @brief Begins the transition from the option grid to controller assignment.
 * @param object Object used to store per-frame panel movement deltas.
 * @param work Work state receiving the transition timer and initial port.
 */
static void CreatePadConfig(OMOBJ *object, ConfigWork *work) {
    Vec target_pos;
    Vec movement_delta;
    s32 i;

    HuSprAttrSet(pauseCursorPos, 0, 4);
    for (i = 0; i < 8; i++) {
        if (i != 0) {
            BoardModelVisibilitySet(boxModelID[i], 0);
        }
    }
    object->rot.x = 576.0f;
    object->rot.y = 112.0f;
    object->rot.z = (32.0f - object->rot.x) / 20.0f;
    work->transition_frames = 20;
    ShowPadConfigSprite(1);
    HuSprGrpPosSet(padConfigSprGrp, object->rot.x, object->rot.y);
    work->selection = 0;
    work->pending_difficulty = 0;
    target_pos.x = padConfigPosTbl[work->selection][0];
    target_pos.y = padConfigPosTbl[work->selection][1];
    HuSprPosSet(pauseCursorPos, 0, target_pos.x, target_pos.y);
    HuSprZRotSet(pauseCursorPos, 0, 0.0f);
    padBoxPos.x = boxPosTbl[0][0];
    padBoxPos.y = boxPosTbl[0][1];
    padBoxPos.z = 700.0f;
    target_pos.x = 96.0f;
    target_pos.y = 176.0f;
    target_pos.z = padBoxPos.z;
    VECSubtract(&target_pos, &padBoxPos, &movement_delta);
    object->scale.x = movement_delta.x / 20.0f;
    object->scale.y = movement_delta.y / 20.0f;
    work->state = 1;
}

/**
 * @brief Advances the controller panel's 20-frame slide transition.
 * @param object Object carrying the panel's position increments.
 * @param work Work state carrying remaining frames and closing direction.
 */
static void ScrollInPadConfig(OMOBJ *object, ConfigWork *work) {
    Vec world_pos;
    s32 i;

    if (work->transition_frames == 0) {
        HuSprAttrReset(pauseCursorPos, 0, HUSPR_ATTR_DISPOFF);
        if (work->pad_config_closing != 0) {
            for (i = 0; i < 8; i++) {
                if (i != 0) {
                    BoardModelVisibilitySet(boxModelID[i], 1);
                }
            }
            work->option_active = 0;
            work->pad_config_closing = 0;
            work->selection = 0;
            work->pending_difficulty = 0;
            work->state = 0;
        } else {
            work->state = 2;
        }
        return;
    }
    object->rot.x += object->rot.z;
    HuSprGrpPosSet(padConfigSprGrp, object->rot.x, object->rot.y);
    padBoxPos.x += object->scale.x;
    padBoxPos.y += object->scale.y;
    Hu3D2Dto3D(&padBoxPos, 1, &world_pos);
    BoardModelPosSetV(boxModelID[0], &world_pos);
    work->transition_frames--;
}

/**
 * @brief Moves among controller ports and opens difficulty selection.
 * @param object Active configuration object; retained for the state machine.
 * @param work Work state containing the selected controller port.
 */
static void CursorMovePadConfig(OMOBJ *object, ConfigWork *work) {
    float cursor_position[4];
    s32 player;
    u32 input;

    HuWinMesSet(settingsWin, MAKE_MESSID(16, 31));
    input = HuPadDStkRep[pausePad] | HuPadBtnDown[pausePad];
    player = CheckPort(work->selection);
    if (input == 0x100) {
        work->state = 3;
        if (GWPlayer[player].com) {
            work->original_cpu_difficulty = GWPlayer[player].diff + 1;
            work->pending_difficulty = GWPlayer[player].diff + 1;
        } else {
            work->original_cpu_difficulty = 0;
            work->pending_difficulty = 0;
        }
        HuSprBankSet(padConfigSprGrp, work->selection + 8, 1);
        HuAudFXPlay(2);
    } else if (input == 0x200) {
        HuAudFXPlay(3);
        work->state = 4;
    } else {
        if (input == 1) {
            work->selection--;
        }
        if (input == 2) {
            work->selection++;
        }
        if (work->selection < 0) {
            work->selection = 0;
        } else if (work->selection >= 4) {
            work->selection = 3;
        } else if (input == 1 || input == 2) {
            HuAudFXPlay(0);
            work->input_delay = 4;
        }
        cursor_position[0] = padConfigPosTbl[work->selection][0];
        cursor_position[1] = padConfigPosTbl[work->selection][1];
        HuSprPosSet(pauseCursorPos, 0, cursor_position[0], cursor_position[1]);
    }
}

/**
 * @brief Edits human/CPU status and CPU difficulty for a controller port.
 * @param object Active configuration object; retained for the state machine.
 * @param work Work state containing the port and pending difficulty choice.
 */
static void ChangeDiffPadConfig(OMOBJ *object, ConfigWork *work) {
    s32 player;
    s32 selection_delta;
    s32 difficulty_sprite_bank;
    s32 max_difficulty;
    u32 direction_input;

    direction_input = 0;
    if (HuPadStkX[pausePad] < -20) {
        direction_input |= 1;
    } else if (HuPadStkX[pausePad] > 20) {
        direction_input |= 2;
    }
    player = CheckPort(work->selection);
    if (GWGameStat.veryHardUnlock != 0) {
        max_difficulty = 4;
    } else {
        max_difficulty = 3;
    }
    HuWinMesSet(settingsWin, MAKE_MESSID(16, 32));
    if (direction_input != 0) {
        if (work->previous_input != 0 && work->previous_input == direction_input) {
            if (work->repeat_delay++ < 10) {
                return;
            }
        } else {
            if (work->repeat_delay++ < 2) {
                return;
            }
            work->previous_input = direction_input;
        }
    } else {
        work->previous_input = 0;
    }
    work->repeat_delay = 0;
    selection_delta = 0;
    if (direction_input == 1) {
        selection_delta = -1;
    }
    if (direction_input == 2) {
        selection_delta = 1;
    }
    if (selection_delta != 0) {
        HuAudFXPlay(0);
        work->input_delay = 4;
    } else {
        if (HuPadBtnDown[pausePad] == 0x100) {
            work->state = 2;
            if (work->pending_difficulty != 0) {
                GWPlayer[player].com = 1;
                GWPlayerCfg[player].iscom = 1;
                GWPlayer[player].diff = work->pending_difficulty - 1;
                GWPlayerCfg[player].diff = work->pending_difficulty - 1;
            } else {
                GWPlayer[player].com = 0;
                GWPlayerCfg[player].iscom = 0;
            }
            HuSprBankSet(padConfigSprGrp, work->selection + 8, 0);
            HuAudFXPlay(2);
            work->previous_input = 0x100;
        }
        if (HuPadBtnDown[pausePad] == 0x200) {
            work->state = 2;
            work->pending_difficulty = work->original_cpu_difficulty;
            HuSprBankSet(padConfigSprGrp, work->selection + 8, 0);
            HuAudFXPlay(3);
            work->previous_input = 0x200;
        }
        work->repeat_delay = 0;
    }
    work->pending_difficulty += selection_delta;
    if (GWGameStat.veryHardUnlock == 0 && work->pending_difficulty == 4) {
        work->pending_difficulty += selection_delta;
    }
    if (work->pending_difficulty > max_difficulty) {
        work->pending_difficulty = 0;
    } else if (work->pending_difficulty < 0) {
        work->pending_difficulty = max_difficulty;
    }
    if (work->pending_difficulty != 0) {
        difficulty_sprite_bank = work->pending_difficulty + 3;
    } else {
        difficulty_sprite_bank = work->selection;
    }
    HuSprBankSet(padConfigSprGrp, work->selection + 4, difficulty_sprite_bank);
}

/**
 * @brief Configures the controller panel's slide back to the option grid.
 * @param object Object receiving the per-frame return movement.
 * @param work Work state marked as closing for the shared slide updater.
 */
static void ScrollOutPadConfig(OMOBJ *object, ConfigWork *work) {
    Vec target_pos;
    Vec movement_delta;

    HuSprAttrSet(pauseCursorPos, 0, 4);
    object->rot.x = 32.0f;
    object->rot.y = 112.0f;
    object->rot.z = (608.0f - object->rot.x) / 20.0f;
    work->transition_frames = 0x14;
    ShowPadConfigSprite(1);
    HuSprGrpPosSet(padConfigSprGrp, object->rot.x, object->rot.y);
    target_pos.x = -40.0f + boxPosTbl[0][0];
    target_pos.y = -32.0f + boxPosTbl[0][1];
    HuSprPosSet(pauseCursorPos, 0, target_pos.x, target_pos.y);
    HuSprZRotSet(pauseCursorPos, 0, 45.0f);
    target_pos.x = boxPosTbl[0][0];
    target_pos.y = boxPosTbl[0][1];
    target_pos.z = 700.0f;
    VECSubtract(&target_pos, &padBoxPos, &movement_delta);
    object->scale.x = movement_delta.x / 20.0f;
    object->scale.y = movement_delta.y / 20.0f;
    work->state = 1;
    work->pad_config_closing = 1;
}

/**
 * @brief Edits a two-state pause option and animates its box rotation.
 * @param object Configuration object storing rotation state.
 * @param work Work state for the active option editor.
 * @param option Option-box index to edit.
 * @return `1` once the editor has closed; otherwise `0`.
 */
static s32 UpdatePauseBox(OMOBJ *object, ConfigWork *work, s32 option) {
    float model_scale;
    float target_rotation;
    float current_rotation;
    s32 previous_selection;
    u32 input;
    Mtx model_matrix;

    switch (work->state) {
        case 0:
            work->original_selection = work->selection = boxState[option];
            object->rot.x = boxRot[option];
            object->rot.y = 0.0f;
            work->state = 1;
            model_scale = 0.7f;
            BoardModelScaleSet(boxModelID[option], model_scale, model_scale, model_scale);
            return 0;
        case 1:
            input = HuPadDStk[pausePad] | HuPadBtnDown[pausePad];
            previous_selection = work->selection;
            if (input == 1) {
                work->selection++;
            }
            if (input == 2) {
                work->selection--;
            }
            if (work->selection < 0) {
                work->selection = 0;
            } else if (work->selection > 1) {
                work->selection = 1;
            } else if (input == 1 || input == 2) {
                HuAudFXPlay(0);
            }
            if (input == 0x100) {
                work->state = 3;
                HuAudFXPlay(2);
            } else if (input == 0x200) {
                work->state = 4;
                HuAudFXPlay(3);
            } else if (previous_selection != work->selection) {
                if (work->selection != 0) {
                    target_rotation = 0.0f;
                } else {
                    target_rotation = -90.0f;
                }
                current_rotation = object->rot.x;
                object->rot.y = (target_rotation - current_rotation) / 30.0f;
                work->transition_frames = 30;
                work->state = 2;
            }
            break;
        case 2:
            if (work->transition_frames != 0) {
                object->rot.x += object->rot.y;
                work->transition_frames--;
            } else {
                work->state = 1;
            }
            break;
        case 3:
            boxState[option] = work->selection;
            boxRot[option] = object->rot.x;
            work->original_selection = -1;
            work->state = 5;
            work->write_only_s16 = 0;
            break;
        case 4:
            boxState[option] = work->original_selection;
            work->selection = boxState[option];
            object->rot.x = boxRot[option];
            work->state = 5;
            work->write_only_s16 = 0;
            break;
        case 5:
            model_scale = 0.5f;
            BoardModelScaleSet(boxModelID[option], model_scale, model_scale, model_scale);
            work->option_active = 0;
            work->selection = 0;
            work->pending_difficulty = 0;
            work->state = 0;
            return 1;
    }
    RotateBox(3.5f, object->rot.x, model_matrix);
    BoardModelMtxSet(boxModelID[option], &model_matrix);
    boxState[option] = work->selection;
    return 0;
}

/**
 * @brief Edits a two- or three-state option and animates its box rotation.
 * @param object Configuration object storing rotation state.
 * @param work Work state for the active option editor.
 * @param option Option-box index; index 3 has two states without custom packs.
 * @return `1` once the editor has closed; otherwise `0`.
 */
static s32 UpdatePauseBoxExt(OMOBJ *object, ConfigWork *work, s32 option) {
    float model_scale;
    float target_rotation;
    float current_rotation;
    s32 previous_selection;
    s32 max_selection;
    u32 input;
    Mtx model_matrix;

    if (option == 3 && GWGameStat.customPackEnable == 0) {
        max_selection = 1;
    } else {
        max_selection = 2;
    }
    switch (work->state) {
        case 0:
            work->original_selection = work->selection = boxState[option];
            object->rot.x = boxRot[option];
            object->rot.y = 0.0f;
            work->state = 1;
            model_scale = 0.7f;
            BoardModelScaleSet(boxModelID[option], model_scale, model_scale, model_scale);
            return 0;
        case 1:
            input = HuPadDStk[pausePad] | HuPadBtnDown[pausePad];
            previous_selection = work->selection;
            if (input == 1) {
                work->selection--;
            }
            if (input == 2) {
                work->selection++;
            }
            if (work->selection < 0) {
                work->selection = 0;
            } else if (work->selection > max_selection) {
                work->selection = max_selection;
            } else if (input == 1 || input == 2) {
                HuAudFXPlay(0);
            }
            if (input == 0x100) {
                work->state = 3;
                HuAudFXPlay(2);
            } else if (input == 0x200) {
                work->state = 4;
                HuAudFXPlay(3);
            } else if (previous_selection != work->selection) {
                switch (work->selection) {
                    case 0:
                        target_rotation = 90.0f;
                        break;
                    case 1:
                        target_rotation = 0.0f;
                        break;
                    case 2:
                        target_rotation = -90.0f;
                        break;
                }
                current_rotation = object->rot.x;
                object->rot.y = (target_rotation - current_rotation) / 30.0f;
                work->transition_frames = 30;
                work->state = 2;
            }
            break;
        case 2:
            if (work->transition_frames != 0) {
                object->rot.x += object->rot.y;
                work->transition_frames--;
            } else {
                work->state = 1;
            }
            break;
        case 3:
            boxState[option] = work->selection;
            boxRot[option] = object->rot.x;
            work->state = 5;
            work->write_only_s16 = 0;
            work->original_selection = -1;
            break;
        case 4:
            boxState[option] = work->original_selection;
            work->selection = boxState[option];
            object->rot.x = boxRot[option];
            work->state = 5;
            work->write_only_s16 = 0;
            break;
        case 5:
            model_scale = 0.5f;
            BoardModelScaleSet(boxModelID[option], model_scale, model_scale, model_scale);
            work->option_active = 0;
            work->selection = 0;
            work->pending_difficulty = 0;
            work->state = 0;
            return 1;
    }
    RotateBox(3.5f, object->rot.x, model_matrix);
    BoardModelMtxSet(boxModelID[option], &model_matrix);
    boxState[option] = work->selection;
    return 0;
}

/**
 * @brief Handles navigation and selection in the two-row settings grid.
 * @param object Configuration object whose cursor position is updated by caller.
 * @param work Work state containing the grid coordinates and availability flags.
 * @return Selected option index in the range 0 through 7.
 */
static s32 ExecPauseConfig(OMOBJ *object, ConfigWork *work) {
    s32 selection_allowed;
    s32 selection_blocked;
    s32 column_delta;
    s32 row_delta;
    u32 input;

    input = 0;
    input = HuPadBtnDown[pausePad];
    input |= HuPadDStkRep[pausePad];
    column_delta = row_delta = 0;
    if (input == 8) {
        row_delta = -1;
    }
    if (input == 4) {
        row_delta = 1;
    }
    if (input == 1) {
        column_delta = -1;
    }
    if (input == 2) {
        column_delta = 1;
    }
    work->cursor_column += column_delta;
    work->cursor_row += row_delta;
    if (work->cursor_row < 0) {
        work->cursor_row = 0;
    } else if (work->cursor_row >= 2) {
        work->cursor_row = 1;
    } else if (row_delta != 0) {
        HuAudFXPlay(0);
        work->input_delay = 4;
    }
    if (work->cursor_column < 0) {
        work->cursor_column = 0;
    } else if (work->cursor_column >= 4) {
        work->cursor_column = 3;
    } else if (column_delta != 0) {
        HuAudFXPlay(0);
        work->input_delay = 4;
    }
    if (input == 0x100) {
        selection_allowed = 1;
        selection_blocked = 0;
        switch (work->cursor_column + work->cursor_row * 4) {
            case 6:
                if (work->save_unavailable != 0) {
                    selection_allowed = 0;
                    selection_blocked = 1;
                }
                break;
            case 0:
                if (GWPartyGet() == 0) {
                    selection_allowed = 0;
                    selection_blocked = 1;
                }
                break;
        }
        if (selection_blocked != 0) {
            HuAudFXPlay(4);
        } else if (selection_allowed != 0) {
            HuAudFXPlay(2);
            work->option_active = 1;
        }
    }
    if (input == 0x200) {
        HuAudFXPlay(0x1C);
        work->destroy_requested = 1;
    }
    return work->cursor_column + work->cursor_row * 4;
}

/** @brief Creates the pause menu's control-hint and settings text windows. */
static void CreatePauseControlWin(void) {
    float window_size[2];
    float window_x;
    float window_y;
    u32 message_id;

    message_id = MAKE_MESSID(16, 53);
    HuWinMesMaxSizeGet(1, window_size, message_id);
    window_x = 152.0f;
    window_y = 280.0f;
    settingsControlWin = HuWinCreate(window_x, window_y, window_size[0], window_size[1], 0);
    HuWinBGTPLvlSet(settingsControlWin, 0.0f);
    HuWinMesSpeedSet(settingsControlWin, 0);
    HuWinMesSet(settingsControlWin, message_id);
    HuWinDispOff(settingsControlWin);
    if (GWBoardGet() == 7 || GWBoardGet() == 8) {
        message_id = MAKE_MESSID(16, 73);
    } else {
        message_id = MAKE_MESSID(16, 52);
    }
    HuWinMesMaxSizeGet(1, window_size, message_id);
    window_x = -10000.0f;
    window_y = 376.0f;
    controlWin = HuWinCreate(window_x, window_y, window_size[0], window_size[1], 0);
    HuWinBGTPLvlSet(controlWin, 0.0f);
    HuWinMesSpeedSet(controlWin, 0);
    HuWinMesSet(controlWin, message_id);
    HuWinDispOff(controlWin);
    window_x = 128.0f;
    window_y = 312.0f;
    window_size[0] = 432.0f;
    window_size[1] = 128.0f;
    settingsWin = HuWinCreate(window_x, window_y, window_size[0], window_size[1], 0);
    HuWinDispOff(settingsWin);
    HuWinMesSpeedSet(settingsWin, 0);
    window_x = 640.0f;
    window_y = 112.0f;
    window_size[0] = 504.0f;
    window_size[1] = 128.0f;
}

/** @brief Destroys all pause-menu text windows that were created. */
static void DeletePauseControlWin(void) {
    if (settingsControlWin != -1) {
        HuWinKill(settingsControlWin);
        settingsControlWin = -1;
    }
    if (controlWin != -1) {
        HuWinKill(controlWin);
        controlWin = -1;
    }
    if (settingsWin != -1) {
        HuWinKill(settingsWin);
        settingsWin = -1;
    }
}

/**
 * @brief Waits for resume, settings, or extra-board quit input.
 * @return `0` to resume, `1` to open settings, or `2` to quit an extra board.
 */
static s32 WaitPauseInput(void) {
    s32 is_extra_board;
    s32 result;
    u32 input;

    mainScreenF = 1;
    if (GWBoardGet() == 7 || GWBoardGet() == 8) {
        is_extra_board = 1;
    } else {
        is_extra_board = 0;
    }
    HuWinDispOn(controlWin);
    SetBoxVisible(0);
    HuSprAttrReset(pauseSprGrp, 0, HUSPR_ATTR_DISPOFF);
    HuSprAttrSet(pauseCursorPos, 0, HUSPR_ATTR_DISPOFF);
    PauseCreateNumber(0, GWSystem.turn);
    PauseCreateNumber(1, GWSystem.max_turn);
    HuPrcSleep(4);
    result = 0;
    while (1) {
        input = HuPadBtnDown[pausePad];
        if (input == 0x1000 || input == 0x200) {
            result = 0;
            HuAudFXPlay(0x1C);
            break;
        } else if (is_extra_board == 0) {
            if (input == 0x100) {
                result = 1;
                HuAudFXPlay(1);
                break;
            }
        } else if (input == 0x10) {
            pauseQuitF = 1;
            result = 2;
            mgQuitExtraF = 1;
            HuAudFXPlay(0x1C);
            break;
        }
        HuPrcVSleep();
    }
    HuWinDispOff(controlWin);
    if (pauseSprGrp != -1) {
        HuSprGrpKill(pauseSprGrp);
        pauseSprGrp = -1;
    }
    HuPrcSleep(2);
    return result;
}

/** @brief Shows the settings grid and waits for its object to close. */
static void ShowPauseConfig(void) {
    float cursor_x;
    float cursor_y;

    mainScreenF = 0;
    HuWinDispOn(settingsControlWin);
    HuWinDispOn(settingsWin);
    SetBoxVisible(1);
    HuSprAttrReset(pauseCursorPos, 0, HUSPR_ATTR_DISPOFF);
    BoardModelVisibilitySet(hostMdl, 1);
    if (GWPartyGet() == 0) {
        cursor_x = boxPosTbl[0][0];
        cursor_y = boxPosTbl[0][1];
        HuSprPosSet(pauseCursorPos, 1, cursor_x, cursor_y);
        HuSprAttrReset(pauseCursorPos, 1, HUSPR_ATTR_DISPOFF);
    } else {
        HuSprAttrSet(pauseCursorPos, 1, HUSPR_ATTR_DISPOFF);
    }
    while (configObj) {
        HuPrcVSleep();
    }
}

/** @brief Assigns character and human/CPU banks to controller-panel sprites. */
static void CreatePadConfigSprite(void) {
    float sprite_x;
    float sprite_y;
    s32 difficulty_bank;
    s32 character;
    s32 difficulty;
    s32 is_cpu;
    s32 i;
    s32 j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            if (i == GWPlayer[j].port) {
                break;
            }
        }
        sprite_x = i * 96 + 178;
        sprite_y = 64.0f;
        character = GWPlayer[j].character;
        difficulty = GWPlayer[j].diff;
        is_cpu = GWPlayer[j].com;
        HuSprBankSet(padConfigSprGrp, i, character);
        if (is_cpu != 0) {
            difficulty_bank = difficulty + 4;
        } else {
            difficulty_bank = i;
        }
        HuSprBankSet(padConfigSprGrp, i + 4, difficulty_bank);
        HuSprPosSet(padConfigSprGrp, i, sprite_x, sprite_y);
        HuSprPosSet(padConfigSprGrp, i + 4, sprite_x, sprite_y);
        HuSprPosSet(padConfigSprGrp, i + 8, sprite_x, sprite_y);
    }
    HuSprPosSet(padConfigSprGrp, 12, 260.0f, 64.0f);
    HuSprGrpPosSet(padConfigSprGrp, 0.0f, 0.0f);
}

/**
 * @brief Shows or hides all controller-assignment sprites.
 * @param show Nonzero to show the sprites; zero to hide them.
 */
static void ShowPadConfigSprite(s32 show) {
    s32 port;
    s32 i;

    for (i = 0; i < 4; i++) {
        port = GWPlayer[i].port;
        if (show != 0) {
            HuSprAttrReset(padConfigSprGrp, port, HUSPR_ATTR_DISPOFF);
            HuSprAttrReset(padConfigSprGrp, port + 4, HUSPR_ATTR_DISPOFF);
            HuSprAttrReset(padConfigSprGrp, port + 8, HUSPR_ATTR_DISPOFF);
        } else {
            HuSprAttrSet(padConfigSprGrp, port, HUSPR_ATTR_DISPOFF);
            HuSprAttrSet(padConfigSprGrp, port + 4, HUSPR_ATTR_DISPOFF);
            HuSprAttrSet(padConfigSprGrp, port + 8, HUSPR_ATTR_DISPOFF);
        }
    }
    if (show != 0) {
        HuSprAttrReset(padConfigSprGrp, 12, HUSPR_ATTR_DISPOFF);
    } else {
        HuSprAttrSet(padConfigSprGrp, 12, HUSPR_ATTR_DISPOFF);
    }
}

/**
 * @brief Sets the current-turn or maximum-turn digit sprites.
 * @param number_type Zero selects current-turn digits; nonzero selects maximum-turn digits.
 * @param value Turn value displayed as one or two decimal digits.
 */
static void PauseCreateNumber(s32 number_type, s8 value) {
    s32 tens_sprite;
    s32 tens_digit;
    s32 ones_digit;

    ones_digit = value % 10;
    tens_digit = value / 10;
    if (number_type == 0) {
        tens_sprite = 3;
    } else {
        tens_sprite = 5;
    }
    if (tens_digit != 0) {
        HuSprBankSet(pauseSprGrp, tens_sprite, tens_digit);
    } else {
        HuSprAttrSet(pauseSprGrp, tens_sprite, 4);
    }
    HuSprBankSet(pauseSprGrp, tens_sprite + 1, ones_digit);
}

/** @brief Checks whether the board pause process exists. @return `TRUE` while paused. */
BOOL BoardPauseActiveCheck(void) {
    return (mainProcess != NULL) ? TRUE : FALSE;
}

/**
 * @brief Checks connected eligible controllers for a pause-button request.
 * @return `TRUE` after recording the requesting controller; otherwise `FALSE`.
 */
BOOL BoardPauseReqCheck(void) {
    s32 port;
    s32 i;

    if (BoardPauseActiveCheck()) {
        return FALSE;
    }
    omSysPauseEnable(0);
    if (BoardPauseDisableGet() != 0) {
        return FALSE;
    }
    for (i = 0; i < 4; i++) {
        port = GWPlayer[i].port;
        if (HuPadStatGet(port) == 0 && (GWPartyGet() != 0 || GWPlayer[i].com == 0) && (HuPadBtnDown[port] & 0x1000)) {
            pausePad = port;
            return TRUE;
        }
    }
    return FALSE;
}

/**
 * @brief Maps a controller port to its player index.
 * @param port Controller port number.
 * @return Matching player index, or `4` when no player uses the port.
 */
static s32 CheckPort(s32 port) {
    s32 i;

    for (i = 0; i < 4; i++) {
        if (port == GWPlayer[i].port) {
            break;
        }
    }
    return i;
}

/**
 * @brief Builds a box transform relative to the current camera rotation.
 * @param x_rotation Additional X-axis rotation in degrees.
 * @param y_rotation Additional Y-axis rotation in degrees.
 * @param[out] matrix Matrix receiving the combined camera and box rotations.
 */
static void RotateBox(float x_rotation, float y_rotation, Mtx matrix) {
    Vec camera_rotation;
    Mtx camera_x_matrix;
    Mtx camera_y_matrix;
    Mtx box_x_matrix;
    Mtx box_y_matrix;

    BoardCameraRotGet(&camera_rotation);
    PSMTXRotRad(camera_x_matrix, 'x', MTXDegToRad(camera_rotation.x));
    PSMTXRotRad(camera_y_matrix, 'y', MTXDegToRad(camera_rotation.y));
    PSMTXRotRad(box_x_matrix, 'x', MTXDegToRad(x_rotation));
    PSMTXRotRad(box_y_matrix, 'y', MTXDegToRad(y_rotation));
    PSMTXConcat(camera_y_matrix, camera_x_matrix, camera_y_matrix);
    PSMTXConcat(box_x_matrix, box_y_matrix, box_y_matrix);
    PSMTXConcat(camera_y_matrix, box_y_matrix, matrix);
}
