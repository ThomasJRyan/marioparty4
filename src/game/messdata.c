#include "dolphin.h"

/**
 * @brief Locates the message bank selected by a packed message identifier.
 *
 * @param messdata Message-data archive base.
 * @param id Packed bank and message identifier.
 * @return Pointer to the selected bank, or NULL if the bank is absent.
 */
static void *MessData_MesDataGet(void *messdata, u32 id)
{
    s32 i;
    s32 max_bank;
    u16 *banks;
    u16 bank;
    s32 *data;
    bank = id >> 16;
    data = messdata;
    max_bank = *data;
    data++;
    banks = (u16 *)(((u8 *)messdata)+(*data));
    for(i=max_bank; i != 0; i--, banks += 2) {
        if(*banks == bank) {
            break;
        }
    }
    if(i == 0) {
        return NULL;
    } else {
        data += banks[1];
        return (((u8 *)messdata)+(*data));
    }
}

/**
 * @brief Resolves a message within one message bank.
 *
 * @param messbank Message-bank base.
 * @param id Packed identifier whose low 16 bits select the message.
 * @return Pointer to the message data, or NULL if the index is out of range.
 */
static void *_MessData_MesPtrGet(void *messbank, u32 id)
{
    u16 index;
    s32 max_index;
    s32 *data;
    
    index = id & 0xFFFF;
    data = messbank;
    max_index = *data;
    data++;
    if(max_index <= index) {
        return NULL;
    } else {
        data =  data+index;
        return (((u8 *)messbank)+(*data));
    }
}

/**
 * @brief Resolves a packed message identifier from a message-data archive.
 *
 * @param messdata Message-data archive base.
 * @param id Packed bank and message identifier.
 * @return Pointer to the message data, or NULL if its bank or index is absent.
 */
void *MessData_MesPtrGet(void *messdata, u32 id)
{
    void *bank = MessData_MesDataGet(messdata, id);
    if(bank) {
        return _MessData_MesPtrGet(bank, id);
    }
    return NULL;
}
