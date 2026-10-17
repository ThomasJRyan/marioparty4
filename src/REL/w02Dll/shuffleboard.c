#include "REL/w02Dll.h"
#include "game/audio.h"
#include "game/pad.h"
#include "game/objsub.h"
#include "game/frand.h"
#include "game/chrman.h"

#include "ext_math.h"

#ifndef __MWERKS__
#include "game/board/audio.h"
#endif


HUPROCESS *shuffleboardProcess;
HUPROCESS *shuffleboardItemSpinProcess;
s16 shuffleboardDirection;
s16 shuffleboardPlayerIndex;
s16 shuffleboardPlayerShadowModelId;
s16 shuffleboardCartModelId;
s16 shuffleboardItemModelIds[15];
Vec shuffleboardItemPositions[15];
Vec shuffleboardItemRotations[15];
s16 shuffleboardItemVisible[15];
s16 shuffleboardPlayerMotionIds[2];
float shuffleboardPlayerAngle;
Vec shuffleboardTargetPosition;
s16 shuffleboardSpaceIndex;
Vec shuffleboardCameraTarget;

s32 shuffleboardModelData[8] = {
	DATA_MAKE_NUM(DATADIR_W02, 0x2E),
	DATA_MAKE_NUM(DATADIR_W02, 0x2F),
	DATA_MAKE_NUM(DATADIR_W02, 0x30),
	DATA_MAKE_NUM(DATADIR_W02, 0x31),
	DATA_MAKE_NUM(DATADIR_W02, 0x32),
	DATA_MAKE_NUM(DATADIR_W02, 0x33),
	DATA_MAKE_NUM(DATADIR_W02, 0x34),
	DATA_MAKE_NUM(DATADIR_W02, 0x35)
};

s32 shuffleboardMotionData[8] = {
	DATA_MAKE_NUM(DATADIR_W02, 0x26),
	DATA_MAKE_NUM(DATADIR_W02, 0x27),
	DATA_MAKE_NUM(DATADIR_W02, 0x28),
	DATA_MAKE_NUM(DATADIR_W02, 0x29),
	DATA_MAKE_NUM(DATADIR_W02, 0x2A),
	DATA_MAKE_NUM(DATADIR_W02, 0x2B),
	DATA_MAKE_NUM(DATADIR_W02, 0x2C),
	DATA_MAKE_NUM(DATADIR_W02, 0x2D)
};

s32 ShuffleboardModelsPosition(void)
{
	s32 modelIndex;
	Vec spacePosition;
	Vec targetPosition;
	BoardModelVisibilitySet(boardModelIds[13], 1);
	BoardModelVisibilitySet(boardModelIds[14], 1);
	targetPosition.x = targetPosition.y = targetPosition.z = 0;
	BoardModelRotSetV(boardModelIds[13], &targetPosition);
	BoardModelRotSetV(boardModelIds[14], &targetPosition);
	BoardModelPosSetV(boardModelIds[13], &shuffleboardNorthModelPosition);
	BoardModelPosSetV(boardModelIds[14], &shuffleboardSouthModelPosition);
	BoardModelVisibilitySet(rouletteGambleMarkerModelId, 1);
	BoardModelVisibilitySet(shuffleboardGambleMarkerModelId, 1);
	modelIndex = BoardSpaceFlagSearch(0, 0x200);
	BoardSpacePosGet(0, modelIndex, &spacePosition);
	BoardModelPosSetV(rouletteGambleMarkerModelId, &spacePosition);
	BoardModelRotSetV(rouletteGambleMarkerModelId, &targetPosition);
	modelIndex = BoardSpaceFlagSearch(0, 0x400);
	BoardSpacePosGet(0, modelIndex, &spacePosition);
	BoardModelPosSetV(shuffleboardGambleMarkerModelId, &spacePosition);
	BoardModelRotSetV(shuffleboardGambleMarkerModelId, &targetPosition);
	return 1;
}

s32 ShuffleboardEventCleanup(void);
void ShuffleboardEventProcess(void);

