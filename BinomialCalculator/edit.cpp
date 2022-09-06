#include "framework.h"
#include "edit.h"
#include "BinomialCalculator.h"

#include <CommCtrl.h>

using namespace std;

void initNumericEdit(HWND edit)
{
	RECT rect;
	GetClientRect(edit, &rect);
	LOGFONT font;
	GetObject(hNormalFont, sizeof(LOGFONT), &font);
	rect.top += (rect.bottom - rect.top + font.lfHeight - 1.5f) / 2;
	SendMessage(edit, EM_SETRECTNP, 0, (LPARAM)&rect);
}

static LRESULT CALLBACK editSubclassProc(HWND hEdit, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	return ((NumericEdit*)GetWindowLongPtr(hEdit, GWLP_USERDATA))->wndProc(msg, wParam, lParam);
}

void NumericEdit::attach(HWND hEdit)
{
	this->hEdit = hEdit;
	SetWindowLongPtr(hEdit, GWLP_USERDATA, (LONG_PTR)this);
	SetWindowSubclass(hEdit, editSubclassProc, 0, 0);
	initNumericEdit(hEdit);
}

LRESULT NumericEdit::wndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_KEYDOWN:
		if (wParam == VK_RETURN)
		{
			updateStr();
			return (INT_PTR)TRUE;
		}
		break;
	case WM_KILLFOCUS:
		{
			updateStr();
			break;
		}
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
			SendMessage(hEdit, EM_SETSEL, curUndo.size(), -1);
			SendMessage(GetParent(hEdit), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hEdit), EN_CHANGE), 0);
			return (INT_PTR)TRUE;
		}
	case WM_PASTE:
		OpenClipboard(hEdit);
		wstring str;
		char* text = (char*)GetClipboardData(CF_TEXT);
		if (text == nullptr)
		{
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
		SendMessage(hEdit, EM_REPLACESEL, (WPARAM)0, (LPARAM)str.c_str());
		return (INT_PTR)TRUE;
	}
	return DefSubclassProc(hEdit, msg, wParam, lParam);
}

HWND NumericEdit::getHwnd()
{
	return hEdit;
}

void NumericEdit::setText(LPCTSTR str)
{
	updateStr();
	if (str[0] != '\0' && str != curUndo)
	{
		lastUndo = curUndo;
		curUndo = str;
	}
	SetWindowText(hEdit, str);
	SendMessage(hEdit, EM_SETSEL, lstrlen(str), -1);
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