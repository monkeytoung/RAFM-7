/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API メインプログラム ]
 */

#ifdef _WIN32

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <imm.h>
#include <shellapi.h>
#include <assert.h>
#include "xm7.h"
#include "w32.h"
#include "w32_bar.h"
#include "w32_draw.h"
#include "w32_dd.h"
#include "w32_kbd.h"
#include "w32_sch.h"
#include "w32_snd.h"
#include "w32_res.h"
#include "w32_sub.h"
#include "w32_cfg.h"

/*
 *	グローバル ワーク
 */
HINSTANCE hAppInstance;					/* アプリケーション インスタンス */
HWND hMainWnd;							/* メインウインドウ */
HWND hDrawWnd;							/* ドローウインドウ */
int nErrorCode;							/* エラーコード */
BOOL bMenuLoop;							/* メニューループ中 */
BOOL bCloseReq;							/* 終了要求フラグ */
LONG lCharWidth;						/* キャラクタ横幅 */
LONG lCharHeight;						/* キャラクタ縦幅 */
BOOL bSync;								/* 実行に同期 */
BOOL bActivate;							/* アクティベートフラグ */

/*
 *	スタティック ワーク
 */
static CRITICAL_SECTION CSection;		/* クリティカルセクション */

/*-[ サブウィンドウサポート ]-----------------------------------------------*/

/*
 *	テキストフォント作成
 *	※呼び出し元でDeleteObjectすること
 */
HFONT FASTCALL CreateTextFont(void)
{
	HFONT hFont;
	LANGID lang;

	/* デフォルト言語を取得 */
	lang = GetUserDefaultLangID();

	/* 言語判定して、シフトJIS・ANSIどちらかのフォントを作る */
	if ((lang & 0xff) == 0x11) {
		/* 日本語 */
		hFont = CreateFont(-16, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE,
			SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, FIXED_PITCH, NULL);
	}
	else {
		/* 英語 */
		hFont = CreateFont(-16, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE,
			ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, FIXED_PITCH, NULL);
	}

	ASSERT(hFont);
	return hFont;
}

/*
 *	サブウィンドウセットアップ
 */
static void FASTCALL SetupSubWnd(HWND hWnd)
{
	HFONT hFont;
	HFONT hBackup;
	HDC hDC;
	TEXTMETRIC tm;

	ASSERT(hWnd);

	/* ウインドウワークをクリア */
	memset(hSubWnd, 0, sizeof(hSubWnd));

	/* フォント作成 */
	hFont = CreateTextFont();
	ASSERT(hFont);

	/* DC取得、フォントセレクト */
	hDC = GetDC(hWnd);
	ASSERT(hDC);
	hBackup = SelectObject(hDC, hFont);
	ASSERT(hBackup);

	/* テキストメトリック取得 */
	GetTextMetrics(hDC, &tm);

	/* フォント、DCクリーンアップ */
	SelectObject(hDC, hBackup);
	DeleteObject(hFont);
	ReleaseDC(hWnd, hDC);

	/* 結果をストア */
	lCharWidth = tm.tmAveCharWidth;
	lCharHeight = tm.tmHeight + tm.tmExternalLeading;
}

/*-[ 同期 ]-----------------------------------------------------------------*/

/*
 *	VMをロック
 */
void FASTCALL LockVM(void)
{
	EnterCriticalSection(&CSection);
}

/*
 *	VMをアンロック
 */
void FASTCALL UnlockVM(void)
{
	LeaveCriticalSection(&CSection);
}

/*-[ ドローウインドウ ]-----------------------------------------------------*/

/*
 * 	ウインドウプロシージャ
 */
