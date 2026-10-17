#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/esprite.h"
#include "game/gamework.h"
#include "game/hsfex.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/printfunc.h"
#include "game/wipe.h"

// #ifndef __MWERKS__
#include "game/frand.h"
#include "version.h"
// #endif

typedef struct CharacterSelectionWork {
    /* 0x00 */ s32 selectionState;
    /* 0x04 */ s16 selectionSprites[8];
    /* 0x14 */ s16 characterModels[8];
    /* 0x24 */ u8 playerSettings[2];
    /* 0x28 */ s32 isComputerPlayer;
    /* 0x2C */ s32 inputDelay;
} CharacterSelectionWork; /* size = 0x30 */

typedef struct ScoreDisplayWork {
    /* 0x00 */ s16 scoreDigits[6];
    /* 0x0C */ s32 currentScore;
    /* 0x10 */ s32 recordScore;
    /* 0x14 */ s32 recordBeaten;
} ScoreDisplayWork; /* size = 0x18 */

typedef struct SequenceDisplayWork {
    /* 0x00 */ s32 sequenceState;
    /* 0x04 */ s32 sequenceMode;
    /* 0x08 */ s16 sequenceHandle;
} SequenceDisplayWork; /* size = 0xC */

typedef struct TimerDisplayWork {
    /* 0x00 */ s32 menuState;
    /* 0x04 */ s16 menuSelection;
    /* 0x08 */ s32 stateTimer;
} TimerDisplayWork; /* size = 0xC */

typedef struct SelectionEffectWork {
    /* 0x00 */ s32 playerIndex;
    /* 0x04 */ s32 animationState;
    /* 0x08 */ u32 effectType;
    /* 0x0C */ s16 effectSprites[2];
} SelectionEffectWork; /* size = 0x10 */

typedef struct PlayerCharacterWork {
    /* 0x000 */ s32 playerIndex;
    /* 0x004 */ s32 animationState;
    /* 0x008 */ s32 characterType;
    /* 0x00C */ Vec worldPosition;
    /* 0x018 */ float modelRotationX;
    /* 0x01C */ float modelRotationY;
    /* 0x020 */ float modelRotationZ;
    /* 0x024 */ char pad24[0x4];
    /* 0x028 */ s32 modelIds[5];
    /* 0x03C */ s32 modelMotionIds[5];
    /* 0x050 */ float movementVelocityX;
    /* 0x054 */ float effectAlpha;
    /* 0x058 */ float movementVelocityZ;
    /* 0x05C */ float movementSpeed;
    /* 0x060 */ s16 currentMotionIndex;
    /* 0x062 */ s16 modelIdsBySlot[0xC];
    /* 0x07A */ s16 modelMotionsBySlot[0xC][6];
    /* 0x10A */ s16 effectSprites[2];
    /* 0x10E */ char pad10E[2];
} PlayerCharacterWork; /* size = 0x110 */

typedef struct CameraWork {
    /* 0x00 */ Vec cameraEye;
    /* 0x0C */ Vec cameraTarget;
    /* 0x18 */ float cameraBank;
    /* 0x1C */ float cameraZoom;
    /* 0x20 */ float cameraRotation;
    /* 0x24 */ float eyeStartX;
    /* 0x28 */ float eyeStartY;
    /* 0x2C */ float eyeStartZ;
    /* 0x30 */ float targetStartX;
    /* 0x34 */ float targetStartY;
    /* 0x38 */ float targetStartZ;
    /* 0x3C */ char cameraTransitionPadding[0xC];
    /* 0x48 */ Vec eyeDestination;
    /* 0x54 */ Vec targetDestination;
    /* 0x60 */ char cameraInterpolationPadding[0xC];
    /* 0x6C */ float eyeTransitionProgress;
    /* 0x70 */ float eyeTransitionStep;
    /* 0x74 */ float targetTransitionProgress;
    /* 0x78 */ float targetTransitionStep;
} CameraWork; /* size = 0x7C */

typedef struct DirectionalLightSetup {
    /* 0x00 */ Vec lightPosition;
    /* 0x0C */ Vec lightDirection;
    /* 0x18 */ GXColor lightColor;
} DirectionalLightSetup; /* size = 0x1C */

void CameraEyeSet(float eyeX, float eyeY, float eyeZ);
void CameraTargetSet(float targetX, float targetY, float targetZ);
void CameraDebugInputUpdate(void);
void PlayerCharacterStateProcess(void);
void PlayerCharacterIdleState(PlayerCharacterWork *character);
void PlayerCharacterIntroState(PlayerCharacterWork *character);
void CharacterModelResourcesInit(void);
void PlayerCharacterSelectionEffectState(PlayerCharacterWork *character);
void PlayerCharacterSelectionFlashState(PlayerCharacterWork *character);
void PlayerCharacterWalkLeftState(PlayerCharacterWork *character);
void PlayerCharacterWalkRightState(PlayerCharacterWork *character);
void PlayerCharacterWalkDepthState(PlayerCharacterWork *character);
void PlayerCharacterScoreEffectState(PlayerCharacterWork *character);
void PlayerCharacterFallState(PlayerCharacterWork *character);
void PlayerCharacterTurnState(PlayerCharacterWork *character);
void PlayerCharacterExitLeftState(PlayerCharacterWork *character);
void PlayerCharacterEnterRightState(PlayerCharacterWork *character);
void PlayerCharacterMotionIntegrate(PlayerCharacterWork *character, float turnRate);
void PlayerCharacterScoreTurnState(PlayerCharacterWork *character);
void PlayerCharacterModelUpdate(PlayerCharacterWork *character);
float AngleInterpolate(float startAngle, float targetAngle, float interpolationRate);
void PlayerCharacterModelSet(s32 playerIndex, u32 modelType);
void ResultsSequenceProcess(void);
void ResultsSummaryProcess(void);
void PlayerCharacterResetProcess(void);
void SelectionConfirmProcess(void);
void SelectedCharacterReactProcess(void);
void PlayerCharacterRaceProcess(void);
void WinnerCharacterProcess(void);
void AllCharactersCelebrateProcess(void);
void AllCharactersReactProcess(void);
void TimerDisplayProcess(void);
s32 TimerFramesGet(void);
void TimerStart(s32 frameCount);
void TimerCancel(void);
void SequenceDisplayProcess(void);
void SequenceDisplayStart(s32 sequenceMode);
s32 SequenceDisplayIsActive(void);
void ScoreDisplayProcess(void);
void ScoreAdd(s32 scoreDelta);
s32 ScoreGet(void);
s32 RecordScoreGet(void);
s32 RecordScoreUpdate(void);
void DirectionalLightProcess(void);
void CharacterSelectionProcess(void);
void CharacterSelectionWait(CharacterSelectionWork *selection);
void CharacterSelectionInputProcess(CharacterSelectionWork *selection);
void CharacterSelectionConfirmProcess(CharacterSelectionWork *selection);
void CharacterSelectionFinish(CharacterSelectionWork *selection);
void SelectionEffectProcess(void);
void SelectionEffectWait(SelectionEffectWork *effect);
void SelectionEffectAppearProcess(SelectionEffectWork *effect);
void SelectionEffectPulseProcess(SelectionEffectWork *effect);
void ObjectShutdownProcess(void);
void SelectionEffectDisappearProcess(SelectionEffectWork *effect);
void GameMainProcess(void);
void CameraProcess(void);

HUPROCESS *objectManagerProcess;
HUPROCESS *childProcesses[0x13];
CameraWork cameraWork;
PlayerCharacterWork playerCharacterWork[5];
SelectionEffectWork selectionEffectWork[5];
TimerDisplayWork timerDisplayWork;
SequenceDisplayWork sequenceDisplayWork;
ScoreDisplayWork scoreDisplayWork;
s16 lightHandles[1];
CharacterSelectionWork characterSelectionWork;
s32 selectedPlayerIndex;
s32 challengerPlayerIndex;
s32 playerCountThreshold;
s32 rumbleEnabled;
s32 cameraDebugEnabled;
u8 rumbleController;
s16 resultSequenceHandle;

