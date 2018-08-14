 /*
 * History:
 *	2016/12/30 - [JianTang] Created file
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


#ifndef __AMBA_PAGE_H__
#define __AMBA_PAGE_H__

class CAmbaEncodeFormatProp: public CBasePropertyPage
{
	typedef CBasePropertyPage inherited;

public:
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
	CAmbaEncodeFormatProp(LPUNKNOWN lpunk, HRESULT *phr);

	virtual HRESULT OnConnect(IUnknown *punk);
	virtual HRESULT OnDisconnect();

	virtual HRESULT OnActivate();
	virtual HRESULT OnDeactivate();
	virtual HRESULT OnApplyChanges();

	virtual INT_PTR OnReceiveMessage(HWND hwnd, UINT uMsg,
		WPARAM wParam, LPARAM lParam);

private:
	void SetDirty();
	int EditGetInt(HWND hwnd);
	int EnableWindowById(WORD id, BOOL enable);

	HRESULT GetAllInfo();
	HRESULT SendAllInfo();

private:
	IAmbaRecordControl *m_pRecordControl;
	IAmbaRecordControl::FORMAT *m_pFormat;
};

class CAmbaEncodeParamProp: public CBasePropertyPage
{
	typedef CBasePropertyPage inherited;

public:
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
	CAmbaEncodeParamProp(LPUNKNOWN lpunk, HRESULT *phr);

	virtual HRESULT OnConnect(IUnknown *punk);
	virtual HRESULT OnDisconnect();

	virtual HRESULT OnActivate();
	virtual HRESULT OnDeactivate();
	virtual HRESULT OnApplyChanges();

	virtual INT_PTR OnReceiveMessage(HWND hwnd, UINT uMsg,
		WPARAM wParam, LPARAM lParam); 

private:
	void SetConfig(IAmbaRecordControl::H264_PARAM *pParam,
		WORD *pControls, DWORD numControls, BOOL enable);
	void SetLowDelay(IAmbaRecordControl::H264_PARAM *pParam, WORD *pControls);

private:
	void SetDirty();
	int EditGetInt(HWND hwnd);

private:
	IAmbaRecordControl *m_pRecordControl;
	IAmbaRecordControl::FORMAT *m_pFormat;
	IAmbaRecordControl::PARAM *m_pParam;
};

class CAmbaImgParamProp: public CBasePropertyPage
{
	typedef CBasePropertyPage inherited;

public:
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
	CAmbaImgParamProp(LPUNKNOWN lpunk, HRESULT *phr);

	virtual HRESULT OnConnect(IUnknown *punk);
	virtual HRESULT OnDisconnect();

	virtual HRESULT OnActivate();
	virtual HRESULT OnDeactivate();
	virtual HRESULT OnApplyChanges();

	virtual INT_PTR OnReceiveMessage(HWND hwnd, UINT uMsg,
		WPARAM wParam, LPARAM lParam); 

private:
	void SetDirty();
	int EditGetInt(HWND hwnd);

private:
	IAmbaRecordControl *m_pRecordControl;
	IAmbaRecordControl::IMG_PARAM *m_pImgParam;
};

class CAmbaMdParamProp: public CBasePropertyPage
{
	typedef CBasePropertyPage inherited;

public:
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
	CAmbaMdParamProp(LPUNKNOWN lpunk, HRESULT *phr);

	virtual HRESULT OnConnect(IUnknown *punk);
	virtual HRESULT OnDisconnect();

	virtual HRESULT OnActivate();
	virtual HRESULT OnDeactivate();
	virtual HRESULT OnApplyChanges();

	virtual INT_PTR OnReceiveMessage(HWND hwnd, UINT uMsg,
		WPARAM wParam, LPARAM lParam); 

private:
	void SetDirty();
	int EditGetInt(HWND hwnd);

private:
	IAmbaRecordControl *m_pRecordControl;
	IAmbaRecordControl::MD_PARAM *m_pMdParam;
};

class CAmbaAboutProp: public CBasePropertyPage
{
	typedef CBasePropertyPage inherited;

public:
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
	CAmbaAboutProp(LPUNKNOWN lpunk, HRESULT *phr);
};

class CAmbaMuxerParamProp: public CBasePropertyPage
{
	typedef CBasePropertyPage inherited;

public:
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
	CAmbaMuxerParamProp(LPUNKNOWN lpunk, HRESULT *phr);

	virtual HRESULT OnConnect(IUnknown *punk);
	virtual HRESULT OnDisconnect();

	virtual HRESULT OnActivate();
	virtual HRESULT OnDeactivate();
	virtual HRESULT OnApplyChanges();

	virtual INT_PTR OnReceiveMessage(HWND hwnd, UINT uMsg,
		WPARAM wParam, LPARAM lParam); 

private:
	void SetDirty();
	int EditGetInt(HWND hwnd);
	int EnableWindowById(WORD id, BOOL enable);

	HRESULT GetAllInfo();
	HRESULT SendAllInfo();
	void OnButtonBrowse();

private:
	IAmbaRecordControl *m_pRecordControl;
	IAmbaRecordControl::FORMAT *m_pFormat;
};


#endif // __AMBA_PAGE_H__

