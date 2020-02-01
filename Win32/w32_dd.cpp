/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API DirectDraw ]
 */

#ifdef _WIN32

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define DIRECTDRAW_VERSION		0x300	/* DirectX3を指定 */
#include <ddraw.h>
#include <assert.h>
#include "xm7.h"
#include "subctrl.h"
#include "display.h"
#include "multipag.h"
#include "ttlpalet.h"
#include "apalet.h"
#include "fdc.h"
#include "tapelp.h"
#include "keyboard.h"
#include "w32.h"
#include "w32_bar.h"
#include "w32_res.h"
#include "w32_draw.h"
#include "w32_dd.h"

/*
 *	グローバル ワーク
 */
WORD rgbTTLDD[8];						/* 640x200 パレット */
WORD rgbAnalogDD[4096];					/* 320x200 パレット */
BOOL bDD480Line;						/* 640x480優先フラグ */
BOOL bDD480Status;						/* 640x480ステータスフラグ */

/*
 *	スタティック ワーク
 */
static BOOL bAnalog;					/* アナログモードフラグ */
static RECT BkRect;						/* ウインドウ時の矩形 */
static BOOL bMouseCursor;				/* マウスカーソルフラグ */
static LPDIRECTDRAW2 lpdd2;				/* DirectDraw2 */
static LPDIRECTDRAWSURFACE lpdds[2];	/* DirectDrawSurface3 */
static LPDIRECTDRAWCLIPPER lpddc;		/* DirectDrawClipper */
static UINT nPixelFormat;				/* 320x200 ピクセルフォーマット */
static UINT nDrawTop;					/* 描画範囲上 */
static UINT nDrawBottom;				/* 描画範囲下 */
static BOOL bPaletFlag;					/* パレット変更フラグ */
static BOOL bDD480Flag;					/* 640x480 フラグ */
static BOOL bClearFlag;					/* 上下クリアフラグ */

static char szRunMessage[128];			/* RUNメッセージ */
static char szStopMessage[128];			/* STOPメッセージ */
static char szCaption[128];				/* キャプション */
static int nCAP;						/* CAPキー */
static int nKANA;						/* かなキー */
static int nINS;						/* INSキー */
static int nDrive[2];					/* フロッピードライブ */
static char szDrive[2][16 + 1];			/* フロッピードライブ */
static int nTape;						/* テープ */

/*
 *	アセンブラ関数のためのプロトタイプ宣言
 */
#ifdef __cplusplus
extern "C" {
#endif
void Render640DD(LPVOID lpSurface, LONG lPitch, int first, int last);
void Render320DD(LPVOID lpSurface, LONG lPitch, int first, int last);
#ifdef __cplusplus
}
#endif

/*
 *	初期化
 */
void FASTCALL InitDD(void)
{
	/* ワークエリア初期化(設定ワークは変更しない) */
	bAnalog = FALSE;
	bMouseCursor = TRUE;
	lpdd2 = NULL;
	memset(lpdds, 0, sizeof(lpdds));
	lpddc = NULL;
	bClearFlag = TRUE;

	/* ステータスライン */
	szCaption[0] = '\0';
	nCAP = -1;
	nKANA = -1;
	nINS = -1;
	nDrive[0] = -1;
	nDrive[1] = -1;
	szDrive[0][0] = '\0';
	szDrive[1][0] = '\0';
	nTape = -1;

	/* メッセージをロード */
	if (LoadString(hAppInstance, IDS_RUNCAPTION,
					szRunMessage, sizeof(szRunMessage)) == 0) {
		szRunMessage[0] = '\0';
	}
	if (LoadString(hAppInstance, IDS_STOPCAPTION,
					szStopMessage, sizeof(szStopMessage)) == 0) {
		szStopMessage[0] = '\0';
	}
}

/*
 *	クリーンアップ
 */
void FASTCALL CleanDD(void)
{
	DWORD dwStyle;
	RECT brect;
	int i;

	/* DirectDrawClipper */
	if (lpddc) {
		lpddc->Release();
		lpddc = NULL;
	}

	/* DirectDrawSurface3 */
	for (i=0; i<2; i++) {
		if (lpdds[i]) {
			lpdds[i]->Release();
			lpdds[i] = NULL;
		}
	}

	/* DirectDraw2 */
	if (lpdd2) {
		lpdd2->Release();
		lpdd2 = NULL;
	}

	/* ウインドウスタイルを戻す */
	dwStyle = GetWindowLong(hMainWnd, GWL_STYLE);
	dwStyle &= ~WS_POPUP;
	dwStyle |= (WS_CAPTION | WS_BORDER | WS_SYSMENU);
	SetWindowLong(hMainWnd, GWL_STYLE, dwStyle);
	dwStyle = GetWindowLong(hMainWnd, GWL_EXSTYLE);
	dwStyle |= WS_EX_WINDOWEDGE;
	SetWindowLong(hMainWnd, GWL_EXSTYLE, dwStyle);

	/* ウインドウ位置を戻す */
	SetWindowPos(hMainWnd, HWND_NOTOPMOST, BkRect.left, BkRect.top,
		(BkRect.right - BkRect.left), (BkRect.bottom - BkRect.top),
		SWP_DRAWFRAME);

	MoveWindow(hDrawWnd, 0, 0, 640, 400, TRUE);
	if (hStatusBar) {
		GetWindowRect(hStatusBar, &brect);
		MoveWindow(hStatusBar, 0, 400,
					(brect.right - brect.left),
					(brect.bottom - brect.top),
					TRUE);
	}
}

