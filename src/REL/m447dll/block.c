#include "REL/m447dll.h"
#include "game/audio.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/sprite.h"

#include "ext_math.h"

void M447BlockDestroy(void *blockState);
void M447BlockProcess(void);
void M447BlockModelsCreate(M447BlockState *blockState);
void M447BlockModelsDestroy(M447BlockState *blockState);
void M447BlockTransformUpdate(M447BlockState *blockState);
void M447BlockIdleUpdate(M447BlockState *blockState);
void M447BlockBounceUpdate(M447BlockState *blockState);
void M447BlockShakeUpdate(M447BlockState *blockState);
void M447BlockArcUpdate(M447BlockState *blockState);
void M447BlockVanishUpdate(M447BlockState *blockState);
void M447BlockParticleUpdate(M447BlockState *blockState);
void M447BlockParticleStart(M447BlockState *blockState);
void M447BlockParticleEmit(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);

u32 randomSeed;
M447BlockState *blockStates[5];
s32 alternateLayoutEnabled;

const s32 blockModelAssetIds[]
    = { DATA_MAKE_NUM(DATADIR_M447, 12), DATA_MAKE_NUM(DATADIR_M447, 13), DATA_MAKE_NUM(DATADIR_M447, 10), DATA_MAKE_NUM(DATADIR_M447, 11),
          DATA_MAKE_NUM(DATADIR_M447, 4), DATA_MAKE_NUM(DATADIR_M447, 5), DATA_MAKE_NUM(DATADIR_M447, 16), DATA_MAKE_NUM(DATADIR_M447, 17),
          DATA_MAKE_NUM(DATADIR_M447, 8), DATA_MAKE_NUM(DATADIR_M447, 9), DATA_MAKE_NUM(DATADIR_M447, 6), DATA_MAKE_NUM(DATADIR_M447, 7),
          DATA_MAKE_NUM(DATADIR_M447, 18), DATA_MAKE_NUM(DATADIR_M447, 19), DATA_MAKE_NUM(DATADIR_M447, 20), DATA_MAKE_NUM(DATADIR_M447, 21),
          DATA_MAKE_NUM(DATADIR_M447, 22), DATA_MAKE_NUM(DATADIR_M447, 23), DATA_MAKE_NUM(DATADIR_M447, 14), DATA_MAKE_NUM(DATADIR_M447, 15) };

const float blockLandingHeights[20]
    = { 30.0f, 30.0f, 80.0f, 30.0f, 80.0f, 80.0f, 60.0f, 60.0f, 80.0f, 80.0f, 30.0f, 30.0f, 45.0f, 45.0f, 50.0f, 50.0f, 30.0f, 30.0f, 75.0f, 50.0f };

M447BlockState *M447BlockCreate(void)
{
    M447BlockState *object;

    object = HuMemDirectMallocNum(HEAP_HEAP, sizeof(*object), HU_MEMNUM_OVL);
    if (!object) {
        OSReport("M447BlockCreate ERROR...\n");
        return NULL;
    }
    object->actionState = 0;
    object->cardIndex = 0;
    object->specialCardFlag = 0;
    object->actionStep = 0;
    object->position.x = 0.0f;
    object->position.y = 0.0f;
    object->position.z = 0.0f;
    object->modelRotationX = 0.0f;
    object->modelRotationY = 0.0f;
    object->modelRotationZ = 0.0f;
    object->modelScaleX = 0.0f;
    object->modelScaleY = 0.0f;
    object->modelScaleZ = 0.0f;
    object->rotation.x = 0.0f;
    object->rotation.y = 0.0f;
    object->rotation.z = 0.0f;
    object->modelAlpha = 1.0f;
    M447BlockModelsCreate(object);
    object->process = HuPrcChildCreate(M447BlockProcess, 105, 0x3000, 0, objectManager);
    object->process->user_data = object;
    return object;
}

