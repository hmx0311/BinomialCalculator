// BinomialCalculator.cpp : 定义应用程序的入口点。
//
#include "framework.h"
#include "BinomialCalculator.h"

#include "button.h"
#include "edit.h"
#include "listbox.h"

#include "model.h"

#include <CommCtrl.h>
#include <windowsx.h>

#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"UxTheme.lib")

#define MAX_LOADSTRING 100

#define DISPLAYED_ITEM_COUNT 5
#define LIST_ITEM_HEIGHT (1.36f * abs(logFont.lfHeight))
#define LARGE_FONT_HEIGHT (-1.2f * abs(logFont.lfHeight) - 2.5f)

#define PROB_LEN 4
#define NUM_LEN 8
#define SPIN_LEN 4
#define _STR(x) #x
#define STR(x) _STR(x)
#define MAX_DECIMAL(x) (int)(1e ## x-1)

// 全局变量:
HINSTANCE hInst;                                // 当前实例

HFONT hNormalFont;
HFONT hLargeFont;

HWND hMainDlg;
NumericEdit successProbabilityEdit;
NumericEdit numTrialsEdit;
HWND hNumTrialsSpin;
NumericEdit numSuccessEdit;
HWND hNumSuccessSpin;
HWND hResultText;
Button calculateButton;
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
void showEditSpin(HWND hSpin, HWND hEdit);
void hideEditSpin(HWND hSpin, HWND hEdit);
void updateSuccessProbability();
void updateNumTrials();
void updateNumSuccess();
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

	hInst = hInstance; // 将实例句柄存储在全局变量中
	CreateDialog(hInst, MAKEINTRESOURCE(IDD_BINOMIALCALCULATOR_DIALOG), NULL, dlgProc);
	if (!hMainDlg)
	{
		return FALSE;
	}

	if (swscanf(lpCmdLine, _T("bet_probability_calculator "
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

	MSG msg;

	// 主消息循环:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		switch (msg.message)
		{
		case WM_KEYDOWN:
			switch (msg.wParam)
			{
			case VK_RETURN:
				switch (GetDlgCtrlID(GetFocus()))
				{
				case IDC_SUCCESS_PROBABILITY_EDIT:
					updateSuccessProbability();
					break;
				}
				calcProbability();
				break;
			case VK_ESCAPE:
				continue;
			}
			break;
		}
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

	hButtonTheme = OpenThemeData(GetDlgItem(hDlg, IDC_CALCULATE_BUTTON), _T("Button"));

	successProbabilityEdit.attach(GetDlgItem(hDlg, IDC_SUCCESS_PROBABILITY_EDIT));
	numTrialsEdit.attach(GetDlgItem(hDlg, IDC_NUM_TRIALS_EDIT));
	hNumTrialsSpin = GetDlgItem(hDlg, IDC_NUM_TRIALS_SPIN);
	numSuccessEdit.attach(GetDlgItem(hDlg, IDC_NUM_SUCCESS_EDIT));
	hNumSuccessSpin = GetDlgItem(hDlg, IDC_NUM_SUCCESS_SPIN);
	hResultText = GetDlgItem(hDlg, IDC_RESULT_TEXT);
	SetWindowSubclass(hResultText, readOnlyEditSubclassProc, 0, 0);
	calculateButton.attach(GetDlgItem(hDlg, IDC_CALCULATE_BUTTON));
	clearHistoryResultButton.attach(GetDlgItem(hDlg, IDC_CLEAR_HISTORY_RESULT_BUTTON));
	historyResultListBox.attach(GetDlgItem(hDlg, IDC_HISTORY_RESULT_LISTBOX));

	clearHistoryResultButton.setIcon((HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_BIN), IMAGE_ICON, 0, 0, LR_SHARED));

	Edit_LimitText(numTrialsEdit.getHwnd(), NUM_LEN);
	Edit_LimitText(numSuccessEdit.getHwnd(), NUM_LEN);

	showEditSpin(hNumTrialsSpin, numTrialsEdit.getHwnd());
	SendMessage(hNumTrialsSpin, UDM_SETBUDDY, (WPARAM)numTrialsEdit.getHwnd(), 0);
	showEditSpin(hNumSuccessSpin, numSuccessEdit.getHwnd());
	SendMessage(hNumSuccessSpin, UDM_SETBUDDY, (WPARAM)numSuccessEdit.getHwnd(), 0);

	// Create the tooltip. hInst is the global instance handle.
	INITCOMMONCONTROLSEX icex = { sizeof(icex),ICC_TREEVIEW_CLASSES };
	InitCommonControlsEx(&icex);
	HWND hwndTip = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL,
		WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		hDlg, NULL,
		hInst, NULL);

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

			if (IsWindowVisible(hNumTrialsSpin))
			{
				showEditSpin(hNumTrialsSpin, numTrialsEdit.getHwnd());
			}
			if (IsWindowVisible(hNumSuccessSpin))
			{
				showEditSpin(hNumSuccessSpin, numSuccessEdit.getHwnd());
			}

			RECT rcDlg;
			GetWindowRect(hDlg, &rcDlg);
			MapWindowRect(HWND_DESKTOP, hDlg, &rcDlg);
			SetWindowPos(hDlg, nullptr, 0, 0, rcList.right + rcList.left - 2 * rcDlg.left, rcList.top + listHeight + rcList.left - rcDlg.top - rcDlg.left, SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW);

			if (GetWindowStyle(hResultText) & SS_CENTER)
			{
				SetWindowFont(hResultText, hLargeFont, FALSE);
			}
			break;
		}
	case WM_THEMECHANGED:
		CloseThemeData(hButtonTheme);
		hButtonTheme = OpenThemeData(calculateButton.getHwnd(), _T("Button"));
		InvalidateRect(hDlg, nullptr, TRUE);
		return (INT_PTR)TRUE;
	case WM_CTLCOLORSTATIC:
		if ((HWND)lParam == hResultText && !(GetWindowStyle(hResultText) & SS_CENTER))
		{
			SetBkColor((HDC)wParam, GetSysColor(COLOR_BTNFACE));
			SetTextColor((HDC)wParam, RGB(255, 0, 0));
			return (INT_PTR)GetSysColorBrush(COLOR_BTNFACE);
		}
		break;
	case WM_DRAWITEM:
		{
			PDRAWITEMSTRUCT pDrawItemStruct = (PDRAWITEMSTRUCT)lParam;
			HDC hDC = pDrawItemStruct->hDC;
			if (BufferedPaintRenderAnimation(pDrawItemStruct->hwndItem, hDC))
			{
				return (INT_PTR)TRUE;
			}
			switch (pDrawItemStruct->CtlType)
			{
			case ODT_BUTTON:
				((Button*)GetWindowLongPtr(pDrawItemStruct->hwndItem, GWLP_USERDATA))->drawItem(hDC, pDrawItemStruct->itemState, pDrawItemStruct->rcItem);
				return (INT_PTR)TRUE;
			case ODT_LISTBOX:
				((ResultList*)GetWindowLongPtr(pDrawItemStruct->hwndItem, GWLP_USERDATA))->drawItem(hDC, pDrawItemStruct->itemID, pDrawItemStruct->itemState, pDrawItemStruct->rcItem);
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
				updateNumTrials();
				numSuccessEdit.setText(numSuccessStr);
				updateNumSuccess();
				double cumulativeProbability = binomialCumulativeProbability(successProbability, numTrials, numSuccess);
				if (hBet != nullptr)
				{
					PostMessage(hBet, BPC_PROBABILITY, (WPARAM)hMainDlg, *(LPARAM*)(&cumulativeProbability));
				}
				TCHAR str[2 * (PROB_LEN + 2) + 3];
				_stprintf(str, _T("%." STR(PROB_LEN) "f  %." STR(PROB_LEN) "f"), cumulativeProbability, 1 - cumulativeProbability);
				SetWindowFont(hResultText, hLargeFont, FALSE);
				SetWindowText(hResultText, str);
				return (INT_PTR)TRUE;
			}
		case IDC_SUCCESS_PROBABILITY_EDIT:
			switch (HIWORD(wParam))
			{
			case EN_KILLFOCUS:
				updateSuccessProbability();
				return (INT_PTR)TRUE;
			case EN_CHANGE:
				SetWindowText(hResultText, _T(""));
				return (INT_PTR)TRUE;
			}
			break;
		case IDC_NUM_TRIALS_EDIT:
			switch (HIWORD(wParam))
			{
			case EN_CHANGE:
				updateNumTrials();
				SetWindowText(hResultText, _T(""));
				return (INT_PTR)TRUE;
			}
			break;
		case IDC_NUM_SUCCESS_EDIT:
			switch (HIWORD(wParam))
			{
			case EN_CHANGE:
				updateNumSuccess();
				SetWindowText(hResultText, _T(""));
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
				if (numTrials > MAX_DECIMAL(SPIN_LEN))
				{
					hideEditSpin(hNumTrialsSpin, numTrialsEdit.getHwnd());
				}
				else if (!IsWindowVisible(hNumTrialsSpin))
				{
					showEditSpin(hNumTrialsSpin, numTrialsEdit.getHwnd());
				}
				SetWindowText(hResultText, _T(""));
				TCHAR str[NUM_LEN + 1];
				_itow(numTrials, str, 10);
				SetWindowText(numTrialsEdit.getHwnd(), str);
				Edit_SetSel(numTrialsEdit.getHwnd(), lstrlen(str), -1);
				SetFocus(numTrialsEdit.getHwnd());
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
				if (numSuccess > MAX_DECIMAL(SPIN_LEN))
				{
					hideEditSpin(hNumSuccessSpin, numSuccessEdit.getHwnd());
				}
				else
				{
					showEditSpin(hNumSuccessSpin, numSuccessEdit.getHwnd());
				}
				SetWindowText(hResultText, _T(""));
				TCHAR str[NUM_LEN + 1];
				_itow(numSuccess, str, 10);
				SetWindowText(numSuccessEdit.getHwnd(), str);
				Edit_SetSel(numSuccessEdit.getHwnd(), lstrlen(str), -1);
				SetFocus(numSuccessEdit.getHwnd());
				break;
			}
		}
		break;
	case WM_CLOSE:
		if (hBet != NULL)
		{
			PostMessage(hBet, BPC_DISCONNECT, (WPARAM)hMainDlg, 0);
		}
		CloseThemeData(hButtonTheme);
		PostQuitMessage(0);
		break;
	}
	return (INT_PTR)FALSE;
}

