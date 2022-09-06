#include "framework.h"
#include "edit.h"
#include "BinomialCalculator.h"

#include <CommCtrl.h>
#include <string>
#include <unordered_map>

using namespace std;

static unordered_map<HWND, pair<wstring, wstring>> undoStrMap;

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

LRESULT CALLBACK numericEditSubclassProc(HWND hEdit, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (msg)
	{
	case WM_KILLFOCUS:
	{
		TCHAR temp[10];
		GetWindowText(hEdit, temp, 10);
		auto& undoStr = undoStrMap.at(hEdit);
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
		auto& undoStr = undoStrMap.at(hEdit);
		TCHAR temp[10];
		GetWindowText(hEdit, temp, 10);
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
		SetWindowText(hEdit, undoStr.first.c_str());
		SendMessage(hEdit, EM_SETSEL, undoStr.first.size(), -1);
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


LRESULT CALLBACK readOnlyEditSubclassProc(HWND hEdit, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	LRESULT result = DefSubclassProc(hEdit, msg, wParam, lParam);
	HideCaret(hEdit);
	return result;
}