void ShuffleboardEventStart(u32 directionFlag)
{
	BoardEventFlagSet();
	if(directionFlag < 2) {
		shuffleboardDirection = 0;
	} else {
		shuffleboardDirection = 1;
	}
	shuffleboardPlayerIndex = GWSystem.player_curr;
	BoardPlayerPostTurnHookSet(shuffleboardPlayerIndex, ShuffleboardModelsPosition);
	shuffleboardProcess = HuPrcCreate(ShuffleboardEventProcess, 8196, 8192, 0);
	HuPrcDestructorSet2(shuffleboardProcess, (void *)ShuffleboardEventCleanup);
	while(shuffleboardProcess) {
		HuPrcVSleep();
	}
	BoardEventFlagReset();
}

s32 ShuffleboardEventCleanup(void)
{
	shuffleboardProcess = NULL;
}

void ShuffleboardPlayerLand(void);
void ShuffleboardItemsSpawn(void);
void ShuffleboardCartApproach(void);
void ShuffleboardPlayerRide(void);
void ShuffleboardPlayerMoveToSpace(s32 targetSpaceIndex);

void ShuffleboardEventProcess(void)
{
	s32 modelIndex;
	s32 spaceIndex;
	Vec cameraPosition;
	float targetPosition[2];
	if(shuffleboardDirection) {
		BoardModelVisibilitySet(rouletteGambleMarkerModelId, 0);
		BoardModelVisibilitySet(boardModelIds[13], 0);
		shuffleboardCartModelId = boardModelIds[14];
		shuffleboardPlayerShadowModelId = shuffleboardGambleMarkerModelId;
		cameraPosition.x = cameraPosition.z = 0;
		cameraPosition.y = -90;
		spaceIndex = BoardSpaceFlagSearch(0, 0x40);
	} else {
		BoardModelVisibilitySet(shuffleboardGambleMarkerModelId, 0);
		BoardModelVisibilitySet(boardModelIds[14], 0);
		shuffleboardCartModelId = boardModelIds[13];
		shuffleboardPlayerShadowModelId = rouletteGambleMarkerModelId;
		cameraPosition.x = cameraPosition.z = 0;
		cameraPosition.y = 90;
		spaceIndex = BoardSpaceFlagSearch(0, 0x100);
	}
	shuffleboardPlayerMotionIds[0] = BoardPlayerMotionCreate(shuffleboardPlayerIndex, shuffleboardModelData[GWPlayer[shuffleboardPlayerIndex].character]);
	shuffleboardPlayerMotionIds[1] = BoardPlayerMotionCreate(shuffleboardPlayerIndex, shuffleboardMotionData[GWPlayer[shuffleboardPlayerIndex].character]);
	ShuffleboardPlayerLand();
	BoardModelRotSetV(shuffleboardCartModelId, &cameraPosition);
	BoardCameraMoveSet(0);
	BoardCameraTargetPlayerSet(shuffleboardPlayerIndex);
	BoardCameraRotSet(-10, -cameraPosition.y);
	BoardCameraZoomSet(900);
	BoardStatusShowSetAll(0);
	HuWinMesMaxSizeGet(1, targetPosition, MAKE_MESSID(0x13, 0x1C));
	shuffleboardSpaceIndex = HuWinCreate(-10000, 400, targetPosition[0], targetPosition[1], 0);
	HuWinMesSet(shuffleboardSpaceIndex, MAKE_MESSID(0x13, 0x1C));
	HuWinBGTPLvlSet(shuffleboardSpaceIndex, 0);
	HuWinMesSpeedSet(shuffleboardSpaceIndex, 0);
	ShuffleboardItemsSpawn();
	BoardAudSeqPause(0, 1, 1000);
	HuPrcSleep(45);
	BoardCameraTargetGet(&shuffleboardCameraTarget);
	BoardCameraTargetModelSet(-1);
	ShuffleboardCartApproach();
	PlayerFXPlay(shuffleboardPlayerIndex, 291);
	ShuffleboardPlayerRide();
	HuPrcKill(shuffleboardItemSpinProcess);
	for(modelIndex=0; modelIndex<15; modelIndex++) {
		BoardModelKill(shuffleboardItemModelIds[modelIndex]);
		
	}
	BoardStatusShowSetAll(1);
	BoardCameraMoveSet(0);
	BoardCameraViewSet(1);
	BoardCameraMotionWait();
	BoardCameraMoveSet(1);
	BoardAudSeqPause(0, 0, 1000);
	ShuffleboardPlayerMoveToSpace(spaceIndex);
	BoardPlayerMotionKill(shuffleboardPlayerIndex, shuffleboardPlayerMotionIds[0]);
	BoardPlayerMotionKill(shuffleboardPlayerIndex, shuffleboardPlayerMotionIds[1]);
	HuPrcEnd();
}

