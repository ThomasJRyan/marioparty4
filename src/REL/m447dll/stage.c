#include "REL/m447dll.h"
#include "game/audio.h"
#include "game/data.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/process.h"
#include "game/sprite.h"

#include "ext_math.h"

typedef struct {
    /* 0x00 */ Vec position;
    /* 0x0C */ Vec direction;
    /* 0x18 */ GXColor color;
} M447LightDefinition; // Size 0x1C

void M447StageProcess(void);
void M447StageModelsCreate(M447StageState *stageState);
void M447StageModelsDestroy(M447StageState *stageState);
void M447StageLightingInit(M447StageState *stageState);
void M447StageIdleUpdate(M447StageState *stageState);
void M447StageCardFlipUpdate(M447StageState *stageState);
void M447StageRevealUpdate(M447StageState *stageState);
void M447StageCardFlyoutUpdate(M447StageState *stageState);
void M447StageCardReturnUpdate(M447StageState *stageState);
void M447StageConfettiStart(M447StageState *stageState);
void M447StageConfettiUpdate(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);

const s32 stageModelAssetIds[] = { DATA_MAKE_NUM(DATADIR_M447, 0), DATA_MAKE_NUM(DATADIR_M447, 2), DATA_MAKE_NUM(DATADIR_M447, 3),
    DATA_MAKE_NUM(DATADIR_M447, 1), DATA_MAKE_NUM(DATADIR_M447, 1), DATA_MAKE_NUM(DATADIR_M447, 1), DATA_MAKE_NUM(DATADIR_M447, 1),
    DATA_MAKE_NUM(DATADIR_M447, 1), DATA_MAKE_NUM(DATADIR_M447, 24), DATA_MAKE_NUM(DATADIR_M447, 24), DATA_MAKE_NUM(DATADIR_M447, 24),
    DATA_MAKE_NUM(DATADIR_M447, 24), DATA_MAKE_NUM(DATADIR_M447, 24), DATA_MAKE_NUM(DATADIR_M447, 25), DATA_MAKE_NUM(DATADIR_M447, 26),
    DATA_MAKE_NUM(DATADIR_M447, 28), DATA_MAKE_NUM(DATADIR_M447, 29), DATA_MAKE_NUM(DATADIR_M447, 29), DATA_MAKE_NUM(DATADIR_M447, 29),
    DATA_MAKE_NUM(DATADIR_M447, 29), DATA_MAKE_NUM(DATADIR_M447, 29), 0 };

const Vec playerSpawnPositions[]
    = { { 0.0f, 0.1f, 0.0f }, { 400.0f, 0.1f, 0.0f }, { -400.0f, 0.1f, 0.0f }, { 200.0f, 0.1f, -400.0f }, { -200.0f, 0.1f, -400.0f } };

M447LightDefinition stageLightDefinitions[] = { { { 0.0f, 2500.0f, 0.0f }, { 0.0f, -0.5f, 0.0f }, { 0xFF, 0xFF, 0xFF, 0xFF } },
    { { 0.0f, -100.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0xFF, 0xFF, 0xFF, 0xFF } },
    { { 400.0f, -100.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0xFF, 0xFF, 0xFF, 0xFF } },
    { { -400.0f, -100.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0xFF, 0xFF, 0xFF, 0xFF } },
    { { 200.0f, -100.0f, -400.0f }, { 0.0f, 1.0f, 0.0f }, { 0xFF, 0xFF, 0xFF, 0xFF } },
    { { -200.0f, -100.0f, -400.0f }, { 0.0f, 1.0f, 0.0f }, { 0xFF, 0xFF, 0xFF, 0xFF } } };

