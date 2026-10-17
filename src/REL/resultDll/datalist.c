#include "REL/resultdll.h"

#ifndef __MWERKS__
#include "game/esprite.h"
#endif

s16 resultStageModelIds[32];
s16 resultStageMotionIds[32];
s16 resultStageSpriteIds[33];
s16 unreferencedResultDatalistValue;

void StageModelCreate(ResultStageModel *modelEntries)
{
	ResultStageModel *currentModelEntry;
	s16 entryIndex;
	s16 createdModelId;
	s16 entryClearIndex;
	HU3DMODEL *createdModelData;
	void *modelData;
	currentModelEntry = modelEntries;
	for(entryClearIndex=0; entryClearIndex<32; entryClearIndex++) {
		resultStageModelIds[entryClearIndex] = resultStageMotionIds[entryClearIndex] = -1;
	}
	for(entryIndex=0; currentModelEntry->dataNumber != DATA_NUM_LISTEND; currentModelEntry++, entryIndex++) {
		if(currentModelEntry->modelType == 0) {
			modelData = HuDataSelHeapReadNum(currentModelEntry->dataNumber, HU_MEMNUM_OVL, HEAP_MODEL);
			createdModelId = Hu3DModelCreate(modelData);
			resultStageModelIds[entryIndex] = createdModelId;
			Hu3DModelAttrSet(createdModelId, currentModelEntry->attributes);
			Hu3DModelPosSetV(createdModelId, &currentModelEntry->pos);
			Hu3DModelRotSetV(createdModelId, &currentModelEntry->rot);
			Hu3DModelScaleSetV(createdModelId, &currentModelEntry->scale);
			createdModelData = &Hu3DData[createdModelId];
			if(createdModelData->motId != -1) {
				resultStageMotionIds[entryIndex] = createdModelData->motId;
			}
		} else if(currentModelEntry->modelType == 1) {
			modelData = HuDataSelHeapReadNum(currentModelEntry->dataNumber, HU_MEMNUM_OVL, HEAP_MODEL);
			resultStageMotionIds[entryIndex] = Hu3DJointMotion(resultStageModelIds[currentModelEntry->motionLinkIndex], modelData);
			
		}
		(void)entryIndex;
	}
	currentModelEntry = modelEntries;
	for(entryIndex=0; currentModelEntry->dataNumber != DATA_NUM_LISTEND; currentModelEntry++, entryIndex++) {
		if(currentModelEntry->modelType == 2) {
			createdModelId = Hu3DModelLink(resultStageModelIds[currentModelEntry->linkedModelIndex]);
			resultStageModelIds[entryIndex] = createdModelId;
			Hu3DModelAttrSet(createdModelId, currentModelEntry->attributes);
			Hu3DModelPosSetV(createdModelId, &currentModelEntry->pos);
			Hu3DModelRotSetV(createdModelId, &currentModelEntry->rot);
			Hu3DModelScaleSetV(createdModelId, &currentModelEntry->scale);
		}
		if(currentModelEntry->motionLinkIndex != -1) {
			Hu3DMotionSet(resultStageModelIds[entryIndex], resultStageMotionIds[currentModelEntry->motionLinkIndex]);
		}
		(void)entryIndex;
	}
}

void StageSpriteCreate(ResultStageSprite *spriteEntries)
{
	ResultStageSprite *currentSpriteEntry;
	s16 createdSpriteId;
	s16 entryClearIndex;
	s16 entryIndex;
	currentSpriteEntry = spriteEntries;
	for(entryClearIndex=0; entryClearIndex<32; entryClearIndex++) {
		resultStageSpriteIds[entryClearIndex] = -1;
	}
	for(entryIndex=0; currentSpriteEntry->dataNumber; entryIndex++, currentSpriteEntry++) {
		createdSpriteId = espEntry(currentSpriteEntry->dataNumber, 100, 0);
		resultStageSpriteIds[entryIndex] = createdSpriteId;
		espPosSet(createdSpriteId, currentSpriteEntry->x, currentSpriteEntry->y);
		espColorSet(createdSpriteId, currentSpriteEntry->color.r, currentSpriteEntry->color.g, currentSpriteEntry->color.b);
		espTPLvlSet(createdSpriteId, currentSpriteEntry->color.a/255.0f);
		espPriSet(createdSpriteId, currentSpriteEntry->priority);
		espAttrSet(createdSpriteId, currentSpriteEntry->attributes);
	}
}

void StageSpriteKill(void)
{
	s16 spriteIndex;
	for(spriteIndex=0; spriteIndex<32; spriteIndex++) {
		if(resultStageSpriteIds[spriteIndex] != -1) {
			espKill(resultStageSpriteIds[spriteIndex]);
		}
	}
}
