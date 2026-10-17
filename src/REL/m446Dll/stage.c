#include "REL/m446Dll.h"

#include "game/hu3d.h"

// bss
M446ListNode *m446StageListHead;

// data
s32 m446StageModelFiles[5] = { 0x4D0008, 0x4D0007, 0x4D0006, 0x4D001F, 0x4D001F };
Vec m446StageModelPositions[5] = {
    { 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.1f, 0.0f },
    { 0.0f, 0.0f, 0.0f },
    { 160.0f, 160.0f, 0.0f },
    { -160.0f, 160.0f, 0.0f },
};
M446StageVertex m446StageLightVertices[3] = { { { 160.0f, 160.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0xFF, 0xFF, 0xFF, 0xFF } },
    { { -160.0f, 160.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0xFF, 0xFF, 0xFF, 0xFF } },
    { { 0.0f, 160.0f, 500.0f }, { 0.0f, 0.0f, 0.0f }, { 0xB0, 0xB0, 0xB0, 0xFF } } };
Vec m446StageLightDirections[3] = {
    { 1.0f, 3000.0f, 1.0f },
    { 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f },
};

s32 M446StageInitialize(void)
{
    m446StageListHead = 0;
    return 1;
}

void M446StageUpdate(void)
{
    M446ListNode *listNode;
    M446StageWork *stageWork;

    listNode = m446StageListHead;
    if (listNode) {
        do {
            stageWork = listNode->value;
            M446CardDestroy((M446CardWork *)stageWork); // ???
            listNode = listNode->next;
        } while (listNode);
    }
}

void M446StagesUpdate(void)
{
    s32 playerIndex;
    M446ListNode *listNode;
    M446StageWork *stageWork;

    listNode = m446StageListHead;
    if (listNode) {
        do {
            stageWork = listNode->value;
            if ((stageWork->stageLightsActive != 0) && (stageWork->stageLightIntensity < 1.0f)) {
                if ((stageWork->stageLightIntensity += 0.002f) > 1.0f) {
                    stageWork->stageLightIntensity = 1.0f;
                }
                Hu3DGLightColorSet(stageWork->lightIds[2], m446StageLightVertices[2].color.r * stageWork->stageLightIntensity, m446StageLightVertices[2].color.g * stageWork->stageLightIntensity,
                    m446StageLightVertices[2].color.b * stageWork->stageLightIntensity, m446StageLightVertices[2].color.a * stageWork->stageLightIntensity);
            }
            if (stageWork->stageActive != 0) {
                for (playerIndex = 0; playerIndex <= 1; playerIndex++) {
                    if (stageWork->playerLightIntensity < 1.0f) {
                        if ((stageWork->playerLightIntensity += 0.002f) > 1.0f) {
                            stageWork->playerLightIntensity = 1.0f;
                        }
                        Hu3DGLightColorSet(stageWork->lightIds[playerIndex], stageWork->playerLightIntensity * m446StageLightVertices[playerIndex].color.r,
                            stageWork->playerLightIntensity * m446StageLightVertices[playerIndex].color.g, stageWork->playerLightIntensity * m446StageLightVertices[playerIndex].color.b,
                            stageWork->playerLightIntensity * m446StageLightVertices[playerIndex].color.a);
                    }
                }
                if ((stageWork->stageMotionState == 0) && (Hu3DMotionEndCheck(stageWork->modelIds[2]) != 0)) {
                    Hu3DMotionStartEndSet(stageWork->modelIds[2], 180.0f, 359.0f);
                    Hu3DModelAttrSet(stageWork->modelIds[2], HU3D_MOTATTR_LOOP);
                    stageWork->stageMotionState = 1;
                }
                if ((stageWork->leftMotionState == 0) && (Hu3DMotionEndCheck(stageWork->modelIds[3]) != 0)) {
                    Hu3DMotionStartEndSet(stageWork->modelIds[3], 120.0f, 159.0f);
                    Hu3DModelAttrSet(stageWork->modelIds[3], HU3D_MOTATTR_LOOP);
                    stageWork->leftMotionState = 1;
                }
                if ((stageWork->rightMotionState == 0) && (Hu3DMotionEndCheck(stageWork->modelIds[4]) != 0)) {
                    Hu3DMotionStartEndSet(stageWork->modelIds[4], 120.0f, 159.0f);
                    Hu3DModelAttrSet(stageWork->modelIds[4], HU3D_MOTATTR_LOOP);
                    stageWork->rightMotionState = 1;
                }
            }
            listNode = listNode->next;
        } while (listNode);
    }
}

