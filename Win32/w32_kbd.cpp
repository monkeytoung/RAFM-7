/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API キーボード ]
 */

#ifdef _WIN32

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define DIRECTINPUT_VERSION		0x0300		/* DirectX3を指定 */
#include <dinput.h>
#include <mmsystem.h>
#include <assert.h>
#include "xm7.h"
#include "mainetc.h"
#include "keyboard.h"
#include "device.h"
#include "w32.h"
#include "w32_sch.h"
#include "w32_kbd.h"

/*
 *	グローバル ワーク
 */
BYTE kbd_map[256];							/* キーボード マップ */
BYTE kbd_table[256];						/* 対応するFM-7物理コード */
int nJoyType[2];							/* ジョイスティックタイプ */
int nJoyRapid[2][2];						/* 連射タイプ */
int nJoyCode[2][7];							/* 生成コード */

/*
 *	スタティック ワーク
 */
static LPDIRECTINPUT lpdi;					/* DirectInput */
static LPDIRECTINPUTDEVICE lpkbd;			/* キーボードデバイス */
static DWORD keytime;						/* キーボードポーリング時間 */
static BYTE joydat[2];						/* ジョイスティックデータ */
static BYTE joybk[2];						/* ジョイスティックバックアップ */
static int joyrapid[2][2];					/* ジョイスティック連射カウンタ */
static DWORD joytime;						/* ジョイスティックポーリング時間 */

/*
 *	DirectInputコード→FM-7 物理コード
 *	コード対照表(106キーボード用)
 */
