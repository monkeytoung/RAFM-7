/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API サブウィンドウ３ ]
 */

#ifdef _WIN32

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdlib.h>
#include "xm7.h"
#include "keyboard.h"
#include "rtc.h"
#include "mmr.h"
#include "w32.h"
#include "w32_res.h"
#include "w32_sub.h"

/*
 *	スタティック ワーク
 */
static BYTE *pMMR;						/* MMR Drawバッファ */
static BYTE *pKeyboard;					/* キーボード Drawバッファ */

/*-[ MMRウインドウ ]---------------------------------------------------------*/

/*
 *	MMRウインドウ
 *	MMRマップ セットアップ
 */
static void FASTCALL SetupMMRMap(BYTE *p, int y, int x)
{
	char string[128];
	char temp[16];
	int i, j;

	ASSERT(p);
	ASSERT(y > 0);
	ASSERT(x > 0);

	/* タイトル */
	strcpy(string, "MMR Map:");
	memcpy(&p[x * y], string, strlen(string));
	y++;

	/* 各yごとに */
	for (i=0; i<4; i++) {
		/* 16進ダンプを作成 */
		sprintf(string, "Segment %d-", i);
		for (j=0; j<16; j++) {
			sprintf(temp, "%02X", mmr_reg[i * 0x10 + j]);
			strcat(string, temp);
		}

		/* MMR使用中でセグメント合致すれば、反転させる */
		if (mmr_flag && (mmr_seg == i)) {
			for (j=10;;j++) {
				if (string[j] == '\0') {
					break;
				}
				string[j] |= 0x80;
			}
		}

		/* セット */
		memcpy(&p[x * y], string, strlen(string));
		y++;
	}
}

/*
 *	MMRウインドウ
 *	メモリマップ セットアップ
 */
static void FASTCALL SetupMMRMemory(BYTE *p, int y, int x)
{
	char string[128];
	char temp[16];
	BYTE mem[16];
	int i, j, k;

	ASSERT(p);
	ASSERT(y > 0);
	ASSERT(x > 0);

	/* memの16バイトに、メモリ空間を設定 */
	for (i=0; i<16; i++) {
		mem[i] = (BYTE)(0x30 + i);
	}
	/* MMR */
	if (mmr_flag) {
		for (i=0; i<16; i++) {
			mem[i] = mmr_reg[mmr_seg * 0x10 + i];
		}
	}

	/* タイトル */
	strcpy(string, "Physical Memory Map:");
	memcpy(&p[x * y], string, strlen(string));
	y++;

	/* 各yごとに */
	for (i=0; i<4; i++) {
		/* 16進ダンプを作成 */
		switch (i) {
			case 0:
				strcpy(string, "Extend    ");
				break;
			case 1:
				strcpy(string, "Sub CPU   ");
				break;
			case 2:
				strcpy(string, "Japanese  ");
				break;
			case 3:
				strcpy(string, "Standard  ");
				break;
			default:
				ASSERT(FALSE);
		}
		for (j=0; j<16; j++) {
			sprintf(temp, "%02X", i * 0x10 + j);
			/* 反転処理 */
			for (k=0; k<16; k++) {
				if (mem[k] == (i * 0x10 + j)) {
					temp[0] |= 0x80;
					temp[1] |= 0x80;
				}
			}
			/* 追加 */
			strcat(string, temp);
		}

		/* セット */
		memcpy(&p[x * y], string, strlen(string));
		y++;
	}

}

/*
 *	MMRウインドウ
 *	セットアップ
 */
