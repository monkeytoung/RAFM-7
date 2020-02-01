/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API ウインドウ表示 ]
 */

#ifdef _WIN32

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdlib.h>
#include "xm7.h"
#include "subctrl.h"
#include "display.h"
#include "multipag.h"
#include "ttlpalet.h"
#include "apalet.h"
#include "w32.h"
#include "w32_draw.h"
#include "w32_gdi.h"

/*
 *	グローバル ワーク
 */
WORD rgbTTLGDI[8];							/* デジタルパレット */
WORD rgbAnalogGDI[4096];					/* アナログパレット */
BYTE *pBitsGDI;								/* ビットデータ */

/*
 *	スタティック ワーク
 */
static BOOL bAnalog;						/* アナログモードフラグ */
static HBITMAP hBitmap;						/* ビットマップ ハンドル */
static UINT nDrawTop;						/* 描画範囲上 */
static UINT nDrawBottom;					/* 描画範囲下 */
static BOOL bPaletFlag;						/* パレット変更フラグ */
static BOOL bClearFlag;						/* クリアフラグ */

/*
 *	アセンブラ関数のためのプロトタイプ宣言
 */
#ifdef __cplusplus
extern "C" {
#endif
void Render640GDI(int first, int last);
void Render320GDI(int first, int last);
#ifdef __cplusplus
}
#endif

/*
 *	初期化
 */
void FASTCALL InitGDI(void)
{
	/* ワークエリア初期化 */
	bAnalog = FALSE;
	hBitmap = NULL;
	pBitsGDI = NULL;
	memset(rgbTTLGDI, 0, sizeof(rgbTTLGDI));
	memset(rgbAnalogGDI, 0, sizeof(rgbAnalogGDI));

	nDrawTop = 0;
	nDrawBottom = 200;
	bPaletFlag = FALSE;
}

/*
 *	クリーンアップ
 */
void FASTCALL CleanGDI(void)
{
	if (hBitmap) {
		DeleteObject(hBitmap);
		hBitmap = NULL;
		pBitsGDI = NULL;
	}
}

/*
 *	GDIセレクト共通
 */
static BOOL FASTCALL SelectSub(HWND hWnd)
{
	BITMAPINFOHEADER *pbmi;
	HDC hDC;

	ASSERT(hWnd);
	ASSERT(!hBitmap);
	ASSERT(!pBitsGDI);

	/* メモリ確保 */
	pbmi = (BITMAPINFOHEADER*)malloc(sizeof(BITMAPINFOHEADER) +
									 sizeof(RGBQUAD));
	if (!pbmi) {
		return FALSE;
	}
	memset(pbmi, 0, sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD));

	/* ビットマップ情報作成 */
	pbmi->biSize = sizeof(BITMAPINFOHEADER);
	pbmi->biWidth = 640;
	pbmi->biHeight = -400;
	pbmi->biPlanes = 1;
	pbmi->biBitCount = 16;
	pbmi->biCompression = BI_RGB;

	/* DC取得、DIBセクション作成 */
	hDC = GetDC(hWnd);
	hBitmap = CreateDIBSection(hDC, (BITMAPINFO*)pbmi, DIB_RGB_COLORS,
								(void**)&pBitsGDI, NULL, 0);
	ReleaseDC(hWnd, hDC);
	free(pbmi);
	if (!hBitmap) {
		return FALSE;
	}

	/* 全エリアを、一度クリア */
	bClearFlag = TRUE;
	return TRUE;
}

/*
 *  640x200、デジタルモード
 *	セレクト
 */
static BOOL FASTCALL Select640(HWND hWnd)
{
	if (!pBitsGDI) {
		if (!SelectSub(hWnd)) {
			return FALSE;
		}
	}

	/* 全領域無効 */
	nDrawTop = 0;
	nDrawBottom = 200;
	bPaletFlag = TRUE;

	/* デジタルモード */
	bAnalog = FALSE;

	return TRUE;
}

/*
 *  320x200、アナログモード
 *	セレクト
 */
static BOOL FASTCALL Select320(HWND hWnd)
{
	if (!pBitsGDI) {
		if (!SelectSub(hWnd)) {
			return FALSE;
		}
	}

	/* 全領域無効 */
	nDrawTop = 0;
	nDrawBottom = 200;
	bPaletFlag = TRUE;

	/* アナログモード */
	bAnalog = TRUE;

	return TRUE;
}

/*
 *	セレクト
 */
