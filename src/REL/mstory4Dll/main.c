#include "game/data.h"
#include "game/process.h"
#include "game/printfunc.h"
#include "game/pad.h"
#include "game/object.h"
#include "game/armem.h"
#include "game/wipe.h"
#include "game/gamework_data.h"
#include "game/flag.h"
#include "game/chrman.h"
#include "math.h"
#include "REL/executor.h"
#include "game/board/main.h"

#ifndef __MWERKS__
#include "game/objsub.h"
#endif

typedef struct StoryItemResourceEntry {
	s32 dataNum;
	s32 resourceMode;
	s32 resourceState;
	s32 resourceFlags;
	s32 highlightName;
} StoryItemResourceEntry;

StoryItemResourceEntry storyItemResourceTable[] = {
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x00), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x01), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x02), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x03), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x04), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x05), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x06), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x07), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x08), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x09), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x0A), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x0B), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x0C), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x0D), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x0E), 1, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x0F), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x10), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x11), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x12), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x13), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x14), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x15), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x16), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x17), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x18), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x19), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x1A), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x1B), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x1C), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x1D), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x1E), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x1F), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x20), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x21), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x22), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x23), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x24), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x25), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x26), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x27), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x28), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x29), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x2A), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x2B), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x2C), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x2D), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x2E), 0, 0, 0, 1 },
	{ DATA_MAKE_NUM(DATADIR_MSTORY4, 0x2F), 0, 0, 0, 1 },
};

char *boardNameTable[] = {
	"Map1:Fun Party",
	"Map2:Exciting Party",
	"Map3:Adventure Party",
	"Map4:Scary Party",
	"Map5:Resort Party",
	"Map6:Bowser Party"
};

char *storyEventNameTable[] = {
#if VERSION_PAL
	"(Map1-Map6) >>>> not event <<<<",
#else
	"(Map1-Map6) Board Game",
#endif
	"(Map1-Map6) Story:Board Clear Event",
	"(Map1-Map6) Story:Board Miss Event",
	"(Map1-Map6) Story:MiniGame Clear Event", 
	"(Map1-Map6) Story:MiniGame Miss Event",
	"(Map6)      Story:BowserParty Inst Event",
	"(Map6)      Story:Ending"
};

#if VERSION_PAL
char *languageNameTable[] = {
	"GERMANY",
	"FRENCH",
	"ITALY",
	"SPANISH"
};
#endif

#if VERSION_PAL
s32 selectedStoryEventIndex = 1;
#endif

s32 boardClearFlags[] = {
	0, 0, 0, 0, 0, 0,
};

s32 boardDataDirectoryTable[] = {
	DATADIR_W01,
	DATADIR_W02,
	DATADIR_W03,
	DATADIR_W04,
	DATADIR_W05,
	DATADIR_W06,
	DATADIR_W10
};

#if VERSION_PAL

#endif

void StoryMapEventSelectionProcess(void);
void InitializeStorySelection(void);
void ItemDebugSelectionProcess(void);

s32 boardDataReadComplete;
s32 selectedSubmenuItemIndex;
#if VERSION_NTSC
s32 selectedStoryEventIndex;
#endif
s32 selectedBoardIndex;
s32 selectionMode;
HUPROCESS *storySelectionObjectManager;

void LoadStoryBoardDataProcess(void)
{
	s32 asyncReadStatus = HuDataDirReadAsync(DATADIR_BOARD);
	if(asyncReadStatus != -1) {
		while(!HuDataGetAsyncStat(asyncReadStatus)) {
			HuPrcVSleep();
		}
	}
	HuAR_MRAMtoARAM(DATADIR_BOARD);
	while(HuARDMACheck()) {
		HuPrcVSleep();
	}
	HuDataDirClose(DATADIR_BOARD);
	if(_CheckFlag(0x1000B)) {
		asyncReadStatus = HuDataDirReadAsync(DATADIR_W10);
	} else {
		asyncReadStatus = HuDataDirReadAsync(boardDataDirectoryTable[GWSystem.board]);
	}
	if(asyncReadStatus != -1) {
		while(!HuDataGetAsyncStat(asyncReadStatus)) {
			HuPrcVSleep();
		}
	}
	boardDataReadComplete = 1;
	HuPrcEnd();
	while(1) {
		HuPrcVSleep();
	}
}

