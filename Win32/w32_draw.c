/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API 表示 ]
 */

#ifdef _WIN32

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include "xm7.h"
#include "device.h"
#include "subctrl.h"
#include "display.h"
#include "w32.h"
#include "w32_gdi.h"
#include "w32_dd.h"
#include "w32_draw.h"

/*
 *	グローバル ワーク
 */
BOOL bFullScreen;							/* フルスクリーンフラグ */
BOOL bFullScan;								/* フルスキャンフラグ */

/*
 *	初期化
 */
void FASTCALL InitDraw(void)
{
	/* ワークエリア初期化 */
	bFullScreen = FALSE;
	bFullScan = FALSE;

	/* 起動後はGDIなので、GDIを初期化 */
	InitGDI();
}

/*
 *	クリーンアップ
 */
void FASTCALL CleanDraw(void)
{
	/* フルスクリーンフラグに応じて、クリーンアップ */
	if (bFullScreen) {
		CleanDD();
	}
	else {
		CleanGDI();
	}
}

/*
 *  セレクト
 */
BOOL FASTCALL SelectDraw(HWND hWnd)
{
	ASSERT(hWnd);

	/* 起動後はGDIを選択 */
	return SelectGDI(hWnd);
}

/*
 *	モード切り替え
 */
void FASTCALL ModeDraw(HWND hWnd, BOOL bFullFlag)
{
	ASSERT(hWnd);

	/* 現状と一致していれば、変える必要なし */
	if (bFullFlag == bFullScreen) {
		return;
	}

	if (bFullFlag) {
		/* フルスクリーンへ */
		CleanGDI();
		InitDD();
		bFullScreen = TRUE;
		if (!SelectDD()) {
			/* フルスクリーン失敗 */
			bFullScreen = FALSE;
			CleanDD();
			InitGDI();
			SelectGDI(hWnd);
		}
	}
	else {
		/* ウインドウへ */
		bFullScreen = FALSE;
		CleanDD();
		InitGDI();
		if (!SelectGDI(hWnd)) {
			/* ウインドウ失敗 */
			CleanGDI();
			bFullScreen = TRUE;
			InitDD();
			SelectDD();
		}
	}
}

/*
 *	描画(通常)
 */
void FASTCALL OnDraw(HWND hWnd, HDC hDC)
{
	ASSERT(hWnd);
	ASSERT(hDC);

	/* エラーなら何もしない */
	if (nErrorCode == 0) {
		if (bFullScreen) {
			DrawDD();
		}
		else {
			DrawGDI(hWnd, hDC);
		}
	}
}

/*
 *	描画(WM_PAINT)
 */
void FASTCALL OnPaint(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hDC;

	ASSERT(hWnd);

	hDC = BeginPaint(hWnd, &ps);

	/* エラーなら何もしない */
	if (nErrorCode == 0) {
		/* 再描画指示 */
		if (bFullScreen) {
			ReDrawDD();
		}
		else {
			ReDrawGDI();
		}

		/* 描画 */
		OnDraw(hWnd, hDC);
	}

	EndPaint(hWnd, &ps);
}

/*-[ VMとの接続 ]-----------------------------------------------------------*/

/*
 *	VRAM書き込み通知
 */
void FASTCALL vram_notify(WORD addr, BYTE dat)
{
	/* アクティブページがディスプレイページと異なれば、問題なし */
	if (!mode320) {
		if (vram_active != vram_display) {
			return;
		}
	}

	if (bFullScreen) {
		VramDD(addr);
	}
	else {
		VramGDI(addr);
	}
}

/*
 *	TTLパレット通知
 */
void FASTCALL ttlpalet_notify(void)
{
	if (bFullScreen) {
		DigitalDD();
	}
	else {
		DigitalGDI();
	}
}

/*
 *	アナログパレット通知
 */
void FASTCALL apalet_notify(void)
{
	if (bFullScreen) {
		AnalogDD();
	}
	else {
		AnalogGDI();
	}
}

/*
 *  再描画要求通知
 */
void FASTCALL display_notify(void)
{
	if (bFullScreen) {
		/* ReDrawは無駄なクリアを含むので、多少細工 */
		AnalogDD();
	}
	else {
		AnalogGDI();
	}
}

/*
 *	ディジタイズ要求通知
 */
void FASTCALL digitize_notify(void)
{
}

#endif	/* _WIN32 */
