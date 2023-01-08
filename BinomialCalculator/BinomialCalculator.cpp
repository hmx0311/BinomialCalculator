// BinomialCalculator.cpp : 定义应用程序的入口点。
//
#include "framework.h"
#include "BinomialCalculator.h"

#include "button.h"
#include "edit.h"
#include "listbox.h"

#include "model.h"

#include <windowsx.h>

#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"UxTheme.lib")
#pragma comment(lib,"imm32.lib")

#define DISPLAYED_ITEM_COUNT 5
#define LIST_ITEM_HEIGHT (1.36f * abs(logFont.lfHeight))
#define LARGE_FONT_HEIGHT (-1.42f * abs(logFont.lfHeight))

// 全局变量:
HINSTANCE hInst;                                // 当前实例

HFONT hNormalFont;
HFONT hLargeFont;

HWND hMainDlg;
NumericEdit successProbabilityEdit;
NumSpinEdit numTrialsEdit;
NumSpinEdit numSuccessEdit;
HWND hResultText[2];
HWND hErrorText;
Button clearHistoryResultButton;
ResultList historyResultListBox;

double successProbability = 0;
int numTrials = 0;
int numSuccess = 0;

HWND hBet;
UINT BPC_CONNECTED;
UINT BPC_DISCONNECT;
UINT BPC_PROBABILITY;

// 此代码模块中包含的函数的前向声明:
BOOL                initDlg(HWND);
INT_PTR CALLBACK    dlgProc(HWND, UINT, WPARAM, LPARAM);
void updateSuccessProbability();
void calcProbability();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ PTSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);


	// 执行应用程序初始化:
	SetProcessDPIAware();
	BufferedPaintInit();
	ImmDisableIME(GetCurrentThreadId());

	hInst = hInstance; // 将实例句柄存储在全局变量中
	CreateDialog(hInst, MAKEINTRESOURCE(IDD_BINOMIALCALCULATOR_DIALOG), NULL, dlgProc);
	if (!hMainDlg)
	{
		return FALSE;
	}

	if (_stscanf(lpCmdLine, _T("bet_probability_calculator "
		"HWND=%p "
		"connect_message=%d "
		"disconnect_message=%d "
		"probability_message=%d"),
		&hBet,
		&BPC_CONNECTED,
		&BPC_DISCONNECT,
		&BPC_PROBABILITY) == 4)
	{
		PostMessage(hBet, BPC_CONNECTED, (WPARAM)hMainDlg, 0);
		SetWindowText(hMainDlg, _T("bet - 二项分布计算器"));
	}
	else
	{
		hBet = nullptr;
	}

	MSG msg;

	// 主消息循环:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!IsDialogMessage(hMainDlg, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	BufferedPaintUnInit();

	return (int)msg.wParam;
}

BOOL initDlg(HWND hDlg)
{
	hMainDlg = hDlg;
	HICON hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_BINOMIALCALCULATOR));
	SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

	hNormalFont = GetWindowFont(hDlg);
	LOGFONT logFont;
	GetObject(hNormalFont, sizeof(LOGFONT), &logFont);
	logFont.lfHeight = LARGE_FONT_HEIGHT;
	hLargeFont = CreateFontIndirect(&logFont);

	successProbabilityEdit.attach(GetDlgItem(hDlg, IDC_SUCCESS_PROBABILITY_EDIT));
	numTrialsEdit.attach(GetDlgItem(hDlg, IDC_NUM_TRIALS_EDIT), GetDlgItem(hDlg, IDC_NUM_TRIALS_SPIN));
	numSuccessEdit.attach(GetDlgItem(hDlg, IDC_NUM_SUCCESS_EDIT), GetDlgItem(hDlg, IDC_NUM_SUCCESS_SPIN));
	hResultText[0] = GetDlgItem(hDlg, IDC_L_RESULT_TEXT);
	SetWindowSubclass(hResultText[0], readOnlyEditSubclassProc, 0, 0);
	SetWindowFont(hResultText[0], hLargeFont, FALSE);
	hResultText[1] = GetDlgItem(hDlg, IDC_R_RESULT_TEXT);
	SetWindowSubclass(hResultText[1], readOnlyEditSubclassProc, 0, 0);
	SetWindowFont(hResultText[1], hLargeFont, FALSE);
	hErrorText= GetDlgItem(hDlg, IDC_ERROR_TEXT);
	HWND hCalculateButton = GetDlgItem(hDlg, IDC_CALCULATE_BUTTON);
	SendMessage(hCalculateButton, WM_UPDATEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEFOCUS), 0);
	SetWindowSubclass(hCalculateButton, buttonSubclassProc, 0, 0);
	clearHistoryResultButton.attach(GetDlgItem(hDlg, IDC_CLEAR_HISTORY_RESULT_BUTTON));
	historyResultListBox.attach(GetDlgItem(hDlg, IDC_HISTORY_RESULT_LISTBOX));

	hButtonTheme = OpenThemeData(hDlg, _T("Button"));

	clearHistoryResultButton.setIcon((HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_BIN), IMAGE_ICON, 0, 0, LR_SHARED));

	Edit_LimitText(successProbabilityEdit.getHwnd(), PROB_LEN);
	Edit_LimitText(numTrialsEdit.getHwnd(), NUM_LEN);
	Edit_LimitText(numSuccessEdit.getHwnd(), NUM_LEN);

	// Create the tooltip. hInst is the global instance handle.
	INITCOMMONCONTROLSEX icex = { sizeof(icex),ICC_TREEVIEW_CLASSES };
	InitCommonControlsEx(&icex);
	HWND hwndTip = CreateWindow(TOOLTIPS_CLASS, nullptr,
		WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		hDlg, nullptr,
		hInst, nullptr);

	TCHAR pszText[] = _T("清除历史记录");

	// Associate the tooltip with the tool.
	TOOLINFO toolInfo = { 0 };
	toolInfo.cbSize = sizeof(toolInfo);
	toolInfo.hwnd = hDlg;
	toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	toolInfo.uId = (UINT_PTR)clearHistoryResultButton.getHwnd();
	toolInfo.lpszText = pszText;
	SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);

	return TRUE;
}