/*
 *	セレクト
 */
BOOL FASTCALL SelectDD(void)
{
	DWORD dwStyle;
	LPDIRECTDRAW lpdd;
	DDSURFACEDESC ddsd;
	DDPIXELFORMAT ddpf;
	RECT brect;

	/* assert */
	ASSERT(hMainWnd);

	/* ウインドウ矩形を記憶する */
	GetWindowRect(hMainWnd, &BkRect);

	/* ウインドウスタイルを変更 */
	dwStyle = GetWindowLong(hMainWnd, GWL_STYLE);
	dwStyle &= ~(WS_CAPTION | WS_BORDER | WS_SYSMENU);
	dwStyle |= WS_POPUP;
	SetWindowLong(hMainWnd, GWL_STYLE, dwStyle);
	dwStyle = GetWindowLong(hMainWnd, GWL_EXSTYLE);
	dwStyle &= ~WS_EX_WINDOWEDGE;
	SetWindowLong(hMainWnd, GWL_EXSTYLE, dwStyle);

	/* DirectDrawオブジェクトを作成 */
	if (FAILED(DirectDrawCreate(NULL, &lpdd, NULL))) {
		return FALSE;
	}

	/* 協調モードを設定 */
	if (FAILED(lpdd->SetCooperativeLevel(hMainWnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN))) {
		lpdd->Release();
		return FALSE;
	}

	/* DirectDraw2インタフェースを取得 */
	if (FAILED(lpdd->QueryInterface(IID_IDirectDraw2, (LPVOID*)&lpdd2))) {
		lpdd->Release();
		return FALSE;
	}

	/* ここまで来れば、DirectDrawはもはや必要ない */
	lpdd->Release();

	/* 画面モードを設定 */
	if (bDD480Line) {
		bDD480Flag = TRUE;
		if (FAILED(lpdd2->SetDisplayMode(640, 480, 16, 0, 0))) {
			bDD480Flag = FALSE;
			if (FAILED(lpdd2->SetDisplayMode(640, 400, 16, 0, 0))) {
				return FALSE;
			}
		}
	}
	else {
		bDD480Flag = FALSE;
		if (FAILED(lpdd2->SetDisplayMode(640, 400, 16, 0, 0))) {
			bDD480Flag = TRUE;
			if (FAILED(lpdd2->SetDisplayMode(640, 480, 16, 0, 0))) {
				return FALSE;
			}
		}
	}

	/* プライマリサーフェイスを作成 */
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	if (FAILED(lpdd2->CreateSurface(&ddsd, &lpdds[0], NULL))) {
		return FALSE;
	}

	/* ワークサーフェイスを作成(DDSCAPS_SYSTEMMEMORYを指定する) */
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
	ddsd.dwWidth = 640;
	if (bDD480Flag) {
		ddsd.dwHeight = 480;
	}
	else {
		ddsd.dwHeight = 400;
	}
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	if (FAILED(lpdd2->CreateSurface(&ddsd, &lpdds[1], NULL))) {
		return FALSE;
	}

	/* ピクセルフォーマットを得る */
	memset(&ddpf, 0, sizeof(ddpf));
	ddpf.dwSize = sizeof(ddpf);
	if (FAILED(lpdds[1]->GetPixelFormat(&ddpf))) {
		return FALSE;
	}

	/* ピクセルフォーマットをチェック。HELで規定されている２タイプのみ対応 */
	if (!(ddpf.dwFlags & DDPF_RGB)) {
		return FALSE;
	}
	nPixelFormat = 0;
	if ((ddpf.dwRBitMask == 0xf800) &&
		(ddpf.dwGBitMask == 0x07e0) &&
		(ddpf.dwBBitMask == 0x001f)) {
		nPixelFormat = 1;
	}
	if ((ddpf.dwRBitMask == 0x7c00) &&
		(ddpf.dwGBitMask == 0x03e0) &&
		(ddpf.dwBBitMask == 0x001f)) {
		nPixelFormat = 2;
	}
	if (nPixelFormat == 0) {
		return FALSE;
	}

	/* クリッパーを作成、割り当て */
	if (FAILED(lpdd2->CreateClipper(NULL, &lpddc, NULL))) {
		return FALSE;
	}
	if (FAILED(lpddc->SetHWnd(NULL, hDrawWnd))) {
		return FALSE;
	}

	/* ウインドウサイズを変更(640x480時) */
	if (bDD480Flag) {
		if (hStatusBar) {
			GetWindowRect(hStatusBar, &brect);
		}
		else {
			brect.top = 0;
			brect.bottom = 0;
		}
		MoveWindow(hMainWnd, 0, 0, 640, 480 + (brect.bottom - brect.top), TRUE);
		MoveWindow(hDrawWnd, 0, 0, 640, 480, TRUE);
		if (hStatusBar) {
			MoveWindow(hStatusBar, 0, 480, 0, (brect.bottom - brect.top), TRUE);
		}
	}

	/* ワークセット、完了 */
	bAnalog = FALSE;
	ReDrawDD();
	return TRUE;
}

