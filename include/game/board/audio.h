#ifndef _BOARD_AUDIO_H
#define _BOARD_AUDIO_H

#include "dolphin/types.h"

void BoardMusStartBoard(void);
void BoardMusStart(s32 board_no, s32 music_id, s8 vol, u16 speed);
void BoardAudSeqFadeOutFast(s32 board_no);
void BoardAudSeqFadeOut(s32 board_no, u16 speed);
void BoardMusLoudSet(s32 board_no, s32 is_quieter);
void BoardMusVolPanSet(s32 board_no, s8 vol, u16 fade_speed);
void BoardAudSeqPause(s32 board_no, s32 pause, u16 speed);
s32 BoardMusStatusGet(s32 board_no);
void BoardAudSeqClear(void);
void BoardAudSeqFadeOutAll(void);
void BoardAudFXPlay(void);
void BoardAudFXStop(void);

#endif