static BYTE kbd_106_table[] = {
	DIK_ESCAPE,			0x5c,			/* BREAK(ESC) */
	DIK_F1,				0x5d,			/* PF1 */
	DIK_F2,				0x5e,			/* PF2 */
	DIK_F3,				0x5f,			/* PF3 */
	DIK_F4,				0x60,			/* PF4 */
	DIK_F5,				0x61,			/* PF5 */
	DIK_F6,				0x62,			/* PF6 */
	DIK_F7,				0x63,			/* PF7 */
	DIK_F8,				0x64,			/* PF8 */
	DIK_F9,				0x65,			/* PF9 */
	DIK_F11,			0x66,			/* PF10(F11) */

	DIK_KANJI,			0x01,			/* ESC(半角/全角) */
	DIK_1,				0x02,			/* 1 */
	DIK_2,				0x03,			/* 2 */
	DIK_3,				0x04,			/* 3 */
	DIK_4,				0x05,			/* 4 */
	DIK_5,				0x06,			/* 5 */
	DIK_6,				0x07,			/* 6 */
	DIK_7,				0x08,			/* 7 */
	DIK_8,				0x09,			/* 8 */
	DIK_9,				0x0a,			/* 9 */
	DIK_0,				0x0b,			/* 0 */
	DIK_MINUS,			0x0c,			/* - */
	DIK_CIRCUMFLEX,		0x0d,			/* ^ */
	DIK_YEN,			0x0e,			/* \ */
	DIK_BACK,			0x0f,			/* BS */

	DIK_TAB,			0x10,			/* TAB */
	DIK_Q,				0x11,			/* Q */
	DIK_W,				0x12,			/* W */
	DIK_E,				0x13,			/* E */
	DIK_R,				0x14,			/* R */
	DIK_T,				0x15,			/* T */
	DIK_Y,				0x16,			/* Y */
	DIK_U,				0x17,			/* U */
	DIK_I,				0x18,			/* I */
	DIK_O,				0x19,			/* O */
	DIK_P,				0x1a,			/* P */
	DIK_AT,				0x1b,			/* @ */
	DIK_LBRACKET,		0x1c,			/* [ */
	DIK_RETURN,			0x1d,			/* CR */

	DIK_LCONTROL,		0x52,			/* CTRL(左Ctrl) */
	DIK_A,				0x1e,			/* A */
	DIK_S,				0x1f,			/* S */
	DIK_D,				0x20,			/* D */
	DIK_F,				0x21,			/* F */
	DIK_G,				0x22,			/* G */
	DIK_H,				0x23,			/* H */
	DIK_J,				0x24,			/* J */
	DIK_K,				0x25,			/* K */
	DIK_L,				0x26,			/* L */
	DIK_SEMICOLON,		0x27,			/* ; */
	DIK_COLON,			0x28,			/* : */
	DIK_RBRACKET,		0x29,			/* ] */

	DIK_LSHIFT,			0x53,			/* 左SHIFT */
	DIK_Z,				0x2a,			/* Z */
	DIK_X,				0x2b,			/* X */
	DIK_C,				0x2c,			/* C */
	DIK_V,				0x2d,			/* V */
	DIK_B,				0x2e,			/* B */
	DIK_N,				0x2f,			/* N */
	DIK_M,				0x30,			/* M */
	DIK_COMMA,			0x31,			/* , */
	DIK_PERIOD,			0x32,			/* . */
	DIK_SLASH,			0x33,			/* / */
	DIK_BACKSLASH,		0x34,			/* _ */
	DIK_RSHIFT,			0x54,			/* 右SHIFT */

	DIK_CAPITAL,		0x55,			/* CAP(Caps Lock) */
	DIK_NOCONVERT,		0x56,			/* GRAPH(無変換) */
	DIK_CONVERT,		0x57,			/* 左SPACE(変換) */
	DIK_KANA,			0x58,			/* 中SPACE(カタカナ) */
	DIK_SPACE,			0x35,			/* 右SPACE(SPACE) */
	DIK_RCONTROL,		0x5a,			/* かな(右Ctrl) */

	DIK_INSERT,			0x48,			/* INS(Insert) */
	DIK_DELETE,			0x4b,			/* DEL(Delete) */
	DIK_UP,				0x4d,			/* ↑ */
	DIK_LEFT,			0x4f,			/* ← */
	DIK_DOWN,			0x50,			/* ↓ */
	DIK_RIGHT,			0x51,			/* → */

	DIK_HOME,			0x49,			/* EL(Home) */
	DIK_PRIOR,			0x4a,			/* CLS(Page Up) */
	DIK_END,			0x4c,			/* DUP(End) */
	DIK_NEXT,			0x4e,			/* HOME(Page Down) */

	DIK_MULTIPLY,		0x36,			/* Tenkey * */
	DIK_DIVIDE,			0x37,			/* Tenkey / */
	DIK_ADD,			0x38,			/* Tenkey + */
	DIK_SUBTRACT,		0x39,			/* Tenkey - */
	DIK_NUMPAD7,		0x3a,			/* Tenkey 7 */
	DIK_NUMPAD8,		0x3b,			/* Tenkey 8 */
	DIK_NUMPAD9,		0x3c,			/* Tenkey 9 */
	DIK_NUMPAD4,		0x3e,			/* Tenkey 4 */
	DIK_NUMPAD5,		0x3f,			/* Tenkey 5 */
	DIK_NUMPAD6,		0x40,			/* Tenkey 6 */
	DIK_NUMPAD1,		0x42,			/* Tenkey 1 */
	DIK_NUMPAD2,		0x43,			/* Tenkey 2 */
	DIK_NUMPAD3,		0x44,			/* Tenkey 3 */
	DIK_NUMPAD0,		0x46,			/* Tenkey 0 */
	DIK_DECIMAL,		0x47,			/* Tenkey . */
	DIK_NUMPADENTER,	0x45			/* Tenkey CR */
};

/*
 *	DirectInputコード→FM-7 物理コード
 *	コード対照表(NEC PC-98キーボード用)
 */
