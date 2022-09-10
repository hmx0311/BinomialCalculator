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
#define LIST_ITEM_HEIGHT 1.36f
#define LARGE_FONT_HEIGHT (1.2f * font.lfHeight - 2.5f)

#define PROB_LEN 4
#define NUM_LEN 8
#define _STR(x) #x
#define STR(x) _STR(x)

// 全局变量:
HINSTANCE hInst;                                // 当前实例

HFONT hNormalFont;
HFONT hLargeFont;

HWND hMainDlg;
NumericEdit successProbabilityEdit;
Button clearSuccessProbabilityButton;
NumericEdit numTrialsEdit;
Button clearNumTrialsButton;
NumericEdit numSuccessEdit;
Button clearNumSuccessButton;
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
	hMainDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_BINOMIALCALCULATOR_DIALOG), NULL, dlgProc);
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
				case IDC_NUM_TRIALS_EDIT:
					updateNumTrials();
					break;
				case IDC_NUM_SUCCESS_EDIT:
					updateNumSuccess();
				}
				calcProbability();
				break;
			case VK_ESCAPE:
				continue;
			case VK_UP:
				switch (GetDlgCtrlID(GetFocus()))
				{
				case IDC_SUCCESS_PROBABILITY_EDIT:
					continue;
				case IDC_NUM_TRIALS_EDIT:
					SetFocus(successProbabilityEdit.getHwnd());
					SendMessage(successProbabilityEdit.getHwnd(), EM_SETSEL, 0, -1);
					continue;
				case IDC_NUM_SUCCESS_EDIT:
					SetFocus(numTrialsEdit.getHwnd());
					SendMessage(numTrialsEdit.getHwnd(), EM_SETSEL, 0, -1);
					continue;
				}
				break;
			case VK_DOWN:
				switch (GetDlgCtrlID(GetFocus()))
				{
				case IDC_SUCCESS_PROBABILITY_EDIT:
					SetFocus(numTrialsEdit.getHwnd());
					SendMessage(numTrialsEdit.getHwnd(), EM_SETSEL, 0, -1);
					continue;
				case IDC_NUM_TRIALS_EDIT:
					SetFocus(numSuccessEdit.getHwnd());
					SendMessage(numSuccessEdit.getHwnd(), EM_SETSEL, 0, -1);
					continue;
				case IDC_NUM_SUCCESS_EDIT:
					continue;
				}
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
	HICON hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_BINOMIALCALCULATOR));
	SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

	hNormalFont = (HFONT)SendMessage(hDlg, WM_GETFONT, 0, 0);
	LOGFONT font;
	GetObject(hNormalFont, sizeof(LOGFONT), &font);
	font.lfHeight = LARGE_FONT_HEIGHT;
	hLargeFont = CreateFontIndirect(&font);

	hButtonTheme = OpenThemeData(GetDlgItem(hDlg, IDC_CLEAR_SUCCESS_PROBABILITY_BUTTON), _T("Button"));

	successProbabilityEdit.attach(GetDlgItem(hDlg, IDC_SUCCESS_PROBABILITY_EDIT));
	clearSuccessProbabilityButton.attach(GetDlgItem(hDlg, IDC_CLEAR_SUCCESS_PROBABILITY_BUTTON));
	numTrialsEdit.attach(GetDlgItem(hDlg, IDC_NUM_TRIALS_EDIT));
	clearNumTrialsButton.attach(GetDlgItem(hDlg, IDC_CLEAR_NUM_TRIALS_BUTTON));
	numSuccessEdit.attach(GetDlgItem(hDlg, IDC_NUM_SUCCESS_EDIT));
	clearNumSuccessButton.attach(GetDlgItem(hDlg, IDC_CLEAR_NUM_SUCCESS_BUTTON));
	hResultText = GetDlgItem(hDlg, IDC_RESULT_TEXT);
	SetWindowSubclass(hResultText, readOnlyEditSubclassProc, 0, 0);
	calculateButton.attach(GetDlgItem(hDlg, IDC_CALCULATE_BUTTON));
	clearHistoryResultButton.attach(GetDlgItem(hDlg, IDC_CLEAR_HISTORY_RESULT_BUTTON));
	historyResultListBox.attach(GetDlgItem(hDlg, IDC_HISTORY_RESULT_LISTBOX));

	SendMessage(successProbabilityEdit.getHwnd(), EM_SETLIMITTEXT, PROB_LEN, 0);
	SendMessage(numTrialsEdit.getHwnd(), EM_SETLIMITTEXT, NUM_LEN, 0);
	SendMessage(numSuccessEdit.getHwnd(), EM_SETLIMITTEXT, NUM_LEN, 0);

	clearSuccessProbabilityButton.setBkgBrush(GetSysColorBrush(COLOR_WINDOW));
	clearNumTrialsButton.setBkgBrush(GetSysColorBrush(COLOR_WINDOW));
	clearNumSuccessButton.setBkgBrush(GetSysColorBrush(COLOR_WINDOW));

	HICON hClearIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_CLEAR), IMAGE_ICON, 0, 0, LR_SHARED);
	clearSuccessProbabilityButton.setIcon(hClearIcon);
	clearNumTrialsButton.setIcon(hClearIcon);
	clearNumSuccessButton.setIcon(hClearIcon);
	clearHistoryResultButton.setIcon((HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_BIN), IMAGE_ICON, 0, 0, LR_SHARED));

	// Create the tooltip. hInst is the global instance handle.
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
	case WM_DPICHANGED:
		{
			hNormalFont = (HFONT)SendMessage(hDlg, WM_GETFONT, 0, 0);
			LOGFONT font;
			GetObject(hNormalFont, sizeof(LOGFONT), &font);
			int listItemHeight = -LIST_ITEM_HEIGHT * font.lfHeight;
			RECT rect;
			GetWindowRect(historyResultListBox.getHwnd(), &rect);
			SendMessage(historyResultListBox.getHwnd(), LB_SETITEMHEIGHT, 0, listItemHeight);
			SetWindowPos(historyResultListBox.getHwnd(), nullptr, 0, 0, rect.right - rect.left, 4 + DISPLAYED_ITEM_COUNT * listItemHeight, SWP_NOMOVE);
			font.lfHeight = LARGE_FONT_HEIGHT;
			DeleteObject(hLargeFont);
			hLargeFont = CreateFontIndirect(&font);
			if (GetWindowLongPtr(hResultText, GWL_STYLE) & SS_CENTER)
			{
				SendMessage(hResultText, WM_SETFONT, (WPARAM)hLargeFont, FALSE);
			}
			break;
		}
	case WM_THEMECHANGED:
		CloseThemeData(hButtonTheme);
		hButtonTheme = OpenThemeData(clearSuccessProbabilityButton.getHwnd(), _T("Button"));
		InvalidateRect(hDlg, nullptr, TRUE);
		return (INT_PTR)TRUE;
	case WM_INITDIALOG:
		return (INT_PTR)initDlg(hDlg);
	case WM_CTLCOLORSTATIC:
		if ((HWND)lParam == hResultText && !(GetWindowLongPtr(hResultText, GWL_STYLE) & SS_CENTER))
		{
			SetTextColor((HDC)wParam, RGB(255, 0, 0));
			SetBkColor((HDC)wParam, GetSysColor(COLOR_BTNFACE));
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
				{
					Button* button = (Button*)GetWindowLongPtr(pDrawItemStruct->hwndItem, GWLP_USERDATA);
					if (button != nullptr)
					{
						button->drawItem(hDC, pDrawItemStruct->itemState, pDrawItemStruct->rcItem);
					}
					break;
				}
			case ODT_LISTBOX:
				((ResultList*)GetWindowLongPtr(pDrawItemStruct->hwndItem, GWLP_USERDATA))->drawItem(hDC, pDrawItemStruct->itemID, pDrawItemStruct->itemState, pDrawItemStruct->rcItem);
				break;
			}
			return (INT_PTR)TRUE;
		}
	case WM_MEASUREITEM:
		{
			HFONT hFont = (HFONT)SendMessage(hDlg, WM_GETFONT, 0, 0);
			LOGFONT font;
			GetObject(hFont, sizeof(LOGFONT), &font);
			((PMEASUREITEMSTRUCT)lParam)->itemHeight = -LIST_ITEM_HEIGHT * font.lfHeight;
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
				SendMessage(hResultText, WM_SETFONT, (WPARAM)hLargeFont, FALSE);
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
			}
			break;
		case IDC_NUM_TRIALS_EDIT:
			switch (HIWORD(wParam))
			{
			case EN_KILLFOCUS:
				updateNumTrials();
				return (INT_PTR)TRUE;
			case EN_CHANGE:
				SetWindowText(hResultText, _T(""));
			}
			break;
		case IDC_NUM_SUCCESS_EDIT:
			switch (HIWORD(wParam))
			{
			case EN_KILLFOCUS:
				updateNumSuccess();
				return (INT_PTR)TRUE;
			case EN_CHANGE:
				SetWindowText(hResultText, _T(""));
			}
			break;
		case IDC_CLEAR_SUCCESS_PROBABILITY_BUTTON:
			SetWindowText(hResultText, _T(""));
			successProbabilityEdit.setText(_T(""));
			SetFocus(successProbabilityEdit.getHwnd());
			return (INT_PTR)TRUE;
		case IDC_CLEAR_NUM_TRIALS_BUTTON:
			SetWindowText(hResultText, _T(""));
			numTrialsEdit.setText(_T(""));
			SetFocus(numTrialsEdit.getHwnd());
			return (INT_PTR)TRUE;
		case IDC_CLEAR_NUM_SUCCESS_BUTTON:
			SetWindowText(hResultText, _T(""));
			numSuccessEdit.setText(_T(""));
			SetFocus(numSuccessEdit.getHwnd());
			return (INT_PTR)TRUE;
		case IDC_CALCULATE_BUTTON:
			calcProbability();
			return (INT_PTR)TRUE;
		case IDC_CLEAR_HISTORY_RESULT_BUTTON:
			historyResultListBox.reset();
			return (INT_PTR)TRUE;
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
}