static const s32 characterModelResourceIds[0xC] = {
    DATA_MAKE_NUM(DATADIR_M459, 0x05),
    DATA_MAKE_NUM(DATADIR_M459, 0x03),
    DATA_MAKE_NUM(DATADIR_M459, 0x04),
    DATA_MAKE_NUM(DATADIR_M459, 0x01),
    DATA_MAKE_NUM(DATADIR_M459, 0x02),
    DATA_MAKE_NUM(DATADIR_M459, 0x05),
    DATA_MAKE_NUM(DATADIR_M459, 0x03),
    DATA_MAKE_NUM(DATADIR_M459, 0x04),
    DATA_MAKE_NUM(DATADIR_M459, 0x01),
    DATA_MAKE_NUM(DATADIR_M459, 0x02),
    DATA_MAKE_NUM(DATADIR_M459, 0x0E),
    DATA_MAKE_NUM(DATADIR_M459, 0x0F),
};
static const s32 characterMotionResourceIds[5][6] = {
    {
        DATA_MAKE_NUM(DATADIR_M459, 0x28),
        DATA_MAKE_NUM(DATADIR_M459, 0x29),
        DATA_MAKE_NUM(DATADIR_M459, 0x2A),
        DATA_MAKE_NUM(DATADIR_M459, 0x2C),
        DATA_MAKE_NUM(DATADIR_M459, 0x2B),
        DATA_MAKE_NUM(DATADIR_M459, 0x2D),
    },
    {
        DATA_MAKE_NUM(DATADIR_M459, 0x1C),
        DATA_MAKE_NUM(DATADIR_M459, 0x1D),
        DATA_MAKE_NUM(DATADIR_M459, 0x1E),
        DATA_MAKE_NUM(DATADIR_M459, 0x20),
        DATA_MAKE_NUM(DATADIR_M459, 0x1F),
        DATA_MAKE_NUM(DATADIR_M459, 0x21),
    },
    {
        DATA_MAKE_NUM(DATADIR_M459, 0x22),
        DATA_MAKE_NUM(DATADIR_M459, 0x23),
        DATA_MAKE_NUM(DATADIR_M459, 0x24),
        DATA_MAKE_NUM(DATADIR_M459, 0x26),
        DATA_MAKE_NUM(DATADIR_M459, 0x25),
        DATA_MAKE_NUM(DATADIR_M459, 0x27),
    },
    {
        DATA_MAKE_NUM(DATADIR_M459, 0x10),
        DATA_MAKE_NUM(DATADIR_M459, 0x11),
        DATA_MAKE_NUM(DATADIR_M459, 0x12),
        DATA_MAKE_NUM(DATADIR_M459, 0x14),
        DATA_MAKE_NUM(DATADIR_M459, 0x13),
        DATA_MAKE_NUM(DATADIR_M459, 0x15),
    },
    {
        DATA_MAKE_NUM(DATADIR_M459, 0x16),
        DATA_MAKE_NUM(DATADIR_M459, 0x17),
        DATA_MAKE_NUM(DATADIR_M459, 0x18),
        DATA_MAKE_NUM(DATADIR_M459, 0x1A),
        DATA_MAKE_NUM(DATADIR_M459, 0x19),
        DATA_MAKE_NUM(DATADIR_M459, 0x1B),
    },
};
static const s32 effectSpriteResourceIds[2] = { DATA_MAKE_NUM(DATADIR_M459, 0x3C), DATA_MAKE_NUM(DATADIR_M459, 0x3D) };
static const s32 selectionEffectResourceIds[2] = { DATA_MAKE_NUM(DATADIR_M459, 0x3A), DATA_MAKE_NUM(DATADIR_M459, 0x3B) };
static const s32 scoreDigitResourceIds[6] = {
    DATA_MAKE_NUM(DATADIR_M459, 0x30),
    DATA_MAKE_NUM(DATADIR_M459, 0x30),
    DATA_MAKE_NUM(DATADIR_M459, 0x2F),
    DATA_MAKE_NUM(DATADIR_M459, 0x2F),
    DATA_MAKE_NUM(DATADIR_M459, 0x31),
    DATA_MAKE_NUM(DATADIR_M459, 0x2E),
};
static const s32 cursorModelResourceIds[1] = { DATA_MAKE_NUM(DATADIR_M459, 0x00) };
static const s32 characterSelectionModelResourceIds[8] = {
    DATA_MAKE_NUM(DATADIR_M459, 0x06),
    DATA_MAKE_NUM(DATADIR_M459, 0x07),
    DATA_MAKE_NUM(DATADIR_M459, 0x08),
    DATA_MAKE_NUM(DATADIR_M459, 0x09),
    DATA_MAKE_NUM(DATADIR_M459, 0x0A),
    DATA_MAKE_NUM(DATADIR_M459, 0x0B),
    DATA_MAKE_NUM(DATADIR_M459, 0x0C),
    DATA_MAKE_NUM(DATADIR_M459, 0x0D),
};
static const s32 selectionSpriteResourceIds[8] = {
    DATA_MAKE_NUM(DATADIR_M459, 0x32),
    DATA_MAKE_NUM(DATADIR_M459, 0x33),
    DATA_MAKE_NUM(DATADIR_M459, 0x34),
    DATA_MAKE_NUM(DATADIR_M459, 0x35),
    DATA_MAKE_NUM(DATADIR_M459, 0x36),
    DATA_MAKE_NUM(DATADIR_M459, 0x37),
    DATA_MAKE_NUM(DATADIR_M459, 0x38),
    DATA_MAKE_NUM(DATADIR_M459, 0x39),
};
static const s32 roundDelayFrames[0x63] = { 0x14, 0x13, 0x12, 0x11, 0x10, 0xF, 0xE, 0xD, 0xC, 0xB, 0xA, 9, 8, 7, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 };

static const s32 controllerButtonMasks[4] = {0x80000000, 0x40000000, 0x20000000, 0x10000000};

void ObjectSetup(void)
{
    s32 processIndex;
    u32 controllerCapabilities;

    objectManagerProcess = omInitObjMan(1, 0x2000);
    omGameSysInit(objectManagerProcess);
    CharacterModelResourcesInit();
    controllerCapabilities = HuPadRumbleGet();
    if ((controllerCapabilities & 0x80000000) != 0) {
        rumbleEnabled = GWGameStat.rumble;
    }
    else {
        rumbleEnabled = 0;
    }
    cameraDebugEnabled = 0;
    for (processIndex = 0; processIndex < 0x13; processIndex++) {
        childProcesses[processIndex] = NULL;
    }
    childProcesses[0] = HuPrcChildCreate(ObjectShutdownProcess, 0x100, 0x1000, 0, objectManagerProcess);
    childProcesses[1] = HuPrcChildCreate(GameMainProcess, 0x101, 0x3000, 0, objectManagerProcess);
}

void CharacterModelResourcesInit(void)
{
    PlayerCharacterWork *character;
    s32 modelId;
    s32 playerIndex;
    CharacterSelectionWork *selection;
    s16 *cursorModelIds;
    s32 playerIndex2;
    s16 *scoreDigitSprites;
    SelectionEffectWork *effect;

    for (playerIndex = 0; playerIndex < 5; playerIndex++) {
        character = &playerCharacterWork[playerIndex];
        for (modelId = 0; modelId < 0xC; modelId++) {
            if ((modelId >= 5) && (modelId <= 9)) {
                character->modelIdsBySlot[modelId] = Hu3DModelLink(character->modelIdsBySlot[modelId - 5]);
            }
            else {
                character->modelIdsBySlot[modelId] = Hu3DModelCreateFile(characterModelResourceIds[modelId]);
            }
            if ((modelId >= 0) && (modelId <= 4)) {
                Hu3DModelLayerSet(character->modelIdsBySlot[modelId], 2);
            }
            else if ((modelId >= 5) && (modelId <= 9)) {
                Hu3DModelLayerSet(character->modelIdsBySlot[modelId], 0);
                Hu3DModelAttrSet(character->modelIdsBySlot[modelId], HU3D_ATTR_CULL_FRONT);
            }
            else {
                Hu3DModelLayerSet(character->modelIdsBySlot[modelId], 3);
            }
            if (modelId == 0xA) {
                Hu3DModelAttrSet(character->modelIdsBySlot[modelId], HU3D_ATTR_ZWRITE_OFF);
            }
            if ((modelId >= 0) && (modelId <= 4)) {
                Hu3DModelShadowSet(character->modelIdsBySlot[modelId]);
            }
            Hu3DModelAttrSet(character->modelIdsBySlot[modelId], HU3D_ATTR_DISPOFF);
        }
        for (playerIndex2 = 0; playerIndex2 < 5; playerIndex2++) {
            for (modelId = 0; modelId < 6; modelId++) {
                character->modelMotionsBySlot[playerIndex2][modelId] = Hu3DJointMotionFile(character->modelIdsBySlot[playerIndex2], characterMotionResourceIds[playerIndex2][modelId]);
            }
        }
        for (modelId = 0; modelId < 2; modelId++) {
            character->effectSprites[modelId] = espEntry(effectSpriteResourceIds[modelId], 0, 0);
            espDispOff(character->effectSprites[modelId]);
        }
    }
    CharEffectLayerSet(3);
    for (playerIndex = 0; playerIndex < 5; playerIndex++) {
        effect = &selectionEffectWork[playerIndex];
        for (modelId = 0; modelId < 2; modelId++) {
            effect->effectSprites[modelId] = espEntry(selectionEffectResourceIds[modelId], 0, 0);
            espDispOff(effect->effectSprites[modelId]);
        }
    }
    scoreDigitSprites = scoreDisplayWork.scoreDigits;
    for (playerIndex = 0; playerIndex < 6; playerIndex++) {
        scoreDigitSprites[playerIndex] = espEntry(scoreDigitResourceIds[playerIndex], 0, 0);
    }
    espColorSet(scoreDigitSprites[5], 0, 0, 0);
    cursorModelIds = lightHandles;
    for (playerIndex = 0; playerIndex < 1; playerIndex++) {
        cursorModelIds[playerIndex] = Hu3DModelCreateFile(cursorModelResourceIds[playerIndex]);
        Hu3DModelLayerSet(cursorModelIds[playerIndex], 1);
        Hu3DModelShadowMapSet(cursorModelIds[playerIndex]);
    }
    selection = &characterSelectionWork;
    for (playerIndex = 0; playerIndex < 8; playerIndex++) {
        selection->characterModels[playerIndex] = Hu3DModelCreateFile(characterSelectionModelResourceIds[playerIndex]);
        Hu3DModelLayerSet(selection->characterModels[playerIndex], 2);
        Hu3DModelAttrSet(selection->characterModels[playerIndex], HU3D_ATTR_ZWRITE_OFF);
        Hu3DModelAttrSet(selection->characterModels[playerIndex], HU3D_ATTR_DISPOFF);
    }
    for (playerIndex = 0; playerIndex < 8; playerIndex++) {
        selection->selectionSprites[playerIndex] = espEntry(selectionSpriteResourceIds[playerIndex], 0, 0);
        espDispOff(selection->selectionSprites[playerIndex]);
    }
}

void ObjectShutdownProcess(void)
{
    s32 processIndex;

    while (omSysExitReq == 0) {
        HuPrcVSleep();
    }
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);

    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    HuAudFadeOut(0x3E8);
    for (processIndex = 0; processIndex < 0x13; processIndex++) {
        if ((processIndex != 0) && childProcesses[processIndex]) {
            HuPrcKill(childProcesses[processIndex]);
            childProcesses[processIndex] = NULL;
        }
    }
    HuPrcVSleep();
    omOvlReturnEx(1, 1);
    HuPrcEnd();
    childProcesses[0] = NULL;
}

