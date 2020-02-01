/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API コントロールバー ]
 */

#ifdef _WIN32

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <assert.h>
#include "xm7.h"
#include "keyboard.h"
#include "tapelp.h"
#include "display.h"
#include "ttlpalet.h"
#include "apalet.h"
#include "subctrl.h"
#include "fdc.h"
#include "w32.h"
#include "w32_bar.h"
#include "w32_draw.h"
#include "w32_res.h"

/*
 *	グローバル ワーク
 */
HWND hStatusBar;						/* ステータスバー */

/*
 *	スタティック ワーク
 */
static char szIdleMessage[128];			/* アイドルメッセージ */
static char szRunMessage[128];			/* RUNメッセージ */
static char szStopMessage[128];			/* STOPメッセージ */
static char szCaption[128];				/* キャプション */
static int nCAP;						/* CAPキー */
static int nKANA;						/* かなキー */
static int nINS;						/* INSキー */
static int nDrive[2];					/* フロッピードライブ */
static char szDrive[2][16 + 1];			/* フロッピードライブ */
static int nTape;						/* テープ */

/*-[ ステータスバー ]-------------------------------------------------------*/

/*
 *	アクセスマクロ
 */
#define Status_SetParts(hwnd, nParts, aWidths) \
	SendMessage((hwnd), SB_SETPARTS, (WPARAM) nParts, (LPARAM) (LPINT) aWidths)

#define Status_SetText(hwnd, iPart, uType, szText) \
	SendMessage((hwnd), SB_SETTEXT, (WPARAM) (iPart | uType), (LPARAM) (LPSTR) szText)

/*
 *	ペイン定義
 */
#define PANE_DEFAULT	0
#define PANE_DRIVE1		1
#define PANE_DRIVE0		2
#define PANE_TAPE		3
#define PANE_CAP		4
#define PANE_KANA		5
#define PANE_INS		6

/*
 *	ステータスバー作成
 */
HWND FASTCALL CreateStatus(HWND hParent)
{
	HWND hWnd;

	ASSERT(hParent);

	/* メッセージをロード */
	if (LoadString(hAppInstance, IDS_IDLEMESSAGE,
					szIdleMessage, sizeof(szIdleMessage)) == 0) {
		szIdleMessage[0] = '\0';
	}
	if (LoadString(hAppInstance, IDS_RUNCAPTION,
					szRunMessage, sizeof(szRunMessage)) == 0) {
		szRunMessage[0] = '\0';
	}
	if (LoadString(hAppInstance, IDS_STOPCAPTION,
					szStopMessage, sizeof(szStopMessage)) == 0) {
		szRunMessage[0] = '\0';
	}

	/* ステータスバーを作成 */
	hWnd = CreateStatusWindow(WS_CHILD | WS_VISIBLE | CCS_BOTTOM,
								szIdleMessage, hParent, ID_STATUS_BAR);

	return hWnd;
}

/*
 *	キャプション描画
 */
static void FASTCALL DrawMainCaption(void)
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

	/* 比較描画 */
	string[127] = '\0';
	if (memcmp(szCaption, string, strlen(string) + 1) != 0) {
		strcpy(szCaption, string);
		SetWindowText(hMainWnd, szCaption);
	}
}

/*
 *	CAPキー描画
 */
static void FASTCALL DrawCAP(void)
{
	int num;

	/* 番号決定 */
	if (caps_flag) {
		num = 1;
	}
	else {
		num = 0;
	}

	/* 同じなら何もしない */
	if (nCAP == num) {
		return;
	}

	/* 描画、ワーク更新 */
	nCAP = num;
	Status_SetText(hStatusBar, PANE_CAP, SBT_OWNERDRAW, PANE_CAP);
}

/*
 *	かなキー描画
 */
static void FASTCALL DrawKANA(void)
{
	int num;

	/* 番号決定 */
	if (kana_flag) {
		num = 1;
	}
	else {
		num = 0;
	}

	/* 同じなら何もしない */
	if (nKANA == num) {
		return;
	}

	/* 描画、ワーク更新 */
	nKANA = num;
	Status_SetText(hStatusBar, PANE_KANA, SBT_OWNERDRAW, PANE_KANA);
}

