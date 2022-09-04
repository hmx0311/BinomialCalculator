// BinomialCalculator.cpp : 定义应用程序的入口点。
//
#include "framework.h"
#include "BinomialCalculator.h"

#include "Button.h"
#include "edit.h"

#include "model.h"

#include <CommCtrl.h>

#pragma comment(lib, "UxTheme.lib")

#define MAX_LOADSTRING 100

// 全局变量:
HINSTANCE hInst;                                // 当前实例

HFONT hNormalFont;
HFONT HLargeFont;

HWND mainDlg;
HWND successProbabilityEdit;
Button clearSuccessProbabilityButton;
HWND numTrialsEdit;
Button clearNumTrialsButton;
HWND numSuccessEdit;
Button clearNumSuccessButton;
HWND resultText;
Button calculateButton;
Button clearHistoryResultButton;
HWND historyResultText;

double successProbability = 0;
int numTrials = 0;
int numSuccess = 0;
int resultCnt;

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
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);


	// 执行应用程序初始化:
	SetProcessDPIAware();

	hInst = hInstance; // 将实例句柄存储在全局变量中
	mainDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_BINOMIALCALCULATOR_DIALOG), NULL, dlgProc);
	if (!mainDlg)
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
		PostMessage(hBet, BPC_CONNECTED, (WPARAM)mainDlg, 0);
		SetWindowText(mainDlg, _T("bet - 二项分布计算器"));
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
					break;;
				case IDC_NUM_SUCCESS_EDIT:
					updateNumSuccess();
				}
				calcProbability();
			case VK_ESCAPE:
				continue;
			case VK_UP:
				switch (GetDlgCtrlID(GetFocus()))
				{
				case IDC_SUCCESS_PROBABILITY_EDIT:
					continue;
				case IDC_NUM_TRIALS_EDIT:
					SetFocus(successProbabilityEdit);
					SendMessage(successProbabilityEdit, EM_SETSEL, 0, -1);
					continue;
				case IDC_NUM_SUCCESS_EDIT:
					SetFocus(numTrialsEdit);
					SendMessage(numTrialsEdit, EM_SETSEL, 0, -1);
					continue;
				}
				break;
			case VK_DOWN:
				switch (GetDlgCtrlID(GetFocus()))
				{
				case IDC_SUCCESS_PROBABILITY_EDIT:
					SetFocus(numTrialsEdit);
					SendMessage(numTrialsEdit, EM_SETSEL, 0, -1);
					continue;
				case IDC_NUM_TRIALS_EDIT:
					SetFocus(numSuccessEdit);
					SendMessage(numSuccessEdit, EM_SETSEL, 0, -1);
					continue;
				case IDC_NUM_SUCCESS_EDIT:
					continue;
				}
			}
			break;
		}
		if (!IsDialogMessage(mainDlg, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

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
	font.lfHeight = 1.2f * font.lfHeight - 2.5f;
	HLargeFont = CreateFontIndirect(&font);

	defButtonProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hDlg, IDC_CLEAR_SUCCESS_PROBABILITY_BUTTON), GWLP_WNDPROC);
	hButtonTheme = OpenThemeData(GetDlgItem(hDlg, IDC_CLEAR_SUCCESS_PROBABILITY_BUTTON), _T("Button"));

	successProbabilityEdit = GetDlgItem(hDlg, IDC_SUCCESS_PROBABILITY_EDIT);
	clearSuccessProbabilityButton.attach(GetDlgItem(hDlg, IDC_CLEAR_SUCCESS_PROBABILITY_BUTTON));
	numTrialsEdit = GetDlgItem(hDlg, IDC_NUM_TRIALS_EDIT);
	clearNumTrialsButton.attach(GetDlgItem(hDlg, IDC_CLEAR_NUM_TRIALS_BUTTON));
	numSuccessEdit = GetDlgItem(hDlg, IDC_NUM_SUCCESS_EDIT);
	clearNumSuccessButton.attach(GetDlgItem(hDlg, IDC_CLEAR_NUM_SUCCESS_BUTTON));
	resultText = GetDlgItem(hDlg, IDC_RESULT_TEXT);
	calculateButton.attach(GetDlgItem(hDlg, IDC_CALCULATE_BUTTON));
	clearHistoryResultButton.attach(GetDlgItem(hDlg, IDC_CLEAR_HISTORY_RESULT_BUTTON));
	historyResultText = GetDlgItem(hDlg, IDC_HISTORY_RESULT_TEXT);

	defEditProc = (WNDPROC)SetWindowLongPtr(successProbabilityEdit, GWLP_WNDPROC, (LONG_PTR)numericEditProc);
	SetWindowLongPtr(numTrialsEdit, GWLP_WNDPROC, (LONG_PTR)numericEditProc);
	SetWindowLongPtr(numSuccessEdit, GWLP_WNDPROC, (LONG_PTR)numericEditProc);

	SetWindowLongPtr(resultText, GWLP_WNDPROC, (LONG_PTR)readOnlyEditProc);
	SetWindowLongPtr(historyResultText, GWLP_WNDPROC, (LONG_PTR)readOnlyEditProc);


	SendMessage(successProbabilityEdit, EM_SETLIMITTEXT, 4, 0);
	initNumericEdit(successProbabilityEdit);
	SendMessage(numTrialsEdit, EM_SETLIMITTEXT, 8, 0);
	initNumericEdit(numTrialsEdit);
	SendMessage(numSuccessEdit, EM_SETLIMITTEXT, 8, 0);
	initNumericEdit(numSuccessEdit);

	clearSuccessProbabilityButton.setBkgBrush((HBRUSH)GetStockObject(WHITE_BRUSH));
	clearNumTrialsButton.setBkgBrush((HBRUSH)GetStockObject(WHITE_BRUSH));
	clearNumSuccessButton.setBkgBrush((HBRUSH)GetStockObject(WHITE_BRUSH));

	HICON hClearIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_CLEAR));
	clearSuccessProbabilityButton.setIcon(hClearIcon);
	clearNumTrialsButton.setIcon(hClearIcon);
	clearNumSuccessButton.setIcon(hClearIcon);
	HICON hBinIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_BIN));
	clearHistoryResultButton.setIcon(hBinIcon);

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
	case WM_CTLCOLORSTATIC:
		if ((HWND)lParam == resultText && (HFONT)SendMessage(resultText, WM_GETFONT, 0, 0) == hNormalFont)
		{
			HDC hDC = (HDC)wParam;
			SetTextColor(hDC, RGB(255, 0, 0));
			SetBkColor(hDC, GetSysColor(CTLCOLOR_DLG));
			return (INT_PTR)GetSysColorBrush(CTLCOLOR_DLG);
		}
		break;
	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT lpDrawItemStruct = (LPDRAWITEMSTRUCT)lParam;
			HDC hDC = lpDrawItemStruct->hDC;
			if (BufferedPaintRenderAnimation(lpDrawItemStruct->hwndItem, hDC))
			{
				return (INT_PTR)TRUE;
			}
			switch (lpDrawItemStruct->CtlType)
			{
			case ODT_BUTTON:
				{
					Button* button = (Button*)GetWindowLongPtr(lpDrawItemStruct->hwndItem, GWLP_USERDATA);
					if (button != nullptr)
					{
						button->drawItem(hDC, lpDrawItemStruct->itemState, lpDrawItemStruct->rcItem);
					}
					break;
				}
			}
			return (INT_PTR)TRUE;
		}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_SUCCESS_PROBABILITY_EDIT:
			switch (HIWORD(wParam))
			{
			case EN_KILLFOCUS:
				updateSuccessProbability();
				return (INT_PTR)TRUE;
			case EN_CHANGE:
				SetWindowText(resultText, _T(""));
			}
			break;
		case IDC_NUM_TRIALS_EDIT:
			switch (HIWORD(wParam))
			{
			case EN_KILLFOCUS:
				updateNumTrials();
				return (INT_PTR)TRUE;
			case EN_CHANGE:
				SetWindowText(resultText, _T(""));
			}
			break;
		case IDC_NUM_SUCCESS_EDIT:
			switch (HIWORD(wParam))
			{
			case EN_KILLFOCUS:
				updateNumSuccess();
				return (INT_PTR)TRUE;
			case EN_CHANGE:
				SetWindowText(resultText, _T(""));
			}
			break;
		case IDC_CLEAR_SUCCESS_PROBABILITY_BUTTON:
			SetWindowText(resultText, _T(""));
			SetWindowText(successProbabilityEdit, _T(""));
			SetFocus(successProbabilityEdit);
			return (INT_PTR)TRUE;
		case IDC_CLEAR_NUM_TRIALS_BUTTON:
			SetWindowText(resultText, _T(""));
			SetWindowText(numTrialsEdit, _T(""));
			SetFocus(numTrialsEdit);
			return (INT_PTR)TRUE;
		case IDC_CLEAR_NUM_SUCCESS_BUTTON:
			SetWindowText(resultText, _T(""));
			SetWindowText(numSuccessEdit, _T(""));
			SetFocus(numSuccessEdit);
			return (INT_PTR)TRUE;
		case IDC_CALCULATE_BUTTON:
			calcProbability();
			return (INT_PTR)TRUE;
		case IDC_CLEAR_HISTORY_RESULT_BUTTON:
			resultCnt = 0;
			SetWindowText(historyResultText, _T(""));
			return (INT_PTR)TRUE;
		}
		break;
	case WM_CLOSE:
		if (hBet != NULL)
		{
			PostMessage(hBet, BPC_DISCONNECT, (WPARAM)mainDlg, 0);
		}
		CloseThemeData(hButtonTheme);
		PostQuitMessage(0);
		break;
	}
	return (INT_PTR)FALSE;
}