void GameMainProcess(void)
{
    s32 playerIndex;
    s32 challengerEffectType;
    s32 otherCharacterEffectType;
    s32 roundDurationFrames;

    childProcesses[2] = HuPrcChildCreate(CameraProcess, 0xFF, 0x1000, 0, HuPrcCurrentGet());
    for (playerIndex = 0; playerIndex < 5; playerIndex++) {
        childProcesses[playerIndex + 3] = HuPrcChildCreate(PlayerCharacterStateProcess, 0x102, 0x3000, 0, HuPrcCurrentGet());
        *((s32 *)&childProcesses[playerIndex + 3]->user_data) = playerIndex;
    }
    for (playerIndex = 0; playerIndex < 5; playerIndex++) {
        childProcesses[playerIndex + 8] = HuPrcChildCreate(SelectionEffectProcess, 0x105, 0x1000, 0, HuPrcCurrentGet());
        *((s32 *)&childProcesses[playerIndex + 8]->user_data) = playerIndex;
    }
    childProcesses[0x11] = HuPrcChildCreate(CharacterSelectionProcess, 0x103, 0x2000, 0, HuPrcCurrentGet());
    for (playerIndex = 0; playerIndex < 5; playerIndex++) {
        PlayerCharacterModelSet(playerIndex, frandmod(5));
    }
    childProcesses[0xD] = HuPrcChildCreate(TimerDisplayProcess, 0x200, 0x2000, 0, HuPrcCurrentGet());
    childProcesses[0xE] = HuPrcChildCreate(SequenceDisplayProcess, 0x103, 0x2000, 0, HuPrcCurrentGet());
    childProcesses[0xF] = HuPrcChildCreate(ScoreDisplayProcess, 0x104, 0x2000, 0, HuPrcCurrentGet());
    childProcesses[0x10] = HuPrcChildCreate(DirectionalLightProcess, 0x110, 0x2000, 0, HuPrcCurrentGet());
    HuPrcVSleep();
    CameraTargetSet(0.0f, 90.0f, -400.0f);
    CameraEyeSet(0.0f, 300.0f, 1100.0f);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);

    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    childProcesses[0x12] = HuPrcChildCreate(ResultsSequenceProcess, 0x100, 0x4000, 0, HuPrcCurrentGet());

    while (childProcesses[0x12]) {
        HuPrcVSleep();
    }
    while (TRUE) {
        challengerPlayerIndex = frandmod(5);
        otherCharacterEffectType = frandmod(5);
        challengerEffectType = frandmod(5);
        if (otherCharacterEffectType == challengerEffectType) {
            challengerEffectType = (challengerEffectType + 1) % 5;
        }
        for (playerIndex = 0; playerIndex < 5; playerIndex++) {
            if (playerIndex != challengerPlayerIndex) {
                selectionEffectWork[playerIndex].effectType = otherCharacterEffectType;
            }
        }
        selectionEffectWork[challengerPlayerIndex].effectType = challengerEffectType;
        if (ScoreGet() > 0) {
            for (playerIndex = 0; playerIndex < 5; playerIndex++) {
                PlayerCharacterModelSet(playerIndex, frandmod(5));
            }
            childProcesses[0x12] = HuPrcChildCreate(AllCharactersReactProcess, 0x100, 0x4000, 0, HuPrcCurrentGet());
            while (childProcesses[0x12]) {
                HuPrcVSleep();
            }
            HuPrcSleep(0x3C);
            childProcesses[0x12] = HuPrcChildCreate(PlayerCharacterResetProcess, 0x100, 0x4000, 0, HuPrcCurrentGet());
            while (childProcesses[0x12]) {
                HuPrcVSleep();
            }
        }
        roundDurationFrames = roundDelayFrames[ScoreGet()] * REFRESH_RATE;
        TimerStart(roundDurationFrames);
        characterSelectionWork.selectionState = 1;
        while (TRUE) {
            if ((characterSelectionWork.selectionState == 0) || (TimerFramesGet() <= 0)) {
                break;
            }
            HuPrcVSleep();
        }
        TimerCancel();
        if (characterSelectionWork.selectionState != 0) {
            characterSelectionWork.selectionState = 2;
            childProcesses[0x12] = HuPrcChildCreate(WinnerCharacterProcess, 0x100, 0x4000, 0, HuPrcCurrentGet());
            while (childProcesses[0x12]) {
                HuPrcVSleep();
            }
        }
        else {
            childProcesses[0x12] = HuPrcChildCreate(SelectionConfirmProcess, 0x100, 0x4000, 0, HuPrcCurrentGet());
            while (childProcesses[0x12]) {
                HuPrcVSleep();
            }
            if (selectedPlayerIndex == challengerPlayerIndex) {
                childProcesses[0x12] = HuPrcChildCreate(SelectedCharacterReactProcess, 0x100, 0x4000, 0, HuPrcCurrentGet());
                while (childProcesses[0x12]) {
                    HuPrcVSleep();
                }
                goto label;
            }
            else {
                
                childProcesses[0x12] = HuPrcChildCreate(PlayerCharacterRaceProcess, 0x100, 0x4000, 0, HuPrcCurrentGet());
                while (childProcesses[0x12]) {
                    HuPrcVSleep();
                }
                if(0) {
                    label:
                    ScoreAdd(1);
                    if (ScoreGet() != 0x63) {
                        childProcesses[0x12] = HuPrcChildCreate(AllCharactersCelebrateProcess, 0x100, 0x4000, 0, HuPrcCurrentGet());
                        while (childProcesses[0x12]) {
                            HuPrcVSleep();
                        }
                        continue;
                    } 
                }
            }
        }
        break;
    }
    childProcesses[0x12] = HuPrcChildCreate(ResultsSummaryProcess, 0x100, 0x4000, 0, HuPrcCurrentGet());

    while (childProcesses[0x12]) {
        HuPrcVSleep();
    }
    mgRecordExtra = ScoreGet();
    omSysExitReq = 1;
    HuPrcEnd();
    childProcesses[1] = NULL;
    childProcesses[1] = NULL;
}

void CameraProcess(void)
{
    float screenPosition;
    float transitionFactor;
    CameraWork *camera;

    camera = &cameraWork;
    camera->cameraEye.x = 0.0f;
    camera->cameraEye.y = 300.0f;
    camera->cameraEye.z = 1100.0f;
    camera->cameraTarget.x = 90.0f;
    camera->cameraTarget.y = -400.0f;
    camera->cameraTarget.z = 0.0f;
    camera->cameraBank = 0.0f;
    camera->cameraZoom = 1.0f;
    camera->cameraRotation = 0.0f;
    camera->targetTransitionProgress = 1.0f;
    camera->eyeTransitionProgress = 1.0f;
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraPerspectiveSet(1, 30.0f, 10.0f, 8000.0f, 1.2f);
    while (TRUE) {
        CameraDebugInputUpdate();
        if (camera->eyeTransitionProgress < 1.0f) {
            transitionFactor = sind((90.0f * camera->eyeTransitionProgress));
            camera->cameraEye.x = camera->eyeStartX + (transitionFactor * (camera->eyeDestination.x - camera->eyeStartX));
            camera->cameraEye.y = camera->eyeStartY + (transitionFactor * (camera->eyeDestination.y - camera->eyeStartY));
            camera->cameraEye.z = camera->eyeStartZ + (transitionFactor * (camera->eyeDestination.z - camera->eyeStartZ));
            if ((camera->eyeTransitionProgress += camera->eyeTransitionStep) >= 1.0f) {
                camera->cameraEye = camera->eyeDestination;
            }
        }
        if (camera->targetTransitionProgress < 1.0f) {
            transitionFactor = sind((90.0f * camera->targetTransitionProgress));
            camera->cameraTarget.x = camera->targetStartX + (transitionFactor * (camera->targetDestination.x - camera->targetStartX));
            camera->cameraTarget.y = camera->targetStartY + (transitionFactor * (camera->targetDestination.y - camera->targetStartY));
            camera->cameraTarget.z = camera->targetStartZ + (transitionFactor * (camera->targetDestination.z - camera->targetStartZ));
            if ((camera->targetTransitionProgress += camera->targetTransitionStep) >= 1.0f) {
                camera->cameraTarget = camera->targetDestination;
            }
        }
        Hu3DCameraPosSet(1, camera->cameraEye.x, camera->cameraEye.y, camera->cameraEye.z, camera->cameraBank, camera->cameraZoom, camera->cameraRotation,
            camera->cameraTarget.x, camera->cameraTarget.y, camera->cameraTarget.z);
        HuPrcVSleep();
    }
}

void CameraEyeSet(float eyeX, float eyeY, float eyeZ)
{
    CameraWork *camera = &cameraWork;
    camera->cameraEye.x = eyeX;
    camera->cameraEye.y = eyeY;
    camera->cameraEye.z = eyeZ;
}

void CameraTargetSet(float targetX, float targetY, float targetZ)
{
    CameraWork *camera = &cameraWork;
    camera->cameraTarget.x = targetX;
    camera->cameraTarget.y = targetY;
    camera->cameraTarget.z = targetZ;
}

void CameraDebugInputUpdate(void)
{
    CameraWork *camera;

    if (cameraDebugEnabled != 0) {
        camera = &cameraWork;
        camera->cameraTarget.x += (0.25f * HuPadStkX[0]);
        camera->cameraTarget.y += (0.25f * HuPadStkY[0]);
        camera->cameraEye.z += (0.25f * HuPadTrigL[0]);
        camera->cameraEye.z -= (0.25f * HuPadTrigR[0]);
        camera->cameraEye.x += (0.25f * HuPadSubStkX[0]);
        camera->cameraEye.y += (0.25f * HuPadSubStkY[0]);
    }
}

void CameraDebugPrint(void)
{
    CameraWork *camera = &cameraWork;
    print8(0x20, 0x40, 1.5f, "AT  : x=%.2f y=%.2f z=%.2f", camera->cameraTarget.x, camera->cameraTarget.y, camera->cameraTarget.z);
    print8(0x20, 0x50, 1.5f, "EYE : x=%.2f y=%.2f z=%.2f", camera->cameraEye.x, camera->cameraEye.y, camera->cameraEye.z);
}

static const u8 characterDustVariantIds[8] = { 9, 0xE, 0xC, 0xA, 0xB };

void PlayerCharacterStateProcess(void)
{
    PlayerCharacterWork *character;
    s32 playerIndex;
    s32 playerIndex2;

    playerIndex2 = (s32)HuPrcCurrentGet()->user_data;
    character = &playerCharacterWork[playerIndex2];
    character->playerIndex = playerIndex2;
    character->animationState = 0;
    character->worldPosition.x = -260.0f + (130.0f * playerIndex2);
    character->worldPosition.y = 0.0f;
    character->worldPosition.z = 0.0f;
    character->modelRotationX = 0.0f;
    character->modelRotationY = 0.0f;
    character->modelRotationZ = 0.0f;
    character->movementVelocityX = 0.0f;
    character->effectAlpha = 0.0f;
    character->movementVelocityZ = 0.0f;
    Hu3DMotionSet(character->modelIdsBySlot[character->characterType], character->modelMotionsBySlot[character->characterType][character->currentMotionIndex = 0]);
    Hu3DModelAttrSet(character->modelIdsBySlot[character->characterType], HU3D_MOTATTR_LOOP);
    for (playerIndex = 0; playerIndex < 5; playerIndex++) {
        character->modelIds[playerIndex] = CharNpcDustSet(character->modelIdsBySlot[playerIndex], character->modelMotionsBySlot[playerIndex][2], 1, characterDustVariantIds[playerIndex]);
        character->modelMotionIds[playerIndex] = CharNpcDustSet(character->modelIdsBySlot[playerIndex], character->modelMotionsBySlot[playerIndex][1], 0, characterDustVariantIds[playerIndex]);
    }
    while (TRUE) {
        switch (character->animationState) {
            case 0:
                PlayerCharacterIdleState(character);
                break;
            case 1:
                PlayerCharacterIntroState(character);
                break;
            case 3:
                PlayerCharacterSelectionEffectState(character);
                selectionEffectWork[character->playerIndex].animationState = 3;
                break;
            case 4:
                PlayerCharacterSelectionFlashState(character);
                break;
            case 5:
                PlayerCharacterWalkLeftState(character);
                break;
            case 6:
                PlayerCharacterWalkRightState(character);
                break;
            case 7:
                PlayerCharacterScoreEffectState(character);
                break;
            case 8:
                PlayerCharacterFallState(character);
                break;
            case 9:
                PlayerCharacterTurnState(character);
                break;
            case 10:
                PlayerCharacterEnterRightState(character);
                break;
            case 11:
                PlayerCharacterExitLeftState(character);
                break;
            case 12:
                PlayerCharacterWalkDepthState(character);
                break;
            case 13:
                PlayerCharacterScoreTurnState(character);
                break;
        }
    }
}