BOOL FASTCALL SelectGDI(HWND hWnd)
{
	ASSERT(hWnd);

	/* 未初期化なら */
	if (!pBitsGDI) {
		if (mode320) {
			return Select320(hWnd);
		}
		else {
			return Select640(hWnd);
		}
	}

	/* モードが一致していれば、何もしない */
	if (mode320) {
		if (bAnalog) {
			return TRUE;
		}
	}
	else {
		if (!bAnalog) {
			return TRUE;
		}
	}

	/* セレクト */
	if (mode320) {
		return Select320(hWnd);
	}
	else {
		return Select640(hWnd);
	}
}

/*-[ 描画 ]-----------------------------------------------------------------*/

/*
 *	オールクリア
 */
static void FASTCALL AllClear(void)
{
	/* すべてクリア */
	memset(pBitsGDI, 0, 640 * 400 * 2);

	/* 全領域をレンダリング対象とする */
	nDrawTop = 0;
	nDrawBottom = 200;

	bClearFlag = FALSE;
}

/*
 *	フルスキャン
 */
static void FASTCALL RenderFullScan(void)
{
	BYTE *p;
	BYTE *q;
	UINT u;

	/* ポインタ初期化 */
	p = &pBitsGDI[nDrawTop * 2 * 640 * 2];
	q = p + 640 * 2;

	/* ループ */
	for (u=nDrawTop; u<nDrawBottom; u++) {
		memcpy(q, p, 640 * 2);
		p += 640 * 2 * 2;
		q += 640 * 2 * 2;
	}
}

/*
 *	640x400、デジタルモード
 *	パレット設定
 */
static void FASTCALL Palet640(void)
{
	int i;
	int vpage;

	/* パレットテーブル */
	static const WORD rgbTable[] = {
		0x0000 | 0x0000 | 0x0000,
		0x0000 | 0x0000 | 0x001f,
		0x7c00 | 0x0000 | 0x0000,
		0x7c00 | 0x0000 | 0x001f,
		0x0000 | 0x03e0 | 0x0000,
		0x0000 | 0x03e0 | 0x001f,
		0x7c00 | 0x03e0 | 0x0000,
		0x7c00 | 0x03e0 | 0x001f
	};

	/* マルチページより、表示プレーン情報を得る */
	vpage = (~(multi_page >> 4)) & 0x07;

	/* 640x200、デジタルパレット */
	for (i=0; i<8; i++) {
		if (crt_flag) {
			/* CRT ON */
			rgbTTLGDI[i] = rgbTable[ttl_palet[i & vpage] + 0];
		}
		else {
			/* CRT OFF */
			rgbTTLGDI[i] = 0;
		}
	}
}

/*
 *	640x200、デジタルモード
 *	描画
 */
static void FASTCALL Draw640(HWND hWnd, HDC hDC)
{
	HDC hMemDC;
	HBITMAP hDefBitmap;

	ASSERT(hWnd);
	ASSERT(hDC);

	/* パレット設定 */
	if (bPaletFlag) {
		Palet640();
	}

	/* クリア処理 */
	if (bClearFlag) {
		AllClear();
	}

	/* レンダリング */
	if (nDrawTop >= nDrawBottom) {
		return;
	}
	Render640GDI(nDrawTop, nDrawBottom);
	if (bFullScan) {
		RenderFullScan();
	}

	/* メモリDC作成、オブジェクト選択 */
	hMemDC = CreateCompatibleDC(hDC);
	ASSERT(hMemDC);
	hDefBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);

	/* デスクトップの状況いかんでは、拒否もあり得る様 */
	if (hDefBitmap) {
		BitBlt(hDC, 0, nDrawTop * 2, 640, (nDrawBottom - nDrawTop) * 2,
				hMemDC, 0, nDrawTop * 2, SRCCOPY);
		SelectObject(hMemDC, hDefBitmap);

		/* 次回に備え、ワークリセット */
		nDrawTop = 200;
		nDrawBottom = 0;
		bPaletFlag = FALSE;
	}
	else {
		/* 再描画を起こす */
		InvalidateRect(hWnd, NULL, FALSE);
	}

	/* メモリDC削除 */
	DeleteDC(hMemDC);
}

/*
 *	320x200、アナログモード
 *	パレット設定
 */
