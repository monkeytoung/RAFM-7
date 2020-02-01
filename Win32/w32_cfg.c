/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API コンフィギュレーション ]
 */

#ifdef _WIN32

#define STRICT
#define WIN32_LEAN_AND_MEAN
#define NONAMELESSUNION
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <prsht.h>
#include <stdlib.h>
#include <assert.h>
#include "xm7.h"
#include "whg.h"
#include "keyboard.h"
#include "w32.h"
#include "w32_cfg.h"
#include "w32_sch.h"
#include "w32_snd.h"
#include "w32_kbd.h"
#include "w32_dd.h"
#include "w32_draw.h"
#include "w32_res.h"

/*
 *	設定データ定義
 */
typedef struct {
	int fm7_ver;						/* ハードウェアバージョン */
	BOOL cycle_steel;					/* サイクルスチールフラグ */
	DWORD main_speed;					/* メインCPUスピード */
	DWORD mmr_speed;					/* メインCPU(MMR)スピード */
	BOOL bTapeFull;						/* テープモータ時の速度フラグ */

	int nSampleRate;					/* サンプリングレート */
	int nSoundBuffer;					/* サウンドバッファサイズ */
	int nBeepFreq;						/* BEEP周波数 */

	BYTE KeyMap[256];					/* キー変換テーブル */

	int nJoyType[2];					/* ジョイスティックタイプ */
	int nJoyRapid[2][2];				/* ジョイスティック連射 */
	int nJoyCode[2][7];					/* ジョイスティックコード */

	BOOL bDD480Line;					/* 全画面時640x480を優先 */
	BOOL bFullScan;						/* 400ラインモード */
	BOOL bDD480Status;					/* 640x480上下ステータス */

	BOOL bWHGEnable;					/* WHG有効フラグ */
	BOOL bDigitizeEnable;				/* ディジタイズ有効フラグ */
} configdat_t;

/*
 *	スタティック ワーク
 */
static UINT uPropertyState;				/* プロパティシート進行状況 */
static UINT uPropertyHelp;				/* ヘルプID */
static UINT KbdPageSelectID;			/* キーボードダイアログ */
static UINT KbdPageCurrentKey;			/* キーボードダイアログ */
static BYTE KbdPageMap[256];			/* キーボードダイアログ */
static UINT JoyPageIdx;					/* ジョイスティックページ */ 
static configdat_t configdat;			/* コンフィグ用データ */
static configdat_t propdat;				/* プロパティシート用データ */
static char szIniFile[_MAX_PATH];		/* INIファイル名 */
static char *pszSection;				/* セクション名 */

/*
 *	プロトタイプ宣言
 */
static void FASTCALL SheetInit(HWND hDlg);

/*
 *	コモンコントロールへのアクセスマクロ
 */
#define UpDown_GetAccel(hwnd, cAccels, paAccels) \
	(int)SendMessage((hwnd), UDM_GETACCEL, (WPARAM) cAccels, (LPARAM) (LPUDACCEL) paAccels)

#define UpDown_GetBase(hwnd) \
	(int)SendMessage((hwnd), UDM_GETBASE, 0, 0L)

#define UpDown_GetBuddy(hwnd) \
	(HWND)SendMessage((hwnd), UDM_GETBUDDY, 0, 0L)

#define UpDown_GetPos(hwnd) \
	(DWORD)SendMessage((hwnd), UDM_GETPOS, 0, 0L)

#define UpDown_GetRange(hwnd) \
	(DWORD)SendMessage((hwnd), UDM_GETRANGE, 0, 0L)

#define UpDown_SetAccel(hwnd, nAccels, aAccels) \
	(BOOL)SendMessage((hwnd), UDM_SETACCEL, (WPARAM) nAccels, (LPARAM) (LPUDACCEL) aAccels)

#define UpDown_SetBase(hwnd, nBase) \
	(int)SendMessage((hwnd), UDM_SETBASE, (WPARAM) nBase, 0L)

#define UpDown_SetBuddy(hwnd, hwndBuddy) \
	(HWND)SendMessage((hwnd), UDM_SETBUDDY, (WPARAM) (HWND) hwndBuddy, 0L)

#define UpDown_SetPos(hwnd, nPos) \
	(short)SendMessage((hwnd), UDM_SETPOS, 0, (LPARAM) MAKELONG((short) nPos, 0))

#define UpDown_SetRange(hwnd, nUpper, nLower) \
	(void)SendMessage((hwnd), UDM_SETRANGE, 0, (LPARAM) MAKELONG((short) nUpper, (short) nLower))

/*-[ 設定データ ]-----------------------------------------------------------*/

/*
 *	設定データ
 *	ファイル名指定
 */
static void FASTCALL SetCfgFile(void)
{
	char path[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];

	/* INIファイル名設定 */
	GetModuleFileName(NULL, path, sizeof(path));
	_splitpath(path, drive, dir, fname, NULL);
	strcpy(path, drive);
	strcat(path, dir);
	strcat(path, fname);
	strcat(path, ".INI");

	strcpy(szIniFile, path);
}

/*
 *	設定データ
 *	セクション名指定
 */
static void FASTCALL SetCfgSection(char *section)
{
	ASSERT(section);

	/* セクション名設定 */
	pszSection = section;
}

/*
 *	設定データ
 *	ロード(int)
 */
static int LoadCfgInt(char *key, int def)
{
	ASSERT(key);

	return (int)GetPrivateProfileInt(pszSection, key, def, szIniFile);
}

/*
 *	設定データ
 *	ロード(BOOL)
 */