void ShuffleboardPlayerLand(void)
{
	Vec playerPosition;
	Vec cameraPosition;
	float bounceFrame;
	float distanceToTarget;
	float approachAngle;
	float verticalVelocity;
	BoardModelPosGet(shuffleboardPlayerShadowModelId, &shuffleboardTargetPosition);
	BoardPlayerPosGet(shuffleboardPlayerIndex, &playerPosition);
	approachAngle = atan2d(shuffleboardTargetPosition.x-playerPosition.x, shuffleboardTargetPosition.z-playerPosition.z);
	BoardPlayerMotBlendSet(shuffleboardPlayerIndex, approachAngle, 15);
	while(!BoardPlayerMotBlendCheck(shuffleboardPlayerIndex)) {
		HuPrcVSleep();
	}
	BoardCameraTargetModelSet(-1);
	distanceToTarget = VECDistanceXZ(&shuffleboardTargetPosition, &playerPosition);
	VECSubtract(&shuffleboardTargetPosition, &playerPosition, &cameraPosition);
	VECNormalize(&cameraPosition, &cameraPosition);
	VECScale(&cameraPosition, &cameraPosition, distanceToTarget/45.0f);
	BoardPlayerMotionStart(shuffleboardPlayerIndex, 4, 0);
	
	bounceFrame = 0;
	verticalVelocity = 15;
	while(1) {
		cameraPosition.y = verticalVelocity-(0.075f*(0.25f*(bounceFrame*bounceFrame)));
		bounceFrame++;
		VECAdd(&playerPosition, &cameraPosition, &playerPosition);
		distanceToTarget = VECDistanceXZ(&shuffleboardTargetPosition, &playerPosition);
		if(distanceToTarget < 5.0f) {
			cameraPosition.x = cameraPosition.z = 0;
			playerPosition.x = shuffleboardTargetPosition.x;
			playerPosition.z = shuffleboardTargetPosition.z;
		}
		if(playerPosition.y <= shuffleboardTargetPosition.y+8.5f) {
			playerPosition.y = shuffleboardTargetPosition.y+8.5f;
			break;
		}
		BoardPlayerPosSetV(shuffleboardPlayerIndex, &playerPosition);
		HuPrcVSleep();
	}
	BoardPlayerPosSetV(shuffleboardPlayerIndex, &playerPosition);
	BoardPlayerIdleSet(shuffleboardPlayerIndex);
	HuPrcSleep(8);
	if(shuffleboardDirection) {
		BoardPlayerMotBlendSet(shuffleboardPlayerIndex, -90, 15);
	} else {
		BoardPlayerMotBlendSet(shuffleboardPlayerIndex, 90, 15);
	}
	while(!BoardPlayerMotBlendCheck(shuffleboardPlayerIndex)) {
		HuPrcVSleep();
	}
	BoardPlayerMotionShiftSet(shuffleboardPlayerIndex, shuffleboardPlayerMotionIds[0], 0, 10, HU3D_MOTATTR_LOOP);
	HuPrcSleep(20);
	
}

void ShuffleboardItemsSpin(void);

