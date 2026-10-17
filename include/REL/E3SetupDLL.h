#ifndef REL_E3SETUPDLL_H
#define REL_E3SETUPDLL_H

#include "game/animdata.h"
#include "game/object.h"
#include "game/pad.h"

typedef struct e3pad {
	u16 btn;
	u16 btnDown;
	u16 enable;
	u16 reserved_06;
} E3Pad;

typedef struct mg_interface_work {
	/* 0x00 */ ANIMDATA *animations[2];
	/* 0x08 */ s32 reserved_08[5];
	/* 0x1C */ s16 title_sprite_group;
	/* 0x1E */ s16 prompt_sprite_group;
	/* 0x20 */ s32 reserved_20[3];
	/* 0x2C */ s16 press_start_window;
	/* 0x30 */ s32 reserved_30;
} MGInterfaceWork;

typedef struct mg_picture_work {
	/* 0x00 */ ANIMDATA *animations[2];
	/* 0x08 */ s32 reserved_08[9];
	/* 0x2C */ s16 reserved_2C;
	/* 0x2E */ s32 reserved_2E[4];
	/* 0x40 */ s16 sprite_group;
	/* 0x44 */ s32 reserved_44[2];
	/* 0x4C */ s32 reserved_4C;
	/* 0x50 */ s32 reserved_50;
} MGPictureWork;

typedef struct mg_select_work {
	/* 0x00 */ ANIMDATA *animations[4];
	/* 0x10 */ s32 reserved_10[3];
	/* 0x1C */ s16 cursor_sprite_group;
	/* 0x1E */ s16 previous_arrow_group;
	/* 0x20 */ s16 next_arrow_group;
	/* 0x22 */ s16 reserved_22;
	/* 0x24 */ s32 reserved_24[2];
	/* 0x2C */ s16 reserved_2C;
	/* 0x2E */ s16 minigame_windows[4];
	/* 0x36 */ s16 reserved_36;
	/* 0x38 */ s16 minigame_window_positions[4];
	/* 0x40 */ s16 reserved_40;
	/* 0x42 */ s16 selected_window_position;
	/* 0x44 */ s16 selected_minigame_index;
	/* 0x46 */ s16 previous_minigame_index;
} MGSelectWork;

extern E3Pad e3PadData[4];
extern s16 e3PadCtrl;
extern s16 e3ConfigPlayerCnt;
extern s16 e3ConfigPlayerF;
extern s32 e3ExitEnableF;
extern OMOVL e3NextOvl;
extern s32 e3NextEvent;
extern u32 e3ExitMode;
extern float e3CameraFov;
extern OMOBJ *e3MenuMainObj;
extern OMOBJ *e3ViewObj;
extern OMOBJ *e3OvlWatchObj;
extern OMOBJ *e3BGObj;

void E3MainInit(void);
void E3MGSelectInit(void);
s16 E3PadRead(void);
u16 E3PadUpdate(s16 player, E3Pad *pad);
void E3OvlWatchInit(OMOBJ *object);
void E3BGCreate(OMOBJ *object);
void E3LightInit(void);

#endif
