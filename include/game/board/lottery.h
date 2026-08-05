#ifndef _BOARD_LOTTERY_H
#define _BOARD_LOTTERY_H

#include "dolphin/types.h"

void BoardLotteryHostSet(s16 host_model);
s16 BoardLotteryHostGet(void);
void BoardLotteryExec(void);
void BoardLotteryInit(void);
void BoardLotteryKill(void);
void BoardLotteryTutorialExec(void);

#endif