static BYTE kbd_98_table[] = {
	DIK_SYSRQ,			0x5c,			/* BREAK(COPY) */
	DIK_F1,				0x5d,			/* PF1 */
	DIK_F2,				0x5e,			/* PF2 */
	DIK_F3,				0x5f,			/* PF3 */
	DIK_F4,				0x60,			/* PF4 */
	DIK_F5,				0x61,			/* PF5 */
	DIK_F6,				0x62,			/* PF6 */
	DIK_F7,				0x63,			/* PF7 */
	DIK_F8,				0x64,			/* PF8 */
	DIK_F9,				0x65,			/* PF9 */
	DIK_F11,			0x66,			/* PF10(vf1) */

	DIK_ESCAPE,			0x01,			/* ESC */
	DIK_1,				0x02,			/* 1 */
	DIK_2,				0x03,			/* 2 */
	DIK_3,				0x04,			/* 3 */
	DIK_4,				0x05,			/* 4 */
	DIK_5,				0x06,			/* 5 */
	DIK_6,				0x07,			/* 6 */
	DIK_7,				0x08,			/* 7 */
	DIK_8,				0x09,			/* 8 */
	DIK_9,				0x0a,			/* 9 */
	DIK_0,				0x0b,			/* 0 */
	DIK_MINUS,			0x0c,			/* - */
	DIK_CIRCUMFLEX,		0x0d,			/* ^ */
	DIK_YEN,			0x0e,			/* \ */
	DIK_BACK,			0x0f,			/* BS */

	DIK_TAB,			0x10,			/* TAB */
	DIK_Q,				0x11,			/* Q */
	DIK_W,				0x12,			/* W */
	DIK_E,				0x13,			/* E */
	DIK_R,				0x14,			/* R */
	DIK_T,				0x15,			/* T */
	DIK_Y,				0x16,			/* Y */
	DIK_U,				0x17,			/* U */
	DIK_I,				0x18,			/* I */
	DIK_O,				0x19,			/* O */
	DIK_P,				0x1a,			/* P */
	DIK_AT,				0x1b,			/* @ */
	DIK_LBRACKET,		0x1c,			/* [ */
	DIK_RETURN,			0x1d,			/* CR */

	DIK_LCONTROL,		0x52,			/* CTRL(左Ctrl) */
	DIK_A,				0x1e,			/* A */
	DIK_S,				0x1f,			/* S */
	DIK_D,				0x20,			/* D */
	DIK_F,				0x21,			/* F */
	DIK_G,				0x22,			/* G */
	DIK_H,				0x23,			/* H */
	DIK_J,				0x24,			/* J */
	DIK_K,				0x25,			/* K */
	DIK_L,				0x26,			/* L */
	DIK_SEMICOLON,		0x27,			/* ; */
	DIK_COLON,			0x28,			/* : */
	DIK_RBRACKET,		0x29,			/* ] */

	DIK_LSHIFT,			0x53,			/* 左SHIFT(SHIFT) */
	DIK_Z,				0x2a,			/* Z */
	DIK_X,				0x2b,			/* X */
	DIK_C,				0x2c,			/* C */
	DIK_V,				0x2d,			/* V */
	DIK_B,				0x2e,			/* B */
	DIK_N,				0x2f,			/* N */
	DIK_M,				0x30,			/* M */
	DIK_COMMA,			0x31,			/* , */
	DIK_PERIOD,			0x32,			/* . */
	DIK_SLASH,			0x33,			/* / */
	DIK_UNDERLINE,		0x34,			/* _ */
	DIK_RSHIFT,			0x54,			/* 右SHIFT(SHIFT) */

	DIK_CAPITAL,		0x55,			/* CAP(CAPS) */
	DIK_NOCONVERT,		0x56,			/* GRAPH(NFER) */
	// 左SPACEはサポートしない
	DIK_SPACE,			0x35,			/* 右SPACE(SPACE) */
	DIK_KANJI,			0x58,			/* 中SPACE(XFER) */
	DIK_KANA,			0x5a,			/* かな(カナ) */

	DIK_INSERT,			0x48,			/* INS(Insert) */
	DIK_DELETE,			0x4b,			/* DEL(Delete) */
	DIK_UP,				0x4d,			/* ↑ */
	DIK_LEFT,			0x4f,			/* ← */
	DIK_DOWN,			0x50,			/* ↓ */
	DIK_RIGHT,			0x51,			/* → */

	DIK_HOME,			0x49,			/* EL(HOME CLR) */
	DIK_PRIOR,			0x4a,			/* CLS(ROLL DOWN) */
	DIK_END,			0x4c,			/* DUP(HELP) */
	DIK_NEXT,			0x4e,			/* HOME(ROLL UP) */

	DIK_MULTIPLY,		0x36,			/* Tenkey * */
	DIK_DIVIDE,			0x37,			/* Tenkey / */
	DIK_ADD,			0x38,			/* Tenkey + */
	DIK_SUBTRACT,		0x39,			/* Tenkey - */
	DIK_NUMPAD7,		0x3a,			/* Tenkey 7 */
	DIK_NUMPAD8,		0x3b,			/* Tenkey 8 */
	DIK_NUMPAD9,		0x3c,			/* Tenkey 9 */
	DIK_NUMPADEQUALS,	0x3d,			/* Tenkey = */
	DIK_NUMPAD4,		0x3e,			/* Tenkey 4 */
	DIK_NUMPAD5,		0x3f,			/* Tenkey 5 */
	DIK_NUMPAD6,		0x40,			/* Tenkey 6 */
	DIK_NUMPADCOMMA,	0x41,			/* Tenkey , */
	DIK_NUMPAD1,		0x42,			/* Tenkey 1 */
	DIK_NUMPAD2,		0x43,			/* Tenkey 2 */
	DIK_NUMPAD3,		0x44,			/* Tenkey 3 */
	DIK_NUMPAD0,		0x46,			/* Tenkey 0 */
	DIK_DECIMAL,		0x47,			/* Tenkey . */
	DIK_NUMPADENTER,	0x45			/* Tenkey CR */
};