static void FASTCALL SetupMMR(BYTE *p, int x, int y)
{
	char string[128];

	ASSERT(p);
	ASSERT(x > 0);
	ASSERT(y > 0);

	/* 一旦スペースで埋める */
	memset(p, 0x20, x * y);

	/* MMR */
	if (mmr_flag) {
		sprintf(string, "MMR  Enable(Seg:%1d)", mmr_seg);
	}
	else {
		strcpy(string, "MMR        Disable");
	}
	memcpy(&p[x * 0 + 0], string, strlen(string));

	/* TWR */
	if (twr_flag) {
		sprintf(string, "TWR  Enable($%02X00)", (twr_reg + 0x7c) & 0xff);
	}
	else {
		strcpy(string, "TWR        Disable");
	}
	memcpy(&p[x * 1 + 0], string, strlen(string));

	/* ブートRAM */
	strcpy(string, "Boot RAM          R");
	if (bootram_rw) {
		strcat(string, "W");
	}
	else {
		strcat(string, "O");
	}
	memcpy(&p[x * 0 + 22], string, strlen(string));

	/* イニシエートROM */
	strcpy(string, "Initiate ROM ");
	if (initrom_en) {
		strcat(string, " Enable");
	}
	else {
		strcat(string, "Disable");
	}
	memcpy(&p[x * 1 + 22], string, strlen(string));

	/* MMRマップ */
	SetupMMRMap(p, 3, x);

	/* メモリマップ */
	SetupMMRMemory(p, 9, x);
}

/*
 *	MMRウインドウ
 *	描画
 */
static void FASTCALL DrawMMR(HWND hWnd, HDC hDC)
{
	RECT rect;
	BYTE *p, *q;
	int x, y;
	int xx, yy;
	HFONT hFont;
	HFONT hBackup;
	char dat;
	COLORREF tcolor, bcolor;

	ASSERT(hWnd);
	ASSERT(hDC);

	/* ウインドウジオメトリを得る */
	GetClientRect(hWnd, &rect);
	x = rect.right / lCharWidth;
	y = rect.bottom / lCharHeight;
	if ((x == 0) || (y == 0)) {
		return;
	}

	/* セットアップ */
	p = pMMR;
	if (!p) {
		return;
	}
	SetupMMR(p, x, y);

	/* フォントセレクト */
	hFont = CreateTextFont();
	hBackup = SelectObject(hDC, hFont);

	/* 比較描画 */
	q = &p[x * y];
	for (yy=0; yy<y; yy++) {
		for (xx=0; xx<x; xx++) {
			if (*p != *q) {
				if (*p >= 0x80) {
					/* 反転表示 */
					dat = (char)(*p & 0x7f);
					bcolor = GetBkColor(hDC);
					tcolor = GetTextColor(hDC);
					SetTextColor(hDC, bcolor);
					SetBkColor(hDC, tcolor);
					TextOut(hDC, xx * lCharWidth, yy * lCharHeight,
						&dat, 1);
					SetTextColor(hDC, tcolor);
					SetBkColor(hDC, bcolor);
				}
				else {
					/* 通常表示 */
					TextOut(hDC, xx * lCharWidth, yy * lCharHeight,
						(LPCTSTR)p, 1);
				}
				*q = *p;
			}
			p++;
			q++;
		}
	}

	/* 終了 */
	SelectObject(hDC, hBackup);
	DeleteObject(hFont);
}

/*
 *	MMRウインドウ
 *	リフレッシュ
 */
void FASTCALL RefreshMMR(void)
{
	HWND hWnd;
	HDC hDC;

	/* 常に呼ばれるので、存在チェックすること */
	if (hSubWnd[SWND_MMR] == NULL) {
		return;
	}

	/* 描画 */
	hWnd = hSubWnd[SWND_MMR];
	hDC = GetDC(hWnd);
	DrawMMR(hWnd, hDC);
	ReleaseDC(hWnd, hDC);
}

/*
 *	MMRウインドウ
 *	再描画
 */
