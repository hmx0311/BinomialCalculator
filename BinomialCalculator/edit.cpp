#include "framework.h"
#include "edit.h"
#include "BinomialCalculator.h"

#include <CommCtrl.h>
#include <Windowsx.h>

#define IDC_CLEAR_BUTTON 1000

using namespace std;

static LRESULT CALLBACK editSubclassProc(HWND hEdit, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	return ((NumericEdit*)GetWindowLongPtr(hEdit, GWLP_USERDATA))->wndProc(msg, wParam, lParam);
}


void NumericEdit::attach(HWND hEdit)
{
	this->hEdit = hEdit;
	SetWindowLongPtr(hEdit, GWLP_USERDATA, (LONG_PTR)this);
	SetWindowSubclass(hEdit, editSubclassProc, 0, 0);

	INITCOMMONCONTROLSEX icex = { sizeof(icex),ICC_STANDARD_CLASSES };
	InitCommonControlsEx(&icex);
	HWND hClearButton = CreateWindow(_T("Button"), nullptr,
		WS_CHILD | WS_VISIBLE | BS_FLAT,
		0,0,0,0,
		hEdit, (HMENU)IDC_CLEAR_BUTTON, hInst, nullptr);
	clearButton.attach(hClearButton);
	clearButton.setIcon((HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_CLEAR), IMAGE_ICON, 0, 0, LR_SHARED));
	clearButton.setBkgBrush(GetSysColorBrush(COLOR_WINDOW));

	initLayout();
}

LRESULT NumericEdit::wndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DPICHANGED_AFTERPARENT:
		initLayout();
		break;
	case WM_CONTEXTMENU:
		{
			RECT rect;
			GetWindowRect(hEdit, &rect);
			POINT pos = { lParam == -1 ? (rect.left + rect.right) / 2 : GET_X_LPARAM(lParam),lParam == -1 ? (rect.top + rect.bottom) / 2 : GET_Y_LPARAM(lParam) };
			HMENU hMenu = CreatePopupMenu();
			DWORD sel = Edit_GetSel(hEdit);
			TCHAR temp[20];
			GetWindowText(hEdit,temp, 20);
			AppendMenu(hMenu, (curUndo.empty() || lstrcmp(temp, curUndo.c_str()) == 0 && lastUndo.empty()) ? MF_ENABLED : MF_GRAYED, 1, _T("³·Ïú(&U)"));
			AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
			AppendMenu(hMenu, HIWORD(sel) == LOWORD(sel) ? MF_GRAYED : MF_ENABLED, 2, _T("¼ôÇÐ(&T)"));
			AppendMenu(hMenu, HIWORD(sel) == LOWORD(sel) ? MF_GRAYED : MF_ENABLED, 3, _T("¸´ÖÆ(&C)"));
			OpenClipboard(hEdit);
			AppendMenu(hMenu, GetClipboardData(CF_TEXT) == nullptr ? MF_GRAYED : MF_ENABLED, 4, _T("Õ³Ìù(&P)"));
			CloseClipboard();
			AppendMenu(hMenu, HIWORD(sel) == LOWORD(sel) ? MF_GRAYED : MF_ENABLED, 5, _T("É¾³ý(&D)"));
			AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
			AppendMenu(hMenu, LOWORD(sel) == 0 && HIWORD(sel) == GetWindowTextLength(hEdit) ? MF_GRAYED : MF_ENABLED, 6, _T("È«Ñ¡(&A)"));
			switch (TrackPopupMenu(hMenu, TPM_NONOTIFY | TPM_RETURNCMD, pos.x, pos.y, 0, hEdit, nullptr))
			{
			case 1:
				Edit_Undo(hEdit);
				break;
			case 2:
				SendMessage(hEdit, WM_CUT, 0, 0);
				break;
			case 3:
				SendMessage(hEdit, WM_COPY, 0, 0);
				break;
			case 4:
				SendMessage(hEdit, WM_PASTE, 0, 0);
				break;
			case 5:
				Edit_ReplaceSel(hEdit, _T(""));
				break;
			case 6:
				Edit_SetSel(hEdit, 0, INT_MAX);
				break;
			}
			DestroyMenu(hMenu);
			return (LRESULT)TRUE;
		}
	case WM_KEYDOWN:
		if (wParam == VK_RETURN)
		{
			updateStr();
			break;
		}
		break;
	case WM_KILLFOCUS:
		SendMessage(hEdit, WM_KEYUP, VK_UP, MAKELONG(1, KF_UP | KF_REPEAT | KF_EXTENDED));
		updateStr();
		break;
	case EM_SETSEL:
		if (wParam == 0 && lParam == -1)
		{
			return (INT_PTR)TRUE;
		}
		break;
	case EM_UNDO:
		{
			TCHAR temp[10];
			GetWindowText(hEdit, temp, 10);
			if (curUndo.empty())
			{
				curUndo = temp;
				return (INT_PTR)TRUE;
			}
			if (lstrcmp(temp, curUndo.c_str()) == 0)
			{
				if (lastUndo.empty())
				{
					return (INT_PTR)TRUE;
				}
				lastUndo.swap(curUndo);
			}
			else if (temp[0] != '\0')
			{
				lastUndo = temp;
			}
			SetWindowText(hEdit, curUndo.c_str());
			Edit_SetSel(hEdit, curUndo.size(), -1);
			SendMessage(GetParent(hEdit), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hEdit), EN_CHANGE), 0);
			return (INT_PTR)TRUE;
		}
	case WM_PASTE:
		{
			OpenClipboard(hEdit);
			wstring str;
			char* text = (char*)GetClipboardData(CF_TEXT);
			if (text == nullptr)
			{
				CloseClipboard();
				return (INT_PTR)TRUE;
			}
			for (; *text != '\0'; text++)
			{
				switch (*text)
				{
				case '\t':
				case '\r':
				case '\n':
				case ' ':
				case ',':
					break;
				default:
					if (*text < 0)
					{
						if (*(WCHAR*)text == '¡¡')
						{
							text++;
							break;
						}
						str.clear();
					}
					if (isdigit(*text))
					{
						str.push_back(*text);
					}
					else
					{
						str.clear();
					}
				}
			}
			CloseClipboard();
			Edit_ReplaceSel(hEdit, str.c_str());
			return (INT_PTR)TRUE;
		}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_CLEAR_BUTTON:
			setText(_T(""));
			SetFocus(hEdit);
			SendMessage(GetParent(hEdit), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hEdit), EN_CHANGE), 0);
			return (LRESULT)TRUE;
		}
		break;
	}
	return DefSubclassProc(hEdit, msg, wParam, lParam);
}

