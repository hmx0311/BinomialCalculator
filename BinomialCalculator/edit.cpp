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
		WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_FLAT,
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
	case WM_DRAWITEM:
		{
			PDRAWITEMSTRUCT pDrawItemStruct = (PDRAWITEMSTRUCT)lParam;
			HDC hDC = pDrawItemStruct->hDC;
			if (BufferedPaintRenderAnimation(pDrawItemStruct->hwndItem, hDC))
			{
				return (INT_PTR)TRUE;
			}
			((Button*)GetWindowLongPtr(pDrawItemStruct->hwndItem, GWLP_USERDATA))->drawItem(hDC, pDrawItemStruct->itemState, pDrawItemStruct->rcItem);
			return (INT_PTR)TRUE;
		}
	case WM_DPICHANGED_AFTERPARENT:
		initLayout();
		break;
	case WM_KEYDOWN:
		if (wParam == VK_RETURN)
		{
			updateStr();
			return (INT_PTR)TRUE;
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
	case WM_UNDO:
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

void NumericEdit::getVCenteredRect(PRECT pRect)
{
	GetClientRect(hEdit, pRect);
	LOGFONT logFont;
	GetObject(hNormalFont, sizeof(LOGFONT), &logFont);
	pRect->top = 0.5f * (pRect->bottom - pRect->top - (abs(logFont.lfHeight) + 1.5f));
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
	RECT rc;
	getVCenteredRect(&rc);
	rc.right -= rc.bottom - rc.top;
	Edit_SetRectNoPaint(hEdit, &rc);
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