void PlayerMotionShiftSet(PlayerCharacterWork *character, s16 motionIndex, float blendStart, float blendDuration, u32 motionAttributes)
{
    Hu3DMotionShiftSet(character->modelIdsBySlot[character->characterType], character->modelMotionsBySlot[character->characterType][character->currentMotionIndex = motionIndex], blendStart, blendDuration, motionAttributes);
    Hu3DMotionShiftSet(character->modelIdsBySlot[character->characterType + 5], character->modelMotionsBySlot[character->characterType][motionIndex], blendStart, blendDuration, motionAttributes);
    Hu3DModelScaleSet(character->modelIdsBySlot[character->characterType + 5], 1.0f, -1.0f, 1.0f);
}

void PlayerMotionSet(PlayerCharacterWork *character, s16 motionIndex, u32 motionAttributes)
{
    Hu3DMotionSet(character->modelIdsBySlot[character->characterType], character->modelMotionsBySlot[character->characterType][character->currentMotionIndex = motionIndex]);
    Hu3DMotionSet(character->modelIdsBySlot[character->characterType + 5], character->modelMotionsBySlot[character->characterType][motionIndex]);
    Hu3DModelScaleSet(character->modelIdsBySlot[character->characterType + 5], 1.0f, -1.0f, 1.0f);
    Hu3DModelAttrSet(character->modelIdsBySlot[character->characterType], motionAttributes);
    Hu3DModelAttrSet(character->modelIdsBySlot[character->characterType + 5], motionAttributes);
}

void PlayerCharacterIdleState(PlayerCharacterWork *character)
{
    s32 previousAnimationState;
    do {
        previousAnimationState = character->animationState;
        PlayerCharacterModelUpdate(character);
        HuPrcVSleep();
    } while (character->animationState == previousAnimationState);
}

static const s32 characterIntroSoundIds[5] = { 0x36, 0x44, 0x47, 0x41, 0x4C };

