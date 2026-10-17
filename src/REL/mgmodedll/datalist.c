#include "REL/mgmodedll.h"

#ifndef __MWERKS__
#include "game/esprite.h"
#endif

DataListModel mgModeModelDataList[] = {
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x00), 0, 0, -1, -1, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x01), 0x40000001, 0, -1, -1, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x02), 0, 0, -1, -1, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x03), 0, 1, -1, 2, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x04), 0, 1, -1, 2, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x5E), 0, 0, -1, -1, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x5F), 0, 1, -1, 5, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x60), 0, 1, -1, 5, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x61), 0, 1, -1, 5, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x05), 0x1, 0, -1, -1, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x06), 0x1, 0, -1, -1, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x07), 0x1, 0, -1, -1, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x08), 0x1, 0, -1, -1, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1 } },
    { -1, 0, 0, -1, -1, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1 } },
};

DataListSprite mgModeSpriteDataList[] = {
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x0C), 0, 10, 288, -200, { 255, 255, 255, 255 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x09), 0, 10, 288, -200, { 255, 255, 255, 255 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x0A), 0, 10, 288, -200, { 255, 255, 255, 255 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x0D), 4, 100, 288, 240, { 255, 255, 255, 255 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x18), 4, 200, 288, 240, { 255, 255, 255, 255 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x1A), 4, 20, 288, 240, { 255, 255, 255, 255 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x1B), 4, 30, 288, 240, { 255, 255, 255, 255 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x1C), 4, 40, 288, 240, { 255, 255, 255, 255 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x1D), 4, 50, 288, 240, { 255, 255, 255, 255 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x1E), 4, 50, 288, 240, { 255, 255, 255, 255 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x1F), 4, 50, 288, 240, { 255, 255, 255, 255 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x20), 4, 50, 288, 240, { 255, 255, 255, 255 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x21), 4, 50, 288, 240, { 255, 255, 255, 255 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x25), 4, 200, 288, 240, { 255, 255, 255, 255 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x0E), 4, 210, 288, 240, { 255, 255, 255, 255 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x0F), 4, 210, 288, 240, { 255, 255, 255, 255 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x19), 4, 200, 288, 240, { 255, 255, 255, 255 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x28), 4, 10, 288, 240, { 255, 255, 255, 255 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x28), 4, 10, 288, 240, { 255, 255, 255, 255 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x2A), 4, 10, 288, 240, { 255, 255, 255, 255 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x2E), 4, 200, 288, 240, { 255, 255, 255, 255 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x2F), 4, 200, 288, 240, { 255, 255, 255, 255 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x32), 4, 5, 288, 240, { 255, 255, 255, 255 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x31), 4, 4, 288, 240, { 255, 255, 255, 255 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x30), 4, 4, 288, 240, { 255, 255, 255, 255 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x29), 4, 4, 288, 240, { 255, 255, 255, 255 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x29), 4, 4, 288, 240, { 255, 255, 255, 255 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x33), 4, 50, 288, 240, { 255, 255, 255, 255 } },
    { DATA_MAKE_NUM(DATADIR_MGMODE, 0x34), 4, 50, 288, 256, { 255, 255, 255, 255 } },
    { 0, 0, 0, 0, 0, { 0, 0, 0, 0 } },
};

void CreateDataListModels(DataListModel *modelList)
{
    DataListModel *list;
    s16 i;
    s16 model;
    s16 clearI;
    HU3DMODEL *modelPtr;
    void *data;
    list = modelList;
    for (clearI = 0; clearI < 13; clearI++) {
        mgModeModelIds[clearI] = mgModeMotionIds[clearI] = -1;
    }
    for (i = 0; list->datanum != DATA_NUM_LISTEND; list++, i++) {
        if (list->type == 0) {
            data = HuDataSelHeapReadNum(list->datanum, HU_MEMNUM_OVL, HEAP_MODEL);
            model = Hu3DModelCreate(data);
            mgModeModelIds[i] = model;
            Hu3DModelAttrSet(model, list->attr);
            Hu3DModelPosSetV(model, &list->pos);
            Hu3DModelRotSetV(model, &list->rot);
            Hu3DModelScaleSetV(model, &list->scale);
            modelPtr = &Hu3DData[model];
            if (modelPtr->motId != -1) {
                mgModeMotionIds[i] = modelPtr->motId;
            }
        }
        else if (list->type == 1) {
            data = HuDataSelHeapReadNum(list->datanum, HU_MEMNUM_OVL, HEAP_MODEL);
            mgModeMotionIds[i] = Hu3DJointMotion(mgModeModelIds[list->motLink], data);
        }
        (void)i;
    }
    list = modelList;
    for (i = 0; list->datanum != DATA_NUM_LISTEND; list++, i++) {
        if (list->type == 2) {
            model = Hu3DModelLink(mgModeModelIds[list->link]);
            mgModeModelIds[i] = model;
            Hu3DModelAttrSet(model, list->attr);
            Hu3DModelPosSetV(model, &list->pos);
            Hu3DModelRotSetV(model, &list->rot);
            Hu3DModelScaleSetV(model, &list->scale);
        }
        if (list->type == 0 && list->motLink != -1) {
            Hu3DMotionSet(mgModeModelIds[i], mgModeMotionIds[list->motLink]);
        }
        (void)i;
    }
}

void CreateDataListSprites(DataListSprite *spriteList)
{
    DataListSprite *list;
    s16 sprite;
    s16 clearI;
    s16 i;
    list = spriteList;
    for (clearI = 0; clearI < 29; clearI++) {
        mgModeSpriteIds[clearI] = -1;
    }
    for (i = 0; list->datanum; i++, list++) {
        sprite = espEntry(list->datanum, 100, 0);
        mgModeSpriteIds[i] = sprite;
        espPosSet(sprite, list->x, list->y);
        espColorSet(sprite, list->color.r, list->color.g, list->color.b);
        espTPLvlSet(sprite, list->color.a / 255.0f);
        espPriSet(sprite, list->prio);
        espAttrSet(sprite, list->attr);
    }
}

void DestroyDataListSprites(void)
{
    s16 i;
    for (i = 0; i < 29; i++) {
        if (mgModeSpriteIds[i] != -1) {
            espKill(mgModeSpriteIds[i]);
        }
    }
}
