#include "REL/w10Dll.h"
#include "game/data.h"
#include "game/board/model.h"
#include "game/board/tutorial.h"
#include "game/board/window.h"

#ifndef __MWERKS__
#include "game/hsfex.h"
#include "game/board/main.h"
#endif

static s16 hostMdl;
static s16 hostPositionIndex;
static HUPROCESS *hostProc;

static s32 hostMot[2] = {
    DATA_MAKE_NUM(DATADIR_W10, 16),
    -1
};

static Vec hostPosTbl[2] = {
    {70.0f, 355.0f, 200.f},
    {70.0f, 175.0f, 200.f},
};

static void UpdateHost(void);


void HostCreate(void) {
    hostPositionIndex = 0;
    hostMdl = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W10, 15), hostMot, 0);
    BoardModelMotionStart(hostMdl, 1, 0x40000001);
    BoardTutorialHostSet(hostMdl);
    BoardModelLayerSet(hostMdl, 6);
    hostProc = HuPrcCreate(UpdateHost, 0x1FFF, 0x1800, 0);
}

void HostWinExec(s32 messageId) {
    s16 hostWindowStyle;

    switch (hostPositionIndex) {
    case 0:
        hostWindowStyle = 5;
        break;
    case 1:
        hostWindowStyle = 6;
        break;
    }
    BoardWinCreate(hostWindowStyle, messageId, -1);
    BoardWinWait();
    BoardWinKill();
}

void HostPosSet(s16 positionIndex) {
    hostPositionIndex = positionIndex;
}

static void UpdateHost(void) {
    Mtx rot;
    Vec hostPosition;
    Vec cameraRotation;
    
    while (1) {
        if (BoardIsKill() != 0) {
            BoardModelKill(hostMdl);
            BoardTutorialHostSet(-1);
            HuPrcEnd();
        }
        Hu3D2Dto3D(&hostPosTbl[hostPositionIndex], 1, &hostPosition);
        BoardModelPosSetV(hostMdl, &hostPosition);
        BoardCameraRotGet(&cameraRotation);
        MTXRotDeg(rot, 'y', 15);
        BoardModelMtxSet(hostMdl, &rot);
        BoardModelRotSet(hostMdl, cameraRotation.x, 0.0f, 0.0f);
        BoardModelScaleSet(hostMdl, 0.2f, 0.2f, 0.2f);
        HuPrcVSleep();        
    }
}
