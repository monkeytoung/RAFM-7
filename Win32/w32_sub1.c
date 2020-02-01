/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API サブウィンドウ１ ]
 */

#ifdef _WIN32

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <assert.h>
#include <stdlib.h>
#include "xm7.h"
#include "event.h"
#include "w32.h"
#include "w32_res.h"
#include "w32_sub.h"

/*
 *	グローバル ワーク
 */
HWND hSubWnd[SWND_MAXNUM];				/* サブウインドウ */

/*
 *	スタティック ワーク
 */
static WORD AddrHistory[16];			/* アドレスヒストリ */
static WORD AddrBuf;					/* アドレスバッファ */
static int AddrNum;						/* アドレスヒストリ数 */
static BYTE *pBreakPoint;				/* ブレークポイント Drawバッファ */
static HMENU hBreakPoint;				/* ブレークポイント メニューハンドル */
static POINT PosBreakPoint;				/* ブレークポイント マウス位置 */
static BYTE *pScheduler;				/* スケジューラ Drawバッファ */
static BYTE *pCPURegisterMain;			/* CPUレジスタ Drawバッファ */
static BYTE *pCPURegisterSub;			/* CPUレジスタ Drawバッファ */
static BYTE *pDisAsmMain;				/* 逆アセンブル Drawバッファ */
static BYTE *pDisAsmSub;				/* 逆アセンブル Drawバッファ */
static WORD wDisAsmMain;				/* 逆アセンブル アドレス */
static WORD wDisAsmSub;					/* 逆アセンブル アドレス */
static HMENU hDisAsmMain;				/* 逆アセンブル メニューハンドル */
static HMENU hDisAsmSub;				/* 逆アセンブル メニューハンドル */
static BYTE *pMemoryMain;				/* メモリダンプ Drawバッファ */
static BYTE *pMemorySub;				/* メモリダンプ Drawバッファ */
static WORD wMemoryMain;				/* メモリダンプ アドレス */
static WORD wMemorySub;					/* メモリダンプ アドレス */
static HMENU hMemoryMain;				/* メモリダンプ メニューハンドル */
static HMENU hMemorySub;				/* メモリダンプ メニューハンドル */

/*-[ アドレス入力ダイアログ ]------------------------------------------------*/

/*
 *	アドレス入力ダイアログ
 *	ダイアログ初期化
 */
static BOOL FASTCALL AddrDlgInit(HWND hDlg)
{
	HWND hWnd;
	RECT prect;
	RECT drect;
	int i;
	char string[128];

	ASSERT(hDlg);

	/* 親ウインドウの中央に設定 */
	hWnd = GetParent(hDlg);
	GetWindowRect(hWnd, &prect);
	GetWindowRect(hDlg, &drect);
	drect.right -= drect.left;
	drect.bottom -= drect.top;
	drect.left = (prect.right - prect.left) / 2 + prect.left;
	drect.left -= (drect.right / 2);
	drect.top = (prect.bottom - prect.top) / 2 + prect.top;
	drect.top -= (drect.bottom / 2);
	MoveWindow(hDlg, drect.left, drect.top, drect.right, drect.bottom, FALSE);

	/* コンボボックス処理 */
	hWnd = GetDlgItem(hDlg, IDC_ADDRCOMBO);
	ASSERT(hWnd);

	/* ヒストリを挿入 */
	ComboBox_ResetContent(hWnd);
	for (i=AddrNum; i>0; i--) {
		sprintf(string, "%04X", AddrHistory[i - 1]);
		ComboBox_AddString(hWnd, string);
	}

	/* アドレスを設定 */
	sprintf(string, "%04X", AddrBuf);
	ComboBox_SetText(hWnd, string);

	return TRUE;
}

/*
 *	アドレス入力ダイアログ
 *	ダイアログOK
 */
static void FASTCALL AddrDlgOK(HWND hDlg)
{
	HWND hWnd;
	char string[128];
	int i;

	ASSERT(hDlg);

	/* コンボボックス処理 */
	hWnd = GetDlgItem(hDlg, IDC_ADDRCOMBO);
	ASSERT(hWnd);

	/* 現在の値を取得 */
	ComboBox_GetText(hWnd, string, sizeof(string) - 1);
	AddrBuf = (WORD)strtol(string, NULL, 16);

	/* ヒストリをシフト、挿入、カウントアップ */
	for (i=14; i>=0; i--) {
		AddrHistory[i + 1] = AddrHistory[i];
	}
	AddrHistory[0] = AddrBuf;
	if (AddrNum < 16) {
		AddrNum++;
	}
}

/*
 *	アドレス入力ダイアログ
 *	ダイアログプロシージャ
 */
static BOOL CALLBACK AddrDlgProc(HWND hDlg, UINT iMsg,
									WPARAM wParam, LPARAM lParam)
{
	switch (iMsg) {
		/* ダイアログ初期化 */
		case WM_INITDIALOG:
			return AddrDlgInit(hDlg);

		/* コマンド処理 */
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				/* OK */
				case IDOK:
					AddrDlgOK(hDlg);
					EndDialog(hDlg, IDOK);
					return TRUE;

				/* キャンセル */
				case IDCANCEL:
					EndDialog(hDlg, IDCANCEL);
					return TRUE;
			}
			break;
	}

	/* それ以外は、FALSE */
	return FALSE;
}

/*
 *	アドレス入力
 */
static BOOL FASTCALL AddrDlg(HWND hWnd, WORD *pAddr)
{
	int ret;

	ASSERT(hWnd);
	ASSERT(pAddr);

	/* アドレスをセット */
	AddrBuf = *pAddr;

	/* モーダルダイアログ実行 */
	ret = DialogBox(hAppInstance, MAKEINTRESOURCE(IDD_ADDRDLG), hWnd, AddrDlgProc);
	if (ret != IDOK) {
		return FALSE;
	}

	/* アドレスをセットし、帰る */
	*pAddr = AddrBuf;
	return TRUE;
}

/*-[ ブレークポイント ]------------------------------------------------------*/

/*
 *	ブレークポイントウインドウ
 *	セットアップ
 */
static void FASTCALL SetupBreakPoint(BYTE *p, int x, int y)
{
	int i;
	char string[128];

	ASSERT(p);
	ASSERT(x > 0);
	ASSERT(y > 0);

	/* 一旦スペースで埋める */
	memset(p, 0x20, x * y);

	/* ブレークポイントループ */
	for (i=0; i<BREAKP_MAXNUM; i++) {
		/* 文字列作成 */
		string[0] = '\0';
		if (breakp[i].flag == BREAKP_NOTUSE) {
			sprintf(string, "%1d ------------------", i + 1);
		}
		else {
			if (breakp[i].cpu == MAINCPU) {
				sprintf(string, "%1d Main %04X ", i + 1, breakp[i].addr);
			}
			else {
				sprintf(string, "%1d Sub  %04X ", i + 1, breakp[i].addr);
			}
			switch (breakp[i].flag) {
				case BREAKP_ENABLED:
					strcat(string, " Enabled");
					break;
				case BREAKP_DISABLED:
					strcat(string, "Disabled");
					break;
				case BREAKP_STOPPED:
					strcat(string, " Stopped");
					break;
			}
		}

		/* コピー */
		memcpy(&p[x * i], string, strlen(string));
	}
}

