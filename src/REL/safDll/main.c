#include "dolphin.h"
#include "game/hu3d.h"
#include "game/gamework_data.h"
#include "game/printfunc.h"
#include "game/pad.h"
#include "game/process.h"
#include "game/saveload.h"
#include "math.h"

#ifndef __MWERKS__
#include "game/audio.h"
#endif

s32 overlayDefaultValue = 100;
#if VERSION_NTSC
s32 unusedOverlayBuffer[192];
#endif

void SafInit(void)
{

}

void SafMarioEyeDim(s16 modelIndex)
{
	HU3DMODEL *model = &Hu3DData[modelIndex];
	HSFDATA *modelData = model->hsf;
	HSFMATERIAL *material = modelData->material;
	s16 materialIndex, materialAttributeIndex;
	for(materialIndex=0; materialIndex<modelData->materialNum; materialIndex++, material++) {
		for(materialAttributeIndex=0; materialAttributeIndex<material->attrNum; materialAttributeIndex++) {
			HSFATTRIBUTE *attribute = &modelData->attribute[material->attr[materialAttributeIndex]];
			if(strcmp(attribute->bitmap->name, "s3c000m1_eyes") == 0 || strcmp(attribute->bitmap->name, "mario_eyes") == 0) {
				break;
			}
		}
		if(materialAttributeIndex == material->attrNum) {
			material->color[0] *= 0.6f;
			material->color[1] *= 0.3f;
			material->color[2] *= 0.3f;
			material->litColor[0] *= 0.6f;
			material->litColor[1] *= 0.3f;
			material->litColor[2] *= 0.3f;
		}
	}
}

void SafSaveTest(void)
{
	OSTick timeWrite;
	OSTick timeRead;
	s16 character = GWPlayerCfg[0].character;
	SLLoad();
	timeWrite = timeRead = 0;
	while(1) {
		print8(16, 420, 1.5f, "READ  TIME %d", OSTicksToMilliseconds(timeRead));
		print8(16, 430, 1.5f, "WRITE TIME %d", OSTicksToMilliseconds(timeWrite));
		HuPrcVSleep();
		if(HuPadBtnDown[0] & PAD_BUTTON_Y) {
			CharFXStop(character, 282);
			timeWrite = OSGetTick();
			SLSave();
			timeWrite = OSGetTick()-timeWrite;
		}
		if(HuPadBtnDown[0] & PAD_BUTTON_X) {
			timeRead = OSGetTick();
			SLLoad();
			timeRead = OSGetTick()-timeRead;
		}
	}
}