/*
 *	INSキー描画
 */
static void FASTCALL DrawINS(void)
{
	int num;

	/* 番号決定 */
	if (ins_flag) {
		num = 1;
	}
	else {
		num = 0;
	}

	/* 同じなら何もしない */
	if (nINS == num) {
		return;
	}

	/* 描画、ワーク更新 */
	nINS = num;
	Status_SetText(hStatusBar, PANE_INS, SBT_OWNERDRAW, PANE_INS);
}

/*
 *	ドライブ描画
 */
static void FASTCALL DrawDrive(int drive, UINT nID)
{
	int num;
	char *name;

	ASSERT((drive >= 0) && (drive <= 1));

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
			return;
		}
		if (strcmp(szDrive[drive], name) == 0) {
			return;
		}
	}

	/* 描画 */
	nDrive[drive] = num;
	strcpy(szDrive[drive], name);
	Status_SetText(hStatusBar, nID, SBT_OWNERDRAW, nID);
}

/*
 *	テープ描画
 */
static void FASTCALL DrawTape(void)
{
	int num;

	/* ナンバー計算 */
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
		return;
	}

	/* 描画 */
	nTape = num;
	Status_SetText(hStatusBar, PANE_TAPE, SBT_OWNERDRAW, PANE_TAPE);
}

/*
 *	描画
 */
void FASTCALL DrawStatus(void)
{
	/* ウインドウチェック */
	if (!hMainWnd) {
		return;
	}

	DrawMainCaption();

	/* 全画面、ステータスバーチェック */
	if (bFullScreen || !hStatusBar) {
		return;
	}

	DrawCAP();
	DrawKANA();
	DrawINS();
	DrawDrive(0, PANE_DRIVE0);
	DrawDrive(1, PANE_DRIVE1);
	DrawTape();
}

/*
 *	再描画
 */
void FASTCALL PaintStatus(void)
{
	/* 記憶ワークをすべてクリアする */
	szCaption[0] = '\0';
	nCAP = -1;
	nKANA = -1;
	nINS = -1;
	nDrive[0] = -1;
	nDrive[1] = -1;
	szDrive[0][0] = '\0';
	szDrive[1][0] = '\0';
	nTape = -1;

	/* 描画 */
	DrawStatus();
}

/*
 *	オーナードロー
 */