void StoryMapEventSelectionProcess(void)
{
	s32 itemIndex;
	s32 textLeft = 16;
	s32 textTop = 40;
	s32 textLineHeight = 10;
	s32 inputRepeatDelay = 0;
	float textScale = 1.0f;
	GXColor selectionHighlightColor = { 0, 0, 255, 128 };
	OMOVL boardOverlayTable[] = {
		DLL_w01dll,
		DLL_w02dll,
		DLL_w03dll,
		DLL_w04dll,
		DLL_w05dll,
		DLL_w06dll,
		DLL_w10dll
	};
	WipeColorSet(255, 255, 255);
	WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
	while(1) {
		if(!WipeStatGet()) {
			break;
		}
		HuPrcVSleep();
	}
	while(1) {
		HuPrcVSleep();
		fontcolor = FONT_COLOR_WHITE;
		if(!selectionMode) {
			printWin(textLeft,textTop+(textLineHeight*(selectedBoardIndex+2))-2, 400, 10*textScale, &selectionHighlightColor);
		}
		print8(textLeft, textTop, textScale, "MAP NO");
		print8(textLeft, textTop+textLineHeight, textScale, "--------------------------------------------------");
		for(itemIndex=0; itemIndex<6; itemIndex++) {
			if(!selectionMode) {
				print8(textLeft, textTop+(textLineHeight*(itemIndex+2)), textScale, "%s", boardNameTable[itemIndex]);
			} else if(selectedBoardIndex == itemIndex) {
				print8(textLeft, textTop+(textLineHeight*(itemIndex+2)), textScale, "%s", boardNameTable[itemIndex]);
			}
		}
		if(selectionMode == 1) {
			printWin(textLeft, textTop+(textLineHeight*(selectedStoryEventIndex+12))-2, 400, 10*textScale, &selectionHighlightColor);
		}
		print8(textLeft, textTop+(textLineHeight*10), textScale, "EVENT NO");
		print8(textLeft, textTop+(textLineHeight*11), textScale, "--------------------------------------------------");
		if(selectedBoardIndex == 5) {
			for(itemIndex=0; itemIndex<7; itemIndex++) {
				if(selectionMode <= 1) {
					print8(textLeft, textTop+(textLineHeight*(itemIndex+12)), textScale, "%s", storyEventNameTable[itemIndex]);
				} else if(selectedStoryEventIndex == itemIndex) {
					print8(textLeft, textTop+(textLineHeight*(itemIndex+12)), textScale, "%s", storyEventNameTable[itemIndex]);
				}
			}
		} else {
			for(itemIndex=0; itemIndex<5; itemIndex++) {
				if(selectionMode <= 1) {
					print8(textLeft, textTop+(textLineHeight*(itemIndex+12)), textScale, "%s", storyEventNameTable[itemIndex]);
				} else if(selectedStoryEventIndex == itemIndex) {
					print8(textLeft, textTop+(textLineHeight*(itemIndex+12)), textScale, "%s", storyEventNameTable[itemIndex]);
				}
			}
		}
		if(selectionMode == 2) {
			printWin(textLeft, textTop+(textLineHeight*(selectedSubmenuItemIndex+23))-2, 400, 10*textScale, &selectionHighlightColor);
		}
#if VERSION_PAL
		print8(textLeft, textTop+(textLineHeight*21), textScale, "LANGUAGE SELECT");
		print8(textLeft, textTop+(textLineHeight*22), textScale, "--------------------------------------------------");
		for(itemIndex=0; itemIndex<4; itemIndex++) {
			print8(textLeft, textTop+(textLineHeight*(itemIndex+23)), textScale, "%s", languageNameTable[itemIndex]);
		}
#endif
#if VERSION_NTSC
		print8(textLeft, textTop+(textLineHeight*21), textScale, "CLEAR FLAG");
		print8(textLeft, textTop+(textLineHeight*22), textScale, "--------------------------------------------------");
		for(itemIndex=0; itemIndex<6; itemIndex++) {
			if(!boardClearFlags[itemIndex]) {
				print8(textLeft, textTop+(textLineHeight*(itemIndex+23)), textScale, "FALSE - %s", boardNameTable[itemIndex]);
			} else {
				print8(textLeft, textTop+(textLineHeight*(itemIndex+23)), textScale, "TRUE  - %s", boardNameTable[itemIndex]);
			}
		}
#endif
		if(inputRepeatDelay <= 0) {
			switch(selectionMode) {
				case 0:
					if(HuPadStkY[0] <= -30) {
						selectedBoardIndex++;
						if(selectedBoardIndex > 5) {
							selectedBoardIndex = 0;
						}
						inputRepeatDelay = 10;
					} else if(HuPadStkY[0] >= 30) {
						selectedBoardIndex--;
						if(selectedBoardIndex < 0) {
							selectedBoardIndex = 5;
						}
						inputRepeatDelay = 10;
					}
					break;
					
				case 1:
					if(selectedBoardIndex == 5) {
						if(HuPadStkY[0] >= 30) {
							selectedStoryEventIndex--;
#if VERSION_PAL
							if(selectedStoryEventIndex < 1) {
#else
							if(selectedStoryEventIndex < 0) {
#endif
								selectedStoryEventIndex = 6;
							}
							inputRepeatDelay = 10;
						} else if(HuPadStkY[0] <= -30) {
							selectedStoryEventIndex++;
							if(selectedStoryEventIndex > 6) {
#if VERSION_PAL
								selectedStoryEventIndex = 1;
#else
								selectedStoryEventIndex = 0;
#endif
							}
							inputRepeatDelay = 10;
						}
					} else {
						if(HuPadStkY[0] >= 30) {
							selectedStoryEventIndex--;
#if VERSION_PAL
							if(selectedStoryEventIndex < 1) {
#else
							if(selectedStoryEventIndex < 0) {
#endif
								selectedStoryEventIndex = 4;
							}
							inputRepeatDelay = 10;
						} else if(HuPadStkY[0] <= -30) {
							selectedStoryEventIndex++;
							if(selectedStoryEventIndex > 4) {
#if VERSION_PAL
								selectedStoryEventIndex = 1;
#else
								selectedStoryEventIndex = 0;
#endif
							}
							inputRepeatDelay = 10;
						}
					}
					break;
				case 2:
					if(HuPadStkY[0] >= 30) {
						selectedSubmenuItemIndex--;
						if(selectedSubmenuItemIndex < 0) {
#if VERSION_PAL
							selectedSubmenuItemIndex = 3;
#else
							selectedSubmenuItemIndex = 5;
#endif
						}
						inputRepeatDelay = 10;
					} else if(HuPadStkY[0] <= -30) {
						selectedSubmenuItemIndex++;
#if VERSION_PAL
						if(selectedSubmenuItemIndex > 3) {
#else
						if(selectedSubmenuItemIndex > 5) {
#endif
							selectedSubmenuItemIndex = 0;
						}
						inputRepeatDelay = 10;
#if VERSION_NTSC
					} else if(HuPadStkX[0] <= -30) {
						boardClearFlags[selectedSubmenuItemIndex]++;
						if(boardClearFlags[selectedSubmenuItemIndex] > 1) {
							boardClearFlags[selectedSubmenuItemIndex] = 0;
						}
						inputRepeatDelay = 10;
					} else if(HuPadStkX[0] >= 30) {
						boardClearFlags[selectedSubmenuItemIndex]--;
						if(boardClearFlags[selectedSubmenuItemIndex] < 0) {
							boardClearFlags[selectedSubmenuItemIndex] = 1;
						}
						inputRepeatDelay = 10;
#endif
					}
					break;
					
				default:
					break;
			}
			if(selectedBoardIndex != 5 && selectedStoryEventIndex > 4) {
				selectedStoryEventIndex = 4;
			}
			if(HuPadBtnDown[0] & PAD_BUTTON_A) {
				selectionMode++;
				if(selectionMode == 3) {
					break;
				}
			} else if(HuPadBtnDown[0] & PAD_BUTTON_B) {
				selectionMode--;
				if(selectionMode < 0) {
					selectionMode = 0;
				}
			}
			
		} else {
			inputRepeatDelay--;
			continue;
		}
	}
	WipeColorSet(255, 255, 255);
	WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
	while(1) {
		if(!WipeStatGet()) {
			break;
		}
		HuPrcVSleep();
	}
	GWSystem.board = selectedBoardIndex;
#if VERSION_NTSC
	if(boardClearFlags[0] == 1 || selectedBoardIndex == 0 && selectedStoryEventIndex == 1) {
		_SetFlag(FLAG_ID_MAKE(0, 2));
	} else {
		_ClearFlag(FLAG_ID_MAKE(0, 2));
	}
	if(boardClearFlags[1] == 1 || selectedBoardIndex == 1 && selectedStoryEventIndex == 1) {
		_SetFlag(FLAG_ID_MAKE(0, 3));
	} else {
		_ClearFlag(FLAG_ID_MAKE(0, 3));
	}
	if(boardClearFlags[2] == 1 || selectedBoardIndex == 2 && selectedStoryEventIndex == 1) {
		_SetFlag(FLAG_ID_MAKE(0, 4));
	} else {
		_ClearFlag(FLAG_ID_MAKE(0, 4));
	}
	if(boardClearFlags[3] == 1 || selectedBoardIndex == 3 && selectedStoryEventIndex == 1) {
		_SetFlag(FLAG_ID_MAKE(0, 5));
	} else {
		_ClearFlag(FLAG_ID_MAKE(0, 5));
	}
	if(boardClearFlags[4] == 1 || selectedBoardIndex == 4 && selectedStoryEventIndex == 1) {
		_SetFlag(FLAG_ID_MAKE(0, 6));
	} else {
		_ClearFlag(FLAG_ID_MAKE(0, 6));
	}
	if(boardClearFlags[5] == 1 || selectedBoardIndex == 5 && selectedStoryEventIndex == 1) {
		_SetFlag(FLAG_ID_MAKE(0, 7));
	} else {
		_ClearFlag(FLAG_ID_MAKE(0, 7));
	}
#endif
#if VERSION_PAL
	switch (selectedSubmenuItemIndex) {
		case 0:
			GWGameStat.language = 2;
			break;
		case 1:
			GWGameStat.language = 3;
			break;
		case 2:
			GWGameStat.language = 4;
			break;
		case 3:
			GWGameStat.language = 5;
	}
#endif
	if(GWSystem.board == BOARD_ID_MAIN6) {
		_SetFlag(FLAG_ID_MAKE(0, 2));
		_SetFlag(FLAG_ID_MAKE(0, 3));
		_SetFlag(FLAG_ID_MAKE(0, 4));
		_SetFlag(FLAG_ID_MAKE(0, 5));
		_SetFlag(FLAG_ID_MAKE(0, 6));
		switch(selectedStoryEventIndex) {
			case 0:
				HuPrcChildCreate(LoadStoryBoardDataProcess, 100, 12288, 0, storySelectionObjectManager);
				do {
					HuPrcVSleep();
				} while(boardDataReadComplete != 1);
				CharDataClose(-1);
				CharMotionInit(GWPlayerCfg[0].character);
				CharMotionInit(GWPlayerCfg[1].character);
				CharMotionInit(GWPlayerCfg[2].character);
				CharMotionInit(GWPlayerCfg[3].character);
				{
					OMOVLHIS *overlayHistory = omOvlHisGet(0);
					omOvlHisChg(0, DLL_mstory3dll, 0, 0);
					omOvlCallEx(boardOverlayTable[GWSystem.board], 1, 0, 0);
				}
				break;
				
			case 1:
				omOvlGotoEx(DLL_mstory2dll, 1, 1, 0);
				break;
				
			case 2:
				omOvlGotoEx(DLL_mstory2dll, 1, 2, 0);
				break;
				
			case 3:
				GWPlayerCoinWinSet(0, 10);
				omOvlGotoEx(DLL_mstory2dll, 1, 3, 0);
				break;
				
			case 4:
				GWPlayerCoinWinSet(0, 0);
				omOvlGotoEx(DLL_mstory2dll, 1, 3, 0);
				break;
				
			case 5:
				omOvlGotoEx(DLL_mstory2dll, 1, 0, 0);
				break;
				
			case 6:
				omOvlGotoEx(DLL_mstory2dll, 1, 4, 0);
				break;
		}
	} else {
		switch(selectedStoryEventIndex) {
			case 0:
				HuPrcChildCreate(LoadStoryBoardDataProcess, 100, 12288, 0, storySelectionObjectManager);
				do {
					HuPrcVSleep();
				} while(boardDataReadComplete != 1);
				CharDataClose(-1);
				CharMotionInit(GWPlayerCfg[0].character);
				CharMotionInit(GWPlayerCfg[1].character);
				CharMotionInit(GWPlayerCfg[2].character);
				CharMotionInit(GWPlayerCfg[3].character);
				{
					OMOVLHIS *overlayHistory = omOvlHisGet(0);
					omOvlHisChg(0, DLL_mstory3dll, 0, 0);
					omOvlCallEx(boardOverlayTable[GWSystem.board], 1, 0, 0);
				}
				break;
				
			case 1:
				omOvlGotoEx(DLL_mstorydll, 1, 0, 0);
				break;
				
			case 2:
				omOvlGotoEx(DLL_mstorydll, 1, 1, 0);
				break;
			
			case 3:
				GWPlayerCoinWinSet(0, 10);
				omOvlGotoEx(DLL_mstorydll, 1, 2, 0);
				break;
				
			case 4:
				GWPlayerCoinWinSet(0, 0);
				omOvlGotoEx(DLL_mstorydll, 1, 2, 0);
				break;
		}
	}
	HuPrcEnd();
	while(1) {
		HuPrcVSleep();
	}
}

s32 Prolog(void) {
    const VoidFunc* constructorFunction = _ctors;
    while (*constructorFunction != 0) {
        (**constructorFunction)();
        constructorFunction++;
    }
    InitializeStorySelection();
    return 0;
}

void Epilog(void) {
    const VoidFunc* destructorFunction = _dtors;
    while (*destructorFunction != 0) {
        (**destructorFunction)();
        destructorFunction++;
    }
}

void InitializeStorySelection(void)
{
	storySelectionObjectManager = omInitObjMan(62, 8192);
	omGameSysInit(storySelectionObjectManager);
	if((HuPadBtn[0] & PAD_TRIGGER_L) && (HuPadBtn[0] & PAD_TRIGGER_R)) {
		HuPrcChildCreate(ItemDebugSelectionProcess, 2000, 12288, 0, HuPrcCurrentGet());
	} else {
		HuPrcChildCreate(StoryMapEventSelectionProcess, 2000, 12288, 0, HuPrcCurrentGet());
	}
}

char *characterNameTable[] = {
	"Mario",
	"Luigi",
	"Peach",
	"Yoshi",
	"Wario",
	"Donkey",
	"Daisy",
	"Waluigi"
};

void ItemDebugSelectionProcess(void)
{
	s32 selectedItemIndex = 0;
	s32 textLeft = 16;
	s32 textTop = 40;
	s32 textLineHeight = 15;
	float textScale = 1.5f;
	float inputRepeatDelay = 0;
	GXColor selectionHighlightColor = { 0, 0, 255, 128 };
	CharDataClose(-1);
	WipeColorSet(255, 255, 255);
	WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
	while(1) {
		if(!WipeStatGet()) {
			break;
		}
		HuPrcVSleep();
	}
	while(1) {
		HuPrcVSleep();
		printWin(textLeft, textTop+(textLineHeight*3)-2, 400, 10*textScale, &selectionHighlightColor);
		print8(textLeft, textTop, textScale, "MpgcStory - ItemDebug");
		print8(textLeft, textTop+textLineHeight, textScale, "--------------------------------------------------");
		if(selectedItemIndex < 9) {
			print8(textLeft, textTop+(textLineHeight*3), textScale, "Item No    : 0%d", selectedItemIndex+1);
		} else {
			print8(textLeft, textTop+(textLineHeight*3), textScale, "Item No    : %d", selectedItemIndex+1);
		}
		print8(textLeft, textTop+(textLineHeight*5), textScale, "Chara Name :");
		fontcolor = 13;
		print8(textLeft, textTop+(textLineHeight*5), textScale, "             %s", characterNameTable[selectedItemIndex/6]);
		fontcolor = 15;
		print8(textLeft, textTop+(textLineHeight*6), textScale, "Item  Name :");
		if(storyItemResourceTable[selectedItemIndex].highlightName == 1) {
			fontcolor = 13;
		}
		if(selectedItemIndex < 9) {
			print8(textLeft, textTop+(textLineHeight*6), textScale, "             ys029a0%d.hsf", selectedItemIndex+1);
			fontcolor = 15;
		} else {
			print8(textLeft, textTop+(textLineHeight*6), textScale, "             ys029a%d.hsf", selectedItemIndex+1);
			fontcolor = 15;
		}
		if(inputRepeatDelay  == 0.0f) {
			if(HuPadStkX[0] > 30) {
				selectedItemIndex++;
				inputRepeatDelay = 5;
			} else if(HuPadStkX[0] < -30) {
				selectedItemIndex--;
				inputRepeatDelay = 5;
			} else if(HuPadBtnDown[0] & PAD_TRIGGER_R) {
				selectedItemIndex += 6;
				inputRepeatDelay = 5;
			} else if(HuPadBtnDown[0] & PAD_TRIGGER_L) {
				selectedItemIndex -= 6;
				inputRepeatDelay = 5;
			}
			if(HuPadBtnDown[0] & PAD_BUTTON_A) {
				break;
			}
			if(selectedItemIndex >= 48) {
				selectedItemIndex -= 48;
			}
			if(selectedItemIndex <= -1) {
				selectedItemIndex += 48;
			}
		} else {
			inputRepeatDelay--;
		}
	}
	WipeColorSet(255, 255, 255);
	WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
	while(1) {
		if(!WipeStatGet()) {
			break;
		}
		HuPrcVSleep();
	}
	GWPlayerCfg[0].character = selectedItemIndex/6;
	{
		s32 playerIndex;
		for(playerIndex=1; playerIndex<4; playerIndex++) {
			GWPlayerCfg[playerIndex].character = GWPlayerCfg[playerIndex-1].character+1;
			if(GWPlayerCfg[playerIndex].character > CHARNO_MAX ) {
				GWPlayerCfg[playerIndex].character = 0;
			}				
		}
		GWStoryCharSet(GWPlayerCfg[0].character);
		GWPlayer[0].character = GWPlayerCfg[0].character;
		GWPlayer[1].character = GWPlayerCfg[1].character;
		GWPlayer[2].character = GWPlayerCfg[2].character;
		GWPlayer[3].character = GWPlayerCfg[3].character;
		CharMotionInit(GWPlayerCfg[0].character);
		CharMotionInit(GWPlayerCfg[1].character);
		CharMotionInit(GWPlayerCfg[2].character);
		CharMotionInit(GWPlayerCfg[3].character);
		GWSystem.board = selectedItemIndex%6;
		if(GWSystem.board != BOARD_ID_MAIN6) {
			GWPlayerCoinWinSet(0, 10);
			omOvlGotoEx(DLL_mstorydll, 1, 2, 9999);
		} else {
			GWPlayerCoinWinSet(0, 10);
			omOvlGotoEx(DLL_mstory2dll, 1, 3, 9999);
		}
	}
	HuPrcEnd();
	while(1) {
		HuPrcVSleep();
	}
}
