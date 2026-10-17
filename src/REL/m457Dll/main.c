#include "dolphin/pad.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/esprite.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/printfunc.h"
#include "game/process.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"
#include "string.h"

#include "ext_math.h"
#include "version.h"

s32 rand8(void);

#define MSM_SE_BOWSER_LAUGH 0x3A
#define MSM_SE_BOWSER_HOWL 0x3E
#define MSM_SE_KOOPA_KID_DROP 0x5B
#define MSM_SE_PLAYER_DROP 0x33D
#define MSM_SE_WIND 0x6D7
#define MSM_SE_INVINCIBILITY 0x7E6

#define MSM_MUS_BACKGROUND 0x4D

#define MODEL_ID_PLAYER_PLAYER 0

#define MODEL_ID_GAME_SKY 0
#define MODEL_ID_GAME_RING 1
#define MODEL_ID_GAME_MUSHROOM 2
#define MODEL_ID_GAME_INVINCE_SHINING 3
#define MODEL_ID_GAME_INVINCE_SPARKS 4
#define MODEL_ID_GAME_KOOPA_KID 5
#define MODEL_ID_GAME_WAR_FAN 6

#define CHARACTER_BOWSER -1
#define PAD_IDX_COM -1

#define PLAYER_STATE_INITIAL_ADVANCE 2 // TODO is this right?
#define PLAYER_STATE_FALLING 16

#define PREV_TRIG_L 0
#define PREV_TRIG_R 1

typedef struct ShovePlayerData {
    /* 0x00 */ s32 group;
    /* 0x04 */ s32 player;
    /* 0x08 */ s32 character;
    /* 0x0C */ s32 controllerIndex;
    /* 0x10 */ s32 difficulty;
    /* 0x14 */ s32 state;
    /* 0x18 */ s32 stateTimer;
    /* 0x1C */ s32 invincibilityTimer;
    /* 0x20 */ s32 aButtonCooldown;
    /* 0x24 */ s32 leftTriggerCooldown;
    /* 0x28 */ s32 rightTriggerCooldown;
    /* 0x2C */ s32 reservedPlayerValue;
    /* 0x30 */ s32 stamina;
    /* 0x34 */ s32 staminaRegenTimer;
    /* 0x38 */ s32 previousTrigger;
    /* 0x3C */ s32 computerActionTimer;
    /* 0x40 */ s32 currentMotionIndex;
    /* 0x44 */ s32 requestedMotionIndex;
    /* 0x48 */ float movementSpeed;
    /* 0x4C */ s32 staminaExhausted;
} ShovePlayerData; // Size 0x50

typedef struct PendingSoundEvents {
    /* 0x00 */ s32 eventCount;
    /* 0x04 */ float eventTimers[3];
} PendingSoundEvents; // Size 0x10

typedef struct WindState {
    /* 0x00 */ s32 windUpdatePhase;
    /* 0x04 */ s32 windTransitionTime;
    /* 0x08 */ float windX;
    /* 0x0C */ float windZ;
    /* 0x10 */ float targetWindX;
    /* 0x14 */ float targetWindZ;
} WindState; // Size 0x18

typedef struct InvincibilitySparkConfig {
    /* 0x00 */ float scale;
    /* 0x04 */ float xOffset;
    /* 0x08 */ float yOffset;
} InvincibilitySparkConfig; // Size 0x0C

static void ShoveExit(OMOBJ *object);

static PendingSoundEvents pendingSoundEvents[5];
static OMOBJ *mainObject;
static OMOBJ *viewObject;
static OMOBJ *playerObjectsByGroup[2];
static OMOBJ *gameObject;
static s32 gameTimelineFrame;
static s32 shoveGameState;
static char reservedBssPadding58[4]; // unused
static s32 reservedBssValue54;
static s32 reservedBssValue50;
static HUPROCESS *objectProcess;
static s32 sceneLightId;
static s32 resultPlayerGroup;
static s32 matchEnded;
static s32 reservedBssValue3C;
static char reservedBssPadding38[4]; // unused
static float cameraTransitionStartRotationY;
static float cameraTransitionStartCenterZ;
static float cameraTransitionStartCenterX;
static float cameraTransitionProgress;
static s32 cameraTransitionDirection;
static s32 triggerAlternationMode;
static s32 mushroomDropPhase;
static s32 koopaKidDeparturePhase;
static BOOL againstBrowserMode;
static s32 messageSequenceEndFlag;
static char reservedBssPadding08[8]; // unused
static s32 gameFrameCounter;
static char reservedBssPadding00[4]; // unused

s32 spriteDataNumbers[] = { DATA_MAKE_NUM(DATADIR_M457, 23), DATA_MAKE_NUM(DATADIR_M457, 22), DATA_MAKE_NUM(DATADIR_M457, 24),
    DATA_MAKE_NUM(DATADIR_M457, 24), DATA_MAKE_NUM(DATADIR_M457, 24), DATA_MAKE_NUM(DATADIR_M457, 24), DATA_MAKE_NUM(DATADIR_M457, 24),
    DATA_MAKE_NUM(DATADIR_MGCONST, 0), DATA_MAKE_NUM(DATADIR_M457, 23), DATA_MAKE_NUM(DATADIR_M457, 22), DATA_MAKE_NUM(DATADIR_M457, 24),
    DATA_MAKE_NUM(DATADIR_M457, 24), DATA_MAKE_NUM(DATADIR_M457, 24), DATA_MAKE_NUM(DATADIR_M457, 24), DATA_MAKE_NUM(DATADIR_M457, 24),
    DATA_MAKE_NUM(DATADIR_MGCONST, 0), DATA_MAKE_NUM(DATADIR_M457, 27), DATA_MAKE_NUM(DATADIR_M457, 28), DATA_MAKE_NUM(DATADIR_M457, 25),
    DATA_MAKE_NUM(DATADIR_M457, 25), DATA_MAKE_NUM(DATADIR_M457, 25), DATA_MAKE_NUM(DATADIR_M457, 25), DATA_MAKE_NUM(DATADIR_M457, 25) };

s32 spriteIds[23] = { 0 };

s32 playerMotionFileIds[] = { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0), DATA_MAKE_NUM(DATADIR_MARIOMOT, 5), DATA_MAKE_NUM(DATADIR_MARIOMOT, 6),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 112), DATA_MAKE_NUM(DATADIR_MARIOMOT, 111), DATA_MAKE_NUM(DATADIR_M457, 8), DATA_MAKE_NUM(DATADIR_M457, 0),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 12), DATA_MAKE_NUM(DATADIR_MARIOMOT, 13), DATA_MAKE_NUM(DATADIR_MARIOMOT, 21),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 23), DATA_MAKE_NUM(DATADIR_MARIOMOT, 24) };

u32 playerMotionUsesCharacterVariant[] = { 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0 };

u32 bowserMotionFileIds[] = { DATA_MAKE_NUM(DATADIR_BKOOPA, 8), DATA_MAKE_NUM(DATADIR_BKOOPA, 10), 0, DATA_MAKE_NUM(DATADIR_M457, 31),
    DATA_MAKE_NUM(DATADIR_M457, 32), DATA_MAKE_NUM(DATADIR_M457, 33), DATA_MAKE_NUM(DATADIR_M457, 34), DATA_MAKE_NUM(DATADIR_M457, 35),
    DATA_MAKE_NUM(DATADIR_M457, 36), DATA_MAKE_NUM(DATADIR_M457, 37), DATA_MAKE_NUM(DATADIR_M457, 29), DATA_MAKE_NUM(DATADIR_M457, 30) };

u8 playerMotionLoopFlags[12] = { 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1 };
s32 playerMotionBlendFrames[12] = { 8, 0, 0, 8, 5, 8, 4, 2, 2, 8, 8, 8 };
s32 playerMotionStartFrames[12] = { 0 };

float playerMotionOffsetsByCharacter[9][3] = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 20.0f, 60.0f, 20.0f }, { 20.0f, 70.0f, 20.0f }, { 20.0f, 20.0f, 20.0f },
    { 50.0f, -70.0f, 0.0f }, { 20.0f, 60.0f, 20.0f }, { 100.0f, 0.0f, 50.0f }, { -70.0f, -70.0f, -70.0f } };

InvincibilitySparkConfig invincibilitySparkConfigs[9] = { { 1.5f, 45.0f, 270.0f }, { 1.7f, 45.0f, 270.0f }, { 1.8f, 45.0f, 270.0f }, { 1.6f, 45.0f, 270.0f },
    { 1.7f, 5.0f, 270.0f }, { 2.0f, -85.0f, 295.0f }, { 1.8f, 45.0f, 270.0f }, { 2.0f, 40.0f, 330.0f }, { 2.25f, -30.0f, 300.0f } };

WindState windState = { 0 };

s16 endSequenceHandle = -1;
s16 matchTimerHandle = -1;
s32 backgroundMusicSequenceId = -1;
s32 windSoundHandle = -1;
s16 shoveMessageWindowId = -1;
s32 invincibilitySoundHandle = -1;
s32 mushroomRespawnTimer = 10 * REFRESH_RATE;

static void CameraDebugInputUpdate(void)
{
    Center.x += 10.0 * (((HuPadBtn[0] & PAD_BUTTON_RIGHT) != 0) - ((HuPadBtn[0] & PAD_BUTTON_LEFT) != 0));
    Center.z -= 10.0 * (((HuPadBtn[0] & PAD_BUTTON_UP) != 0) - ((HuPadBtn[0] & PAD_BUTTON_DOWN) != 0));
    Center.y += 10.0 * (((HuPadBtn[0] & PAD_BUTTON_Y) != 0) - ((HuPadBtn[0] & PAD_BUTTON_X) != 0));
    CZoom += 25.0 * (((HuPadBtn[0] & PAD_BUTTON_X) != 0) - ((HuPadBtn[0] & PAD_BUTTON_Y) != 0));
    CRot.y += HuPadSubStkX[0] / 16.0;
    CRot.x += HuPadSubStkY[0] / -16.0;
    print8(64, 392, 1.0f, "%f,%f,%f", Center.x, Center.y, Center.z);
    print8(64, 408, 1.0f, "%f,%f,%f", CRot.x, CRot.y, CRot.z);
    print8(64, 424, 1.0f, "%f", CZoom);
}