static LRESULT CALLBACK DrawWndProc(HWND hWnd, UINT message,
								 WPARAM wParam,LPARAM lParam)
{
	/* メッセージ別 */
	switch (message) {
		/* ウインドウ背景描画 */
		case WM_ERASEBKGND:
			/* エラーなしなら、背景描画せず */
			if (nErrorCode == 0) {
				return 0;
			}
			break;

		/* ウインドウ再描画 */
		case WM_PAINT:
			/* ロックが必要 */
			LockVM();
			OnPaint(hWnd);
			UnlockVM();
			return 0;
	}

	/* デフォルト ウインドウプロシージャ */
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/*
 *	ドローウインドウ作成
 */
static HWND FASTCALL CreateDraw(HWND hParent)
{
	WNDCLASSEX wcex;
	char szWndName[] = "XM7_Draw";
	RECT rect;
	HWND hWnd;

	ASSERT(hParent);

	/* 親ウインドウの矩形を取得 */
	GetClientRect(hParent, &rect);

	/* ウインドウクラスの登録 */
	memset(&wcex, 0, sizeof(wcex));
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_VREDRAW | CS_HREDRAW;
	wcex.lpfnWndProc = DrawWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hAppInstance;
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWndName;
	wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	RegisterClassEx(&wcex);

	/* ウインドウ作成 */
	hWnd = CreateWindow(szWndName,
						szWndName,
						WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
						0,
						0,
						rect.right,
						rect.bottom,
						hParent,
						NULL,
						hAppInstance,
						NULL);

	/* 結果を持ち帰る */
	return hWnd;
}

/*-[ メインウインドウ ]-----------------------------------------------------*/

/*
 *	ウインドウ作成
 */
static void FASTCALL OnCreate(HWND hWnd)
{
	BOOL flag;

	ASSERT(hWnd);

	/* クリティカルセクション作成 */
	InitializeCriticalSection(&CSection);

	/* ドローウインドウ、ステータスバーを作成 */
	hDrawWnd = CreateDraw(hWnd);
	hStatusBar = CreateStatus(hWnd);

	/* IMEを禁止する */
	ImmAssociateContext(hWnd, (HIMC)NULL);

	/* ファイルドロップ許可 */
	DragAcceptFiles(hWnd, TRUE);

	/* ワークエリア初期化 */
	nErrorCode = 0;
	bMenuLoop = FALSE;
	bCloseReq = FALSE;
	bSync = TRUE;
	bActivate = FALSE;

	/* サブウィンドウ準備 */
	SetupSubWnd(hWnd);

	/* コンポーネント初期化 */
	LoadCfg();
	InitDraw();
	InitSnd();
	InitKbd();
	InitSch();

	/* 仮想マシン初期化 */
	if (!system_init()) {
		nErrorCode = 1;
		PostMessage(hWnd, WM_USER, 0, 0);
		return;
	}
	/* 直後、リセット */
	ApplyCfg();
	system_reset();

	/* コンポーネントセレクト */
	flag = TRUE;
	if (!SelectDraw(hDrawWnd)) {
		flag = FALSE;
	}
	if (!SelectSnd(hWnd)) {
		flag = FALSE;
	}
	if (!SelectKbd(hWnd)) {
		flag = FALSE;
	}
	if (!SelectSch()) {
		flag = FALSE;
	}

	/* エラーコードをセットさせ、スタート */
	if (!flag) {
		nErrorCode = 2;
	}
	PostMessage(hWnd, WM_USER, 0, 0);
}

/*
 *	ウインドウクローズ
 */
static void FASTCALL OnClose(HWND hWnd)
{
	ASSERT(hWnd);

	/* メインウインドウを一度消す(タスクバー対策) */
	ShowWindow(hWnd, SW_HIDE);

	/* フラグアップ */
	LockVM();
	bCloseReq = TRUE;
	UnlockVM();

	/* DestroyWindow */
	DestroyWindow(hWnd);
}

/*
 *	ウインドウ削除
 */
static void FASTCALL OnDestroy(void)
{
	/* コンポーネント クリーンアップ */
	SaveCfg();
	CleanSch();
	CleanKbd();
	CleanSnd();
	CleanDraw();

	/* クリティカルセクション削除 */
	DeleteCriticalSection(&CSection);

	/* 仮想マシン クリーンアップ */
	system_cleanup();

	/* ウインドウハンドルをクリア */
	hMainWnd = NULL;
	hDrawWnd = NULL;
	hStatusBar = NULL;
}

/*
 *	サイズ変更
 */
static void FASTCALL OnSize(HWND hWnd, WORD cx, WORD cy)
{
	RECT crect;
	RECT wrect;
	RECT trect;
	RECT srect;
	int height;

	ASSERT(hWnd);
	ASSERT(hDrawWnd);

	/* 最小化の場合は、何もしない */
	if ((cx == 0) && (cy == 0)) {
		return;
	}

	/* フルスクリーン時も同様 */
	if (bFullScreen) {
		return;
	}

	/* ツールバー、ステータスバーの有無を考慮に入れ、計算 */
	height = 400;
	memset(&trect, 0, sizeof(trect));
	if (IsWindowVisible(hStatusBar)) {
		GetWindowRect(hStatusBar, &srect);
		height += (srect.bottom - srect.top);
	}
	else {
		memset(&srect, 0, sizeof(srect));
	}

	/* クライアント領域のサイズを取得 */
	GetClientRect(hWnd, &crect);

	/* 要求サイズと比較し、補正 */
	if ((crect.right != 640) || (crect.bottom != height)) {
		GetWindowRect(hWnd, &wrect);
		wrect.right -= wrect.left;
		wrect.bottom -= wrect.top;
		wrect.right -= crect.right;
		wrect.bottom -= crect.bottom;
		wrect.right += 640;
		wrect.bottom += height;
		MoveWindow(hWnd, wrect.left, wrect.top, wrect.right, wrect.bottom, TRUE);
	}

	/* メインウインドウ配置 */
	MoveWindow(hDrawWnd, 0, trect.bottom, 640, 400, TRUE);

	/* ステータスバー配置 */
	if (IsWindowVisible(hStatusBar)) {
		MoveWindow(hStatusBar, 0, trect.bottom + 400, 640,
					(srect.bottom - srect.top), TRUE);
		SizeStatus(640);
	}
}

/*
 *	キック(メニューには未定義)
 */
static void FASTCALL OnKick(HWND hWnd)
{
	char buffer[128];

	buffer[0] = '\0';

	/* エラーコード別 */
	switch (nErrorCode) {
		/* エラーなし */
		case 0:
			/* 実行開始 */
			stopreq_flag = FALSE;
			run_flag = TRUE;

			/* コマンドライン処理 */
			OnCmdLine(GetCommandLine());
			break;

		/* VM初期化エラー */
		case 1:
			LoadString(hAppInstance, IDS_VMERROR, buffer, sizeof(buffer));
			MessageBox(hWnd, buffer, "XM7", MB_ICONSTOP | MB_OK);
			PostMessage(hWnd, WM_CLOSE, 0, 0);
			break;

		/* コンポーネント初期化エラー */
		case 2:
			LoadString(hAppInstance, IDS_COMERROR, buffer, sizeof(buffer));
			MessageBox(hWnd, buffer, "XM7", MB_ICONSTOP | MB_OK);
			PostMessage(hWnd, WM_CLOSE, 0, 0);
			break;
	}
}

/*
 * 	ウインドウプロシージャ
 */
static LRESULT CALLBACK WndProc(HWND hWnd, UINT message,
								 WPARAM wParam,LPARAM lParam)
{
	HDC hDC;
	PAINTSTRUCT ps;

	/* メッセージ別 */
	switch (message) {
		/* ウインドウ作成 */
		case WM_CREATE:
			OnCreate(hWnd);
			break;

		/* ウインドウクローズ */
		case WM_CLOSE:
			OnClose(hWnd);
			break;

		/* ウインドウ削除 */
		case WM_DESTROY:
			OnDestroy();
			PostQuitMessage(0);
			return 0;

		/* ウインドウサイズ変更 */
		case WM_SIZE:
			OnSize(hWnd, LOWORD(lParam), HIWORD(wParam));
			break;

		/* ウインドウ再描画 */
		case WM_PAINT:
			/* ロックが必要 */
			LockVM();
			hDC = BeginPaint(hWnd, &ps);
			PaintStatus();
			EndPaint(hWnd, &ps);
			UnlockVM();
			return 0;

		/* ユーザ+0:スタート */
		case WM_USER + 0:
			OnKick(hWnd);
			return 0;

		/* ユーザ+1:コマンドライン */
		case WM_USER + 1:
			OnCmdLine((LPTSTR)wParam);
			break;

		/* メニューループ開始 */
		case WM_ENTERMENULOOP:
			bMenuLoop = TRUE;
			EnterMenuDD(hWnd);
			break;

		/* メニュー選択 */
		case WM_MENUSELECT:
			OnMenuSelect(wParam);
			break;

		/* メニューループ終了 */
		case WM_EXITMENULOOP:
			ExitMenuDD();
			OnExitMenuLoop();
			bMenuLoop = FALSE;
			break;

		/* メニューポップアップ */
		case WM_INITMENUPOPUP:
			if (!HIWORD(lParam)) {
				OnMenuPopup(hWnd, (HMENU)wParam, (UINT)LOWORD(lParam));
				return 0;
			}
			break;

		/* メニューコマンド */
		case WM_COMMAND:
			EnterMenuDD(hWnd);
			OnCommand(hWnd, LOWORD(wParam));
			ExitMenuDD();
			return 0;

		/* ファイルドロップ */
		case WM_DROPFILES:
			OnDropFiles((HANDLE)wParam);
			return 0;

		/* アクティベート */
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_INACTIVE) {
				bActivate = FALSE;
			}
			else {
				bActivate = TRUE;
			}
			break;

		/* オーナードロー */
		case WM_DRAWITEM:
			if (wParam == ID_STATUS_BAR) {
				OwnerDrawStatus((LPDRAWITEMSTRUCT)lParam);
				return TRUE;
			}
			break;
	}

	/* デフォルト ウインドウプロシージャ */
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/*-[ アプリケーション ]-----------------------------------------------------*/

/*
 *	XM7ウインドウを検索、コマンドライン渡す
 */
static BOOL FASTCALL FindXM7Wnd(void)
{
	HWND hWnd;
	char string[128];

	/* ウインドウクラスで検索 */
	hWnd = FindWindow("XM7", NULL);
	if (!hWnd) {
		return FALSE;
	}

	/* テキスト文字列取得 */
	GetWindowText(hWnd, string, sizeof(string));
	string[5] = '\0';
	if (strcmp("XM7 [", string) != 0) {
		return FALSE;
	}

	/* メッセージを送信 */
	SendMessage(hWnd, WM_USER + 1, (WPARAM)GetCommandLine(), (LPARAM)NULL);
	return TRUE;
}

/*
 *	インスタンス初期化
 */
static HWND FASTCALL InitInstance(HINSTANCE hInst, int nCmdShow)
{
	HWND hWnd;
	WNDCLASSEX wcex;
	char szAppName[] = "XM7";

	ASSERT(hInst);

	/* ウインドウハンドルクリア */
	hMainWnd = NULL;
	hDrawWnd = NULL;
	hStatusBar = NULL;

	/* ウインドウクラスの登録 */
	memset(&wcex, 0, sizeof(wcex));
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_VREDRAW | CS_HREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInst;
	wcex.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_APPICON));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDR_MAINMENU);
	wcex.lpszClassName = szAppName;
	wcex.hIconSm = LoadIcon(hInst, MAKEINTRESOURCE(IDI_APPICON));
	RegisterClassEx(&wcex);

	/* ウインドウ作成 */
	hWnd = CreateWindow(szAppName,
						szAppName,
						WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_BORDER |
						WS_CLIPCHILDREN | WS_MINIMIZEBOX,
						CW_USEDEFAULT,
						CW_USEDEFAULT,
						CW_USEDEFAULT,
						CW_USEDEFAULT,
						NULL,
						NULL,
						hInst,
						NULL);
	if (!hWnd) {
		return NULL;
	}

	/* ウインドウ表示 */
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return hWnd;
}

/*
 *	WinMain
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
						LPSTR lpszCmdParam, int nCmdShow)
{
	MSG msg;
	HACCEL hAccel;

	/* XM7チェック、コマンドライン渡し */
	if (FindXM7Wnd()) {
		return 0;
	}

	/* インスタンスを保存、初期化 */
	hAppInstance = hInstance;
	hMainWnd = InitInstance(hInstance, nCmdShow);
	if (!hMainWnd) {
		return FALSE;
	}

	/* アクセラレータをロード */
	hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR));
	ASSERT(hAccel);

	/* メッセージ ループ */
	for (;;) {
		/* メッセージをチェックし、処理 */
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				break;
			}
			if (!TranslateAccelerator(hMainWnd, hAccel, &msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			continue;
		}

		/* ステータス描画＆スリープ */
		if (nErrorCode == 0) {
			DrawStatus();
		}
		if (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
			Sleep(20);
		}
	}

	/* 終了 */
	return msg.wParam;
}

#endif	/* _WIN32 */