void PlayerCharacterIntroState(PlayerCharacterWork *character)
{
    Vec screenPosition;
    float transitionFactor;

    s32 frameIndex;
    s16 modelId;
    s16 spriteId;
    s32 previousAnimationState;
    s32 previousAnimationState2;
    s32 previousAnimationState3;
    s32 previousAnimationState4;

    if (character->playerIndex == 0) {
        HuAudFXPlay(0x30D);
    }
    modelId = character->modelIdsBySlot[10];
    Hu3DModelAttrReset(modelId, HU3D_ATTR_DISPOFF);
    Hu3DModelScaleSet(modelId, 1.0f, 1.0f, 1.0f);
    Hu3DModelTPLvlSet(modelId, 1.0f);
    screenPosition = character->worldPosition;
    for (frameIndex = 0; frameIndex < 0x28; frameIndex++) {
        transitionFactor = sind((2.25f * frameIndex));
        screenPosition.y = 170.0f + (350.0f - (350.0f * transitionFactor));
        Hu3DModelPosSet(modelId, screenPosition.x, screenPosition.y, screenPosition.z);
        previousAnimationState = character->animationState;
        PlayerCharacterModelUpdate(character);
        HuPrcVSleep();
        if (character->animationState != previousAnimationState) {
            return;
        }
    }
    if (character->playerIndex == 0) {
        HuAudFXPlay(0x312);
        HuAudFXPlay(0x314);
    }
    for (frameIndex = 0; frameIndex < 0x14; frameIndex++) {
        transitionFactor = sind((4.5f * frameIndex));
        transitionFactor = 1.0f - transitionFactor;
        Hu3DModelScaleSet(modelId, transitionFactor, transitionFactor, transitionFactor);
        Hu3DModelTPLvlSet(modelId, transitionFactor);
        screenPosition.y = 170.0f - (2.0f * frameIndex);
        Hu3DModelPosSet(modelId, screenPosition.x, screenPosition.y, screenPosition.z);
        previousAnimationState2 = character->animationState;
        PlayerCharacterModelUpdate(character);
        HuPrcVSleep();
        if (character->animationState != previousAnimationState2) {
            return;
        }
    }
    Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
    HuAudFXPlay(characterIntroSoundIds[character->characterType]);
    spriteId = character->effectSprites[0];
    screenPosition = character->worldPosition;
    screenPosition.y = 200.0f;
    Hu3D3Dto2D(&screenPosition, 1, &screenPosition);
    espPosSet(spriteId, screenPosition.x, screenPosition.y);
    espTPLvlSet(spriteId, 1.0f);
    espDispOn(spriteId);
    PlayerMotionShiftSet(character, 3, 0.0f, 8.0f, HU3D_ATTR_NONE);
    for (frameIndex = 0; frameIndex < 0x3C; frameIndex++) {
        previousAnimationState3 = character->animationState;
        PlayerCharacterModelUpdate(character);
        HuPrcVSleep();
        if (character->animationState != previousAnimationState3) {
            return;
        }
    }
    PlayerMotionShiftSet(character, 0, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
    for (frameIndex = 0; frameIndex <= 0xA; frameIndex++) {
        espTPLvlSet(spriteId, 1.0f - (0.1f * frameIndex));
        previousAnimationState4 = character->animationState;
        PlayerCharacterModelUpdate(character);
        HuPrcVSleep();
        if (character->animationState != previousAnimationState4) {
            return;
        }
    }
    espDispOff(spriteId);
    playerCharacterWork[character->playerIndex].animationState = 0;
}

void PlayerCharacterSelectionEffectState(PlayerCharacterWork *character)
{
    s32 previousAnimationState;

    selectionEffectWork[character->playerIndex].animationState = 1;
    do {
        previousAnimationState = character->animationState;
        PlayerCharacterModelUpdate(character);
        HuPrcVSleep();
    } while (character->animationState == previousAnimationState);
}

void PlayerCharacterSelectionFlashState(PlayerCharacterWork *character)
{
    Vec screenPosition;

    s32 frameIndex;
    s16 spriteId;
    s32 previousAnimationState;
    s32 previousAnimationState2;

    spriteId = character->effectSprites[0];
    screenPosition = character->worldPosition;
    screenPosition.y = 200.0f;
    Hu3D3Dto2D(&screenPosition, 1, &screenPosition);
    espPosSet(spriteId, screenPosition.x, screenPosition.y);
    espTPLvlSet(spriteId, 1.0f);
    espDispOn(spriteId);
    for (frameIndex = 0; frameIndex < 0x1E; frameIndex++) {
        previousAnimationState = character->animationState;
        PlayerCharacterModelUpdate(character);
        HuPrcVSleep();
        if (character->animationState != previousAnimationState) {
            return;
        }
    }
    for (frameIndex = 0; frameIndex <= 0xA; frameIndex++) {
        espTPLvlSet(spriteId, 1.0f - (0.1f * frameIndex));
        previousAnimationState2 = character->animationState;
        PlayerCharacterModelUpdate(character);
        HuPrcVSleep();
        if (character->animationState != previousAnimationState2) {
            return;
        }
    }
    espDispOff(spriteId);
    playerCharacterWork[character->playerIndex].animationState = 0;
}

void PlayerCharacterWalkLeftState(PlayerCharacterWork *character)
{
    s32 frameIndex;
    s32 previousAnimationState;
    s32 previousAnimationState2;

    if (playerCountThreshold > character->playerIndex) {
        for (frameIndex = 0; frameIndex < 0x1E; frameIndex++) {
            character->movementVelocityX = 0.0001f;
            PlayerCharacterMotionIntegrate(character, 0.1f);
            previousAnimationState = character->animationState;
            PlayerCharacterModelUpdate(character);
            HuPrcVSleep();
            if (character->animationState != previousAnimationState) {
                return;
            }
        }
    }
    else {
        for (frameIndex = 0; frameIndex < 0x1E; frameIndex++) {
            character->movementVelocityX = -0.0001f;
            PlayerCharacterMotionIntegrate(character, 0.1f);
            previousAnimationState2 = character->animationState;
            PlayerCharacterModelUpdate(character);
            HuPrcVSleep();
            if (character->animationState != previousAnimationState2) {
                return;
            }
        }
    }
    PlayerMotionShiftSet(character, 0, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
    playerCharacterWork[character->playerIndex].animationState = 0;
}

static const s32 characterMovementSoundIds[5] = { 0x38, 0x44, 0x47, 0x41, 0x4C };

void PlayerCharacterWalkRightState(PlayerCharacterWork *character)
{
    s32 frameIndex;
    s32 previousAnimationState;
    s32 previousAnimationState2;

    if (playerCountThreshold > character->playerIndex) {
        for (frameIndex = 0; frameIndex < 0x1E; frameIndex++) {
            character->movementVelocityX = 0.0001f;
            PlayerCharacterMotionIntegrate(character, 0.1f);
            previousAnimationState = character->animationState;
            PlayerCharacterModelUpdate(character);
            HuPrcVSleep();
            if (character->animationState != previousAnimationState) {
                return;
            }
        }
    }
    else {
        for (frameIndex = 0; frameIndex < 0x1E; frameIndex++) {
            character->movementVelocityX = -0.0001f;
            PlayerCharacterMotionIntegrate(character, 0.1f);
            previousAnimationState2 = character->animationState;
            PlayerCharacterModelUpdate(character);
            HuPrcVSleep();
            if (character->animationState != previousAnimationState2) {
                return;
            }
        }
    }
    HuAudFXPlay(characterMovementSoundIds[character->characterType]);
    PlayerMotionShiftSet(character, 0, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
    playerCharacterWork[character->playerIndex].animationState = 0;
}

void PlayerCharacterWalkDepthState(PlayerCharacterWork *character)
{
    s32 frameIndex;
    s32 previousAnimationState;
    s32 previousAnimationState2;

    if (playerCountThreshold > character->playerIndex) {
        for (frameIndex = 0; frameIndex < 0x1E; frameIndex++) {
            character->movementVelocityZ = 0.0001f;
            PlayerCharacterMotionIntegrate(character, 0.1f);
            previousAnimationState = character->animationState;
            PlayerCharacterModelUpdate(character);
            HuPrcVSleep();
            if (character->animationState != previousAnimationState) {
                return;
            }
        }
    }
    else {
        for (frameIndex = 0; frameIndex < 0x1E; frameIndex++) {
            character->movementVelocityZ = 0.0001f;
            PlayerCharacterMotionIntegrate(character, 0.1f);
            previousAnimationState2 = character->animationState;
            PlayerCharacterModelUpdate(character);
            HuPrcVSleep();
            if (character->animationState != previousAnimationState2) {
                return;
            }
        }
    }
    PlayerMotionShiftSet(character, 0, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
    playerCharacterWork[character->playerIndex].animationState = 0;
}

void PlayerCharacterScoreEffectState(PlayerCharacterWork *character)
{
    Vec screenPosition;

    s32 frameIndex;
    s16 spriteId;
    s32 previousAnimationState;
    s32 previousAnimationState2;

    spriteId = character->effectSprites[1];
    screenPosition = character->worldPosition;
    screenPosition.y = 200.0f;
    Hu3D3Dto2D(&screenPosition, 1, &screenPosition);
    espPosSet(spriteId, screenPosition.x, screenPosition.y);
    espTPLvlSet(spriteId, 1.0f);
    espDispOn(spriteId);
    for (frameIndex = 0; frameIndex < 0x3C; frameIndex++) {
        previousAnimationState = character->animationState;
        PlayerCharacterModelUpdate(character);
        HuPrcVSleep();
        if (character->animationState != previousAnimationState) {
            return;
        }
    }
    for (frameIndex = 0; frameIndex <= 0xA; frameIndex++) {
        espTPLvlSet(spriteId, 1.0f - (0.1f * frameIndex));
        previousAnimationState2 = character->animationState;
        PlayerCharacterModelUpdate(character);
        HuPrcVSleep();
        if (character->animationState != previousAnimationState2) {
            return;
        }
    }
    espDispOff(spriteId);
    playerCharacterWork[character->playerIndex].animationState = 0;
}

static const s32 characterTurnSoundIds[5] = { 0x38, 0x44, 0x47, 0x41, 0x4C };

void PlayerCharacterFallState(PlayerCharacterWork *character)
{
    s32 frameIndex;
    s32 previousAnimationState;
    s32 previousAnimationState2;
    s32 previousAnimationState3;

    for (frameIndex = 0; frameIndex < 0x1E; frameIndex++) {
        previousAnimationState = character->animationState;
        PlayerCharacterModelUpdate(character);
        HuPrcVSleep();
        if (character->animationState != previousAnimationState) {
            return;
        }
    }
    for (frameIndex = 0; frameIndex < 0xF; frameIndex++) {
        character->movementVelocityZ = 0.0001f;
        PlayerCharacterMotionIntegrate(character, 0.2f);
        previousAnimationState2 = character->animationState;
        PlayerCharacterModelUpdate(character);
        HuPrcVSleep();
        if (character->animationState != previousAnimationState2) {
            return;
        }
    }
    HuAudFXPlay(characterTurnSoundIds[character->characterType]);
    PlayerMotionShiftSet(character, 5, 0.0f, 8.0f, HU3D_ATTR_NONE);
    for (frameIndex = 0; frameIndex < 0x8C; frameIndex++) {
        previousAnimationState3 = character->animationState;
        PlayerCharacterModelUpdate(character);
        HuPrcVSleep();
        if (character->animationState != previousAnimationState3) {
            return;
        }
    }
    playerCharacterWork[character->playerIndex].animationState = 0;
}

static const s32 characterFallSoundIds[5] = { 0x39, 0x43, 0x46, 0x40, 0x4B };

void PlayerCharacterTurnState(PlayerCharacterWork *character)
{
    Vec screenPosition;

    s16 modelId;
    s32 frameIndex;
    s32 previousAnimationState;
    s32 previousAnimationState2;

    modelId = character->modelIdsBySlot[11];
    screenPosition = character->worldPosition;
    screenPosition.y = 80.0f;
    screenPosition.z = -50.0f;
    Hu3DModelPosSet(modelId, screenPosition.x, screenPosition.y, screenPosition.z);
    Hu3DMotionTimeSet(modelId, 0.0f);
    Hu3DModelAttrReset(modelId, HU3D_ATTR_DISPOFF);
    for (frameIndex = 0; frameIndex < 0x1E; frameIndex++) {
        previousAnimationState = character->animationState;
        PlayerCharacterModelUpdate(character);
        HuPrcVSleep();
        if (character->animationState != previousAnimationState) {
            return;
        }
    }
    Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
    HuAudFXPlay(characterFallSoundIds[character->characterType]);
    PlayerMotionShiftSet(character, 4, 0.0f, 8.0f, HU3D_ATTR_NONE);
    while (Hu3DMotionEndCheck(character->modelIdsBySlot[character->characterType]) == 0) {
        previousAnimationState2 = character->animationState;
        PlayerCharacterModelUpdate(character);
        HuPrcVSleep();
        if (character->animationState != previousAnimationState2) {
            return;
        }
    }

    if (character->characterType == 4) {
        PlayerMotionSet(character, 0, HU3D_MOTATTR_LOOP);
    }
    else {
        PlayerMotionShiftSet(character, 0, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
    }
    playerCharacterWork[character->playerIndex].animationState = 0;
}

void PlayerCharacterExitLeftState(PlayerCharacterWork *character)
{
    s32 frameIndex;
    s32 previousAnimationState;
    s32 previousAnimationState2;

    for (frameIndex = 0; frameIndex < (character->playerIndex * 8); frameIndex++) {
        previousAnimationState = character->animationState;
        PlayerCharacterModelUpdate(character);
        HuPrcVSleep();
        if (character->animationState != previousAnimationState) {
            return;
        }
    }
    while (TRUE) {
        character->movementVelocityX = -8.0f;
        PlayerCharacterMotionIntegrate(character, 0.2f);
        previousAnimationState2 = character->animationState;
        PlayerCharacterModelUpdate(character);
        HuPrcVSleep();
        if (character->animationState == previousAnimationState2) {
            if (character->worldPosition.x < -600.0f) {
                playerCharacterWork[character->playerIndex].animationState = 0;
                break;
            }
        }
        else {
            break;
        }
    }
}

void PlayerCharacterEnterRightState(PlayerCharacterWork *character)
{
    float interpolationValue;
    s32 frameIndex;
    s32 previousAnimationState;
    s32 previousAnimationState2;
    s32 previousAnimationState3;

    character->worldPosition.x = 600.0f;
    character->worldPosition.z = 0.0f;
    character->worldPosition.y = 0.0f;
    interpolationValue = -260.0f + (130.0f * character->playerIndex);
    for (frameIndex = 0; frameIndex < (character->playerIndex * 0x1E); frameIndex++) {
        previousAnimationState = character->animationState;
        PlayerCharacterModelUpdate(character);
        HuPrcVSleep();
        if (character->animationState != previousAnimationState) {
            return;
        }
    }
loop1:
    character->movementVelocityX = -8.0f;
    PlayerCharacterMotionIntegrate(character, 0.2f);
    if (character->worldPosition.x < interpolationValue) {
        character->worldPosition.x = interpolationValue;
    }
    else {
        previousAnimationState2 = character->animationState;
        PlayerCharacterModelUpdate(character);
        HuPrcVSleep();
        if (character->animationState != previousAnimationState2) {
            return;
        }
        goto loop1;
    }
    PlayerMotionShiftSet(character, 1, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
    for (frameIndex = 0; frameIndex < 0xF; frameIndex++) {
        character->movementVelocityZ = 0.0001f;
        PlayerCharacterMotionIntegrate(character, 0.2f);
        previousAnimationState3 = character->animationState;
        PlayerCharacterModelUpdate(character);
        HuPrcVSleep();
        if (character->animationState != previousAnimationState3) {
            return;
        }
    }
    character->modelRotationY = 0.0f;
    PlayerMotionShiftSet(character, 0, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
    playerCharacterWork[character->playerIndex].animationState = 0;
}

void PlayerCharacterMotionIntegrate(PlayerCharacterWork *character, float turnRate)
{
    float speedMagnitude;
    float interpolationValue;
    float interpolationValue2;
    float interpolationValue3;
    s16 motionType;

    motionType = 0;
    character->movementSpeed = sqrtf((character->movementVelocityX * character->movementVelocityX) + (character->movementVelocityZ * character->movementVelocityZ));
    if (0.0001f <= character->movementSpeed) {
        if (10.0f <= character->movementSpeed) {
            character->movementVelocityX /= character->movementSpeed;
            character->movementVelocityZ /= character->movementSpeed;
            character->movementSpeed = 10.0f;
            character->movementVelocityX = 10.0f * character->movementVelocityX;
            character->movementVelocityZ = 10.0f * character->movementVelocityZ;
        }
        character->modelRotationY = AngleInterpolate(character->modelRotationY, atan2d(character->movementVelocityX, character->movementVelocityZ), turnRate);
        if (8.0f <= character->movementSpeed) {
            motionType = 2;
        }
        else {
            motionType = 1;
        }
    }
    character->worldPosition.x += character->movementVelocityX;
    character->worldPosition.z += character->movementVelocityZ;
    if (character->currentMotionIndex != motionType) {
        PlayerMotionShiftSet(character, motionType, 0.0f, 4.0f, HU3D_MOTATTR_LOOP);
    }
    character->movementVelocityX = 0.0f;
    character->movementVelocityZ = 0.0f;
}

void PlayerCharacterScoreTurnState(PlayerCharacterWork *character)
{
    s32 frameIndex;
    s32 previousAnimationState;
    s32 previousAnimationState2;
    s32 previousAnimationState3;

    if (playerCountThreshold > character->playerIndex) {
        for (frameIndex = 0; frameIndex < 0x1E; frameIndex++) {
            character->movementVelocityZ = 0.0001f;
            PlayerCharacterMotionIntegrate(character, 0.4f);
            previousAnimationState = character->animationState;
            PlayerCharacterModelUpdate(character);
            HuPrcVSleep();
            if (character->animationState != previousAnimationState) {
                return;
            }
        }
    }
    else {
        for (frameIndex = 0; frameIndex < 0x1E; frameIndex++) {
            character->movementVelocityZ = 0.0001f;
            PlayerCharacterMotionIntegrate(character, 0.4f);
            previousAnimationState2 = character->animationState;
            PlayerCharacterModelUpdate(character);
            HuPrcVSleep();
            if (character->animationState != previousAnimationState2) {
                return;
            }
        }
    }
    PlayerMotionShiftSet(character, 4, 0.0f, 8.0f, HU3D_ATTR_NONE);
    while (Hu3DMotionEndCheck(character->modelIdsBySlot[character->characterType]) == 0) {
        previousAnimationState3 = character->animationState;
        PlayerCharacterModelUpdate(character);
        HuPrcVSleep();
        if (character->animationState != previousAnimationState3) {
            return;
        }
    }
    if (character->characterType == 4) {
        PlayerMotionSet(character, 0, HU3D_MOTATTR_LOOP);
    }
    else {
        PlayerMotionShiftSet(character, 0, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
    }
    playerCharacterWork[character->playerIndex].animationState = 0;
}

void PlayerCharacterModelUpdate(PlayerCharacterWork *character)
{
    Hu3DModelPosSet(character->modelIdsBySlot[character->characterType], character->worldPosition.x, character->worldPosition.y, character->worldPosition.z);
    Hu3DModelPosSet(character->modelIdsBySlot[character->characterType + 5], character->worldPosition.x, character->worldPosition.y, character->worldPosition.z);
    Hu3DModelRotSet(character->modelIdsBySlot[character->characterType], character->modelRotationX, character->modelRotationY, character->modelRotationZ);
    Hu3DModelRotSet(character->modelIdsBySlot[character->characterType + 5], character->modelRotationX, character->modelRotationY, character->modelRotationZ);
}

float AngleInterpolate(float startAngle, float targetAngle, float interpolationRate)
{
    float angleDelta;
    float normalizedAngle;

    angleDelta = fmod(targetAngle - startAngle, 360.0);
    if (0.0f > angleDelta) {
        angleDelta += 360.0f;
    }
    if (180.0f < angleDelta) {
        angleDelta -= 360.0f;
    }
    normalizedAngle = fmod(startAngle + (angleDelta * interpolationRate), 360.0);
    if (0.0f > normalizedAngle) {
        normalizedAngle += 360.0f;
    }
    return normalizedAngle;
}

void PlayerCharacterModelSet(s32 playerIndex, u32 modelType)
{
    PlayerCharacterWork *character;
    s32 modelSlot;

    character = &playerCharacterWork[playerIndex];
    character->characterType = modelType;
    for (modelSlot = 0; modelSlot < 5; modelSlot++) {
        if (modelSlot != character->characterType) {
            Hu3DModelAttrSet(character->modelIdsBySlot[modelSlot], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(character->modelIdsBySlot[modelSlot + 5], HU3D_ATTR_DISPOFF);
        }
        Hu3DModelPosSet(character->modelIdsBySlot[modelSlot], 0.0f, 0.0f, -1000.0f);
        Hu3DModelPosSet(character->modelIdsBySlot[modelSlot + 5], 0.0f, 0.0f, -1000.0f);
    }
    Hu3DModelAttrReset(character->modelIdsBySlot[character->characterType], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(character->modelIdsBySlot[character->characterType + 5], HU3D_ATTR_DISPOFF);
    PlayerMotionShiftSet(character, 0, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
}

void ResultsSequenceProcess(void)
{
    s32 playerIndex;

    HuPrcSleep(0x3C);
    for (playerIndex = 0; playerIndex < 5; playerIndex++) {
        playerCharacterWork[playerIndex].animationState = 1;
    }
    while (playerCharacterWork->animationState != 0) {
        HuPrcVSleep();
    }
    resultSequenceHandle = HuAudSeqPlay(0x43);
    SequenceDisplayStart(0);
    while (SequenceDisplayIsActive() != 0) {
        HuPrcVSleep();
    }
    childProcesses[0x12] = NULL;
    HuPrcEnd();
}

void ResultsSummaryProcess(void)
{
    s32 playerIndex;
    s32 finalScore;
    s16 sequenceHandle;
    s32 previousRecordScore;

    HuAudSeqFadeOut(resultSequenceHandle, 0x64);
#if VERSION_PAL
    HuAudFXPlay(0x815);
#else
    HuAudFXPlay(0x814);
#endif
    previousRecordScore = RecordScoreGet();
    if (ScoreGet() > previousRecordScore) {
        SequenceDisplayStart(1);
        while (SequenceDisplayIsActive() != 0) {
            HuPrcVSleep();
        }
        finalScore = ScoreGet();
        GWMGRecordSet(0xB, finalScore);
        sequenceHandle = MGSeqCreate(0xE, finalScore);
        HuAudFXPlay(0x26);
        RecordScoreUpdate();
        while (MGSeqStatGet(sequenceHandle) != 0) {
            HuPrcVSleep();
        }
    }
    else {
        SequenceDisplayStart(1);
        while (SequenceDisplayIsActive() != 0) {
            HuPrcVSleep();
        }
    }
    HuAudSStreamPlay(3);
    if (ScoreGet() >= 0x63) {
        for (playerIndex = 0; playerIndex < 5; playerIndex++) {
            playerCharacterWork[playerIndex].animationState = 0xD;
        }
        while (playerCharacterWork[playerIndex].animationState == 0xD) {
            HuPrcVSleep();
        }
    }
    HuPrcSleep(0xD2);
    childProcesses[0x12] = NULL;
    HuPrcEnd();
}

void PlayerCharacterResetProcess(void)
{
    s32 playerIndex;

    for (playerIndex = 0; playerIndex < 5; playerIndex++) {
        playerCharacterWork[playerIndex].animationState = 1;
    }
    while (playerCharacterWork->animationState != 0) {
        HuPrcVSleep();
    }
    childProcesses[0x12] = NULL;
    HuPrcEnd();
}

void SelectionConfirmProcess(void)
{
    HuPrcSleep(0x1E);
    HuAudFXPlay(0x30D);
    characterSelectionWork.selectionState = 3;
    while (characterSelectionWork.selectionState != 0) {
        HuPrcVSleep();
    }
    childProcesses[0x12] = NULL;
    HuPrcEnd();
}

void SelectedCharacterReactProcess(void)
{
    HuAudFXPlay(9);
    playerCharacterWork[selectedPlayerIndex].animationState = 9;
    while (playerCharacterWork[selectedPlayerIndex].animationState == 9) {
        HuPrcVSleep();
    }
    HuPrcSleep(8);
    childProcesses[0x12] = NULL;
    HuPrcEnd();
}

void PlayerCharacterRaceProcess(void)
{
    s32 playerIndex;

    playerCharacterWork[selectedPlayerIndex].animationState = 7;
    HuPrcSleep(0x1E);
    playerCharacterWork[challengerPlayerIndex].animationState = 8;
    HuPrcSleep(0x50);
    playerCountThreshold = challengerPlayerIndex;
    for (playerIndex = 0; playerIndex < 5; playerIndex++) {
        if ((playerIndex != challengerPlayerIndex) && (playerIndex != selectedPlayerIndex)) {
            playerCharacterWork[playerIndex].animationState = 6;
        }
    }
    while (playerCharacterWork[selectedPlayerIndex].animationState == 7) {
        HuPrcVSleep();
    }
    HuPrcSleep(0x1E);
    playerCharacterWork[selectedPlayerIndex].animationState = 5;
    while (playerCharacterWork[selectedPlayerIndex].animationState == 5) {
        HuPrcVSleep();
    }
    HuPrcSleep(0x3C);
    childProcesses[0x12] = NULL;
    HuPrcEnd();
}

void WinnerCharacterProcess(void)
{
    s32 playerIndex;

    playerCharacterWork[challengerPlayerIndex].animationState = 8;
    HuPrcSleep(0x6E);
    playerCountThreshold = challengerPlayerIndex;
    for (playerIndex = 0; playerIndex < 5; playerIndex++) {
        if (playerIndex != challengerPlayerIndex) {
            playerCharacterWork[playerIndex].animationState = 5;
        }
    }
    while (playerCharacterWork[challengerPlayerIndex].animationState != 8) {
        HuPrcVSleep();
    }
    HuPrcSleep(0x3C);
    childProcesses[0x12] = NULL;
    HuPrcEnd();
}

void AllCharactersCelebrateProcess(void)
{
    s32 playerIndex;
    s32 activeCharacterCount;

    for (playerIndex = 0; playerIndex < 5; playerIndex++) {
        playerCharacterWork[playerIndex].animationState = 0xB;
    }
    while (TRUE) {
        activeCharacterCount = 0;
        for (playerIndex = 0; playerIndex < 5; playerIndex++) {
            if (playerCharacterWork[playerIndex].animationState != 0) {
                activeCharacterCount++;
            }
        }
        if (activeCharacterCount != 0) {
            HuPrcVSleep();
        }
        else {
            break;
        }
    }
    childProcesses[0x12] = NULL;
    HuPrcEnd();
}

void AllCharactersReactProcess(void)
{
    s32 playerIndex;
    s32 activeCharacterCount;

    for (playerIndex = 0; playerIndex < 5; playerIndex++) {
        playerCharacterWork[playerIndex].animationState = 0xA;
    }
    while (TRUE) {
        activeCharacterCount = 0;
        for (playerIndex = 0; playerIndex < 5; playerIndex++) {
            if (playerCharacterWork[playerIndex].animationState != 0) {
                activeCharacterCount++;
            }
        }
        if (activeCharacterCount != 0) {
            HuPrcVSleep();
        }
        else {
            break;
        }
    }
    childProcesses[0x12] = NULL;
    HuPrcEnd();
}

void TimerDisplayProcess(void)
{
    TimerDisplayWork *timerDisplay;

    timerDisplay = &timerDisplayWork;
    timerDisplay->menuState = 0;
    timerDisplay->stateTimer = 0;
    timerDisplay->menuSelection = -1;
    while (TRUE) {
        switch (timerDisplay->menuState) {
            case 4:
            case 0:
                break;
            case 1:
                if (timerDisplay->menuSelection == -1) {
                    timerDisplay->menuSelection = MGSeqCreate(1, timerDisplay->stateTimer / REFRESH_RATE, -1, -1);
                }
                timerDisplay->menuState = 2;
                break;
            case 3:
                if (timerDisplay->menuSelection != -1) {
                    MGSeqParamSet(timerDisplay->menuSelection, 2, -1);
                    timerDisplay->menuSelection = -1;
                }
                timerDisplay->menuState = 0;
                break;
            case 2:
                if ((timerDisplay->menuSelection != -1) && (timerDisplay->stateTimer > 0)) {
                    if (--timerDisplay->stateTimer <= 0) {
                        timerDisplay->stateTimer = 0;
                        timerDisplay->menuState = 3;
                    }
                    if ((timerDisplay->stateTimer % REFRESH_RATE) == 0) {
                        MGSeqParamSet(timerDisplay->menuSelection, 1, timerDisplay->stateTimer / REFRESH_RATE);
                    }
                }
                break;
        }
        HuPrcVSleep();
    }
}

s32 TimerFramesGet(void)
{
    return timerDisplayWork.stateTimer;
}

void TimerStart(s32 frameCount)
{
    timerDisplayWork.stateTimer = frameCount;
    timerDisplayWork.menuState = 1;
}

void TimerStop(void)
{
    timerDisplayWork.menuState = 4;
}

void TimerCancel(void)
{
    timerDisplayWork.menuState = 3;
}

void SequenceDisplayProcess(void)
{
    SequenceDisplayWork *sequenceDisplay;

    sequenceDisplay = &sequenceDisplayWork;
    sequenceDisplay->sequenceState = 0;
    sequenceDisplay->sequenceHandle = -1;
    while (TRUE) {
        switch (sequenceDisplay->sequenceState) {
            case 0:
                break;
            case 2:
                if (sequenceDisplay->sequenceHandle == -1) {
                    switch (sequenceDisplay->sequenceMode) {
                        case 0:
                            sequenceDisplay->sequenceHandle = MGSeqCreate(3, 0);
                            break;
                        case 1:
                            sequenceDisplay->sequenceHandle = MGSeqCreate(3, 1);
                            break;
                    }
                }
                sequenceDisplay->sequenceState = 1;
                break;
            case 1:
                if (MGSeqStatGet(sequenceDisplay->sequenceHandle) == 0) {
                    MGSeqKill(sequenceDisplay->sequenceHandle);
                    sequenceDisplay->sequenceHandle = -1;
                    sequenceDisplay->sequenceState = 0;
                }
                break;
        }
        HuPrcVSleep();
    }
}

void SequenceDisplayStart(s32 sequenceMode)
{
    sequenceDisplayWork.sequenceState = 2;
    sequenceDisplayWork.sequenceMode = sequenceMode;
}

s32 SequenceDisplayIsActive(void)
{
    return sequenceDisplayWork.sequenceState != 0;
}

void ScoreDisplayProcess(void)
{
    float interpolationValue;
    ScoreDisplayWork *scoreDisplay;
    s32 spriteIndex;
    s32 digitValue;
    s32 scaleIncreasing;

    scoreDisplay = &scoreDisplayWork;
    scoreDisplay->currentScore = 0;
    scoreDisplay->recordScore = (s16)GWMGRecordGet(0xB);
    scoreDisplay->recordBeaten = 0;
    espPosSet(scoreDisplay->scoreDigits[5], 508.0f, 64.0f);
    espPosSet(scoreDisplay->scoreDigits[4], 488.0f, 56.0f);
    for (spriteIndex = 0; spriteIndex < 2; spriteIndex++) {
        espPosSet(scoreDisplay->scoreDigits[spriteIndex + 2], (spriteIndex * 0x10) + 0x1FC, 57.0f);
        espPosSet(scoreDisplay->scoreDigits[spriteIndex], (spriteIndex * 0x10) + 0x1FC, 75.0f);
    }
    espTPLvlSet(scoreDisplay->scoreDigits[5], 0.5f);
    espPriSet(scoreDisplay->scoreDigits[5], 0x65);
    espPriSet(scoreDisplay->scoreDigits[4], 0x64);
    espPriSet(scoreDisplay->scoreDigits[2], 0x64);
    espPriSet(scoreDisplay->scoreDigits[3], 0x64);
    espPriSet(scoreDisplay->scoreDigits[0], 0x64);
    espPriSet(scoreDisplay->scoreDigits[1], 0x64);
    interpolationValue = 1.0f;
    while (TRUE) {
        if (scoreDisplay->recordBeaten != 0) {
            if (scaleIncreasing != 0) {
                interpolationValue += 0.02;
                interpolationValue = interpolationValue;
                if (interpolationValue >= 1.2f) {
                    interpolationValue = 1.2f;
                    scaleIncreasing = 0;
                }
            }
            else {
                interpolationValue -= 0.02;
                interpolationValue = interpolationValue;
                if (interpolationValue <= 1.0f) {
                    interpolationValue = 1.0f;
                    scaleIncreasing = 1;
                }
            }
            espScaleSet(scoreDisplay->scoreDigits[4], interpolationValue, interpolationValue);
            for (spriteIndex = 0; spriteIndex < 2; spriteIndex++) {
                espScaleSet(scoreDisplay->scoreDigits[spriteIndex + 2], interpolationValue, interpolationValue);
            }
        }
        digitValue = scoreDisplay->currentScore;
        espBankSet(scoreDisplay->scoreDigits[0], digitValue / 10);
        digitValue = digitValue % 10;
        espBankSet(scoreDisplay->scoreDigits[1], digitValue);
        digitValue = scoreDisplay->recordScore;
        espBankSet(scoreDisplay->scoreDigits[2], digitValue / 10);
        digitValue = digitValue % 10;
        espBankSet(scoreDisplay->scoreDigits[3], digitValue);
        HuPrcVSleep();
    }
}

void ScoreAdd(s32 scoreDelta)
{
    scoreDisplayWork.currentScore += scoreDelta;
}

s32 ScoreGet(void)
{
    return scoreDisplayWork.currentScore;
}

s32 RecordScoreGet(void)
{
    return scoreDisplayWork.recordScore;
}

s32 RecordScoreUpdate(void)
{
    if (scoreDisplayWork.recordScore < scoreDisplayWork.currentScore) {
        scoreDisplayWork.recordScore = scoreDisplayWork.currentScore;
        scoreDisplayWork.recordBeaten = 1;
    }
}

DirectionalLightSetup directionalLightSetup = { { 0.0f, 300.0f, 0.0f }, { 0.0f, -300.0f, -600.0f }, { 0xFF, 0xFF, 0xFF, 0xFF } };

void DirectionalLightProcess(void)
{
    HU3DLIGHT *lightProperties;
    s16 *lightHandlesForPlayer;

    lightHandlesForPlayer = lightHandles;
    Hu3DLighInit();
    lightHandlesForPlayer[1] = Hu3DGLightCreateV(&directionalLightSetup.lightPosition, &directionalLightSetup.lightDirection, &directionalLightSetup.lightColor);
    Hu3DGLightInfinitytSet(lightHandlesForPlayer[1]);
    lightProperties = &Hu3DGlobalLight[lightHandlesForPlayer[1]];
    {
        Vec shadowTargetPosition = { 0.0f, 3000.0f, 1.0f };
        Vec shadowUpDirection = { 0.0f, 1.0f, 0.0f };
        Vec shadowTargetOrigin = { 0.0f, 0.0f, 0.0f };
        Hu3DShadowCreate(30.0f, 20.0f, 5000.0f);
        Hu3DShadowTPLvlSet(0.45f);
        Hu3DShadowPosSet(&shadowTargetPosition, &shadowUpDirection, &shadowTargetOrigin);
    }
    Hu3DShadowTPLvlSet(0.3f);
    Hu3DModelAttrSet(lightHandlesForPlayer[0], HU3D_MOTATTR_LOOP);
    while (TRUE) {
        HuPrcVSleep();
    }
}

static const s32 selectionEffectTiming[6][3];

void CharacterSelectionProcess(void)
{
    CharacterSelectionWork *selection;

    selection = &characterSelectionWork;
    selection->selectionState = 0;
    selection->playerSettings[0] = GWPlayerCfg->pad_idx;
    selection->playerSettings[1] = GWPlayerCfg->character;
    selection->isComputerPlayer = GWPlayerCfg->iscom;
    selection->inputDelay = 0x14;
    selectedPlayerIndex = 0;
    rumbleController = selection->playerSettings[0];
    while (TRUE) {
        switch (selection->selectionState) {
            case 0:
                CharacterSelectionWait(selection);
                continue;
            case 1:
                CharacterSelectionInputProcess(selection);
                continue;
            case 2:
                break;
            case 3:
                CharacterSelectionConfirmProcess(selection);
                continue;
            default:
                continue;
        }
        CharacterSelectionFinish(selection);
    }
}

void CharacterSelectionWait(CharacterSelectionWork *selection)
{
    s32 previousSelectionState;

    do {
        previousSelectionState = selection->selectionState;
        HuPrcVSleep();
    } while (selection->selectionState == previousSelectionState);
}

void CharacterSelectionInputProcess(CharacterSelectionWork *selection)
{
    Vec characterPosition;
    Vec cameraOffset;
    Vec screenPosition;
    float transitionFactor;

    u8 directionInput;
    s16 selectionSprite;
    s32 previousPlayerIndex;
    u16 buttonInput;
    s32 frameIndex;
    PlayerCharacterWork *character;
    s32 randomChoice;
    s32 computerChoiceMade;
    s32 previousSelectionState;
    s32 previousSelectionState2;
    s32 remainingTimerFrames;

#ifdef NON_MATCHING
    characterPosition.x = 0.0f;
    characterPosition.y = 0.0f;
    characterPosition.z = 0.0f;
#endif

    selectedPlayerIndex = 0;
    previousPlayerIndex = -1;
    selectionSprite = selection->selectionSprites[selection->playerSettings[1]];
    espDispOn(selectionSprite);
    computerChoiceMade = 0;
    while(1) {
        directionInput = 0;
        buttonInput = 0;
        if (selection->isComputerPlayer != 0) {
            if (selection->inputDelay <= 0) {
                remainingTimerFrames = timerDisplayWork.stateTimer;
                if ((remainingTimerFrames > REFRESH_RATE * 3) && (computerChoiceMade == 0)) {
                    randomChoice = frandmod(0xA);
                    if (randomChoice >= 5) {
                        directionInput |= 1;
                    }
                    else if (randomChoice > 1) {
                        directionInput |= 2;
                    }
                    else {
                        computerChoiceMade = 1;
                    }
                }
                else {
                    if (selectedPlayerIndex > challengerPlayerIndex) {
                        directionInput |= 1;
                    }
                    else if (selectedPlayerIndex < challengerPlayerIndex) {
                        directionInput |= 2;
                    }
                    if (selectedPlayerIndex == challengerPlayerIndex) {
                        buttonInput |= 0x100;
                    }
                }
                selection->inputDelay = 0x1E;
            }
            else {
                selection->inputDelay--;
            }
        }
        else {
            directionInput = HuPadDStkRep[selection->playerSettings[0]];
            buttonInput = HuPadBtnDown[selection->playerSettings[0]];
        }
        if (previousPlayerIndex != -1) {
            if ((buttonInput & 0x100) != 0) {
                HuAudFXPlay(2);
                break;
            }
            else {
                if ((directionInput & 1) != 0) {
                    if (--selectedPlayerIndex < 0) {
                        selectedPlayerIndex = 4;
                    }
                    HuAudFXPlay(0);
                }
                else if ((directionInput & 2) != 0) {
                    if (++selectedPlayerIndex >= 5) {
                        selectedPlayerIndex = 0;
                    }
                    HuAudFXPlay(0);
                }
                goto block30;
            }
        }
        else {
        block30:
            if (previousPlayerIndex != selectedPlayerIndex) {
                character = &playerCharacterWork[selectedPlayerIndex];
                cameraOffset = characterPosition;
                characterPosition = character->worldPosition;
                characterPosition.y = 40.0f;
                espPriSet(selectionSprite, 0x32);
                if (previousPlayerIndex == -1) {
                    cameraOffset = characterPosition;
                }
                else {
                    selectionEffectWork[previousPlayerIndex].animationState = 3;
                }
                for (frameIndex = 0; frameIndex <= 4; frameIndex++) {
                    transitionFactor = sind((90.0f * (0.25f * frameIndex)));
                    transitionFactor *= transitionFactor * transitionFactor;
                    screenPosition.x = cameraOffset.x + (transitionFactor * (characterPosition.x - cameraOffset.x));
                    screenPosition.y = cameraOffset.y + (transitionFactor * (characterPosition.y - cameraOffset.y));
                    screenPosition.z = cameraOffset.z + (transitionFactor * (characterPosition.z - cameraOffset.z));
                    Hu3D3Dto2D(&screenPosition, 1, &screenPosition);
                    espPosSet(selectionSprite, screenPosition.x, screenPosition.y);
                    previousSelectionState = selection->selectionState;
                    HuPrcVSleep();
                    if (selection->selectionState != previousSelectionState) {
                        return;
                    }
                }
                espPosSet(selectionSprite, screenPosition.x, screenPosition.y);
                selectionEffectWork[selectedPlayerIndex].animationState = 1;
            }
            previousPlayerIndex = selectedPlayerIndex;
            previousSelectionState2 = selection->selectionState;
            HuPrcVSleep();
            if (selection->selectionState != previousSelectionState2) {
                return;
            }
        }
    }
    selectionEffectWork[selectedPlayerIndex].animationState = 3;
    espDispOff(selectionSprite);
    characterSelectionWork.selectionState = 0;
}

void CharacterSelectionConfirmProcess(CharacterSelectionWork *selection)
{
    Vec screenPosition;
    float transitionFactor;

    s32 frameIndex;
    s16 characterModelId;
    PlayerCharacterWork *character;
    s32 previousSelectionState;
    s32 previousSelectionState2;
    s32 previousSelectionState3;
    s32 previousSelectionState4;

    character = &playerCharacterWork[selectedPlayerIndex];
    playerCharacterWork[selectedPlayerIndex].animationState = 4;
    for (frameIndex = 0; frameIndex < 0x3C; frameIndex++) {
        previousSelectionState = selection->selectionState;
        HuPrcVSleep();
        if (selection->selectionState != previousSelectionState) {
            return;
        }
    }
    characterModelId = selection->characterModels[selection->playerSettings[1]];
    Hu3DModelAttrReset(characterModelId, HU3D_ATTR_DISPOFF);
    Hu3DModelTPLvlSet(characterModelId, 1.0f);
    Hu3DModelScaleSet(characterModelId, 1.0f, 1.0f, 1.0f);
    screenPosition = character->worldPosition;
    for (frameIndex = 0; frameIndex < 0x3C; frameIndex++) {
        transitionFactor = sind((1.5000001f * frameIndex));
        screenPosition.y = 170.0f + (350.0f - (350.0f * transitionFactor));
        Hu3DModelPosSet(characterModelId, screenPosition.x, screenPosition.y, screenPosition.z);
        previousSelectionState2 = selection->selectionState;
        HuPrcVSleep();
        if (selection->selectionState != previousSelectionState2) {
            return;
        }
    }
    for (frameIndex = 0; frameIndex < 0x14; frameIndex++) {
        transitionFactor = sind((4.5f * frameIndex));
        transitionFactor = 1.0f - transitionFactor;
        Hu3DModelScaleSet(characterModelId, transitionFactor, transitionFactor, transitionFactor);
        Hu3DModelTPLvlSet(characterModelId, transitionFactor);
        Hu3DModelRotSet(characterModelId, 0.0f, 540.0f * transitionFactor, 0.0f);
        screenPosition.y = 170.0f - (2.0f * frameIndex);
        Hu3DModelPosSet(characterModelId, screenPosition.x, screenPosition.y, screenPosition.z);
        previousSelectionState3 = selection->selectionState;
        HuPrcVSleep();
        if (selection->selectionState != previousSelectionState3) {
            return;
        }
    }
    Hu3DModelAttrSet(characterModelId, HU3D_ATTR_DISPOFF);
    playerCountThreshold = selectedPlayerIndex;
    for (frameIndex = 0; frameIndex < 5; frameIndex++) {
        if (frameIndex != selectedPlayerIndex) {
            playerCharacterWork[frameIndex].animationState = 5;
        }
    }
    for (frameIndex = 0; frameIndex < 0x78; frameIndex++) {
        previousSelectionState4 = selection->selectionState;
        HuPrcVSleep();
        if (selection->selectionState != previousSelectionState4) {
            return;
        }
    }
    characterSelectionWork.selectionState = 0;
}

void CharacterSelectionFinish(CharacterSelectionWork *selection)
{
    selectionEffectWork[selectedPlayerIndex].animationState = 3;
    espDispOff(selection->selectionSprites[selection->playerSettings[1]]);
    characterSelectionWork.selectionState = 0;
}

void SelectionEffectProcess(void)
{
    SelectionEffectWork *effect;
    s32 playerIndex;

    playerIndex = (s32)HuPrcCurrentGet()->user_data;
    effect = &selectionEffectWork[playerIndex];
    effect->playerIndex = playerIndex;
    effect->animationState = 0;
    while (TRUE) {
        switch (effect->animationState) {
            case 0:
                SelectionEffectWait(effect);
                break;
            case 1:
                SelectionEffectAppearProcess(effect);
                break;
            case 2:
                SelectionEffectPulseProcess(effect);
                break;
            case 3:
                SelectionEffectDisappearProcess(effect);
                break;
        }
        
    }
}

void SelectionEffectWait(SelectionEffectWork *effect)
{
    s32 previousAnimationState;

    do {
        previousAnimationState = effect->animationState;
        HuPrcVSleep();
    } while (effect->animationState == previousAnimationState);
}

void SelectionEffectAppearProcess(SelectionEffectWork *effect)
{
    Vec screenPosition;
    float interpolationValue;

    PlayerCharacterWork *character;
    s16 spriteId;
    s16 spriteId2;
    s32 frameIndex;
    s32 previousAnimationState;

    spriteId2 = effect->effectSprites[1];
    spriteId = effect->effectSprites[0];
    character = &playerCharacterWork[effect->playerIndex];
    screenPosition = character->worldPosition;
    screenPosition.y = 200.0f;
    Hu3D3Dto2D(&screenPosition, 1, &screenPosition);
    espPosSet(spriteId2, screenPosition.x, screenPosition.y);
    espPosSet(spriteId, screenPosition.x, screenPosition.y);
    espScaleSet(spriteId, 0.0f, 0.0f);
    espPriSet(spriteId2, 0x32);
    espPriSet(spriteId, 0x31);
    if (rumbleEnabled == 0) {
        espScaleSet(spriteId2, 1.0f, 1.0f);
        espScaleSet(spriteId, 0.5f, 0.5f);
        espDispOn(spriteId2);
        espDispOn(spriteId);
    }
    interpolationValue = 0.0f;
    for (frameIndex = 0; frameIndex < 0xA; frameIndex++) {
        espTPLvlSet(spriteId2, interpolationValue);
        espTPLvlSet(spriteId, interpolationValue);
        interpolationValue = 0.08f * frameIndex;
        previousAnimationState = effect->animationState;
        HuPrcVSleep();
        if (effect->animationState != previousAnimationState) {
            return;
        }
    }
    espTPLvlSet(spriteId2, 0.8f);
    espTPLvlSet(spriteId, 0.8f);
    selectionEffectWork[effect->playerIndex].animationState = 2;
}

static const s32 selectionEffectTiming[6][3] = {
    { 0, 0xF, 0xF },
    { 1, 0xA, 0xA },
    { 1, 0x14, 5 },
    { 2, 0x14, 0xA },
    { 2, 5, 0xA },
    { 0x3F000000, 0x3F000000, 0x3F000000 },
};

void SelectionEffectPulseProcess(SelectionEffectWork *effect)
{
    float interpolationValue;
    float interpolationValue2;
    s32 frameIndex;
    s32 pulseDurationFrames;
    s16 spriteId;
    s32 vibrationPattern;
    s32 fadeDurationFrames;
    s32 previousAnimationState;
    s32 previousAnimationState2;

    spriteId = effect->effectSprites[0];
    vibrationPattern = selectionEffectTiming[effect->effectType][0];
    pulseDurationFrames = selectionEffectTiming[effect->effectType][1];
    fadeDurationFrames = selectionEffectTiming[effect->effectType][2];
    while (TRUE) {
        if (rumbleEnabled != 0) {
            switch (vibrationPattern) {
                case 0:
                    omVibrate(0, pulseDurationFrames, 6, 6);
                    break;
                case 1:
                    omVibrate(0, pulseDurationFrames, 4, 2);
                    break;
                case 2:
                    omVibrate(0, pulseDurationFrames, 0xC, 0);
                    break;
            }
        }
        switch (vibrationPattern) {
            case 0:
                interpolationValue2 = 0.4f;
                break;
            case 1:
                interpolationValue2 = 0.55f;
                break;
            case 2:
                interpolationValue2 = 0.7f;
                break;
        }
        for (frameIndex = 0; frameIndex < pulseDurationFrames; frameIndex++) {
            interpolationValue = frameIndex * (interpolationValue2 * (1.0f / pulseDurationFrames));
            interpolationValue += 0.5f;
            espScaleSet(spriteId, interpolationValue, interpolationValue);
            previousAnimationState = effect->animationState;
            HuPrcVSleep();
            if (effect->animationState != previousAnimationState) {
                return;
            }
        }

        for (frameIndex = 0; frameIndex < fadeDurationFrames; frameIndex++) {
            interpolationValue = interpolationValue2 - (frameIndex * (interpolationValue2 * (1.0f / fadeDurationFrames)));
            interpolationValue += 0.5f;
            espScaleSet(spriteId, interpolationValue, interpolationValue);
            previousAnimationState2 = effect->animationState;
            HuPrcVSleep();
            if (effect->animationState != previousAnimationState2) {
                return;
            }
        }
    }
}

void SelectionEffectDisappearProcess(SelectionEffectWork *effect)
{
    float interpolationValue;
    s32 frameIndex;
    s16 spriteId;
    s16 spriteId2;
    s32 previousAnimationState;

    spriteId = effect->effectSprites[1];
    spriteId2 = effect->effectSprites[0];
    if (rumbleEnabled != 0) {
        HuPadRumbleStop(rumbleController);
    }
    interpolationValue = 0.8f;
    for (frameIndex = 0; frameIndex < 0xA; frameIndex++) {
        espTPLvlSet(spriteId, interpolationValue);
        espTPLvlSet(spriteId2, interpolationValue);
        interpolationValue = 0.8f - (0.08f * frameIndex);
        previousAnimationState = effect->animationState;
        HuPrcVSleep();
        if (effect->animationState != previousAnimationState) {
            return;
        }
    }
    espDispOff(spriteId);
    espDispOff(spriteId2);
    selectionEffectWork[effect->playerIndex].animationState = 0;
}