void updateNumSuccess()
{
	TCHAR str[NUM_LEN + 1];
	GetWindowText(numSuccessEdit.getHwnd(), str, NUM_LEN + 1);
	numSuccess = _wtoi(str);
}

void calcProbability()
{
	if (successProbability == 0)
	{
		SendMessage(hResultText, WM_SETFONT, (WPARAM)hNormalFont, FALSE);
		LONG_PTR style = GetWindowLongPtr(hResultText, GWL_STYLE);
		style &= ~SS_CENTER;
		SetWindowLongPtr(hResultText, GWL_STYLE, style);
		SetWindowText(hResultText, _T("成功概率不能为0"));
		SetFocus(successProbabilityEdit.getHwnd());
		SendMessage(successProbabilityEdit.getHwnd(), EM_SETSEL, 0, -1);
		return;
	}
	if (numTrials == 0)
	{
		SendMessage(hResultText, WM_SETFONT, (WPARAM)hNormalFont, FALSE);
		LONG_PTR style = GetWindowLongPtr(hResultText, GWL_STYLE);
		style &= ~SS_CENTER;
		SetWindowLongPtr(hResultText, GWL_STYLE, style);
		SetWindowText(hResultText, _T("试验次数不能为0"));
		SetFocus(numTrialsEdit.getHwnd());
		SendMessage(numTrialsEdit.getHwnd(), EM_SETSEL, 0, -1);
		return;
	}
	if (numSuccess == 0)
	{
		SendMessage(hResultText, WM_SETFONT, (WPARAM)hNormalFont, FALSE);
		LONG_PTR style = GetWindowLongPtr(hResultText, GWL_STYLE);
		style &= ~SS_CENTER;
		SetWindowLongPtr(hResultText, GWL_STYLE, style);
		SetWindowText(hResultText, _T("成功次数不能为0"));
		SetFocus(numSuccessEdit.getHwnd());
		SendMessage(numSuccessEdit.getHwnd(), EM_SETSEL, 0, -1);
		return;
	}
	if (numSuccess > numTrials)
	{
		SendMessage(hResultText, WM_SETFONT, (WPARAM)hNormalFont, FALSE);
		LONG_PTR style = GetWindowLongPtr(hResultText, GWL_STYLE);
		style &= ~SS_CENTER;
		SetWindowLongPtr(hResultText, GWL_STYLE, style);
		SetWindowText(hResultText, _T("成功次数不能大于试验次数"));
		SetFocus(numSuccessEdit.getHwnd());
		SendMessage(numSuccessEdit.getHwnd(), EM_SETSEL, 0, -1);
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
	LONG_PTR style = GetWindowLongPtr(hResultText, GWL_STYLE);
	style |= SS_CENTER;
	SetWindowLongPtr(hResultText, GWL_STYLE, style);
	SendMessage(hResultText, WM_SETFONT, (WPARAM)hLargeFont, FALSE);
	SetWindowText(hResultText, str + (PROB_LEN + 2 + 2 * NUM_LEN + 4));
	historyResultListBox.addResult(str);
}