/*
 *	DirectInputコード→FM-7 物理コード
 *	コード対照表(101キーボード用)
 */
static BYTE kbd_101_table[] = {
	DIK_ESCAPE,			0x5c,			/* BREAK(ESC) */
	DIK_F1,				0x5d,			/* PF1 */
	DIK_F2,				0x5e,			/* PF2 */
	DIK_F3,				0x5f,			/* PF3 */
	DIK_F4,				0x60,			/* PF4 */
	DIK_F5,				0x61,			/* PF5 */
	DIK_F6,				0x62,			/* PF6 */
	DIK_F7,				0x63,			/* PF7 */
	DIK_F8,				0x64,			/* PF8 */
	DIK_F9,				0x65,			/* PF9 */
	DIK_F11,			0x66,			/* PF10(F11) */

	DIK_GRAVE,			0x01,			/* ESC(`) */
	DIK_1,				0x02,			/* 1 */
	DIK_2,				0x03,			/* 2 */
	DIK_3,				0x04,			/* 3 */
	DIK_4,				0x05,			/* 4 */
	DIK_5,				0x06,			/* 5 */
	DIK_6,				0x07,			/* 6 */
	DIK_7,				0x08,			/* 7 */
	DIK_8,				0x09,			/* 8 */
	DIK_9,				0x0a,			/* 9 */
	DIK_0,				0x0b,			/* 0 */
	DIK_MINUS,			0x0c,			/* - */
	DIK_EQUALS,			0x0d,			/* ^(=) */
	DIK_BACKSLASH,		0x0e,			/* \ */
	DIK_BACK,			0x0f,			/* BS */

	DIK_TAB,			0x10,			/* TAB */
	DIK_Q,				0x11,			/* Q */
	DIK_W,				0x12,			/* W */
	DIK_E,				0x13,			/* E */
	DIK_R,				0x14,			/* R */
	DIK_T,				0x15,			/* T */
	DIK_Y,				0x16,			/* Y */
	DIK_U,				0x17,			/* U */
	DIK_I,				0x18,			/* I */
	DIK_O,				0x19,			/* O */
	DIK_P,				0x1a,			/* P */
	DIK_LBRACKET,		0x1b,			/* @([) */
	DIK_RBRACKET,		0x1c,			/* [(]) */
	DIK_RETURN,			0x1d,			/* CR */

	DIK_LCONTROL,		0x52,			/* CTRL(左Ctrl) */
	DIK_A,				0x1e,			/* A */
	DIK_S,				0x1f,			/* S */
	DIK_D,				0x20,			/* D */
	DIK_F,				0x21,			/* F */
	DIK_G,				0x22,			/* G */
	DIK_H,				0x23,			/* H */
	DIK_J,				0x24,			/* J */
	DIK_K,				0x25,			/* K */
	DIK_L,				0x26,			/* L */
	DIK_SEMICOLON,		0x27,			/* ; */
	DIK_APOSTROPHE,		0x28,			/* :(') */
	/* ] 割り当てなし */

	DIK_LSHIFT,			0x53,			/* 左SHIFT */
	DIK_Z,				0x2a,			/* Z */
	DIK_X,				0x2b,			/* X */
	DIK_C,				0x2c,			/* C */
	DIK_V,				0x2d,			/* V */
	DIK_B,				0x2e,			/* B */
	DIK_N,				0x2f,			/* N */
	DIK_M,				0x30,			/* M */
	DIK_COMMA,			0x31,			/* , */
	DIK_PERIOD,			0x32,			/* . */
	DIK_SLASH,			0x33,			/* / */
	/* _ 割り当てなし */
	DIK_RSHIFT,			0x54,			/* 右SHIFT */

	DIK_CAPITAL,		0x55,			/* CAP(Caps Lock) */
	DIK_NUMLOCK,		0x56,			/* GRAPH(Num Lock) */
	/* 左スペース割り当てなし */
	/* 中スペース割り当てなし */
	DIK_SPACE,			0x35,			/* 右SPACE(SPACE) */
	DIK_RCONTROL,		0x5a,			/* かな(右Ctrl) */

	DIK_INSERT,			0x48,			/* INS(Insert) */
	DIK_DELETE,			0x4b,			/* DEL(Delete) */
	DIK_UP,				0x4d,			/* ↑ */
	DIK_LEFT,			0x4f,			/* ← */
	DIK_DOWN,			0x50,			/* ↓ */
	DIK_RIGHT,			0x51,			/* → */

	DIK_HOME,			0x49,			/* EL(Home) */
	DIK_PRIOR,			0x4a,			/* CLS(Page Up) */
	DIK_END,			0x4c,			/* DUP(End) */
	DIK_NEXT,			0x4e,			/* HOME(Page Down) */

	DIK_MULTIPLY,		0x36,			/* Tenkey * */
	DIK_DIVIDE,			0x37,			/* Tenkey / */
	DIK_ADD,			0x38,			/* Tenkey + */
	DIK_SUBTRACT,		0x39,			/* Tenkey - */
	DIK_NUMPAD7,		0x3a,			/* Tenkey 7 */
	DIK_NUMPAD8,		0x3b,			/* Tenkey 8 */
	DIK_NUMPAD9,		0x3c,			/* Tenkey 9 */
	DIK_NUMPAD4,		0x3e,			/* Tenkey 4 */
	DIK_NUMPAD5,		0x3f,			/* Tenkey 5 */
	DIK_NUMPAD6,		0x40,			/* Tenkey 6 */
	DIK_NUMPAD1,		0x42,			/* Tenkey 1 */
	DIK_NUMPAD2,		0x43,			/* Tenkey 2 */
	DIK_NUMPAD3,		0x44,			/* Tenkey 3 */
	DIK_NUMPAD0,		0x46,			/* Tenkey 0 */
	DIK_DECIMAL,		0x47,			/* Tenkey . */
	DIK_NUMPADENTER,	0x45			/* Tenkey CR */
};