void ShuffleboardItemsSpawn(void)
{
	float distance;
	float speed;
	float angle;
	float rotationSpeed;
	float bounceFrame;
	
	s32 modelPositions[15];
	float playerPositions[15];
	float targetPosition[15];
	s32 modelIndex;
	s32 spaceIndex;
	for(modelIndex=0; modelIndex<15; modelIndex++) {
		shuffleboardItemModelIds[modelIndex] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 0x0A), NULL, 1);
		BoardModelVisibilitySet(shuffleboardItemModelIds[modelIndex], 0);
		shuffleboardItemVisible[modelIndex] = 1;
	}
	distance = ((1.0f/255.0f)*(90.0f*frand8()))+45;
	if(frandBool()) {
		distance += 180.0f;
	}
	if(shuffleboardDirection) {
		speed = 1430;
		angle = -1730;
	} else {
		speed = -1730;
		angle = 1430;
	}
	bounceFrame = (angle-speed)/15.0f;
	for(modelIndex=0; modelIndex<15; modelIndex++) {
		shuffleboardItemPositions[modelIndex].x = speed+(bounceFrame*modelIndex);
		shuffleboardItemPositions[modelIndex].z = -2100+(190*sind(distance));
		shuffleboardItemPositions[modelIndex].y = (500.0f+(50.0f+shuffleboardTargetPosition.y))+(modelIndex*50)+(400.0f*((1.0f/255.0f)*frand8()));
		distance += 60.0f;
		shuffleboardItemRotations[modelIndex].x = shuffleboardItemRotations[modelIndex].z = 0;
		shuffleboardItemRotations[modelIndex].y = (360.0f*((1.0f/255.0f)*frand8()));
		BoardModelVisibilitySet(shuffleboardItemModelIds[modelIndex], 1);
		BoardModelPosSetV(shuffleboardItemModelIds[modelIndex], &shuffleboardItemPositions[modelIndex]);
		BoardModelRotSetV(shuffleboardItemModelIds[modelIndex], &shuffleboardItemRotations[modelIndex]);
	}
	shuffleboardItemSpinProcess = HuPrcCreate(ShuffleboardItemsSpin, 8196, 2048, 0);
	for(modelIndex=0; modelIndex<15; modelIndex++) {
		targetPosition[modelIndex] = playerPositions[modelIndex] = 0;
		modelPositions[modelIndex] = 1;
	}
	while(1) {
		spaceIndex = 0;
		for(modelIndex=0; modelIndex<15; modelIndex++) {
			if(modelPositions[modelIndex] == 1) {
				rotationSpeed = playerPositions[modelIndex]-((1.0f/15.0f)*(0.25f*(targetPosition[modelIndex]*targetPosition[modelIndex])));
				shuffleboardItemPositions[modelIndex].y += rotationSpeed;
				if(shuffleboardItemPositions[modelIndex].y <= 50.0f+shuffleboardTargetPosition.y) {
					shuffleboardItemPositions[modelIndex].y = 50.0f+shuffleboardTargetPosition.y;
					playerPositions[modelIndex] = -rotationSpeed*0.2f;
					targetPosition[modelIndex] = 0;
					if(playerPositions[modelIndex] < 5.0f) {
						playerPositions[modelIndex] = 0;
						modelPositions[modelIndex] = 0;
					}
					HuAudFXPlay(18);
				}
				BoardModelPosSetV(shuffleboardItemModelIds[modelIndex], &shuffleboardItemPositions[modelIndex]);
				targetPosition[modelIndex]++;
			} else {
				spaceIndex++;
			}
		}
		if(spaceIndex == 15) {
			break;
		}
		HuPrcVSleep();
	}
}

