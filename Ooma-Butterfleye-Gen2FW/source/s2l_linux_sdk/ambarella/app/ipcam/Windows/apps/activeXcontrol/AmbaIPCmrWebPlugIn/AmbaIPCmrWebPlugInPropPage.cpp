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

// AmbaIPCmrWebPlugInPropPage.cpp : Implementation of the CAmbaIPCmrWebPlugInPropPage property page class.

#include "stdafx.h"
#include "AmbaIPCmrWebPlugIn.h"
#include "AmbaIPCmrWebPlugInPropPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNCREATE(CAmbaIPCmrWebPlugInPropPage, COlePropertyPage)



// Message map

BEGIN_MESSAGE_MAP(CAmbaIPCmrWebPlugInPropPage, COlePropertyPage)
END_MESSAGE_MAP()



// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CAmbaIPCmrWebPlugInPropPage, "AMBAIPCMRWEBPL.AmbaIPCmrWebPlPropPage.1",
	0x82e2a79e, 0x4791, 0x41c4, 0x96, 0xe7, 0xbf, 0x30, 0x22, 0x4c, 0x40, 0x72)



// CAmbaIPCmrWebPlugInPropPage::CAmbaIPCmrWebPlugInPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CAmbaIPCmrWebPlugInPropPage

BOOL CAmbaIPCmrWebPlugInPropPage::CAmbaIPCmrWebPlugInPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_AMBAIPCMRWEBPLUGIN_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}



// CAmbaIPCmrWebPlugInPropPage::CAmbaIPCmrWebPlugInPropPage - Constructor

CAmbaIPCmrWebPlugInPropPage::CAmbaIPCmrWebPlugInPropPage() :
	COlePropertyPage(IDD, IDS_AMBAIPCMRWEBPLUGIN_PPG_CAPTION)
{
}



// CAmbaIPCmrWebPlugInPropPage::DoDataExchange - Moves data between page and properties

void CAmbaIPCmrWebPlugInPropPage::DoDataExchange(CDataExchange* pDX)
{
	DDP_PostProcessing(pDX);
}



// CAmbaIPCmrWebPlugInPropPage message handlers