/*-[ 描画 ]-----------------------------------------------------------------*/

/*
 *	全領域クリア
 */
static void FASTCALL AllClear()
{
	DDSURFACEDESC ddsd;
	HRESULT hResult;
	RECT rect;
	BYTE *p;
	int i;

	/* フラグチェック */
	if (!bClearFlag) {
		return;
	}

	/* サーフェイスをロック */
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	hResult = lpdds[1]->Lock(NULL, &ddsd,
							 DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);
	if (hResult != DD_OK) {
		/* サーフェイスがロストしていれば、リストア */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[1]->Restore();
		}
		/* 次回は全領域更新 */
		ReDrawDD();
		return;
	}

	/* オールクリア */
	p = (BYTE *)ddsd.lpSurface;
	if (bDD480Flag) {
		for (i=0; i<480; i++) {
			memset(p, 0, 640 * 2);
			p += ddsd.lPitch;
		}
	}
	else {
		for (i=0; i<400; i++) {
			memset(p, 0, 640 * 2);
			p += ddsd.lPitch;
		}
	}

	/* サーフェイスをアンロック */
	lpdds[1]->Unlock(ddsd.lpSurface);

	/* ワークリセット */
	bClearFlag = FALSE;
	nDrawTop = 0;
	nDrawBottom = 200;

	/* ステータスラインをクリア */
	szCaption[0] = '\0';
	nCAP = -1;
	nKANA = -1;
	nINS = -1;
	nDrive[0] = -1;
	nDrive[1] = -1;
	szDrive[0][0] = '\0';
	szDrive[1][0] = '\0';
	nTape = -1;

	/* 640x480、ステータスなし時のみBlt */
	if (!bDD480Flag || bDD480Status) {
		return;
	}

	/* 条件が揃えば、Blt */
	rect.top = 0;
	rect.left = 0;
	rect.right = 640;
	rect.bottom = 40;
	hResult = lpdds[0]->Blt(&rect, lpdds[1], &rect, DDBLT_WAIT, NULL);
	if (hResult != DD_OK) {
		/* サーフェイスがロストしていれば、リストア */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[0]->Restore();
			lpdds[1]->Restore();
		}
		/* 次回は全領域更新 */
		ReDrawDD();
		return;
	}
	rect.top = 440;
	rect.left = 0;
	rect.right = 640;
	rect.bottom = 480;
	hResult = lpdds[0]->Blt(&rect, lpdds[1], &rect, DDBLT_WAIT, NULL);
	if (hResult != DD_OK) {
		/* サーフェイスがロストしていれば、リストア */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[0]->Restore();
			lpdds[1]->Restore();
		}
		/* 次回は全領域更新 */
		ReDrawDD();
		return;
	}
}

/*
 *	スキャンライン描画
 */
static void FASTCALL RenderFullScan(BYTE *pSurface, LONG lPitch)
{
	UINT u;

	/* フラグチェック */
	if (!bFullScan) {
		return;
	}

	/* 初期設定 */
	pSurface += nDrawTop * 2 * lPitch;

	/* ループ */
	for (u=nDrawTop; u<nDrawBottom; u++) {
		memcpy(&pSurface[lPitch], pSurface, 640 * 2);
		pSurface += (lPitch * 2);
	}
}

/*
 *	ステータスライン(キャプション)描画
 */
static void FASTCALL DrawCaption(void)
{
	HDC hDC;
	HRESULT hResult;
	RECT rect;

	ASSERT(bDD480Flag);
	ASSERT(bDD480Status);

	/* DC取得 */
	hResult = lpdds[1]->GetDC(&hDC);
	if (hResult != DD_OK) {
		/* サーフェイスがロストしていれば、リストア */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[1]->Restore();
		}
		/* 次回は全領域更新 */
		ReDrawDD();
		return;
	}

	/* ExtTextOutを使い、一度で描画 */
	SetBkColor(hDC, RGB(0, 0, 0));
	SetTextColor(hDC, RGB(255, 255, 255));
	rect.top = 0;
	rect.left = 0;
	rect.right = 640;
	rect.bottom = 40;
	ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rect,
							 szCaption, strlen(szCaption), NULL);

	/* DCを解放 */
	hResult = lpdds[1]->ReleaseDC(hDC);
	if (hResult != DD_OK) {
		/* サーフェイスがロストしていれば、リストア */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[1]->Restore();
		}
		/* 次回は全領域更新 */
		ReDrawDD();
		return;
	}

	/* Blt */
	hResult = lpdds[0]->Blt(&rect, lpdds[1], &rect, DDBLT_WAIT, NULL);
	if (hResult != DD_OK) {
		/* サーフェイスがロストしていれば、リストア */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[0]->Restore();
			lpdds[1]->Restore();
		}
		/* 次回は全領域更新 */
		ReDrawDD();
		return;
	}
}

