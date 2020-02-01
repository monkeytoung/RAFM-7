/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API メニューコマンド ]
 */

#ifdef _WIN32

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <stdlib.h>
#include <assert.h>
#include <tchar.h>
#include "xm7.h"
#include "fdc.h"
#include "tapelp.h"
#include "tools.h"
#include "w32.h"
#include "w32_bar.h"
#include "w32_draw.h"
#include "w32_snd.h"
#include "w32_sch.h"
#include "w32_sub.h"
#include "w32_cfg.h"
#include "w32_res.h"

/*
 *	スタティック ワーク
 */
static char InitialDir[_MAX_DRIVE + _MAX_PATH];
static char StatePath[_MAX_PATH];
static char DiskTitle[16 + 1];

/*
 *	プロトタイプ宣言
 */
static void FASTCALL OnRefresh(HWND hWnd);

/*-[ タイトル入力ダイアログ ]------------------------------------------------*/

/*
 *	タイトル入力ダイアログ
 *	ダイアログ初期化
 */
static BOOL FASTCALL TitleDlgInit(HWND hDlg)
{
	HWND hWnd;
	RECT prect;
	RECT drect;

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

	/* エディットテキスト処理 */
	hWnd = GetDlgItem(hDlg, IDC_TITLEEDIT);
	ASSERT(hWnd);
	strcpy(DiskTitle, "Default");
	SetWindowText(hWnd, DiskTitle);

	return TRUE;
}

/*
 *	タイトル入力ダイアログ
 *	ダイアログOK
 */
static void FASTCALL TitleDlgOK(HWND hDlg)
{
	HWND hWnd;
	char string[128];

	ASSERT(hDlg);

	/* エディットテキスト処理 */
	hWnd = GetDlgItem(hDlg, IDC_TITLEEDIT);
	ASSERT(hWnd);

	/* 文字列を取得、コピー */
	GetWindowText(hWnd, string, sizeof(string) - 1);
	memset(DiskTitle, 0, sizeof(DiskTitle));
	string[16] = '\0';
	strcpy(DiskTitle, string);
}

/*
 *	タイトル入力ダイアログ
 *	ダイアログプロシージャ
 */