/*
 *	ブレークポイントウインドウ
 *	描画
 */
static void FASTCALL DrawBreakPoint(HWND hWnd, HDC hDC)
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
	p = pBreakPoint;
	if (!p) {
		return;
	}
	SetupBreakPoint(p, x, y);

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
 *	ブレークポイントウインドウ
 *	リフレッシュ
 */
void FASTCALL RefreshBreakPoint(void)
{
	HWND hWnd;
	HDC hDC;

	/* 常に呼ばれるので、存在チェックすること */
	if (hSubWnd[SWND_BREAKPOINT] == NULL) {
		return;
	}

	/* 描画 */
	hWnd = hSubWnd[SWND_BREAKPOINT];
	hDC = GetDC(hWnd);
	DrawBreakPoint(hWnd, hDC);
	ReleaseDC(hWnd, hDC);
}

/*
 *	ブレークポイントウインドウ
 *	再描画
 */
static void FASTCALL PaintBreakPoint(HWND hWnd)
{
	HDC hDC;
	PAINTSTRUCT ps;
	RECT rect;
	BYTE *p;
	int x, y;

	ASSERT(hWnd);

	/* ポインタを設定 */
	p = pBreakPoint;

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
	DrawBreakPoint(hWnd, hDC);
	EndPaint(hWnd, &ps);
}

/*
 *	ブレークポイント
 *	コマンド
 */
static void FASTCALL CmdBreakPoint(HWND hWnd, WORD wID)
{
	int num;
	POINT point;

	ASSERT(hWnd);

	/* インデックス番号取得 */
	point = PosBreakPoint;
	num = point.y / lCharHeight;
	if ((num < 0) || (num >= BREAKP_MAXNUM)) {
		return;
	}

	/* コマンド別 */
	switch (wID) {
		/* ジャンプ */
		case IDM_BREAKP_JUMP:
			if (breakp[num].flag != BREAKP_NOTUSE) {
				if (breakp[num].cpu == MAINCPU) {
					AddrDisAsm(TRUE, breakp[num].addr);
				}
				else {
					AddrDisAsm(FALSE, breakp[num].addr);
				}
			}
			break;

		/* イネーブル */
		case IDM_BREAKP_ENABLE:
			if (breakp[num].flag == BREAKP_DISABLED) {
				breakp[num].flag = BREAKP_ENABLED;
				InvalidateRect(hWnd, NULL, FALSE);
				RefreshDisAsm();
			}
			break;

		/* ディセーブル */
		case IDM_BREAKP_DISABLE:
			if ((breakp[num].flag == BREAKP_ENABLED) || (breakp[num].flag == BREAKP_STOPPED)) {
				breakp[num].flag = BREAKP_DISABLED;
				InvalidateRect(hWnd, NULL, FALSE);
				RefreshDisAsm();
			}
			break;

		/* クリア */
		case IDM_BREAKP_CLEAR:
			breakp[num].flag = BREAKP_NOTUSE;
			InvalidateRect(hWnd, NULL, FALSE);
			RefreshDisAsm();
			break;

		/* 全てクリア */
		case IDM_BREAKP_ALL:
			for (num=0; num<BREAKP_MAXNUM; num++) {
				breakp[num].flag = BREAKP_NOTUSE;
			}
			InvalidateRect(hWnd, NULL, FALSE);
			RefreshDisAsm();
			break;
	}
}

/*
 *	ブレークポイントウインドウ
 *	ウインドウプロシージャ
 */
static LRESULT CALLBACK BreakPointProc(HWND hWnd, UINT message,
								 WPARAM wParam, LPARAM lParam)
{
	POINT point;
	int i;
	HMENU hMenu;

	/* メッセージ別 */
	switch (message) {
		/* ウインドウ再描画 */
		case WM_PAINT:
			/* ロックが必要 */
			LockVM();
			PaintBreakPoint(hWnd);
			UnlockVM();
			return 0;

		/* 左クリック */
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
			/* カーソル位置から、決定 */
			i = HIWORD(lParam) / lCharHeight;
			if ((i >= 0) && (i < BREAKP_MAXNUM)) {
				if (breakp[i].flag != BREAKP_NOTUSE) {
					if (breakp[i].cpu == MAINCPU) {
						AddrDisAsm(TRUE, breakp[i].addr);
					}
					else {
						AddrDisAsm(FALSE, breakp[i].addr);
					}
				}
			}
			return 0;

		/* コンテキストメニュー */
		case WM_RBUTTONDOWN:
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);
			PosBreakPoint = point;
			hMenu = GetSubMenu(hBreakPoint, 0);
			ClientToScreen(hWnd, &point);
			TrackPopupMenu(hMenu, 0, point.x, point.y, 0, hWnd, NULL);
			return 0;

		/* ウインドウ削除 */
		case WM_DESTROY:
			/* メニュー削除 */
			DestroyMenu(hBreakPoint);

			/* メインウインドウへ自動通知 */
			for (i=0; i<SWND_MAXNUM; i++) {
				if (hSubWnd[i] == hWnd) {
					hSubWnd[i] = NULL;
					/* Drawバッファ削除 */
					free(pBreakPoint);
					pBreakPoint = NULL;
				}
			}
			break;

		/* コマンド */
		case WM_COMMAND:
			CmdBreakPoint(hWnd, LOWORD(wParam));
			break;
	}

	/* デフォルト ウインドウプロシージャ */
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/*
 *	ブレークポイントウインドウ
 *	ウインドウ作成
 */
