/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API バージョン情報ダイアログ ]
 */

#ifdef _WIN32

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <assert.h>
#include "xm7.h"
#include "w32.h"
#include "w32_res.h"

/*
 *	スタティック ワーク
 */
static RECT AboutURLRect;
static BOOL bAboutURLHit;
static char pszAboutURL[128];

/*
 *	ダイアログ初期化
 */
static void FASTCALL AboutDlgInit(HWND hDlg)
{
	HWND hWnd;
	RECT prect;
	RECT drect;
	POINT point;

	ASSERT(hDlg);

	/* 文字列リソースをロード */
	pszAboutURL[0] = '\0';
	LoadString(hAppInstance, IDS_ABOUTURL, pszAboutURL, sizeof(pszAboutURL));

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

	/* IDC_URLのクライアント座標を取得 */
	hWnd = GetDlgItem(hDlg, IDC_URL);
	ASSERT(hWnd);
	GetWindowRect(hWnd, &prect);

	point.x = prect.left;
	point.y = prect.top;
	ScreenToClient(hDlg, &point);
	AboutURLRect.left = point.x;
	AboutURLRect.top = point.y;

	point.x = prect.right;
	point.y = prect.bottom;
	ScreenToClient(hDlg, &point);
	AboutURLRect.right = point.x;
	AboutURLRect.bottom = point.y;

	/* IDC_URLを削除 */
	DestroyWindow(hWnd);

	/* その他 */
	bAboutURLHit = FALSE;
}

/*
 *	ダイアログ描画
 */
static void AboutDlgPaint(HWND hDlg)
{
	HDC hDC;
	PAINTSTRUCT ps;
	HFONT hFont;
	HFONT hDefFont;
	TEXTMETRIC tm;
	LOGFONT lf;

	ASSERT(hDlg);

	/* DCを取得 */
	hDC = BeginPaint(hDlg, &ps);

	/* GUIフォントのメトリックを得る */
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	hDefFont = (HFONT)SelectObject(hDC, hFont);
	GetTextMetrics(hDC, &tm);
	memset(&lf, 0, sizeof(lf));
	GetTextFace(hDC, LF_FACESIZE, lf.lfFaceName);
	SelectObject(hDC, hDefFont);

	/* アンダーラインを付加したフォントを作成 */
	lf.lfHeight = tm.tmHeight;
	lf.lfWidth = 0;
	lf.lfEscapement = 0;
	lf.lfOrientation = 0;
	lf.lfWeight = FW_DONTCARE;
	lf.lfItalic = tm.tmItalic;
	lf.lfUnderline = TRUE;
	lf.lfStrikeOut = tm.tmStruckOut;
	lf.lfCharSet = tm.tmCharSet;
	lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
	lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lf.lfQuality = DEFAULT_QUALITY;
	lf.lfPitchAndFamily = tm.tmPitchAndFamily;
	hFont = CreateFontIndirect(&lf);
	hDefFont = (HFONT)SelectObject(hDC, hFont);

	/* 描画 */
	if (bAboutURLHit) {
		SetTextColor(hDC, RGB(255, 0, 0));
	}
	else {
		SetTextColor(hDC, RGB(0, 0, 255));
	}
	SetBkColor(hDC, GetSysColor(COLOR_3DFACE));
	DrawText(hDC, pszAboutURL, strlen(pszAboutURL),
		&AboutURLRect, DT_CENTER | DT_SINGLELINE);

	/* フォントを戻す */
	SelectObject(hDC, hDefFont);
	DeleteObject(hFont);

	/* DC解放 */
	EndPaint(hDlg, &ps);
}

/*
 *	ダイアログプロシージャ
 */
static BOOL CALLBACK AboutDlgProc(HWND hDlg, UINT iMsg,
									WPARAM wParam, LPARAM lParam)
{
	POINT point;
	BOOL bFlag;
	HCURSOR hCursor;

	switch (iMsg) {

		/* ダイアログ初期化 */
		case WM_INITDIALOG:
			AboutDlgInit(hDlg);
			return TRUE;

		/* 再描画 */
		case WM_PAINT:
			AboutDlgPaint(hDlg);
			return 0;

		/* コマンド */
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				/* 終了 */
				case IDOK:
				case IDCANCEL:
					EndDialog(hDlg, 0);
					return TRUE;
			}

		/* 領域チェック */
		case WM_NCHITTEST:
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);
			ScreenToClient(hDlg, &point);
			bFlag = PtInRect(&AboutURLRect, point);
			/* フラグが異なったら、更新して再描画 */
			if (bFlag != bAboutURLHit) {
				bAboutURLHit = bFlag;
				InvalidateRect(hDlg, &AboutURLRect, FALSE);
			}
			return FALSE;

		/* カーソル設定 */
		case WM_SETCURSOR:
			if (bAboutURLHit) {
				/* OSのバージョンによってはIDC_HANDは失敗する */
				hCursor = LoadCursor(NULL, MAKEINTRESOURCE(32649));
				if (!hCursor) {
					hCursor = LoadCursor(NULL, IDC_IBEAM);
				}
				SetCursor(hCursor);

				/* マウスが押されれば、URL実行 */
				if ((HIWORD(lParam) == WM_LBUTTONDOWN) ||
					(HIWORD(lParam) == WM_LBUTTONDBLCLK)) {
					ShellExecute(hDlg, NULL, pszAboutURL, NULL, NULL, SW_SHOWNORMAL);
				}
				return TRUE;
			}
	}

	/* それ以外はFALSE */
	return FALSE;
}

/*
 *	バージョン情報
 */
void FASTCALL OnAbout(HWND hWnd)
{
	ASSERT(hWnd);

	/* モーダルダイアログ実行 */
	DialogBox(hAppInstance, MAKEINTRESOURCE(IDD_ABOUTDLG), hWnd, AboutDlgProc);
}

#endif	/* _WIN32 */