void ShuffleboardCartApproach(void)
{
	Vec modelPosition;
	Vec playerPosition;
	Vec modelRotation;
	Vec movementVector;
	float movementDistance;
	s32 stepIndex;
	BoardMusStart(1, 20, 127, 0);
	BoardModelPosGet(shuffleboardCartModelId, &modelPosition);
	BoardModelMotionStart(shuffleboardCartModelId, 2, 0x40000001);
	for(stepIndex=0; stepIndex<60; stepIndex++) {
		if(shuffleboardDirection) {
			modelPosition.x += 5.0f;
		} else {
			modelPosition.x -= 5.0f;
		}
		BoardModelPosSetV(shuffleboardCartModelId, &modelPosition);
		HuPrcVSleep();
	}
	BoardModelRotGet(shuffleboardCartModelId, &modelRotation);
	BoardPlayerPosGet(shuffleboardPlayerIndex, &playerPosition);
	VECSubtract(&playerPosition, &modelPosition, &movementVector);
	movementDistance = VECMag(&movementVector);
	modelRotation.y = atan2d(shuffleboardItemPositions[0].x-playerPosition.x, shuffleboardItemPositions[0].z-playerPosition.z);
	modelPosition.x = playerPosition.x+(movementDistance*sind(modelRotation.y-180.0f));
	modelPosition.z = playerPosition.z+(movementDistance*cosd(modelRotation.y-180.0f));
	BoardModelPosSetV(shuffleboardCartModelId, &modelPosition);
	BoardModelRotSetV(shuffleboardCartModelId, &modelRotation);
	shuffleboardPlayerAngle = modelRotation.y;
	HuPrcSleep(20);
	BoardModelMotionStart(shuffleboardCartModelId, 3, 0x40000001);
	BoardModelMotionSpeedSet(shuffleboardCartModelId, 4.0f);
	VECSubtract(&playerPosition, &modelPosition, &movementVector);
	movementDistance = ((VECMag(&movementVector)-50)-50)/20;
	VECNormalize(&movementVector, &movementVector);
	VECScale(&movementVector, &movementVector, movementDistance);
	for(stepIndex=0; stepIndex<20; stepIndex++) {
		VECAdd(&modelPosition, &movementVector, &modelPosition);
		BoardModelPosSetV(shuffleboardCartModelId, &modelPosition);
		HuPrcVSleep();
	}
	BoardModelMotionStart(shuffleboardCartModelId, 1, 0x40000001);
	BoardModelMotionSpeedSet(shuffleboardCartModelId, 1.0f);
	omVibrate(shuffleboardPlayerIndex, 12, 4, 2);
	HuAudFXPlay(1069);
}

void ShuffleboardComputerSteer(s8 *stickInput);