HWND NumericEdit::getHwnd()
{
	return hEdit;
}

void NumericEdit::setText(PCTSTR str)
{
	updateStr();
	if (str[0] != '\0' && str != curUndo)
	{
		lastUndo = curUndo;
		curUndo = str;
	}
	SetWindowText(hEdit, str);
	Edit_SetSel(hEdit, lstrlen(str), -1);
}

void NumericEdit::initLayout()
{
	RECT rcVCentered;
	GetClientRect(hEdit, &rcVCentered);
	LOGFONT logFont;
	GetObject(hNormalFont, sizeof(LOGFONT), &logFont);
	rcVCentered.top = 0.5f * (rcVCentered.bottom - rcVCentered.top - (abs(logFont.lfHeight) + 1.5f));
	Edit_SetRectNoPaint(hEdit, &rcVCentered);
	RECT rcEdit;
	GetWindowRect(hEdit, &rcEdit);
	MapWindowRect(HWND_DESKTOP, hEdit, &rcEdit);
	int buttonPadding = (rcEdit.bottom - rcEdit.top) / 6;
	SetWindowPos(clearButton.getHwnd(), nullptr,
		rcEdit.right - (rcEdit.bottom - rcEdit.top - buttonPadding), rcEdit.top + buttonPadding, rcEdit.bottom - rcEdit.top - 2 * buttonPadding, rcEdit.bottom - rcEdit.top - 2 * buttonPadding,
		SWP_NOZORDER);
}

void NumericEdit::updateStr()
{
	TCHAR temp[10];
	GetWindowText(hEdit, temp, 10);
	if (temp[0] == '\0' || temp == curUndo)
	{
		return;
	}
	lastUndo = curUndo;
	curUndo = temp;
}


LRESULT CALLBACK readOnlyEditSubclassProc(HWND hEdit, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	LRESULT result = DefSubclassProc(hEdit, msg, wParam, lParam);
	HideCaret(hEdit);
	return result;
}