void updateSuccessProbability()
{
	WCHAR str[5];
	GetWindowText(successProbabilityEdit, str, 5);
	successProbability = 0;
	for (int i = lstrlen(str) - 1; i >= 0; i--)
	{
		int c = str[i] - '0';
		successProbability = (successProbability + c) * 0.1;
	}
}

void updateNumTrials()
{
	WCHAR str[9];
	GetWindowText(numTrialsEdit, str, 9);
	numTrials = _wtoi(str);
}

void updateNumSuccess()
{
	WCHAR str[9];
	GetWindowText(numSuccessEdit, str, 9);
	numSuccess = _wtoi(str);
}

void calcProbability()
{
	if (successProbability == 0)
	{
		SendMessage(resultText, WM_SETFONT, (WPARAM)hNormalFont, FALSE);
		SetWindowText(resultText, _T("成功概率不能为0"));
		SetFocus(successProbabilityEdit);
		SendMessage(successProbabilityEdit, EM_SETSEL, 0, -1);
		return;
	}
	if (numTrials == 0)
	{
		SendMessage(resultText, WM_SETFONT, (WPARAM)hNormalFont, FALSE);
		SetWindowText(resultText, _T("试验次数不能为0"));
		SetFocus(numTrialsEdit);
		SendMessage(numTrialsEdit, EM_SETSEL, 0, -1);
		return;
	}
	if (numSuccess == 0)
	{
		SendMessage(resultText, WM_SETFONT, (WPARAM)hNormalFont, FALSE);
		SetWindowText(resultText, _T("成功次数不能为0"));
		SetFocus(numSuccessEdit);
		SendMessage(numSuccessEdit, EM_SETSEL, 0, -1);
		return;
	}
	if (numSuccess > numTrials)
	{
		SendMessage(resultText, WM_SETFONT, (WPARAM)hNormalFont, FALSE);
		SetWindowText(resultText, _T("成功次数不能大于试验次数"));
		SetFocus(numSuccessEdit);
		SendMessage(numSuccessEdit, EM_SETSEL, 0, -1);
		return;
	}
	double cumulativeProbability = binomialCumulativeProbability(successProbability, numTrials, numSuccess);
	PostMessage(hBet, BPC_PROBABILITY, (WPARAM)mainDlg, *(LPARAM*)(&cumulativeProbability));
	LPWSTR str = new WCHAR[42 * (resultCnt + 1)];
	swprintf(str, 41, _T("%.4f %8d %8d  %.4f  %.4f"), successProbability, numTrials, numSuccess, cumulativeProbability, 1 - cumulativeProbability);
	SendMessage(resultText, WM_SETFONT, (WPARAM)HLargeFont, FALSE);
	SetWindowText(resultText, str + 26);
	if (resultCnt > 0)
	{
		str[40] = '\r';
		str[41] = '\n';
		GetWindowText(historyResultText, str + 42, 42 * resultCnt);
	}
	SetWindowText(historyResultText, str);
	delete[](str);
	resultCnt++;
}