/*
 *	デフォルトマップ取得
 */
void FASTCALL GetDefMapKbd(BYTE *pMap, int mode)
{
	int i;
	int type;

	ASSERT(pMap);
	ASSERT((mode >= 0) && (mode <= 3));

	/* 一度、すべてクリア */
	memset(pMap, 0, 256);

	/* キーボードタイプ取得 */
	switch (mode) {
		/* 自動判別 */
		case 0:
			type = GetKeyboardType(1);
			if (type & 0xd00) {
				type = 0xd00;
			}
			else {
				type = 0;
			}
			break;
		/* 106 */
		case 1:
			type = 0;
			break;
		/* PC-98 */
		case 2:
			type = 0xd00;
			break;
		/* 101 */
		case 3:
			type = 1;
			break;
		/* その他 */
		default:
			ASSERT(FALSE);
			break;
	}

	/* 変換テーブルにデフォルト値をセット */
	if (type & 0xd00) {
		/* PC-98 */
		for (i=0; i<sizeof(kbd_98_table)/2; i++) {
			pMap[kbd_98_table[i * 2]] = kbd_98_table[i * 2 + 1];
		}
		return;
	}

	if (type == 0) {
		/* 106 */
		for (i=0; i<sizeof(kbd_106_table)/2; i++) {
			pMap[kbd_106_table[i * 2]] = kbd_106_table[i * 2 + 1];
		}
	}
	else {
		/* 101 */
		for (i=0; i<sizeof(kbd_101_table)/2; i++) {
			pMap[kbd_101_table[i * 2]] = kbd_101_table[i * 2 + 1];
		}
	}
}

/*
 *	マップ設定
 */
void FASTCALL SetMapKbd(BYTE *pMap)
{
	ASSERT(pMap);

	/* コピーするだけ */
	memcpy(kbd_table, pMap, sizeof(kbd_table));
}

/*
 *	初期化
 */