INT_PTR CALLBACK dlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == BPC_DISCONNECT)
	{
		PostQuitMessage(0);
		return (INT_PTR)TRUE;
	}
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)initDlg(hDlg);
	case WM_DPICHANGED:
		{
			hNormalFont = GetWindowFont(hDlg);
			LOGFONT logFont;
			GetObject(hNormalFont, sizeof(LOGFONT), &logFont);
			int listItemHeight = LIST_ITEM_HEIGHT;
			logFont.lfHeight = LARGE_FONT_HEIGHT;
			DeleteObject(hLargeFont);
			hLargeFont = CreateFontIndirect(&logFont);

			ListBox_SetItemHeight(historyResultListBox.getHwnd(), 0, listItemHeight);
			int listHeight = 4 + DISPLAYED_ITEM_COUNT * listItemHeight;
			RECT rcList;
			GetWindowRect(historyResultListBox.getHwnd(), &rcList);
			MapWindowRect(HWND_DESKTOP, hDlg, &rcList);
			SetWindowPos(historyResultListBox.getHwnd(), nullptr, 0, 0, rcList.right - rcList.left, listHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW);

			RECT rcDlg;
			GetWindowRect(hDlg, &rcDlg);
			MapWindowRect(HWND_DESKTOP, hDlg, &rcDlg);
			SetWindowPos(hDlg, nullptr, 0, 0, rcList.right + rcList.left - 2 * rcDlg.left, rcList.top + listHeight + rcList.left - rcDlg.top - rcDlg.left, SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW);

			SetWindowFont(hResultText[0], hLargeFont, FALSE);
			SetWindowFont(hResultText[1], hLargeFont, FALSE);
			break;
		}
	case WM_THEMECHANGED:
		CloseThemeData(hButtonTheme);
		hButtonTheme = OpenThemeData(hDlg, _T("Button"));
		InvalidateRect(hDlg, nullptr, TRUE);
		return (INT_PTR)TRUE;
	case WM_CTLCOLORSTATIC:
		if ((HWND)lParam == hErrorText)
		{
			SetBkColor((HDC)wParam, GetSysColor(COLOR_BTNFACE));
			SetTextColor((HDC)wParam, RGB(255, 0, 0));
			return (INT_PTR)GetSysColorBrush(COLOR_BTNFACE);
		}
		break;
	case WM_DRAWITEM:
		{
			PDRAWITEMSTRUCT pDrawItemStruct = (PDRAWITEMSTRUCT)lParam;
			switch (pDrawItemStruct->CtlType)
			{
			case ODT_LISTBOX:
				((ResultList*)GetWindowLongPtr(pDrawItemStruct->hwndItem, GWLP_USERDATA))->drawItem(pDrawItemStruct->hDC, pDrawItemStruct->itemID, pDrawItemStruct->itemState, pDrawItemStruct->rcItem);
				return (INT_PTR)TRUE;
			}
			break;
		}
	case WM_MEASUREITEM:
		{
			HFONT hFont = GetWindowFont(hDlg);
			LOGFONT logFont;
			GetObject(hFont, sizeof(LOGFONT), &logFont);
			((PMEASUREITEMSTRUCT)lParam)->itemHeight = LIST_ITEM_HEIGHT;
			return (INT_PTR)TRUE;
		}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_RETRIEVE_RESULT:
			{
				TCHAR successProbabilityStr[PROB_LEN + 1], numTrialsStr[NUM_LEN + 1], numSuccessStr[NUM_LEN + 1];
				if (_stscanf((LPTSTR)lParam, _T("0.%" STR(PROB_LEN) "s %" STR(NUM_LEN) "s %" STR(NUM_LEN) "s"), successProbabilityStr, numTrialsStr, numSuccessStr) != 3)
				{
					return (INT_PTR)TRUE;
				}
				successProbabilityEdit.setText(successProbabilityStr);
				updateSuccessProbability();
				numTrialsEdit.setText(numTrialsStr);
				numTrials = numTrialsEdit.updateNum();
				numSuccessEdit.setText(numSuccessStr);
				numSuccess = numSuccessEdit.updateNum();
				double cumulativeProbability = binomialCumulativeProbability(successProbability, numTrials, numSuccess);
				if (hBet != nullptr)
				{
					PostMessage(hBet, BPC_PROBABILITY, (WPARAM)hMainDlg, *(LPARAM*)(&cumulativeProbability));
				}
				TCHAR str[PROB_LEN + 3];
				_stprintf(str, _T("%." STR(PROB_LEN) "f"), cumulativeProbability);
				SetWindowText(hResultText[0], str);
				ShowWindow(hResultText[0], SW_SHOW);
				_stprintf(str, _T("%." STR(PROB_LEN) "f"), 1 - cumulativeProbability);
				SetWindowText(hResultText[1], str);
				ShowWindow(hResultText[1], SW_SHOW);
				return (INT_PTR)TRUE;
			}
		case IDC_SUCCESS_PROBABILITY_EDIT:
			switch (HIWORD(wParam))
			{
			case EN_KILLFOCUS:
				updateSuccessProbability();
				return (INT_PTR)TRUE;
			case EN_CHANGE:
				ShowWindow(hResultText[0], SW_HIDE);
				ShowWindow(hResultText[1], SW_HIDE);
				ShowWindow(hErrorText, SW_HIDE);
				return (INT_PTR)TRUE;
			}
			break;
		case IDC_NUM_TRIALS_EDIT:
			switch (HIWORD(wParam))
			{
			case EN_CHANGE:
				numTrials = numTrialsEdit.updateNum();
				ShowWindow(hResultText[0], SW_HIDE);
				ShowWindow(hResultText[1], SW_HIDE);
				ShowWindow(hErrorText, SW_HIDE);
				return (INT_PTR)TRUE;
			}
			break;
		case IDC_NUM_SUCCESS_EDIT:
			switch (HIWORD(wParam))
			{
			case EN_CHANGE:
				numSuccess = numSuccessEdit.updateNum();
				ShowWindow(hResultText[0], SW_HIDE);
				ShowWindow(hResultText[1], SW_HIDE);
				ShowWindow(hErrorText, SW_HIDE);
				return (INT_PTR)TRUE;
			}
			break;
		case IDC_CALCULATE_BUTTON:
			calcProbability();
			return (INT_PTR)TRUE;
		case IDC_CLEAR_HISTORY_RESULT_BUTTON:
			historyResultListBox.reset();
			return (INT_PTR)TRUE;
		}
		break;
	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->idFrom)
		{
		case IDC_NUM_TRIALS_SPIN:
			{
				numTrials -= ((LPNMUPDOWN)lParam)->iDelta;
				if (numTrials < 1)
				{
					numTrials = 1;
				}
				else if (numTrials > MAX_DECIMAL(NUM_LEN))
				{
					numTrials = MAX_DECIMAL(NUM_LEN);
				}
				ShowWindow(hResultText[0], SW_HIDE);
				ShowWindow(hResultText[1], SW_HIDE);
				ShowWindow(hErrorText, SW_HIDE);
				TCHAR str[NUM_LEN + 1];
				_itow(numTrials, str, 10);
				SetWindowText(numTrialsEdit.getHwnd(), str);
				Edit_SetSel(numTrialsEdit.getHwnd(), lstrlen(str), -1);
				SetFocus(numTrialsEdit.getHwnd());
				numTrialsEdit.updateSpin();
				break;
			}
		case IDC_NUM_SUCCESS_SPIN:
			{
				numSuccess -= ((LPNMUPDOWN)lParam)->iDelta;
				if (numSuccess < 1)
				{
					numSuccess = 1;
				}
				else if (numSuccess > MAX_DECIMAL(NUM_LEN))
				{
					numSuccess = MAX_DECIMAL(NUM_LEN);
				}
				ShowWindow(hResultText[0], SW_HIDE);
				ShowWindow(hResultText[1], SW_HIDE);
				ShowWindow(hErrorText, SW_HIDE);
				TCHAR str[NUM_LEN + 1];
				_itow(numSuccess, str, 10);
				SetWindowText(numSuccessEdit.getHwnd(), str);
				Edit_SetSel(numSuccessEdit.getHwnd(), lstrlen(str), -1);
				SetFocus(numSuccessEdit.getHwnd());
				numSuccessEdit.updateSpin();
				break;
			}
		}
		break;
	case WM_CLOSE:
		if (hBet != nullptr)
		{
			PostMessage(hBet, BPC_DISCONNECT, (WPARAM)hMainDlg, 0);
		}
		CloseThemeData(hButtonTheme);
		PostQuitMessage(0);
		break;
	}
	return (INT_PTR)FALSE;
}

