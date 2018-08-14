/**
 * simple_capserver.h
 *
 * History:
 *    2015/01/05 - [Zhi He] create file
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


#ifndef __SIMPLE_CAPSERVER_H__
#define __SIMPLE_CAPSERVER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

class CISimpleCapServer : public ISimpleCapServer
{
protected:
    CISimpleCapServer();
    virtual ~CISimpleCapServer();

public:
    static CISimpleCapServer *Create();

public:
    virtual void Destroy();
    virtual int Initialize();

public:
    virtual int Start();
    virtual int Stop();

public:
    virtual void Mute();
    virtual void UnMute();

private:
    void loadCurrentConfig(const char *configfile);
    void storeCurrentConfig(const char *configfile);

    EECode checkWithSystemSettings();
    EECode loadAudioDevices();

    EECode setupMediaAPI();
    void destroyMediaAPI();

private:
    SMediaSimpleAPIContext mMediaAPIContxt;
    SSystemSoundInputDevices mSystemSoundInputDevices;

    CMediaSimpleAPI *mpMediaSimpleAPI;

    TU8 mCurrentSoundInputDeviceIndex;
    TU8 mbSetupMediaContext;
    TU8 mbRunning;
    TU8 mbStarted;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