void hideEditSpin(HWND hSpin, HWND hEdit)
{
	SetWindowPos(hSpin, nullptr, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_HIDEWINDOW);
}

void showEditSpin(HWND hSpin, HWND hEdit)
{
	RECT rcSpin, rcEdit;
	GetWindowRect(hSpin, &rcSpin);
	GetWindowRect(hEdit, &rcEdit);
	MapWindowRect(HWND_DESKTOP, hMainDlg, &rcEdit);
	SetWindowPos(hSpin, nullptr,
		rcEdit.right - (rcSpin.right - rcSpin.left) - (rcEdit.bottom - rcEdit.top), rcEdit.top,
		rcSpin.right - rcSpin.left, rcEdit.bottom - rcEdit.top - 2,
		SWP_NOZORDER | SWP_SHOWWINDOW);
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

void updateNumTrials()
{
	TCHAR str[NUM_LEN + 1];
	GetWindowText(numTrialsEdit.getHwnd(), str, NUM_LEN + 1);
	numTrials = _wtoi(str);
	if (lstrlen(str) > SPIN_LEN)
	{
		SetWindowPos(hNumTrialsSpin, nullptr, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_HIDEWINDOW);
	}
	else if (!IsWindowVisible(hNumTrialsSpin))
	{
		showEditSpin(hNumTrialsSpin, numTrialsEdit.getHwnd());
	}
}

void updateNumSuccess()
{
	TCHAR str[NUM_LEN + 1];
	GetWindowText(numSuccessEdit.getHwnd(), str, NUM_LEN + 1);
	numSuccess = _wtoi(str);
	if (lstrlen(str) > SPIN_LEN)
	{
		SetWindowPos(hNumSuccessSpin, nullptr, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_HIDEWINDOW);
	}
	else if (!IsWindowVisible(hNumSuccessSpin))
	{
		showEditSpin(hNumSuccessSpin, numSuccessEdit.getHwnd());
	}
}

void calcProbability()
{
	if (successProbability == 0)
	{
		SetWindowFont(hResultText, hNormalFont, FALSE);
		SetWindowLong(hResultText, GWL_STYLE, GetWindowStyle(hResultText) & ~SS_CENTER);
		SetWindowText(hResultText, _T("成功概率不能为0"));
		SetFocus(successProbabilityEdit.getHwnd());
		Edit_SetSel(successProbabilityEdit.getHwnd(), 0, INT_MAX);
		return;
	}
	if (numTrials == 0)
	{
		SetWindowFont(hResultText, hNormalFont, FALSE);
		SetWindowLong(hResultText, GWL_STYLE, GetWindowStyle(hResultText) & ~SS_CENTER);
		SetWindowText(hResultText, _T("试验次数不能为0"));
		SetFocus(numTrialsEdit.getHwnd());
		Edit_SetSel(numTrialsEdit.getHwnd(), 0, INT_MAX);
		return;
	}
	if (numSuccess == 0)
	{
		SetWindowFont(hResultText, hNormalFont, FALSE);
		SetWindowLong(hResultText, GWL_STYLE, GetWindowStyle(hResultText) & ~SS_CENTER);
		SetWindowText(hResultText, _T("成功次数不能为0"));
		SetFocus(numSuccessEdit.getHwnd());
		Edit_SetSel(numSuccessEdit.getHwnd(), 0, INT_MAX);
		return;
	}
	if (numSuccess > numTrials)
	{
		SetWindowFont(hResultText, hNormalFont, FALSE);
		SetWindowLong(hResultText, GWL_STYLE, GetWindowStyle(hResultText) & ~SS_CENTER);
		SetWindowText(hResultText, _T("成功次数不能大于试验次数"));
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
	SetWindowLong(hResultText, GWL_STYLE, GetWindowStyle(hResultText) | SS_CENTER);
	SetWindowFont(hResultText, hLargeFont, FALSE);
	SetWindowText(hResultText, str + (PROB_LEN + 2 + 2 * NUM_LEN + 4));
	historyResultListBox.addResult(str);
}