M447StageState *M447StageCreate(void)
{
    M447StageState *object;
    s32 i;

    object = HuMemDirectMallocNum(HEAP_HEAP, sizeof(*object), HU_MEMNUM_OVL);
    if (!object) {
        return NULL;
    }
    object->actionState = 0;
    object->stageAnimationData = NULL;
    object->countdownFrames = 0;
    for (i = 0; i < 5; i++) {
        object->playerCardMotionActive[i] = 0;
        object->playerCardRevealed[i] = 0;
    }
    M447StageModelsCreate(object);
    M447StageLightingInit(object);
    object->process = HuPrcChildCreate(M447StageProcess, 0x65, 0x3000, 0, objectManager);
    object->process->user_data = object;
    return object;
}

void M447StageDestroy(M447StageState *stage)
{
    M447StageModelsDestroy(stage);
    HuPrcKill(stage->process);
    HuMemDirectFree(stage);
}

void M447StageActionSet(M447StageState *stage, s32 actionIndex)
{
    stage->actionState = actionIndex;
    stage->actionTimer = 0;
}

BOOL M447StageActionActiveCheck(M447StageState *stage)
{
    return stage->actionState != 0;
}

void M447PlayerSpawnPositionGet(s32 cardIndex, Vec *position)
{
    *position = playerSpawnPositions[cardIndex];
}

void M447PlayerCardReveal(M447StageState *stage, s32 playerIndex)
{
    s16 objectWork;

    objectWork = stage->modelIds[playerIndex + 3];
    Hu3DModelAttrReset(objectWork, HU3D_MOTATTR_PAUSE);
    Hu3DMotionTimeSet(objectWork, 0.0f);
    Hu3DMotionStartEndSet(objectWork, 0.0f, 40.0f);
    stage->playerCardMotionActive[playerIndex] = 1;
}

s32 M447UnrevealedPlayerCountGet(M447StageState *stage)
{
    s32 playerIndex;
    s32 i;

    playerIndex = 0;
    for (i = 0; i < 5; i++) {
        if (stage->playerCardMotionActive[i] == 0) {
            playerIndex++;
        }
    }
    return playerIndex;
}

void M447StageCountdownSet(M447StageState *stage, s32 frameCount)
{
    stage->countdownFrames = frameCount;
}

void (*stageActionHandlers[])(M447StageState *) = {
    M447StageIdleUpdate, M447StageCardFlipUpdate, M447StageRevealUpdate, M447StageCardFlyoutUpdate, M447StageCardReturnUpdate,
    NULL // padding?
};

void M447StageProcess(void)
{
    Vec position;
    M447StageState *objectWork;
    s16 cardIndex;
    s32 i;

    objectWork = HuPrcCurrentGet()->user_data;
    while (TRUE) {
        if (stageActionHandlers[objectWork->actionState]) {
            stageActionHandlers[objectWork->actionState](objectWork);
        }
        for (i = 0; i < 5; i++) {
            cardIndex = objectWork->modelIds[i + 3];
            if (Hu3DMotionEndCheck(cardIndex)) {
                Hu3DMotionStartEndSet(cardIndex, 20.0f, 40.0f);
                Hu3DModelAttrSet(cardIndex, HU3D_MOTATTR_LOOP);
            }
        }
        if (objectWork->countdownFrames > 0) {
            M447CameraPositionGet(&position);
            if (--objectWork->countdownFrames == 0) {
                position.y = 300.0f;
            }
            else {
                position.y = M447RandomValueGet() % 20 - 10 + 300.0f;
            }
            M447CameraPositionSet(&position);
        }
        HuPrcVSleep();
    }
}