void ShuffleboardPlayerRide(void)
{
	Vec playerPosition;
	Vec playerRotation;
	Vec itemPosition;
	s8 stickInput;
	float movementSpeed;
	float rotationSpeed;
	float horizontalStartX;
	float horizontalEndX;
	float pathHalfWidth;
	s16 itemIndex;
	s8 crossedBoundary;
	s16 controllerPort;
	controllerPort = GWPlayer[shuffleboardPlayerIndex].port;
	if(shuffleboardDirection) {
		horizontalStartX = 2430;
		horizontalEndX = -1830;
	} else {
		horizontalStartX = -2730;
		horizontalEndX = 1530;
	}
	pathHalfWidth = (horizontalEndX-horizontalStartX)/2;
	while(1) {
		crossedBoundary = 0;
		BoardPlayerPosGet(shuffleboardPlayerIndex, &playerPosition);
		if(GWPlayer[shuffleboardPlayerIndex].com) {
			ShuffleboardComputerSteer(&stickInput);
		} else {
			stickInput = HuPadStkX[controllerPort];
		}
		rotationSpeed = -stickInput*0.025f;
		if(shuffleboardDirection) {
			if(shuffleboardPlayerAngle < -90.0f) {
				shuffleboardPlayerAngle += rotationSpeed;
				if(shuffleboardPlayerAngle < -165.0f) {
					shuffleboardPlayerAngle = -165.0f;
				}
				if(shuffleboardPlayerAngle > -105.0f) {
					shuffleboardPlayerAngle = -105.0f;
				}
			} else {
				shuffleboardPlayerAngle += rotationSpeed;
				if(shuffleboardPlayerAngle < -75.0f) {
					shuffleboardPlayerAngle = -75.0f;
				}
				if(shuffleboardPlayerAngle > -15.0f) {
					shuffleboardPlayerAngle = -15.0f;
				}
			}
		} else {
			if(shuffleboardPlayerAngle > 90.0f) {
				shuffleboardPlayerAngle += rotationSpeed;
				if(shuffleboardPlayerAngle > 165.0f) {
					shuffleboardPlayerAngle = 165.0f;
				}
				if(shuffleboardPlayerAngle < 105.0f) {
					shuffleboardPlayerAngle = 105.0f;
				}
			} else {
				shuffleboardPlayerAngle += rotationSpeed;
				if(shuffleboardPlayerAngle > 75.0f) {
					shuffleboardPlayerAngle = 75.0f;
				}
				if(shuffleboardPlayerAngle < 15.0f) {
					shuffleboardPlayerAngle = 15.0f;
				}
			}
		}
		movementSpeed = 4.0+(15.0*(1.0-fabs((playerPosition.x-(pathHalfWidth+horizontalStartX))/pathHalfWidth)));
		playerPosition.x += movementSpeed*sind(shuffleboardPlayerAngle);
		playerPosition.z += movementSpeed*cosd(shuffleboardPlayerAngle);
		if(shuffleboardDirection) {
			if(playerPosition.z-50 < -2340) {
				playerPosition.z = -2290;
				shuffleboardPlayerAngle = (-90-shuffleboardPlayerAngle)+-90;
				crossedBoundary = 1;
			} else if(playerPosition.z+50 > -1860) {
				shuffleboardPlayerAngle = (-90-shuffleboardPlayerAngle)+-90;
				playerPosition.z = -1910;
				crossedBoundary = 1;
			}
		} else {
			if(playerPosition.z-50 < -2340) {
				playerPosition.z = -2290;
				shuffleboardPlayerAngle = (90-shuffleboardPlayerAngle)+90;
				crossedBoundary = 1;
			} else if(playerPosition.z+50 > -1860) {
				shuffleboardPlayerAngle = (90-shuffleboardPlayerAngle)+90;
				playerPosition.z = -1910;
				crossedBoundary = 1;
			}
		}
		BoardPlayerPosSetV(shuffleboardPlayerIndex, &playerPosition);
		playerPosition.y -= 8.5f;
		BoardModelPosSetV(shuffleboardPlayerShadowModelId, &playerPosition);
		if(crossedBoundary) {
			HuAudFXPlay(1060);
		}
		BoardPlayerRotGet(shuffleboardPlayerIndex, &playerRotation);
		playerRotation.y += 1.25f*movementSpeed;
		if(playerRotation.y >= 360.0f) {
			playerRotation.y -= 360.0f;
		}
		BoardPlayerRotSetV(shuffleboardPlayerIndex, &playerRotation);
		BoardModelRotGet(shuffleboardPlayerShadowModelId, &playerRotation);
		playerRotation.y += 1.25f*movementSpeed;
		if(playerRotation.y >= 360.0f) {
			playerRotation.y -= 360.0f;
		}
		BoardModelRotSetV(shuffleboardPlayerShadowModelId, &playerRotation);
		for(itemIndex=0; itemIndex<15; itemIndex++) {
			if(shuffleboardItemVisible[itemIndex] == 0) {
				continue;
			}
			BoardModelPosGet(shuffleboardItemModelIds[itemIndex], &itemPosition);
			if(VECDistanceXZ(&itemPosition, &playerPosition) < 100.0f) {
				shuffleboardItemVisible[itemIndex] = 0;
				BoardModelVisibilitySet(shuffleboardItemModelIds[itemIndex], 0);
				CharEffectCoinGlowCreate(1, &itemPosition);
				BoardPlayerCoinsAdd(shuffleboardPlayerIndex, 1);
				HuAudFXPlay(7);
			}
		}
		shuffleboardCameraTarget.x = playerPosition.x;
		BoardCameraTargetSet(shuffleboardCameraTarget.x, shuffleboardCameraTarget.y, shuffleboardCameraTarget.z);
		if(movementSpeed <= 3.0f) {
			break;
		}
		HuPrcVSleep();
	}
	HuWinKill(shuffleboardSpaceIndex);
	BoardPlayerMotionStart(shuffleboardPlayerIndex, shuffleboardPlayerMotionIds[1], 0);
	BoardPlayerMotionEndWait(shuffleboardPlayerIndex);
	BoardPlayerMotionShiftSet(shuffleboardPlayerIndex, 6, 0, 15, HU3D_MOTATTR_LOOP);
	HuPrcSleep(40);
	BoardAudSeqFadeOut(1, 1000);
	HuPrcSleep(70);
	BoardPlayerMotBlendSet(shuffleboardPlayerIndex, 0, 1);
}