void M447BlockDestroy(void *blockState)
{
    M447BlockDestroy(blockState);
    HuMemDirectFree(blockState);
}

void M447BlockActionSet(M447BlockState *block, s32 action)
{
    block->actionState = action;
    block->actionStep = 0;
}

void M447BlockCardSet(M447BlockState *block, s32 cardIndex, s32 isSpecialCard)
{
    if (alternateLayoutEnabled != 0) {
        block->cardIndex = cardIndex * 2 + (isSpecialCard != 0 ? 0 : 1);
    }
    else {
        block->cardIndex = cardIndex * 2 + (isSpecialCard != 0 ? 1 : 0);
    }
    block->specialCardFlag = isSpecialCard;
    block->modelAlpha = 1.0f;
    block->modelScaleX = 1.0f;
    block->modelScaleY = 1.0f;
    block->modelScaleZ = 1.0f;
    Hu3DModelAttrSet(block->modelIds[block->cardIndex], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(block->modelIds[21], HU3D_ATTR_DISPOFF);
}

void M447BlockLayoutSet(s32 alternateLayout)
{
    alternateLayoutEnabled = alternateLayout;
}

void (*blockActionHandlers[])(M447BlockState *) = { M447BlockIdleUpdate, M447BlockBounceUpdate, M447BlockShakeUpdate, M447BlockArcUpdate, M447BlockVanishUpdate, M447BlockParticleUpdate };

void M447BlockProcess(void)
{
    M447BlockState *objectWork;

    objectWork = HuPrcCurrentGet()->user_data;
    while (TRUE) {
        blockActionHandlers[objectWork->actionState](objectWork);
        M447BlockTransformUpdate(objectWork);
        HuPrcVSleep();
    }
}

void M447BlockModelsCreate(M447BlockState *blockState)
{
    s32 i;

    for (i = 0; i <= 19; i++) {
        blockState->modelIds[i] = Hu3DModelCreateFile(blockModelAssetIds[i]);
        Hu3DModelAttrSet(blockState->modelIds[i], HU3D_ATTR_ZWRITE_OFF);
        Hu3DModelLayerSet(blockState->modelIds[i], 7);
    }
    blockState->modelIds[20] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M447, 30));
    Hu3DModelLayerSet(blockState->modelIds[20], 7);
    blockState->modelIds[21] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M447, 25));
    Hu3DModelLayerSet(blockState->modelIds[21], 7);
    blockState->particleAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M447, 57));
    blockState->modelIds[22] = Hu3DParticleCreate(blockState->particleAnimation, 64);
    Hu3DParticleHookSet(blockState->modelIds[22], M447BlockParticleEmit);
    Hu3DParticleBlendModeSet(blockState->modelIds[22], 1);
    Hu3DModelLayerSet(blockState->modelIds[22], 6);
    Hu3DModelAttrSet(blockState->modelIds[22], HU3D_ATTR_DISPOFF);
    for (i = 0; i <= 19; i++) {
        Hu3DModelAttrSet(blockState->modelIds[i], HU3D_ATTR_DISPOFF);
    }
    Hu3DModelAttrReset(blockState->modelIds[blockState->cardIndex], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(blockState->modelIds[20], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(blockState->modelIds[21], HU3D_ATTR_DISPOFF);
    for (i = 0; i <= 19; i++) {
        Hu3DModelShadowSet(blockState->modelIds[i]);
    }
}

void M447BlockModelsDestroy(M447BlockState *blockState)
{
    s32 i;

    for (i = 0; i < 23; i++) {
        Hu3DModelKill(blockState->modelIds[i]);
    }
}

void M447BlockTransformUpdate(M447BlockState *blockState)
{
    HU3DMODEL *blockWork;
    Mtx position;

    blockWork = &Hu3DData[blockState->modelIds[blockState->cardIndex]];
    MTXIdentity(blockWork->mtx);
    MTXRotRad(position, 'x', MTXDegToRad(blockState->rotation.x));
    MTXConcat(blockWork->mtx, position, blockWork->mtx);
    MTXRotRad(position, 'y', MTXDegToRad(blockState->rotation.y));
    MTXConcat(blockWork->mtx, position, blockWork->mtx);
    MTXRotRad(position, 'z', MTXDegToRad(blockState->rotation.z));
    MTXConcat(blockWork->mtx, position, blockWork->mtx);
    Hu3DModelPosSet(blockState->modelIds[blockState->cardIndex], blockState->position.x, blockState->position.y, blockState->position.z);
    Hu3DModelPosSet(blockState->modelIds[20], blockState->position.x, blockState->position.y - 100.0f, blockState->position.z);
    Hu3DModelPosSet(blockState->modelIds[21], blockState->position.x, blockState->position.y, blockState->position.z + 120.0f * blockState->modelScaleZ);
    Hu3DModelRotSet(blockState->modelIds[blockState->cardIndex], blockState->modelRotationX, blockState->modelRotationY, blockState->modelRotationZ);
    Hu3DModelScaleSet(blockState->modelIds[blockState->cardIndex], blockState->modelScaleX, blockState->modelScaleY, blockState->modelScaleZ);
    Hu3DModelScaleSet(blockState->modelIds[21], blockState->modelScaleX, blockState->modelScaleY, blockState->modelScaleZ);
    Hu3DModelTPLvlSet(blockState->modelIds[blockState->cardIndex], blockState->modelAlpha);
}

void M447BlockIdleUpdate(M447BlockState *blockState) { }

void M447BlockBounceUpdate(M447BlockState *blockState)
{
    float distance;

    switch (blockState->actionStep) {
        case 0:
            blockState->actionProgress = 0.0f;
            blockState->actionProgressStep = 0.016666668f;
            blockState->position.y = -100.0f;
            blockState->modelRotationY = 0.0f;
            blockState->modelScaleX = 0.0f;
            blockState->modelScaleY = 0.0f;
            blockState->modelScaleZ = 0.0f;
            blockState->rotationSpeedY = -50.0f;
            Hu3DModelAttrReset(blockState->modelIds[blockState->cardIndex], HU3D_ATTR_DISPOFF);
            blockState->actionStep = 1;
            /* fallthrough */
        case 1:
            blockState->modelRotationY += blockState->rotationSpeedY;
            distance = sind(90.0f * blockState->actionProgress);
            blockState->position.y = -100.0f + 500.0f * distance;
            blockState->modelScaleX = blockState->actionProgress;
            blockState->modelScaleY = blockState->actionProgress;
            blockState->modelScaleZ = blockState->actionProgress;
            blockState->modelAlpha = distance;
            blockState->actionProgress += blockState->actionProgressStep;
            if (blockState->actionProgress < 1.0f) {
                break;
            }
            blockState->position.y = 400.0f;
            blockState->modelScaleX = 1.0f;
            blockState->modelScaleY = 1.0f;
            blockState->modelScaleZ = 1.0f;
            blockState->modelAlpha = 1.0f;
            blockState->actionStep = 2;
            /* fallthrough */
        case 2:
            M447BlockActionSet(blockState, 2);
            break;
    }
}

void M447BlockShakeUpdate(M447BlockState *blockState)
{
    float distance;

    switch (blockState->actionStep) {
        case 0:
            blockState->actionProgress = 0.1f * (M447RandomValueGet() % 10);
            blockState->actionProgressStep = 0.011111111f;
            blockState->actionStep = 1;
            /* fallthrough */
        case 1:
            if (blockState->rotationSpeedY < -8.0f) {
                blockState->rotationSpeedY += 0.05f;
            }
            distance = sind(180.0f * blockState->actionProgress);
            distance *= distance;
            blockState->position.y = 400.0f + distance * 25.0f;
            blockState->modelRotationY += blockState->rotationSpeedY;
            blockState->actionProgress += blockState->actionProgressStep;
            if (blockState->actionProgress < 1.0f) {
                break;
            }
            blockState->actionProgress = 0.0f;
            break;
        case 2:
            M447BlockActionSet(blockState, 0);
            break;
    }
}

void M447BlockArcUpdate(M447BlockState *blockState)
{
    float distance;

    switch (blockState->actionStep) {
        case 0:
            blockState->actionProgress = 0.0f;
            blockState->actionProgressStep = 0.5f;
            blockState->rotationProgress = 0.0f;
            blockState->rotationProgressStep = 0.05f;
            blockState->modelAlpha = 1.0f;
            blockState->position.y = 400.0f;
            blockState->rotationStart = blockState->rotation;
            blockState->targetRotationX = 0.0f;
            blockState->targetRotationY = blockState->rotation.y;
            blockState->targetRotationZ = 0.0f;
            blockState->actionStep = 1;
            /* fallthrough */
        case 1:
            distance = blockState->rotationProgress;
            blockState->rotation.x = blockState->rotationStart.x + distance * (blockState->targetRotationX - blockState->rotationStart.x);
            blockState->rotation.y = blockState->rotationStart.y + distance * (blockState->targetRotationY - blockState->rotationStart.y);
            blockState->rotation.z = blockState->rotationStart.z + distance * (blockState->targetRotationZ - blockState->rotationStart.z);
            if (blockState->rotationProgress < 1.0f) {
                blockState->rotationProgress += blockState->rotationProgressStep;
                if (blockState->rotationProgress > 1.0f) {
                    blockState->rotationProgress = 1.0f;
                }
            }
            distance = blockState->actionProgress;
            blockState->position.y = 400.0 + 50.0f * distance * sind(90) - 4.9f * distance * distance;
            blockState->actionProgress += blockState->actionProgressStep;
            if (blockState->position.y >= blockLandingHeights[blockState->cardIndex]) {
                break;
            }
            blockState->position.y = blockLandingHeights[blockState->cardIndex];
            HuAudFXPlay(0x785);
            M447StageCountdownSet(stageState, 30);
            blockState->actionStep = 2;
            /* fallthrough */
        case 2:
            Hu3DModelAttrReset(blockState->modelIds[20], HU3D_ATTR_DISPOFF);
            Hu3DMotionTimeSet(blockState->modelIds[20], 0.0f);
            M447BlockActionSet(blockState, 4);
            break;
    }
}

void M447BlockVanishUpdate(M447BlockState *blockState)
{
    switch (blockState->actionStep) {
        case 0:
            blockState->actionProgress = 0.0f;
            blockState->actionProgressStep = 0.016666668f;
            blockState->actionStep = 1;
            /* fallthrough */
        case 1:
            blockState->modelAlpha = 1.0 - sind(90.0f * blockState->actionProgress);
            blockState->actionProgress += blockState->actionProgressStep;
            if (blockState->actionProgress < 1.0f) {
                break;
            }
            blockState->actionStep = 2;
            /* fallthrough */
        case 2:
            Hu3DModelAttrSet(blockState->modelIds[blockState->cardIndex], HU3D_ATTR_DISPOFF);
            M447BlockActionSet(blockState, 0);
            break;
    }
}

void M447BlockParticleUpdate(M447BlockState *blockState)
{
    float distance;

    switch (blockState->actionStep) {
        case 0:
            blockState->actionProgress = 0.0f;
            blockState->actionProgressStep = 0.008333334f;
            Hu3DModelAttrReset(blockState->modelIds[21], HU3D_ATTR_DISPOFF);
            Hu3DMotionTimeSet(blockState->modelIds[21], 0.0f);
            Hu3DModelAttrSet(blockState->modelIds[21], HU3D_MOTATTR_LOOP);
            blockState->particlePath[0] = blockState->position;
            blockState->particlePath[1].x = 0.0f;
            blockState->particlePath[1].y = 200.0f;
            blockState->particlePath[1].z = -700.0f - blockState->position.z / 2;
            blockState->particlePath[2].x = 0.0f;
            blockState->particlePath[2].y = 400.0f;
            blockState->particlePath[2].z = -700.0f;
            M447BlockParticleStart(blockState);
            HuAudFXPlay(0x781);
            blockState->actionStep = 1;
            /* fallthrough */
        case 1:
            distance = blockState->actionProgress;
            blockState->modelScaleX = 1.0f - distance;
            blockState->modelScaleY = 1.0f - distance;
            blockState->modelScaleZ = 1.0f - distance;
            M447SplinePositionGet(&blockState->position, blockState->particlePath, 3, distance);
            blockState->actionProgress += blockState->actionProgressStep;
            if (blockState->actionProgress < 1.0f) {
                break;
            }
            blockState->actionStep = 2;
            /* fallthrough */
        case 2:
            M447StageActionSet(stageState, 2);
            M447BlockActionSet(blockState, 0);
            break;
    }
}

void M447BlockParticleStart(M447BlockState *blockState)
{
    HU3DMODEL *blockWork;
    HU3DPARTICLE *objectWork;

    blockWork = &Hu3DData[blockState->modelIds[22]];
    objectWork = blockWork->hookData;
    blockWork->attr &= ~1;
    objectWork->dataCnt = 0;
    objectWork->emitCnt = 1;
    objectWork->work = blockState;
}

void M447BlockParticleEmit(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    GXColor position = { 0xFF, 0xFF, 0xFF, 0xFF };
    M447BlockState *cardIndex;
    HU3DPARTICLEDATA *loopIndex;
    s32 stateIndex;
    s32 i;

    if (particle->emitCnt == 0) {
        return;
    }
    cardIndex = particle->work;
    if (particle->dataCnt == 0) {
        particle->dataCnt = 1;
        for (i = 0, loopIndex = particle->data; i < particle->maxCnt; i++, loopIndex++) {
            loopIndex->time = i * 5;
            loopIndex->parManId = 0;
            loopIndex->scale = 0.0f;
            loopIndex->color.r = position.r;
            loopIndex->color.g = position.g;
            loopIndex->color.b = position.b;
            loopIndex->color.a = position.a;
        }
    }
    for (i = 0, stateIndex = 0, loopIndex = particle->data; i < particle->maxCnt; i++, loopIndex++) {
        if (loopIndex->time > 0 && --loopIndex->time > 0) {
            continue;
        }
        if (loopIndex->parManId == 0 && cardIndex->actionState == 5 && cardIndex->position.z > -700.0f) {
            loopIndex->pos.x = cardIndex->position.x;
            loopIndex->pos.y = cardIndex->position.y;
            loopIndex->pos.z = cardIndex->position.z;
            loopIndex->scale = 20.0f;
            loopIndex->parManId = 1;
        }
        else if (loopIndex->parManId == 1) {
            loopIndex->pos.y -= 1.0f;
            loopIndex->pos.z += 3.0f;
            loopIndex->scale = 20.0f + M447RandomValueGet() % 60;
            if (loopIndex->color.a > 10) {
                loopIndex->color.a -= 4;
            }
            else {
                loopIndex->color.a = 0;
                loopIndex->parManId = 2;
            }
        }
        else {
            stateIndex++;
        }
    }
    if (stateIndex == particle->maxCnt) {
        OSReport("*****************************************************\n");
        OSReport("*****************************************************\n");
        OSReport("*****************************************************\n");
        OSReport("*****************************************************\n");
        OSReport("*****************************************************\n");
        OSReport("*****************************************************\n");
        OSReport("*****************************************************\n");
        OSReport("*****************************************************\n");
        particle->emitCnt = 0;
    }
}

void M447RandomSeedSet(s32 seed)
{
    randomSeed = seed;
}

s32 M447RandomValueGet(void)
{
    return ((randomSeed = randomSeed * 0x41C64E6D + 0x3039) >> 16) % (1 << 15);
}
