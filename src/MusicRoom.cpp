#define TH08_MUSICROOM_SUPERVISOR_PRELOAD_OUT_OF_LINE
#include "th_pch.h"
#undef TH08_MUSICROOM_SUPERVISOR_PRELOAD_OUT_OF_LINE

#include "AsciiManager.hpp"
#include "GameManager.hpp"
#include "MusicRoom.hpp"
#include "SoundPlayer.hpp"
#include "i18n.hpp"
#ifdef TH08_IOS
#include "modern/ios/ios_touch.hpp"
#endif

namespace th08
{

namespace
{

bool IsCp932LeadByte(unsigned char value)
{
    return (value >= 0x81 && value <= 0x9f) ||
           (value >= 0xe0 && value <= 0xfc);
}

bool IsCp932TrailByte(unsigned char value)
{
    return (value >= 0x40 && value <= 0x7e) ||
           (value >= 0x80 && value <= 0xfc);
}

size_t CompleteCp932Length(const char *text, size_t available)
{
    size_t offset = 0;
    size_t complete = 0;
    while (offset < available && text[offset] != '\0')
    {
        const unsigned char value = static_cast<unsigned char>(text[offset]);
        if (IsCp932LeadByte(value))
        {
            if (offset + 1 >= available || text[offset + 1] == '\0' ||
                !IsCp932TrailByte(static_cast<unsigned char>(text[offset + 1])))
                break;
            offset += 2;
        }
        else
        {
            ++offset;
        }
        complete = offset;
    }
    return complete;
}

void CopyCp932Text(char *destination, size_t destinationSize, const char *source)
{
    if (destination == NULL || destinationSize == 0)
        return;
    destination[0] = '\0';
    if (source == NULL)
        return;

    const size_t length = CompleteCp932Length(source, destinationSize - 1);
    if (length != 0)
        memcpy(destination, source, length);
    destination[length] = '\0';
}

void TerminateParsedCp932Text(char *text, size_t textSize, size_t bytesCopied)
{
    if (text == NULL || textSize == 0)
        return;
    if (bytesCopied >= textSize)
        bytesCopied = textSize - 1;
    text[bytesCopied] = '\0';
    text[CompleteCp932Length(text, bytesCopied)] = '\0';
}

} // namespace

const char *g_BgmNotUnlockedWarning[] = {
    TH_WARN_BGM_NOT_UNLOCKED0, TH_WARN_BGM_NOT_UNLOCKED1, TH_WARN_BGM_NOT_UNLOCKED2, TH_WARN_BGM_NOT_UNLOCKED3,
    TH_WARN_BGM_NOT_UNLOCKED4, TH_WARN_BGM_NOT_UNLOCKED5, TH_WARN_BGM_NOT_UNLOCKED6, TH_WARN_BGM_NOT_UNLOCKED4};

ZunResult MusicRoom::CheckInputEnable()
{
    i32 i;

    if (this->frameCount == 0)
    {
        for (i = 0; i < ARRAY_SIZE_SIGNED(this->songNameVms); i++)
        {
            if (this->cursor == i)
            {
                this->songNameVms[i].SetInterrupt(1);
            }
            else
            {
                this->songNameVms[i].SetInterrupt(2);
            }
        }

        for (i = 0; i < ARRAY_SIZE_SIGNED(this->descriptionVms) - 1; i++)
        {
            this->descriptionVms[i].SetInterrupt(1);
        }
    }

    if (this->frameCount >= 8)
    {
        this->inputState = 1;
    }

    return ZUN_SUCCESS;
}

#pragma var_order(oldListingOffset, i, buf1, buf2, pos)
i32 MusicRoom::ProcessInput()
{
    i32 oldListingOffset = this->listingOffset;
    i32 i;

    if (WAS_PRESSED_SCROLLING(TH_BUTTON_UP))
    {
        this->cursor--;
        if (this->cursor < 0)
        {
            this->cursor = this->numDescriptors - 1;
            this->listingOffset = this->numDescriptors - 10;
            if (this->listingOffset < 0)
            {
                this->listingOffset = 0;
            }
        }
        else if (this->listingOffset > this->cursor)
        {
            this->listingOffset = this->cursor;
        }

        for (i = 0; i < ARRAY_SIZE_SIGNED(this->songNameVms); i++)
        {
            if (this->cursor == i)
            {
                this->songNameVms[i].SetInterrupt(1);
            }
            else
            {
                this->songNameVms[i].SetInterrupt(2);
            }
        }

        for (i = 0; i < ARRAY_SIZE_SIGNED(this->descriptionVms) - 1; i++)
        {
            this->descriptionVms[i].SetInterrupt(2);
        }

        this->frameCount = 0;
    }

    if (WAS_PRESSED_SCROLLING(TH_BUTTON_DOWN))
    {
        this->cursor++;

        if (this->cursor >= this->numDescriptors)
        {
            this->cursor = 0;
            this->listingOffset = 0;
        }
        else
        {
            if (this->listingOffset <= this->cursor - 10)
            {
                this->listingOffset = this->cursor - 9;
            }
        }

        for (i = 0; i < ARRAY_SIZE_SIGNED(this->songNameVms); i++)
        {
            if (this->cursor == i)
            {
                this->songNameVms[i].SetInterrupt(1);
            }
            else
            {
                this->songNameVms[i].SetInterrupt(2);
            }
        }

        for (i = 0; i < ARRAY_SIZE_SIGNED(this->descriptionVms) - 1; i++)
        {
            this->descriptionVms[i].SetInterrupt(2);
        }

        this->frameCount = 0;
    }

    char buf1[66];
    char buf2[66];

    if (this->frameCount < 30)
    {
        i = -1;

        if (this->frameCount == 10)
        {
            i = 0;
        }
        else if (this->frameCount == 12)
        {
            i = 1;
        }
        else if (this->frameCount == 14)
        {
            i = 2;
        }
        else if (this->frameCount == 16)
        {
            i = 3;
        }
        else if (this->frameCount == 18)
        {
            i = 4;
        }
        else if (this->frameCount == 20)
        {
            i = 5;
        }
        else if (this->frameCount == 22)
        {
            i = 6;
        }

        if (i >= 0)
        {
            g_Supervisor.textAnm->SetAndExecuteScriptIdx(&this->descriptionVms[i], 10 + i);
            this->descriptionVms[i].SetInterrupt(1);

            memset(buf1, 0, sizeof(buf1));

            if (this->selectedSongIndex == this->cursor || this->bgmUnlocked[this->cursor])
            {
                CopyCp932Text(buf1, sizeof(buf1),
                              this->trackDescriptors[this->cursor].descriptors[i]);
            }
            else
            {
                CopyCp932Text(buf1, sizeof(buf1), g_BgmNotUnlockedWarning[i]);
            }

            if (buf1[0] != '\0')
            {
                this->descriptionVms[i].flag1 = true;
                g_AnmManager->DrawTextLeft(&this->descriptionVms[i], 0xffe0c0, 0x300000, buf1);
            }
            else
            {
                this->descriptionVms[i].flag1 = false;
            }
        }
    }

    if (WAS_PRESSED(TH_BUTTON_SHOOT | TH_BUTTON_ENTER))
    {
        this->selectedSongIndex = this->cursor;

        if (g_Supervisor.IsMusicPreloadEnabled())
        {
            g_SoundPlayer.StartBGM("thbgm.dat");
        }

        g_Supervisor.PlayAudio(this->trackDescriptors[this->selectedSongIndex].path, 0);

        this->frameCount = 0;

        g_Supervisor.textAnm->SetAndExecuteScriptIdx(&this->descriptionVms[7], 17);
        this->descriptionVms[7].SetInterrupt(1);

        memset(buf2, 0, sizeof(buf2));

        CopyCp932Text(buf2, sizeof(buf2),
                      this->trackDescriptors[this->cursor].descriptors[0]);

        g_AnmManager->DrawTextLeft(&this->descriptionVms[7], 0xffe0c0, 0x300000, buf2);
    }

    if (WAS_PRESSED(TH_BUTTON_BOMB | TH_BUTTON_MENU))
    {
        g_Supervisor.curState = SupervisorState_TitleScreen;

        Float3 capturePosition(500.0, 440.0f, 0.0f);
        g_Supervisor.SetupLoadingVmsAndInitCapture(&capturePosition);

        return 1;
    }

    if (WAS_PRESSED(TH_BUTTON_SKIP))
    {
        g_Supervisor.FadeOutMusic(8.0f);
    }
    if (WAS_PRESSED(TH_BUTTON_RESET))
    {
        if (g_Supervisor.IsMusicPreloadEnabled())
        {
            g_SoundPlayer.StartBGM("thbgm.dat");
        }

        g_Supervisor.PlayAudio(this->trackDescriptors[this->selectedSongIndex].path, 0);
    }

    this->frameCount++;

    return ZUN_SUCCESS;
}

ZunResult MusicRoom::RegisterChain()
{
    static MusicRoom g_MusicRoom;

    MusicRoom *musicRoom = &g_MusicRoom;

    memset(musicRoom, 0, sizeof(MusicRoom));

    musicRoom->calcChain = g_Chain.CreateElem((ChainCallback)MusicRoom::OnUpdate);
    musicRoom->calcChain->arg = musicRoom;
    musicRoom->calcChain->addedCallback = (ChainLifetimeCallback)MusicRoom::AddedCallback;
    musicRoom->calcChain->deletedCallback = (ChainLifetimeCallback)MusicRoom::DeletedCallback;

    if (g_Chain.AddToCalcChain(musicRoom->calcChain, 4) != ZUN_SUCCESS)
    {
        return ZUN_ERROR;
    }

    musicRoom->drawChain = g_Chain.CreateElem((ChainCallback)MusicRoom::OnDraw);
    musicRoom->drawChain->arg = musicRoom;
    g_Chain.AddToDrawChain(musicRoom->drawChain, 3);

    return ZUN_SUCCESS;
}

ChainCallbackResult MusicRoom::OnUpdate(MusicRoom *musicRoom)
{
    i32 i;

    i32 oldInputSetting = musicRoom->inputState;

start:
    switch (musicRoom->inputState)
    {
    case 0:
        if (musicRoom->CheckInputEnable() == ZUN_SUCCESS)
        {
            break;
        }

        goto start;
    case 1:
        if (musicRoom->ProcessInput())
        {
            return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
        }
    }

    if (oldInputSetting != musicRoom->inputState)
    {
        musicRoom->frameCount = 0;
    }
    else
    {
        musicRoom->frameCount++;
    }

    g_AnmManager->ExecuteScript(&musicRoom->mainVms[0]);

    for (i = 0; i < ARRAY_SIZE_SIGNED(musicRoom->songNameVms); i++)
    {
        g_AnmManager->ExecuteScript(&musicRoom->songNameVms[i]);
    }

    for (i = 0; i < ARRAY_SIZE_SIGNED(musicRoom->descriptionVms); i++)
    {
        g_AnmManager->ExecuteScript(&musicRoom->descriptionVms[i]);
    }

    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

#pragma var_order(color, i, arrowText, position)
ChainCallbackResult MusicRoom::OnDraw(MusicRoom *musicRoom)
{
    int i;
    Float3 position;
    D3DCOLOR color;
    char arrowText[4];

    arrowText[0] = 0x7f;
    arrowText[1] = 0x0;

    g_AnmManager->ClearTexture();
    g_AnmManager->CopySurfaceToBackbuffer(0, 0, 0, 0, 0);
    g_AnmManager->DrawNoRotation(&musicRoom->mainVms[0]);

    for (i = musicRoom->listingOffset; i < musicRoom->listingOffset + 10; i++)
    {
        if (i >= musicRoom->numDescriptors)
        {
            break;
        }

        g_AsciiManager.SetColor(musicRoom->songNameVms[i].color1.d3dColor);

        musicRoom->songNameVms[i].pos.x = 93.0f;
        musicRoom->songNameVms[i].pos.y = (((i + 1 - musicRoom->listingOffset) * 18) + 104.0f) - 20.0f;
        musicRoom->songNameVms[i].pos.z = 0.0f;

        g_AnmManager->DrawNoRotation(&musicRoom->songNameVms[i]);

        position = musicRoom->songNameVms[i].pos;
        position.x -= 60.0f;

        if (musicRoom->cursor == i)
        {
            g_AsciiManager.AddString(&position, arrowText);
        }

        position.x += 15.0f;
        g_AsciiManager.AddFormatText(&position, "%2d.", i + 1);
    }

    /* again, ???? */
    i++;

    for (i = 0; i < ARRAY_SIZE_SIGNED(musicRoom->descriptionVms) - 1; i++)
    {
        g_AnmManager->DrawNoRotation(&musicRoom->descriptionVms[i]);
    }

    g_AsciiManager.SetColor(0xffffffff);

    position.x = 320.0f;
    position.y = 32.0f;
    position.z = 0.0f;

    g_AsciiManager.AddFormatText(&position, "Now Playing");

    position = musicRoom->descriptionVms[7].pos;
    color = musicRoom->descriptionVms[7].color1.d3dColor;

    musicRoom->descriptionVms[7].pos.x = 320.0f;
    musicRoom->descriptionVms[7].pos.y = 52.0f;
    musicRoom->descriptionVms[7].pos.z = 0.0f;
    musicRoom->descriptionVms[7].color1.d3dColor = 0xffffffff;

    g_AnmManager->DrawNoRotation(&musicRoom->descriptionVms[7]);

    musicRoom->descriptionVms[7].pos = position;
    musicRoom->descriptionVms[7].color1.d3dColor = color;

    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

#pragma var_order(i, currentLine, fileSize, ptr, charIdx, musicCmtFile)
ZunResult MusicRoom::AddedCallback(MusicRoom *musicRoom)
{
    if (g_AnmManager->LoadSurface(0, "result/music.jpg") != ZUN_SUCCESS)
    {
        return ZUN_ERROR;
    }

    musicRoom->musicAnm = g_AnmManager->LoadAnm(23, "music00.anm");
    if (musicRoom->musicAnm == NULL)
    {
        return ZUN_ERROR;
    }

    musicRoom->selectedSongIndex = 0;
    musicRoom->musicAnm->SetAndExecuteScriptIdx(&musicRoom->mainVms[0], 0);
    musicRoom->frameCount = 0;

    u32 fileSize;
    char *musicCmtFile;
    char *ptr;

    musicCmtFile = ptr = (char *)FileSystem::OpenFile("sprt/musiccmt.txt", (i32 *)&fileSize, FALSE);
    if (ptr == NULL)
    {
        return ZUN_ERROR;
    }

    musicRoom->trackDescriptors = ZUN_NEW_ARRAY(TrackDescriptor, 32, "MusicCmtInf");

    int i = -1;
    u32 charIdx;
    int currentLine;

    while ((ptr - musicCmtFile) < fileSize)
    {
        if (*ptr == '@')
        {
            ptr++;
            i++;
            if (i >= 32)
            {
#ifdef TH08_IOS
                modern::LogStartup("music-room: descriptor limit reached; remaining data ignored");
#endif
                goto out;
            }
            charIdx = 0;

            while (*ptr != '\n' && *ptr != '\r')
            {
                if (charIdx + 1 < sizeof(musicRoom->trackDescriptors[i].path))
                    musicRoom->trackDescriptors[i].path[charIdx] = *ptr;

                ptr++;
                charIdx++;

                if ((ptr - musicCmtFile) >= fileSize)
                {
                    goto out;
                }
            }
            musicRoom->trackDescriptors[i].path[
                charIdx < sizeof(musicRoom->trackDescriptors[i].path)
                    ? charIdx : sizeof(musicRoom->trackDescriptors[i].path) - 1] = '\0';

            while (*ptr == '\n' || *ptr == '\r')
            {
                ptr++;

                if ((ptr - musicCmtFile) >= fileSize)
                {
                    goto out;
                }
            }

            charIdx = 0;

            while (*ptr != '\n' && *ptr != '\r')
            {
                if (charIdx + 1 < sizeof(musicRoom->trackDescriptors[i].title))
                    musicRoom->trackDescriptors[i].title[charIdx] = *ptr;

                ptr++;
                charIdx++;

                if ((ptr - musicCmtFile) >= fileSize)
                {
                    goto out;
                }
            }
            TerminateParsedCp932Text(
                musicRoom->trackDescriptors[i].title,
                sizeof(musicRoom->trackDescriptors[i].title),
                charIdx);

            while (*ptr == '\n' || *ptr == '\r')
            {
                ptr++;

                if ((ptr - musicCmtFile) >= fileSize)
                {
                    goto out;
                }
            }

            for (currentLine = 0; currentLine < 7; currentLine++)
            {
                if (*ptr == '@')
                {
                    break;
                }

                memset(&musicRoom->trackDescriptors[i].descriptors[currentLine], 0,
                       sizeof(musicRoom->trackDescriptors[i].descriptors[0]));

                charIdx = 0;

                while (*ptr != '\n' && *ptr != '\r')
                {
                    if (charIdx + 1 < sizeof(musicRoom->trackDescriptors[i].descriptors[currentLine]))
                        musicRoom->trackDescriptors[i].descriptors[currentLine][charIdx] = *ptr;

                    ptr++;
                    charIdx++;

                    if ((ptr - musicCmtFile) >= fileSize)
                    {
                        goto out;
                    }
                }
                TerminateParsedCp932Text(
                    musicRoom->trackDescriptors[i].descriptors[currentLine],
                    sizeof(musicRoom->trackDescriptors[i].descriptors[currentLine]),
                    charIdx);

                while (*ptr == '\n' || *ptr == '\r')
                {
                    ptr++;

                    if ((ptr - musicCmtFile) >= fileSize)
                    {
                        goto out;
                    }
                }
            }
        }
        else
        {
            *ptr++;
        }
    }

out:
    musicRoom->numDescriptors = i + 1;

#ifdef TH08_IOS
    const char *smokeTest = SDL_getenv("TH08_IOS_SMOKE_TEST");
    if (smokeTest != NULL && strcmp(smokeTest, "musicroom") == 0 &&
        musicRoom->numDescriptors > 2)
    {
        for (int line = 0; line < 7; ++line)
        {
            const u8 *raw = reinterpret_cast<const u8 *>(
                musicRoom->trackDescriptors[2].descriptors[line]);
            char message[224];
            int written = SDL_snprintf(message, sizeof(message),
                                       "music-room/raw track=3 line=%d bytes=", line + 1);
            for (int byteIndex = 0; byteIndex < 66 && raw[byteIndex] != 0 &&
                 written > 0 && written + 3 < static_cast<int>(sizeof(message)); ++byteIndex)
            {
                written += SDL_snprintf(message + written, sizeof(message) - written,
                                        "%02X", static_cast<unsigned int>(raw[byteIndex]));
            }
            modern::LogStartup(message);
        }
    }
#endif

    for (i = 0; i < musicRoom->numDescriptors; i++)
    {
        musicRoom->bgmUnlocked[i] = g_GameManager.plst.bgmUnlocked[i];
    }

    for (i = 0; i < musicRoom->numDescriptors; i++)
    {
        musicRoom->musicAnm->SetAndExecuteScriptIdx(&musicRoom->songNameVms[i], 1 + i);
#ifdef TH08_IOS
        // The mobile port allows every Music Room entry to be selected and
        // played even on a fresh save. Hiding the corresponding title behind
        // question marks makes the touch list impossible to identify.
        g_AnmManager->DrawTextLeft(&musicRoom->songNameVms[i], 0xc0e0ff, 0x302080,
                                   musicRoom->trackDescriptors[i].title);
#else
        if (musicRoom->bgmUnlocked[i])
        {
            g_AnmManager->DrawTextLeft(&musicRoom->songNameVms[i], 0xc0e0ff, 0x302080,
                                       musicRoom->trackDescriptors[i].title);
        }
        else
        {
            g_AnmManager->DrawTextLeft(&musicRoom->songNameVms[i], 0x80a0c0, 0x100040, TH_SONG_NAME_NOT_UNLOCKED);
        }
#endif

        musicRoom->songNameVms[i].pos.x = 93.0f;
        musicRoom->songNameVms[i].pos.y = (((i + 1) * 18) + 104.0f) - 20.0f;
        musicRoom->songNameVms[i].pos.z = 0.0f;
        musicRoom->songNameVms[i].anchor = 3;
    }

    g_ZunMemory.Free(musicCmtFile);

    // TitleScreen starts the loading overlay before handing control to the
    // Music Room. Unlike the other asynchronous screens, MusicRoom has no
    // setup thread to dismiss it, so leaving state 1 active draws the
    // "Now Loading" / "少女祈祷中" VMs forever over the ready screen.
    g_Supervisor.HideLoadingVms();
#ifdef TH08_IOS
    modern::LogStartup("music-room: ready; loading overlay dismissed");
#endif

    return ZUN_SUCCESS;
}

ZunResult MusicRoom::DeletedCallback(MusicRoom *musicRoom)
{
    ZUN_DELETE(musicRoom->trackDescriptors);

    g_AnmManager->ReleaseSurface(0);
    g_AnmManager->ReleaseAnm(23);

    g_Chain.Cut(musicRoom->drawChain);
    musicRoom->drawChain = NULL;

    return ZUN_SUCCESS;
}

// FUNCTION: th08 0x449c79
inline ZunBool Supervisor::IsMusicPreloadEnabled()
{
    return this->cfg.opts.preloadMusic;
}

} /* namespace th08 */
