#include "framework.h"
#include "edit.h"
#include "BinomialCalculator.h"

#include <Windowsx.h>

#define IDC_CLEAR_BUTTON 1000

#define NUM_SHOW_SPIN_FRAMES 11
#define SHOW_SPIN_FRAME_INTERVAL 17
#define IDT_FRAME_TIMER 1

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
		0, 0, 0, 0,
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
			SetFocus(hEdit);
			RECT rect;
			GetWindowRect(hEdit, &rect);
			POINT pos = { lParam == -1 ? (rect.left + rect.right) / 2 : GET_X_LPARAM(lParam),lParam == -1 ? (rect.top + rect.bottom) / 2 : GET_Y_LPARAM(lParam) };
			HMENU hMenu = CreatePopupMenu();
			DWORD sel = Edit_GetSel(hEdit);
			TCHAR temp[20];
			GetWindowText(hEdit, temp, 20);
			AppendMenu(hMenu, curUndo.empty() || lstrcmp(temp, curUndo.c_str()) == 0 && lastUndo.empty() ? MF_GRAYED : MF_ENABLED, 1, _T("撤销(&U)"));
			AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
			AppendMenu(hMenu, HIWORD(sel) == LOWORD(sel) ? MF_GRAYED : MF_ENABLED, 2, _T("剪切(&T)"));
			AppendMenu(hMenu, HIWORD(sel) == LOWORD(sel) ? MF_GRAYED : MF_ENABLED, 3, _T("复制(&C)"));
			AppendMenu(hMenu, IsClipboardFormatAvailable(CF_TEXT) ? MF_ENABLED : MF_GRAYED, 4, _T("粘贴(&P)"));
			AppendMenu(hMenu, HIWORD(sel) == LOWORD(sel) ? MF_GRAYED : MF_ENABLED, 5, _T("删除(&D)"));
			AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
			AppendMenu(hMenu, LOWORD(sel) == 0 && HIWORD(sel) == GetWindowTextLength(hEdit) ? MF_GRAYED : MF_ENABLED, 6, _T("全选(&A)"));
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
			return 0;
		}
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_RETURN:
			updateStr();
			break;
		case VK_ESCAPE:
			SetWindowText(hEdit, curUndo.c_str());
			Edit_SetSel(hEdit, curUndo.size(), -1);
			SendMessage(GetParent(hEdit), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hEdit), EN_CHANGE), 0);
			return 0;
		}
		break;
	case WM_KILLFOCUS:
		updateStr();
		break;
	case EM_UNDO:
		{
			TCHAR temp[10];
			GetWindowText(hEdit, temp, 10);
			if (curUndo.empty())
			{
				curUndo = temp;
				return (LRESULT)TRUE;
			}
			if (lstrcmp(temp, curUndo.c_str()) == 0)
			{
				if (lastUndo.empty())
				{
					return (LRESULT)TRUE;
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
			return (LRESULT)TRUE;
		}
	case WM_PASTE:
		{
			OpenClipboard(hEdit);
			char* text = (char*)GetClipboardData(CF_TEXT);
			if (text == nullptr)
			{
				CloseClipboard();
				return 0;
			}
			wstring str;
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
						if (*(WCHAR*)text == '　')
						{
							text++;
							break;
						}
						str.clear();
					}
					else if (isdigit(*text))
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
			return 0;
		}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_CLEAR_BUTTON:
			setText(_T(""));
			SetFocus(hEdit);
			SendMessage(GetParent(hEdit), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hEdit), EN_CHANGE), 0);
			return 0;
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



void NumSpinEdit::attach(HWND hEdit, HWND hSpin)
{
	this->hSpin = hSpin;
	NumericEdit::attach(hEdit);
	hMsgWnd = CreateWindow(_T("Message"), nullptr, NULL, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInst, NULL);
	SendMessage(hSpin, UDM_SETBUDDY, (WPARAM)hMsgWnd, 0);
}

