/**
 * simple_player.h
 *
 * History:
 *    2015/01/09 - [Zhi He] create file
 *
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


#ifndef __SIMPLE_PLAYER_H__
#define __SIMPLE_PLAYER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

class CISimplePlayer: public ISimplePlayer
{
protected:
    CISimplePlayer();
    virtual ~CISimplePlayer();

public:
    static CISimplePlayer *Create();

public:
    virtual void Destroy();
    virtual int Initialize(void *owner = NULL, void *p_external_msg_queue = NULL);

public:
    virtual int Play(char *url, char *title_name = NULL);

public:
    void MainLoop();

private:
    EECode setupContext();
    EECode startContext();
    EECode stopContext();
    void destroyContext();

private:
    EECode processMsgLoop();
    EECode waitPlay();

private:
    int loadCurrentConfig(const char *configfile);
    int storeCurrentConfig(const char *configfile);
    int checkWithSystemSettings();

private:
    SMediaSimpleAPIContext mMediaAPIContxt;
    IThread *mpMainThread;
    CIQueue *mpMsgQueue;
    CIQueue *mpExternalMsgQueue;
    CMediaSimpleAPI *mpMediaAPI;
    IGenericEngineControlV4 *mpMediaEngineAPI;
    IClockSource *mpClockSource;
    CIClockManager *mpClockManager;
    CIClockReference *mpClockReference;
    ISceneDirector *mpDirector;
    CIScene *mpScene;
    CIGUILayer *mpLayer;
    IVisualDirectRendering *mpVisualDirectRendering;
    CGUIArea *mpTexture;

    ISoundComposer *mpSoundComposer;
    CISoundTrack *mpSoundTrack;
    ISoundDirectRendering *mpSoundDirectRendering;

    int mbSetupContext;
    int mbStartContext;
    int mbThreadStarted;
    int mbReconnecting;
    int mbExit;

private:
    TDimension mPbMainWindowWidth;
    TDimension mPbMainWindowHeight;

    char mPreferDisplay[DMaxPreferStringLen];
    char mPreferPlatform[DMaxPreferStringLen];

private:
    char mWindowTitleName[256];
    void *mpOwner;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