/*
 *	ステータスライン(キャプション)
 */
static BOOL FASTCALL StatusCaption(void)
{
	char string[256];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];

	/* 動作状況に応じて、コピー */
	if (run_flag) {
		strcpy(string, szRunMessage);
	}
	else {
		strcpy(string, szStopMessage);
	}
	strcat(string, " ");

	/* フロッピーディスクドライブ 0 */
	if (fdc_ready[0] != FDC_TYPE_NOTREADY) {
		strcat(string, "- ");

		/* ファイルネーム＋拡張子のみ取り出す */
		_splitpath(fdc_fname[0], drive, dir, fname, ext);
		strcat(string, fname);
		strcat(string, ext);
		strcat(string, " ");
	}

	/* フロッピーディスクドライブ 1 */
	if (fdc_ready[1] != FDC_TYPE_NOTREADY) {
		if ((strcmp(fdc_fname[0], fdc_fname[1]) != 0) ||
			 (fdc_ready[0] == FDC_TYPE_NOTREADY)) {
			strcat(string, "(");

			/* ファイルネーム＋拡張子のみ取り出す */
			_splitpath(fdc_fname[1], drive, dir, fname, ext);
			strcat(string, fname);
			strcat(string, ext);
			strcat(string, ") ");
		}
	}

	/* テープ */
	if (tape_fileh != -1) {
		strcat(string, "- ");

		/* ファイルネーム＋拡張子のみ取り出す */
		_splitpath(tape_fname, drive, dir, fname, ext);
		strcat(string, fname);
		strcat(string, ext);
		strcat(string, " ");
	}

	/* 比較 */
	string[127] = '\0';
	if (memcmp(szCaption, string, strlen(string) + 1) != 0) {
		strcpy(szCaption, string);
		return TRUE;
	}

	/* 前回と同じなので、描画しなくてよい */
	return FALSE;
}

/*
 *	ステータスライン(CAP)
 */
static BOOL FASTCALL StatusCAP(void)
{
	HDC hDC;
	HRESULT hResult;
	RECT rect;
	int num;

	/* 値取得、比較 */
	if (caps_flag) {
		num = 1;
	}
	else {
		num = 0;
	}
	if (num == nCAP) {
		return FALSE;
	}

	/* DC取得 */
	hResult = lpdds[1]->GetDC(&hDC);
	if (hResult != DD_OK) {
		/* サーフェイスがロストしていれば、リストア */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[1]->Restore();
		}
		/* 次回は全領域更新 */
		ReDrawDD();
		return FALSE;
	}

	/* -1なら、全領域クリア */
	rect.left = 0;
	rect.right = 640;
	rect.top = 440;
	rect.bottom = 480;
	SetBkColor(hDC, RGB(0, 0, 0));
	SetTextColor(hDC, RGB(255, 255, 255));
	if (nCAP == -1) {
		/* クリア */
		ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);

		/* "CAP"の文字描画 */
		TextOut(hDC, 500, 444, "CAP", 3);

		/* ワクを描画 */
		rect.left = 500;
		rect.right = rect.left + 30;
		rect.top = 465;
		rect.bottom = rect.top + 10;
		FrameRect(hDC, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
	}

	/* ここでコピー */
	nCAP = num;

	/* メイン色描画 */
	rect.left = 501;
	rect.right = rect.left + 28;
	rect.top = 466;
	rect.bottom = rect.top + 8;
	if (nCAP == 1) {
		SetBkColor(hDC, RGB(255, 0, 0));
	}
	else {
		SetBkColor(hDC, RGB(0, 0, 0));
	}
	ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);

	/* DC解放 */
	hResult = lpdds[1]->ReleaseDC(hDC);
	if (hResult != DD_OK) {
		/* サーフェイスがロストしていれば、リストア */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[1]->Restore();
		}
		/* 次回は全領域更新 */
		ReDrawDD();
		return FALSE;
	}

	return TRUE;
}

/*
 *	ステータスライン(かな)
 */