static void ShovePlayerUpdate(OMOBJ *object)
{
    ShovePlayerData *playerData = object->data;
    ShovePlayerData *opponentData = playerObjectsByGroup[1 - object->work[0]]->data;
    float reservedFloatScratchA = 0.0f;
    float reservedFloatScratchB = 0.0f;
    float reservedFloatScratchC = 0.0f;
    s32 reservedIntegerScratch = 0;
    s16 motionIndex = -1;
    s16 motionBlendFrames = -1;
    s16 motionStartFrame = -1;
    BOOL aPressedF = (playerData->controllerIndex == PAD_IDX_COM) ? 0 : ((HuPadBtnDown[playerData->controllerIndex] & PAD_BUTTON_A) > 0);
    BOOL rPressedF = (playerData->controllerIndex == PAD_IDX_COM) ? 0 : ((HuPadBtnDown[playerData->controllerIndex] & PAD_TRIGGER_R) > 0);
    BOOL lPressedF = (playerData->controllerIndex == PAD_IDX_COM) ? 0 : ((HuPadBtnDown[playerData->controllerIndex] & PAD_TRIGGER_L) > 0);
    s32 i;

    if (playerData->controllerIndex == PAD_IDX_COM) {
        playerData->computerActionTimer--;
        if (playerData->computerActionTimer <= 0) {
            if (playerData->state == 1) {
                aPressedF = TRUE;
            }
            else {
                rPressedF = lPressedF = TRUE;
            }
            if (playerData->state >= 8 || (playerData->state >= 3 && playerData->stateTimer >= 10)) {
                switch (playerData->difficulty) {
                    case 0:
                        playerData->computerActionTimer = rand8() % 14 + 4 + (playerData->state != 8 ? 28 - playerData->stateTimer : 0);
                        break;
                    case 1:
                        playerData->computerActionTimer = rand8() % 9 + 4 + (playerData->state != 8 ? 28 - playerData->stateTimer : 0);
                        break;
                    case 2:
                        playerData->computerActionTimer = rand8() % 10 + 12
                            + ((playerData->state != 8 && playerData->stateTimer >= 9 && playerData->stateTimer <= 13) ? 13 - playerData->stateTimer : 0);
                        break;
                    case 3:
                        playerData->computerActionTimer = rand8() % 5 + 4
                            + ((playerData->state != 8 && playerData->stateTimer >= 2 && playerData->stateTimer <= 17) ? 17 - playerData->stateTimer : 0);
                        if (opponentData->invincibilityTimer > 0 || playerData->state == 7) {
                            // don't waste stamina
                            lPressedF = rPressedF = 0;
                        }
                        break;
                }
            }
        }
    }
    if (playerData->previousTrigger == PREV_TRIG_L) {
        lPressedF = FALSE;
    }
    if (playerData->previousTrigger == PREV_TRIG_R) {
        rPressedF = FALSE;
    }
    if (playerData->aButtonCooldown > 0) {
        aPressedF = FALSE;
        playerData->aButtonCooldown--;
    }
    else if (aPressedF) {
        playerData->aButtonCooldown = 20;
    }
    if (playerData->leftTriggerCooldown > 0) {
        lPressedF = FALSE;
        playerData->leftTriggerCooldown--;
    }
    else if (lPressedF) {
        playerData->leftTriggerCooldown = 6;
    }
    if (playerData->rightTriggerCooldown > 0) {
        rPressedF = FALSE;
        playerData->rightTriggerCooldown--;
    }
    else if (rPressedF) {
        playerData->rightTriggerCooldown = 6;
    }
    if (shoveGameState == 1008 && playerData->state >= 8 && playerData->state <= 14) {
        s32 prevStamina = playerData->stamina;
        if (lPressedF) {
            playerData->stamina -= 7;
        }
        if (rPressedF) {
            playerData->stamina -= 7;
        }
        if (prevStamina > 0 && playerData->stamina <= 0) {
            // cap at -1
            playerData->stamina = -1;
            playerData->staminaExhausted = 1;
        }
    }
    if (triggerAlternationMode == 0 && shoveGameState == 1008 && playerData->state >= 7 && playerData->state <= 14 && (lPressedF || rPressedF)) {
        playerData->previousTrigger = lPressedF ? PREV_TRIG_L : rPressedF ? PREV_TRIG_R : PREV_TRIG_L;
    }
    if (playerData->group == 0) {
        playerObjectsByGroup[0]->trans.x -= playerData->movementSpeed;
        playerObjectsByGroup[1]->trans.x -= opponentData->movementSpeed;
    }
    switch (shoveGameState) {
        case 1001:
            if (gameTimelineFrame == 0) {
                Hu3DModelAttrReset(object->mdlId[MODEL_ID_PLAYER_PLAYER], HU3D_ATTR_DISPOFF);
            }
            if (gameTimelineFrame <= 30) {
                if (playerData->character == CHARACTER_BOWSER) {
                    Hu3DMotionTimeSet(object->mdlId[MODEL_ID_PLAYER_PLAYER], 138.0f);
                }
                object->trans.y = 2400.0 * (1.0 - gameTimelineFrame / 30.0);
            }
            if (playerData->character != CHARACTER_BOWSER && gameTimelineFrame == 30) {
                motionIndex = 2;
            }
            if (gameTimelineFrame == 40 && playerData->character != CHARACTER_BOWSER) {
                motionIndex = 0;
            }
            if (gameTimelineFrame == 45 && playerData->character == CHARACTER_BOWSER) {
                motionIndex = 0;
                Hu3DMotionShiftSet(object->mdlId[MODEL_ID_PLAYER_PLAYER], object->mtnId[motionIndex], playerMotionStartFrames[motionIndex], 35.0f, HU3D_MOTATTR_LOOP);
                playerData->currentMotionIndex = motionIndex;
            }
            if (gameTimelineFrame == 30) {
                windState.windTransitionTime += 50;
            }
            if (gameTimelineFrame == 30) {
                HuAudFXPlay(MSM_SE_PLAYER_DROP);
            }
            break;
        case 1005:
            if (gameTimelineFrame == 0) {
                motionIndex = 3;
            }
            if (gameTimelineFrame == (s32)((21.0f / 18) * REFRESH_RATE)) {
                motionIndex = 4;
            }
            break;
        case 1006:
            if (gameTimelineFrame == 5) {
                motionIndex = 5;
            }
            break;
        case 1010:
            if (resultPlayerGroup == playerData->group && gameTimelineFrame >= 15) {
                if (gameTimelineFrame == 15) {
                    playerData->state = 0;
                    playerData->stateTimer = 0;
                    if (playerData->character == CHARACTER_BOWSER) {
                        HuAudFXPlay(MSM_SE_BOWSER_LAUGH);
                    }
                }
                motionIndex = 10;
                if (object->rot.y > 180.0f) {
                    object->rot.y -= 360.0f;
                }
                object->rot.y *= 0.9;
            }
            if (resultPlayerGroup == 2 && gameTimelineFrame >= 15) {
                if (gameTimelineFrame == 15) {
                    playerData->state = 0;
                    playerData->stateTimer = 0;
                }
                motionIndex = 11;
                if (object->rot.y > 180.0f) {
                    object->rot.y -= 360.0f;
                }
                object->rot.y *= 0.9;
            }
            break;
    }
    playerData->stateTimer += (playerData->stateTimer < 0x10000);
    if (playerData->state == 3 || playerData->state == 5) {
        if (playerData->stateTimer == 1 && shoveMessageWindowId != -1) {
            HuWinExCleanup(shoveMessageWindowId);
            shoveMessageWindowId = -1;
        }
        if ((playerData->state == 3 && playerData->stateTimer == 23) || (playerData->state == 5 && playerData->stateTimer == 15)) {
            float messageSize[2];
            HuWinMesMaxSizeGet(1, messageSize, MAKE_MESSID(48, 35));
            if (shoveMessageWindowId == -1) {
                shoveMessageWindowId = HuWinCreate(-10000.0f, 400.0f, messageSize[0], messageSize[1], 1);
            }
            HuWinBGTPLvlSet(shoveMessageWindowId, 0.0f);
            HuWinMesSpeedSet(shoveMessageWindowId, 0);
            HuWinMesSet(shoveMessageWindowId, MAKE_MESSID(48, 35));
        }
    }
    switch (playerData->state) {
        case 0:
            if (shoveGameState == 1008) {
                playerData->state = 1;
                playerData->stateTimer = 0;
            }
            break;
        case 1:
            if (playerData->stateTimer == 1 && playerData->controllerIndex == PAD_IDX_COM) {
                switch (playerData->difficulty) {
                    case 0:
                        playerData->computerActionTimer = rand8() % 5 + 15;
                        break;
                    case 1:
                        playerData->computerActionTimer = rand8() % 5 + 12;
                        break;
                    case 2:
                        playerData->computerActionTimer = rand8() % 5 + 10;
                        break;
                    case 3:
                        playerData->computerActionTimer = rand8() % 4 + 9;
                        break;
                }
            }
            // initial advance
            if (aPressedF) {
                playerData->state = PLAYER_STATE_INITIAL_ADVANCE;
                playerData->stateTimer = 0;
                motionIndex = 6;
                object->trans.x += (playerData->group == 0) ? ((rand8() % 2) * 2 - 1) : 0;
            }
            break;
        case PLAYER_STATE_INITIAL_ADVANCE:
            object->trans.x += 8.695652f * (playerData->group == 0 ? 1 : -1);
            break;
        case 3:
            object->trans.x += 4.347826f * (playerData->group == 0 ? 1 : -1);
            if (playerData->stateTimer >= 23) {
                playerData->state = 8;
                playerData->stateTimer = 0;
                motionIndex = 4;
            }
            break;
        case 4:
            if (playerData->stateTimer == 1) {
                motionIndex = 7;
            }
            object->trans.x += -4.347826f * (playerData->group == 0 ? 1 : -1);
            if (playerData->stateTimer >= 23) {
                playerData->state = 8;
                playerData->stateTimer = 0;
                motionIndex = 4;
            }
            break;
        case 5:
            object->trans.x += 8.695652f * (playerData->group == 0 ? 1 : -1);
            if (playerData->stateTimer >= 15) {
                playerData->state = 8;
                playerData->stateTimer = 0;
                motionIndex = 4;
            }
            break;
        case 6:
            if (playerData->stateTimer == 1) {
                motionIndex = 8;
            }
            object->trans.x += -8.695652f * (playerData->group == 0 ? 1 : -1);
            if (playerData->stateTimer >= 7) {
                playerData->state = 8;
                playerData->stateTimer = 0;
                motionBlendFrames = 7;
                motionIndex = 4;
            }
            if (playerData->stateTimer >= 15) {
                playerData->state = 8;
                playerData->stateTimer = 0;
            }
            break;
        case 8:
            if (playerData->stamina == 0) {
                motionBlendFrames = 0;
                playerData->state = 7;
                playerData->stateTimer = 1;
                playerData->staminaRegenTimer = 0;
            }
            else {
                if (!lPressedF && !rPressedF) {
                    break;
                }
                playerData->state = 9;
                playerData->stateTimer = 0;
                motionBlendFrames = 3;
                motionStartFrame = (playerData->previousTrigger == PREV_TRIG_L ? 0 : 11) + 5;
                motionIndex = 6;
                if (triggerAlternationMode == 1) {
                    playerData->previousTrigger = lPressedF ? PREV_TRIG_L : rPressedF ? PREV_TRIG_R : PREV_TRIG_L;
                }
                break;
            }
            /* fallthrough */
        case 7:
            if (playerData->stateTimer == 1) {
                s32 noStaminaSeIdTbl[] = { 0x12A, 0x16A, 0x1AA, 0x1EA, 0x22A, 0x26A, 0x2AA, 0x2EA, 0x03B };
                HuAudFXPlay(noStaminaSeIdTbl[playerData->character == CHARACTER_BOWSER ? 8 : playerData->character]);
                motionIndex = 9;
                playerData->staminaExhausted = 0;
            }
            if (playerData->stamina > 0) {
                playerData->state = 8;
                playerData->stateTimer = 0;
                motionIndex = 4;
            }
            break;
        case 9:
            object->trans.x += 8.695652f * (playerData->group == 0 ? 1 : -1);
            if (playerData->stateTimer >= 23) {
                playerData->state = 8;
                playerData->stateTimer = 0;
                motionIndex = 4;
            }
            break;
        case 12:
            if (playerData->stateTimer == 7) {
                motionBlendFrames = 0;
                motionStartFrame = 7;
                motionIndex = 7;
            }
            if (playerData->stateTimer == 15) {
                motionBlendFrames = 5;
                motionIndex = 4;
            }
            if (playerData->stateTimer >= 20) {
                playerData->state = 8;
                playerData->stateTimer = 0;
                motionIndex = 4;
            }
            break;
        case 10:
            object->trans.x += 4.347826f * (playerData->group == 0 ? 1 : -1);
            if (playerData->stateTimer >= 23) {
                playerData->state = 8;
                playerData->stateTimer = 0;
                motionIndex = 4;
            }
            break;
        case 11:
            object->trans.x += -4.347826f * (playerData->group == 0 ? 1 : -1);
            if (playerData->stateTimer == 1) {
                motionIndex = 7;
            }
            if (playerData->stateTimer >= 23) {
                playerData->state = 8;
                playerData->stateTimer = 0;
                motionIndex = 4;
            }
            break;
        case 13:
            object->trans.x += 8.695652f * (playerData->group == 0 ? 1 : -1);
            if (playerData->stateTimer >= 23) {
                playerData->state = 8;
                playerData->stateTimer = 0;
                motionIndex = 4;
            }
            break;
        case 14:
            object->trans.x += -8.695652f * (playerData->group == 0 ? 1 : -1);
            if (playerData->stateTimer == 1) {
                motionIndex = 8;
            }
            if (playerData->stateTimer == 11) {
                motionBlendFrames = 11;
                motionIndex = 4;
            }
            if (playerData->stateTimer >= 23) {
                playerData->state = 8;
                playerData->stateTimer = 0;
            }
            if (playerData->staminaRegenTimer < 8) {
                playerData->staminaRegenTimer = 8;
            }
            break;
        case 15:
            if (playerData->stateTimer == 1 && playerData->invincibilityTimer > 4) {
                playerData->invincibilityTimer = 4;
            }
            if (playerData->stateTimer == 1 && shoveMessageWindowId != -1) {
                HuWinExCleanup(shoveMessageWindowId);
                shoveMessageWindowId = -1;
            }
            if (playerData->stateTimer == 7) {
                motionIndex = 4;
                motionBlendFrames = 20;
            }
            if (playerData->stateTimer == REFRESH_RATE) {
                resultPlayerGroup = playerData->group;
                playerData->invincibilityTimer = 0;
            }
            break;
        case PLAYER_STATE_FALLING:
            // make the player fall down in the correct direction
            object->trans.x += -39.130432f * (playerData->group == 0 ? 1 : -1);
            object->trans.y += 39.130432f;
            if (playerData->stateTimer == 1) {
                s32 fallSeIdTbl[] = { 0x123, 0x163, 0x1A3, 0x1E3, 0x223, 0x263, 0x2A3, 0x2E3, 0x03C };
                HuAudFXPlay(fallSeIdTbl[playerData->character == CHARACTER_BOWSER ? 8 : playerData->character]);
                motionIndex = 8;
                motionBlendFrames = 0;
                motionStartFrame = 6;
            }
            object->rot.z = (playerData->group == 0) ? -20 : 20;
            break;
        case 17:
            if (playerData->stateTimer == 1 && playerData->invincibilityTimer > 4) {
                playerData->invincibilityTimer = 4;
            }
            if (playerData->stateTimer == 1 && shoveMessageWindowId != -1) {
                HuWinExCleanup(shoveMessageWindowId);
                shoveMessageWindowId = -1;
            }
            if (playerData->stateTimer == 7) {
                motionIndex = 0;
                motionBlendFrames = 10;
            }
            if (playerData->stateTimer >= 7 && playerData->stateTimer < 17) {
                object->trans.x += (playerData->group == 0 ? -1 : 1) * 2;
            }
            if (playerData->stateTimer == REFRESH_RATE) {
                resultPlayerGroup = 2;
                playerData->invincibilityTimer = 0;
            }
            break;
    }
    switch (playerData->state) {
        case 12:
            if (playerData->group == 0) {
                break;
            }
            /* fallthrough */
        case 3:
        case 5:
        case 10:
        case 13: {
            Vec itemHookPosition;
            s32 contactStep;
            char *itemHookName;

            s32 currentMotionFrame = Hu3DMotionTimeGet(object->mdlId[MODEL_ID_PLAYER_PLAYER]);
            if ((opponentData->state != 4 && opponentData->state != 6) || opponentData->stateTimer != 1) {
                if ((currentMotionFrame + 1) % 11 != 0 || (playerData->state == 12 && currentMotionFrame + 1 > 11)) {
                    break;
                }
            }
            contactStep = ((currentMotionFrame + 1) / 11 - 1) % 2;
            {
                char bowserItemHookNames[][50] = { "g000mN-itemhook_R", "g000mN-itemhook_L" };

                if (playerData->character != CHARACTER_BOWSER) {
                    itemHookName = CharModelItemHookGet(playerData->character, 1, (contactStep % 2 == 0) ? 0 : 1);
                }
                else {
                    itemHookName = (contactStep % 2 == 0) ? bowserItemHookNames[0] : bowserItemHookNames[1];
                }
            }
            Hu3DModelObjPosGet(object->mdlId[MODEL_ID_PLAYER_PLAYER], itemHookName, &itemHookPosition);
            Hu3DModelPosSet(gameObject->mdlId[contactStep + 7], itemHookPosition.x + (playerData->group == 0 ? 1 : -1) * 50, itemHookPosition.y, 75.0f + itemHookPosition.z);
            Hu3DMotionTimeSet(gameObject->mdlId[contactStep + 7], 2.0f);
            Hu3DModelAttrReset(gameObject->mdlId[contactStep + 7], 1);
            if (pendingSoundEvents[playerData->character == CHARACTER_BOWSER ? 1 : 0].eventCount < 3) {
                pendingSoundEvents[playerData->character == CHARACTER_BOWSER ? 1 : 0]
                    .eventTimers[pendingSoundEvents[playerData->character == CHARACTER_BOWSER ? 1 : 0].eventCount]
                    = 0.0f;
                pendingSoundEvents[playerData->character == CHARACTER_BOWSER ? 1 : 0].eventCount++;
            }
            // bounds check
            if (fabs(playerObjectsByGroup[1 - playerData->group]->trans.x) >= 800.0 && (opponentData->state == 11 || opponentData->state == 14)) {
                playerData->state = 15;
                opponentData->state = PLAYER_STATE_FALLING;
                playerData->stateTimer = opponentData->stateTimer = 0;
                if (pendingSoundEvents[4].eventCount < 3) {
                    pendingSoundEvents[4].eventTimers[pendingSoundEvents[4].eventCount] = 0.0f;
                    pendingSoundEvents[4].eventCount++;
                }
                if (backgroundMusicSequenceId != -1) {
                    HuAudSeqFadeOut(backgroundMusicSequenceId, 100);
                    backgroundMusicSequenceId = -1;
                }
            }
        } break;
    }
    playerData->staminaRegenTimer++;
    if (playerData->state != PLAYER_STATE_FALLING && playerData->state != 15 && playerData->state != 17 && shoveGameState == 1008
        && playerData->staminaRegenTimer >= 30 && (playerData->staminaRegenTimer - 30) % 30 == 0) {
        // regen stamina
        playerData->stamina += (playerData->staminaExhausted == 0 || playerData->state == 7) ? 10 : 0;
    }
    // apply invincibility to stamina and cap it
    playerData->stamina = (playerData->invincibilityTimer > 0 || playerData->stamina >= 100) ? 100 : (playerData->stamina < 0) ? 0 : playerData->stamina;
    for (i = 0; i < 5; i++) {
        float scale = (playerData->stamina >= i * 20 + 20) ? 1.0 : (playerData->stamina <= i * 20) ? 0.0 : (playerData->stamina - i * 20) / 20.0;
        espScaleSet(spriteIds[i + (playerData->group == 0 ? 2 : 10)], scale, scale);
        if (playerData->invincibilityTimer > 0) {
            // stamina bar mushroom start positions/offsets
            s32 posXTbl[] = { 106, 469 };
            s32 posYTbl[] = { 78, 78 };
            s32 ofsXTbl[] = { 0, 0, -33, -9, 15, 39, 63, -68, 0, 0, -33, -9, 15, 39, 63, -68 }; // second half is reservedFloatScratchA
            s32 ofsYTbl[] = { 0, 0, -1, -1, -1, -1, -1, -2, 0, 0, -1, -1, -1, -1, -1, -2 };

            // make the mushrooms in the stamina bar glow
            espPosSet(spriteIds[i + 18], posXTbl[playerData->group] + ofsXTbl[i + 2], posYTbl[playerData->group] + ofsYTbl[i + 2]);
            espScaleSet(spriteIds[i + 18], (playerData->invincibilityTimer % 4 == 0) ? 1.1 : 1.0, (playerData->invincibilityTimer % 4 == 0) ? 1.1 : 1.0);
            espTPLvlSet(spriteIds[i + 18], (playerData->invincibilityTimer % 4 < 2) ? 1.0 : (motionStartFrame % 4 == 2) ? 0.5 : 0.0);
            espDispOn(spriteIds[i + 18]);
        }
        else if (opponentData->invincibilityTimer == 0) {
            espDispOff(spriteIds[i + 18]);
        }
    }
    if (playerData->invincibilityTimer > 0) {
        s32 sparkCharacterIndex = (playerData->character != CHARACTER_BOWSER) ? playerData->character : 8;
        // make the player shine
        Hu3DModelPosSet(gameObject->mdlId[MODEL_ID_GAME_INVINCE_SHINING],
            object->trans.x + invincibilitySparkConfigs[sparkCharacterIndex].xOffset * (playerData->group == 0 ? 1 : -1), object->trans.y + invincibilitySparkConfigs[sparkCharacterIndex].yOffset,
            150.0f + object->trans.z);
        Hu3DModelScaleSet(gameObject->mdlId[MODEL_ID_GAME_INVINCE_SHINING], 0.75 * invincibilitySparkConfigs[sparkCharacterIndex].scale, 0.75 * invincibilitySparkConfigs[sparkCharacterIndex].scale,
            0.75 * invincibilitySparkConfigs[sparkCharacterIndex].scale);
        Hu3DModelAttrReset(gameObject->mdlId[MODEL_ID_GAME_INVINCE_SHINING], 1);
        Hu3DModelAttrSet(gameObject->mdlId[MODEL_ID_GAME_INVINCE_SHINING], HU3D_ATTR_ZCMP_OFF);
        // add sparks
        Hu3DModelPosSet(gameObject->mdlId[MODEL_ID_GAME_INVINCE_SPARKS],
            object->trans.x + invincibilitySparkConfigs[sparkCharacterIndex].xOffset * (playerData->group == 0 ? 1 : -1), object->trans.y + invincibilitySparkConfigs[sparkCharacterIndex].yOffset,
            150.0f + object->trans.z);
        Hu3DModelScaleSet(gameObject->mdlId[MODEL_ID_GAME_INVINCE_SPARKS], 0.75 * invincibilitySparkConfigs[sparkCharacterIndex].scale, 0.75 * invincibilitySparkConfigs[sparkCharacterIndex].scale,
            0.75 * invincibilitySparkConfigs[sparkCharacterIndex].scale);
        Hu3DModelAttrReset(gameObject->mdlId[MODEL_ID_GAME_INVINCE_SPARKS], 1);
        Hu3DModelAttrSet(gameObject->mdlId[MODEL_ID_GAME_INVINCE_SPARKS], HU3D_ATTR_ZCMP_OFF);
        playerData->invincibilityTimer--;
        if (playerData->invincibilityTimer == 0) {
            HuAudFXStop(invincibilitySoundHandle);
        }
    }
    else if (opponentData->invincibilityTimer == 0) {
        Hu3DModelAttrSet(gameObject->mdlId[MODEL_ID_GAME_INVINCE_SHINING], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(gameObject->mdlId[MODEL_ID_GAME_INVINCE_SPARKS], HU3D_ATTR_DISPOFF);
    }
    if (playerData->requestedMotionIndex >= 0) {
        motionIndex = playerData->requestedMotionIndex;
    }
    if (motionIndex >= 0 && motionIndex != playerData->currentMotionIndex) {
        if (motionBlendFrames == 0) {
            Hu3DMotionSet(object->mdlId[MODEL_ID_PLAYER_PLAYER], object->mtnId[motionIndex]);
            Hu3DMotionTimeSet(object->mdlId[MODEL_ID_PLAYER_PLAYER], (motionStartFrame == -1) ? 0 : motionStartFrame);
        }
        else {
            Hu3DMotionShiftSet(object->mdlId[MODEL_ID_PLAYER_PLAYER], object->mtnId[motionIndex], (motionStartFrame != -1) ? motionStartFrame : playerMotionStartFrames[motionIndex],
                (motionBlendFrames != -1) ? motionBlendFrames : playerMotionBlendFrames[motionIndex], playerMotionLoopFlags[motionIndex] ? HU3D_MOTATTR_NONE : HU3D_MOTATTR_LOOP);
        }
        playerData->currentMotionIndex = motionIndex;
    }
    if (playerData->group == 1) {
        HU3DMODEL *playerModel = &Hu3DData[object->mdlId[MODEL_ID_PLAYER_PLAYER]];
    }
}

s32 cameraMode = -1;
float cameraTransitionStartZoom = 3500.0f;
float cameraTransitionStartCenterY = 100.0f;
float cameraTransitionStartRotationX = -59.0f;

static void ShoveGameUpdate(OMOBJ *object)
{
    ShovePlayerData *playerDataLocal[2];
    OMOBJ *playerObjects[2];
    ShovePlayerData *playerStates[2];
    s32 playerActionKinds[2];
    OMOBJ *playerObjectsLocal[2];
    ShovePlayerData *playerStatesByGroup[2];
    HU3DMODEL *koopaKidExitModel;
    HU3DMODEL *koopaKidModelData;
    HU3DMODEL *mushroomModelData;
    HU3DMODEL *playerModelData;
    float spriteAlpha;
    float currentMotionOffsetX;
    float shiftedMotionOffsetX;
    float koopaKidFacingAngle;
    float motionBlendProgress;
    float koopaKidDropEase;
    float koopaKidOverlayAlpha;
    float koopaKidLandingEase;
    float windOffsetX;
    float koopaKidDropHeight;
    float koopaKidDropScale;
    float windOffsetZ;
    float koopaKidDropProgress;
    float targetWindDistance;
    float cameraBlendProgress;
    s32 invinciblePlayerId;
    s32 currentPlayerIndex;
    s32 otherPlayerIndex;
    s32 previousCameraMode;
    s32 characterIndex;
    s32 currentMotionKind;
    s32 shiftedMotionKind;
    s32 i;

    for (i = 0; i < 2; i++) {
        playerDataLocal[i] = playerObjectsByGroup[i]->data;
    }
    gameFrameCounter++;
    switch (shoveGameState) {
        case 1003:
            koopaKidDropHeight = 1800.0 * (1.0 - gameTimelineFrame / 30.0);
            koopaKidDropHeight = (koopaKidDropHeight < 0.0f) ? 0.0f : koopaKidDropHeight;
            Hu3DModelPosSet(gameObject->mdlId[MODEL_ID_GAME_KOOPA_KID], 0.0f, koopaKidDropHeight, 0.0f);
            Hu3DModelAttrReset(gameObject->mdlId[MODEL_ID_GAME_KOOPA_KID], HU3D_ATTR_DISPOFF);
            if (gameTimelineFrame == 31) {
                HuAudFXPlay(MSM_SE_KOOPA_KID_DROP);
            }
            if (gameTimelineFrame >= 31 && gameTimelineFrame <= 38) {
                koopaKidDropEase = 1.0 - (gameTimelineFrame - 31) / 7.0;
                Hu3DModelRotSet(object->mdlId[MODEL_ID_GAME_KOOPA_KID], 25.0 * (koopaKidDropEase * 0.5 + koopaKidDropEase * koopaKidDropEase * 0.5), 0.0f, 0.0f);
            }
            break;
        case 1002:
            if (gameTimelineFrame == 0) {
                reservedBssValue3C = 0;
            }
            if (gameTimelineFrame == (REFRESH_RATE/15)*5) {
                espAttrReset(spriteIds[16], HUSPR_ATTR_DISPOFF);
            }
            if ((gameTimelineFrame >= (REFRESH_RATE/15)*5 && gameTimelineFrame <= REFRESH_RATE - 10)
                || (gameTimelineFrame >= 3 * REFRESH_RATE && gameTimelineFrame <= ((7*REFRESH_RATE)/2))) {
                spriteAlpha = (gameTimelineFrame <= 120) ? (gameTimelineFrame - ((REFRESH_RATE/15)*5)) / (0.5 * REFRESH_RATE)
                                             : (1.0 - (gameTimelineFrame - 3 * REFRESH_RATE) / (0.5 * REFRESH_RATE));
                espTPLvlSet(spriteIds[16], spriteAlpha);
            }
            if (gameTimelineFrame == (s32)(3.5 * REFRESH_RATE)) {
                espAttrSet(spriteIds[16], HUSPR_ATTR_DISPOFF);
            }
            break;
        case 1006:
            if (gameTimelineFrame == 0) {
                Hu3DMotionSet(object->mdlId[MODEL_ID_GAME_KOOPA_KID], object->mtnId[2]);
                Hu3DModelAttrReset(object->mdlId[MODEL_ID_GAME_KOOPA_KID], HU3D_MOTATTR_LOOP);
            }
            if (gameTimelineFrame == 15) {
                Hu3DModelAttrSet(object->mdlId[MODEL_ID_GAME_KOOPA_KID], HU3D_MOTATTR_PAUSE);
            }
            break;
        case 1008:
            if (gameTimelineFrame == 1) {
                Hu3DModelAttrReset(object->mdlId[MODEL_ID_GAME_KOOPA_KID], HU3D_MOTATTR_PAUSE);
                for (i = 0; i < 16; i++) {
                    espDispOn(spriteIds[i]);
                }
            }
#if VERSION_NTSC
            if (gameTimelineFrame <= 25) {
                koopaKidDropProgress = gameTimelineFrame / 25.0;
                koopaKidDropScale = 1.0f - (1.0f - koopaKidDropProgress) * (1.0f - koopaKidDropProgress) * (1.0f - koopaKidDropProgress);
                koopaKidDropScale = 0.25 + 1.375 * koopaKidDropScale;
                koopaKidOverlayAlpha = 1.0f - (1.0f - koopaKidDropProgress) * (1.0f - koopaKidDropProgress);
                koopaKidOverlayAlpha = 2.0f * (1.0f - koopaKidDropProgress);
                espAttrReset(spriteIds[17], HU3D_ATTR_SHADOW);
#if VERSION_ENG
                espScaleSet(spriteIds[17], 0.85 * koopaKidDropScale, 0.85 * koopaKidDropScale);
#else
                espScaleSet(spriteIds[17], koopaKidDropScale, koopaKidDropScale);
#endif
                espTPLvlSet(spriteIds[17], (koopaKidOverlayAlpha > 1.0f) ? 1.0f : koopaKidOverlayAlpha);
            }
            else {
                espAttrSet(spriteIds[17], HUSPR_ATTR_DISPOFF);
            }
#endif
            for (i = 0; i < 2; i++) {
                playerObjects[i] = playerObjectsByGroup[i];
                playerStates[i] = playerObjectsByGroup[i]->data;
            }
            for (i = 0; i < 2; i++) {
                playerActionKinds[i] = (playerStates[1 - i]->invincibilityTimer > 0) ? 2
                    : (playerStates[i]->state == 8)               ? 2
                    : (playerStates[i]->state == 1)               ? 1
                    : (playerStates[i]->state == 9)               ? (playerStates[i]->stateTimer >= 0 ? 3 : 2)
                    : (playerStates[i]->state == 2)               ? 4
                    : (playerStates[i]->state == 7)               ? 0
                                                           : -1;
            }
            if (playerActionKinds[0] >= 0 && playerActionKinds[1] >= 0 && fabs(playerObjects[0]->trans.x - playerObjects[1]->trans.x) < 300.0) {
                playerObjects[0]->trans.x = (playerStates[0]->invincibilityTimer > 0) ? playerObjects[0]->trans.x
                    : (playerStates[1]->invincibilityTimer > 0)               ? (playerObjects[1]->trans.x - 300.0f)
                                                                : ((playerObjects[0]->trans.x + playerObjects[1]->trans.x) / 2.0 - 150.0);
                playerObjects[0]->trans.x = (playerObjects[0]->trans.x >= 650.0) ? 650.0 : (playerObjects[0]->trans.x <= -950.0) ? -950.0 : playerObjects[0]->trans.x;
                playerObjects[1]->trans.x = playerObjects[0]->trans.x + 300.0f;
                if (playerActionKinds[0] >= 3 || playerActionKinds[1] >= 3) {
                    currentPlayerIndex = (playerActionKinds[0] >= 3) ? 0 : 1;
                    otherPlayerIndex = 1 - currentPlayerIndex;
                    switch (playerActionKinds[otherPlayerIndex]) {
                        case 4:
                            if (playerObjects[currentPlayerIndex]->trans.x * (currentPlayerIndex == 0 ? 1 : -1) < playerObjects[otherPlayerIndex]->trans.x * (otherPlayerIndex == 0 ? 1 : -1)) {
                                currentPlayerIndex = 1 - currentPlayerIndex;
                                otherPlayerIndex = 1 - otherPlayerIndex;
                            }
                            playerStates[currentPlayerIndex]->state = 3;
                            playerStates[otherPlayerIndex]->state = 4;
                            break;
                        case 1:
                            playerStates[currentPlayerIndex]->state = 5;
                            playerStates[otherPlayerIndex]->state = 6;
                            break;
                        case 3:
                            playerStates[currentPlayerIndex]->state = 12;
                            playerStates[otherPlayerIndex]->state = 12;
                            break;
                        case 2:
                            playerStates[currentPlayerIndex]->state = 10;
                            playerStates[otherPlayerIndex]->state = 11;
                            break;
                        case 0:
                            playerStates[currentPlayerIndex]->state = 13;
                            playerStates[otherPlayerIndex]->state = 14;
                            break;
                    }
                    playerStates[0]->stateTimer = playerStates[1]->stateTimer = 0;
                }
            }
            /* fallthrough */
        case 1009:
            if (shoveGameState != 1008 && matchEnded == 0) {
                break;
            }
            for (i = 0; i < 2; i++) {
                playerObjectsLocal[i] = playerObjectsByGroup[i];
                playerStatesByGroup[i] = playerObjectsByGroup[i]->data;
            }
            switch (koopaKidDeparturePhase) {
                case 0:
                    if (matchEnded == 0 && koopaKidDeparturePhase == 0 && (playerObjectsLocal[0]->trans.x > -150.0f || playerObjectsLocal[1]->trans.x < 150.0f)) {
                        koopaKidDeparturePhase = (-playerObjectsLocal[0]->trans.x < playerObjectsLocal[1]->trans.x ? 1 : 2);
                        Hu3DModelPosSet(gameObject->mdlId[9], 0.0f, 100.0f, 0.0f);
                        Hu3DMotionTimeSet(gameObject->mdlId[9], 0.0f);
                        Hu3DModelAttrReset(gameObject->mdlId[9], HU3D_ATTR_DISPOFF);
                        if (pendingSoundEvents[4].eventCount < 3) {
                            pendingSoundEvents[4].eventTimers[pendingSoundEvents[4].eventCount] = 0.0f;
                            pendingSoundEvents[4].eventCount++;
                        }
                    }
                    if (koopaKidDeparturePhase == 0) {
                        break;
                    }
                    /* fallthrough */
                case 1:
                case 2:
                    koopaKidModelData = &Hu3DData[object->mdlId[MODEL_ID_GAME_KOOPA_KID]];
                    koopaKidModelData->rot.y += (koopaKidDeparturePhase == 1 ? 18 : -18);
                    koopaKidModelData->rot.z = (koopaKidDeparturePhase == 1 ? -20 : 20);
                    koopaKidModelData->pos.x += 5.130000000000001 * (koopaKidDeparturePhase == 1 ? 1 : -1);
                    koopaKidModelData->pos.y += 47.0;
                    koopaKidModelData->pos.z += -20.0f;
                    if (koopaKidModelData->pos.y > 1800.0f) {
                        koopaKidDeparturePhase += 2;
                    }
                    break;
            }
            switch (mushroomDropPhase) {
                case 0:
                case 2:
                    mushroomRespawnTimer -= (koopaKidDeparturePhase >= 3 && playerStatesByGroup[0]->state < 16 && playerStatesByGroup[1]->state < 16);
                    if (mushroomRespawnTimer != 0) {
                        break;
                    }
                    Hu3DModelAttrReset(object->mdlId[MODEL_ID_GAME_MUSHROOM], HU3D_ATTR_DISPOFF);
                    Hu3DModelPosSet(object->mdlId[MODEL_ID_GAME_MUSHROOM],
                        (mushroomDropPhase == 0) ? 0.0 : (2.0 * ((rand8() % 256) / 255.0 - 0.5) * 400.0), 1500.0f, 0.0f);
                    mushroomDropPhase = 1;
                    /* fallthrough */
                case 1:
                    mushroomModelData = &Hu3DData[object->mdlId[MODEL_ID_GAME_MUSHROOM]];
                    // descend mushroom
                    mushroomModelData->pos.y -= 10.0f;
                    if (mushroomModelData->pos.y <= 225.0f) {
                        float playerDistsToMushroom[] = { 100000000.0f, 100000000.0f };
                        // make sure it doesn't fall through the ring
                        mushroomModelData->pos.y = (mushroomModelData->pos.y < 100.0f) ? 100.0f : mushroomModelData->pos.y;
                        for (i = 0; i < 2; i++) {
                            if (fabs(playerObjectsLocal[i]->trans.x + (i == 0 ? 40 : -40) - mushroomModelData->pos.x) < 180.0) {
                                playerDistsToMushroom[i] = fabs(playerObjectsLocal[i]->trans.x + (i == 0 ? 40 : -40) - mushroomModelData->pos.x);
                            }
                        }
                        if (playerDistsToMushroom[0] != playerDistsToMushroom[1]) {
                            invinciblePlayerId = (playerDistsToMushroom[0] < playerDistsToMushroom[1]) ? 0 : 1;
                            Hu3DModelAttrSet(object->mdlId[MODEL_ID_GAME_MUSHROOM], HU3D_ATTR_DISPOFF);
                            mushroomDropPhase = 2;
                            mushroomRespawnTimer = 10 * REFRESH_RATE;
                            playerStatesByGroup[invinciblePlayerId]->invincibilityTimer = 5 * REFRESH_RATE;
                            playerStatesByGroup[invinciblePlayerId]->staminaExhausted = 0;
                            if (pendingSoundEvents[3].eventCount < 3) {
                                pendingSoundEvents[3].eventTimers[pendingSoundEvents[3].eventCount] = 0.0f;
                                pendingSoundEvents[3].eventCount++;
                            }
                            invincibilitySoundHandle = HuAudFXPlay(MSM_SE_INVINCIBILITY);
                        }
                    }
                    if (playerStatesByGroup[0]->state >= 16 || playerStatesByGroup[1]->state >= 16 || matchEnded != 0) {
                        mushroomDropPhase = 3;
                        mushroomRespawnTimer = 0;
                    }
                    if (mushroomDropPhase == 1) {
                        break;
                    }
                    /* fallthrough */
                case 3:
                    mushroomRespawnTimer++;
                    if (mushroomRespawnTimer < REFRESH_RATE && mushroomRespawnTimer % 2 == 0) {
                        Hu3DModelAttrReset(object->mdlId[MODEL_ID_GAME_MUSHROOM], HU3D_ATTR_DISPOFF);
                    }
                    else {
                        Hu3DModelAttrSet(object->mdlId[MODEL_ID_GAME_MUSHROOM], HU3D_ATTR_DISPOFF);
                    }
                    break;
            }
            break;
    }
    switch (shoveGameState) {
        case 1009:
            if (matchEnded == 0) {
                if (gameTimelineFrame <= 30) {
                    Hu3DModelPosSet(gameObject->mdlId[MODEL_ID_GAME_KOOPA_KID], 0.0f, (30 - gameTimelineFrame) * 50 * 0.94, 0.0f);
                    Hu3DModelRotSet(gameObject->mdlId[MODEL_ID_GAME_KOOPA_KID], 0.0f, 0.0f, koopaKidDeparturePhase == 3 ? -20 : 20);
                    Hu3DModelAttrReset(gameObject->mdlId[MODEL_ID_GAME_KOOPA_KID], HU3D_ATTR_DISPOFF);
                }
                if (gameTimelineFrame == 30) {
                    Hu3DMotionSet(object->mdlId[MODEL_ID_GAME_KOOPA_KID], object->mtnId[0]);
                    Hu3DModelAttrReset(object->mdlId[MODEL_ID_GAME_KOOPA_KID], HU3D_MOTATTR_LOOP);
                }
                if (gameTimelineFrame == 30) {
                    HuAudFXPlay(MSM_SE_KOOPA_KID_DROP);
                }
                if (gameTimelineFrame >= 30 && gameTimelineFrame <= 37) {
                    koopaKidLandingEase = 1.0 - (gameTimelineFrame - 30) / 7.0;
                    Hu3DModelRotSet(object->mdlId[MODEL_ID_GAME_KOOPA_KID], 25.0 * (koopaKidLandingEase * 0.5 + koopaKidLandingEase * koopaKidLandingEase * 0.5), 0.0f, 0.0f);
                }
            }
            else if (koopaKidDeparturePhase == 0) {
                Hu3DMotionSet(gameObject->mdlId[MODEL_ID_GAME_KOOPA_KID], object->mtnId[0]);
                Hu3DModelAttrReset(gameObject->mdlId[MODEL_ID_GAME_KOOPA_KID], HU3D_MOTATTR_LOOP);
            }
            break;
        case 1010:
            if (matchEnded == 0) {
                if (gameTimelineFrame == 1) {
                    Hu3DMotionSet(object->mdlId[MODEL_ID_GAME_KOOPA_KID], object->mtnId[3]);
                    Hu3DModelAttrReset(object->mdlId[MODEL_ID_GAME_KOOPA_KID], HU3D_MOTATTR_LOOP);
                }
                koopaKidExitModel = &Hu3DData[object->mdlId[MODEL_ID_GAME_KOOPA_KID]];
                koopaKidFacingAngle = (playerObjectsByGroup[resultPlayerGroup]->trans.x < 0.0f) ? -90 : 90;
                koopaKidExitModel->rot.y = koopaKidFacingAngle + 0.8 * (koopaKidExitModel->rot.y - koopaKidFacingAngle);
            }
            break;
    }
    previousCameraMode = cameraMode;
    {
        float cameraModePresets[][6] = { { 3500.0f, 0.0f, 100.0f, 0.0f, -59.0f, 0.0f }, { 3200.0f, 0.0f, 100.0f, 0.0f, -30.0f, 0.0f },
            { 2100.0f, 0.0f, 200.0f, 0.0f, -30.0f, 0.0f }, { 2000.0f, 0.0f, 200.0f, 0.0f, -30.0f, 0.0f },
            { 800.0f, 0.0f, 200.0f, 0.0f, -89.0f, 0.0f }, { 1800.0f, 0.0f, 200.0f, 0.0f, -30.0f, 0.0f } };

        if (resultPlayerGroup != -1 && matchEnded == 0) {
            cameraModePresets[2][1] = playerObjectsByGroup[resultPlayerGroup]->trans.x;
        }
        if (cameraMode == -1) {
            cameraMode = 0;
            cameraTransitionDirection = 1;
            cameraTransitionProgress = 1.0f;
        }
        if (cameraMode == 0) {
            cameraMode = 1;
            cameraTransitionDirection = 1;
            cameraTransitionProgress = 0.5f;
        }
        if (shoveGameState == 1010 && resultPlayerGroup != 2 && gameTimelineFrame >= 15 && cameraMode == 1) {
            cameraMode = 2;
            cameraTransitionDirection = 0;
            cameraTransitionProgress = 0.0f;
        }
        if (previousCameraMode != cameraMode && previousCameraMode != -1) {
            cameraTransitionStartZoom = CZoom;
            cameraTransitionStartCenterY = Center.y;
            cameraTransitionStartCenterZ = Center.z;
            cameraTransitionStartRotationX = CRot.x;
            cameraTransitionStartRotationY = CRot.y;
        }
        if (cameraTransitionProgress <= 1.0f) {
            if (cameraTransitionDirection == 0) {
                cameraTransitionProgress = 1.0 - 0.95 * (1.0f - cameraTransitionProgress);
                cameraBlendProgress = cameraTransitionProgress;
            }
            else {
                cameraTransitionProgress += 0.008333333333333333;
                cameraTransitionProgress = (cameraTransitionProgress > 1.0f) ? 1.0f : cameraTransitionProgress;
                cameraBlendProgress = 0.5 + -0.5 * cosd(180.0f * cameraTransitionProgress);
            }
            cameraTransitionProgress = (cameraTransitionProgress > 1.0f) ? 1.0f : cameraTransitionProgress;
            CZoom = cameraBlendProgress * cameraModePresets[cameraMode][0] + cameraTransitionStartZoom * (1.0f - cameraBlendProgress);
            Center.x = cameraBlendProgress * cameraModePresets[cameraMode][1] + cameraTransitionStartCenterX * (1.0f - cameraBlendProgress);
            Center.y = cameraBlendProgress * cameraModePresets[cameraMode][2] + cameraTransitionStartCenterY * (1.0f - cameraBlendProgress);
            Center.z = cameraBlendProgress * cameraModePresets[cameraMode][3] + cameraTransitionStartCenterZ * (1.0f - cameraBlendProgress);
            CRot.x = cameraBlendProgress * cameraModePresets[cameraMode][4] + cameraTransitionStartRotationX * (1.0f - cameraBlendProgress);
            CRot.y = cameraBlendProgress * cameraModePresets[cameraMode][5] + cameraTransitionStartRotationY * (1.0f - cameraBlendProgress);
        }
    }
    if (shoveGameState == 1001) {
        windOffsetX = 0.0f;
        windOffsetZ = 0.0f;
        if (windState.windUpdatePhase % 2 >= 0) {
            windState.windX = windState.targetWindX;
            windState.windZ = windState.targetWindZ;
            do {
                windState.targetWindX = 2.0 * (rand8() / 255.0) - 1.0;
                windState.targetWindZ = 2.0 * (rand8() / 255.0) - 1.0;
                windState.targetWindX = 0.0f;
                targetWindDistance = (windState.targetWindX - windState.windX) * (windState.targetWindX - windState.windX)
                    + (windState.targetWindZ - windState.windZ) * (windState.targetWindZ - windState.windZ);
            } while (sqrtf(targetWindDistance) < 0.75);
        }
        windOffsetX = windState.windX;
        windOffsetZ = windState.windZ;
        windOffsetX *= windState.windTransitionTime * 2;
        windOffsetZ *= windState.windTransitionTime * 2;
        windState.windUpdatePhase = !windState.windUpdatePhase;
        windState.windTransitionTime = (windState.windTransitionTime > 24) ? 23 : (windState.windTransitionTime - 1 > 0) ? (windState.windTransitionTime - 1) : 0;
        Center.x = 0.0f;
        Center.y = 100.0f + windOffsetZ;
    }
    if (shoveGameState >= 1008) {
        for (i = 0; i < 2; i++) {
            playerModelData = &Hu3DData[playerObjectsByGroup[i]->mdlId[MODEL_ID_PLAYER_PLAYER]];
            characterIndex = (playerDataLocal[i]->character != CHARACTER_BOWSER) ? playerDataLocal[i]->character : 8;
            currentMotionKind = (playerModelData->motId == playerObjectsByGroup[i]->mtnId[4]) ? 0
                : (playerModelData->motId == playerObjectsByGroup[i]->mtnId[6])     ? 1
                : (playerModelData->motId == playerObjectsByGroup[i]->mtnId[7])     ? 2
                                                                    : -1;
            shiftedMotionKind = (playerModelData->motIdShift == playerObjectsByGroup[i]->mtnId[4]) ? 0
                : (playerModelData->motIdShift == playerObjectsByGroup[i]->mtnId[6])     ? 1
                : (playerModelData->motIdShift == playerObjectsByGroup[i]->mtnId[7])     ? 2
                                                                    : -1;
            currentMotionOffsetX = (currentMotionKind == -1) ? 0.0f : playerMotionOffsetsByCharacter[characterIndex][currentMotionKind];
            shiftedMotionOffsetX = (shiftedMotionKind == -1) ? 0.0f : playerMotionOffsetsByCharacter[characterIndex][shiftedMotionKind];
            motionBlendProgress = (playerModelData->motIdShift == -1) ? 0.0f : (playerModelData->motOvlWork.end == 0.0f) ? 1.0f : (playerModelData->motOvlWork.start / playerModelData->motOvlWork.end);
            playerDataLocal[i]->movementSpeed = (i == 0 ? 1 : -1) * (currentMotionOffsetX * (1.0f - motionBlendProgress) + shiftedMotionOffsetX * motionBlendProgress);
            playerObjectsByGroup[i]->trans.x += playerDataLocal[i]->movementSpeed;
            Hu3DModelPosSet(playerObjectsByGroup[i]->mdlId[MODEL_ID_PLAYER_PLAYER], playerObjectsByGroup[i]->trans.x, playerObjectsByGroup[i]->trans.y, playerObjectsByGroup[i]->trans.z);
        }
    }
}

static void ShowWrestlingIntroMessage(void)
{
    ShovePlayerData *playerData[2];
    s16 unreferencedValue;
    s16 window;
    s32 i;

    unreferencedValue = 0;
    for (i = 0; i < 2; i++) {
        playerData[i] = playerObjectsByGroup[i]->data;
    }
    HuWinComKeyWait(PAD_BUTTON_A, PAD_BUTTON_A, PAD_BUTTON_A, PAD_BUTTON_A, REFRESH_RATE);
    HuWinComKeyReset();
    window = HuWinExCreateStyled(36.0f, 344.0f, 504, 96, 6, 0);
    HuWinBGTPLvlSet(window, 0.8f);
    HuWinMesSpeedSet(window, 1);
    HuWinExAnimIn(window);
    HuWinDisablePlayerSet(window,
        0xF & ((playerData[0]->controllerIndex != PAD_IDX_COM) ? ~(1 << playerData[0]->controllerIndex) : 0xF)
            & ((playerData[1]->controllerIndex != PAD_IDX_COM) ? ~(1 << playerData[1]->controllerIndex) : 0xF));
    HuWinMesSet(window, MAKE_MESSID(48, 31));
    if ((playerData[0]->controllerIndex == PAD_IDX_COM || playerData[0]->character == CHARACTER_BOWSER)
        && (playerData[1]->controllerIndex == PAD_IDX_COM || playerData[1]->character == CHARACTER_BOWSER)) {
        HuWinComKeyWait(PAD_BUTTON_A, PAD_BUTTON_A, PAD_BUTTON_A, PAD_BUTTON_A, VERSION_JP ? 48 : REFRESH_RATE);
        HuWinComKeyWait(PAD_BUTTON_A, PAD_BUTTON_A, PAD_BUTTON_A, PAD_BUTTON_A, VERSION_JP ? 90 : REFRESH_RATE);
        HuWinComKeyWait(PAD_BUTTON_A, PAD_BUTTON_A, PAD_BUTTON_A, PAD_BUTTON_A, VERSION_JP ? 72 : REFRESH_RATE);
    }
    HuWinMesWait(window);
    HuWinExAnimOut(window);
    HuWinExCleanup(window);
    messageSequenceEndFlag = 0;
    HuPrcEnd();
}

static void ShowShoveInstructionMessage(void)
{
    float windowAnimationProgress;
    s16 windowId;
    WindowData *windowData;
    ShovePlayerData *playerStates[2];
#if VERSION_PAL
    float secondWindowAnimationProgress;
    double windowStartX;
    double secondWindowStartX;
    float firstMessageSize[2];
    float secondMessageSize[2];
    WindowData *secondWindowData;
#endif
    s32 windowAnimationFrames;
    s16 reservedInputState;
    s32 secondWindowAnimationFrames;
    s32 i;

    reservedInputState = 0;
    for (i = 0; i < 2; i++) {
        playerStates[i] = playerObjectsByGroup[i]->data;
    }
    HuWinComKeyWait(PAD_BUTTON_A, PAD_BUTTON_A, PAD_BUTTON_A, PAD_BUTTON_A, REFRESH_RATE);
    HuWinComKeyReset();
#if VERSION_NTSC
    windowId = HuWinCreate(215.0f, 100.0f, 130, 37, 1);
#else
    HuWinMesMaxSizeGet(1, firstMessageSize, MAKE_MESSID(48, 32));
    windowId = HuWinCreate(-10000.0f, 100.0f, firstMessageSize[0], 0x25, 1);
#endif
    HuSprColorSet(winData[windowId].group, 0, 0xFF, 0xFF, 0xCC);
    HuWinBGTPLvlSet(windowId, 0.9f);
    windowData = &winData[windowId];
#if VERSION_PAL
    windowStartX = windowData->pos_x;
#endif
    windowData->mess_color = 0;
    windowData->mess_shadow_color = 9;
    windowData->attr |= 0x80;
    for (windowAnimationFrames = 15, windowAnimationProgress = 0.0f; windowAnimationFrames != 0; windowAnimationFrames--, windowAnimationProgress += 2.0f / 30) {
        HuWinScaleSet(windowId, 1.0f, 1.0 * (1.0f - (1.0f - windowAnimationProgress) * (1.0f - windowAnimationProgress)));
#if VERSION_NTSC
        HuWinPosSet(windowId, 215.0f, 100.0 + 37.0 * (1.0f - (1.0f - windowAnimationProgress) * (1.0f - windowAnimationProgress)) / -4.0);
#else
        HuWinPosSet(windowId, windowStartX, 100.0 + 37.0 * (1.0f - (1.0f - windowAnimationProgress) * (1.0f - windowAnimationProgress)) / -4.0);
#endif
        HuPrcVSleep();
    }
    HuWinScaleSet(windowId, 1.0f, 1.0f);
    HuWinMesSet(windowId, MAKE_MESSID(48, 32));
    HuPrcSleep(REFRESH_RATE);
    HuWinExAnimOut(windowId);
    HuWinExCleanup(windowId);
#if VERSION_PAL
    windowId = 0;
#endif
    messageSequenceEndFlag = 0;
#if VERSION_PAL
    while (shoveGameState != 1008) {
        HuPrcVSleep();
    }
    HuWinMesMaxSizeGet(1, secondMessageSize, MAKE_MESSID(48, 33));
    windowId = HuWinCreate(-10000.0f, 100.0f, secondMessageSize[0], secondMessageSize[1], 1);
    HuSprColorSet(winData[windowId].group, 0, 0xFF, 0xFF, 0xCC);
    HuWinBGTPLvlSet(windowId, 0.9f);
    secondWindowData = &winData[windowId];
    secondWindowStartX = secondWindowData->pos_x;
    secondWindowData->mess_color = 0;
    secondWindowData->mess_shadow_color = 9;
    secondWindowData->attr |= 0x80;
    for (secondWindowAnimationFrames = 5, secondWindowAnimationProgress = 0.0f; secondWindowAnimationFrames != 0; secondWindowAnimationFrames--, secondWindowAnimationProgress += 0.2f) {
        HuWinScaleSet(windowId, 1.0f, (double)(1.0f - ((1.0f - secondWindowAnimationProgress) * (1.0f - secondWindowAnimationProgress))));
        HuWinPosSet(windowId, secondWindowStartX, 100.0 + (((double)secondMessageSize[1] * (1.0f - ((1.0f - secondWindowAnimationProgress) * (1.0f - secondWindowAnimationProgress)))) / -4.0));
        HuPrcVSleep();
    }
    HuWinScaleSet(windowId, 1.0f, 1.0f);
    HuWinMesSet(windowId, MAKE_MESSID(48, 33));
    HuWinMesSpeedSet(windowId, 0);
    HuPrcSleep(0x32);
    HuWinExAnimOut(windowId);
    HuWinExCleanup(windowId);
    windowId = 0;
#endif
    HuPrcEnd();
}

static void ShoveMain(OMOBJ *object)
{
    ShovePlayerData *playerData[2];
    float messageSize[2];
    s32 soundEffectHandle;
    s32 i;

    for (i = 0; i < 5; i++) {
        s32 shoveSoundEffectIds[] = { 0x7E1, 0x7E2, 0x7E3, 0x7E4, 0x7E5 };

        if (pendingSoundEvents[i].eventCount > 3) {
            pendingSoundEvents[i].eventCount = 3;
        }
        if (pendingSoundEvents[i].eventCount > 0) {
            soundEffectHandle = HuAudFXPlay(shoveSoundEffectIds[i]);
            pendingSoundEvents[i].eventCount--;
        }
    }
    for (i = 0; i < 2; i++) {
        playerData[i] = playerObjectsByGroup[i]->data;
    }
    if (omSysExitReq != 0 && WipeStatGet() == 0) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        object->objFunc = ShoveExit;
    }
    switch (shoveGameState) {
        case 1000:
            if (WipeStatGet() == 0) {
                shoveGameState++;
                gameTimelineFrame = 0;
            }
            break;
        case 1001:
            gameTimelineFrame++;
            if (gameTimelineFrame == 60) {
                shoveGameState = (againstBrowserMode ? 1002 : 1003);
                gameTimelineFrame = 0;
            }
            break;
        case 1002:
            gameTimelineFrame++;
            if (gameTimelineFrame > (s32)(3.5 * REFRESH_RATE)) {
                shoveGameState++;
                gameTimelineFrame = 0;
            }
            break;
        case 1003:
            gameTimelineFrame++;
            if (gameTimelineFrame == 60) {
                shoveGameState = (againstBrowserMode ? 1004 : 1005);
                gameTimelineFrame = 0;
            }
            break;
        case 1004:
            if (gameTimelineFrame == 0) {
                HuPrcChildCreate(ShowWrestlingIntroMessage, 100, 0x2000, 0, HuPrcCurrentGet());
                messageSequenceEndFlag = 1;
                gameTimelineFrame++;
            }
            else if (messageSequenceEndFlag == 0) {
                gameTimelineFrame++;
                if (playerData[0]->state == 0) {
                    shoveGameState++;
                    gameTimelineFrame = 0;
                }
            }
            break;
        case 1005:
            gameTimelineFrame++;
            if (gameTimelineFrame == (((REFRESH_RATE*REFRESH_RATE)+79)/80)*4) {
                shoveGameState++;
                gameTimelineFrame = 0;
            }
            break;
        case 1006:
            if (gameTimelineFrame == 0) {
                HuWinMesMaxSizeGet(1, messageSize, MAKE_MESSID(48, 34));
                shoveMessageWindowId = HuWinCreate(-10000.0f, 400.0f, messageSize[0], messageSize[1], 1);
                HuWinBGTPLvlSet(shoveMessageWindowId, 0.0f);
                HuWinMesSpeedSet(shoveMessageWindowId, 0);
                HuWinMesSet(shoveMessageWindowId, MAKE_MESSID(48, 34));
                HuPrcChildCreate(ShowShoveInstructionMessage, 100, 0x2000, 0, HuPrcCurrentGet());
                messageSequenceEndFlag = 1;
                gameTimelineFrame++;
            }
            else if (messageSequenceEndFlag == 0) {
                gameTimelineFrame++;
                if (playerData[0]->state == 0) {
                    shoveGameState++;
                    gameTimelineFrame = rand8() * (REFRESH_RATE / 2) % 5 + REFRESH_RATE;
                }
            }
            else {
                gameTimelineFrame++;
            }
            break;
        case 1007:
            gameTimelineFrame -= 1;
            if (gameTimelineFrame <= 0) {
                shoveGameState++;
                gameTimelineFrame = 0;
#if VERSION_NTSC
                HuAudFXPlay(MSM_SE_BOWSER_HOWL);
#endif
            }
            break;
        case 1008:
            gameTimelineFrame++;
            if (resultPlayerGroup != -1) {
                shoveGameState = 1009;
                gameTimelineFrame = 0;
                break;
            }
            if (matchTimerHandle == -1 && gameTimelineFrame >= 270 * REFRESH_RATE && playerData[0]->state < 16 && playerData[1]->state < 16) {
                matchTimerHandle = MGSeqTimerCreate(30);
            }
            if (matchTimerHandle == -1) {
                break;
            }
            if (playerData[0]->state < 16 && playerData[1]->state < 16) {
                MGSeqParamSet(matchTimerHandle, 1, (301 * REFRESH_RATE - 1 - gameTimelineFrame) / REFRESH_RATE);
                if (gameTimelineFrame > 300 * REFRESH_RATE) {
                    MGSeqParamSet(matchTimerHandle, 2, -1);
                    matchTimerHandle = -1;
                }
            }
            if (gameTimelineFrame > 300 * REFRESH_RATE && playerData[0]->state < 16 && playerData[1]->state < 16) {
                shoveGameState = 1009;
                gameTimelineFrame = 0;
                matchEnded = 1;
                playerData[0]->state = playerData[1]->state = 17;
                playerData[0]->stateTimer = playerData[1]->stateTimer = 0;
                MGSeqFinishCreate();
                if (backgroundMusicSequenceId != -1) {
                    HuAudSeqFadeOut(backgroundMusicSequenceId, 100);
                    backgroundMusicSequenceId = -1;
                }
            }
            break;
        case 1009:
            if (gameTimelineFrame == 1 && matchTimerHandle != -1) {
                MGSeqParamSet(matchTimerHandle, 2, -1);
                matchTimerHandle = -1;
            }
            gameTimelineFrame++;
            if (gameTimelineFrame >= 2 * REFRESH_RATE) {
                shoveGameState++;
                gameTimelineFrame = 0;
            }
            break;
        case 1010:
            gameTimelineFrame++;
            if (gameTimelineFrame == 15) {
                if (resultPlayerGroup != 2) {
                    if (playerData[resultPlayerGroup]->character != CHARACTER_BOWSER) {
                        HuAudSStreamPlay(1);
                    }
                    else {
                        HuAudSStreamPlay(3);
                    }
                    if (againstBrowserMode) {
                        if (resultPlayerGroup == 0) {
                            _SetFlag(FLAG_ID_MAKE(1, 7));
                        }
                    }
                    else {
                        GWPlayerCoinWinSet(playerData[resultPlayerGroup]->player, 10);
                    }
                }
                else if (resultPlayerGroup == 2) {
                    HuAudSStreamPlay(4);
                }
            }
            if (((((REFRESH_RATE*3)+2)/4)*5.0f) < gameTimelineFrame) {
                if (windSoundHandle != -1) {
                    HuAudFXStop(windSoundHandle);
                    windSoundHandle = -1;
                }
                endSequenceHandle = -1;
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
                object->objFunc = ShoveExit;
            }
            break;
    }
}