void M447StageModelsCreate(M447StageState *stageState)
{
    s32 i;

    for (i = 0; i <= 20; i++) {
        stageState->modelIds[i] = Hu3DModelCreateFile(stageModelAssetIds[i]);
        Hu3DModelLayerSet(stageState->modelIds[i], 0);
    }
    stageState->stageAnimationData = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M447, 57));
    stageState->modelIds[21] = Hu3DParticleCreate(stageState->stageAnimationData, 64);
    Hu3DParticleHookSet(stageState->modelIds[21], M447StageConfettiUpdate);
    Hu3DParticleBlendModeSet(stageState->modelIds[21], 1);
    Hu3DModelLayerSet(stageState->modelIds[21], 0);
    Hu3DModelAttrSet(stageState->modelIds[21], HU3D_ATTR_DISPOFF);
    Hu3DModelShadowMapSet(stageState->modelIds[2]);
    for (i = 0; i < 5; i++) {
        Hu3DModelPosSet(stageState->modelIds[i + 3], playerSpawnPositions[i].x, playerSpawnPositions[i].y, playerSpawnPositions[i].z);
        Hu3DModelAttrSet(stageState->modelIds[i + 3], HU3D_MOTATTR_PAUSE);
        Hu3DModelShadowMapSet(stageState->modelIds[i + 3]);
    }
    for (i = 8; i <= 15; i++) {
        Hu3DModelAttrSet(stageState->modelIds[i], HU3D_ATTR_DISPOFF);
    }
    for (i = 16; i <= 20; i++) {
        Hu3DModelAttrSet(stageState->modelIds[i], HU3D_ATTR_DISPOFF);
    }
}

void M447StageModelsDestroy(M447StageState *stageState)
{
    s32 i;

    for (i = 0; i < 22; i++) {
        Hu3DModelKill(stageState->modelIds[i]);
    }
}

void M447StageLightingInit(M447StageState *stageState)
{
    s32 i;

    Hu3DLighInit();
    for (i = 0; i < 6; i++) {
        stageState->lightIds[i] = Hu3DGLightCreateV(&stageLightDefinitions[i].position, &stageLightDefinitions[i].direction, &stageLightDefinitions[i].color);
        switch (i) {
            case 0:
                Hu3DGLightInfinitytSet(stageState->lightIds[i]);
                Hu3DGLightStaticSet(stageState->lightIds[i], 1);
                break;
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
                Hu3DGLightStaticSet(stageState->lightIds[i], 1);
                Hu3DGLightSpotSet(stageState->lightIds[i], 20.0f, 2);
                Hu3DGLightColorSet(stageState->lightIds[i], 0, 0, 0, 0);
                break;
        }
    }
}

void M447StageIdleUpdate(M447StageState *stageState) { }