static BOOL FASTCALL StatusKANA(void)
{
	HDC hDC;
	HRESULT hResult;
	RECT rect;
	int num;

	/* 値取得、比較 */
	if (kana_flag) {
		num = 1;
	}
	else {
		num = 0;
	}
	if (num == nKANA) {
		return FALSE;
	}

	/* DC取得 */
	hResult = lpdds[1]->GetDC(&hDC);
	if (hResult != DD_OK) {
		/* サーフェイスがロストしていれば、リストア */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[1]->Restore();
		}
		/* 次回は全領域更新 */
		ReDrawDD();
		return FALSE;
	}

	SetBkColor(hDC, RGB(0, 0, 0));
	SetTextColor(hDC, RGB(255, 255, 255));
	if (nKANA == -1) {
		/* "かな"の文字描画 */
		TextOut(hDC, 546, 444, "かな", 4);

		/* ワクを描画 */
		rect.left = 546;
		rect.right = rect.left + 30;
		rect.top = 465;
		rect.bottom = rect.top + 10;
		FrameRect(hDC, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
	}

	/* ここでコピー */
	nKANA = num;

	/* メイン色描画 */
	rect.left = 547;
	rect.right = rect.left + 28;
	rect.top = 466;
	rect.bottom = rect.top + 8;
	if (nKANA == 1) {
		SetBkColor(hDC, RGB(255, 0, 0));
	}
	else {
		SetBkColor(hDC, RGB(0, 0, 0));
	}
	ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);

	/* DC解放 */
	hResult = lpdds[1]->ReleaseDC(hDC);
	if (hResult != DD_OK) {
		/* サーフェイスがロストしていれば、リストア */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[1]->Restore();
		}
		/* 次回は全領域更新 */
		ReDrawDD();
		return FALSE;
	}

	return TRUE;
}

/*
 *	ステータスライン(INS)
 */
static BOOL FASTCALL StatusINS(void)
{
	HDC hDC;
	HRESULT hResult;
	RECT rect;
	int num;

	/* 値取得、比較 */
	if (ins_flag) {
		num = 1;
	}
	else {
		num = 0;
	}
	if (num == nINS) {
		return FALSE;
	}

	/* DC取得 */
	hResult = lpdds[1]->GetDC(&hDC);
	if (hResult != DD_OK) {
		/* サーフェイスがロストしていれば、リストア */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[1]->Restore();
		}
		/* 次回は全領域更新 */
		ReDrawDD();
		return FALSE;
	}

	SetBkColor(hDC, RGB(0, 0, 0));
	SetTextColor(hDC, RGB(255, 255, 255));
	if (nINS == -1) {
		/* "INS"の文字描画 */
		TextOut(hDC, 593 + 2, 444, "INS", 3);

		/* ワクを描画 */
		rect.left = 593;
		rect.right = rect.left + 30;
		rect.top = 465;
		rect.bottom = rect.top + 10;
		FrameRect(hDC, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
	}

	/* ここでコピー */
	nINS = num;

	/* メイン色描画 */
	rect.left = 594;
	rect.right = rect.left + 28;
	rect.top = 466;
	rect.bottom = rect.top + 8;
	if (nINS == 1) {
		SetBkColor(hDC, RGB(255, 0, 0));
	}
	else {
		SetBkColor(hDC, RGB(0, 0, 0));
	}
	ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);

	/* DC解放 */
	hResult = lpdds[1]->ReleaseDC(hDC);
	if (hResult != DD_OK) {
		/* サーフェイスがロストしていれば、リストア */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[1]->Restore();
		}
		/* 次回は全領域更新 */
		ReDrawDD();
		return FALSE;
	}

	return TRUE;
}

/*
 *	ステータスライン(フロッピードライブ)
 */