static void FASTCALL PaintMMR(HWND hWnd)
{
	HDC hDC;
	PAINTSTRUCT ps;
	RECT rect;
	BYTE *p;
	int x, y;

	ASSERT(hWnd);

	/* ポインタを設定 */
	p = pMMR;

	/* ウインドウジオメトリを得る */
	GetClientRect(hWnd, &rect);
	x = rect.right / lCharWidth;
	y = rect.bottom / lCharHeight;

	/* 後半エリアをFFで埋める */
	if ((x > 0) && (y > 0)) {
		memset(&p[x * y], 0xff, x * y);
	}

	/* 描画 */
	hDC = BeginPaint(hWnd, &ps);
	ASSERT(hDC);
	DrawMMR(hWnd, hDC);
	EndPaint(hWnd, &ps);
}

/*
 *	MMRウインドウ
 *	ウインドウプロシージャ
 */
static LRESULT CALLBACK MMRProc(HWND hWnd, UINT message,
								 WPARAM wParam, LPARAM lParam)
{
	int i;

	/* メッセージ別 */
	switch (message) {
		/* ウインドウ再描画 */
		case WM_PAINT:
			/* ロックが必要 */
			LockVM();
			PaintMMR(hWnd);
			UnlockVM();
			return 0;

		/* ウインドウ削除 */
		case WM_DESTROY:
			/* メインウインドウへ自動通知 */
			for (i=0; i<SWND_MAXNUM; i++) {
				if (hSubWnd[i] == hWnd) {
					hSubWnd[i] = NULL;
					/* Drawバッファ削除 */
					free(pMMR);
					pMMR = NULL;
				}
			}
			break;
	}

	/* デフォルト ウインドウプロシージャ */
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/*
 *	MMRウインドウ
 *	ウインドウ作成
 */
HWND FASTCALL CreateMMR(HWND hParent, int index)
{
	WNDCLASSEX wcex;
	char szClassName[] = "XM7_MMR";
	char szWndName[128];
	RECT rect;
	RECT crect, wrect;
	HWND hWnd;

	ASSERT(hParent);

	/* ウインドウ矩形を計算 */
	rect.left = lCharWidth * index;
	rect.top = lCharHeight * index;
	if (rect.top >= 380) {
		rect.top -= 380;
	}
	rect.right = lCharWidth * 42;
	rect.bottom = lCharHeight * 14;

	/* ウインドウタイトルを決定、バッファ確保 */
	LoadString(hAppInstance, IDS_SWND_MMR,
				szWndName, sizeof(szWndName));
	pMMR = malloc(2 * (rect.right / lCharWidth) *
								(rect.bottom / lCharHeight));

	/* ウインドウクラスの登録 */
	memset(&wcex, 0, sizeof(wcex));
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_VREDRAW | CS_HREDRAW;
	wcex.lpfnWndProc = MMRProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hAppInstance;
	wcex.hIcon = LoadIcon(hAppInstance, MAKEINTRESOURCE(IDI_WNDICON));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szClassName;
	wcex.hIconSm = LoadIcon(hAppInstance, MAKEINTRESOURCE(IDI_WNDICON));
	RegisterClassEx(&wcex);

	/* ウインドウ作成 */
	hWnd = CreateWindow(szClassName,
						szWndName,
						WS_CHILD | WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION
						| WS_VISIBLE | WS_MINIMIZEBOX | WS_CLIPSIBLINGS,
						rect.left,
						rect.top,
						rect.right,
						rect.bottom,
						hParent,
						NULL,
						hAppInstance,
						NULL);

	/* 有効なら、サイズ補正して手前に置く */
	if (hWnd) {
		GetWindowRect(hWnd, &wrect);
		GetClientRect(hWnd, &crect);
		wrect.right += (rect.right - crect.right);
		wrect.bottom += (rect.bottom - crect.bottom);
		SetWindowPos(hWnd, HWND_TOP, wrect.left, wrect.top,
			wrect.right - wrect.left, wrect.bottom - wrect.top, SWP_NOMOVE);
	}

	/* 結果を持ち帰る */
	return hWnd;
}

/*-[ キーボードウインドウ ]--------------------------------------------------*/

/*
 *	キーボードウインドウ
 *	セットアップ(フラグ)
 */
static void FASTCALL SetupKeyboardFlag(BYTE *p, char *title, BOOL flag)
{
	char string[18];
	int i;

	/* 初期化 */
	memset(string, 0x20, 18);
	string[17] = '\0';

	/* コピー */
	for (i=0; i<17; i++) {
		if (title[i] == '\0') {
			break;
		}
		string[i] = title[i];
	}

	/* フラグに応じて設定 */
	if (flag) {
		strcpy(&string[15], "On");
	}
	else {
		strcpy(&string[14], "Off");
	}

	/* セット */
	memcpy(p, string, 17);
}

/*
 *	キーボードウインドウ
 *	セットアップ
 */
static void FASTCALL SetupKeyboard(BYTE *p, int x, int y)
{
	char string[128];

	ASSERT(p);
	ASSERT(x > 0);
	ASSERT(y > 0);

	/* 一旦スペースで埋める */
	memset(p, 0x20, x * y);

	/* コード */
	sprintf(string, "Logical Code     %04X", key_fm7);
	memcpy(&p[x * 0 + 0], string, strlen(string));
	sprintf(string, "Scan Code          %02X", key_scan);
	memcpy(&p[x * 1 + 0], string, strlen(string));

	/* コードフォーマット */
	strcpy(string, "Code Format ");
	switch (key_format) {
		case KEY_FORMAT_9BIT:
			strcat(string,"     FM-7");
			break;
		case KEY_FORMAT_FM16B:
			strcat(string, "FM-16Beta");
			break;
		case KEY_FORMAT_SCAN:
			strcat(string, "     Scan");
			break;
		default:
			ASSERT(FALSE);
	}
	memcpy(&p[x * 2 + 0], string, strlen(string));

	/* キーリピート */
	strcpy(string, "Key Repeat");
	memcpy(&p[x * 4 + 0], string, strlen(string));
	if (key_repeat_flag) {
		strcpy(string, " Enable");
	}
	else {
		strcpy(string, "Disable");
	}
	memcpy(&p[x * 4 + 14], string, strlen(string));
	sprintf(string, "Repeat Time 1  %4dms", key_repeat_time1 / 1000);
	memcpy(&p[x * 5 + 0], string, strlen(string));
	sprintf(string, "Repeat Time 2  %4dms", key_repeat_time2 / 1000);
	memcpy(&p[x * 6 + 0], string, strlen(string));

	/* フラグ */
	SetupKeyboardFlag(&p[x * 0 + 24], "CAP  LED" , caps_flag);
	SetupKeyboardFlag(&p[x * 1 + 24], "KANA LED", kana_flag);
	SetupKeyboardFlag(&p[x * 2 + 24], "INS  LED", ins_flag);
	SetupKeyboardFlag(&p[x * 3 + 24], "SHIFT", shift_flag);
	SetupKeyboardFlag(&p[x * 4 + 24], "CTRL", ctrl_flag);
	SetupKeyboardFlag(&p[x * 5 + 24], "GRAPH", graph_flag);
	SetupKeyboardFlag(&p[x * 6 + 24], "BREAK", break_flag);

	/* 時計 */
	sprintf(string, "RTC Date     %02d/%02d/%02d", rtc_year, rtc_month, rtc_day);
	memcpy(&p[x * 8 + 0], string, strlen(string));
	sprintf(string, "RTC Time     %02d:%02d:%02d", rtc_hour, rtc_minute, rtc_second);
	memcpy(&p[x * 9 + 0], string, strlen(string));
	sprintf(string, "RTC 12h/24h");
	memcpy(&p[x * 10 + 0], string, strlen(string));
	if (rtc_24h) {
		strcpy(string, "24h");
	}
	else {
		strcpy(string, "12h");
	}
	memcpy(&p[x * 10 + 18], string, strlen(string));
	sprintf(string, "RTC AM/PM");
	memcpy(&p[x * 11 + 0], string, strlen(string));
	if (rtc_pm) {
		strcpy(string, "PM");
	}
	else {
		strcpy(string, "AM");
	}
	memcpy(&p[x * 11 + 19], string, strlen(string));
	sprintf(string, "RTC Leap Mode");
	memcpy(&p[x * 12 + 0], string, strlen(string));
	sprintf(string, "%1d", rtc_leap);
	memcpy(&p[x * 12 + 20], string, strlen(string));

	/* スーパーインポーズ */
	strcpy(string, "Super Impose   ");
	switch (simpose_mode) {
		case 0:
			strcat(string, "PC");
			break;
		case 1:
			strcat(string, "On");
			break;
		case 2:
			strcat(string, "TV");
			break;
	}
	memcpy(&p[x * 8 + 24], string, strlen(string));
	strcpy(string, "Brightness   ");
	if (simpose_half) {
		strcat(string, "Half");
	}
	else {
		strcat(string, "Full");
	}
	memcpy(&p[x * 9 + 24], string, strlen(string));

	/* ディジタイズ */
	strcpy(string, "Digitize  ");
	if (digitize_enable) {
		strcat(string, " Enable");
	}
	else {
		strcat(string, "Disable");
	}
	memcpy(&p[x * 11 + 24], string, strlen(string));
	strcpy(string, "Key Wait    ");
	if (digitize_keywait) {
		strcat(string, " Wait");
	}
	else {
		strcat(string, "Ready");
	}
	memcpy(&p[x * 12 + 24], string, strlen(string));
}

/*
 *	キーボードウインドウ
 *	描画
 */
static void FASTCALL DrawKeyboard(HWND hWnd, HDC hDC)
{
	RECT rect;
	BYTE *p, *q;
	int x, y;
	int xx, yy;
	HFONT hFont;
	HFONT hBackup;

	ASSERT(hWnd);
	ASSERT(hDC);

	/* ウインドウジオメトリを得る */
	GetClientRect(hWnd, &rect);
	x = rect.right / lCharWidth;
	y = rect.bottom / lCharHeight;
	if ((x == 0) || (y == 0)) {
		return;
	}

	/* セットアップ */
	p = pKeyboard;
	if (!p) {
		return;
	}
	SetupKeyboard(p, x, y);

	/* フォントセレクト */
	hFont = CreateTextFont();
	hBackup = SelectObject(hDC, hFont);

	/* 比較描画 */
	q = &p[x * y];
	for (yy=0; yy<y; yy++) {
		for (xx=0; xx<x; xx++) {
			if (*p != *q) {
				TextOut(hDC, xx * lCharWidth, yy * lCharHeight,
					(LPCTSTR)p, 1);
				*q = *p;
			}
			p++;
			q++;
		}
	}

	/* 終了 */
	SelectObject(hDC, hBackup);
	DeleteObject(hFont);
}

/*
 *	キーボードウインドウ
 *	リフレッシュ
 */
void FASTCALL RefreshKeyboard(void)
{
	HWND hWnd;
	HDC hDC;

	/* 常に呼ばれるので、存在チェックすること */
	if (hSubWnd[SWND_KEYBOARD] == NULL) {
		return;
	}

	/* 描画 */
	hWnd = hSubWnd[SWND_KEYBOARD];
	hDC = GetDC(hWnd);
	DrawKeyboard(hWnd, hDC);
	ReleaseDC(hWnd, hDC);
}

/*
 *	キーボードウインドウ
 *	再描画
 */
static void FASTCALL PaintKeyboard(HWND hWnd)
{
	HDC hDC;
	PAINTSTRUCT ps;
	RECT rect;
	BYTE *p;
	int x, y;

	ASSERT(hWnd);

	/* ポインタを設定 */
	p = pKeyboard;

	/* ウインドウジオメトリを得る */
	GetClientRect(hWnd, &rect);
	x = rect.right / lCharWidth;
	y = rect.bottom / lCharHeight;

	/* 後半エリアをFFで埋める */
	if ((x > 0) && (y > 0)) {
		memset(&p[x * y], 0xff, x * y);
	}

	/* 描画 */
	hDC = BeginPaint(hWnd, &ps);
	ASSERT(hDC);
	DrawKeyboard(hWnd, hDC);
	EndPaint(hWnd, &ps);
}

/*
 *	キーボードウインドウ
 *	ウインドウプロシージャ
 */
static LRESULT CALLBACK KeyboardProc(HWND hWnd, UINT message,
								 WPARAM wParam, LPARAM lParam)
{
	int i;

	/* メッセージ別 */
	switch (message) {
		/* ウインドウ再描画 */
		case WM_PAINT:
			/* ロックが必要 */
			LockVM();
			PaintKeyboard(hWnd);
			UnlockVM();
			return 0;

		/* ウインドウ削除 */
		case WM_DESTROY:
			/* メインウインドウへ自動通知 */
			for (i=0; i<SWND_MAXNUM; i++) {
				if (hSubWnd[i] == hWnd) {
					hSubWnd[i] = NULL;
					/* Drawバッファ削除 */
					free(pKeyboard);
					pKeyboard = NULL;
				}
			}
			break;
	}

	/* デフォルト ウインドウプロシージャ */
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/*
 *	キーボードウインドウ
 *	ウインドウ作成
 */
HWND FASTCALL CreateKeyboard(HWND hParent, int index)
{
	WNDCLASSEX wcex;
	char szClassName[] = "XM7_Keyboard";
	char szWndName[128];
	RECT rect;
	RECT crect, wrect;
	HWND hWnd;

	ASSERT(hParent);

	/* ウインドウ矩形を計算 */
	rect.left = lCharWidth * index;
	rect.top = lCharHeight * index;
	if (rect.top >= 380) {
		rect.top -= 380;
	}
	rect.right = lCharWidth * 41;
	rect.bottom = lCharHeight * 13;

	/* ウインドウタイトルを決定、バッファ確保 */
	LoadString(hAppInstance, IDS_SWND_KEYBOARD,
				szWndName, sizeof(szWndName));
	pKeyboard = malloc(2 * (rect.right / lCharWidth) *
								(rect.bottom / lCharHeight));

	/* ウインドウクラスの登録 */
	memset(&wcex, 0, sizeof(wcex));
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_VREDRAW | CS_HREDRAW;
	wcex.lpfnWndProc = KeyboardProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hAppInstance;
	wcex.hIcon = LoadIcon(hAppInstance, MAKEINTRESOURCE(IDI_WNDICON));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szClassName;
	wcex.hIconSm = LoadIcon(hAppInstance, MAKEINTRESOURCE(IDI_WNDICON));
	RegisterClassEx(&wcex);

	/* ウインドウ作成 */
	hWnd = CreateWindow(szClassName,
						szWndName,
						WS_CHILD | WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION
						| WS_VISIBLE | WS_MINIMIZEBOX | WS_CLIPSIBLINGS,
						rect.left,
						rect.top,
						rect.right,
						rect.bottom,
						hParent,
						NULL,
						hAppInstance,
						NULL);

	/* 有効なら、サイズ補正して手前に置く */
	if (hWnd) {
		GetWindowRect(hWnd, &wrect);
		GetClientRect(hWnd, &crect);
		wrect.right += (rect.right - crect.right);
		wrect.bottom += (rect.bottom - crect.bottom);
		SetWindowPos(hWnd, HWND_TOP, wrect.left, wrect.top,
			wrect.right - wrect.left, wrect.bottom - wrect.top, SWP_NOMOVE);
	}

	/* 結果を持ち帰る */
	return hWnd;
}

#endif	/* _WIN32 */