void M447StageCardFlipUpdate(M447StageState *stageState)
{
    s32 i;

    switch (stageState->actionTimer) {
        case 0:
            stageState->actionTimer = 1;
            break;
        case 1:
            Hu3DModelPosSet(stageState->modelIds[14], 0.0f, 0.0f, 0.0f);
            Hu3DMotionTimeSet(stageState->modelIds[14], 0.0f);
            Hu3DModelAttrReset(stageState->modelIds[14], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(stageState->modelIds[14], HU3D_MOTATTR_LOOP);
            stageState->stateTimer = 0;
            stageState->actionTimer = 2;
            HuAudFXPlay(0x77E);
            for (i = 0; i < 2; i++) {
                M447PlayerVibrationSet(playerStates[i], 0, 0x3C);
            }
            break;
        case 2:
            if (stageState->stateTimer++ >= 60) {
                stageState->actionTimer = 3;
            }
            break;
        case 3:
            Hu3DModelAttrSet(stageState->modelIds[14], HU3D_ATTR_DISPOFF);
            stageState->actionState = 0;
            stageState->actionTimer = 0;
            break;
    }
}

void M447StageRevealUpdate(M447StageState *stageState)
{
    switch (stageState->actionTimer) {
        case 0:
            stageState->actionTimer = 1;
            break;
        case 1:
            M447StageConfettiStart(stageState);
            Hu3DModelPosSet(stageState->modelIds[15], 0.0f, 0.0f, 0.0f);
            Hu3DMotionTimeSet(stageState->modelIds[15], 0.0f);
            Hu3DModelAttrReset(stageState->modelIds[15], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(stageState->modelIds[15], HU3D_MOTATTR_LOOP);
            stageState->stateTimer = 0;
            stageState->actionTimer = 2;
            HuAudFXPlay(0x782);
            break;
        case 2:
            if (stageState->stateTimer++ >= 60) {
                stageState->actionTimer = 3;
            }
            break;
        case 3:
            Hu3DModelAttrSet(stageState->modelIds[15], HU3D_ATTR_DISPOFF);
            stageState->actionState = 0;
            stageState->actionTimer = 0;
            break;
    }
}

void M447StageCardFlyoutUpdate(M447StageState *stageState)
{
    float distance;
    s16 cardIndex;
    s32 i;

    switch (stageState->actionTimer) {
        case 0:
            stageState->cardMotionProgress = 0.0f;
            stageState->cardMotionSpeed = 0.011111111f;
            stageState->actionTimer = 1;
            HuAudFXPlay(0x77F);
            break;
        case 1:
            distance = -90.0f * stageState->cardMotionProgress;
            for (i = 0; i < 5; i++) {
                Hu3DModelPosSet(stageState->modelIds[i + 3], playerSpawnPositions[i].x, distance, playerSpawnPositions[i].z);
            }
            stageState->cardMotionProgress += stageState->cardMotionSpeed;
            if (stageState->cardMotionProgress < 1.0f) {
                break;
            }
            for (i = 0; i < 5; i++) {
                Hu3DMotionTimeSet(stageState->modelIds[i + 3], 0.0f);
                Hu3DModelAttrSet(stageState->modelIds[i + 3], HU3D_MOTATTR_PAUSE);
                stageState->playerCardMotionActive[i] = 0;
                stageState->playerCardRevealed[i] = 0;
            }
            stageState->actionTimer = 2;
            break;
        case 2:
            for (i = 0; i < 5; i++) {
                cardIndex = stageState->modelIds[i + 8];
                Hu3DModelPosSet(cardIndex, playerSpawnPositions[i].x, playerSpawnPositions[i].y, playerSpawnPositions[i].z);
                Hu3DMotionTimeSet(cardIndex, 0.0f);
                Hu3DModelAttrReset(cardIndex, HU3D_ATTR_DISPOFF);
                Hu3DGLightColorSet(stageState->lightIds[i + 1], stageLightDefinitions[i + 1].color.r, stageLightDefinitions[i + 1].color.g, stageLightDefinitions[i + 1].color.b,
                    stageLightDefinitions[i + 1].color.a);
            }
            stageState->stateTimer = 0;
            stageState->actionTimer = 3;
            HuAudFXPlay(0x780);
            break;
        case 3:
            if (stageState->stateTimer++ >= 60) {
                for (i = 0; i < 5; i++) {
                    Hu3DModelAttrSet(stageState->modelIds[i + 8], HU3D_ATTR_DISPOFF);
                    Hu3DGLightColorSet(stageState->lightIds[i + 1], 0, 0, 0, 0);
                }
                stageState->actionTimer = 4;
            }
            break;
        case 4:
            stageState->actionState = 0;
            stageState->actionTimer = 0;
            break;
    }
}

void M447StageCardReturnUpdate(M447StageState *stageState)
{
    float distance;
    s16 cardIndex;
    s32 i;

    switch (stageState->actionTimer) {
        case 0:
            stageState->cardMotionProgress = 0.0f;
            stageState->cardMotionSpeed = 0.033333335f;
            stageState->actionTimer = 1;
            HuAudFXPlay(0x77F);
            break;
        case 1:
            distance = 90.0f * stageState->cardMotionProgress + -90.0f + 0.1f;
            for (i = 0; i < 5; i++) {
                Hu3DModelPosSet(stageState->modelIds[i + 3], playerSpawnPositions[i].x, distance, playerSpawnPositions[i].z);
            }
            stageState->cardMotionProgress += stageState->cardMotionSpeed;
            if (stageState->cardMotionProgress < 1.0f) {
                break;
            }
            for (i = 0; i < 5; i++) {
                Hu3DModelPosSet(stageState->modelIds[i + 3], playerSpawnPositions[i].x, playerSpawnPositions[i].y, playerSpawnPositions[i].z);
            }
            HuAudFXPlay(0x783);
            stageState->actionTimer = 2;
            break;
        case 2:
            for (i = 0; i < 5; i++) {
                cardIndex = stageState->modelIds[i + 16];
                Hu3DModelPosSet(cardIndex, playerSpawnPositions[i].x, playerSpawnPositions[i].y, playerSpawnPositions[i].z);
                Hu3DMotionTimeSet(cardIndex, 0.0f);
                Hu3DModelAttrReset(cardIndex, HU3D_MOTATTR_LOOP);
                Hu3DModelAttrReset(cardIndex, HU3D_ATTR_DISPOFF);
            }
            stageState->stateTimer = 0;
            stageState->actionTimer = 3;
            break;
        case 3:
            if (stageState->stateTimer++ >= 20) {
                for (i = 0; i < 5; i++) {
                    Hu3DModelAttrSet(stageState->modelIds[i + 16], HU3D_ATTR_DISPOFF);
                }
                stageState->actionTimer = 4;
                break;
            }
            break;
        case 4:
            stageState->actionState = 0;
            stageState->actionTimer = 0;
            break;
    }
}

void M447StageConfettiStart(M447StageState *stageState)
{
    HU3DMODEL *blockWork;
    HU3DPARTICLE *objectWork;

    blockWork = &Hu3DData[stageState->modelIds[21]];
    objectWork = blockWork->hookData;
    blockWork->attr &= ~1;
    objectWork->dataCnt = 0;
    objectWork->emitCnt = 1;
    objectWork->pos.x = 0.0f;
    objectWork->pos.y = 350.0f;
    objectWork->pos.z = -700.0f;
    HuAudFXPlay(0x780);
}

void M447StageConfettiUpdate(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    GXColor position = { 0xFF, 0xFF, 0xFF, 0xFF };
    HU3DPARTICLEDATA *loopIndex;
    s32 i;

    if (particle->emitCnt == 0) {
        return;
    }
    if (particle->dataCnt == 0) {
        particle->dataCnt = 1;
        particle->pos.z = 0.0f;
        particle->unk_10.z = 0.016666668f;
        for (i = 0, loopIndex = particle->data; i < particle->maxCnt; i++, loopIndex++) {
            loopIndex->time = 0;
            loopIndex->scale = 20.0f;
            loopIndex->color.r = position.r;
            loopIndex->color.g = position.g;
            loopIndex->color.b = position.b;
            loopIndex->color.a = position.a;
            loopIndex->pos.x = particle->pos.x;
            loopIndex->pos.y = particle->pos.y;
            loopIndex->pos.z = particle->pos.z;
            loopIndex->vel.x = -8.0f + 0.1f * (M447RandomValueGet() % 160);
            loopIndex->vel.y = -8.0f + 0.1f * (M447RandomValueGet() % 160);
            loopIndex->vel.z = 0.0f;
        }
    }
    for (i = 0, loopIndex = particle->data; i < particle->maxCnt; i++, loopIndex++) {
        if (loopIndex->time < 0) {
            continue;
        }
        loopIndex->pos.x += loopIndex->vel.x;
        loopIndex->pos.y += loopIndex->vel.y;
        loopIndex->pos.z = -700.0 + 500.0 * sind(90.0f * particle->pos.z);
        loopIndex->color.a = 255.0f * (1.0f - particle->pos.z);
        loopIndex->scale = 20.0f + M447RandomValueGet() % 30;
    }
    particle->pos.z += particle->unk_10.z;
    if (particle->pos.z >= 1.0f) {
        model->attr |= 1;
        particle->emitCnt = 0;
    }
}