static void ShoveGameSetup(OMOBJ *object)
{
    s32 i;

    for (i = 0; i < 2; i++) {
        ShovePlayerData *playerData[2];
        playerData[i] = playerObjectsByGroup[i]->data;
    }
    object->mdlId[MODEL_ID_GAME_KOOPA_KID] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_BKOOPA, 14));
    Hu3DModelAttrSet(object->mdlId[MODEL_ID_GAME_KOOPA_KID], HU3D_MOTATTR_LOOP);
    object->mdlId[MODEL_ID_GAME_WAR_FAN] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M457, 38));
    Hu3DModelAttrSet(object->mdlId[MODEL_ID_GAME_WAR_FAN], HU3D_MOTATTR_LOOP);
    Hu3DModelHookSet(object->mdlId[MODEL_ID_GAME_KOOPA_KID], "g007m0-itemhook_r", object->mdlId[MODEL_ID_GAME_WAR_FAN]);
    Hu3DModelPosSet(object->mdlId[MODEL_ID_GAME_KOOPA_KID], 0.0f, 0.0f, 0.0f);
    Hu3DModelRotSet(object->mdlId[MODEL_ID_GAME_KOOPA_KID], 15.0f, 0.0f, 0.0f);
    object->mtnId[0] = Hu3DJointMotionFile(object->mdlId[MODEL_ID_GAME_KOOPA_KID], DATA_MAKE_NUM(DATADIR_M457, 39));
    object->mtnId[1] = Hu3DJointMotionFile(object->mdlId[MODEL_ID_GAME_KOOPA_KID], DATA_MAKE_NUM(DATADIR_M457, 40));
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[MODEL_ID_GAME_KOOPA_KID], DATA_MAKE_NUM(DATADIR_M457, 41));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[MODEL_ID_GAME_KOOPA_KID], DATA_MAKE_NUM(DATADIR_M457, 42));
    Hu3DMotionSet(object->mdlId[MODEL_ID_GAME_KOOPA_KID], object->mtnId[0]);
    Hu3DModelLayerSet(object->mdlId[MODEL_ID_GAME_WAR_FAN], 2);
    Hu3DModelLayerSet(object->mdlId[MODEL_ID_GAME_KOOPA_KID], 2);
    Hu3DModelAttrSet(object->mdlId[MODEL_ID_GAME_KOOPA_KID], HU3D_ATTR_DISPOFF);
    Hu3DModelShadowSet(object->mdlId[MODEL_ID_GAME_WAR_FAN]);
    Hu3DModelShadowSet(object->mdlId[MODEL_ID_GAME_KOOPA_KID]);
    object->mdlId[MODEL_ID_GAME_SKY] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M457, 16));
    Hu3DModelAttrSet(object->mdlId[MODEL_ID_GAME_SKY], HU3D_MOTATTR_LOOP);
    Hu3DMotionSpeedSet(object->mdlId[MODEL_ID_GAME_SKY], 0.1f);
    Hu3DModelLayerSet(object->mdlId[MODEL_ID_GAME_SKY], 1);
    object->mdlId[MODEL_ID_GAME_RING] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M457, 17));
    Hu3DModelAttrSet(object->mdlId[MODEL_ID_GAME_RING], HU3D_MOTATTR_LOOP);
    Hu3DModelShadowMapSet(object->mdlId[MODEL_ID_GAME_RING]);
    Hu3DModelLayerSet(object->mdlId[MODEL_ID_GAME_RING], 1);
    Hu3DModelScaleSet(object->mdlId[MODEL_ID_GAME_RING], 1.0f, 1.0f, 1.0f);
    object->mdlId[MODEL_ID_GAME_MUSHROOM] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M457, 21));
    Hu3DModelAttrSet(object->mdlId[MODEL_ID_GAME_MUSHROOM], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrSet(object->mdlId[MODEL_ID_GAME_MUSHROOM], HU3D_ATTR_DISPOFF);
    Hu3DModelLayerSet(object->mdlId[MODEL_ID_GAME_MUSHROOM], 2);
    object->mdlId[MODEL_ID_GAME_INVINCE_SHINING] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M457, 19));
    Hu3DModelAttrSet(object->mdlId[MODEL_ID_GAME_INVINCE_SHINING], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrSet(object->mdlId[MODEL_ID_GAME_INVINCE_SHINING], HU3D_ATTR_DISPOFF);
    Hu3DModelLayerSet(object->mdlId[MODEL_ID_GAME_INVINCE_SHINING], 2);
    object->mdlId[MODEL_ID_GAME_INVINCE_SPARKS] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M457, 20));
    Hu3DModelAttrSet(object->mdlId[MODEL_ID_GAME_INVINCE_SPARKS], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrSet(object->mdlId[MODEL_ID_GAME_INVINCE_SPARKS], HU3D_ATTR_DISPOFF);
    Hu3DModelLayerSet(object->mdlId[MODEL_ID_GAME_INVINCE_SPARKS], 4);
    for (i = 0; i < 3; i++) {
        object->mdlId[i + 7] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M457, 18));
        Hu3DModelLayerSet(object->mdlId[i + 7], 4);
        Hu3DModelAttrSet(object->mdlId[i + 7], HU3D_ATTR_DISPOFF);
    }
    for (i = 0; i < 16; i++) {
        s32 playerHudBaseX[] = { 106, 469 };
        s32 playerHudBaseY[] = { 78, 78 };
        s32 staminaIconOffsetsX[] = { 0, 0, -33, -9, 15, 39, 63, -68, 0, 0, -33, -9, 15, 39, 63, -68 };
        s32 staminaIconOffsetsY[] = { 0, 0, -1, -1, -1, -1, -1, -2, 0, 0, -1, -1, -1, -1, -1, -2 };

        if (spriteDataNumbers[i] == DATA_MAKE_NUM(DATADIR_MGCONST, 0)) {
            ShovePlayerData *player = playerObjectsByGroup[i == 7 ? 0 : 1]->data;
            spriteIds[i]
                = espEntry((player->character == CHARACTER_BOWSER) ? DATA_MAKE_NUM(DATADIR_M457, 26) : (spriteDataNumbers[i] + player->character), 0, 0);
        }
        else {
            spriteIds[i] = espEntry(spriteDataNumbers[i], 0, 0);
        }
        espDrawNoSet(spriteIds[i], 0);
        espDispOff(spriteIds[i]);
        espAttrSet(spriteIds[i], HUSPR_ATTR_NOANIM);
        espBankSet(spriteIds[i], 0);
        espPriSet(spriteIds[i], 128 - i);
        espPosSet(spriteIds[i], staminaIconOffsetsX[i] + playerHudBaseX[i < 8 ? 0 : 1], staminaIconOffsetsY[i] + playerHudBaseY[i < 8 ? 0 : 1]);
        if ((i >= 8 ? i - 8 : i) != 0) {
            (void)1; // Required to match.
        }
        else {
            espTPLvlSet(spriteIds[i], 0.5f);
            espColorSet(spriteIds[i], 0, 0, 0);
        }
    }
    for (i = 16; i < 23; i++) {
        s32 extraSpriteIndex = i - 16;
        s32 extraSpriteX[23] = { 288, 288 };
        s32 extraSpriteY[23] = { 240, 240 };

        spriteIds[i] = espEntry(spriteDataNumbers[i], 0, 0);
        espDrawNoSet(spriteIds[i], 0);
        espAttrSet(spriteIds[i], HUSPR_ATTR_NOANIM | HUSPR_ATTR_DISPOFF);
        espBankSet(spriteIds[i], 0);
        espPriSet(spriteIds[i], 128 - i);
        espPosSet(spriteIds[i], extraSpriteX[extraSpriteIndex], extraSpriteY[extraSpriteIndex]);
        espTPLvlSet(spriteIds[i], 1.0f);
    }
    object->objFunc = ShoveGameUpdate;
}

