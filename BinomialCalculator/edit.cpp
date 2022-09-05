#include "framework.h"
#include "edit.h"
#include "BinomialCalculator.h"

#include "string"
#include <unordered_map>

using namespace std;

unordered_map<HWND, pair<wstring, wstring>> undoStrMap;

WNDPROC defEditProc;

void initNumericEdit(HWND edit)
{
	RECT rect;
	GetClientRect(edit, &rect);
	LOGFONT font;
	GetObject(hNormalFont, sizeof(LOGFONT), &font);
	rect.top += (rect.bottom - rect.top + font.lfHeight - 1.5f) / 2;
	SendMessage(edit, EM_SETRECTNP, 0, (LPARAM)&rect);

	undoStrMap.emplace(edit, make_pair(_T(""), _T("")));
}

LRESULT CALLBACK numericEditProc(HWND edit, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_KILLFOCUS:
	{
		TCHAR temp[10];
		GetWindowText(edit, temp, 10);
		auto& undoStr = undoStrMap.at(edit);
		if (temp[0] == '\0' || lstrcmp(temp, undoStr.first.c_str()) == 0)
		{
			break;
		}
		undoStr.second = undoStr.first;
		undoStr.first = temp;
		break;
	}
	case EM_UNDO:
	{
		auto& undoStr = undoStrMap.at(edit);
		TCHAR temp[10];
		GetWindowText(edit, temp, 10);
		if (undoStr.first.empty())
		{
			undoStr.first = temp;
			return (INT_PTR)TRUE;
		}
		if (lstrcmp(temp, undoStr.first.c_str()) == 0)
		{
			if (undoStr.second.empty())
			{
				return (INT_PTR)TRUE;
			}
			undoStr.second.swap(undoStr.first);
		}
		else if (temp[0] != '\0')
		{
			undoStr.second = temp;
		}
		SetWindowText(edit, undoStr.first.c_str());
		SendMessage(edit, EM_SETSEL, undoStr.first.size(), -1);
		SendMessage(GetParent(edit), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(edit), EN_CHANGE), 0);
		return (INT_PTR)TRUE;
	}
	case WM_PASTE:
		OpenClipboard(edit);
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
		SendMessage(edit, EM_REPLACESEL, (WPARAM)0, (LPARAM)str.c_str());
		return (INT_PTR)TRUE;
	}
	return CallWindowProc(defEditProc, edit, message, wParam, lParam);
}


LRESULT CALLBACK readOnlyEditProc(HWND edit, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = CallWindowProc(defEditProc, edit, message, wParam, lParam);
	HideCaret(edit);
	return result;
}