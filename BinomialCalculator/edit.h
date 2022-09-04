#pragma once

extern WNDPROC defEditProc;

void initNumericEdit(HWND);

LRESULT CALLBACK numericEditProc(HWND, UINT, WPARAM, LPARAM);

LRESULT CALLBACK readOnlyEditProc(HWND, UINT, WPARAM, LPARAM);