static BOOL FASTCALL LoadCfgBool(char *key, BOOL def)
{
	int dat;

	ASSERT(key);

	/* 読み込み */
	if (def) {
		dat = LoadCfgInt(key, 1);
	}
	else {
		dat = LoadCfgInt(key, 0);
	}

	/* 評価 */
	if (dat != 0) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

/*
 *	設定データ
 *	ロード
 */
void FASTCALL LoadCfg(void)
{
	int i;
	int j;
	char string[128];
	BOOL flag;
	static const int JoyTable[] = {
		0x70, 0x71, 0x72, 0x73, 0, 0x74, 0x75
	};

	SetCfgFile();

	/* Generalセクション */
	SetCfgSection("General");
	configdat.fm7_ver = LoadCfgInt("Version", 2);
	if ((configdat.fm7_ver < 0) || (configdat.fm7_ver > 2)) {
		configdat.fm7_ver = 2;
	}
	configdat.cycle_steel = LoadCfgBool("CycleSteel", TRUE);
	configdat.main_speed = LoadCfgInt("MainSpeed", 1752);
	if ((configdat.main_speed < 0) || (configdat.main_speed > 9999)) {
		configdat.main_speed = 1752;
	}
	configdat.mmr_speed = LoadCfgInt("MMRSpeed", 1466);
	if ((configdat.mmr_speed < 0) || (configdat.mmr_speed > 9999)) {
		configdat.mmr_speed = 1466;
	}
	configdat.bTapeFull = LoadCfgBool("TapeFullSpeed", TRUE);

	/* Soundセクション */
	SetCfgSection("Sound");
	configdat.nSampleRate = LoadCfgInt("SampleRate", 44100);
	if ((configdat.nSampleRate != 0) &&
		(configdat.nSampleRate != 22050) &&
		(configdat.nSampleRate != 44100)) {
		configdat.nSampleRate = 44100;
	}
	configdat.nSoundBuffer = LoadCfgInt("SoundBuffer", 80);
	if ((configdat.nSoundBuffer < 20) || (configdat.nSoundBuffer > 1000)) {
		configdat.nSoundBuffer = 80;
	}
	configdat.nBeepFreq = LoadCfgInt("BeepFreq", 1200);
	if ((configdat.nBeepFreq < 100) || (configdat.nBeepFreq > 9999)) {
		configdat.nBeepFreq = 1200;
	}

	/* Keyboardセクション */
	SetCfgSection("Keyboard");
	flag = FALSE;
	for (i=0; i<256; i++) {
		sprintf(string, "Key%d", i);
		configdat.KeyMap[i] = (BYTE)LoadCfgInt(string, 0);
		/* どれか一つでもロードできたら、ok */
		if (configdat.KeyMap[i] != 0) {
			flag = TRUE;
		}
	}
	/* フラグが降りていれば、デフォルトのマップをもらう */
	if (!flag) {
		GetDefMapKbd(configdat.KeyMap, 0);
	}

	/* JoyStickセクション */
	SetCfgSection("JoyStick");
	for (i=0; i<2; i++) {
		sprintf(string, "Type%d", i);
		configdat.nJoyType[i] = LoadCfgInt(string, i + 1);
		if ((configdat.nJoyType[i] < 0) || (configdat.nJoyType[i] > 3)) {
			configdat.nJoyType[i] = i + 1;
		}

		for (j=0; j<2; j++) {
			sprintf(string, "Rapid%d", i * 10 + j);
			configdat.nJoyRapid[i][j] = LoadCfgInt(string, 0);
			if ((configdat.nJoyRapid[i][j] < 0) || (configdat.nJoyRapid[i][j] > 9)) {
				configdat.nJoyRapid[i][j] = 0;
			}

		}

		flag = TRUE;
		for (j=0; j<7; j++) {
			sprintf(string, "Code%d", i * 10 + j);
			configdat.nJoyCode[i][j] = LoadCfgInt(string, -1);
			if ((configdat.nJoyCode[i][j] < 0) || (configdat.nJoyCode[i][j] > 0x75)) {
				flag = FALSE;
			}
		}
		/* レンジエラーなら初期値設定 */
		if (!flag) {
			for (j=0; j<7; j++) {
				configdat.nJoyCode[i][j] = JoyTable[j];
			}
		}
	}

	/* Screenセクション */
	SetCfgSection("Screen");
	configdat.bDD480Line = LoadCfgBool("DD480Line", FALSE);
	configdat.bFullScan = LoadCfgBool("FullScan", FALSE);
	configdat.bDD480Status = LoadCfgBool("DD480Status", TRUE);

	/* Optionセクション */
	SetCfgSection("Option");
	configdat.bWHGEnable = LoadCfgBool("WHGEnable", TRUE);
	configdat.bDigitizeEnable = LoadCfgBool("DigitizeEnable", TRUE);
}

/*
 *	設定データ
 *	セーブ(文字列)
 */
static void FASTCALL SaveCfgString(char *key, char *string)
{
	ASSERT(key);
	ASSERT(string);

	WritePrivateProfileString(pszSection, key, string, szIniFile);
}

/*
 *	設定データ
 *	セーブ(４バイトint)
 */
static void FASTCALL SaveCfgInt(char *key, int dat)
{
	char string[128];

	ASSERT(key);

	sprintf(string, "%d", dat);
	SaveCfgString(key, string);
}

/*
 *	設定データ
 *	セーブ(BOOL)
 */
static void FASTCALL SaveCfgBool(char *key, BOOL dat)
{
	ASSERT(key);

	if (dat) {
		SaveCfgInt(key, 1);
	}
	else {
		SaveCfgInt(key, 0);
	}
}

/*
 *	設定データ
 *	セーブ
 */
void FASTCALL SaveCfg(void)
{
	int i;
	int j;
	char string[128];

	SetCfgFile();

	/* Generalセクション */
	SetCfgSection("General");
	SaveCfgInt("Version", configdat.fm7_ver);
	SaveCfgBool("CycleSteel", configdat.cycle_steel);
	SaveCfgInt("MainSpeed", configdat.main_speed);
	SaveCfgInt("MMRSpeed", configdat.mmr_speed);
	SaveCfgBool("TapeFullSpeed", configdat.bTapeFull);

	/* Soundセクション */
	SetCfgSection("Sound");
	SaveCfgInt("SampleRate", configdat.nSampleRate);
	SaveCfgInt("SoundBuffer", configdat.nSoundBuffer);
	SaveCfgInt("BeepFreq", configdat.nBeepFreq);

	/* Keyboardセクション */
	SetCfgSection("Keyboard");
	for (i=0; i<256; i++) {
		if (configdat.KeyMap[i] != 0) {
			sprintf(string, "Key%d", i);
			SaveCfgInt(string, configdat.KeyMap[i]);
		}
	}

	/* JoyStickセクション */
	SetCfgSection("JoyStick");
	for (i=0; i<2; i++) {
		sprintf(string, "Type%d", i);
		SaveCfgInt(string, configdat.nJoyType[i]);

		for (j=0; j<2; j++) {
			sprintf(string, "Rapid%d", i * 10 + j);
			SaveCfgInt(string, configdat.nJoyRapid[i][j]);
		}

		for (j=0; j<7; j++) {
			sprintf(string, "Code%d", i * 10 + j);
			SaveCfgInt(string, configdat.nJoyCode[i][j]);
		}
	}

	/* Screenセクション */
	SetCfgSection("Screen");
	SaveCfgBool("DD480Line", configdat.bDD480Line);
	SaveCfgBool("FullScan", configdat.bFullScan);
	SaveCfgBool("DD480Status", configdat.bDD480Status);

	/* Optionセクション */
	SetCfgSection("Option");
	SaveCfgBool("WHGEnable", configdat.bWHGEnable);
	SaveCfgBool("DigitizeEnable", configdat.bDigitizeEnable);
}

/*
 *	設定データ適用
 *	※VMのロックは行っていないので注意
 */
void FASTCALL ApplyCfg(void)
{
	/* Generalセクション */
	fm7_ver = configdat.fm7_ver;
	cycle_steel = configdat.cycle_steel;
	main_speed = configdat.main_speed * 10;
	mmr_speed = configdat.mmr_speed * 10;
	bTapeFullSpeed = configdat.bTapeFull;

	/* Soundセクション */
	nSampleRate = configdat.nSampleRate;
	nSoundBuffer = configdat.nSoundBuffer;
	nBeepFreq = configdat.nBeepFreq;
	ApplySnd();

	/* Keyboardセクション */
	SetMapKbd(configdat.KeyMap);

	/* JoyStickセクション */
	memcpy(nJoyType, configdat.nJoyType, sizeof(nJoyType));
	memcpy(nJoyRapid, configdat.nJoyRapid, sizeof(nJoyRapid));
	memcpy(nJoyCode, configdat.nJoyCode, sizeof(nJoyCode));

	/* Screenセクション */
	bDD480Line = configdat.bDD480Line;
	bFullScan = configdat.bFullScan;
	bDD480Status = configdat.bDD480Status;
	InvalidateRect(hDrawWnd, NULL, FALSE);

	/* Optionセクション */
	whg_enable = configdat.bWHGEnable;
	digitize_enable = configdat.bDigitizeEnable;
}

/*-[ ヘルプサポート ]-------------------------------------------------------*/

/*
 *	サポートIDリスト
 *	(先に来るものほど優先)
 */
static const UINT PageHelpList[] = {
	IDC_GP_MACHINEG,
	IDC_GP_SPEEDMODEG,
	IDC_GP_SPEEDG,
	IDC_SP_RATEG,
	IDC_SP_BUFFERG,
	IDC_SP_BEEPG,
	IDC_KP_LIST,
	IDC_KP_101B,
	IDC_KP_106B,
	IDC_KP_98B,
	IDC_JP_PORTG,
	IDC_JP_TYPEG,
	IDC_JP_RAPIDG,
	IDC_JP_CODEG,
	IDC_SCP_MODEG,
	IDC_SCP_24K,
	IDC_SCP_CAPTIONB,
	IDC_OP_WHGG,
	IDC_OP_DIGITIZEG,
	0
};

/*
 *	ページヘルプ
 */
static void FASTCALL PageHelp(HWND hDlg, UINT uID)
{
	POINT point;
	RECT rect;
	HWND hWnd;
	int i;
	char string[128];

	ASSERT(hDlg);

	/* ポイント作成 */
	GetCursorPos(&point);

	/* ヘルプリストに載っているIDを回る */
	for (i=0; ;i++) {
		/* 終了チェック */
		if (PageHelpList[i] == 0) {
			break;
		}

		/* ウインドウハンドル取得 */
		hWnd = GetDlgItem(hDlg, PageHelpList[i]);
		if (!hWnd) {
			continue;
		}
		if (!IsWindowVisible(hWnd)) {
			continue;
		}

		/* 矩形を得て、PtInRectでチェックする */
		GetWindowRect(hWnd, &rect);
		if (!PtInRect(&rect, point)) {
			continue;
		}

		/* キャッシュチェック */
		if (PageHelpList[i] == uPropertyHelp) {
			return;
		}
		uPropertyHelp = PageHelpList[i];

		/* 文字列リソースをロード、設定 */
		string[0] = '\0';
		LoadString(hAppInstance, uPropertyHelp, string, sizeof(string));
		hWnd = GetDlgItem(hDlg, uID);
		if (hWnd) {
			SetWindowText(hWnd, string);
		}

		/* 設定終了 */
		return;
	}

	/* ヘルプリスト範囲外の矩形。文字列なし */
	if (uPropertyHelp == 0) {
		return;
	}
	uPropertyHelp = 0;

	string[0] = '\0';
	hWnd = GetDlgItem(hDlg, uID);
	if (hWnd) {
		SetWindowText(hWnd, string);
	}
}

/*-[ 全般ページ ]-----------------------------------------------------------*/

/*
 *	全般ページ
 *	ダイアログ初期化
 */
static void FASTCALL GeneralPageInit(HWND hDlg)
{
	HWND hWnd;
	char string[128];

	ASSERT(hDlg);

	/* シート初期化 */
	SheetInit(hDlg);

	/* 動作機種 */
	if (propdat.fm7_ver == 1) {
		CheckDlgButton(hDlg, IDC_GP_FM7, BST_CHECKED);
	}
	else {
		CheckDlgButton(hDlg, IDC_GP_FM77AV, BST_CHECKED);
	}

	/* 動作モード */
	if (propdat.cycle_steel == TRUE) {
		CheckDlgButton(hDlg, IDC_GP_HIGHSPEED, BST_CHECKED);
	}
	else {
		CheckDlgButton(hDlg, IDC_GP_LOWSPEED, BST_CHECKED);
	}

	/* CPU選択 */
	hWnd = GetDlgItem(hDlg, IDC_GP_CPUCOMBO);
	ASSERT(hWnd);
	string[0] = '\0';
	ComboBox_ResetContent(hWnd);
	LoadString(hAppInstance, IDS_GP_MAINCPU, string, sizeof(string));
	ComboBox_AddString(hWnd, string);
	LoadString(hAppInstance, IDS_GP_MAINMMR, string, sizeof(string));
	ComboBox_AddString(hWnd, string);
	ComboBox_SetCurSel(hWnd, 0);

	/* CPU速度 */
	hWnd = GetDlgItem(hDlg, IDC_GP_CPUSPIN);
	ASSERT(hWnd);
	UpDown_SetRange(hWnd, 9999, 0);
	UpDown_SetPos(hWnd, propdat.main_speed);

	/* テープモータフラグ */
	if (propdat.bTapeFull) {
		CheckDlgButton(hDlg, IDC_GP_TAPESPEED, BST_CHECKED);
	}
	else {
		CheckDlgButton(hDlg, IDC_GP_TAPESPEED, BST_UNCHECKED);
	}
}

/*
 *	全般ページ
 *	コマンド
 */
static void FASTCALL GeneralPageCmd(HWND hDlg, WORD wID, WORD wNotifyCode, HWND hWnd)
{
	int index;
	HWND hSpin;

	ASSERT(hDlg);

	/* ID別 */
	switch (wID) {
		/* CPU選択コンボボックス */
		case IDC_GP_CPUCOMBO:
			/* 選択対象が変わったら、新しい値をロード */
			if (wNotifyCode == CBN_SELCHANGE) {
				index = ComboBox_GetCurSel(hWnd);
				hSpin = GetDlgItem(hDlg, IDC_GP_CPUSPIN);
				ASSERT(hSpin);
				switch (index) {
					case 0:
						UpDown_SetPos(hSpin, propdat.main_speed);
						break;
					case 1:
						UpDown_SetPos(hSpin, propdat.mmr_speed);
						break;
					default:
						ASSERT(FALSE);
				}
			}
			break;
	}
}

/*
 *	全般ページ
 *	垂直スクロール
 */
static void FASTCALL GeneralPageVScroll(HWND hDlg, WORD wPos, HWND hWnd)
{
	int index;

	/* チェック */
	if (hWnd != GetDlgItem(hDlg, IDC_GP_CPUSPIN)) {
		return;
	}

	/* 値を格納 */
	index = ComboBox_GetCurSel(hWnd);
	switch (index) {
		case 0:
			propdat.main_speed = (DWORD)wPos;
			break;
		case 1:
			propdat.mmr_speed = (DWORD)wPos;
			break;
		default:
			ASSERT(FALSE);
			break;
	}
}

/*
 *	全般ページ
 *	適用
 */
static void FASTCALL GeneralPageApply(HWND hDlg)
{
	/* ステート変更 */
	uPropertyState = 2;

	/* FM-7バージョン */
	if (IsDlgButtonChecked(hDlg, IDC_GP_FM7) == BST_CHECKED) {
		propdat.fm7_ver = 1;
	}
	else {
		propdat.fm7_ver = 2;
	}

	/* サイクルスチール */
	if (IsDlgButtonChecked(hDlg, IDC_GP_HIGHSPEED) == BST_CHECKED) {
		propdat.cycle_steel = TRUE;
	}
	else {
		propdat.cycle_steel = FALSE;
	}

	/* テープ高速モード */
	if (IsDlgButtonChecked(hDlg, IDC_GP_TAPESPEED) == BST_CHECKED) {
		propdat.bTapeFull = TRUE;
	}
	else {
		propdat.bTapeFull = FALSE;
	}
}

/*
 *	全般ページ
 *	ダイアログプロシージャ
 */
static BOOL CALLBACK GeneralPageProc(HWND hDlg, UINT msg,
									 WPARAM wParam, LPARAM lParam)
{
	LPNMHDR pnmh;

	switch (msg) {
		/* 初期化 */
		case WM_INITDIALOG:
			GeneralPageInit(hDlg);
			return TRUE;

		/* コマンド */
		case WM_COMMAND:
			GeneralPageCmd(hDlg, LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
			return TRUE;

		/* 通知 */
		case WM_NOTIFY:
			pnmh = (LPNMHDR)lParam;
			if (pnmh->code == PSN_APPLY) {
				GeneralPageApply(hDlg);
				return TRUE;
			}
			break;

		/* 垂直スクロール */
		case WM_VSCROLL:
			GeneralPageVScroll(hDlg, HIWORD(wParam), (HWND)lParam);
			break;

		/* カーソル設定 */
		case WM_SETCURSOR:
			if (HIWORD(lParam) == WM_MOUSEMOVE) {
				PageHelp(hDlg, IDC_GP_HELP);
			}
			break;
	}

	return FALSE;
}

/*
 *	全般ページ
 *	作成
 */
static HPROPSHEETPAGE FASTCALL GeneralPageCreate(void)
{
	PROPSHEETPAGE psp;

	/* 構造体を作成 */
	memset(&psp, 0, sizeof(PROPSHEETPAGE));
	psp.dwSize = PROPSHEETPAGE_V1_SIZE;
	psp.dwFlags = 0;
	psp.hInstance = hAppInstance;
	psp.u.pszTemplate = MAKEINTRESOURCE(IDD_GENERALPAGE);
	psp.pfnDlgProc = GeneralPageProc;

	return CreatePropertySheetPage(&psp);
}

/*-[ サウンドページ ]-------------------------------------------------------*/

/*
 *	サウンドページ
 *	垂直スクロール
 */
static void FASTCALL SoundPageVScroll(HWND hDlg, WORD wPos, HWND hWnd)
{
	HWND hBuddyWnd;
	char string[128];

	ASSERT(hDlg);
	ASSERT(hWnd);

	/* ウインドウハンドルをチェック */
	if (hWnd != GetDlgItem(hDlg, IDC_SP_BUFSPIN)) {
		return;
	}

	/* ポジションから、バディウインドウに値を設定 */
	hBuddyWnd = GetDlgItem(hDlg, IDC_SP_BUFEDIT);
	ASSERT(hBuddyWnd);
	sprintf(string, "%d", wPos * 10);
	SetWindowText(hBuddyWnd, string);
}

/*
 *	サウンドページ
 *	ダイアログ初期化
 */
static void FASTCALL SoundPageInit(HWND hDlg)
{
	HWND hWnd;

	/* シート初期化 */
	SheetInit(hDlg);

	/* サンプリングレート */
	switch (propdat.nSampleRate) {
		case 44100:
			CheckDlgButton(hDlg, IDC_SP_44K, BST_CHECKED);
			break;
		case 22050:
			CheckDlgButton(hDlg, IDC_SP_22K, BST_CHECKED);
			break;
		case 0:
			CheckDlgButton(hDlg, IDC_SP_NONE, BST_CHECKED);
			break;
		default:
			ASSERT(FALSE);
			break;
	}

	/* サウンドバッファ */
	hWnd = GetDlgItem(hDlg, IDC_SP_BUFSPIN);
	ASSERT(hWnd);
	UpDown_SetRange(hWnd, 100, 2);
	UpDown_SetPos(hWnd, propdat.nSoundBuffer / 10);
	SoundPageVScroll(hDlg, LOWORD(UpDown_GetPos(hWnd)), hWnd);

	/* BEEP周波数 */
	hWnd = GetDlgItem(hDlg, IDC_SP_BEEPSPIN);
	ASSERT(hWnd);
	UpDown_SetRange(hWnd, 9999, 100);
	UpDown_SetPos(hWnd, propdat.nBeepFreq);
}

/*
 *	サウンドページ
 *	適用
 */
static void FASTCALL SoundPageApply(HWND hDlg)
{
	HWND hWnd;
	UINT uPos;

	/* ステート変更 */
	uPropertyState = 2;

	/* サンプリングレート */
	if (IsDlgButtonChecked(hDlg, IDC_SP_44K) == BST_CHECKED) {
		propdat.nSampleRate = 44100;
	}
	if (IsDlgButtonChecked(hDlg, IDC_SP_22K) == BST_CHECKED) {
		propdat.nSampleRate = 22050;
	}
	if (IsDlgButtonChecked(hDlg, IDC_SP_NONE) == BST_CHECKED) {
		propdat.nSampleRate = 0;
	}

	/* サウンドバッファ */
	hWnd = GetDlgItem(hDlg, IDC_SP_BUFSPIN);
	ASSERT(hWnd);
	uPos = LOWORD(UpDown_GetPos(hWnd));
	propdat.nSoundBuffer = uPos * 10;

	/* BEEP周波数 */
	hWnd = GetDlgItem(hDlg, IDC_SP_BEEPSPIN);
	ASSERT(hWnd);
	propdat.nBeepFreq = LOWORD(UpDown_GetPos(hWnd));
}

/*
 *	サウンドページ
 *	ダイアログプロシージャ
 */
static BOOL CALLBACK SoundPageProc(HWND hDlg, UINT msg,
									 WPARAM wParam, LPARAM lParam)
{
	LPNMHDR pnmh;

	switch (msg) {
		/* 初期化 */
		case WM_INITDIALOG:
			SoundPageInit(hDlg);
			return TRUE;

		/* 通知 */
		case WM_NOTIFY:
			pnmh = (LPNMHDR)lParam;
			if (pnmh->code == PSN_APPLY) {
				SoundPageApply(hDlg);
				return TRUE;
			}
			break;

		/* 垂直スクロール */
		case WM_VSCROLL:
			SoundPageVScroll(hDlg, HIWORD(wParam), (HWND)lParam);
			break;

		/* カーソル設定 */
		case WM_SETCURSOR:
			if (HIWORD(lParam) == WM_MOUSEMOVE) {
				PageHelp(hDlg, IDC_SP_HELP);
			}
			break;
	}

	return FALSE;
}

/*
 *	サウンドページ
 *	作成
 */
static HPROPSHEETPAGE FASTCALL SoundPageCreate(void)
{
	PROPSHEETPAGE psp;

	/* 構造体を作成 */
	memset(&psp, 0, sizeof(PROPSHEETPAGE));
	psp.dwSize = PROPSHEETPAGE_V1_SIZE;
	psp.dwFlags = 0;
	psp.hInstance = hAppInstance;
	psp.u.pszTemplate = MAKEINTRESOURCE(IDD_SOUNDPAGE);
	psp.pfnDlgProc = SoundPageProc;

	return CreatePropertySheetPage(&psp);
}

/*-[ キー入力ダイアログ ]---------------------------------------------------*/

/*
 *	キーボードページ
 *	DirectInput キーコードテーブル
 */
static char *KbdPageDirectInput[] = {
	NULL,					/* 0x00 */
	"DIK_ESCAPE",			/* 0x01 */
	"DIK_1",				/* 0x02 */
	"DIK_2",				/* 0x03 */
	"DIK_3",				/* 0x04 */
	"DIK_4",				/* 0x05 */
	"DIK_5",				/* 0x06 */
	"DIK_6",				/* 0x07 */
	"DIK_7",				/* 0x08 */
	"DIK_8",				/* 0x09 */
	"DIK_9",				/* 0x0A */
	"DIK_0",				/* 0x0B */
	"DIK_MINUS",			/* 0x0C */
	"DIK_EQUALS",			/* 0x0D */
	"DIK_BACK",				/* 0x0E */
	"DIK_TAB",				/* 0x0F */
	"DIK_Q",				/* 0x10 */
	"DIK_W",				/* 0x11 */
	"DIK_E",				/* 0x12 */
	"DIK_R",				/* 0x13 */
	"DIK_T",				/* 0x14 */
	"DIK_Y",				/* 0x15 */
	"DIK_U",				/* 0x16 */
	"DIK_I",				/* 0x17 */
	"DIK_O",				/* 0x18 */
	"DIK_P",				/* 0x19 */
	"DIK_LBRACKET",			/* 0x1A */
	"DIK_RBRACKET",			/* 0x1B */
	"DIK_RETURN",			/* 0x1C */
	"DIK_LCONTROL",			/* 0x1D */
	"DIK_A",				/* 0x1E */
	"DIK_S",				/* 0x1F */
	"DIK_D",				/* 0x20 */
	"DIK_F",				/* 0x21 */
	"DIK_G",				/* 0x22 */
	"DIK_H",				/* 0x23 */
	"DIK_J",				/* 0x24 */
	"DIK_K",				/* 0x25 */
	"DIK_L",				/* 0x26 */
	"DIK_SEMICOLON",		/* 0x27 */
	"DIK_APOSTROPHE",		/* 0x28 */
	"DIK_GRAVE",			/* 0x29 */
	"DIK_LSHIFT",			/* 0x2A */
	"DIK_BACKSLASH",		/* 0x2B */
	"DIK_Z",				/* 0x2C */
	"DIK_X",				/* 0x2D */
	"DIK_C",				/* 0x2E */
	"DIK_V",				/* 0x2F */
	"DIK_B",				/* 0x30 */
	"DIK_N",				/* 0x31 */
	"DIK_M",				/* 0x32 */
	"DIK_COMMA",			/* 0x33 */
	"DIK_PERIOD",			/* 0x34 */
	"DIK_SLASH",			/* 0x35 */
	"DIK_RSHIFT",			/* 0x36 */
	"DIK_MULTIPLY",			/* 0x37 */
	"DIK_LMENU",			/* 0x38 */
	"DIK_SPACE",			/* 0x39 */
	"DIK_CAPITAL",			/* 0x3A */
	"DIK_F1",				/* 0x3B */
	"DIK_F2",				/* 0x3C */
	"DIK_F3",				/* 0x3D */
	"DIK_F4",				/* 0x3E */
	"DIK_F5",				/* 0x3F */
	"DIK_F6",				/* 0x40 */
	"DIK_F7",				/* 0x41 */
	"DIK_F8",				/* 0x42 */
	"DIK_F9",				/* 0x43 */
	"DIK_F10",				/* 0x44 */
	"DIK_NUMLOCK",			/* 0x45 */
	"DIK_SCROLL",			/* 0x46 */
	"DIK_NUMPAD7",			/* 0x47 */
	"DIK_NUMPAD8",			/* 0x48 */
	"DIK_NUMPAD9",			/* 0x49 */
	"DIK_SUBTRACT",			/* 0x4A */
	"DIK_NUMPAD4",			/* 0x4B */
	"DIK_NUMPAD5",			/* 0x4C */
	"DIK_NUMPAD6",			/* 0x4D */
	"DIK_ADD",				/* 0x4E */
	"DIK_NUMPAD1",			/* 0x4F */
	"DIK_NUMPAD2",			/* 0x50 */
	"DIK_NUMPAD3",			/* 0x51 */
	"DIK_NUMPAD0",			/* 0x52 */
	"DIK_DECIMAL",			/* 0x53 */
	NULL,					/* 0x54 */
	NULL,					/* 0x55 */
	"DIK_OEM_102",			/* 0x56 */
	"DIK_F11",				/* 0x57 */
	"DIK_F12",				/* 0x58 */
	NULL,					/* 0x59 */
	NULL,					/* 0x5A */
	NULL,					/* 0x5B */
	NULL,					/* 0x5C */
	NULL,					/* 0x5D */
	NULL,					/* 0x5E */
	NULL,					/* 0x5F */
	NULL,					/* 0x60 */
	NULL,					/* 0x61 */
	NULL,					/* 0x62 */
	NULL,					/* 0x63 */
	"DIK_F13",				/* 0x64 */
	"DIK_F14",				/* 0x65 */
	"DIK_F15",				/* 0x66 */
	NULL,					/* 0x67 */
	NULL,					/* 0x68 */
	NULL,					/* 0x69 */
	NULL,					/* 0x6A */
	NULL,					/* 0x6B */
	NULL,					/* 0x6C */
	NULL,					/* 0x6D */
	NULL,					/* 0x6E */
	NULL,					/* 0x6F */
	"DIK_KANA",				/* 0x70 */
	NULL,					/* 0x71 */
	NULL,					/* 0x72 */
	"DIK_ABNT_C1",			/* 0x73 */
	NULL,					/* 0x74 */
	NULL,					/* 0x75 */
	NULL,					/* 0x76 */
	NULL,					/* 0x77 */
	NULL,					/* 0x78 */
	"DIK_CONVERT",			/* 0x79 */
	NULL,					/* 0x7A */
	"DIK_NOCONVERT",		/* 0x7B */
	NULL,					/* 0x7C */
	"DIK_YEN",				/* 0x7D */
	"DIK_ABNT_C2",			/* 0x7E */
	NULL,					/* 0x7F */
	NULL,					/* 0x80 */
	NULL,					/* 0x81 */
	NULL,					/* 0x82 */
	NULL,					/* 0x83 */
	NULL,					/* 0x84 */
	NULL,					/* 0x85 */
	NULL,					/* 0x86 */
	NULL,					/* 0x87 */
	NULL,					/* 0x88 */
	NULL,					/* 0x89 */
	NULL,					/* 0x8A */
	NULL,					/* 0x8B */
	NULL,					/* 0x8C */
	"DIK_NUMPADEQUALS",		/* 0x8D */
	NULL,					/* 0x8E */
	NULL,					/* 0x8F */
	"DIK_PREVTRACK",		/* 0x90 */
	"DIK_AT",				/* 0x91 */
	"DIK_COLON",			/* 0x92 */
	"DIK_UNDERLINE",		/* 0x93 */
	"DIK_KANJI",			/* 0x94 */
	"DIK_STOP",				/* 0x95 */
	"DIK_AX",				/* 0x96 */
	"DIK_UNLABELED",		/* 0x97 */
	NULL,					/* 0x98 */
	"DIK_NEXTTRACK",		/* 0x99 */
	NULL,					/* 0x9A */
	NULL,					/* 0x9B */
	"DIK_NUMPADENTER",		/* 0x9C */
	"DIK_RCONTROL",			/* 0x9D */
	NULL,					/* 0x9E */
	NULL,					/* 0x9F */
	"DIK_MUTE",				/* 0xA0 */
	"DIK_CALCULATOR",		/* 0xA1 */
	"DIK_PLAYPAUSE",		/* 0xA2 */
	NULL,					/* 0xA3 */
	"DIK_MEDIASTOP",		/* 0xA4 */
	NULL,					/* 0xA5 */
	NULL,					/* 0xA6 */
	NULL,					/* 0xA7 */
	NULL,					/* 0xA8 */
	NULL,					/* 0xA9 */
	NULL,					/* 0xAA */
	NULL,					/* 0xAB */
	NULL,					/* 0xAC */
	NULL,					/* 0xAD */
	"DIK_VOLUMEDOWN",		/* 0xAE */
	NULL,					/* 0xAF */
	"DIK_VOLUMEUP",			/* 0xB0 */
	NULL,					/* 0xB1 */
	"DIK_WEBHOME",			/* 0xB2 */
	"DIK_NUMPADCOMMA",		/* 0xB3 */
	NULL,					/* 0xB4 */
	"DIK_DIVIDE",			/* 0xB5 */
	NULL,					/* 0xB6 */
	"DIK_SYSRQ",			/* 0xB7 */
	"DIK_RMENU",			/* 0xB8 */
	NULL,					/* 0xB9 */
	NULL,					/* 0xBA */
	NULL,					/* 0xBB */
	NULL,					/* 0xBC */
	NULL,					/* 0xBD */
	NULL,					/* 0xBE */
	NULL,					/* 0xBF */
	NULL,					/* 0xC0 */
	NULL,					/* 0xC1 */
	NULL,					/* 0xC2 */
	NULL,					/* 0xC3 */
	NULL,					/* 0xC4 */
	"DIK_PAUSE",			/* 0xC5 */
	NULL,					/* 0xC6 */
	"DIK_HOME",				/* 0xC7 */
	"DIK_UP",				/* 0xC8 */
	"DIK_PRIOR",			/* 0xC9 */
	NULL,					/* 0xCA */
	"DIK_LEFT",				/* 0xCB */
	NULL,					/* 0xCC */
	"DIK_RIGHT",			/* 0xCD */
	NULL,					/* 0xCE */
	"DIK_END",				/* 0xCF */
	"DIK_DOWN",				/* 0xD0 */
	"DIK_NEXT",				/* 0xD1 */
	"DIK_INSERT",			/* 0xD2 */
	"DIK_DELETE",			/* 0xD3 */
	NULL,					/* 0xD4 */
	NULL,					/* 0xD5 */
	NULL,					/* 0xD6 */
	NULL,					/* 0xD7 */
	NULL,					/* 0xD8 */
	NULL,					/* 0xD9 */
	NULL,					/* 0xDA */
	"DIK_LWIN",				/* 0xDB */
	"DIK_RWIN",				/* 0xDC */
	"DIK_APPS",				/* 0xDD */
	"DIK_POWER",			/* 0xDE */
	"DIK_SLEEP",			/* 0xDF */
	NULL,					/* 0xE0 */
	NULL,					/* 0xE1 */
	NULL,					/* 0xE2 */
	"DIK_WAKE",				/* 0xE3 */
	NULL,					/* 0xE4 */
	"DIK_WEBSEARCH",		/* 0xE5 */
	"DIK_WEBFAVORITES",		/* 0xE6 */
	"DIK_WEBREFRESH",		/* 0xE7 */
	"DIK_WEBSTOP",			/* 0xE8 */
	"DIK_WEBFORWARD",		/* 0xE9 */
	"DIK_WEBBACK",			/* 0xEA */
	"DIK_MYCOMPUTER",		/* 0xEB */
	"DIK_MAIL",				/* 0xEC */
	"DIK_MEDIASELECT",		/* 0xED */
	NULL,					/* 0xEE */
	NULL,					/* 0xEF */
	NULL,					/* 0xF0 */
	NULL,					/* 0xF1 */
	NULL,					/* 0xF2 */
	NULL,					/* 0xF3 */
	NULL,					/* 0xF4 */
	NULL,					/* 0xF5 */
	NULL,					/* 0xF6 */
	NULL,					/* 0xF7 */
	NULL,					/* 0xF8 */
	NULL,					/* 0xF9 */
	NULL,					/* 0xFA */
	NULL,					/* 0xFB */
	NULL,					/* 0xFC */
	NULL,					/* 0xFD */
	NULL,					/* 0xFE */
	NULL					/* 0xFF */
};

/*
 *	キーボードページ
 *	FM77AV キーコードテーブル
 */
static char *KbdPageFM77AV[] = {
	NULL, NULL,				/* 0x00 */
	"ESC", NULL,			/* 0x01 */
	"1", "ぬ",				/* 0x02 */
	"2", "ふ",				/* 0x03 */
	"3", "あ",				/* 0x04 */
	"4", "う",				/* 0x05 */
	"5", "え",				/* 0x06 */
	"6", "お",				/* 0x07 */
	"7", "や",				/* 0x08 */
	"8", "ゆ",				/* 0x09 */
	"9", "よ",				/* 0x0A */
	"0", "わ",				/* 0x0B */
	"-", "ほ",				/* 0x0C */
	"^", "へ",				/* 0x0D */
	"\\", "ー",				/* 0x0E */
	"BS", NULL,				/* 0x0F */
	"TAB", NULL,			/* 0x10 */
	"Q", "た",				/* 0x11 */
	"W", "て",				/* 0x12 */
	"E", "い",				/* 0x13 */
	"R", "す",				/* 0x14 */
	"T", "か",				/* 0x15 */
	"Y", "ん",				/* 0x16 */
	"U", "な",				/* 0x17 */
	"I", "に",				/* 0x18 */
	"O", "ら",				/* 0x19 */
	"P", "せ",				/* 0x1A */
	"@", "゛",				/* 0x1B */
	"[", "゜",				/* 0x1C */
	"RETURN", NULL,			/* 0x1D */
	"A", "ち",				/* 0x1E */
	"S", "と",				/* 0x1F */
	"D", "し",				/* 0x20 */
	"F", "は",				/* 0x21 */
	"G", "き",				/* 0x22 */
	"H", "く",				/* 0x23 */
	"J", "ま",				/* 0x24 */
	"K", "の",				/* 0x25 */
	"L", "り",				/* 0x26 */
	";", "れ",				/* 0x27 */
	":", "け",				/* 0x28 */
	"]", "む",				/* 0x29 */
	"Z", "つ",				/* 0x2A */
	"X", "さ",				/* 0x2B */
	"C", "そ",				/* 0x2C */
	"V", "ひ",				/* 0x2D */
	"B", "こ",				/* 0x2E */
	"N", "み",				/* 0x2F */
	"M", "も",				/* 0x30 */
	",", "ね",				/* 0x31 */
	".", "る",				/* 0x32 */
	"/", "め",				/* 0x33 */
	"_", "ろ",				/* 0x34 */
	"SPACE(右)", NULL,		/* 0x35 */
	"*", "テンキー",		/* 0x36 */
	"/", "テンキー",		/* 0x37 */
	"+", "テンキー",		/* 0x38 */
	"-", "テンキー",		/* 0x39 */
	"7", "テンキー",		/* 0x3A */
	"8", "テンキー",		/* 0x3B */
	"9", "テンキー",		/* 0x3C */
	"=", "テンキー",		/* 0x3D */
	"4", "テンキー",		/* 0x3E */
	"5", "テンキー",		/* 0x3F */
	"6", "テンキー",		/* 0x40 */
	",", "テンキー",		/* 0x41 */
	"1", "テンキー",		/* 0x42 */
	"2", "テンキー",		/* 0x43 */
	"3", "テンキー",		/* 0x44 */
	"RETURN", "テンキー",	/* 0x45 */
	"0", "テンキー",		/* 0x46 */
	".", "テンキー",		/* 0x47 */
	"INS", NULL,			/* 0x48 */
	"EL", NULL,				/* 0x49 */
	"CLS", NULL,			/* 0x4A */
	"DEL", NULL,			/* 0x4B */
	"DUP", NULL,			/* 0x4C */
	"↑", NULL,				/* 0x4D */
	"HOME", NULL,			/* 0x4E */
	"←", NULL,				/* 0x4F */
	"↓", NULL,				/* 0x50 */
	"→", NULL,				/* 0x51 */
	"CTRL", NULL,			/* 0x52 */
	"SHIFT(左)", NULL,		/* 0x53 */
	"SHIFT(右)", NULL,		/* 0x54 */
	"CAP", NULL,			/* 0x55 */
	"GRAPH", NULL,			/* 0x56 */
	"SPACE(左)", NULL,		/* 0x57 */
	"SPACE(中)", NULL,		/* 0x58 */
	NULL, NULL,				/* 0x59 */
	"かな", NULL,			/* 0x5A */
	NULL, NULL,				/* 0x5B */
	"BREAK", NULL,			/* 0x5C */
	"PF1",	NULL,			/* 0x5D */
	"PF2",	NULL,			/* 0x5E */
	"PF3",	NULL,			/* 0x5F */
	"PF4",	NULL,			/* 0x60 */
	"PF5",	NULL,			/* 0x61 */
	"PF6",	NULL,			/* 0x62 */
	"PF7",	NULL,			/* 0x63 */
	"PF8",	NULL,			/* 0x64 */
	"PF9",	NULL,			/* 0x65 */
	"PF10",	NULL			/* 0x66 */
};

/*
 *	インデックス→FM77AV キーコード
 */
static int FASTCALL KbdPageIndex2FM77AV(int index)
{
	int i;

	for (i=0; i<sizeof(KbdPageFM77AV)/sizeof(char *)/2; i++) {
		/* NULLのキーはスキップ */
		if (KbdPageFM77AV[i * 2] == NULL) {
			continue;
		}

		/* NULLでなければ、チェック＆デクリメント */
		if (index == 0) {
			return i;
		}
		index--;
	}

	/* エラー */
	return 0;
}

/*
 *	FM77AV キーコード→インデックス
 */
static int FASTCALL KbdPageFM77AV2Index(int keycode)
{
	int i;
	int index;

	index = 0;
	for (i=0; i<sizeof(KbdPageFM77AV)/sizeof(char *)/2; i++) {
		/* キーコードに到達したら終了 */
		if (i == keycode) {
			break;
		}

		/* NULLのキーはスキップ */
		if (KbdPageFM77AV[i * 2] == NULL) {
			continue;
		}

		/* NULLでなければ、インクリメント */
		index++;
	}

	return index;
}

/*
 *	FM77AV キーコード→DirectInput キーコード
 */
static int FASTCALL KbdPageFM77AV2DirectInput(int fm)
{
	int i;

	/* 検索 */
	for (i=0; i<256; i++) {
		if (propdat.KeyMap[i] == fm) {
			return i;
		}
	}

	/* エラー */
	return 0;
}

/*
 *	キー入力ダイアログ
 *	ダイアログ初期化
 */
static void FASTCALL KeyInDlgInit(HWND hDlg)
{
	HWND hWnd;
	RECT prect;
	RECT drect;
	int fm;
	int di;
	char formstr[128];
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

	/* キー番号テキスト初期化 */
	fm = KbdPageIndex2FM77AV(KbdPageSelectID);
	formstr[0] = '\0';
	LoadString(hAppInstance, IDC_KEYIN_LABEL, formstr, sizeof(formstr));
	sprintf(string, formstr, fm);
	hWnd = GetDlgItem(hDlg, IDC_KEYIN_LABEL);
	SetWindowText(hWnd, string);

	/* DirectInputキーテキスト初期化 */
	di = KbdPageFM77AV2DirectInput(fm);
	ASSERT((di >= 0) && (di < 256));
	hWnd = GetDlgItem(hDlg, IDC_KEYIN_KEY);
	if (KbdPageDirectInput[di]) {
		SetWindowText(hWnd, KbdPageDirectInput[di]);
	}
	else {
		LoadString(hAppInstance, IDC_KEYIN_KEY, string, sizeof(string));
		SetWindowText(hWnd, string);
	}

	/* グローバルワーク初期化 */
	KbdPageCurrentKey = di;
	GetKbd(KbdPageMap);

	/* タイマーをスタート */
	SetTimer(hDlg, IDD_KEYINDLG, 50, NULL);
}

/*
 *	キー入力ダイアログ
 *	タイマー
 */
static void FASTCALL KeyInTimer(HWND hDlg)
{
	BYTE buf[256];
	int i;
	HWND hWnd;
	char string[128];

	ASSERT(hDlg);

	/* キー入力 */
	LockVM();
	GetKbd(buf);
	UnlockVM();

	/* 今回押されたキーを特定 */
	for (i=0; i<256; i++) {
		if ((KbdPageMap[i] < 0x80) && (buf[i] >= 0x80)) {
			break;
		}
	}

	/* キーマップを更新し、チェック */
	memcpy(KbdPageMap, buf, 256);
	if (i >= 256) {
		return;
	}

	/* キーの番号、テキストをセット */
	KbdPageCurrentKey = i;

	hWnd = GetDlgItem(hDlg, IDC_KEYIN_KEY);
	if (KbdPageDirectInput[i]) {
		SetWindowText(hWnd, KbdPageDirectInput[i]);
	}
	else {
		LoadString(hAppInstance, IDC_KEYIN_KEY, string, sizeof(string));
		SetWindowText(hWnd, string);
	}
}

/*
 *	キー入力ダイアログ
 *	ダイアログプロシージャ
 */
static BOOL CALLBACK KeyInDlgProc(HWND hDlg, UINT iMsg,
									WPARAM wParam, LPARAM lParam)
{
	switch (iMsg) {
		/* ダイアログ初期化 */
		case WM_INITDIALOG:
			KeyInDlgInit(hDlg);
			return TRUE;

		/* コマンド */
		case WM_COMMAND:
			if (LOWORD(wParam) != IDCANCEL) {
				return TRUE;
			}
			/* ESCキーを検知するための工夫 */
			if (KbdPageCurrentKey == 0x01) {
				EndDialog(hDlg, IDCANCEL);
				return TRUE;
			}
			KeyInTimer(hDlg);
			if (KbdPageCurrentKey == 0x01) {
				return TRUE;
			}
			EndDialog(hDlg, IDCANCEL);
			return TRUE;

		/* タイマー */
		case WM_TIMER:
			KeyInTimer(hDlg);
			return TRUE;

		/* 右クリック */
		case WM_RBUTTONDOWN:
			EndDialog(hDlg, IDOK);
			return TRUE;

		/* ウインドウ破棄 */
		case WM_DESTROY:
			KillTimer(hDlg, IDD_KEYINDLG);
			break;

	}

	/* それ以外はFALSE */
	return FALSE;
}

/*-[ キーボードページ ]-----------------------------------------------------*/

/*
 *	キーボードページ
 *	ダイアログ初期化(ヘッダーアイテム)
 */
static void FASTCALL KbdPageInitColumn(HWND hWnd)
{
	int i;
	char string[128];
	TEXTMETRIC tm;
	HDC hDC;
	LV_COLUMN lvc;
	static const UINT uHeaderTable[] = {
		IDS_KP_KEYNO,
		IDS_KP_KEYFM,
		IDS_KP_KEYKANA,
		IDS_KP_KEYDI
	};

	ASSERT(hWnd);

	/* テキストメトリックを取得 */
	hDC = GetDC(hWnd);
	GetTextMetrics(hDC, &tm);
	ReleaseDC(hWnd, hDC);

	/* 挿入ループ */
	for (i=0; i<(sizeof(uHeaderTable)/sizeof(UINT)); i++) {
		/* テキストをロード */
		LoadString(hAppInstance, uHeaderTable[i], string, sizeof(string));

		/* カラム構造体を作成 */
		lvc.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_WIDTH | LVCF_TEXT;
		lvc.iSubItem = i;
		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = (strlen(string) + 1) * tm.tmAveCharWidth;
		lvc.pszText = string;
		lvc.cchTextMax = strlen(string);

		/* カラム挿入 */
		ListView_InsertColumn(hWnd, i, &lvc);
	}
}

/*
 *	キーボードページ
 *	サブアイテム一括設定
 */
static void FASTCALL KbdPageSubItem(HWND hDlg)
{
	HWND hWnd;
	int i;
	int j;
	int index;

	/* リストコントロール取得 */
	hWnd = GetDlgItem(hDlg, IDC_KP_LIST);
	ASSERT(hWnd);

	/* アイテム挿入 */
	for (index=0; ; index++) {
		/* FM77AVキー番号を得る */
		i = KbdPageIndex2FM77AV(index);
		if (i == 0) {
			break;
		}

		/* 該当するDirectXキー番号を得る */
		j = KbdPageFM77AV2DirectInput(i);

		/* 文字列セット */
		if (KbdPageDirectInput[j]) {
			ListView_SetItemText(hWnd, index, 3, KbdPageDirectInput[j]);
		}
		else {
			ListView_SetItemText(hWnd, index, 3, "");
		}
	}
}

/*
 *	キーボードページ
 *	ダイアログ初期化
 */
static void FASTCALL KbdPageInit(HWND hDlg)
{
	HWND hWnd;
	LV_ITEM lvi;
	int index;
	int i;
	char string[128];

	/* シート初期化 */
	SheetInit(hDlg);

	/* リストコントロール取得 */
	hWnd = GetDlgItem(hDlg, IDC_KP_LIST);
	ASSERT(hWnd);

	/* カラム挿入 */
	KbdPageInitColumn(hWnd);

	/* アイテム挿入 */
	for (index=0; ; index++) {
		/* FM77AVキー番号を得る */
		i = KbdPageIndex2FM77AV(index);
		if (i == 0) {
			break;
		}

		/* アイテム挿入 */
		sprintf(string, "%02X", i);
		lvi.mask = LVIF_TEXT;
		lvi.pszText = string;
		lvi.cchTextMax = strlen(string);
		lvi.iItem = index;
		lvi.iSubItem = 0;
		ListView_InsertItem(hWnd, &lvi);

		/* サブアイテム×２をセット */
		if (KbdPageFM77AV[i * 2 + 0]) {
			ListView_SetItemText(hWnd, index, 1, KbdPageFM77AV[i * 2 + 0]);
		}
		if (KbdPageFM77AV[i * 2 + 1]) {
			ListView_SetItemText(hWnd, index, 2, KbdPageFM77AV[i * 2 + 1]);
		}
	}

	/* サブアイテム */
	KbdPageSubItem(hDlg);
}

/*
 *	キーボードページ
 *	コマンド
 */
static void FASTCALL KbdPageCmd(HWND hDlg, WORD wID, WORD wNotifyCode, HWND hWnd)
{
	ASSERT(hDlg);

	switch (wID) {
		/* 106キーボード */
		case IDC_KP_106B:
			GetDefMapKbd(propdat.KeyMap, 1);
			KbdPageSubItem(hDlg);
			break;

		/* PC-98キーボード */
		case IDC_KP_98B:
			GetDefMapKbd(propdat.KeyMap, 2);
			KbdPageSubItem(hDlg);
			break;

		/* 101キーボード */
		case IDC_KP_101B:
			GetDefMapKbd(propdat.KeyMap, 3);
			KbdPageSubItem(hDlg);
			break;
	}
}

/*
 *	キーボードページ
 *	ダブルクリック
 */
static void FASTCALL KbdPageDblClk(HWND hDlg)
{
	HWND hWnd;
	int count;
	int i;
	int j;

	ASSERT(hDlg);

	/* リストコントロール取得 */
	hWnd = GetDlgItem(hDlg, IDC_KP_LIST);
	ASSERT(hWnd);

	/* 選択されているアイテムが無ければ、リターン */
	if (ListView_GetSelectedCount(hWnd) == 0) {
		return;
	}

	/* 登録されているアイテムの個数を取得 */
	count = ListView_GetItemCount(hWnd);

	/* セレクトされているインデックスを得る */
	for (i=0; i<count; i++) {
		if (ListView_GetItemState(hWnd, i, LVIS_SELECTED)) {
			break;
		}
	}

	/* グローバルへ記憶 */
	ASSERT(i < count);
	KbdPageSelectID = i;

	/* モーダルダイアログ実行 */
	if (DialogBox(hAppInstance, MAKEINTRESOURCE(IDD_KEYINDLG),
						hDlg, KeyInDlgProc) != IDOK) {
		return;
	}

	/* マップを修正 */
	j = KbdPageIndex2FM77AV(i);
	for (i=0; i<256; i++) {
		if (propdat.KeyMap[i] == j) {
			propdat.KeyMap[i] = 0;
		}
	}
	propdat.KeyMap[KbdPageCurrentKey] = (BYTE)j;

	/* 再描画 */
	KbdPageSubItem(hDlg);
}

/*
 *	キーボードページ
 *	適用
 */
static void FASTCALL KbdPageApply(HWND hDlg)
{
	/* ステート変更 */
	uPropertyState = 2;
}

/*
 *	キーボードページ
 *	ダイアログプロシージャ
 */
static BOOL CALLBACK KbdPageProc(HWND hDlg, UINT msg,
									 WPARAM wParam, LPARAM lParam)
{
	LPNMHDR pnmh;
	LV_KEYDOWN *plkd;

	switch (msg) {
		/* 初期化 */
		case WM_INITDIALOG:
			KbdPageInit(hDlg);
			return TRUE;

		/* コマンド */
		case WM_COMMAND:
			KbdPageCmd(hDlg, LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
			return TRUE;

		/* 通知 */
		case WM_NOTIFY:
			pnmh = (LPNMHDR)lParam;
			/* ページ終了 */
			if (pnmh->code == PSN_APPLY) {
				KbdPageApply(hDlg);
				return TRUE;
			}
			/* リストビュー ダブルクリック */
			if ((pnmh->idFrom == IDC_KP_LIST) && (pnmh->code == NM_DBLCLK)) {
				KbdPageDblClk(hDlg);
				return TRUE;
			}
			/* リストビュー 特殊キー入力 */
			if ((pnmh->idFrom == IDC_KP_LIST) && (pnmh->code == LVN_KEYDOWN)) {
				plkd = (LV_KEYDOWN*)pnmh;
				/* SPACEが入力されたらキー選択 */
				if (plkd->wVKey == VK_SPACE) {
					KbdPageDblClk(hDlg);
					return TRUE;
				}
			}
			break;

		/* カーソル設定 */
		case WM_SETCURSOR:
			if (HIWORD(lParam) == WM_MOUSEMOVE) {
				PageHelp(hDlg, IDC_KP_HELP);
			}
			break;
	}

	return FALSE;
}

/*
 *	キーボードページ
 *	作成
 */
static HPROPSHEETPAGE FASTCALL KbdPageCreate(void)
{
	PROPSHEETPAGE psp;

	/* 構造体を作成 */
	memset(&psp, 0, sizeof(PROPSHEETPAGE));
	psp.dwSize = PROPSHEETPAGE_V1_SIZE;
	psp.dwFlags = 0;
	psp.hInstance = hAppInstance;
	psp.u.pszTemplate = MAKEINTRESOURCE(IDD_KBDPAGE);
	psp.pfnDlgProc = KbdPageProc;

	return CreatePropertySheetPage(&psp);
}

/*-[ ジョイスティックページ ]-----------------------------------------------*/

/*
 *	ジョイスティックページ
 *	コンボボックステーブル
 */
static const UINT JoyPageComboTable[] = {
	IDC_JP_UPC,
	IDC_JP_DOWNC,
	IDC_JP_LEFTC,
	IDC_JP_RIGHTC,
	IDC_JP_CENTERC,
	IDC_JP_AC,
	IDC_JP_BC
};

/*
 *	ジョイスティックページ
 *	コードテーブル
 */
static const UINT JoyPageCodeTable[] = {
	IDS_JP_TYPE0,
	IDC_JP_UP,
	IDC_JP_DOWN,
	IDC_JP_LEFT,
	IDC_JP_RIGHT,
	IDC_JP_A,
	IDC_JP_B
};

/*
/*
 *	ジョイスティックページ
 *	コンボボックスセレクト
 */
static void FASTCALL JoyPageCombo(HWND hDlg, WORD wID)
{
	int i;
	int j;
	HWND hWnd;
	char string[128];

	ASSERT(hDlg);

	/* IDチェック */
	if (wID != IDC_JP_TYPEC) {
		return;
	}

	/* "使用しない"メッセージをロード */
	string[0] = '\0';
	LoadString(hAppInstance, IDS_JP_TYPE0, string, sizeof(string));

	/* コンボックスをクリア */
	for (i=0; i<sizeof(JoyPageComboTable)/sizeof(UINT); i++) {
		hWnd = GetDlgItem(hDlg, JoyPageComboTable[i]);
		ASSERT(hWnd);
		ComboBox_ResetContent(hWnd);
	}

	/* タイプ取得 */
	hWnd = GetDlgItem(hDlg, IDC_JP_TYPEC);
	ASSERT(hWnd);
	j = ComboBox_GetCurSel(hWnd);

	/* キーボードの場合 */
	if (j == 3) {
		for (i=0; i<sizeof(JoyPageComboTable)/sizeof(UINT); i++) {
			/* ハンドル取得 */
			hWnd = GetDlgItem(hDlg, JoyPageComboTable[i]);
			ASSERT(hWnd);
			/* キーコード挿入 */
			ComboBox_AddString(hWnd, string);
			for (j=0; j<sizeof(KbdPageFM77AV)/sizeof(char*); j+=2) {
				if (KbdPageFM77AV[j]) {
					ComboBox_AddString(hWnd, KbdPageFM77AV[j]);
				}
			}
			/* キーコードにカーソル合わせ */
			if (propdat.nJoyCode[JoyPageIdx][i] == 0) {
				ComboBox_SetCurSel(hWnd, 0);
				continue;
			}
			if (propdat.nJoyCode[JoyPageIdx][i] > 0x66) {
				ComboBox_SetCurSel(hWnd, 0);
				continue;
			}
			j = KbdPageFM77AV2Index(propdat.nJoyCode[JoyPageIdx][i]);
			ComboBox_SetCurSel(hWnd, j + 1);
		}
		return;
	}

	/* ジョイスティックの場合 */
	for (i=0; i<sizeof(JoyPageComboTable)/sizeof(UINT); i++) {
		/* ハンドル取得 */
		hWnd = GetDlgItem(hDlg, JoyPageComboTable[i]);
		ASSERT(hWnd);

		/* 文字列挿入 */
		for (j=0; j<sizeof(JoyPageCodeTable)/sizeof(UINT); j++) {
			string[0] = '\0';
			LoadString(hAppInstance, JoyPageCodeTable[j], string, sizeof(string));
			ComboBox_AddString(hWnd, string);
		}

		/* カーソル設定 */
		if (propdat.nJoyCode[JoyPageIdx][i] < 0x70) {
			ComboBox_SetCurSel(hWnd, 0);
			continue;
		}
		if (propdat.nJoyCode[JoyPageIdx][i] < 0x74) {
			ComboBox_SetCurSel(hWnd, propdat.nJoyCode[JoyPageIdx][i] - 0x70 + 1);
			continue;
		}
		ComboBox_SetCurSel(hWnd, propdat.nJoyCode[JoyPageIdx][i] - 0x74 + 5);
	}
}

/*
 *	ジョイスティックページ
 *	データセット
 */
static void FASTCALL JoyPageSet(HWND hDlg)
{
	HWND hWnd;

	ASSERT(hDlg);
	ASSERT((JoyPageIdx == 0) || (JoyPageIdx == 1));

	/* 連射コンボボックス */
	hWnd = GetDlgItem(hDlg, IDC_JP_RAPIDAC);
	ASSERT(hWnd);
	ComboBox_SetCurSel(hWnd, propdat.nJoyRapid[JoyPageIdx][0]);
	hWnd = GetDlgItem(hDlg, IDC_JP_RAPIDBC);
	ASSERT(hWnd);
	ComboBox_SetCurSel(hWnd, propdat.nJoyRapid[JoyPageIdx][1]);

	/* タイプコンボボックス */
	hWnd = GetDlgItem(hDlg, IDC_JP_TYPEC);
	ASSERT(hWnd);
	ComboBox_SetCurSel(hWnd, propdat.nJoyType[JoyPageIdx]);

	/* コード処理 */
	JoyPageCombo(hDlg, IDC_JP_TYPEC);
}

/*
 *	ジョイスティックページ
 *	データ取得
 */
static void FASTCALL JoyPageGet(HWND hDlg)
{
	HWND hWnd;
	int i;
	int j;

	ASSERT(hDlg);
	ASSERT((JoyPageIdx == 0) || (JoyPageIdx == 1));

	/* 連射コンボボックス */
	hWnd = GetDlgItem(hDlg, IDC_JP_RAPIDAC);
	ASSERT(hWnd);
	propdat.nJoyRapid[JoyPageIdx][0] = ComboBox_GetCurSel(hWnd);
	hWnd = GetDlgItem(hDlg, IDC_JP_RAPIDBC);
	ASSERT(hWnd);
	propdat.nJoyRapid[JoyPageIdx][1] = ComboBox_GetCurSel(hWnd);

	/* タイプコンボボックス */
	hWnd = GetDlgItem(hDlg, IDC_JP_TYPEC);
	ASSERT(hWnd);
	propdat.nJoyType[JoyPageIdx] = ComboBox_GetCurSel(hWnd);

	/* コード */
	if (propdat.nJoyType[JoyPageIdx] == 3) {
		/* キーボード */
		for (i=0; i<sizeof(JoyPageComboTable)/sizeof(UINT); i++) {
			/* ハンドル取得 */
			hWnd = GetDlgItem(hDlg, JoyPageComboTable[i]);
			ASSERT(hWnd);

			/* コード変換、セット */
			j = ComboBox_GetCurSel(hWnd);
			if (j != 0) {
				j = KbdPageIndex2FM77AV(j - 1);
			}
			propdat.nJoyCode[JoyPageIdx][i] = j;
		}
		return;
	}

	/* ジョイスティック */
	for (i=0; i<sizeof(JoyPageComboTable)/sizeof(UINT); i++) {
		/* ハンドル取得 */
		hWnd = GetDlgItem(hDlg, JoyPageComboTable[i]);
		ASSERT(hWnd);

		/* コード変換、セット */
		j = ComboBox_GetCurSel(hWnd);
		if (j == 0) {
			propdat.nJoyCode[JoyPageIdx][i] = 0;
			continue;
		}
		if (j < 5) {
			propdat.nJoyCode[JoyPageIdx][i] = (j - 1) + 0x70;
			continue;
		}
		propdat.nJoyCode[JoyPageIdx][i] = (j - 5) + 0x74;
	}
}

/*
 *	ジョイスティックページ
 *	ボタン押された
 */
static void FASTCALL JoyPageButton(HWND hDlg, WORD wID)
{
	ASSERT(hDlg);

	switch (wID) {
		/* ポート1 を選択 */
		case IDC_JP_PORT1:
			JoyPageGet(hDlg);
			JoyPageIdx = 0;
			JoyPageSet(hDlg);
			break;

		/* ポート2 を選択 */
		case IDC_JP_PORT2:
			JoyPageGet(hDlg);
			JoyPageIdx = 1;
			JoyPageSet(hDlg);
			break;

		/* それ以外 */
		default:
			ASSERT(FALSE);
			break;
	}
}

/*
 *	ジョイスティックページ
 *	ダイアログ初期化
 */
static void FASTCALL JoyPageInit(HWND hDlg)
{
	HWND hWnd[2];
	int i;
	char string[128];

	/* シート初期化 */
	ASSERT(hDlg);
	SheetInit(hDlg);

	/* タイプコンボボックス */
	hWnd[0] = GetDlgItem(hDlg, IDC_JP_TYPEC);
	ASSERT(hWnd[0]);
	ComboBox_ResetContent(hWnd[0]);
	for (i=0; i<4; i++) {
		string[0] = '\0';
		LoadString(hAppInstance, IDS_JP_TYPE0 + i, string, sizeof(string));
		ComboBox_AddString(hWnd[0], string);
	}
	ComboBox_SetCurSel(hWnd[0], 0);

	/* 連射コンボボックス */
	hWnd[0] = GetDlgItem(hDlg, IDC_JP_RAPIDAC);
	ASSERT(hWnd[0]);
	ComboBox_ResetContent(hWnd[0]);
	hWnd[1] = GetDlgItem(hDlg, IDC_JP_RAPIDBC);
	ASSERT(hWnd[1]);
	ComboBox_ResetContent(hWnd[1]);
	for (i=0; i<10; i++) {
		string[0] = '\0';
		LoadString(hAppInstance, IDS_JP_RAPID0 + i, string, sizeof(string));
		ComboBox_AddString(hWnd[0], string);
		ComboBox_AddString(hWnd[1], string);
	}
	ComboBox_SetCurSel(hWnd[0], 0);
	ComboBox_SetCurSel(hWnd[1], 0);

	/* ポート選択グループボタン */
	CheckDlgButton(hDlg, IDC_JP_PORT1, BST_CHECKED);
	JoyPageIdx = 0;
	JoyPageSet(hDlg);
}

/*
 *	ジョイスティックページ
 *	適用
 */
static void FASTCALL JoyPageApply(HWND hDlg)
{
	ASSERT(hDlg);

	/* データ取得 */
	JoyPageGet(hDlg);

	/* ステート変更 */
	uPropertyState = 2;
}

/*
 *	ジョイスティックページ
 *	ダイアログプロシージャ
 */
static BOOL CALLBACK JoyPageProc(HWND hDlg, UINT msg,
									 WPARAM wParam, LPARAM lParam)
{
	LPNMHDR pnmh;

	switch (msg) {
		/* 初期化 */
		case WM_INITDIALOG:
			JoyPageInit(hDlg);
			return TRUE;

		/* コマンド */
		case WM_COMMAND:
			switch (HIWORD(wParam)) {
				/* ボタンクリック */
				case BN_CLICKED:
					JoyPageButton(hDlg, LOWORD(wParam));
					break;
				/* コンボ選択 */
				case CBN_SELCHANGE:
					JoyPageCombo(hDlg, LOWORD(wParam));
					break;
			}
			return TRUE;

		/* 通知 */
		case WM_NOTIFY:
			pnmh = (LPNMHDR)lParam;
			if (pnmh->code == PSN_APPLY) {
				JoyPageApply(hDlg);
				return TRUE;
			}
			break;

		/* カーソル設定 */
		case WM_SETCURSOR:
			if (HIWORD(lParam) == WM_MOUSEMOVE) {
				PageHelp(hDlg, IDC_JP_HELP);
			}
			break;
	}

	return FALSE;
}

/*
 *	ジョイスティックページ
 *	作成
 */
static HPROPSHEETPAGE FASTCALL JoyPageCreate(void)
{
	PROPSHEETPAGE psp;

	/* 構造体を作成 */
	memset(&psp, 0, sizeof(PROPSHEETPAGE));
	psp.dwSize = PROPSHEETPAGE_V1_SIZE;
	psp.dwFlags = 0;
	psp.hInstance = hAppInstance;
	psp.u.pszTemplate = MAKEINTRESOURCE(IDD_JOYPAGE);
	psp.pfnDlgProc = JoyPageProc;

	return CreatePropertySheetPage(&psp);
}

/*-[ スクリーンページ ]-----------------------------------------------------*/

/*
 *	スクリーンページ
 *	ダイアログ初期化
 */
static void FASTCALL ScrPageInit(HWND hDlg)
{
	/* シート初期化 */
	ASSERT(hDlg);
	SheetInit(hDlg);

	/* 全画面優先モード */
	if (propdat.bDD480Line) {
		CheckDlgButton(hDlg, IDC_SCP_480, BST_CHECKED);
	}
	else {
		CheckDlgButton(hDlg, IDC_SCP_400, BST_CHECKED);
	}

	/* フルスキャン(24k) */
	if (propdat.bFullScan) {
		CheckDlgButton(hDlg, IDC_SCP_24K, BST_CHECKED);
	}
	else {
		CheckDlgButton(hDlg, IDC_SCP_24K, BST_UNCHECKED);
	}

	/* 上下ステータス */
	if (propdat.bDD480Status) {
		CheckDlgButton(hDlg, IDC_SCP_CAPTIONB, BST_CHECKED);
	}
	else {
		CheckDlgButton(hDlg, IDC_SCP_CAPTIONB, BST_UNCHECKED);
	}
}

/*
 *	スクリーンページ
 *	適用
 */
static void FASTCALL ScrPageApply(HWND hDlg)
{
	ASSERT(hDlg);

	/* ステート変更 */
	uPropertyState = 2;

	/* 全画面優先モード */
	if (IsDlgButtonChecked(hDlg, IDC_SCP_480) == BST_CHECKED) {
		propdat.bDD480Line = TRUE;
	}
	else {
		propdat.bDD480Line = FALSE;
	}

	/* フルスキャン(24k) */
	if (IsDlgButtonChecked(hDlg, IDC_SCP_24K) == BST_CHECKED) {
		propdat.bFullScan = TRUE;
	}
	else {
		propdat.bFullScan = FALSE;
	}

	/* 上下ステータス */
	if (IsDlgButtonChecked(hDlg, IDC_SCP_CAPTIONB) == BST_CHECKED) {
		propdat.bDD480Status = TRUE;
	}
	else {
		propdat.bDD480Status = FALSE;
	}
}

/*
 *	スクリーンページ
 *	ダイアログプロシージャ
 */
static BOOL CALLBACK ScrPageProc(HWND hDlg, UINT msg,
									 WPARAM wParam, LPARAM lParam)
{
	LPNMHDR pnmh;

	switch (msg) {
		/* 初期化 */
		case WM_INITDIALOG:
			ScrPageInit(hDlg);
			return TRUE;

		/* 通知 */
		case WM_NOTIFY:
			pnmh = (LPNMHDR)lParam;
			if (pnmh->code == PSN_APPLY) {
				ScrPageApply(hDlg);
				return TRUE;
			}
			break;

		/* カーソル設定 */
		case WM_SETCURSOR:
			if (HIWORD(lParam) == WM_MOUSEMOVE) {
				PageHelp(hDlg, IDC_SCP_HELP);
			}
			break;
	}

	return FALSE;
}

/*
 *	スクリーンページ
 *	作成
 */
static HPROPSHEETPAGE FASTCALL ScrPageCreate(void)
{
	PROPSHEETPAGE psp;

	/* 構造体を作成 */
	memset(&psp, 0, sizeof(PROPSHEETPAGE));
	psp.dwSize = PROPSHEETPAGE_V1_SIZE;
	psp.dwFlags = 0;
	psp.hInstance = hAppInstance;
	psp.u.pszTemplate = MAKEINTRESOURCE(IDD_SCRPAGE);
	psp.pfnDlgProc = ScrPageProc;

	return CreatePropertySheetPage(&psp);
}

/*-[ オプションページ ]-----------------------------------------------------*/

/*
 *	オプションページ
 *	ダイアログ初期化
 */
static void FASTCALL OptPageInit(HWND hDlg)
{
	/* シート初期化 */
	ASSERT(hDlg);
	SheetInit(hDlg);

	/* WHG */
	if (propdat.bWHGEnable) {
		CheckDlgButton(hDlg, IDC_OP_WHGB, BST_CHECKED);
	}
	else {
		CheckDlgButton(hDlg, IDC_OP_WHGB, BST_UNCHECKED);
	}

	/* ビデオディジタイズ */
	if (propdat.bDigitizeEnable) {
		CheckDlgButton(hDlg, IDC_OP_DIGITIZEB, BST_CHECKED);
	}
	else {
		CheckDlgButton(hDlg, IDC_OP_DIGITIZEB, BST_UNCHECKED);
	}
}

/*
 *	オプションページ
 *	適用
 */
static void FASTCALL OptPageApply(HWND hDlg)
{
	ASSERT(hDlg);

	/* ステート変更 */
	uPropertyState = 2;

	/* WHG */
	if (IsDlgButtonChecked(hDlg, IDC_OP_WHGB) == BST_CHECKED) {
		propdat.bWHGEnable = TRUE;
	}
	else {
		propdat.bWHGEnable = FALSE;
	}

	/* ビデオディジタイズ */
	if (IsDlgButtonChecked(hDlg, IDC_OP_DIGITIZEB) == BST_CHECKED) {
		propdat.bDigitizeEnable = TRUE;
	}
	else {
		propdat.bDigitizeEnable = FALSE;
	}
}

/*
 *	オプションページ
 *	ダイアログプロシージャ
 */
static BOOL CALLBACK OptPageProc(HWND hDlg, UINT msg,
									 WPARAM wParam, LPARAM lParam)
{
	LPNMHDR pnmh;

	switch (msg) {
		/* 初期化 */
		case WM_INITDIALOG:
			OptPageInit(hDlg);
			return TRUE;

		/* 通知 */
		case WM_NOTIFY:
			pnmh = (LPNMHDR)lParam;
			if (pnmh->code == PSN_APPLY) {
				OptPageApply(hDlg);
				return TRUE;
			}
			break;

		/* カーソル設定 */
		case WM_SETCURSOR:
			if (HIWORD(lParam) == WM_MOUSEMOVE) {
				PageHelp(hDlg, IDC_OP_HELP);
			}
			break;
	}

	return FALSE;
}

/*
 *	オプションページ
 *	作成
 */
static HPROPSHEETPAGE FASTCALL OptPageCreate(void)
{
	PROPSHEETPAGE psp;

	/* 構造体を作成 */
	memset(&psp, 0, sizeof(PROPSHEETPAGE));
	psp.dwSize = PROPSHEETPAGE_V1_SIZE;
	psp.dwFlags = 0;
	psp.hInstance = hAppInstance;
	psp.u.pszTemplate = MAKEINTRESOURCE(IDD_OPTPAGE);
	psp.pfnDlgProc = OptPageProc;

	return CreatePropertySheetPage(&psp);
}

/*-[ プロパティシート ]-----------------------------------------------------*/

/*
 *	プロパティシート
 *	初期化
 */
static void FASTCALL SheetInit(HWND hDlg)
{
	RECT drect;
	RECT prect;
	LONG lStyleEx;

	/* 初期化フラグをチェック、シート初期化済みに設定 */
	if (uPropertyState > 0) {
		return;
	}
	uPropertyState = 1;

	/* プロパティシートを、メインウインドウの中央に寄せる */
	GetWindowRect(hMainWnd, &prect);
	GetWindowRect(GetParent(hDlg), &drect);
	drect.right -= drect.left;
	drect.bottom -= drect.top;
	drect.left = (prect.right - prect.left) / 2 + prect.left;
	drect.left -= (drect.right / 2);
	drect.top = (prect.bottom - prect.top) / 2 + prect.top;
	drect.top -= (drect.bottom / 2);
	MoveWindow(GetParent(hDlg), drect.left, drect.top, drect.right, drect.bottom, FALSE);

	/* プロパティシートからヘルプボタンを除去 */
	lStyleEx = GetWindowLong(GetParent(hDlg), GWL_EXSTYLE);
	lStyleEx &= ~WS_EX_CONTEXTHELP;
	SetWindowLong(GetParent(hDlg), GWL_EXSTYLE, lStyleEx);
}

/*
 *	設定(C)
 */
void FASTCALL OnConfig(HWND hWnd)
{
	PROPSHEETHEADER pshead;
	HPROPSHEETPAGE hpspage[6];
	int i;

	ASSERT(hWnd);

	/* データ転送 */
	propdat = configdat;

	/* プロパティページ作成 */
	hpspage[0] = GeneralPageCreate();
	hpspage[1] = SoundPageCreate();
	hpspage[2] = KbdPageCreate();
	hpspage[3] = JoyPageCreate();
	hpspage[4] = ScrPageCreate();
	hpspage[5] = OptPageCreate();

	/* プロパティページチェック */
	for (i=0; i<sizeof(hpspage)/sizeof(HPROPSHEETPAGE); i++) {
		if (!hpspage[i]) {
			return;
		}
	}

	/* プロパティシート作成 */
	memset(&pshead, 0, sizeof(pshead));
	pshead.dwSize = PROPSHEETHEADER_V1_SIZE;
	pshead.dwFlags = PSH_NOAPPLYNOW;
	pshead.hwndParent = hWnd;
	pshead.hInstance = hAppInstance;
	pshead.pszCaption = MAKEINTRESOURCE(IDS_CONFIGCAPTION);
	pshead.nPages = sizeof(hpspage) / sizeof(HPROPSHEETPAGE);
	pshead.u2.nStartPage = 0;
	pshead.u3.phpage = hpspage;

	/* プロパティシート実行 */
	uPropertyState = 0;
	uPropertyHelp = 0;
	PropertySheet(&pshead);

	/* 結果がok以外なら終了 */
	if (uPropertyState != 2) {
		return;
	}

	/* okなので、データ転送 */
	configdat = propdat;

	/* 適用 */
	LockVM();
	ApplyCfg();
	UnlockVM();
}

#endif	/* _WIN32 */