static BOOL FASTCALL StatusDrive(int drive)
{
	HDC hDC;
	HRESULT hResult;
	RECT rect;
	int num;
	char *name;

	ASSERT((drive == 0) || (drive == 1));

	/* 番号セット */
	if (fdc_ready[drive] == FDC_TYPE_NOTREADY) {
		num = 255;
	}
	else {
		num = fdc_access[drive];
		if (num == FDC_ACCESS_SEEK) {
			num = FDC_ACCESS_READY;
		}
	}

	/* 名前取得 */
	name = "";
	if (fdc_ready[drive] == FDC_TYPE_D77) {
		name = fdc_name[drive][ fdc_media[drive] ];
	}
	if (fdc_ready[drive] == FDC_TYPE_2D) {
		name = "2D DISK";
	}

	/* 番号比較 */
	if (nDrive[drive] == num) {
		if (fdc_ready[drive] == FDC_TYPE_NOTREADY) {
			return FALSE;
		}
		if (strcmp(szDrive[drive], name) == 0) {
			return FALSE;
		}
	}

	/* DC取得 */
	hResult = lpdds[1]->GetDC(&hDC);
	if (hResult != DD_OK) {
		/* サーフェイスがロストしていれば、リストア */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[1]->Restore();
		}
		/* 次回は全領域更新 */
		ReDrawDD();
		return FALSE;
	}

	/* ここでコピー */
	nDrive[drive] = num;
	strcpy(szDrive[drive], name);

	/* 座標設定 */
	SetBkColor(hDC, RGB(0, 0, 0));
	SetTextColor(hDC, RGB(255, 255, 255));
	rect.left = (drive ^ 1) * 160;
	rect.right = ((drive ^ 1) + 1) * 160 - 4;
	rect.top = 444;
	rect.bottom = 474;

	/* 色決定 */
	if (nDrive[drive] != 255) {
		SetBkColor(hDC, RGB(63, 63, 63));
	}
	if (nDrive[drive] == FDC_ACCESS_READ) {
		SetBkColor(hDC, RGB(191, 0, 0));
	}
	if (nDrive[drive] == FDC_ACCESS_WRITE) {
		SetBkColor(hDC, RGB(0, 0, 191));
	}

	/* 背景を塗りつぶす */
	ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);

	/* DrawText */
	DrawText(hDC, szDrive[drive], strlen(szDrive[drive]), &rect,
		DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

	/* DC解放 */
	hResult = lpdds[1]->ReleaseDC(hDC);
	if (hResult != DD_OK) {
		/* サーフェイスがロストしていれば、リストア */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[1]->Restore();
		}
		/* 次回は全領域更新 */
		ReDrawDD();
		return FALSE;
	}

	return TRUE;
}

/*
 *	ステータスライン(テープ)
 */
static BOOL FASTCALL StatusTape(void)
{
	HDC hDC;
	HRESULT hResult;
	RECT rect;
	int num;
	char string[64];

	/* 番号セット */
	num = 30000;
	if (tape_fileh != -1) {
		num = (int)((tape_offset >> 8) % 10000);
		if (tape_motor) {
			if (tape_rec) {
				num += 20000;
			}
			else {
				num += 10000;
			}
		}
	}

	/* 番号比較 */
	if (nTape == num) {
		return FALSE;
	}

	/* DC取得 */
	hResult = lpdds[1]->GetDC(&hDC);
	if (hResult != DD_OK) {
		/* サーフェイスがロストしていれば、リストア */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[1]->Restore();
		}
		/* 次回は全領域更新 */
		ReDrawDD();
		return FALSE;
	}

	/* ここでコピー */
	nTape = num;

	/* 座標設定 */
	SetBkColor(hDC, RGB(0, 0, 0));
	SetTextColor(hDC, RGB(255, 255, 255));
	rect.left = 360;
	rect.right = rect.left + 80;
	rect.top = 444;
	rect.bottom = 474;

	/* 色、文字列決定 */
	if (nTape >= 30000) {
		string[0] = '\0';
	}
	else {
		sprintf(string, "%04d", nTape % 10000);
		if (nTape >= 10000) {
			if (nTape >= 20000) {
				SetBkColor(hDC, RGB(0, 0, 191));
			}
			else {
				SetBkColor(hDC, RGB(191, 0, 0));
			}
		}
		else {
			SetBkColor(hDC, RGB(63, 63, 63));
		}
	}

	/* 背景を塗りつぶす */
	ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);

	/* DrawText */
	DrawText(hDC, string, strlen(string), &rect,
		DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

	/* DC解放 */
	hResult = lpdds[1]->ReleaseDC(hDC);
	if (hResult != DD_OK) {
		/* サーフェイスがロストしていれば、リストア */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[1]->Restore();
		}
		/* 次回は全領域更新 */
		ReDrawDD();
		return FALSE;
	}

	return TRUE;
}

/*
 *	ステータスライン描画
 */
static void FASTCALL StatusLine(void)
{
	BOOL flag;
	HRESULT hResult;
	RECT rect;

	/* 640x480、ステータスありの場合のみ */
	if (!bDD480Flag || !bDD480Status) {
		return;
	}

	/* キャプション */
	if (StatusCaption()) {
		DrawCaption();
	}

	flag = FALSE;

	/* キーボードステータス */
	if (StatusCAP()) {
		flag = TRUE;
	}
	if (StatusKANA()) {
		flag = TRUE;
	}
	if (StatusINS()) {
		flag = TRUE;
	}
	if (StatusDrive(0)) {
		flag = TRUE;
	}
	if (StatusDrive(1)) {
		flag = TRUE;
	}
	if (StatusTape()) {
		flag = TRUE;
	}

	/* フラグが降りていれば、描画する必要なし */
	if (!flag) {
		return;
	}

	/* Blt */
	rect.left = 0;
	rect.right = 640;
	rect.top = 440;
	rect.bottom = 480;
	hResult = lpdds[0]->Blt(&rect, lpdds[1], &rect, DDBLT_WAIT, NULL);
	if (hResult != DD_OK) {
		/* サーフェイスがロストしていれば、リストア */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[0]->Restore();
			lpdds[1]->Restore();
		}
		/* 次回は全領域更新 */
		ReDrawDD();
		return;
	}
}

