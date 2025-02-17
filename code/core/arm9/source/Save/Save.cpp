#include "common.h"
#include <string.h>
#include "Fat/ff.h"
#include "Core/Environment.h"
#include "MemFastSearch.h"
#include "SaveSwi.h"
#include "SaveTypeInfo.h"
#include "Save.h"

[[gnu::section(".ewram.bss")]]
u8 gSaveData[SAVE_DATA_SIZE] alignas(32);

[[gnu::section(".ewram.bss")]]
FIL gSaveFile alignas(32);

static DWORD sClusterTable[64];

bool sav_tryPatchFunction(const u32* signature, u32 saveSwiNumber, void* patchFunction)
{
    u32* function = (u32*)mem_fastSearch16((const u32*)0x02200000, 0x200000, signature);
    if (!function)
        return false;

    sav_swiTable[saveSwiNumber] = patchFunction;
    *(u16*)function = SAVE_THUMB_SWI(saveSwiNumber);
    return true;
}

static void loadSaveClusterMap(void)
{
    sClusterTable[0] = sizeof(sClusterTable) / sizeof(DWORD);
    gSaveFile.cltbl = sClusterTable;
    f_lseek(&gSaveFile, CREATE_LINKMAP);
}

static void fillSaveFile(u32 start, u32 end, u8 saveFill)
{
    f_lseek(&gSaveFile, start);
    for (u32 i = start; i < end; ++i)
    {
        UINT written = 0;
        f_write(&gSaveFile, &saveFill, 1, &written);
    }
    f_sync(&gSaveFile);
}

void sav_initializeSave(const SaveTypeInfo* saveTypeInfo, const char* savePath)
{
    u32 saveSize = saveTypeInfo ? saveTypeInfo->size : (32 * 1024);
    u8 saveFill = 0;
    if (saveTypeInfo && (saveTypeInfo->type & SAVE_TYPE_MASK) == SAVE_TYPE_FLASH)
    {
        saveFill = 0xFF;
    }

    memset(gSaveData, saveFill, SAVE_DATA_SIZE);
    if (Environment::IsIsNitroEmulator())
    {
        memset((void*)ISNITRO_SAVE_BUFFER, saveFill, ISNITRO_SAVE_BUFFER_SIZE);
    }
    memset(&gSaveFile, 0, sizeof(gSaveFile));
    if (f_open(&gSaveFile, savePath, FA_OPEN_EXISTING | FA_READ | FA_WRITE) == FR_OK)
    {
        bool clusterMapLoaded = false;
        u32 initialSize = f_size(&gSaveFile);
        if (initialSize < saveSize)
        {
            if (f_lseek(&gSaveFile, saveSize) == FR_OK)
            {
                f_rewind(&gSaveFile);
                loadSaveClusterMap();
                clusterMapLoaded = true;
                fillSaveFile(initialSize, saveSize, saveFill);
            }
        }

        if (!clusterMapLoaded)
        {
            loadSaveClusterMap();
            clusterMapLoaded = true;
        }

        if (saveSize <= SAVE_DATA_SIZE)
        {
            f_rewind(&gSaveFile);
            UINT read = 0;
            f_read(&gSaveFile, gSaveData, saveSize, &read);
        }

        if (Environment::IsIsNitroEmulator())
        {
            f_rewind(&gSaveFile);
            UINT read = 0;
            f_read(&gSaveFile, (void*)ISNITRO_SAVE_BUFFER, saveSize, &read);
        }
    }
    else if (!Environment::IsIsNitroEmulator())
    {
        if (f_open(&gSaveFile, savePath, FA_CREATE_NEW | FA_READ | FA_WRITE) == FR_OK)
        {
            if (f_lseek(&gSaveFile, saveSize) == FR_OK)
            {
                f_rewind(&gSaveFile);
                loadSaveClusterMap();
                fillSaveFile(0, saveSize, saveFill);
            }
        }
    }
}

extern "C" u8 sav_readSaveByteFromFile(u32 saveAddress)
{
    u8 saveByte;
    if (Environment::IsIsNitroEmulator())
    {
        // save buffer in extended memory
        saveByte = ISNITRO_SAVE_BUFFER[saveAddress];
    }
    else
    {
        // write to file
        f_lseek(&gSaveFile, saveAddress);
        UINT bytesRead = 0;
        f_read(&gSaveFile, &saveByte, 1, &bytesRead);
    }
    return saveByte;
}

extern "C" void sav_writeSaveByteToFile(u32 saveAddress, u8 data)
{
    if (Environment::IsIsNitroEmulator())
    {
        // save buffer in extended memory
        ISNITRO_SAVE_BUFFER[saveAddress] = data;
    }
    else
    {
        // write to file
        f_lseek(&gSaveFile, saveAddress);
        UINT bytesWritten = 0;
        f_write(&gSaveFile, &data, 1, &bytesWritten);
    }
}

extern "C" void sav_flushSaveFile(void)
{
    f_sync(&gSaveFile);
}