LRESULT NumSpinEdit::wndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_TIMER:
		switch (wParam)
		{
		case IDT_FRAME_TIMER:
			drawSpinFrame();
			return 0;
		}
		break;
	case WM_MOUSEWHEEL:
		SendMessage(hMsgWnd, msg, wParam, lParam);
		break;
	case WM_KEYDOWN:
	case WM_KEYUP:
		switch (wParam)
		{
		case VK_UP:
		case VK_DOWN:
			SendMessage(hMsgWnd, msg, wParam, lParam);
			break;
		}
		break;
	case WM_KILLFOCUS:
		SendMessage(hMsgWnd, WM_KEYUP, VK_UP, MAKELONG(1, KF_UP | KF_REPEAT | KF_EXTENDED));
		break;
	}
	return NumericEdit::wndProc(msg, wParam, lParam);
}

int NumSpinEdit::updateNum()
{
	updateSpin();
	TCHAR str[NUM_LEN + 1];
	GetWindowText(hEdit, str, NUM_LEN + 1);
	return _wtoi(str);
}

void NumSpinEdit::updateSpin()
{
	if ((GetWindowTextLength(hEdit) > MAX_SPIN_LEN) == (curSpinFrame == NUM_SHOW_SPIN_FRAMES))
	{
		return;
	}
	SendMessage(hSpin, WM_LBUTTONUP, 0, 0);
	SetTimer(hEdit, IDT_FRAME_TIMER, SHOW_SPIN_FRAME_INTERVAL, nullptr);
	drawSpinFrame();
}

void NumSpinEdit::initLayout()
{
	NumericEdit::initLayout();
	RECT rcSpin, rcEdit;
	GetWindowRect(hSpin, &rcSpin);
	GetWindowRect(hEdit, &rcEdit);
	MapWindowRect(HWND_DESKTOP, GetParent(hSpin), &rcEdit);
	SetWindowPos(hSpin, nullptr, 0, 0, rcSpin.right - rcSpin.left, rcEdit.bottom - rcEdit.top - 1, SWP_NOMOVE | SWP_NOZORDER);
	SetWindowRgn(hSpin, CreateRectRgn(0, 0, rcSpin.right - rcSpin.left, rcEdit.bottom - rcEdit.top - 2), TRUE);
}

void NumSpinEdit::drawSpinFrame()
{
	if (GetWindowTextLength(hEdit) > MAX_SPIN_LEN)
	{
		curSpinFrame++;
	}
	else
	{
		curSpinFrame--;
	}
	RECT rcSpin;
	GetWindowRect(hSpin, &rcSpin);
	SetWindowRgn(hSpin, CreateRectRgn(
		(float)curSpinFrame / NUM_SHOW_SPIN_FRAMES * (rcSpin.right - rcSpin.left),
		0,
		rcSpin.right - rcSpin.left,
		rcSpin.bottom - rcSpin.top - 1),
		TRUE);
	if (curSpinFrame == 0 || curSpinFrame == NUM_SHOW_SPIN_FRAMES)
	{
		KillTimer(hEdit, IDT_FRAME_TIMER);
	}
}


LRESULT CALLBACK readOnlyEditSubclassProc(HWND hEdit, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (msg)
	{
	case WM_CONTEXTMENU:
		{
			SetFocus(hEdit);
			RECT rect;
			GetWindowRect(hEdit, &rect);
			POINT pos = { lParam == -1 ? (rect.left + rect.right) / 2 : GET_X_LPARAM(lParam),lParam == -1 ? (rect.top + rect.bottom) / 2 : GET_Y_LPARAM(lParam) };
			HMENU hMenu = CreatePopupMenu();
			DWORD sel = Edit_GetSel(hEdit);
			AppendMenu(hMenu, HIWORD(sel) == LOWORD(sel) ? MF_GRAYED : MF_ENABLED, 1, _T("复制(&C)"));
			AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
			AppendMenu(hMenu, LOWORD(sel) == 0 && HIWORD(sel) == GetWindowTextLength(hEdit) ? MF_GRAYED : MF_ENABLED, 2, _T("全选(&A)"));
			switch (TrackPopupMenu(hMenu, TPM_NONOTIFY | TPM_RETURNCMD, pos.x, pos.y, 0, hEdit, nullptr))
			{
			case 1:
				SendMessage(hEdit, WM_COPY, 0, 0);
				break;
			case 2:
				Edit_SetSel(hEdit, 0, INT_MAX);
				break;
			}
			DestroyMenu(hMenu);
			return 0;
		}
	}
	LRESULT result = DefSubclassProc(hEdit, msg, wParam, lParam);
	HideCaret(hEdit);
	return result;
}