void FASTCALL InitKbd(void)
{
	/* ワークエリア初期化(キーボード) */
	lpdi = NULL;
	lpkbd = NULL;
	memset(kbd_map, 0, sizeof(kbd_map));
	memset(kbd_table, 0, sizeof(kbd_table));
	keytime = 0;

	/* ワークエリア初期化(ジョイスティック) */
	joytime = 0;
	memset(joydat, 0, sizeof(joydat));
	memset(joybk, 0, sizeof(joybk));
	memset(joyrapid, 0, sizeof(joyrapid));

	/* デフォルトマップを設定 */
	GetDefMapKbd(kbd_table, 0);
}

/*
 *	クリーンアップ
 */
void FASTCALL CleanKbd(void)
{
	/* DirectInputDevice(キーボード)を解放 */
	if (lpkbd) {
		lpkbd->Unacquire();
		lpkbd->Release();
		lpkbd = NULL;
	}

	/* DirectInputを解放 */
	if (lpdi) {
		lpdi->Release();
		lpdi = NULL;
	}
}

/*
 *	セレクト
 */
BOOL FASTCALL SelectKbd(HWND hWnd)
{
	ASSERT(hWnd);

	/* DirectInputオブジェクトを作成 */
	if (FAILED(DirectInputCreate(hAppInstance, DIRECTINPUT_VERSION,
							&lpdi, NULL))) {
		return FALSE;
	}

	/* キーボードデバイスを作成 */
	if (FAILED(lpdi->CreateDevice(GUID_SysKeyboard, &lpkbd, NULL))) {
		return FALSE;
	}

	/* キーボードデータ形式を設定 */
	if (FAILED(lpkbd->SetDataFormat(&c_dfDIKeyboard))) {
		return FALSE;
	}

	/* 協調レベルを設定 */
	if (FAILED(lpkbd->SetCooperativeLevel(hWnd,
						DISCL_BACKGROUND | DISCL_NONEXCLUSIVE))) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	ポーリング
 */
void FASTCALL PollKbd(void)
{
	HRESULT hr;
	BYTE buf[256];
	int i;
	BYTE fm7;
	BOOL bFlag;

	/* DirectInputチェック */
	if (!lpkbd) {
		return;
	}

	/* アクティベートチェック */
	if (!bActivate) {
		return;
	}

	/* 時間チェック */
	if ((dwExecTotal - keytime) < 20000) {
		return;
	}
	keytime = dwExecTotal;

	/* デバイスのアクセス権を取得(何回取得してもよい) */
	hr = lpkbd->Acquire();
	if ((hr != DI_OK) && (hr != S_FALSE)) {
		return;
	}

	/* デバイス状態を取得 */
	if (lpkbd->GetDeviceState(sizeof(buf), buf) != DI_OK) {
		return;
	}

	/* フラグoff */
	bFlag = FALSE;

	/* 今までの状態と比較して、順に変換する */
	for (i=0; i<sizeof(buf); i++) {
		if ((buf[i] & 0x80) != (kbd_map[i] & 0x80)) {
			if (buf[i] & 0x80) {
				/* キー押下 */
				fm7 = kbd_table[i];
				if (fm7 > 0) {
					if (!bMenuLoop) {
						keyboard_make(fm7);
						bFlag = TRUE;
					}
				}
			}
			else {
				/* キー離した */
				fm7 = kbd_table[i];
				if (fm7 > 0) {
					keyboard_break(fm7);
					bFlag = TRUE;
				}
			}

			/* データをコピー */
			kbd_map[i] = buf[i];

			/* １つでもキーを処理したら、抜ける */
			if (bFlag) {
				break;
			}
		}
	}
}

/*
 *	ポーリング＆キー情報取得
 *	※VMのロックは行っていないので注意
 */
BOOL FASTCALL GetKbd(BYTE *pBuf)
{
	HRESULT hr;

	ASSERT(pBuf);

	/* メモリクリア */
	memset(pBuf, 0, 256);

	/* DirectInputチェック */
	if (!lpkbd) {
		return FALSE;
	}

	/* デバイスのアクセス権を取得(何回取得してもよい) */
	hr = lpkbd->Acquire();
	if ((hr != DI_OK) && (hr != S_FALSE)) {
		return FALSE;
	}

	/* デバイス状態を取得 */
	if (lpkbd->GetDeviceState(256, pBuf) != DI_OK) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	ジョイスティック
 *	デバイスより読み込み
 */
static BYTE FASTCALL GetJoy(int index)
{
	int num;
	JOYINFOEX jiex;
	BYTE ret;

	/* assert */
	ASSERT((index == 0) || (index == 1));

	/* サポートジョイスティック数を取得 */
	num = joyGetNumDevs();
	if (index >= num) {
		return 0;
	}

	/* データクリア */
	ret = 0;

	/* データ取得 */
	memset(&jiex, 0, sizeof(jiex));
	jiex.dwSize = sizeof(jiex);
	jiex.dwFlags = JOY_RETURNX | JOY_RETURNY | JOY_RETURNBUTTONS |
					JOY_RETURNCENTERED;
	if (joyGetPosEx(index, &jiex) == JOYERR_NOERROR) {

		/* データ評価(方向) */
		if (jiex.dwXpos < 0x4000) {
			ret |= 0x04;
		}
		if (jiex.dwXpos > 0xc000) {
			ret |= 0x08;
		}
		if (jiex.dwYpos < 0x4000) {
			ret |= 0x01;
		}
		if (jiex.dwYpos > 0xc000) {
			ret |= 0x02;
		}

		/* データ評価(ボタン) */
		if (jiex.dwButtons & JOY_BUTTON1) {
			ret |= 0x20;
		}
		if (jiex.dwButtons & JOY_BUTTON2) {
			ret |= 0x10;
		}
	}

	return ret;
}

/*
 *	ジョイスティック
 *	連射カウンタテーブル(片側、20ms単位)
 */
static const JoyRapidCounter[] = {
	0,			/* なし */
	25,			/* 1ショット */
	12,			/* 2ショット */
	8,			/* 3ショット */
	6,			/* 4ショット */
	5,			/* 5ショット */
	4,			/* 6ショット */
	3,			/* 8ショット */
	2,			/* 12ショット */
	1			/* 25ショット */
};

/*
 *	ジョイスティック
 *	デバイスより読み込み(連射つき)
 */
static BYTE FASTCALL GetRapidJoy(int index)
{
	int i;
	BYTE bit;
	BYTE dat;

	/* assert */
	ASSERT((index == 0) || (index == 1));

	/* データ取得 */
	dat = GetJoy(index);

	/* ボタンチェック */
	bit = 0x10;
	for (i=0; i<2; i++) {
		if ((dat & bit) && (nJoyRapid[index][i] > 0)) {
			/* 連射ありで押されている。カウンタチェック */
			if (joyrapid[index][i] == 0) {
				/* 初期カウンタを代入 */
				joyrapid[index][i] = JoyRapidCounter[nJoyRapid[index][i]];
			}
			else {
				/* カウンタデクリメント */
				joyrapid[index][i]--;
				if ((joyrapid[index][i] & 0xff) == 0) {
					/* 反転タイミングなので、時間を加算して反転 */
					joyrapid[index][i] += JoyRapidCounter[nJoyRapid[index][i]];
					joyrapid[index][i] ^= 0x100;
				}
			}
			/* ボタンが押されていないように振る舞う */
			if (joyrapid[index][i] >= 0x100) {
				dat &= (BYTE)(~bit);
			}
		}
		else {
			/* ボタンが押されてないので、連射カウンタクリア */
			joyrapid[index][i] = 0;
		}
		/* 次のビットへ */
		bit <<= 1;
	}

	return dat;
}

/*
 *	ジョイスティック
 *	コード変換
 */
static BYTE FASTCALL PollJoyCode(int code)
{
	/* 0x70未満は無視 */
	if (code < 0x70) {
		return 0;
	}

	/* 0x70から上下左右 */
	switch (code) {
		/* 上 */
		case 0x70:
			return 0x01;
		/* 下 */
		case 0x71:
			return 0x02;
		/* 左 */
		case 0x72:
			return 0x04;
		/* 右 */
		case 0x73:
			return 0x08;
		/* Aボタン */
		case 0x74:
			return 0x10;
		/* Bボタン */
		case 0x75:
			return 0x20;
		/* それ以外 */
		default:
			ASSERT(FALSE);
			break;
	}

	return 0;
}

/*
 *	ジョイスティック
 *	ポーリング(ジョイスティック)
 */
static BYTE FASTCALL PollJoySub(int index, BYTE dat)
{
	int i;
	BYTE ret;
	BYTE bit;

	/* assert */
	ASSERT((index == 0) || (index == 1));

	/* 終了データクリア */
	ret = 0;

	/* 方向 */
	bit = 0x01;
	for (i=0; i<4; i++) {
		/* ボタンが押されているか */
		if (dat & bit) {
			/* コード変換 */
			ret |= PollJoyCode(nJoyCode[index][i]);
		}
		bit <<= 1;
	}

	/* センターチェック */
	if ((dat & 0x0f) == 0) {
		if ((joybk[index] & 0x0f) != 0) {
			ret |= PollJoyCode(nJoyCode[index][4]);
		}
	}

	/* ボタン */
	if (dat & 0x10) {
		ret |= PollJoyCode(nJoyCode[index][5]);
	}
	if (dat & 0x20) {
		ret |= PollJoyCode(nJoyCode[index][6]);
	}

	return ret;
}

/*
 *	ジョイスティック
 *	ポーリング(キーボード)
 */
static void FASTCALL PollJoyKbd(int index, BYTE dat)
{
	BYTE bit;
	int i;

	/* 上下左右 */
	bit = 0x01;
	for (i=0; i<4; i++) {
		if (dat & bit) {
			/* 初めて押さたら、make発行 */
			if ((joybk[index] & bit) == 0) {
				if ((nJoyCode[index][i] > 0) && (nJoyCode[index][i] <= 0x66)) {
					keyboard_make((BYTE)nJoyCode[index][i]);
				}
			}
		}
		else {
			/* 初めて離されたら、break発行 */
			if ((joybk[index] & bit) != 0) {
				if ((nJoyCode[index][i] > 0) && (nJoyCode[index][i] <= 0x66)) {
					keyboard_break((BYTE)nJoyCode[index][i]);
				}
			}
		}
		bit <<= 1;
	}

	/* センターチェック */
	if ((dat & 0x0f) == 0) {
		if ((joybk[index] & 0x0f) != 0) {
			/* make/breakを続けて出す */
			if ((nJoyCode[index][4] > 0) && (nJoyCode[index][4] <= 0x66)) {
				keyboard_make((BYTE)nJoyCode[index][4]);
				keyboard_break((BYTE)nJoyCode[index][4]);
			}
		}
	}

	/* ボタン */
	bit = 0x10;
	for (i=0; i<2; i++) {
		if (dat & bit) {
			/* 初めて押さたら、make発行 */
			if ((joybk[index] & bit) == 0) {
				if ((nJoyCode[index][i + 5] > 0) && (nJoyCode[index][i + 5] <= 0x66)) {
					keyboard_make((BYTE)nJoyCode[index][i + 5]);
				}
			}
		}
		else {
			/* 初めて離されたら、break発行 */
			if ((joybk[index] & bit) != 0) {
				if ((nJoyCode[index][i + 5] > 0) && (nJoyCode[index][i + 5] <= 0x66)) {
					keyboard_break((BYTE)nJoyCode[index][i + 5]);
				}
			}
		}
		bit <<= 1;
	}
}

/*
 *	ジョイスティック
 *	ポーリング
 */
void FASTCALL PollJoy(void)
{
	BYTE dat;
	int i;

	/* 間隔チェック */
	if ((dwExecTotal - joytime) < 20000) {
		return;
	}
	joytime = dwExecTotal;

	/* データをクリア */
	joydat[0] = 0;
	joydat[1] = 0;

	/* 無効チェック */
	if ((nJoyType[0] == 0) && (nJoyType[1] == 0)) {
		return;
	}

	/* デバイスループ */
	for (i=0; i<2; i++) {
		/* データ取得(連射つき) */
		dat = GetRapidJoy(i);

		/* タイプ別 */
		switch (nJoyType[i]) {
			/* ジョイスティックポート1 */
			case 1:
				joydat[0] = PollJoySub(i, dat);
				break;
			/* ジョイスティックポート2 */
			case 2:
				joydat[1] = PollJoySub(i, dat);
				break;
			/* キーボード */
			case 3:
				PollJoyKbd(i, dat);
				break;
		}

		/* データ更新 */
		joybk[i] = dat;
	}
}

/*
 *	ジョイスティック
 *	データリクエスト
 */
BYTE FASTCALL joy_request(BYTE no)
{
	ASSERT((no == 0) || (no == 1));

	return joydat[no];
}

#endif	/* _WIN32 */