static void FASTCALL Palet320(void)
{
	int i, j;
	WORD color;
	BYTE r, g, b;
	int amask;

	/* アナログマスクを作成 */
	amask = 0;
	if (!(multi_page & 0x10)) {
		amask |= 0x000f;
	}
	if (!(multi_page & 0x20)) {
		amask |= 0x00f0;
	}
	if (!(multi_page & 0x40)) {
		amask |= 0x0f00;
	}

	for (i=0; i<4096; i++) {
		/* 最下位から5bitづつB,G,R */
		color = 0;
		if (crt_flag) {
			j = i & amask;
			r = apalet_r[j];
			g = apalet_g[j];
			b = apalet_b[j];
		}
		else {
			r = 0;
			g = 0;
			b = 0;
		}

		/* R */
		r <<= 1;
		if (r > 0) {
			r |= 0x01;
		}
		color |= (WORD)r;
		color <<= 5;

		/* G */
		g <<= 1;
		if (g > 0) {
			g |= 0x01;
		}
		color |= (WORD)g;
		color <<= 5;

		/* B */
		b <<= 1;
		if (b > 0) {
			b |= 0x01;
		}
		color |= (WORD)b;

		/* セット */
		rgbAnalogGDI[i] = color;
	}
}

/*
 *	320x200、アナログモード
 *	描画
 */
static void FASTCALL Draw320(HWND hWnd, HDC hDC)
{
	HDC hMemDC;
	HBITMAP hDefBitmap;

	ASSERT(hWnd);
	ASSERT(hDC);

	/* パレット設定 */
	if (bPaletFlag) {
		Palet320();
	}

	/* クリア処理 */
	if (bClearFlag) {
		AllClear();
	}

	/* レンダリング */
	if (nDrawTop >= nDrawBottom) {
		return;
	}
	Render320GDI(nDrawTop, nDrawBottom);
	if (bFullScan) {
		RenderFullScan();
	}

	/* メモリDC作成、オブジェクト選択 */
	hMemDC = CreateCompatibleDC(hDC);
	ASSERT(hMemDC);
	hDefBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);

	/* デスクトップの状況いかんでは、拒否もあり得る様 */
	if (hDefBitmap) {
		BitBlt(hDC, 0, nDrawTop * 2, 640, (nDrawBottom - nDrawTop) * 2,
				hMemDC, 0, nDrawTop * 2, SRCCOPY);
		SelectObject(hMemDC, hDefBitmap);

		/* 次回に備え、ワークリセット */
		nDrawTop = 200;
		nDrawBottom = 0;
		bPaletFlag = FALSE;
	}
	else {
		/* 再描画を起こす */
		InvalidateRect(hWnd, NULL, FALSE);
	}

	/* メモリDC解放 */
	DeleteDC(hMemDC);
}

/*
 *	描画
 */
void FASTCALL DrawGDI(HWND hWnd, HDC hDC)
{
	ASSERT(hWnd);
	ASSERT(hDC);

	/* 640-320 自動切り替え */
	SelectGDI(hWnd);

	/* どちらかを使って描画 */
	if (bAnalog) {
		Draw320(hWnd, hDC);
	}
	else {
		Draw640(hWnd, hDC);
	}
}

/*-[ VMとの接続 ]-----------------------------------------------------------*/

/*
 *	VRAMセット
 */
void FASTCALL VramGDI(WORD addr)
{
	UINT y;

	/* y座標算出 */
	if (bAnalog) {
		addr &= 0x1fff;
		y = (UINT)(addr / 40);
	}
	else {
		addr &= 0x3fff;
		y = (UINT)(addr / 80);
	}
	if (y >= 200) {
		return;
	}

	/* 更新 */
	if (nDrawTop > y) {
		nDrawTop = y;
	}
	if (nDrawBottom <= y) {
		nDrawBottom = y + 1;
	}
}

/*
 *	TTLパレットセット
 */
void FASTCALL DigitalGDI(void)
{
	bPaletFlag = TRUE;
	nDrawTop = 0;
	nDrawBottom = 200;
}

/*
 *	アナログパレットセット
 */
void FASTCALL AnalogGDI(void)
{
	bPaletFlag = TRUE;
	nDrawTop = 0;
	nDrawBottom = 200;
}

/*
 *	再描画要求
 */
void FASTCALL ReDrawGDI(void)
{
	/* 再描画 */
	nDrawTop = 0;
	nDrawBottom = 200;
	bPaletFlag = TRUE;
	bClearFlag = TRUE;
}

#endif	/* _WIN32 */