M446StageWork *M446StageCreate(void)
{
    s32 playerIndex;
    M446ListNode *listNode3;
    M446ListNode *stageListNode;
    M446StageWork *stageWork;

    stageWork = HuMemDirectMallocNum(HEAP_HEAP, 0x30, HU_MEMNUM_OVL);
    if (!stageWork) {
        return NULL;
    }
    stageWork->playerLightIntensity = 0.0f;
    stageWork->stageLightIntensity = 0.0f;
    stageWork->stageMotionState = 0;
    stageWork->leftMotionState = 0;
    stageWork->rightMotionState = 0;
    stageWork->stageActive = 0;
    stageWork->stageLightsActive = 0;
    stageWork->reserved = 0;

    for (playerIndex = 0; playerIndex < 5; playerIndex++) {
        stageWork->modelIds[playerIndex] = Hu3DModelCreateFile(m446StageModelFiles[playerIndex]);
        Hu3DModelPosSet(stageWork->modelIds[playerIndex], m446StageModelPositions[playerIndex].x, m446StageModelPositions[playerIndex].y, m446StageModelPositions[playerIndex].z);
    }
    Hu3DModelAttrSet(stageWork->modelIds[3], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(stageWork->modelIds[4], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(stageWork->modelIds[3], HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrSet(stageWork->modelIds[4], HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrReset(stageWork->modelIds[3], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrReset(stageWork->modelIds[4], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrReset(stageWork->modelIds[2], HU3D_MOTATTR_LOOP);

    for (playerIndex = 0; playerIndex < 3; playerIndex++) {
        stageWork->lightIds[playerIndex] = Hu3DGLightCreateV(&m446StageLightVertices[playerIndex].position, &m446StageLightVertices[playerIndex].direction, &m446StageLightVertices[playerIndex].color);
        Hu3DGLightStaticSet(stageWork->lightIds[playerIndex], 1);
        Hu3DGLightPointSet(stageWork->lightIds[playerIndex], 100.0f, 1.0f, 3);
        Hu3DGLightColorSet(stageWork->lightIds[playerIndex], 0, 0, 0, 0);
    }
    Hu3DShadowCreate(30.0f, 20.0f, 20000.0f);
    Hu3DShadowTPLvlSet(1.0f);
    Hu3DShadowPosSet(&m446StageLightDirections[0], &m446StageLightDirections[1], &m446StageLightDirections[2]);
    Hu3DModelShadowMapSet(stageWork->modelIds[0]);
    Hu3DModelShadowSet(stageWork->modelIds[1]);
    listNode3 = M446ListNodeCreate(stageWork);
    if (!listNode3) {
        OSReport("[!] m446StageCreate ERROR...\n");
        HuMemDirectFree(stageWork);
        return NULL;
    }
    if (m446StageListHead) {
        M446ListAppend(m446StageListHead, listNode3);
    }
    else {
        m446StageListHead = listNode3;
    }
    return stageWork;
}

void M446StageDestroy(M446StageWork *stageWork)
{
    s32 playerIndex;
    M446ListNode *listNode2;

    listNode2 = m446StageListHead;
    do {
        if (stageWork == listNode2->value) {
            if (listNode2 == m446StageListHead) {
                m446StageListHead = listNode2->next;
            }
            M446ListRemove(listNode2);
            M446ListNodeDestroy(listNode2);

            for (playerIndex = 0; playerIndex < 5; playerIndex++) {
                Hu3DModelKill(stageWork->modelIds[playerIndex]);
            }
            HuMemDirectFree(stageWork);
            return;
        }
        listNode2 = listNode2->next;
    } while (listNode2);
}

void M446StageStart(M446StageWork *stageWork)
{
    Hu3DModelAttrReset(stageWork->modelIds[3], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(stageWork->modelIds[4], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(stageWork->modelIds[3], HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrReset(stageWork->modelIds[4], HU3D_MOTATTR_PAUSE);
    stageWork->stageActive = 1;
    stageWork->leftMotionState = 0;
    stageWork->rightMotionState = 0;
    stageWork->stageMotionState = 0;
}

void M446StageUpdateState(M446StageWork *stageWork)
{
    Hu3DModelAttrSet(stageWork->modelIds[3], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(stageWork->modelIds[4], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(stageWork->modelIds[3], HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrSet(stageWork->modelIds[4], HU3D_MOTATTR_PAUSE);
    Hu3DMotionStartEndSet(stageWork->modelIds[2], 0.0f, 359.0f);
    Hu3DMotionStartEndSet(stageWork->modelIds[3], 0.0f, 119.0f);
    Hu3DMotionStartEndSet(stageWork->modelIds[4], 0.0f, 119.0f);
    stageWork->stageActive = 0;
    stageWork->leftMotionState = 0;
    stageWork->rightMotionState = 0;
    stageWork->stageMotionState = 0;
}

void M446StageRenderUpdate(M446StageWork *stageWork)
{
    stageWork->stageLightsActive = 1;
}