/*
 *	640x200、デジタルモード
 *	パレット設定
 */
static void FASTCALL Palet640(void)
{
	int i;
	int vpage;

	/* パレットテーブル */
	static const WORD rgbTable[] = {
		/* nPixelFormat = 1 */
		0x0000 | 0x0000 | 0x0000,
		0x0000 | 0x0000 | 0x001f,
		0xf800 | 0x0000 | 0x0000,
		0xf800 | 0x0000 | 0x001f,
		0x0000 | 0x07e0 | 0x0000,
		0x0000 | 0x07e0 | 0x001f,
		0xf800 | 0x07e0 | 0x0000,
		0xf800 | 0x07e0 | 0x001f,

		/* nPixelFormat = 2 */
		0x0000 | 0x0000 | 0x0000,
		0x0000 | 0x0000 | 0x001f,
		0x7c00 | 0x0000 | 0x0000,
		0x7c00 | 0x0000 | 0x001f,
		0x0000 | 0x03e0 | 0x0000,
		0x0000 | 0x03e0 | 0x001f,
		0x7c00 | 0x03e0 | 0x0000,
		0x7c00 | 0x03e0 | 0x001f
	};

	/* フラグがセットされていなければ、何もしない */
	if (!bPaletFlag) {
		return;
	}

	/* マルチページより、表示プレーン情報を得る */
	vpage = (~(multi_page >> 4)) & 0x07;

	/* 640x200、デジタルパレット */
	for (i=0; i<8; i++) {
		if (crt_flag) {
			/* CRT ON */
			if (nPixelFormat == 1) {
				rgbTTLDD[i] = rgbTable[ttl_palet[i & vpage] + 0];
			}
			if (nPixelFormat == 2) {
				rgbTTLDD[i] = rgbTable[ttl_palet[i & vpage] + 8];
			}
		}
		else {
			/* CRT OFF */
			rgbTTLDD[i] = 0;
		}
	}

	/* フラグ降ろす */
	bPaletFlag = FALSE;
}

/*
 *	640x200、デジタルモード
 *	描画
 */
static void FASTCALL Draw640(void)
{
	DDSURFACEDESC ddsd;
	HRESULT hResult;
	RECT rect;
	BYTE *p;

	/* オールクリア */
	AllClear();

	/* パレット設定 */
	Palet640();

	/* ステータスライン */
	StatusLine();

	/* レンダリングチェック */
	if (nDrawTop >= nDrawBottom) {
		return;
	}

	/* サーフェイスをロック */
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	hResult = lpdds[1]->Lock(NULL, &ddsd,
							 DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);
	if (hResult != DD_OK) {
		/* サーフェイスがロストしていれば、リストア */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[1]->Restore();
		}
		/* 次回は全領域更新 */
		ReDrawDD();
		return;
	}

	/* レンダリング */
	p = (BYTE *)ddsd.lpSurface;
	if (bDD480Flag) {
		p += (ddsd.lPitch * 40);
	}
	Render640DD(p, ddsd.lPitch, nDrawTop, nDrawBottom);
	RenderFullScan(p, ddsd.lPitch);

	/* サーフェイスをアンロック */
	lpdds[1]->Unlock(ddsd.lpSurface);

	/* Blt */
	rect.top = nDrawTop * 2;
	rect.left = 0;
	rect.right = 640;
	rect.bottom = nDrawBottom * 2;
	if (bDD480Flag) {
		rect.top += 40;
		rect.bottom += 40;
	}
	hResult = lpdds[0]->Blt(&rect, lpdds[1], &rect, DDBLT_WAIT, NULL);
	if (hResult != DD_OK) {
		/* サーフェイスがロストしていれば、リストア */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[0]->Restore();
			lpdds[1]->Restore();
		}
		/* 次回は全領域更新 */
		ReDrawDD();
		return;
	}

	/* 次回に備え、ワークリセット */
	nDrawTop = 200;
	nDrawBottom = 0;
}

/*
 *	320x200、アナログモード
 *	パレット設定
 */
static void FASTCALL Palet320(void)
{
	int i, j;
	WORD r, g, b;
	int amask;

	/* フラグがセットされていなければ、何もしない */
	if (!bPaletFlag) {
		return;
	}

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
		if (crt_flag) {
			j = i & amask;
			r = (WORD)apalet_r[j];
			g = (WORD)apalet_g[j];
			b = (WORD)apalet_b[j];
		}
		else {
			r = 0;
			g = 0;
			b = 0;
		}

		/* ピクセルタイプに応じ、WORDデータを作成 */
		if (nPixelFormat == 1) {
			/* R5bit, G6bit, B5bitタイプ */
			r <<= 12;
			if (r > 0) {
				r |= 0x0800;
			}

			g <<= 7;
			if (g > 0) {
				g |= 0x0060;
			}

			b <<= 1;
			if (b > 0) {
				b |= 0x0001;
			}
		}
		if (nPixelFormat == 2) {
			/* R5bit, G5bit, B5bitタイプ */
			r <<= 11;
			if (r > 0) {
				r |= 0x0400;
			}

			g <<= 6;
			if (g > 0) {
				g |= 0x0020;
			}

			b <<= 1;
			if (b > 0) {
				b |= 0x0001;
			}
		}

		/* セット */
		rgbAnalogDD[i] = (WORD)(r | g | b);
	}

	/* フラグ降ろす */
	bPaletFlag = FALSE;
}