void FASTCALL OwnerDrawStatus(DRAWITEMSTRUCT *pDI)
{
	HBRUSH hBrush;
	COLORREF fColor;
	COLORREF bColor;
	char string[128];
	int i;

	ASSERT(pDI);

	/* 文字列、色を決定 */
	switch (pDI->itemData) {
		/* フロッピーディスク */
		case PANE_DRIVE1:
		case PANE_DRIVE0:
			if (pDI->itemData == PANE_DRIVE0) {
				i = 0;
			}
			else {
				i = 1;
			}
			if (nDrive[i] == 255) {
				strcpy(string, "");
			}
			else {
				strcpy(string, szDrive[i]);
			}
			fColor = GetTextColor(pDI->hDC);
			bColor = GetSysColor(COLOR_3DFACE);
			if (nDrive[i] == FDC_ACCESS_READ) {
				fColor = RGB(255, 255, 255);
				bColor = RGB(191, 0, 0);
			}
			if (nDrive[i] == FDC_ACCESS_WRITE) {
				fColor = RGB(255, 255, 255);
				bColor = RGB(0, 0, 191);
			}
			break;

		/* テープ */
		case PANE_TAPE:
			if (nTape >= 30000) {
				string[0] = '\0';
			}
			else {
				sprintf(string, "%04d", nTape % 10000);
			}
			fColor = GetTextColor(pDI->hDC);
			bColor = GetSysColor(COLOR_3DFACE);
			if ((nTape >= 10000) && (nTape < 30000)) {
				if (nTape >= 20000) {
					fColor = RGB(255, 255, 255);
					bColor = RGB(0, 0, 191);
				}
				else {
					fColor = RGB(255, 255, 255);
					bColor = RGB(191, 0, 0);
				}
			}
			break;

		/* CAP */
		case PANE_CAP:
			strcpy(string, "CAP");
			if (nCAP) {
				fColor = RGB(255, 255, 255);
				bColor = RGB(255, 0, 0);
			}
			else {
				fColor = RGB(255, 255, 255);
				bColor = RGB(0, 0, 0);
			}
			break;

		/* かな */
		case PANE_KANA:
			strcpy(string, "かな");
			if (nKANA) {
				fColor = RGB(255, 255, 255);
				bColor = RGB(255, 0, 0);
			}
			else {
				fColor = RGB(255, 255, 255);
				bColor = RGB(0, 0, 0);
			}
			break;

		/* INS */
		case PANE_INS:
			strcpy(string, "INS");
			if (nINS) {
				fColor = RGB(255, 255, 255);
				bColor = RGB(255, 0, 0);
			}
			else {
				fColor = RGB(255, 255, 255);
				bColor = RGB(0, 0, 0);
			}
			break;

		/* それ以外 */
		default:
			ASSERT(FALSE);
			return;
	}

	/* ブラシで塗る */
	hBrush = CreateSolidBrush(bColor);
	if (hBrush) {
		FillRect(pDI->hDC, &(pDI->rcItem), hBrush);
		DeleteObject(hBrush);
	}

	/* テキストを描画 */
	SetTextColor(pDI->hDC, fColor);
	SetBkColor(pDI->hDC, bColor);
	DrawText(pDI->hDC, string, strlen(string), &(pDI->rcItem),
				DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}

/*
 *	サイズ変更
 */
void FASTCALL SizeStatus(LONG cx)
{
	HDC hDC;
	TEXTMETRIC tm;
	LONG cw;
	UINT uPane[7];

	ASSERT(cx > 0);
	ASSERT(hStatusBar);

	/* テキストメトリックを取得 */
	hDC = GetDC(hStatusBar);
	SelectObject(hDC, GetStockObject(SYSTEM_FONT));
	GetTextMetrics(hDC, &tm);
	ReleaseDC(hStatusBar, hDC);
	cw = tm.tmAveCharWidth;

	/* ペインサイズを決定(ペイン右端の位置を設定) */
	uPane[PANE_INS] = cx;
	uPane[PANE_KANA] = uPane[PANE_INS] - cw * 4;
	uPane[PANE_CAP] = uPane[PANE_KANA] - cw * 4;
	uPane[PANE_TAPE] = uPane[PANE_CAP] - cw * 4;
	uPane[PANE_DRIVE0] = uPane[PANE_TAPE] - cw * 5;
	uPane[PANE_DRIVE1] = uPane[PANE_DRIVE0] - cw * 16;
	uPane[PANE_DEFAULT] = uPane[PANE_DRIVE1] - cw * 16;

	/* ペインサイズ設定 */
	Status_SetParts(hStatusBar, sizeof(uPane)/sizeof(UINT), uPane);

	/* 再描画 */
	PaintStatus();
}

/*
 *	メニューセレクト
 */
void FASTCALL OnMenuSelect(WPARAM wParam)
{
	char buffer[128];
	UINT uID;

	ASSERT(hStatusBar);

	/* ステータスバーが表示されていなければ、何もしない */
	if (!IsWindowVisible(hStatusBar)) {
		return;
	}

	/* 同一IDの文字列リソースロードを試みる */
	uID = (UINT)LOWORD(wParam);
	if (LoadString(hAppInstance, uID, buffer, sizeof(buffer)) == 0) {
		buffer[0] = '\0';
	}

	/* セット */
	Status_SetText(hStatusBar, 0, 0, buffer);
}

/*
 *	メニュー終了
 */
void FASTCALL OnExitMenuLoop(void)
{
	ASSERT(hStatusBar);

	/* ステータスバーが表示されていなければ、何もしない */
	if (!IsWindowVisible(hStatusBar)) {
		return;
	}

	/* アイドルメッセージを表示 */
	Status_SetText(hStatusBar, 0, 0, szIdleMessage);
}

#endif	/* _WIN32 */