void updateSuccessProbability()
{
	TCHAR str[PROB_LEN + 1];
	GetWindowText(successProbabilityEdit.getHwnd(), str, PROB_LEN + 1);
	successProbability = 0;
	for (int i = lstrlen(str) - 1; i >= 0; i--)
	{
		int c = str[i] - '0';
		successProbability = (successProbability + c) * 0.1;
	}
}

void calcProbability()
{
	if (successProbability == 0)
	{
		SetWindowText(hErrorText, _T("成功概率不能为0"));
		ShowWindow(hErrorText, SW_SHOW);
		SetFocus(successProbabilityEdit.getHwnd());
		Edit_SetSel(successProbabilityEdit.getHwnd(), 0, INT_MAX);
		return;
	}
	if (numTrials == 0)
	{
		SetWindowText(hErrorText, _T("试验次数不能为0"));
		ShowWindow(hErrorText, SW_SHOW);
		SetFocus(numTrialsEdit.getHwnd());
		Edit_SetSel(numTrialsEdit.getHwnd(), 0, INT_MAX);
		return;
	}
	if (numSuccess == 0)
	{
		SetWindowText(hErrorText, _T("成功次数不能为0"));
		ShowWindow(hErrorText, SW_SHOW);
		SetFocus(numSuccessEdit.getHwnd());
		Edit_SetSel(numSuccessEdit.getHwnd(), 0, INT_MAX);
		return;
	}
	if (numSuccess > numTrials)
	{
		SetWindowText(hErrorText, _T("成功次数不能大于试验次数"));
		ShowWindow(hErrorText, SW_SHOW);
		SetFocus(numSuccessEdit.getHwnd());
		Edit_SetSel(numSuccessEdit.getHwnd(), 0, INT_MAX);
		return;
	}
	double cumulativeProbability = binomialCumulativeProbability(successProbability, numTrials, numSuccess);
	if (hBet != nullptr)
	{
		PostMessage(hBet, BPC_PROBABILITY, (WPARAM)hMainDlg, *(LPARAM*)(&cumulativeProbability));
	}
	TCHAR str[3 * (PROB_LEN + 2) + 2 * NUM_LEN + 7];
	_stprintf(str, _T("%." STR(PROB_LEN) "f %" STR(NUM_LEN) "d %" STR(NUM_LEN) "d  %." STR(PROB_LEN) "f  %." STR(PROB_LEN) "f"),
		successProbability, numTrials, numSuccess, cumulativeProbability, 1 - cumulativeProbability);
	historyResultListBox.addResult(str);
	str[2*(PROB_LEN + 2) + 2 * NUM_LEN + 4]='\0';
	ShowWindow(hResultText[0], SW_SHOW);
	SetWindowText(hResultText[0], str + (PROB_LEN + 2 + 2 * NUM_LEN + 4));
	ShowWindow(hResultText[1], SW_SHOW);
	SetWindowText(hResultText[1], str + (2*(PROB_LEN + 2) + 2 * NUM_LEN + 6));
}

