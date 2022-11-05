#pragma once

#include "button.h"

#include <string>

class NumericEdit
{
protected:
	HWND hEdit;
	Button clearButton;
	std::wstring curUndo = _T("");
	std::wstring lastUndo = _T("");
public:
	void attach(HWND hEdit);
	virtual LRESULT wndProc(UINT msg, WPARAM wParam, LPARAM lParam);
	HWND getHwnd();
	void setText(PCTSTR str);
protected:
	virtual void initLayout();
private:
	void updateStr();
};

class NumSpinEdit
	:public NumericEdit
{
private:
	HWND hSpin;
	HWND hMsgWnd;
	int curShowSpinFrame = 0;
public:
	void attach(HWND hEdit, HWND hSpin);
	virtual LRESULT wndProc(UINT msg, WPARAM wParam, LPARAM lParam);
	int updateNum();
	void updateSpin();
private:
	virtual void initLayout();
};

LRESULT CALLBACK readOnlyEditSubclassProc(HWND hEdit, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);