static void ShovePlayerSetup(OMOBJ *object)
{
    s32 opponentPlayerGroup;
    ShovePlayerData *playerData;
    ShovePlayerData *opponentData;
    s32 i;

    playerData = object->data;
    opponentData = playerObjectsByGroup[1 - object->work[0]]->data;
    memset(playerData, 0, sizeof(*playerData));
    playerData->group = object->work[0];
    playerData->player = (GWPlayerCfg[0].group == playerData->group) ? 0
        : (GWPlayerCfg[1].group == playerData->group)                ? 1
        : (GWPlayerCfg[2].group == playerData->group)                ? 2
        : (GWPlayerCfg[3].group == playerData->group)                ? 3
                                                                     : 4;
    opponentPlayerGroup = (GWPlayerCfg[0].group == 1 - playerData->group) ? 0
        : (GWPlayerCfg[1].group == 1 - playerData->group)                 ? 1
        : (GWPlayerCfg[2].group == 1 - playerData->group)                 ? 2
        : (GWPlayerCfg[3].group == 1 - playerData->group)                 ? 3
                                                                          : 4;
    if (playerData->player == 4) {
        OSReport("m457:wrong player group\n");
        playerData->player = playerData->group;
    }
    if (playerData->group == 1 && againstBrowserMode) {
        playerData->character = -1;
        playerData->controllerIndex = PAD_IDX_COM;
        playerData->player = -1;
        playerData->difficulty = 1;
    }
    else {
        playerData->character = GWPlayerCfg[playerData->player].character;
        playerData->controllerIndex = (playerData->character == CHARACTER_BOWSER || GWPlayerCfg[playerData->player].iscom)
            ? PAD_IDX_COM
            : GWPlayerCfg[playerData->player].pad_idx;
        playerData->difficulty = (playerData->controllerIndex != PAD_IDX_COM) ? 0 : GWPlayerCfg[playerData->player].diff;
    }
    playerData->state = 0;
    playerData->stamina = 100;
    playerData->staminaRegenTimer = 0;
    playerData->previousTrigger = -1;
    playerData->reservedPlayerValue = -1;
    playerData->computerActionTimer = 10000;
    playerData->currentMotionIndex = 1;
    playerData->requestedMotionIndex = -1;
    if (playerData->character != CHARACTER_BOWSER) {
        object->mdlId[MODEL_ID_PLAYER_PLAYER] = CharModelCreate(playerData->character, 1);
    }
    else {
        object->mdlId[MODEL_ID_PLAYER_PLAYER] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_BKOOPA, 7));
    }
    omSetTra(object, 0.0f, playerData->group == 0 ? 0 : 300, 0.0f);
    if (playerData->character != CHARACTER_BOWSER) {
        omSetSca(object, 2.5f, 2.5f, 2.5f);
    }
    else {
        omSetSca(object, 2.0f, 2.0f, 2.0f);
    }
    for (i = 0; i < 12; i++) {
        if (playerData->character != CHARACTER_BOWSER) {
            object->mtnId[i] = CharMotionCreate(playerData->character, playerMotionFileIds[i] + (playerMotionUsesCharacterVariant[i] != 0 ? playerData->character : 0));
        }
        else if (bowserMotionFileIds[i] != 0) {
            object->mtnId[i] = Hu3DJointMotionFile(object->mdlId[MODEL_ID_PLAYER_PLAYER], bowserMotionFileIds[i]);
        }
    }
    Hu3DMotionSet(object->mdlId[MODEL_ID_PLAYER_PLAYER], object->mtnId[playerData->currentMotionIndex]);
    Hu3DMotionTimeSet(object->mdlId[MODEL_ID_PLAYER_PLAYER], 10.0f);
    Hu3DModelAttrReset(object->mdlId[MODEL_ID_PLAYER_PLAYER], HU3D_MOTATTR_LOOP);
    Hu3DModelShadowSet(object->mdlId[MODEL_ID_PLAYER_PLAYER]);
    Hu3DModelAttrSet(object->mdlId[MODEL_ID_PLAYER_PLAYER], HU3D_ATTR_DISPOFF);
    Hu3DModelLayerSet(object->mdlId[MODEL_ID_PLAYER_PLAYER], 3);
    CharModelLayerSetAll2(4);
    object->rot.y = playerData->group * 180 + 90;
    object->trans.x = (playerData->group == 0 ? -1 : 1) * 300;
    object->objFunc = ShovePlayerUpdate;
}

