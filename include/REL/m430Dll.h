#ifndef REL_M430DLL_H
#define REL_M430DLL_H

#include "game/object.h"

s32 Fn14030(void);
u32 Fn14058(void);
u32 Fn14070(void);
u32 Fn14088(void);
void Fn140D0(s32 arg0);
void Fn14100(s32 arg0);
float Fn14130(void);
float Fn148B0(float arg8, float arg9, float argA);

void Fn14A80(HUPROCESS *process);
void Fn14C48(void);
float Fn17FBC(s32 arg0);
void Fn17FFC(s32 arg0, float arg8);
void Fn1AEE0(s32 arg0, Vec *arg1, float arg8, float arg9);
void Fn1B570(s32 arg0, Vec *arg1, float arg8, float arg9, Vec *arg2);
void Fn1BB98(u32 arg0);

void Fn1BBC4(HUPROCESS *process);
void Fn1C00C(void);

#endif