/*
 *	320x200、アナログモード
 *	描画
 */
static void FASTCALL Draw320(void)
{
	DDSURFACEDESC ddsd;
	HRESULT hResult;
	RECT rect;
	BYTE *p;

	/* オールクリア */
	AllClear();

	/* パレット設定 */
	Palet320();

	/* ステータスライン */
	StatusLine();

	/* レンダリングチェック */
	if (nDrawTop >= nDrawBottom) {
		return;
	}

	/* サーフェイスをロック */
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	hResult = lpdds[1]->Lock(NULL, &ddsd,
							 DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);
	if (hResult != DD_OK) {
		/* サーフェイスがロストしていれば、リストア */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[1]->Restore();
		}
		/* 次回は全領域更新 */
		ReDrawDD();
		return;
	}

	/* レンダリング */
	p = (BYTE *)ddsd.lpSurface;
	if (bDD480Flag) {
		p += (ddsd.lPitch * 40);
	}
	Render320DD(p, ddsd.lPitch, nDrawTop, nDrawBottom);
	RenderFullScan(p, ddsd.lPitch);

	/* サーフェイスをアンロック */
	lpdds[1]->Unlock(ddsd.lpSurface);

	/* Blt */
	rect.top = nDrawTop * 2;
	rect.left = 0;
	rect.right = 640;
	rect.bottom = nDrawBottom * 2;
	if (bDD480Flag) {
		rect.top += 40;
		rect.bottom += 40;
	}
	hResult = lpdds[0]->Blt(&rect, lpdds[1], &rect, DDBLT_WAIT, NULL);
	if (hResult != DD_OK) {
		/* サーフェイスがロストしていれば、リストア */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[0]->Restore();
			lpdds[1]->Restore();
		}
		/* 次回は全領域更新 */
		ReDrawDD();
		return;
	}

	/* 次回に備え、ワークリセット */
	nDrawTop = 200;
	nDrawBottom = 0;
}

/*
 *	描画
 */
void FASTCALL DrawDD(void)
{

	/* デジタル・アナログ判定 */
	if (mode320) {
		if (!bAnalog) {
			ReDrawDD();
			bAnalog = TRUE;
		}
	}
	else {
		if (bAnalog) {
			ReDrawDD();
			bAnalog = FALSE;
		}
	}

	/* どちらかを使って描画 */
	if (bAnalog) {
		Draw320();
	}
	else {
		Draw640();
	}
}

/*
 *	メニュー開始
 */
void FASTCALL EnterMenuDD(HWND hWnd)
{
	ASSERT(hWnd);

	/* クリッパーの有無で判定できる */
	if (!lpddc) {
		return;
	}

	/* クリッパーセット */
	LockVM();
	lpdds[0]->SetClipper(lpddc);
	UnlockVM();

	/* マウスカーソルon */
	if (!bMouseCursor) {
		ShowCursor(TRUE);
		bMouseCursor = TRUE;
	}

	/* メニューバーを描画 */
	DrawMenuBar(hWnd);
}

/*
 *	メニュー終了
 */
void FASTCALL ExitMenuDD(void)
{
	/* クリッパーの有無で判定できる */
	if (!lpddc) {
		return;
	}

	/* クリッパー解除 */
	LockVM();
	lpdds[0]->SetClipper(NULL);
	UnlockVM();

	/* マウスカーソルOFF */
	if (bMouseCursor) {
		ShowCursor(FALSE);
		bMouseCursor = FALSE;
	}

	/* 再表示 */
	ReDrawDD();
}

/*-[ VMとの接続 ]-----------------------------------------------------------*/

/*
 *	VRAMセット
 */
void FASTCALL VramDD(WORD addr)
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
void FASTCALL DigitalDD(void)
{
	nDrawTop = 0;
	nDrawBottom = 200;
	bPaletFlag = TRUE;
}

/*
 *	アナログパレットセット
 */
void FASTCALL AnalogDD(void)
{
	nDrawTop = 0;
	nDrawBottom = 200;
	bPaletFlag = TRUE;
}

/*
 *	再描画要求
 */
void FASTCALL ReDrawDD(void)
{
	/* 全領域レンダリング */
	nDrawTop = 0;
	nDrawBottom = 200;
	bPaletFlag = TRUE;
	bClearFlag = TRUE;
}

#endif	/* _WIN32 */