HWND FASTCALL CreateBreakPoint(HWND hParent, int index)
{
	WNDCLASSEX wcex;
	char szClassName[] = "XM7_BreakPoint";
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
	rect.right = lCharWidth * 20;
	rect.bottom = lCharHeight * BREAKP_MAXNUM;

	/* ウインドウタイトルを決定、バッファ確保 */
	LoadString(hAppInstance, IDS_SWND_BREAKPOINT,
				szWndName, sizeof(szWndName));
	pBreakPoint = malloc(2 * (rect.right / lCharWidth) *
								(rect.bottom / lCharHeight));

	/* メニューをロード */
	hBreakPoint = LoadMenu(hAppInstance, MAKEINTRESOURCE(IDR_BREAKPOINTMENU));

	/* ウインドウクラスの登録 */
	memset(&wcex, 0, sizeof(wcex));
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_VREDRAW | CS_HREDRAW;
	wcex.lpfnWndProc = BreakPointProc;
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

/*-[ スケジューラウインドウ ]------------------------------------------------*/

static char *pszSchedulerTitle[] = {
	"Main Timer",
	"Sub  Timer",
	"OPN Timer-A",
	"OPN Timer-B",
	"Key Repeat",
	"BEEP",
	"V-SYNC",
	"V/H-BLANK",
	"Line LSI",
	"Clock",
	"WHG Timer-A",
	"WHG Timer-B",
	"(Reserved)",
	"(Reserved)",
	"(Reserved)",
	"(Reserved)"
};

/*
 *	スケジューラウインドウ
 *	セットアップ
 */
static void FASTCALL SetupScheduler(BYTE *p, int x, int y)
{
	int i;
	char string[128];

	ASSERT(p);
	ASSERT(x > 0);
	ASSERT(y > 0);

	/* 一旦スペースで埋める */
	memset(p, 0x20, x * y);

	/* ループ */
	for (i=0; i<EVENT_MAXNUM; i++) {
		/* タイトル */
		memcpy(&p[x * i], pszSchedulerTitle[i], strlen(pszSchedulerTitle[i]));

		/* カレント、リロード */
		if (event[i].flag != EVENT_NOTUSE) {
			sprintf(string, "%4d.%03dms",   event[i].current / 1000,
											event[i].current % 1000);
			memcpy(&p[x * i + 12], string, strlen(string));

			sprintf(string, "(%4d.%03dms)", event[i].reload / 1000,
											event[i].reload % 1000);
			memcpy(&p[x * i + 23], string, strlen(string));
		}

		/* ステータス */
		switch (event[i].flag) {
			case EVENT_NOTUSE:
				strcpy(string, "");
				break;

			case EVENT_ENABLED:
				strcpy(string, " Enabled");
				break;

			case EVENT_DISABLED:
				strcpy(string, "Disabled");
				break;

			default:
				ASSERT(FALSE);
				break;
		}
		memcpy(&p[x * i + 36], string, strlen(string));
	}
}

/*
 *	スケジューラウインドウ
 *	描画
 */
static void FASTCALL DrawScheduler(HWND hWnd, HDC hDC)
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
	p = pScheduler;
	if (!p) {
		return;
	}
	SetupScheduler(p, x, y);

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
 *	スケジューラウインドウ
 *	リフレッシュ
 */
void FASTCALL RefreshScheduler(void)
{
	HWND hWnd;
	HDC hDC;

	/* 常に呼ばれるので、存在チェックすること */
	if (hSubWnd[SWND_SCHEDULER] == NULL) {
		return;
	}

	/* 描画 */
	hWnd = hSubWnd[SWND_SCHEDULER];
	hDC = GetDC(hWnd);
	DrawScheduler(hWnd, hDC);
	ReleaseDC(hWnd, hDC);
}

/*
 *	スケジューラウインドウ
 *	再描画
 */
static void FASTCALL PaintScheduler(HWND hWnd)
{
	HDC hDC;
	PAINTSTRUCT ps;
	RECT rect;
	BYTE *p;
	int x, y;

	ASSERT(hWnd);

	/* ポインタを設定 */
	p = pScheduler;

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
	DrawScheduler(hWnd, hDC);
	EndPaint(hWnd, &ps);
}

/*
 *	スケジューラウインドウ
 *	ウインドウプロシージャ
 */
static LRESULT CALLBACK SchedulerProc(HWND hWnd, UINT message,
								 WPARAM wParam, LPARAM lParam)
{
	int i;

	/* メッセージ別 */
	switch (message) {
		/* ウインドウ再描画 */
		case WM_PAINT:
			/* ロックが必要 */
			LockVM();
			PaintScheduler(hWnd);
			UnlockVM();
			return 0;

		/* ウインドウ削除 */
		case WM_DESTROY:
			/* メインウインドウへ自動通知 */
			for (i=0; i<SWND_MAXNUM; i++) {
				if (hSubWnd[i] == hWnd) {
					hSubWnd[i] = NULL;
					/* Drawバッファ削除 */
					free(pScheduler);
					pScheduler = NULL;
				}
			}
			break;
	}

	/* デフォルト ウインドウプロシージャ */
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/*
 *	スケジューラウインドウ
 *	ウインドウ作成
 */
HWND FASTCALL CreateScheduler(HWND hParent, int index)
{
	WNDCLASSEX wcex;
	char szClassName[] = "XM7_Scheduler";
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
	rect.right = lCharWidth * 44;
	rect.bottom = lCharHeight * EVENT_MAXNUM;

	/* ウインドウタイトルを決定、バッファ確保 */
	LoadString(hAppInstance, IDS_SWND_SCHEDULER,
				szWndName, sizeof(szWndName));
	pScheduler = malloc(2 * (rect.right / lCharWidth) *
								(rect.bottom / lCharHeight));

	/* ウインドウクラスの登録 */
	memset(&wcex, 0, sizeof(wcex));
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_VREDRAW | CS_HREDRAW;
	wcex.lpfnWndProc = SchedulerProc;
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

/*-[ CPUレジスタウインドウ ]-------------------------------------------------*/

/*
 *	CPUレジスタウインドウ
 *	セットアップ
 */
static void FASTCALL SetupCPURegister(BOOL bMain, BYTE *p, int x, int y)
{
	char buf[128];
	cpu6809_t *pReg;

	ASSERT((bMain == TRUE) || (bMain == FALSE));
	ASSERT(p);
	ASSERT(x > 0);
	ASSERT(y > 0);

	/* 一旦スペースで埋める */
	memset(p, 0x20, x * y);

	/* レジスタバッファを得る */
	if (bMain) {
		pReg = &maincpu;
	}
	else {
		pReg = &subcpu;
	}

	/* セット */
	sprintf(buf, "CC   %02X", pReg->cc);
	memcpy(&p[0 * x + 0], buf, strlen(buf));
	sprintf(buf, "A    %02X", pReg->acc.h.a);
	memcpy(&p[1 * x + 0], buf, strlen(buf));
	sprintf(buf, "B    %02X", pReg->acc.h.b);
	memcpy(&p[2 * x + 0], buf, strlen(buf));
	sprintf(buf, "DP   %02X", pReg->dp);
	memcpy(&p[3 * x + 0], buf, strlen(buf));
	sprintf(buf, "IR %04X", pReg->intr);
	memcpy(&p[4 * x + 0], buf, strlen(buf));

	sprintf(buf, "X  %04X", pReg->x);
	memcpy(&p[0 * x + 10], buf, strlen(buf));
	sprintf(buf, "Y  %04X", pReg->y);
	memcpy(&p[1 * x + 10], buf, strlen(buf));
	sprintf(buf, "U  %04X", pReg->u);
	memcpy(&p[2 * x + 10], buf, strlen(buf));
	sprintf(buf, "S  %04X", pReg->s);
	memcpy(&p[3 * x + 10], buf, strlen(buf));
	sprintf(buf, "PC %04X", pReg->pc);
	memcpy(&p[4 * x + 10], buf, strlen(buf));
}

/*
 *	CPUレジスタウインドウ
 *	描画
 */
static void FASTCALL DrawCPURegister(HWND hWnd, HDC hDC)
{
	int i;
	BOOL bMain;
	RECT rect;
	BYTE *p, *q;
	int x, y;
	int xx, yy;
	HFONT hFont;
	HFONT hBackup;

	ASSERT(hWnd);
	ASSERT(hDC);

	/* Drawバッファを得る */
	for (i=0; i<SWND_MAXNUM; i++) {
		if (hSubWnd[i] == hWnd) {
			if ((i & 1) == 0) {
				p = pCPURegisterMain;
				bMain = TRUE;
			}
			else {
				p = pCPURegisterSub;
				bMain = FALSE;
			}
			break;
		}
	}
	if (!p) {
		return;
	}

	/* ウインドウジオメトリを得る */
	GetClientRect(hWnd, &rect);
	x = rect.right / lCharWidth;
	y = rect.bottom / lCharHeight;
	if ((x == 0) || (y == 0)) {
		return;
	}

	/* セットアップ */
	SetupCPURegister(bMain, p, x, y);

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
 *	CPUレジスタウインドウ
 *	リフレッシュ
 */
void FASTCALL RefreshCPURegister(void)
{
	HWND hWnd;
	HDC hDC;

	/* メインCPU */
	if (hSubWnd[SWND_CPUREG_MAIN]) {
		hWnd = hSubWnd[SWND_CPUREG_MAIN];
		hDC = GetDC(hWnd);
		DrawCPURegister(hWnd, hDC);
		ReleaseDC(hWnd, hDC);
	}

	/* サブCPU */
	if (hSubWnd[SWND_CPUREG_SUB]) {
		hWnd = hSubWnd[SWND_CPUREG_SUB];
		hDC = GetDC(hWnd);
		DrawCPURegister(hWnd, hDC);
		ReleaseDC(hWnd, hDC);
	}
}

/*
 *	CPUレジスタウインドウ
 *	再描画
 */
static void FASTCALL PaintCPURegister(HWND hWnd)
{
	HDC hDC;
	PAINTSTRUCT ps;
	int i;
	RECT rect;
	BYTE *p;
	int x, y;

	ASSERT(hWnd);

	/* Drawバッファを得る */
	for (i=0; i<SWND_MAXNUM; i++) {
		if (hSubWnd[i] == hWnd) {
			if ((i & 1) == 0) {
				p = pCPURegisterMain;
			}
			else {
				p = pCPURegisterSub;
			}
			break;
		}
	}

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
	DrawCPURegister(hWnd, hDC);
	EndPaint(hWnd, &ps);
}

/*
 *	CPUレジスタウインドウ
 *	ウインドウプロシージャ
 */
static LRESULT CALLBACK CPURegisterProc(HWND hWnd, UINT message,
								 WPARAM wParam, LPARAM lParam)
{
	int i;

	/* メッセージ別 */
	switch (message) {
		/* ウインドウ再描画 */
		case WM_PAINT:
			/* ロックが必要 */
			LockVM();
			PaintCPURegister(hWnd);
			UnlockVM();
			return 0;

		/* ウインドウ削除 */
		case WM_DESTROY:
			/* メインウインドウへ自動通知 */
			for (i=0; i<SWND_MAXNUM; i++) {
				if (hSubWnd[i] == hWnd) {
					hSubWnd[i] = NULL;
					/* Drawバッファ削除 */
					if ((i & 1) == 0) {
						ASSERT(pCPURegisterMain);
						free(pCPURegisterMain);
						pCPURegisterMain = NULL;
					}
					else {
						ASSERT(pCPURegisterSub);
						free(pCPURegisterSub);
						pCPURegisterSub = NULL;
					}
				}
			}
			break;
	}

	/* デフォルト ウインドウプロシージャ */
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/*
 *	CPUレジスタウインドウ
 *	ウインドウ作成
 */
HWND FASTCALL CreateCPURegister(HWND hParent, BOOL bMain, int index)
{
	WNDCLASSEX wcex;
	char szClassName[] = "XM7_CPURegister";
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
	rect.right = lCharWidth * 17;
	rect.bottom = lCharHeight * 5;

	/* ウインドウタイトルを決定、バッファ確保 */
	if (bMain) {
		LoadString(hAppInstance, IDS_SWND_CPUREG_MAIN,
					szWndName, sizeof(szWndName));
		pCPURegisterMain = malloc(2 * (rect.right / lCharWidth) *
									(rect.bottom / lCharHeight));
	}
	else {
		LoadString(hAppInstance, IDS_SWND_CPUREG_SUB,
					szWndName, sizeof(szWndName));
		pCPURegisterSub = malloc(2 * (rect.right / lCharWidth) *
									(rect.bottom / lCharHeight));
	}

	/* ウインドウクラスの登録 */
	memset(&wcex, 0, sizeof(wcex));
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_VREDRAW | CS_HREDRAW;
	wcex.lpfnWndProc = CPURegisterProc;
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

/*-[ 逆アセンブルウインドウ ]------------------------------------------------*/

/*
 *	逆アセンブルウインドウ
 *	アドレス設定
 */
void FASTCALL AddrDisAsm(BOOL bMain, WORD wAddr)
{
	SCROLLINFO sif;

	memset(&sif, 0, sizeof(sif));
	sif.cbSize = sizeof(sif);
	sif.fMask = SIF_POS;
	sif.nPos = (int)wAddr;

	/* 存在チェック＆設定 */
	if (bMain) {
		if (hSubWnd[SWND_DISASM_MAIN]) {
			wDisAsmMain = wAddr;
			SetScrollInfo(hSubWnd[SWND_DISASM_MAIN], SB_VERT, &sif, TRUE);
		}
	}
	else {
		if (hSubWnd[SWND_DISASM_SUB]) {
			wDisAsmSub = wAddr;
			SetScrollInfo(hSubWnd[SWND_DISASM_SUB], SB_VERT, &sif, TRUE);
		}
	}

	/* リフレッシュ */
	RefreshDisAsm();
}

/*
 *	逆アセンブルウインドウ
 *	セットアップ
 */
static void FASTCALL SetupDisAsm(BOOL bMain, BYTE *p, int x, int y)
{
	char string[128];
	int addr;
	int cpu;
	int i;
	int j;
	int k;
	int ret;

	ASSERT((bMain == TRUE) || (bMain == FALSE));
	ASSERT(p);
	ASSERT(x > 0);
	ASSERT(y > 0);

	/* 一旦スペースで埋める */
	memset(p, 0x20, x * y);

	/* 初期設定 */
	if (bMain) {
		cpu = MAINCPU;
		addr = (int)wDisAsmMain;
	}
	else {
		cpu = SUBCPU;
		addr = (int)wDisAsmSub;
	}

	for (i=0; i<y; i++) {
		 /* 逆アセンブル */
		ret = disline(cpu, (WORD)addr, string);

		/* セット */
		memcpy(&p[x * i + 2], string, strlen(string));

		/* マーク */
		if ((cpu == MAINCPU) && (maincpu.pc == addr)) {
			p[x * i + 1] = '>';
		}
		if ((cpu == SUBCPU) && (subcpu.pc == addr)) {
			p[x * i + 1] = '>';
		}
		for (j=0; j<BREAKP_MAXNUM; j++) {
			if (breakp[j].addr != addr) {
				continue;
			}
			if (breakp[j].cpu != cpu) {
				continue;
			}
			if (breakp[j].flag == BREAKP_NOTUSE) {
				continue;
			}
			if (breakp[j].flag == BREAKP_DISABLED) {
				continue;
			}

			/* ブレークポイント */
			p[x * i + 0] = (BYTE)('1' + j);

			/* 反転 */
			for (k=0; k<x; k++) {
				 p[x * i + k] = (BYTE)(p[x * i + k] | 0x80);
			}
		}

		/* 加算、オーバーチェック */
		addr += ret;
		if (addr >= 0x10000) {
			break;
		}
	}
}

/*
 *	逆アセンブルウインドウ
 *	描画
 */
static void FASTCALL DrawDisAsm(HWND hWnd, HDC hDC)
{
	int i;
	BOOL bMain;
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

	/* Drawバッファを得る */
	for (i=0; i<SWND_MAXNUM; i++) {
		if (hSubWnd[i] == hWnd) {
			if ((i & 1) == 0) {
				p = pDisAsmMain;
				bMain = TRUE;
			}
			else {
				p = pDisAsmSub;
				bMain = FALSE;
			}
			break;
		}
	}
	if (!p) {
		return;
	}

	/* ウインドウジオメトリを得る */
	GetClientRect(hWnd, &rect);
	x = rect.right / lCharWidth;
	y = rect.bottom / lCharHeight;
	if ((x == 0) || (y == 0)) {
		return;
	}

	/* セットアップ */
	SetupDisAsm(bMain, p, x, y);

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
 *	逆アセンブルウインドウ
 *	リフレッシュ
 */
void FASTCALL RefreshDisAsm(void)
{
	HWND hWnd;
	HDC hDC;

	/* メインCPU */
	if (hSubWnd[SWND_DISASM_MAIN]) {
		hWnd = hSubWnd[SWND_DISASM_MAIN];
		hDC = GetDC(hWnd);
		DrawDisAsm(hWnd, hDC);
		ReleaseDC(hWnd, hDC);
	}

	/* サブCPU */
	if (hSubWnd[SWND_DISASM_SUB]) {
		hWnd = hSubWnd[SWND_DISASM_SUB];
		hDC = GetDC(hWnd);
		DrawDisAsm(hWnd, hDC);
		ReleaseDC(hWnd, hDC);
	}
}

/*
 *	逆アセンブルウインドウ
 *	再描画
 */
static void FASTCALL PaintDisAsm(HWND hWnd)
{
	HDC hDC;
	PAINTSTRUCT ps;
	int i;
	RECT rect;
	BYTE *p;
	int x, y;

	ASSERT(hWnd);

	/* Drawバッファを得る */
	for (i=0; i<SWND_MAXNUM; i++) {
		if (hSubWnd[i] == hWnd) {
			if ((i & 1) == 0) {
				p = pDisAsmMain;
			}
			else {
				p = pDisAsmSub;
			}
			break;
		}
	}

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
	DrawDisAsm(hWnd, hDC);
	EndPaint(hWnd, &ps);
}

/*
 *	逆アセンブルウインドウ
 *	左クリック
 */
static void FASTCALL LButtonDisAsm(HWND hWnd, POINT point)
{
	char string[128];
	int addr;
	BOOL flag;
	int cpu;
	int i;
	int y;
	int ret;

	ASSERT(hWnd);

	/* 行カウントを得る */
	y = point.y / lCharHeight;

	/* 実際に逆アセンブルしてみる */
	for (i=0; i<SWND_MAXNUM; i++) {
		if (hSubWnd[i] == hWnd) {
			if ((i & 1) == 0) {
				cpu = MAINCPU;
				addr = (int)wDisAsmMain;
			}
			else {
				cpu = SUBCPU;
				addr = (int)wDisAsmSub;
			}
			break;
		}
	}

	/* 逆アセンブル ループ */
	for (i=0; i<y; i++) {
		ret = disline(cpu, (WORD)addr, string);
		addr += ret;
		if (addr >= 0x10000) {
			return;
		}
	}

	/* ブレークポイント on/off */
	flag = FALSE;
	for (i=0; i<BREAKP_MAXNUM; i++) {
		if ((breakp[i].cpu == cpu) && (breakp[i].addr == addr)) {
			if (breakp[i].flag != BREAKP_NOTUSE) {
				breakp[i].flag = BREAKP_NOTUSE;
				flag = TRUE;
				break;
			}
		}
	}
	if (!flag) {
		schedule_setbreak(cpu, (WORD)addr);
	}

	/* 再描画 */
	InvalidateRect(hWnd, NULL, FALSE);

	/* ブレークポイントウインドウも、再描画する */
	if (hSubWnd[SWND_BREAKPOINT]) {
		InvalidateRect(hSubWnd[SWND_BREAKPOINT], NULL, FALSE);
	}
}

/*
 *	逆アセンブルウインドウ
 *	コマンド処理
 */
static FASTCALL void CmdDisAsm(HWND hWnd, WORD wID, BOOL bMain)
{
	WORD target;
	cpu6809_t *cpu;

	/* CPU構造体決定 */
	if (bMain) {
		cpu = &maincpu;
	}
	else {
		cpu = &subcpu;
	}

	/* ターゲットアドレス決定 */
	switch (wID) {
		case IDM_DIS_ADDR:
			if (bMain) {
				target = wDisAsmMain;
			}
			else {
				target = wDisAsmSub;
			}
			if (!AddrDlg(hWnd, &target)) {
				return;
			}
			break;

		case IDM_DIS_PC:
			target = cpu->pc;
			break;
		case IDM_DIS_X:
			target = cpu->x;
			break;
		case IDM_DIS_Y:
			target = cpu->y;
			break;
		case IDM_DIS_U:
			target = cpu->u;
			break;
		case IDM_DIS_S:
			target = cpu->s;
			break;

		case IDM_DIS_RESET:
			target = (WORD)((cpu->readmem(0xfffe) << 8) | cpu->readmem(0xffff));
			break;
		case IDM_DIS_NMI:
			target = (WORD)((cpu->readmem(0xfffc) << 8) | cpu->readmem(0xfffd));
			break;
		case IDM_DIS_SWI:
			target = (WORD)((cpu->readmem(0xfffa) << 8) | cpu->readmem(0xfffb));
			break;
		case IDM_DIS_IRQ:
			target = (WORD)((cpu->readmem(0xfff8) << 8) | cpu->readmem(0xfff9));
			break;
		case IDM_DIS_FIRQ:
			target = (WORD)((cpu->readmem(0xfff6) << 8) | cpu->readmem(0xfff7));
			break;
		case IDM_DIS_SWI2:
			target = (WORD)((cpu->readmem(0xfff4) << 8) | cpu->readmem(0xfff5));
			break;
		case IDM_DIS_SWI3:
			target = (WORD)((cpu->readmem(0xfff2) << 8) | cpu->readmem(0xfff3));
			break;
	}

	/* 設定＆更新 */
	AddrDisAsm(bMain, target);
}

/*
 *	逆アセンブルウインドウ
 *	ウインドウプロシージャ
 */
static LRESULT CALLBACK DisAsmProc(HWND hWnd, UINT message,
								 WPARAM wParam, LPARAM lParam)
{
	BOOL bMain;
	WORD wAddr;
	char string[128];
	int ret;
	int i;
	POINT point;
	HMENU hMenu;

	/* メッセージ別 */
	switch (message) {
		/* ウインドウ再描画 */
		case WM_PAINT:
			/* ロックが必要 */
			LockVM();
			PaintDisAsm(hWnd);
			UnlockVM();
			return 0;

		/* ウインドウ削除 */
		case WM_DESTROY:
			/* メインウインドウへ自動通知 */
			for (i=0; i<SWND_MAXNUM; i++) {
				if (hSubWnd[i] == hWnd) {
					hSubWnd[i] = NULL;
					/* Drawバッファ削除 */
					if ((i & 1) == 0) {
						DestroyMenu(hDisAsmMain);
						ASSERT(pDisAsmMain);
						free(pDisAsmMain);
						pDisAsmMain = NULL;
					}
					else {
						DestroyMenu(hDisAsmSub);
						ASSERT(pDisAsmSub);
						free(pDisAsmSub);
						pDisAsmSub = NULL;
					}
				}
			}
			break;

		/* コンテキストメニュー */
		case WM_CONTEXTMENU:
			/* コンテキストメニューをロード */
			if (hSubWnd[SWND_DISASM_MAIN] == hWnd) {
				hMenu = GetSubMenu(hDisAsmMain, 0);
			}
			else {
				hMenu = GetSubMenu(hDisAsmSub, 0);
			}

			/* コンテキストメニューを実行 */
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);
			TrackPopupMenu(hMenu, 0, point.x, point.y, 0, hWnd, NULL);
			return 0;

		/* 左クリック・ダブルクリック */
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);
			LButtonDisAsm(hWnd, point);
			break;

		/* コマンド */
		case WM_COMMAND:
			if (hSubWnd[SWND_DISASM_MAIN] == hWnd) {
				CmdDisAsm(hWnd, LOWORD(wParam), TRUE);
			}
			else {
				CmdDisAsm(hWnd, LOWORD(wParam), FALSE);
			}
			break;

		/* 垂直スクロールバー */
		case WM_VSCROLL:
			/* タイプ判別 */
			if (hSubWnd[SWND_DISASM_MAIN] == hWnd) {
				bMain = TRUE;
				wAddr = wDisAsmMain;
			}
			else {
				bMain = FALSE;
				wAddr = wDisAsmSub;
			}

			/* アクション別 */
			switch (LOWORD(wParam)) {
				/* トップ */
				case SB_TOP:
					wAddr = 0;
					break;
				/* 終端 */
				case SB_BOTTOM:
					wAddr = 0xffff;
					break;
				/* １行上 */
				case SB_LINEUP:
					if (wAddr > 0) {
						wAddr--;
					}
					break;
				/* １行下(ここは工夫) */
				case SB_LINEDOWN:
					if (bMain) {
						ret = disline(MAINCPU, wAddr, string);
					}
					else {
						ret = disline(SUBCPU, wAddr, string);
					}
					i = (int)wAddr;
					i += ret;
					if (i < 0x10000) {
						wAddr += (WORD)ret;
					}
					break;
				/* ページアップ */
				case SB_PAGEUP:
					if (wAddr < 0x80) {
						wAddr = 0;
					}
					else {
						wAddr -= (WORD)0x80;
					}
					break;
				/* ページダウン */
				case SB_PAGEDOWN:
					if (wAddr >= 0xff80) {
						wAddr = 0xffff;
					}
					else {
						wAddr += (WORD)0x80;
					}
					break;
				/* 直接指定 */
				case SB_THUMBTRACK:
					wAddr = HIWORD(wParam);
					break;
			}
			AddrDisAsm(bMain, wAddr);
			RefreshDisAsm();
			break;
	}

	/* デフォルト ウインドウプロシージャ */
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/*
 *	逆アセンブルウインドウ
 *	ウインドウ作成
 */
HWND FASTCALL CreateDisAsm(HWND hParent, BOOL bMain, int index)
{
	WNDCLASSEX wcex;
	char szClassName[] = "XM7_DisAsm";
	char szWndName[128];
	RECT rect;
	RECT crect, wrect;
	HWND hWnd;
	SCROLLINFO si;

	ASSERT(hParent);

	/* ウインドウ矩形を計算 */
	rect.left = lCharWidth * index;
	rect.top = lCharHeight * index;
	if (rect.top >= 380) {
		rect.top -= 380;
	}
	rect.right = lCharWidth * 47;
	rect.bottom = lCharHeight * 8;

	/* ウインドウタイトルを決定、バッファ確保、メニュロード */
	if (bMain) {
		LoadString(hAppInstance, IDS_SWND_DISASM_MAIN,
					szWndName, sizeof(szWndName));
		pDisAsmMain = malloc(2 * (rect.right / lCharWidth) *
									(rect.bottom / lCharHeight));
		wDisAsmMain = maincpu.pc;
		hDisAsmMain = LoadMenu(hAppInstance, MAKEINTRESOURCE(IDR_DISASMMENU));
	}
	else {
		LoadString(hAppInstance, IDS_SWND_DISASM_SUB,
					szWndName, sizeof(szWndName));
		pDisAsmSub = malloc(2 * (rect.right / lCharWidth) *
									(rect.bottom / lCharHeight));
		wDisAsmSub = subcpu.pc;
		hDisAsmSub = LoadMenu(hAppInstance, MAKEINTRESOURCE(IDR_DISASMMENU));
	}

	/* ウインドウクラスの登録 */
	memset(&wcex, 0, sizeof(wcex));
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_VREDRAW | CS_HREDRAW;
	wcex.lpfnWndProc = DisAsmProc;
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
						| WS_VISIBLE | WS_MINIMIZEBOX | WS_CLIPSIBLINGS
						| WS_VSCROLL,
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

		/* スクロールバーの設定が必要 */
		memset(&si, 0, sizeof(si));
		si.cbSize = sizeof(si);
		si.fMask = SIF_ALL;
		si.nMin = 0;
		si.nMax = 0xffff;
		si.nPage = 0x100;
		if (bMain) {
			si.nPos = maincpu.pc;
		}
		else {
			si.nPos = subcpu.pc;
		}
		SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
	}

	/* 結果を持ち帰る */
	return hWnd;
}

/*-[ メモリダンプウインドウ ]------------------------------------------------*/

/*
 *	メモリダンプウインドウ
 *	アドレス設定
 */
void FASTCALL AddrMemory(BOOL bMain, WORD wAddr)
{
	SCROLLINFO sif;

	wAddr &= (WORD)(0xfff0);

	memset(&sif, 0, sizeof(sif));
	sif.cbSize = sizeof(sif);
	sif.fMask = SIF_POS;
	sif.nPos = (int)(wAddr >> 4);

	/* 存在チェック＆設定 */
	if (bMain) {
		if (hSubWnd[SWND_MEMORY_MAIN]) {
			wMemoryMain = wAddr;
			SetScrollInfo(hSubWnd[SWND_MEMORY_MAIN], SB_VERT, &sif, TRUE);
		}
	}
	else {
		if (hSubWnd[SWND_MEMORY_SUB]) {
			wMemorySub = wAddr;
			SetScrollInfo(hSubWnd[SWND_MEMORY_SUB], SB_VERT, &sif, TRUE);
		}
	}

	/* リフレッシュ */
	RefreshMemory();
}

/*
 *	メモリダンプウインドウ
 *	セットアップ
 */
static void FASTCALL SetupMemory(BOOL bMain, BYTE *p, int x, int y)
{
	char string[128];
	char temp[4];
	WORD addr;
	int i;
	int j;

	ASSERT((bMain == TRUE) || (bMain == FALSE));
	ASSERT(p);
	ASSERT(x > 0);
	ASSERT(y > 0);

	/* 一旦スペースで埋める */
	memset(p, 0x20, x * y);

	/* アドレス取得 */
	if (bMain) {
		addr = (WORD)(wMemoryMain & 0xfff0);
	}
	else {
		addr = (WORD)(wMemorySub & 0xfff0);
	}

	/* ループ */
	for (i=0; i<8; i++) {
		/* 作成 */
		sprintf(string, "%04X:", addr);
		for (j=0; j<16; j++) {
			if (bMain) {
				sprintf(temp, " %02X", mainmem_readbnio((WORD)(addr + j)));
			}
			else {
				sprintf(temp, " %02X", submem_readbnio((WORD)(addr + j)));
			}
			strcat(string, temp);
		}

		/* コピー */
		memcpy(&p[x * i], string, strlen(string));

		/* 次へ */
		addr += (WORD)0x0010;
		if (addr == 0) {
			break;
		}
	}
}

/*
 *	メモリダンプウインドウ
 *	描画
 */
static void FASTCALL DrawMemory(HWND hWnd, HDC hDC)
{
	int i;
	BOOL bMain;
	RECT rect;
	BYTE *p, *q;
	int x, y;
	int xx, yy;
	HFONT hFont;
	HFONT hBackup;

	ASSERT(hWnd);
	ASSERT(hDC);

	/* Drawバッファを得る */
	for (i=0; i<SWND_MAXNUM; i++) {
		if (hSubWnd[i] == hWnd) {
			if ((i & 1) == 0) {
				p = pMemoryMain;
				bMain = TRUE;
			}
			else {
				p = pMemorySub;
				bMain = FALSE;
			}
			break;
		}
	}
	if (!p) {
		return;
	}

	/* ウインドウジオメトリを得る */
	GetClientRect(hWnd, &rect);
	x = rect.right / lCharWidth;
	y = rect.bottom / lCharHeight;
	if ((x == 0) || (y == 0)) {
		return;
	}

	/* セットアップ */
	SetupMemory(bMain, p, x, y);

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
 *	メモリダンプウインドウ
 *	リフレッシュ
 */
void FASTCALL RefreshMemory(void)
{
	HWND hWnd;
	HDC hDC;

	/* メインCPU */
	if (hSubWnd[SWND_MEMORY_MAIN]) {
		hWnd = hSubWnd[SWND_MEMORY_MAIN];
		hDC = GetDC(hWnd);
		DrawMemory(hWnd, hDC);
		ReleaseDC(hWnd, hDC);
	}

	/* サブCPU */
	if (hSubWnd[SWND_MEMORY_SUB]) {
		hWnd = hSubWnd[SWND_MEMORY_SUB];
		hDC = GetDC(hWnd);
		DrawMemory(hWnd, hDC);
		ReleaseDC(hWnd, hDC);
	}
}

/*
 *	メモリダンプウインドウ
 *	再描画
 */
static void FASTCALL PaintMemory(HWND hWnd)
{
	HDC hDC;
	PAINTSTRUCT ps;
	int i;
	RECT rect;
	BYTE *p;
	int x, y;

	ASSERT(hWnd);

	/* Drawバッファを得る */
	for (i=0; i<SWND_MAXNUM; i++) {
		if (hSubWnd[i] == hWnd) {
			if ((i & 1) == 0) {
				p = pMemoryMain;
			}
			else {
				p = pMemorySub;
			}
			break;
		}
	}

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
	DrawMemory(hWnd, hDC);
	EndPaint(hWnd, &ps);
}

/*
 *	メモリダンプウインドウ
 *	コマンド処理
 */
static FASTCALL void CmdMemory(HWND hWnd, WORD wID, BOOL bMain)
{
	WORD target;
	cpu6809_t *cpu;

	/* CPU構造体決定 */
	if (bMain) {
		cpu = &maincpu;
	}
	else {
		cpu = &subcpu;
	}

	/* ターゲットアドレス決定 */
	switch (wID) {
		case IDM_DIS_ADDR:
			if (bMain) {
				target = wDisAsmMain;
			}
			else {
				target = wDisAsmSub;
			}
			if (!AddrDlg(hWnd, &target)) {
				return;
			}
			break;

		case IDM_DIS_PC:
			target = cpu->pc;
			break;
		case IDM_DIS_X:
			target = cpu->x;
			break;
		case IDM_DIS_Y:
			target = cpu->y;
			break;
		case IDM_DIS_U:
			target = cpu->u;
			break;
		case IDM_DIS_S:
			target = cpu->s;
			break;

		case IDM_DIS_RESET:
			target = (WORD)((cpu->readmem(0xfffe) << 8) | cpu->readmem(0xffff));
			break;
		case IDM_DIS_NMI:
			target = (WORD)((cpu->readmem(0xfffc) << 8) | cpu->readmem(0xfffd));
			break;
		case IDM_DIS_SWI:
			target = (WORD)((cpu->readmem(0xfffa) << 8) | cpu->readmem(0xfffb));
			break;
		case IDM_DIS_IRQ:
			target = (WORD)((cpu->readmem(0xfff8) << 8) | cpu->readmem(0xfff9));
			break;
		case IDM_DIS_FIRQ:
			target = (WORD)((cpu->readmem(0xfff6) << 8) | cpu->readmem(0xfff7));
			break;
		case IDM_DIS_SWI2:
			target = (WORD)((cpu->readmem(0xfff4) << 8) | cpu->readmem(0xfff5));
			break;
		case IDM_DIS_SWI3:
			target = (WORD)((cpu->readmem(0xfff2) << 8) | cpu->readmem(0xfff3));
			break;
	}

	/* 設定＆更新 */
	AddrMemory(bMain, target);
}

/*
 *	メモリダンプウインドウ
 *	ウインドウプロシージャ
 */
static LRESULT CALLBACK MemoryProc(HWND hWnd, UINT message,
								 WPARAM wParam, LPARAM lParam)
{
	HMENU hMenu;
	POINT point;
	BOOL bMain;
	WORD wAddr;
	int i;

	/* メッセージ別 */
	switch (message) {
		/* ウインドウ再描画 */
		case WM_PAINT:
			/* ロックが必要 */
			LockVM();
			PaintMemory(hWnd);
			UnlockVM();
			return 0;

		/* ウインドウ削除 */
		case WM_DESTROY:
			/* メインウインドウへ自動通知 */
			for (i=0; i<SWND_MAXNUM; i++) {
				if (hSubWnd[i] == hWnd) {
					hSubWnd[i] = NULL;
					/* Drawバッファ削除、メニュー削除 */
					if ((i & 1) == 0) {
						DestroyMenu(hMemoryMain);
						ASSERT(pMemoryMain);
						free(pMemoryMain);
						pMemoryMain = NULL;
					}
					else {
						DestroyMenu(hMemorySub);
						ASSERT(pMemorySub);
						free(pMemorySub);
						pMemorySub = NULL;
					}
				}
			}
			break;

		/* コンテキストメニュー */
		case WM_CONTEXTMENU:
			/* サブメニューを取り出す */
			if (hSubWnd[SWND_MEMORY_MAIN] == hWnd) {
				hMenu = GetSubMenu(hMemoryMain, 0);
			}
			else {
				hMenu = GetSubMenu(hMemorySub, 0);
			}

			/* コンテキストメニューを実行 */
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);
			TrackPopupMenu(hMenu, 0, point.x, point.y, 0, hWnd, NULL);
			return 0;

		/* コマンド */
		case WM_COMMAND:
			if (hSubWnd[SWND_MEMORY_MAIN] == hWnd) {
				CmdMemory(hWnd, LOWORD(wParam), TRUE);
			}
			else {
				CmdMemory(hWnd, LOWORD(wParam), FALSE);
			}
			break;

		/* 垂直スクロールバー */
		case WM_VSCROLL:
			/* タイプ判別 */
			if (hSubWnd[SWND_MEMORY_MAIN] == hWnd) {
				bMain = TRUE;
				wAddr = wMemoryMain;
			}
			else {
				bMain = FALSE;
				wAddr = wMemorySub;
			}

			/* アクション別 */
			switch (LOWORD(wParam)) {
				/* トップ */
				case SB_TOP:
					wAddr = 0;
					break;
				/* 終端 */
				case SB_BOTTOM:
					wAddr = 0xfff0;
					break;
				/* １行上 */
				case SB_LINEUP:
					if (wAddr >= 0x0010) {
						wAddr -= (WORD)0x0010;
					}
					break;
				/* １行下 */
				case SB_LINEDOWN:
					if (wAddr < 0xfff0) {
						wAddr += (WORD)0x0010;
					}
					break;
				/* ページアップ */
				case SB_PAGEUP:
					if (wAddr < 0x100) {
						wAddr = 0;
					}
					else {
						wAddr -= (WORD)0x100;
					}
					break;
				/* ページダウン */
				case SB_PAGEDOWN:
					if (wAddr >= 0xfef0) {
						wAddr = 0xfff0;
					}
					else {
						wAddr += (WORD)0x100;
					}
					break;
				/* 直接指定 */
				case SB_THUMBTRACK:
					wAddr = (WORD)(HIWORD(wParam) * 16);
					break;
			}
			wAddr &= (WORD)0xfff0;
			AddrMemory(bMain, wAddr);
			RefreshMemory();
			break;
	}

	/* デフォルト ウインドウプロシージャ */
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/*
 *	メモリダンプウインドウ
 *	ウインドウ作成
 */
HWND FASTCALL CreateMemory(HWND hParent, BOOL bMain, int index)
{
	WNDCLASSEX wcex;
	char szClassName[] = "XM7_Memory";
	char szWndName[128];
	RECT rect;
	RECT crect, wrect;
	HWND hWnd;
	SCROLLINFO si;

	ASSERT(hParent);

	/* ウインドウ矩形を計算 */
	rect.left = lCharWidth * index;
	rect.top = lCharHeight * index;
	if (rect.top >= 380) {
		rect.top -= 380;
	}
	rect.right = lCharWidth * 53;
	rect.bottom = lCharHeight * 8;

	/* ウインドウタイトルを決定、バッファ確保、メニューロード */
	if (bMain) {
		LoadString(hAppInstance, IDS_SWND_MEMORY_MAIN,
					szWndName, sizeof(szWndName));
		pMemoryMain = malloc(2 * (rect.right / lCharWidth) *
									(rect.bottom / lCharHeight));
		wMemoryMain = maincpu.pc;
		hMemoryMain = LoadMenu(hAppInstance, MAKEINTRESOURCE(IDR_DISASMMENU));
	}
	else {
		LoadString(hAppInstance, IDS_SWND_MEMORY_SUB,
					szWndName, sizeof(szWndName));
		pMemorySub = malloc(2 * (rect.right / lCharWidth) *
									(rect.bottom / lCharHeight));
		wMemorySub = subcpu.pc;
		hMemorySub = LoadMenu(hAppInstance, MAKEINTRESOURCE(IDR_DISASMMENU));
	}

	/* ウインドウクラスの登録 */
	memset(&wcex, 0, sizeof(wcex));
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_VREDRAW | CS_HREDRAW;
	wcex.lpfnWndProc = MemoryProc;
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
						| WS_VISIBLE | WS_MINIMIZEBOX | WS_CLIPSIBLINGS
						| WS_VSCROLL,
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

		/* スクロールバーの設定が必要 */
		memset(&si, 0, sizeof(si));
		si.cbSize = sizeof(si);
		si.fMask = SIF_ALL;
		si.nMin = 0;
		si.nMax = 0xfff;
		si.nPage = 0x10;
		if (bMain) {
			si.nPos = (maincpu.pc >> 4);
		}
		else {
			si.nPos = (subcpu.pc >> 4);
		}
		SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
	}

	/* 結果を持ち帰る */
	return hWnd;
}

#endif	/* _WIN32 */
