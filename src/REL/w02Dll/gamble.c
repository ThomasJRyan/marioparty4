#include "REL/w02Dll.h"
#include "game/audio.h"
#ifndef __MWERKS__
#include "game/frand.h"
#endif
#include "game/pad.h"
#include "game/objsub.h"

#include "game/board/basic_space.h"

s32 gambleSpaceIndex;
HUPROCESS* gambleEventProcess;

/**
 * @brief Starts the gamble minigame sequence
 * 
 * @details The player and Goomba roll dice, with the player winning coins or
 * being sent back to start based on the outcome.
 * 
 *   * Spawns the Goomba model
 *   * Plays the Goomba animation
 *   * Rolls dice for the Goomba with weighted probabilities
 *   * Rolls dice for the player
 *   * Compares the results and gives coins or sends player to start
 * 
 * @return void
 */
void GambleExec(void)
{
	float bounceFrame;
	float verticalDelta;
	float bounceVelocity;
	
	W02GenDice goombaDie;
	W02GenDice playerDie;
	s32 diceOptions[10] = {
		DATA_MAKE_NUM(DATADIR_W02, 0x1C),
		DATA_MAKE_NUM(DATADIR_W02, 0x1D),
		DATA_MAKE_NUM(DATADIR_W02, 0x1E),
		DATA_MAKE_NUM(DATADIR_W02, 0x1F),
		DATA_MAKE_NUM(DATADIR_W02, 0x20),
		DATA_MAKE_NUM(DATADIR_W02, 0x21),
		DATA_MAKE_NUM(DATADIR_W02, 0x22),
		DATA_MAKE_NUM(DATADIR_W02, 0x23),
		DATA_MAKE_NUM(DATADIR_W02, 0x24),
		DATA_MAKE_NUM(DATADIR_W02, 0x25)
	};
	Vec goombaPosition;
	Vec playerPosition;
	BoardSpace *playerBoardSpace;	// Seemingly goes unused?
	s32 currPlayer;
	s16 goombaModelId; // ? Is this the Goomba model hook? Not sure what to name it
	s32 eventScratchValue; // Used for various temporary storage, sleep times, for loops, etc
	s16 rewardModelId;
	s32 goombaDieResult;
	s32 playerBoardSpaceID;
	s32 randomNumber;
	u16 buttonState;
	currPlayer = GWSystem.player_curr;
	OSReport("gamble start %d\n", currPlayer);
	playerBoardSpace = BoardSpaceGet(0, GWPlayer[currPlayer].space_curr);
	BoardRollDispSet(0);
	rewardModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W02, 0x0F), NULL, 0);
	BoardModelVisibilitySet(rewardModelId, 0);

	// Determine which gamble space model to use
	switch(gambleSpaceIndex) {
		case 0:
			goombaModelId = boardModelIds[0];
			break;
			
		case 1:
			goombaModelId = boardModelIds[1];
			break;
			
		case 2:
			goombaModelId = boardModelIds[2];
			break;
	}
	
	BoardPlayerIdleSet(currPlayer);
	BoardPlayerMotBlendSet(currPlayer, -90, 30);
	while(!BoardPlayerMotBlendCheck(currPlayer)) {
		HuPrcVSleep();
	}
	BoardCameraViewSet(3);
	BoardCameraMotionWait();
	BoardModelMotionStart(goombaHostModelId, 1, 0x40000001);
	BoardModelHookSet(goombaModelId, "kuri", goombaHostModelId);
	BoardModelVisibilitySet(goombaHostModelId, 1);
	BoardModelMotionTimeSet(goombaModelId, 0);
	BoardModelAttrReset(goombaModelId, 0x40000002);
	HuAudFXPlay(813);
	while(BoardModelMotionTimeGet(goombaModelId) < 30.0f) {
		HuPrcVSleep();
	}
	BoardModelAttrSet(goombaModelId, 0x40000002);
	BoardModelHookReset(goombaModelId);
	BoardModelPosGet(goombaModelId, &goombaPosition);
	BoardModelPosSetV(goombaHostModelId, &goombaPosition);
	W02MesExec(MAKE_MESSID(0x13, 0x0A));
	goombaDie.dieCount = 1;
	goombaDie.dieModelData = DATA_MAKE_NUM(DATADIR_W02, 0x1B);
	goombaDie.dieFaceData = diceOptions;
	goombaDie.spawnPosition = goombaPosition;
	goombaDie.minimumRoll = 1;
	goombaDie.maximumRoll = 10;
	goombaDie.displayRollTotal = 0;
	goombaDie.dieVerticalSpacing = 1;

	// Generate dice roll with weighted probabilities for the Goomba
	// 60% chance for 4-7
	// 30% chance for 2,3,8,9
	// 10% chance for 1,10
	randomNumber = frandmod(100);
	if(randomNumber <= 59) {
		goombaDie.dieResults[0] = frandmod(4)+4;
	} 
	else if(randomNumber <= 89) {
		if(randomNumber & 0x1) {
			goombaDie.dieResults[0] = 2;
		} 
		else {
			goombaDie.dieResults[0] = 8;
		}
		goombaDie.dieResults[0] += frand() & 0x1;
	} 
	else {
		if(randomNumber & 0x1) {
			goombaDie.dieResults[0] = 1;
		} else {
			goombaDie.dieResults[0] = 10;
		}
	}

	// Start dice roll
	GenDiceCreate(&goombaDie);
	while(!GenDiceAnimationCheck(&goombaDie)) {
		HuPrcVSleep();
	}

	// Add delay before dice is hit, then move the Goomba model
	eventScratchValue = frandmod(45)+45;
	HuPrcSleep(eventScratchValue);
	BoardModelPosGet(goombaHostModelId, &playerPosition);
	BoardModelMotionStart(goombaHostModelId, 4, 0);
	bounceVelocity = 15;
	bounceFrame = 0;
	while(1) {
		verticalDelta = bounceVelocity-(0.08166667f*(0.25f*(bounceFrame*bounceFrame)));
		bounceFrame++;
		playerPosition.y += verticalDelta;
		if(playerPosition.y >= (250.0f+goombaPosition.y)-130.0f) {
			playerPosition.y = (250.0f+goombaPosition.y)-130.0f;
			bounceVelocity = -10;
			bounceFrame = 0;
			goombaDie.dieAnimationComplete = 1;
		}
		if(playerPosition.y <= goombaPosition.y) {
			playerPosition.y = goombaPosition.y;
			goombaDie.dieAnimationComplete = 0;
			break;
		}
		BoardModelPosSetV(goombaHostModelId, &playerPosition);
		HuPrcVSleep();
	}
	BoardModelPosSetV(goombaHostModelId, &goombaPosition);
	BoardModelMotionStart(goombaHostModelId, 1, 0x40000001);
	while(goombaDie.rollComplete == 0) {
		HuPrcVSleep();
	}
	goombaDieResult = goombaDie.totalRollResult;

	// Announce Goomba's roll, if 10 then play player's disappointment sound
	if(goombaDieResult == 10) {
		PlayerFXPlay(currPlayer, 302);
		W02MesExec(MAKE_MESSID(0x13, 0x10));
	} else {
		W02MesExec(MAKE_MESSID(0x13, 0x0C));
	}

	BoardPlayerMotBlendSet(currPlayer, 0, 15);
	while(!BoardPlayerMotBlendCheck(currPlayer)) {
		HuPrcVSleep();
	}
	BoardCameraTargetModelSet(-1);
	if(goombaDieResult != 10) {
		playerDie = goombaDie; // Save on resources, just use Goomba's die
		BoardPlayerPosGet(currPlayer, &playerDie.spawnPosition);
		playerDie.dieVerticalSpacing = 1;

		// Generate player's die roll, ensuring it is not the same as Goomba's
		playerDie.dieResults[0] = frandmod(10)+1;
		if(playerDie.dieResults[0] == goombaDieResult) {
			if(playerDie.dieResults[0] == 9) {
				playerDie.dieResults[0]--; // Player loses if both roll 9
			} else {
				playerDie.dieResults[0]++; // Player wins if both roll 1-8
			}
		}
		
		GenDiceCreate(&playerDie);
		while(!GenDiceAnimationCheck(&playerDie)) {
			HuPrcVSleep();
		}
		
		// Handle player/CPU input to hit die
		buttonState = 0;
		while(!(buttonState & PAD_BUTTON_A)) {
			HuPrcVSleep();

			// If CPU player, simulate button press after short delay
			if(GWPlayer[currPlayer].com) {
				eventScratchValue = frandmod(45)+20;
				HuPrcSleep(eventScratchValue);
				buttonState = PAD_BUTTON_A;
			} 
			// Wait for player to press A button
			else {
				// Get the state of the player's buttons
				buttonState = HuPadBtnDown[GWPlayer[currPlayer].port];
			}
		}

		// Perform player jump animation to hit die
		BoardPlayerDiceJumpStart(currPlayer);
		while(!BoardPlayerDiceJumpCheck(currPlayer)) {
			HuPrcVSleep();
		}

		playerDie.dieAnimationComplete = 1;
		while(GWPlayer[currPlayer].jump) {
			HuPrcVSleep();
		}
		playerDie.dieAnimationComplete = 0;
		while(playerDie.rollComplete == 0) {
			HuPrcVSleep();
		}

		// Announce player's roll and outcome, extra voice line if player loses
		if(playerDie.totalRollResult > goombaDieResult) {
			W02MesExec(MAKE_MESSID(0x13, 0x0D));
		} else {
			PlayerFXPlay(currPlayer, 302);
			W02MesExec(MAKE_MESSID(0x13, 0x0F));
		}
	} else {
		playerDie.totalRollResult = 0;
	}
	GenDiceDestroy(&goombaDie);
	if(goombaDieResult != 10) {
		GenDiceDestroy(&playerDie);
	}
	BoardPlayerPosGet(currPlayer, &playerPosition);

	// If the player wins, award 10 coins
	if(playerDie.totalRollResult > goombaDieResult) {
		playerPosition.y += 250.0f;
		HuAudFXPlay(839);
		eventScratchValue = BoardCoinChgCreate(&playerPosition, 10);
		while(!BoardCoinChgKillCheck(eventScratchValue)) {
			HuPrcVSleep();
		}
		for(eventScratchValue=0; eventScratchValue<10; eventScratchValue++) {
			BoardPlayerCoinsAdd(currPlayer, 1);
			HuAudFXPlay(7);
			HuPrcSleep(6);
		}
		W02MesExec(MAKE_MESSID(0x13, 0x0E));
	} 
	// If the Goomba wins, send the player back to start
	else {
		playerBoardSpaceID = GWPlayer[currPlayer].space_curr;
		BoardCameraTargetSpaceSet(playerBoardSpaceID);
		BoardPlayerMotionShiftSet(currPlayer, 6, 0, 5, HU3D_MOTATTR_LOOP);
		HuPrcSleep(60);
		HuPrcSleep(30);
		BoardModelPosSetV(rewardModelId, &playerPosition);
		BoardModelMotionTimeSet(rewardModelId, 0);
		BoardModelVisibilitySet(rewardModelId, 1);
		HuAudFXPlay(1058);
		while(BoardModelMotionTimeGet(rewardModelId) < BoardModelMotionMaxTimeGet(rewardModelId)) {
			HuPrcVSleep();
		}
		omVibrate(currPlayer, 12, 4, 2);
		verticalDelta = -4;
		for(eventScratchValue=0; eventScratchValue<30; eventScratchValue++) {
			playerPosition.y += verticalDelta;
			verticalDelta *= 1.08f;
			BoardPlayerPosSetV(currPlayer, &playerPosition);
			HuPrcVSleep();
		}
		playerBoardSpaceID = BoardSpaceFlagSearch(0, 0x80000000);
		BoardCameraMoveSet(0);
		BoardCameraTargetSpaceSet(playerBoardSpaceID);
		HuPrcVSleep();
		BoardCameraMoveSet(1);
		BoardSpacePosGet(0, playerBoardSpaceID, &goombaPosition);
		playerPosition = goombaPosition;
		playerPosition.y -= 180.0f;
		BoardPlayerPosSetV(currPlayer, &playerPosition);
		GWPlayer[currPlayer].space_curr = playerBoardSpaceID;
		HuPrcSleep(15);
		BoardModelPosSetV(rewardModelId, &goombaPosition);
		BoardModelMotionTimeSet(rewardModelId, 0);
		HuAudFXPlay(1058);
		while(BoardModelMotionTimeGet(rewardModelId) < BoardModelMotionMaxTimeGet(rewardModelId)) {
			HuPrcVSleep();
		}
		HuPrcSleep(15);
		bounceVelocity = 22;
		bounceFrame = 1;
		while(1) {
			verticalDelta = bounceVelocity-((90.0f/1200.0f)*(0.25f*(bounceFrame*bounceFrame)));
			bounceFrame++;
			playerPosition.y += verticalDelta;
			if(playerPosition.y >= goombaPosition.y) {
				BoardModelAttrSet(rewardModelId, 0x40000004);
			}
			if(verticalDelta < 0 && playerPosition.y <= goombaPosition.y) {
				omVibrate(currPlayer, 12, 4, 2);
				bounceVelocity = -verticalDelta*0.31f;
				bounceFrame = 1;
				HuAudFXPlay(1068);
				if(fabs(bounceVelocity) <= 5.0) {
					break;
				}
				playerPosition.y = goombaPosition.y;
			}
			
			BoardPlayerPosSetV(currPlayer, &playerPosition);
			HuPrcVSleep();
		}
		BoardPlayerPosSetV(currPlayer, &goombaPosition);
		HuPrcSleep(90);
		BoardPlayerIdleSet(currPlayer);
		HuPrcSleep(9);
		BoardCameraTargetPlayerSet(currPlayer);
	}
	BoardModelHookSet(goombaModelId, "kuri", goombaHostModelId);
	BoardModelAttrReset(goombaModelId, 0x40000002);
	if(playerDie.totalRollResult > goombaDieResult) {
		HuPrcSleep(10);
		HuAudFXPlay(815);
	}
	while(BoardModelMotionTimeGet(goombaModelId) < BoardModelMotionMaxTimeGet(goombaModelId)) {
		HuPrcVSleep();
	}
	BoardModelHookReset(goombaModelId);
	BoardCameraViewSet(1);
	BoardCameraMotionWait();
	BoardRollDispSet(1);
	BoardModelVisibilitySet(goombaHostModelId, 0);
	BoardModelKill(rewardModelId);
	HuPrcKill(NULL);
	while(1) {
		HuPrcVSleep();
	}
}

/**
 * @brief Removes the child process reference for the gamble minigame.
 */
void GambleDestroy(void) {
    gambleEventProcess = NULL;
}

/**
 * @brief Bootstraps the gamble board minigame.
 * 
 * @details This function handles the initialization and execution of the 
 * gamble board minigame. It handles creating the child process that runs
 * the minigame logic, and waits for its completion before terminating and
 * returning.
 * 
 * @param actorIndex The index of the actor model (Gambling Goomba) to be used.
 */
void GambleMain(s32 actorIndex) {
    s32 currPlayer;
    currPlayer = GWSystem.player_curr; // Unused?

    gambleSpaceIndex = actorIndex;
    gambleEventProcess = HuPrcChildCreate(GambleExec, 0x2003U, 0x2000U, 0, boardMainProc);
    HuPrcDestructorSet2(gambleEventProcess, GambleDestroy);
    while (gambleEventProcess) {
        HuPrcVSleep();
    }
}
