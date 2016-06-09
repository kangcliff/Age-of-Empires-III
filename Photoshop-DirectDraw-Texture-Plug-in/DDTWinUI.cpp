// Copyright (c) 2016 Cliff Kang
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//
//

#include "DDTUI.h"
#include <CommCtrl.h>

#ifdef _WIN64
INT_PTR
#else
BOOL
#endif
CALLBACK DDTOptionsDialogProc(HWND hWnd, UINT msg,
	WPARAM wParam, LPARAM lParam) {
	switch (msg)
	{
	case WM_INITDIALOG:
	{
		SendMessage(GetDlgItem(hWnd, IDC_RADIO_DIFFUSE1), BM_SETCHECK, TRUE, 0);
		SendMessage(GetDlgItem(hWnd, IDC_RADIO_NONE), BM_SETCHECK, TRUE, 0);
		if (gFormatRecord->imageMode != plugInModeGrayScale)
			SendMessage(GetDlgItem(hWnd, IDC_RADIO_DXT3), BM_SETCHECK, TRUE, 0);

		if (gFormatRecord->imageMode == plugInModeGrayScale) {
			SendMessage(GetDlgItem(hWnd, IDC_RADIO_BGRA), BM_SETCHECK, TRUE, 0);

			EnableWindow(GetDlgItem(hWnd, IDC_RADIO_DIFFUSE1), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_RADIO_DIFFUSE2), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_RADIO_BUMP1), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_RADIO_BUMP2), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_RADIO_NONE), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_RADIO_PLAYERCOLOR), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_RADIO_TRANSPARENT), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_RADIO_BLEND), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_RADIO_BGRA), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_RADIO_DXT1), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_RADIO_DXT3), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_RADIO_DXT5), FALSE);
		}
		else
			EnableWindow(GetDlgItem(hWnd, IDC_RADIO_DXT5), FALSE);

		SendMessage(GetDlgItem(hWnd, IDC_CHECK_MIPMAP), BM_SETCHECK, TRUE, 0);
		SendMessage(GetDlgItem(hWnd, IDC_SLIDER_MIPMAP_SHARPNESS), TBM_SETRANGE, TRUE, MAKELONG(0, 20));
		SendMessage(GetDlgItem(hWnd, IDC_SLIDER_MIPMAP_SHARPNESS), TBM_SETPOS, TRUE, 5);
	}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			*gResult = userCanceledErr;
			EndDialog(hWnd, 0);
			return FALSE;
			break;
		case IDOK:
			gPlugInData->Options.ImageCount = 1;

			if (gFormatRecord->imageMode == plugInModeGrayScale) {
				gPlugInData->Options.TextureUsage = DDT_DIFFUSE;
				gPlugInData->Options.TextureAlphaUsage = DDT_NONE;
				gPlugInData->Options.TextureType = DDT_GREYSCALE;
			}
			else
			{
				gPlugInData->Options.TextureUsage = SendMessage(GetDlgItem(hWnd, IDC_RADIO_DIFFUSE1), BM_GETCHECK, 0, 0) ? DDT_DIFFUSE :
					SendMessage(GetDlgItem(hWnd, IDC_RADIO_DIFFUSE2), BM_GETCHECK, 0, 0) ? DDT_DIFFUSE2 :
					SendMessage(GetDlgItem(hWnd, IDC_RADIO_BUMP1), BM_GETCHECK, 0, 0) ? DDT_BUMP : DDT_BUMP2;
				gPlugInData->Options.TextureAlphaUsage = SendMessage(GetDlgItem(hWnd, IDC_RADIO_NONE), BM_GETCHECK, 0, 0) ? DDT_NONE :
					SendMessage(GetDlgItem(hWnd, IDC_RADIO_PLAYERCOLOR), BM_GETCHECK, 0, 0) ? DDT_PLAYER :
					SendMessage(GetDlgItem(hWnd, IDC_RADIO_TRANSPARENT), BM_GETCHECK, 0, 0) ? DDT_TRANSPARENT : DDT_BLEND;
				gPlugInData->Options.TextureType = SendMessage(GetDlgItem(hWnd, IDC_RADIO_BGRA), BM_GETCHECK, 0, 0) ? DDT_BGRA :
					SendMessage(GetDlgItem(hWnd, IDC_RADIO_DXT1), BM_GETCHECK, 0, 0) ? DDT_DXT1 :
					SendMessage(GetDlgItem(hWnd, IDC_RADIO_DXT3), BM_GETCHECK, 0, 0) ? DDT_DXT3 : DDT_DXT5;

			}

			if (SendMessage(GetDlgItem(hWnd, IDC_CHECK_MIPMAP), BM_GETCHECK, 0, 0)) {
				while ((min(gFormatRecord->imageSize.h, gFormatRecord->imageSize.v) >>
					gPlugInData->Options.ImageCount) >= 2)
					gPlugInData->Options.ImageCount++;
			}

			gPlugInData->Options.MIPMapSharpness = static_cast<float>(SendMessage(GetDlgItem(hWnd, IDC_SLIDER_MIPMAP_SHARPNESS), TBM_GETPOS, 0, 0)) / 20;

			EndDialog(hWnd, 0);
			return TRUE;
			break;
		case IDC_RADIO_DIFFUSE1:
		case IDC_RADIO_DIFFUSE2:
			switch (HIWORD(wParam))
			{
			case BN_CLICKED:
				if (gFormatRecord->imageMode != plugInModeGrayScale) {
					if (!SendMessage(GetDlgItem(hWnd, IDC_RADIO_DXT1), BM_GETCHECK, 0, 0) &&
						!SendMessage(GetDlgItem(hWnd, IDC_RADIO_DXT3), BM_GETCHECK, 0, 0)) {
						EnableWindow(GetDlgItem(hWnd, IDC_RADIO_DXT1), TRUE);
						EnableWindow(GetDlgItem(hWnd, IDC_RADIO_DXT3), TRUE);
						EnableWindow(GetDlgItem(hWnd, IDC_RADIO_DXT5), FALSE);
						if (!SendMessage(GetDlgItem(hWnd, IDC_RADIO_BGRA), BM_GETCHECK, 0, 0)) {
							SendMessage(GetDlgItem(hWnd, IDC_RADIO_DXT1), BM_SETCHECK, FALSE, 0);
							SendMessage(GetDlgItem(hWnd, IDC_RADIO_DXT3), BM_SETCHECK, TRUE, 0);
							SendMessage(GetDlgItem(hWnd, IDC_RADIO_DXT5), BM_SETCHECK, FALSE, 0);
						}
					}
				}
			}
			break;
		case IDC_RADIO_BUMP1:
		case IDC_RADIO_BUMP2:
			switch (HIWORD(wParam))
			{
			case BN_CLICKED:
				if (gFormatRecord->imageMode != plugInModeGrayScale) {
					if (!SendMessage(GetDlgItem(hWnd, IDC_RADIO_DXT5), BM_GETCHECK, 0, 0)) {
						EnableWindow(GetDlgItem(hWnd, IDC_RADIO_DXT1), FALSE);
						EnableWindow(GetDlgItem(hWnd, IDC_RADIO_DXT3), FALSE);
						EnableWindow(GetDlgItem(hWnd, IDC_RADIO_DXT5), TRUE);
						if (!SendMessage(GetDlgItem(hWnd, IDC_RADIO_BGRA), BM_GETCHECK, 0, 0)) {
							SendMessage(GetDlgItem(hWnd, IDC_RADIO_DXT1), BM_SETCHECK, FALSE, 0);
							SendMessage(GetDlgItem(hWnd, IDC_RADIO_DXT3), BM_SETCHECK, FALSE, 0);
							SendMessage(GetDlgItem(hWnd, IDC_RADIO_DXT5), BM_SETCHECK, TRUE, 0);
						}
					}
				}
			}
			break;
		case IDC_CHECK_MIPMAP:
			switch (HIWORD(wParam))
			{
			case BN_CLICKED:
				if (SendMessage(GetDlgItem(hWnd, IDC_CHECK_MIPMAP), BM_GETCHECK, 0, 0)) {
					EnableWindow(GetDlgItem(hWnd, IDC_MIPMAP_SHARPNESS), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_SLIDER_MIPMAP_SHARPNESS), TRUE);
					EnableWindow(GetDlgItem(hWnd, IDC_MIPMAP_SHARPNESS_PERCENTAGE), TRUE);
				}
				else {
					EnableWindow(GetDlgItem(hWnd, IDC_MIPMAP_SHARPNESS), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_SLIDER_MIPMAP_SHARPNESS), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDC_MIPMAP_SHARPNESS_PERCENTAGE), FALSE);
				}
			}
		}
		break;
	case WM_NOTIFY:
		switch (LOWORD(wParam))
		{
		case IDC_SLIDER_MIPMAP_SHARPNESS:
			TCHAR szProgress[5];
			wsprintf(szProgress, TEXT("%d %%"),
				5 * SendMessage(GetDlgItem(hWnd, IDC_SLIDER_MIPMAP_SHARPNESS), TBM_GETPOS, 0, 0));
			SetDlgItemText(hWnd, IDC_MIPMAP_SHARPNESS_PERCENTAGE, szProgress);
		}
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void DDTCreateOptionsDialog()
{
	SPAccessSuite* sSPAccessSuite;
	SPAccessRef sSPAccess = NULL;
	SPPlatformAccessInfo sSPPlatformAccessInfo;

	HINSTANCE hDLLInstance;

	*gResult = gFormatRecord->sSPBasic->AcquireSuite(kSPAccessSuite, kSPAccessSuiteVersion, (const void**)&sSPAccessSuite);

	if (*gResult != noErr)
		return;

	*gResult = sSPAccessSuite->GetPluginAccess(reinterpret_cast<SPPluginRef>(gFormatRecord->plugInRef), &sSPAccess);

	if (*gResult != noErr || sSPAccess == NULL) {
		*gResult = gFormatRecord->sSPBasic->ReleaseSuite(kSPAccessSuite, kSPAccessSuiteVersion);
		return;
	}

	*gResult = sSPAccessSuite->GetAccessInfo(sSPAccess, &sSPPlatformAccessInfo);

	if (*gResult != noErr) {
		*gResult = gFormatRecord->sSPBasic->ReleaseSuite(kSPAccessSuite, kSPAccessSuiteVersion);
		return;
	}

	if (sSPPlatformAccessInfo.resourceAccess != NULL)
		hDLLInstance = reinterpret_cast<HINSTANCE>(sSPPlatformAccessInfo.resourceAccess);
	else
		hDLLInstance = reinterpret_cast<HINSTANCE>(sSPPlatformAccessInfo.defaultAccess);

	*gResult = gFormatRecord->sSPBasic->ReleaseSuite(kSPAccessSuite, kSPAccessSuiteVersion);

	if (*gResult != noErr)
		return;

	DialogBox(hDLLInstance, MAKEINTRESOURCE(IDD_DIALOG_OPTIONS),
		reinterpret_cast<HWND>(reinterpret_cast<PlatformData*>(gFormatRecord->platformData)->hwnd),
		DDTOptionsDialogProc);
}