void ShuffleboardPlayerMoveToSpace(s32 targetSpaceIndex)
{
	float bounceFrame;
	float distanceToTarget;
	float verticalVelocity;
	Vec boardSpacePosition;
	Vec playerPosition;
	Vec movementStep;
	BoardSpacePosGet(0, targetSpaceIndex, &boardSpacePosition);
	BoardPlayerPosGet(shuffleboardPlayerIndex, &playerPosition);
	PlayerFXPlay(shuffleboardPlayerIndex, 290);
	distanceToTarget = VECDistanceXZ(&boardSpacePosition, &playerPosition);
	VECSubtract(&boardSpacePosition, &playerPosition, &movementStep);
	VECNormalize(&movementStep, &movementStep);
	VECScale(&movementStep, &movementStep, distanceToTarget/45.0f);
	BoardPlayerMotionStart(shuffleboardPlayerIndex, 4, 0);
	bounceFrame = 0;
	verticalVelocity = 15;
	while(1) {
		movementStep.y = verticalVelocity-(0.075f*(0.25f*(bounceFrame*bounceFrame)));
		bounceFrame++;
		VECAdd(&playerPosition, &movementStep, &playerPosition);
		distanceToTarget = VECDistanceXZ(&boardSpacePosition, &playerPosition);
		if(distanceToTarget < 5.0f) {
			movementStep.x = movementStep.z = 0;
			playerPosition.x = boardSpacePosition.x;
			playerPosition.z = boardSpacePosition.z;
		}
		if(playerPosition.y <= boardSpacePosition.y) {
			playerPosition.y = boardSpacePosition.y;
			break;
		}
		BoardPlayerPosSetV(shuffleboardPlayerIndex, &playerPosition);
		HuPrcVSleep();
	}
	BoardPlayerPosSetV(shuffleboardPlayerIndex, &playerPosition);
	GWPlayer[shuffleboardPlayerIndex].space_curr = targetSpaceIndex;
	BoardPlayerIdleSet(shuffleboardPlayerIndex);
	HuPrcSleep(8);
	HuPrcSleep(20);
}

void ShuffleboardComputerSteer(s8 *stickInput)
{
	float angle;
	float rotationSpeed;
	Vec targetOffset;
	s32 modelIndex;
	s32 playerIndex;
	*stickInput = 0;
	BoardPlayerPosGet(shuffleboardPlayerIndex, &targetOffset);
	angle = -1000;
	playerIndex = -1;
	for(modelIndex=0; modelIndex<15; modelIndex++) {
		if(shuffleboardItemVisible[modelIndex] == 0) {
			continue;
		}
		rotationSpeed = VECDistanceXZ(&targetOffset, &shuffleboardItemPositions[modelIndex]);
		if(rotationSpeed > angle) {
			angle = rotationSpeed;
			playerIndex = modelIndex;
		}
	}
	if(playerIndex >= 0) {
		if(targetOffset.z > shuffleboardItemPositions[modelIndex].z) {
			*stickInput = -70;
		} else {
			*stickInput = 70;
		}
		if(shuffleboardDirection) {
			*stickInput = -(*stickInput);
		}
	}
}

void ShuffleboardItemsSpin(void)
{
	s32 modelIndex;
	while(1) {
		for(modelIndex=0; modelIndex<15; modelIndex++) {
			if(shuffleboardItemVisible[modelIndex] == 0) {
				continue;
			}
			shuffleboardItemRotations[modelIndex].y += 10.0f;
			if(shuffleboardItemRotations[modelIndex].y >= 360.0f) {
				shuffleboardItemRotations[modelIndex].y -= 360.0f;
			}
			BoardModelRotSetV(shuffleboardItemModelIds[modelIndex], &shuffleboardItemRotations[modelIndex]);
		}
		HuPrcVSleep();
	}
	
}