static void ShoveSetup(OMOBJ *object)
{
    shoveGameState = 1000;
    gameTimelineFrame = 0;
    reservedBssValue54 = 0;
    reservedBssValue50 = 0;
    if (backgroundMusicSequenceId == -1) {
        backgroundMusicSequenceId = HuAudSeqPlay(MSM_MUS_BACKGROUND);
    }
    windSoundHandle = HuAudFXPlay(MSM_SE_WIND);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
    object->objFunc = ShoveMain;
}

void ObjectSetup(void)
{
    OMOBJ *object;
    s32 i;

    OSReport("******* M457 ObjectSetup *********\n");
    objectProcess = omInitObjMan(600, 0x2000);
    omGameSysInit(objectProcess);
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 30.0f, 20.0f, 25000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    CZoom = 3200.0f;
    Center.x = 0.0f;
    Center.y = 0.0f;
    Center.z = -100.0f;
    CRot.x = -30.0f;
    CRot.y = 0.0f;
    CRot.z = 0.0f;
    {
        Vec pos = { 0.0f, 0.0f, 0.0f };
        Vec destination = { 0.25f, -0.75f, -0.45f };
        GXColor color = { 0xFF, 0xFF, 0xFF, 0xFF };

        sceneLightId = Hu3DGLightCreateV(&pos, &destination, &color);
    }
    Hu3DGLightStaticSet(sceneLightId, 1);
    Hu3DGLightInfinitytSet(sceneLightId);
    {
        Vec camPos;
        Vec camUp;
        Vec camTarget;

        camPos.x = 0.0f;
        camPos.y = 10000.0f;
        camPos.z = 0.0f;
        camUp.x = 0.0f;
        camUp.y = 1.0f;
        camUp.z = 0.0f;
        camTarget.x = 0.0f;
        camTarget.y = 0.0f;
        camTarget.z = 0.5f;
        Hu3DShadowCreate(12.0f, 9000.0f, 10010.0f);
        Hu3DShadowTPLvlSet(0.625f);
        Hu3DShadowPosSet(&camPos, &camUp, &camTarget);
    }
    Hu3DShadowSizeSet(224);
    endSequenceHandle = -1;
    resultPlayerGroup = -1;
    againstBrowserMode = _CheckFlag(FLAG_ID_MAKE(1, 0)) != 0;
    HuWinInit(1);
    gameObject = omAddObjEx(objectProcess, 101, 10, 4, 1, ShoveGameSetup);
    for (i = 0; i < 2; i++) {
        object = playerObjectsByGroup[i] = omAddObjEx(objectProcess, 100, 1, 12, 0, ShovePlayerSetup);
        object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(ShovePlayerData), HU_MEMNUM_OVL);
        object->work[0] = i;
    }
    viewObject = omAddObjEx(objectProcess, 0x7FDA, 0, 0, -1, omOutView);
    mainObject = omAddObjEx(objectProcess, 103, 0, 0, -1, ShoveSetup);
}

static void ShoveExit(OMOBJ *object)
{
    ShovePlayerData *playerData;
    s32 i;

    if (WipeStatGet() == 0) {
        for (i = 0; i < 2; i++) {
            playerData = playerObjectsByGroup[i]->data;
        }
        if (endSequenceHandle >= 0) {
            MGSeqKill(endSequenceHandle);
        }
        MGSeqKillAll();
        HuAudAllStop();
        omOvlReturnEx(1, 1);
    }
}