static BOOL CALLBACK TitleDlgProc(HWND hDlg, UINT iMsg,
									WPARAM wParam, LPARAM lParam)
{
	switch (iMsg) {
		/* ダイアログ初期化 */
		case WM_INITDIALOG:
			return TitleDlgInit(hDlg);

		/* コマンド処理 */
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				/* OK */
				case IDOK:
					TitleDlgOK(hDlg);
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

/*-[ 汎用サブ ]-------------------------------------------------------------*/

/*
 *	ファイル選択コモンダイアログ
 */
static BOOL FASTCALL FileSelectSub(BOOL bOpen, UINT uFilterID, char *path, char *defext)
{
	OPENFILENAME ofn;
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME + _MAX_EXT];
	char filter[128];
	int i, j;

	ASSERT((bOpen == TRUE) || (bOpen == FALSE));
	ASSERT(uFilterID > 0);
	ASSERT(path);

	/* データ作成 */
	memset(&ofn, 0, sizeof(ofn));
	memset(path, 0, _MAX_PATH);
	memset(fname, 0, sizeof(fname));
	ofn.lStructSize = 76;	// sizeof(ofn)はV5拡張を含む
	ofn.hwndOwner = hMainWnd;

	LoadString(hAppInstance, uFilterID, filter, sizeof(filter));
	j = strlen(filter);
	for (i=0; i<j; i++) {
		if (filter[i] == '|') {
			filter[i] = '\0';
		}
	}

	ofn.lpstrFilter = filter;
	ofn.lpstrFile = path;
	ofn.nMaxFile = _MAX_PATH;
	ofn.lpstrFileTitle = fname;
	ofn.nMaxFileTitle = sizeof(fname);
	ofn.lpstrDefExt = defext;
	ofn.lpstrInitialDir = InitialDir;

	/* コモンダイアログ実行 */
	if (bOpen) {
		ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
		if (!GetOpenFileName(&ofn)) {
			return FALSE;
		}
	}
	else {
		ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
		if (!GetSaveFileName(&ofn)) {
			return FALSE;
		}
	}

	/* ディレクトリを保存 */
	_splitpath(path, InitialDir, dir, NULL, NULL);
	strcat(InitialDir, dir);

	return TRUE;
}

/*
 *	メニューEnable
 */
static void FASTCALL EnableMenuSub(HMENU hMenu, UINT uID, BOOL bEnable)
{
	ASSERT(hMenu);
	ASSERT(uID > 0);

	if (bEnable) {
		EnableMenuItem(hMenu, uID, MF_BYCOMMAND | MF_ENABLED);
	}
	else {
		EnableMenuItem(hMenu, uID, MF_BYCOMMAND | MF_GRAYED);
	}
}

/*
 *	メニューCheck
 */
static void FASTCALL CheckMenuSub(HMENU hMenu, UINT uID, BOOL bCheck)
{
	ASSERT(hMenu);
	ASSERT(uID > 0);

	if (bCheck) {
		CheckMenuItem(hMenu, uID, MF_BYCOMMAND | MF_CHECKED);
	}
	else {
		CheckMenuItem(hMenu, uID, MF_BYCOMMAND | MF_UNCHECKED);
	}
}

/*-[ ファイルメニュー ]-----------------------------------------------------*/

/*
 *	開く(O)
 */
static void FASTCALL OnOpen(HWND hWnd)
{
	char path[_MAX_PATH];

	ASSERT(hWnd);

	/* ファイル選択サブ */
	if (!FileSelectSub(TRUE, IDS_STATEFILTER, path, NULL)) {
		return;
	}

	/* ステートロード */
	LockVM();
	StopSnd();
	if (!system_load(path)) {
		LoadString(hAppInstance, IDS_STATEERROR, path, sizeof(path));
		MessageBox(hWnd, path, "XM7", MB_ICONSTOP | MB_OK);
		system_reset();
	}
	else {
		strcpy(StatePath, path);
	}
	PlaySnd();
	ResetSch();
	ApplyCfg();
	UnlockVM();

	/* 画面再描画 */
	OnRefresh(hWnd);
}

/*
 *	保存(A)
 */
static void FASTCALL OnSaveAs(HWND hWnd)
{
	char path[_MAX_PATH];

	ASSERT(hWnd);

	/* ファイル選択サブ */
	if (!FileSelectSub(FALSE, IDS_STATEFILTER, path, "XM7")) {
		return;
	}

	/* ステートセーブ */
	LockVM();
	StopSnd();
	if (!system_save(path)) {
		LoadString(hAppInstance, IDS_STATEERROR, path, sizeof(path));
		MessageBox(hWnd, path, "XM7", MB_ICONSTOP | MB_OK);
	}
	else {
		strcpy(StatePath, path);
	}
	PlaySnd();
	ResetSch();
	UnlockVM();
}

/*
 *	名前を付けて保存(S)
 */
static void FASTCALL OnSave(HWND hWnd)
{
	char string[128];

	/* まだ保存されていなければ、名前をつける */
	if (StatePath[0] == '\0') {
		OnSaveAs(hWnd);
		return;
	}

	/* ステートセーブ */
	LockVM();
	StopSnd();
	if (!system_save(StatePath)) {
		LoadString(hAppInstance, IDS_STATEERROR, string, sizeof(string));
		MessageBox(hWnd, string, "XM7", MB_ICONSTOP | MB_OK);
	}
	PlaySnd();
	ResetSch();
	UnlockVM();
}

/*
 *	リセット(R)
 */
static void FASTCALL OnReset(HWND hWnd)
{
	LockVM();
	system_reset();
	UnlockVM();

	/* 再描画 */
	OnRefresh(hWnd);
}

/*
 *	BASICモード(B)
 */
static void FASTCALL OnBasic(void)
{
	LockVM();
	boot_mode = BOOT_BASIC;
	system_reset();
	UnlockVM();
}

/*
 *	DOSモード(D)
 */
static void FASTCALL OnDos(void)
{
	LockVM();
	boot_mode = BOOT_DOS;
	system_reset();
	UnlockVM();
}

/*
 *	終了(X)
 */
static void FASTCALL OnExit(HWND hWnd)
{
	/* ウインドウクローズ */
	PostMessage(hWnd, WM_CLOSE, 0, 0);
}

/*
 *	ファイル(F)メニュー
 */
static BOOL OnFile(HWND hWnd, WORD wID)
{
	ASSERT(hWnd);

	switch (wID) {
		/* オープン */
		case IDM_OPEN:
			OnOpen(hWnd);
			return TRUE;

		/* 保存 */
		case IDM_SAVE:
			OnSave(hWnd);
			return TRUE;

		/* 名前をつけて保存 */
		case IDM_SAVEAS:
			OnSaveAs(hWnd);
			return TRUE;

		/* リセット */
		case IDM_RESET:
			OnReset(hWnd);
			return TRUE;

		/* BASICモード */
		case IDM_BASIC:
			OnBasic();
			return TRUE;

		/* DOSモード */
		case IDM_DOS:
			OnDos();
			return TRUE;

		/* 終了 */
		case IDM_EXIT:
			OnExit(hWnd);
			return TRUE;
	}

	return FALSE;
}

/*
 *	ファイル(F)メニュー更新
 */
static void FASTCALL OnFilePopup(HMENU hMenu)
{
	ASSERT(hMenu);

	CheckMenuSub(hMenu, IDM_BASIC, (boot_mode == BOOT_BASIC));
	CheckMenuSub(hMenu, IDM_DOS, (boot_mode == BOOT_DOS));
}

/*-[ ディスクメニュー ]-----------------------------------------------------*/

/*
 *	ドライブを開く
 */
static void FASTCALL OnDiskOpen(int Drive)
{
	char path[_MAX_PATH];

	ASSERT((Drive == 0) || (Drive == 1));

	/* ファイル選択 */
	if (!FileSelectSub(TRUE, IDS_DISKFILTER, path, NULL)) {
		return;
	}

	/* セット */
	LockVM();
	fdc_setdisk(Drive, path);
	ResetSch();
	UnlockVM();
}

/*
 *	両ドライブを開く
 */
static void FASTCALL OnDiskBoth(void)
{
	char path[_MAX_PATH];

	/* ファイル選択 */
	if (!FileSelectSub(TRUE, IDS_DISKFILTER, path, NULL)) {
		return;
	}

	/* セット */
	LockVM();
	fdc_setdisk(0, path);
	fdc_setdisk(1, NULL);
	if ((fdc_ready[0] != FDC_TYPE_NOTREADY) && (fdc_medias[0] >= 2)) {
		fdc_setdisk(1, path);
		fdc_setmedia(1, 1);
	}
	ResetSch();
	UnlockVM();
}

/*
 *	ディスクイジェクト
 */
static void FASTCALL OnDiskEject(int Drive)
{
	ASSERT((Drive == 0) || (Drive == 1));

	/* イジェクト */
	LockVM();
	fdc_setdisk(Drive, NULL);
	UnlockVM();
}

/*
 *	ディスク一時取り出し
 */
static void FASTCALL OnDiskTemp(int Drive)
{
	ASSERT((Drive == 0) || (Drive == 1));

	/* 書き込み禁止切り替え */
	LockVM();
	if (fdc_teject[Drive]) {
		fdc_teject[Drive] = FALSE;
	}
	else {
		fdc_teject[Drive] = TRUE;
	}
	UnlockVM();
}

/*
 *	ディスク書き込み禁止
 */
static void FASTCALL OnDiskProtect(int Drive)
{
	ASSERT((Drive == 0) || (Drive == 1));

	/* 書き込み禁止切り替え */
	LockVM();
	if (fdc_writep[Drive]) {
		fdc_setwritep(Drive, FALSE);
	}
	else {
		fdc_setwritep(Drive, TRUE);
	}
	ResetSch();
	UnlockVM();
}

/*
 *	ディスク(1)(0)メニュー
 */
static BOOL FASTCALL OnDisk(WORD wID)
{
	switch (wID) {
		/* 開く */
		case IDM_D0OPEN:
			OnDiskOpen(0);
			break;
		case IDM_D1OPEN:
			OnDiskOpen(1);
			break;

		/* 両ドライブで開く */
		case IDM_DBOPEN:
			OnDiskBoth();
			break;

		/* 取り外し */
		case IDM_D0EJECT:
			OnDiskEject(0);
			break;
		case IDM_D1EJECT:
			OnDiskEject(1);
			break;

		/* 一時イジェクト */
		case IDM_D0TEMP:
			OnDiskTemp(0);
			break;
		case IDM_D1TEMP:
			OnDiskTemp(1);
			break;

		/* 書き込み禁止 */
		case IDM_D0WRITE:
			OnDiskProtect(0);
			break;
		case IDM_D1WRITE:
			OnDiskProtect(1);
			break;
	}

	/* メディア交換 */
	if ((wID >= IDM_D0MEDIA00) && (wID <= IDM_D0MEDIA15)) {
		LockVM();
		fdc_setmedia(0, wID - IDM_D0MEDIA00);
		ResetSch();
		UnlockVM();
	}
	if ((wID >= IDM_D1MEDIA00) && (wID <= IDM_D1MEDIA15)) {
		LockVM();
		fdc_setmedia(1, wID - IDM_D1MEDIA00);
		ResetSch();
		UnlockVM();
	}

	return FALSE;
}

/*
 *	ディスク(1)(0)メニュー更新
 */
static void FASTCALL OnDiskPopup(HMENU hMenu, int Drive)
{
	MENUITEMINFO mii;
	char string[128];
	int offset;
	int i;

	ASSERT(hMenu);
	ASSERT((Drive == 0) || (Drive == 1));

	/* メニューすべて削除 */
	while (GetMenuItemCount(hMenu) > 0) {
		DeleteMenu(hMenu, 0, MF_BYPOSITION);
	}

	/* オフセット確定 */
	if (Drive == 0) {
		offset = 0;
	}
	else {
		offset = IDM_D1OPEN - IDM_D0OPEN;
	}

	/* メニュー構造体初期化 */
	memset(&mii, 0, sizeof(mii));
	mii.cbSize = 44;	/* sizeof(mii)はWINVER>=0x0500向け */
	mii.fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID;
	mii.fType = MFT_STRING;
	mii.fState = MFS_ENABLED;

	/* オープンと、両オープン */
	mii.wID = IDM_D0OPEN + offset;
	LoadString(hAppInstance, IDS_DISKOPEN, string, sizeof(string));
	mii.dwTypeData = string;
	mii.cch = strlen(string);
	InsertMenuItem(hMenu, 0, TRUE, &mii);
	mii.wID = IDM_DBOPEN;
	LoadString(hAppInstance, IDS_DISKBOTH, string, sizeof(string));
	mii.cch = strlen(string);
	InsertMenuItem(hMenu, 1, TRUE, &mii);

	/* ディスクが挿入されていなければ、ここまで */
	if (fdc_ready[Drive] == FDC_TYPE_NOTREADY) {
		return;
	}

	/* イジェクト */
	mii.wID = IDM_D0EJECT + offset;
	LoadString(hAppInstance, IDS_DISKEJECT, string, sizeof(string));
	mii.cch = strlen(string);
	InsertMenuItem(hMenu, 2, TRUE, &mii);

	/* セパレータ挿入 */
	mii.fType = MFT_SEPARATOR;
	InsertMenuItem(hMenu, 3, TRUE, &mii);

	/* 一時取り出し */
	mii.wID = IDM_D0TEMP + offset;
	LoadString(hAppInstance, IDS_DISKTEMP, string, sizeof(string));
	if (fdc_teject[Drive]) {
		mii.fState = MFS_CHECKED | MFS_ENABLED;
	}
	else {
		mii.fState = MFS_ENABLED;
	}
	mii.fType = MFT_STRING;
	mii.cch = strlen(string);
	InsertMenuItem(hMenu, 4, TRUE, &mii);

	/* ライトプロテクト */
	mii.wID = IDM_D0WRITE + offset;
	LoadString(hAppInstance, IDS_DISKPROTECT, string, sizeof(string));
	if (fdc_fwritep[Drive]) {
		mii.fState = MFS_GRAYED;
	}
	else {
		if (fdc_writep[Drive]) {
			mii.fState = MFS_CHECKED | MFS_ENABLED;
		}
		else {
			mii.fState = MFS_ENABLED;
		}
	}
	mii.fType = MFT_STRING;
	mii.cch = strlen(string);
	InsertMenuItem(hMenu, 5, TRUE, &mii);

	/* セパレータ挿入 */
	mii.fType = MFT_SEPARATOR;
	InsertMenuItem(hMenu, 6, TRUE, &mii);

	/* 2Dなら特殊処理 */
	if (fdc_ready[Drive] == FDC_TYPE_2D) {
		mii.wID = IDM_D0MEDIA00 + offset;
		mii.fState = MFS_CHECKED | MFS_ENABLED;
		mii.fType = MFT_STRING;
		LoadString(hAppInstance, IDS_DISK2D, string, sizeof(string));
		mii.cch = strlen(string);
		InsertMenuItem(hMenu, 7, TRUE, &mii);
		return;
	}

	/* メディアを回す */
	for (i=0; i<fdc_medias[Drive]; i++) {
		mii.wID = IDM_D0MEDIA00 + offset + i;
		if (fdc_media[Drive] == i) {
			mii.fState = MFS_CHECKED | MFS_ENABLED;
		}
		else {
			mii.fState = MFS_ENABLED;
		}
		mii.fType = MFT_STRING;
		strcpy(string, fdc_name[Drive][i]);
		mii.cch = strlen(string);
		InsertMenuItem(hMenu, 7 + i, TRUE, &mii);
	}
}

/*-[ テープメニュー ]-------------------------------------------------------*/

/*
 *  テープオープン
 */
static void FASTCALL OnTapeOpen(void)
{
	char path[_MAX_PATH];

	/* ファイル選択 */
	if (!FileSelectSub(TRUE, IDS_TAPEFILTER, path, NULL)) {
		return;
	}

	/* セット */
	LockVM();
	tape_setfile(path);
	ResetSch();
	UnlockVM();
}

/*
 *	テープイジェクト
 */
static void FASTCALL OnTapeEject(void)
{
	/* イジェクト */
	LockVM();
	tape_setfile(NULL);
	UnlockVM();
}

/*
 *	巻き戻し
 */
static void FASTCALL OnRew(void)
{
	HCURSOR hCursor;

	/* 巻き戻し */
	hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
	LockVM();
	StopSnd();

	tape_rew();

	PlaySnd();
	ResetSch();
	UnlockVM();
	SetCursor(hCursor);
}

/*
 *	早送り
 */
static void FASTCALL OnFF(void)
{
	HCURSOR hCursor;

	/* 巻き戻し */
	hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
	LockVM();
	StopSnd();

	tape_ff();

	PlaySnd();
	ResetSch();
	UnlockVM();
	SetCursor(hCursor);
}

/*
 *	録音
 */
static void FASTCALL OnRec(void)
{
	/* 録音 */
	LockVM();
	if (tape_rec) {
		tape_setrec(FALSE);
	}
	else {
		tape_setrec(TRUE);
	}
	UnlockVM();
}

/*
 *	テープ(A)メニュー
 */
static BOOL FASTCALL OnTape(WORD wID)
{
	switch (wID) {
		/* 開く */
		case IDM_TOPEN:
			OnTapeOpen();
			return TRUE;

		/* 取り外す */
		case IDM_TEJECT:
			OnTapeEject();
			return TRUE;

		/* 巻き戻し */
		case IDM_REW:
			OnRew();
			return TRUE;

		/* 早送り */
		case IDM_FF:
			OnFF();
			return TRUE;

		/* 録音 */
		case IDM_REC:
			OnRec();
			return TRUE;
	}

	return FALSE;
}

/*
 *	テープ(A)メニュー更新
 */
static void FASTCALL OnTapePopup(HMENU hMenu)
{
	MENUITEMINFO mii;
	char string[128];

	ASSERT(hMenu);

	/* メニューすべて削除 */
	while (GetMenuItemCount(hMenu) > 0) {
		DeleteMenu(hMenu, 0, MF_BYPOSITION);
	}

	/* メニュー構造体初期化 */
	memset(&mii, 0, sizeof(mii));
	mii.cbSize = 44;	// sizeof(mii)はWINVER>=0x0500向け
	mii.fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID;
	mii.fType = MFT_STRING;
	mii.fState = MFS_ENABLED;

	/* オープン */
	mii.wID = IDM_TOPEN;
	LoadString(hAppInstance, IDS_TAPEOPEN, string, sizeof(string));
	mii.dwTypeData = string;
	mii.cch = strlen(string);
	InsertMenuItem(hMenu, 0, TRUE, &mii);

	/* テープがセットされていなければ、ここまで */
	if (tape_fileh == -1) {
		return;
	}

	/* イジェクト */
	mii.wID = IDM_TEJECT;
	LoadString(hAppInstance, IDS_TAPEEJECT, string, sizeof(string));
	mii.cch = strlen(string);
	InsertMenuItem(hMenu, 1, TRUE, &mii);

	/* セパレータ挿入 */
	mii.fType = MFT_SEPARATOR;
	InsertMenuItem(hMenu, 2, TRUE, &mii);
	mii.fType = MFT_STRING;

	/* 巻き戻し */
	mii.wID = IDM_REW;
	LoadString(hAppInstance, IDS_TAPEREW, string, sizeof(string));
	mii.cch = strlen(string);
	InsertMenuItem(hMenu, 3, TRUE, &mii);

	/* 早送り */
	mii.wID = IDM_FF;
	LoadString(hAppInstance, IDS_TAPEFF, string, sizeof(string));
	mii.cch = strlen(string);
	InsertMenuItem(hMenu, 4, TRUE, &mii);

	/* セパレータ挿入 */
	mii.fType = MFT_SEPARATOR;
	InsertMenuItem(hMenu, 5, TRUE, &mii);
	mii.fType = MFT_STRING;

	/* 録音 */
	mii.wID = IDM_REC;
	LoadString(hAppInstance, IDS_TAPEREC, string, sizeof(string));
	mii.cch = strlen(string);
	if (tape_writep) {
		mii.fState = MFS_GRAYED;
	}
	else {
		if (tape_rec) {
			mii.fState = MFS_CHECKED | MFS_ENABLED;
		}
		else {
			mii.fState = MFS_ENABLED;
		}
	}
	InsertMenuItem(hMenu, 6, TRUE, &mii);
}

/*-[ 表示メニュー ]---------------------------------------------------------*/

/*
 *	フロッピーディスクコントローラ(F)
 */
static void FASTCALL OnFDC(void)
{
	ASSERT(hDrawWnd);

	/* ウインドウが存在すれば、クローズ指示を出す */
	if (hSubWnd[SWND_FDC]) {
		PostMessage(hSubWnd[SWND_FDC], WM_CLOSE, 0, 0);
		return;
	}

	/* ウインドウ作成 */
	hSubWnd[SWND_FDC] = CreateFDC(hDrawWnd, SWND_FDC);
}

/*
 *	FM音源レジスタ(O)
 */
static void FASTCALL OnOPNReg(void)
{
	ASSERT(hDrawWnd);

	/* ウインドウが存在すれば、クローズ指示を出す */
	if (hSubWnd[SWND_OPNREG]) {
		PostMessage(hSubWnd[SWND_OPNREG], WM_CLOSE, 0, 0);
		return;
	}

	/* ウインドウ作成 */
	hSubWnd[SWND_OPNREG] = CreateOPNReg(hDrawWnd, SWND_OPNREG);
}

/*
 *	FM音源ディスプレイ(D)
 */
static void FASTCALL OnOPNDisp(void)
{
	ASSERT(hDrawWnd);

	/* ウインドウが存在すれば、クローズ指示を出す */
	if (hSubWnd[SWND_OPNDISP]) {
		PostMessage(hSubWnd[SWND_OPNDISP], WM_CLOSE, 0, 0);
		return;
	}

	/* ウインドウ作成 */
	hSubWnd[SWND_OPNDISP] = CreateOPNDisp(hDrawWnd, SWND_OPNDISP);
}

/*
 *	サブCPUコントロール(C)
 */
static void FASTCALL OnSubCtrl(void)
{
	ASSERT(hDrawWnd);

	/* ウインドウが存在すれば、クローズ指示を出す */
	if (hSubWnd[SWND_SUBCTRL]) {
		PostMessage(hSubWnd[SWND_SUBCTRL], WM_CLOSE, 0, 0);
		return;
	}

	/* ウインドウ作成 */
	hSubWnd[SWND_SUBCTRL] = CreateSubCtrl(hDrawWnd, SWND_SUBCTRL);
}

/*
 *	キーボード(K)
 */
static void FASTCALL OnKeyboard(void)
{
	ASSERT(hDrawWnd);

	/* ウインドウが存在すれば、クローズ指示を出す */
	if (hSubWnd[SWND_KEYBOARD]) {
		PostMessage(hSubWnd[SWND_KEYBOARD], WM_CLOSE, 0, 0);
		return;
	}

	/* ウインドウ作成 */
	hSubWnd[SWND_KEYBOARD] = CreateKeyboard(hDrawWnd, SWND_KEYBOARD);
}

/*
 *	メモリ管理(M)
 */
static void FASTCALL OnMMR(void)
{
	ASSERT(hDrawWnd);

	/* ウインドウが存在すれば、クローズ指示を出す */
	if (hSubWnd[SWND_MMR]) {
		PostMessage(hSubWnd[SWND_MMR], WM_CLOSE, 0, 0);
		return;
	}

	/* ウインドウ作成 */
	hSubWnd[SWND_MMR] = CreateMMR(hDrawWnd, SWND_MMR);
}

/*
 *	ステータスバー(S)
 */
static void FASTCALL OnStatus(HWND hWnd)
{
	RECT rect;

	/* ステータスバーが有効でなければ、何もしない */
	if (!hStatusBar) {
		return;
	}

	if (IsWindowVisible(hStatusBar)) {
		/* 消去 */
		ShowWindow(hStatusBar, SW_HIDE);
	}
	else {
		/* 表示 */
		ShowWindow(hStatusBar, SW_SHOW);
	}

	/* フレームウインドウのサイズを補正 */
	GetClientRect(hWnd, &rect);
	PostMessage(hWnd, WM_SIZE, 0, MAKELPARAM(rect.right, rect.bottom));
}

/*
 *	最新の情報に更新(R)
 */
static void FASTCALL OnRefresh(HWND hWnd)
{
	int i;

	ASSERT(hWnd);
	ASSERT(hDrawWnd);

	/* 逆アセンブルウインドウ　アドレス更新 */
	if (hSubWnd[SWND_DISASM_MAIN]) {
		AddrDisAsm(TRUE, maincpu.pc);
	}
	if (hSubWnd[SWND_DISASM_SUB]) {
		AddrDisAsm(FALSE, subcpu.pc);
	}

	/* メインウインドウ */
	InvalidateRect(hWnd, NULL, FALSE);
	InvalidateRect(hDrawWnd, NULL, FALSE);

	/* サブウインドウ */
	for (i=0; i<SWND_MAXNUM; i++) {
		if (hSubWnd[i]) {
			InvalidateRect(hSubWnd[i], NULL, FALSE);
		}
	}
}

/*
 *	実行に同期(Y)
 */
static void FASTCALL OnSync(void)
{
	bSync = (!bSync);
}

/*
 *	フルスクリーン(U)
 */
static void FASTCALL OnFullScreen(HWND hWnd)
{
	BOOL bRun;

	ASSERT(hWnd);

	/* VMをロック、ストップ */
	LockVM();
	bRun = run_flag;
	run_flag = FALSE;
	StopSnd();

	/* モード切り替え */
	if (bFullScreen) {
		ModeDraw(hWnd, FALSE);
	}
	else {
		ModeDraw(hWnd, TRUE);
	}

	/* VMをアンロック */
	run_flag = bRun;
	ResetSch();
	UnlockVM();
	PlaySnd();
}

/*
 *	表示(V)メニュー
 */
static BOOL FASTCALL OnView(HWND hWnd, WORD wID)
{
	ASSERT(hWnd);

	switch (wID) {
		/* FDC */
		case IDM_FDC:
			OnFDC();
			return TRUE;

		/* OPNレジスタ */
		case IDM_OPNREG:
			OnOPNReg();
			return TRUE;

		/* OPNディスプレイ */
		case IDM_OPNDISP:
			OnOPNDisp();
			return TRUE;

		/* サブCPUコントロール */
		case IDM_SUBCTRL:
			OnSubCtrl();
			return TRUE;

		/* キーボード */
		case IDM_KEYBOARD:
			OnKeyboard();
			return TRUE;

		/* メモリ管理 */
		case IDM_MMR:
			OnMMR();
			return TRUE;

		/* ステータスバー */
		case IDM_STATUS:
			OnStatus(hWnd);
			return TRUE;

		/* 最新の情報に更新 */
		case IDM_REFRESH:
			OnRefresh(hWnd);
			return TRUE;

		/* 実行に同期 */
		case IDM_SYNC:
			OnSync();
			return TRUE;

		/* フルスクリーン */
		case IDM_FULLSCREEN:
			OnFullScreen(hWnd);
			return TRUE;
	}

	return FALSE;
}

/*
 *	表示(V)メニュー更新
 */
static void FASTCALL OnViewPopup(HMENU hMenu)
{
	/* サブウインドウ群 */
	CheckMenuSub(hMenu, IDM_FDC, (BOOL)hSubWnd[SWND_FDC]);
	CheckMenuSub(hMenu, IDM_OPNREG, (BOOL)hSubWnd[SWND_OPNREG]);
	CheckMenuSub(hMenu, IDM_OPNDISP, (BOOL)hSubWnd[SWND_OPNDISP]);
	CheckMenuSub(hMenu, IDM_SUBCTRL, (BOOL)hSubWnd[SWND_SUBCTRL]);
	CheckMenuSub(hMenu, IDM_KEYBOARD, (BOOL)hSubWnd[SWND_KEYBOARD]);
	CheckMenuSub(hMenu, IDM_MMR, (BOOL)hSubWnd[SWND_MMR]);

	/* その他 */
	if (hStatusBar) {
		CheckMenuSub(hMenu, IDM_STATUS, IsWindowVisible(hStatusBar));
	}
	else {
		CheckMenuSub(hMenu, IDM_STATUS, FALSE);
	}
	CheckMenuSub(hMenu, IDM_SYNC, bSync);
	CheckMenuSub(hMenu, IDM_FULLSCREEN, bFullScreen);
}

/*-[ デバッグメニュー ]-----------------------------------------------------*/

/*
 *	実行(X)
 */
static void FASTCALL OnExec(void)
{
	/* 既に実行中なら、何もしない */
	if (run_flag) {
		return;
	}

	/* スタート */
	LockVM();
	stopreq_flag = FALSE;
	run_flag = TRUE;
	UnlockVM();
}

/*
 *	停止(B)
 */
static void FASTCALL OnBreak(void)
{
	/* 既に停止状態なら、何もしない */
	if (!run_flag) {
		return;
	}

	/* 停止 */
	LockVM();
	stopreq_flag = TRUE;
	UnlockVM();
}

/*
 *	トレース(T)
 */
static void FASTCALL OnTrace(HWND hWnd)
{
	ASSERT(hWnd);

	/* 停止状態でなければ、リターン */
	if (run_flag) {
		return;
	}

	/* 実行 */
	schedule_trace();

	/* 表示更新 */
	OnRefresh(hWnd);
}

/*
 *	ブレークポイント(B)
 */
static void FASTCALL OnBreakPoint(void)
{
	ASSERT(hDrawWnd);

	/* ウインドウが存在すれば、クローズ指示を出す */
	if (hSubWnd[SWND_BREAKPOINT]) {
		PostMessage(hSubWnd[SWND_BREAKPOINT], WM_CLOSE, 0, 0);
		return;
	}

	/* ウインドウ作成 */
	hSubWnd[SWND_BREAKPOINT] = CreateBreakPoint(hDrawWnd, SWND_BREAKPOINT);
}

/*
 *	スケジューラ(S)
 */
static void FASTCALL OnScheduler(void)
{
	ASSERT(hDrawWnd);

	/* ウインドウが存在すれば、クローズ指示を出す */
	if (hSubWnd[SWND_SCHEDULER]) {
		PostMessage(hSubWnd[SWND_SCHEDULER], WM_CLOSE, 0, 0);
		return;
	}

	/* ウインドウ作成 */
	hSubWnd[SWND_SCHEDULER] = CreateScheduler(hDrawWnd, SWND_SCHEDULER);
}

/*
 *	CPUレジスタ(C)
 */
static void FASTCALL OnCPURegister(BOOL bMain)
{
	int index;

	ASSERT((bMain == TRUE) || (bMain == FALSE));
	ASSERT(hDrawWnd);

	/* インデックス決定 */
	index = SWND_CPUREG_MAIN;
	if (!bMain) {
		index++;
	}

	/* ウインドウが存在すれば、クローズ指示を出す */
	if (hSubWnd[index]) {
		PostMessage(hSubWnd[index], WM_CLOSE, 0, 0);
		return;
	}

	/* ウインドウ作成 */
	hSubWnd[index] = CreateCPURegister(hDrawWnd, bMain, index);
}

/*
 *	逆アセンブル(D)
 */
static void FASTCALL OnDisAsm(BOOL bMain)
{
	int index;

	ASSERT((bMain == TRUE) || (bMain == FALSE));
	ASSERT(hDrawWnd);

	/* インデックス決定 */
	index = SWND_DISASM_MAIN;
	if (!bMain) {
		index++;
	}

	/* ウインドウが存在すれば、クローズ指示を出す */
	if (hSubWnd[index]) {
		PostMessage(hSubWnd[index], WM_CLOSE, 0, 0);
		return;
	}

	/* ウインドウ作成 */
	hSubWnd[index] = CreateDisAsm(hDrawWnd, bMain, index);
}

/*
 *	メモリダンプ(M)
 */
static void FASTCALL OnMemory(BOOL bMain)
{
	int index;

	ASSERT((bMain == TRUE) || (bMain == FALSE));
	ASSERT(hDrawWnd);

	/* インデックス決定 */
	index = SWND_MEMORY_MAIN;
	if (!bMain) {
		index++;
	}

	/* ウインドウが存在すれば、クローズ指示を出す */
	if (hSubWnd[index]) {
		PostMessage(hSubWnd[index], WM_CLOSE, 0, 0);
		return;
	}

	/* ウインドウ作成 */
	hSubWnd[index] = CreateMemory(hDrawWnd, bMain, index);
}

/*
 *	デバッグ(D)メニュー
 */
static BOOL FASTCALL OnDebug(HWND hWnd, WORD wID)
{
	ASSERT(hWnd);

	switch (wID) {
		/* 実行 */
		case IDM_EXEC:
			OnExec();
			return TRUE;

		/* ブレーク */
		case IDM_BREAK:
			OnBreak();
			return TRUE;

		/* トレース */
		case IDM_TRACE:
			OnTrace(hWnd);
			return TRUE;

		/* ブレークポイント */
		case IDM_BREAKPOINT:
			OnBreakPoint();
			return TRUE;

		/* スケジューラ */
		case IDM_SCHEDULER:
			OnScheduler();
			return TRUE;

		/* CPUレジスタ(メイン) */
		case IDM_CPU_MAIN:
			OnCPURegister(TRUE);
			return TRUE;

		/* CPUレジスタ(サブ) */
		case IDM_CPU_SUB:
			OnCPURegister(FALSE);
			return TRUE;

		/* 逆アセンブル(メイン) */
		case IDM_DISASM_MAIN:
			OnDisAsm(TRUE);
			return TRUE;

		/* 逆アセンブル(サブ) */
		case IDM_DISASM_SUB:
			OnDisAsm(FALSE);
			return TRUE;

		/* メモリダンプ(メイン) */
		case IDM_MEMORY_MAIN:
			OnMemory(TRUE);
			return TRUE;

		/* メモリダンプ(サブ) */
		case IDM_MEMORY_SUB:
			OnMemory(FALSE);
			return TRUE;
	}

	return FALSE;
}

/*
 *	デバッグ(D)メニュー更新
 */
static void FASTCALL OnDebugPopup(HMENU hMenu)
{
	ASSERT(hMenu);

	/* 実行制御 */
	EnableMenuSub(hMenu, IDM_EXEC, !run_flag);
	EnableMenuSub(hMenu, IDM_BREAK, run_flag);
	EnableMenuSub(hMenu, IDM_TRACE, !run_flag);

	/* サブウインドウ群 */
	CheckMenuSub(hMenu, IDM_BREAKPOINT, (BOOL)hSubWnd[SWND_BREAKPOINT]);
	CheckMenuSub(hMenu, IDM_SCHEDULER, (BOOL)hSubWnd[SWND_SCHEDULER]);
	CheckMenuSub(hMenu, IDM_CPU_MAIN, (BOOL)hSubWnd[SWND_CPUREG_MAIN]);
	CheckMenuSub(hMenu, IDM_CPU_SUB, (BOOL)hSubWnd[SWND_CPUREG_SUB]);
	CheckMenuSub(hMenu, IDM_DISASM_MAIN, (BOOL)hSubWnd[SWND_DISASM_MAIN]);
	CheckMenuSub(hMenu, IDM_DISASM_SUB, (BOOL)hSubWnd[SWND_DISASM_SUB]);
	CheckMenuSub(hMenu, IDM_MEMORY_MAIN, (BOOL)hSubWnd[SWND_MEMORY_MAIN]);
	CheckMenuSub(hMenu, IDM_MEMORY_SUB, (BOOL)hSubWnd[SWND_MEMORY_SUB]);
}

/*-[ ツールメニュー ]-------------------------------------------------------*/

/*
 *	画面キャプチャ(C)
 */
static void FASTCALL OnGrpCapture(void)
{
	char path[_MAX_PATH];

	/* ファイル選択 */
	if (!FileSelectSub(FALSE, IDS_GRPCAPFILTER, path, "BMP")) {
		return;
	}

	/* キャプチャ */
	LockVM();
	StopSnd();

	capture_to_bmp(path);

	PlaySnd();
	ResetSch();
	UnlockVM();
}

/*
 *	WAVキャプチャ(W)
 */
static void FASTCALL OnWavCapture(HWND hWnd)
{
	char path[_MAX_PATH];

	ASSERT(hWnd);

	/* 既にキャプチャ中なら、クローズ */
	if (hWavCapture >= 0) {
		LockVM();
		CloseCaptureSnd();
		UnlockVM();
		return;
	}

	/* ファイル選択 */
	if (!FileSelectSub(FALSE, IDS_WAVCAPFILTER, path, "WAV")) {
		return;
	}

	/* キャプチャ */
	LockVM();
	OpenCaptureSnd(path);
	UnlockVM();

	/* 条件判定 */
	if (hWavCapture < 0) {
		LockVM();
		StopSnd();

		LoadString(hAppInstance, IDS_WAVCAPERROR, path, sizeof(path));
		MessageBox(hWnd, path, "XM7", MB_ICONSTOP | MB_OK);

		PlaySnd();
		ResetSch();
		UnlockVM();
	}
}

/*
 *	新規ディスク作成(D)
 */
static void FASTCALL OnNewDisk(HWND hWnd)
{
	char path[_MAX_PATH];
	int ret;

	ASSERT(hWnd);

	/* タイトル入力 */
	ret = DialogBox(hAppInstance, MAKEINTRESOURCE(IDD_TITLEDLG),
						hWnd, TitleDlgProc);
	if (ret != IDOK) {
		return;
	}

	/* ファイル選択 */
	if (!FileSelectSub(FALSE, IDS_NEWDISKFILTER, path, "D77")) {
		return;
	}

	/* 作成 */
	LockVM();
	StopSnd();

	if (make_new_d77(path, DiskTitle)) {
		LoadString(hAppInstance, IDS_NEWDISKOK, path, sizeof(path));
		MessageBox(hWnd, path, "XM7", MB_OK);
	}

	PlaySnd();
	ResetSch();
	UnlockVM();
}

/*
 *	新規テープ作成(T)
 */
static void FASTCALL OnNewTape(HWND hWnd)
{
	char path[_MAX_PATH];

	ASSERT(hWnd);

	/* ファイル選択 */
	if (!FileSelectSub(FALSE, IDS_TAPEFILTER, path, "T77")) {
		return;
	}

	/* 作成 */
	LockVM();
	StopSnd();

	if (make_new_t77(path)) {
		LoadString(hAppInstance, IDS_NEWTAPEOK, path, sizeof(path));
		MessageBox(hWnd, path, "XM7", MB_OK);
	}

	PlaySnd();
	ResetSch();
	UnlockVM();
}

/*
 *	VFD→D77変換(V)
 */
static void FASTCALL OnVFD2D77(HWND hWnd)
{
	char src[_MAX_PATH];
	char dst[_MAX_PATH];
	int ret;

	ASSERT(hWnd);

	/* ファイル選択 */
	if (!FileSelectSub(TRUE, IDS_VFDFILTER, src, "VFD")) {
		return;
	}

	/* タイトル入力 */
	ret = DialogBox(hAppInstance, MAKEINTRESOURCE(IDD_TITLEDLG),
						hWnd, TitleDlgProc);
	if (ret != IDOK) {
		return;
	}

	/* ファイル選択 */
	if (!FileSelectSub(FALSE, IDS_NEWDISKFILTER, dst, "D77")) {
		return;
	}

	/* 作成 */
	LockVM();
	StopSnd();

	if (conv_vfd_to_d77(src, dst, DiskTitle)) {
		LoadString(hAppInstance, IDS_CONVERTOK, src, sizeof(src));
		MessageBox(hWnd, src, "XM7", MB_OK);
	}

	PlaySnd();
	ResetSch();
	UnlockVM();
}

/*
 *	2D→D77変換(2)
 */
static void FASTCALL On2D2D77(HWND hWnd)
{
	char src[_MAX_PATH];
	char dst[_MAX_PATH];
	int ret;

	ASSERT(hWnd);

	/* ファイル選択 */
	if (!FileSelectSub(TRUE, IDS_2DFILTER, src, "2D")) {
		return;
	}

	/* タイトル入力 */
	ret = DialogBox(hAppInstance, MAKEINTRESOURCE(IDD_TITLEDLG),
						hWnd, TitleDlgProc);
	if (ret != IDOK) {
		return;
	}

	/* ファイル選択 */
	if (!FileSelectSub(FALSE, IDS_NEWDISKFILTER, dst, "D77")) {
		return;
	}

	/* 作成 */
	LockVM();
	StopSnd();

	if (conv_2d_to_d77(src, dst, DiskTitle)) {
		LoadString(hAppInstance, IDS_CONVERTOK, src, sizeof(src));
		MessageBox(hWnd, src, "XM7", MB_OK);
	}

	PlaySnd();
	ResetSch();
	UnlockVM();
}

/*
 *	VTP→T77変換(P)
 */
static void FASTCALL OnVTP2T77(HWND hWnd)
{
	char src[_MAX_PATH];
	char dst[_MAX_PATH];

	ASSERT(hWnd);

	/* ファイル選択 */
	if (!FileSelectSub(TRUE, IDS_VTPFILTER, src, "VTP")) {
		return;
	}

	/* ファイル選択 */
	if (!FileSelectSub(FALSE, IDS_TAPEFILTER, dst, "T77")) {
		return;
	}

	/* 作成 */
	LockVM();
	StopSnd();

	if (conv_vtp_to_t77(src, dst)) {
		LoadString(hAppInstance, IDS_CONVERTOK, src, sizeof(src));
		MessageBox(hWnd, src, "XM7", MB_OK);
	}

	PlaySnd();
	ResetSch();
	UnlockVM();
}

/*
 *	ツール(T)メニュー
 */
static BOOL FASTCALL OnTool(HWND hWnd, WORD wID)
{
	ASSERT(hWnd);

	switch (wID) {
		/* 設定 */
		case IDM_CONFIG:
			OnConfig(hWnd);
			return TRUE;

		/* 画面キャプチャ */
		case IDM_GRPCAP:
			OnGrpCapture();
			return TRUE;

		/* WAVキャプチャ */
		case IDM_WAVCAP:
			OnWavCapture(hWnd);
			return TRUE;

		/* 新規ディスク */
		case IDM_NEWDISK:
			OnNewDisk(hWnd);
			return TRUE;

		/* 新規テープ */
		case IDM_NEWTAPE:
			OnNewTape(hWnd);
			return TRUE;

		/* VFD→D77 */
		case IDM_VFD2D77:
			OnVFD2D77(hWnd);
			return TRUE;

		/* 2D→D77 */
		case IDM_2D2D77:
			On2D2D77(hWnd);
			return TRUE;

		/* VTP→T77 */
		case IDM_VTP2T77:
			OnVTP2T77(hWnd);
			return TRUE;
	}

	return FALSE;
}

/*
 *	ツール(T)メニュー更新
 */
static void FASTCALL OnToolPopup(HMENU hMenu)
{
	ASSERT(hMenu);

	/* WAVキャプチャのみ。ハンドルが正でオープン中 */
	CheckMenuSub(hMenu, IDM_WAVCAP, (hWavCapture >= 0));
}

/*-[ ウィンドウメニュー ]---------------------------------------------------*/

/*
 *	重ねて表示(C)
 */
static void FASTCALL OnCascade(void)
{
	/* 重ねて表示 */
	ASSERT(hDrawWnd);
	CascadeWindows(hDrawWnd, 0, NULL, 0, NULL);
}

/*
 *	並べて表示(T)
 */
static void FASTCALL OnTile(void)
{
	/* 並べて表示 */
	ASSERT(hDrawWnd);
	TileWindows(hDrawWnd, MDITILE_VERTICAL, NULL, 0, NULL);
}

/*
 *	全てアイコン化(I)
 */
static void FASTCALL OnIconic(void)
{
	int i;

	/* 全てアイコン化 */
	for (i=0; i<SWND_MAXNUM; i++) {
		if (hSubWnd[i]) {
			ShowWindow(hSubWnd[i], SW_MINIMIZE);
		}
	}
}

/*
 *	アイコンの整列(A)
 */
static void FASTCALL OnArrangeIcon(void)
{
	/* アイコンの整列 */
	ASSERT(hDrawWnd);
	ArrangeIconicWindows(hDrawWnd);
}

/*
 *	ウィンドウ(W)メニュー
 */
static BOOL FASTCALL OnWindow(WORD wID)
{
	int i;

	switch (wID) {
		/* 重ねて表示 */
		case IDM_CASCADE:
			OnCascade();
			return TRUE;

		/* 並べて表示 */
		case IDM_TILE:
			OnTile();
			return TRUE;

		/* 全てアイコン化 */
		case IDM_ICONIC:
			OnIconic();
			return TRUE;

		/* アイコンの整列 */
		case IDM_ARRANGEICON:
			OnArrangeIcon();
			return TRUE;
	}

	/* ウィンドウ選択か */
	if ((wID >= IDM_SWND00) && (wID <= IDM_SWND15)) {
		for (i=0; i<SWND_MAXNUM; i++) {
			if (hSubWnd[i] == NULL) {
				continue;
			}

			/* カウントダウンか、OK */
			if (wID == IDM_SWND00) {
				ShowWindow(hSubWnd[i], SW_RESTORE);
				SetWindowPos(hSubWnd[i], HWND_TOP, 0, 0, 0, 0,
					SWP_NOMOVE | SWP_NOSIZE);
				break;
			}
			else {
				wID--;
			}
		}
	}

	return FALSE;
}

/*
 *	ウィンドウ(W)メニュー更新
 */
static void FASTCALL OnWindowPopup(HMENU hMenu)
{
	int i;
	BOOL flag;
	MENUITEMINFO mii;
	UINT nID;
	int count;
	char string[128];

	ASSERT(hMenu);

	/* 先頭の４つを残して削除 */
	while (GetMenuItemCount(hMenu) > 4) {
		DeleteMenu(hMenu, 4, MF_BYPOSITION);
	}

	/* 有効なサブウインドウがなければ、そのままリターン */
	flag = FALSE;
	for (i=0; i<SWND_MAXNUM; i++) {
		if (hSubWnd[i] != NULL) {
			flag = TRUE;
		}
	}
	if (!flag) {
		return;
	}

	/* メニュー構造体初期化 */
	memset(&mii, 0, sizeof(mii));
	mii.cbSize = 44;	// sizeof(mii)はWINVER>=0x0500向け
	mii.fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID;
	mii.fType = MFT_STRING;
	mii.fState = MFS_ENABLED;

	/* セパレータ挿入 */
	mii.fType = MFT_SEPARATOR;
	InsertMenuItem(hMenu, 4, TRUE, &mii);
	mii.fType = MFT_STRING;

	/* ウインドウタイトルをセット */
	count = 0;
	nID = IDM_SWND00;
	for (i=0; i<SWND_MAXNUM; i++) {
		if (hSubWnd[i] != NULL) {
			/* メニュー挿入 */
			mii.wID = nID;
			memset(string, 0, sizeof(string));
			GetWindowText(hSubWnd[i], string, sizeof(string) - 1);
			mii.dwTypeData = string;
			mii.cch = strlen(string);
			InsertMenuItem(hMenu, count + 5, TRUE, &mii);

			/* 次へ */
			nID++;
			count++;
		}
	}
}

/*-[ ヘルプメニュー ]-------------------------------------------------------*/

/*
 *	ヘルプ(H)メニュー
 */
static BOOL FASTCALL OnHelp(HWND hWnd, WORD wID)
{
	if (wID == IDM_ABOUT) {
		OnAbout(hWnd);
		return TRUE;
	}

	return FALSE;
}

/*-[ メニューコマンド処理 ]-------------------------------------------------*/

/*
 *	メニューコマンド処理
 */
void FASTCALL OnCommand(HWND hWnd, WORD wID)
{
	ASSERT(hWnd);

	if (OnFile(hWnd, wID)) {
		return;
	}
	if (OnDisk(wID)) {
		return;
	}
	if (OnTape(wID)) {
		return;
	}
	if (OnView(hWnd, wID)) {
		return;
	}
	if (OnDebug(hWnd, wID)) {
		return;
	}
	if (OnTool(hWnd, wID)) {
		return;
	}
	if (OnWindow(wID)) {
		return;
	}
	if (OnHelp(hWnd, wID)) {
		return;
	}
}

/*
 *	メニューコマンド更新処理
 */
void FASTCALL OnMenuPopup(HWND hWnd, HMENU hSubMenu, UINT uPos)
{
	HMENU hMenu;

	ASSERT(hWnd);
	ASSERT(hSubMenu);

	/* メインメニューの更新かチェック */
	hMenu = GetMenu(hWnd);
	if (GetSubMenu(hMenu, uPos) != hSubMenu) {
		return;
	}

	/* ロックが必要 */
	LockVM();

	switch (uPos) {
		/* ファイル */
		case 0:
			OnFilePopup(hSubMenu);
			break;

		/* ドライブ1 */
		case 1:
			OnDiskPopup(hSubMenu, 1);
			break;

		/* ドライブ0 */
		case 2:
			OnDiskPopup(hSubMenu, 0);
			break;

		/* テープ */
		case 3:
			OnTapePopup(hSubMenu);
			break;

		/* 表示 */
		case 4:
			OnViewPopup(hSubMenu);
			break;

		/* デバッグ */
		case 5:
			OnDebugPopup(hSubMenu);
			break;

		/* ツール */
		case 6:
			OnToolPopup(hSubMenu);
			break;

		/* ウィンドウ */
		case 7:
			OnWindowPopup(hSubMenu);
			break;
	}

	/* アンロック */
	UnlockVM();
}

/*
 *	ファイルドロップサブ
 */
void FASTCALL OnDropSub(char *path)
{
	char dir[_MAX_DIR];
	char ext[_MAX_EXT];

	ASSERT(path);

	/* 拡張子だけ分離 */
	_splitpath(path, InitialDir, dir, NULL, ext);
	strcat(InitialDir, dir);

	/* D77 */
	if (stricmp(ext, ".D77") == 0) {
		LockVM();
		StopSnd();
		fdc_setdisk(0, path);
		fdc_setdisk(1, NULL);
		if ((fdc_ready[0] != FDC_TYPE_NOTREADY) && (fdc_medias[0] >= 2)) {
			fdc_setdisk(1, path);
			fdc_setmedia(1, 1);
		}
		system_reset();
		PlaySnd();
		ResetSch();
		UnlockVM();
	}

	/* 2D */
	if (stricmp(ext, ".2D") == 0) {
		LockVM();
		StopSnd();
		fdc_setdisk(0, path);
		fdc_setdisk(1, NULL);
		system_reset();
		PlaySnd();
		ResetSch();
		UnlockVM();
	}

	/* T77 */
	if (stricmp(ext, ".T77") == 0) {
		LockVM();
		tape_setfile(path);
		UnlockVM();
	}

	/* XM7 */
	if (stricmp(ext, ".XM7") == 0) {
		LockVM();
		StopSnd();
		if (!system_load(path)) {
			LoadString(hAppInstance, IDS_STATEERROR, path, sizeof(path));
			MessageBox(hMainWnd, path, "XM7", MB_ICONSTOP | MB_OK);
			system_reset();
		}
		else {
			strcpy(StatePath, path);
		}
		PlaySnd();
		ResetSch();
		ApplyCfg();
		UnlockVM();
	}
}

/*
 *	ファイルドロップ
 */
void FASTCALL OnDropFiles(HANDLE hDrop)
{
	char path[_MAX_PATH];

	ASSERT(hDrop);

	/* ファイル名受け取り */
	DragQueryFile(hDrop, 0, path, sizeof(path));
	DragFinish(hDrop);

	/* 処理 */
	OnDropSub(path);
}

/*
 *	コマンドライン処理
 */
void FASTCALL OnCmdLine(LPTSTR lpCmdLine)
{
	LPTSTR p;
	LPTSTR q;
	BOOL flag;

	ASSERT(lpCmdLine);

	/* ファイル名をスキップ */
	p = lpCmdLine;
	flag = FALSE;
	for (;;) {
		/* 終了チェック */
		if (*p == '\0') {
			return;
		}

		/* クォートチェック */
		if (*p == '"') {
			flag = (!flag);
		}

		/* スペースチェック */
		if ((*p == ' ') && !flag) {
			break;
		}

		/* 次へ */
		p = _tcsinc(p);
	}

	/* スペースを読み飛ばす */
	for (;;) {
		/* 終了チェック */
		if (*p == '\0') {
			return;
		}

		if (*p != ' ') {
			break;
		}

		/* 次へ */
		p = _tcsinc(p);
	}

	/* クォートチェック */
	if (*p == '"') {
		p = _tcsinc(p);
		q = p;

		/* クォートが出るまで続ける */
		for (;;) {
			/* 終了チェック */
			if (*q == '\0') {
				return;
			}

			/* クォートチェック */
			if (*q == '"') {
				*q = '\0';
				break;
			}

			/* 次へ */
			q = _tcsinc(q);
		}
	}

	/* 処理 */
	OnDropSub(p);
}

#endif	/* _WIN32 */
