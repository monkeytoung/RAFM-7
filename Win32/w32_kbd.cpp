/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API �L�[�{�[�h ]
 */

#ifdef _WIN32

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define DIRECTINPUT_VERSION		0x0300		/* DirectX3���w�� */
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
 *	�O���[�o�� ���[�N
 */
BYTE kbd_map[256];							/* �L�[�{�[�h �}�b�v */
BYTE kbd_table[256];						/* �Ή�����FM-7�����R�[�h */
int nJoyType[2];							/* �W���C�X�e�B�b�N�^�C�v */
int nJoyRapid[2][2];						/* �A�˃^�C�v */
int nJoyCode[2][7];							/* �����R�[�h */

/*
 *	�X�^�e�B�b�N ���[�N
 */
static LPDIRECTINPUT lpdi;					/* DirectInput */
static LPDIRECTINPUTDEVICE lpkbd;			/* �L�[�{�[�h�f�o�C�X */
static DWORD keytime;						/* �L�[�{�[�h�|�[�����O���� */
static BYTE joydat[2];						/* �W���C�X�e�B�b�N�f�[�^ */
static BYTE joybk[2];						/* �W���C�X�e�B�b�N�o�b�N�A�b�v */
static int joyrapid[2][2];					/* �W���C�X�e�B�b�N�A�˃J�E���^ */
static DWORD joytime;						/* �W���C�X�e�B�b�N�|�[�����O���� */

/*
 *	DirectInput�R�[�h��FM-7 �����R�[�h
 *	�R�[�h�Ώƕ\(106�L�[�{�[�h�p)
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

	DIK_KANJI,			0x01,			/* ESC(���p/�S�p) */
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

	DIK_LCONTROL,		0x52,			/* CTRL(��Ctrl) */
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

	DIK_LSHIFT,			0x53,			/* ��SHIFT */
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
	DIK_RSHIFT,			0x54,			/* �ESHIFT */

	DIK_CAPITAL,		0x55,			/* CAP(Caps Lock) */
	DIK_NOCONVERT,		0x56,			/* GRAPH(���ϊ�) */
	DIK_CONVERT,		0x57,			/* ��SPACE(�ϊ�) */
	DIK_KANA,			0x58,			/* ��SPACE(�J�^�J�i) */
	DIK_SPACE,			0x35,			/* �ESPACE(SPACE) */
	DIK_RCONTROL,		0x5a,			/* ����(�ECtrl) */

	DIK_INSERT,			0x48,			/* INS(Insert) */
	DIK_DELETE,			0x4b,			/* DEL(Delete) */
	DIK_UP,				0x4d,			/* �� */
	DIK_LEFT,			0x4f,			/* �� */
	DIK_DOWN,			0x50,			/* �� */
	DIK_RIGHT,			0x51,			/* �� */

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
 *	DirectInput�R�[�h��FM-7 �����R�[�h
 *	�R�[�h�Ώƕ\(NEC PC-98�L�[�{�[�h�p)
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

	DIK_LCONTROL,		0x52,			/* CTRL(��Ctrl) */
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

	DIK_LSHIFT,			0x53,			/* ��SHIFT(SHIFT) */
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
	DIK_RSHIFT,			0x54,			/* �ESHIFT(SHIFT) */

	DIK_CAPITAL,		0x55,			/* CAP(CAPS) */
	DIK_NOCONVERT,		0x56,			/* GRAPH(NFER) */
	// ��SPACE�̓T�|�[�g���Ȃ�
	DIK_SPACE,			0x35,			/* �ESPACE(SPACE) */
	DIK_KANJI,			0x58,			/* ��SPACE(XFER) */
	DIK_KANA,			0x5a,			/* ����(�J�i) */

	DIK_INSERT,			0x48,			/* INS(Insert) */
	DIK_DELETE,			0x4b,			/* DEL(Delete) */
	DIK_UP,				0x4d,			/* �� */
	DIK_LEFT,			0x4f,			/* �� */
	DIK_DOWN,			0x50,			/* �� */
	DIK_RIGHT,			0x51,			/* �� */

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
 *	DirectInput�R�[�h��FM-7 �����R�[�h
 *	�R�[�h�Ώƕ\(101�L�[�{�[�h�p)
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

	DIK_LCONTROL,		0x52,			/* CTRL(��Ctrl) */
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
	/* ] ���蓖�ĂȂ� */

	DIK_LSHIFT,			0x53,			/* ��SHIFT */
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
	/* _ ���蓖�ĂȂ� */
	DIK_RSHIFT,			0x54,			/* �ESHIFT */

	DIK_CAPITAL,		0x55,			/* CAP(Caps Lock) */
	DIK_NUMLOCK,		0x56,			/* GRAPH(Num Lock) */
	/* ���X�y�[�X���蓖�ĂȂ� */
	/* ���X�y�[�X���蓖�ĂȂ� */
	DIK_SPACE,			0x35,			/* �ESPACE(SPACE) */
	DIK_RCONTROL,		0x5a,			/* ����(�ECtrl) */

	DIK_INSERT,			0x48,			/* INS(Insert) */
	DIK_DELETE,			0x4b,			/* DEL(Delete) */
	DIK_UP,				0x4d,			/* �� */
	DIK_LEFT,			0x4f,			/* �� */
	DIK_DOWN,			0x50,			/* �� */
	DIK_RIGHT,			0x51,			/* �� */

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
 *	�f�t�H���g�}�b�v�擾
 */
void FASTCALL GetDefMapKbd(BYTE *pMap, int mode)
{
	int i;
	int type;

	ASSERT(pMap);
	ASSERT((mode >= 0) && (mode <= 3));

	/* ��x�A���ׂăN���A */
	memset(pMap, 0, 256);

	/* �L�[�{�[�h�^�C�v�擾 */
	switch (mode) {
		/* �������� */
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
		/* ���̑� */
		default:
			ASSERT(FALSE);
			break;
	}

	/* �ϊ��e�[�u���Ƀf�t�H���g�l���Z�b�g */
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
 *	�}�b�v�ݒ�
 */
void FASTCALL SetMapKbd(BYTE *pMap)
{
	ASSERT(pMap);

	/* �R�s�[���邾�� */
	memcpy(kbd_table, pMap, sizeof(kbd_table));
}

/*
 *	������
 */
void FASTCALL InitKbd(void)
{
	/* ���[�N�G���A������(�L�[�{�[�h) */
	lpdi = NULL;
	lpkbd = NULL;
	memset(kbd_map, 0, sizeof(kbd_map));
	memset(kbd_table, 0, sizeof(kbd_table));
	keytime = 0;

	/* ���[�N�G���A������(�W���C�X�e�B�b�N) */
	joytime = 0;
	memset(joydat, 0, sizeof(joydat));
	memset(joybk, 0, sizeof(joybk));
	memset(joyrapid, 0, sizeof(joyrapid));

	/* �f�t�H���g�}�b�v��ݒ� */
	GetDefMapKbd(kbd_table, 0);
}

/*
 *	�N���[���A�b�v
 */
void FASTCALL CleanKbd(void)
{
	/* DirectInputDevice(�L�[�{�[�h)����� */
	if (lpkbd) {
		lpkbd->Unacquire();
		lpkbd->Release();
		lpkbd = NULL;
	}

	/* DirectInput����� */
	if (lpdi) {
		lpdi->Release();
		lpdi = NULL;
	}
}

/*
 *	�Z���N�g
 */
BOOL FASTCALL SelectKbd(HWND hWnd)
{
	ASSERT(hWnd);

	/* DirectInput�I�u�W�F�N�g���쐬 */
	if (FAILED(DirectInputCreate(hAppInstance, DIRECTINPUT_VERSION,
							&lpdi, NULL))) {
		return FALSE;
	}

	/* �L�[�{�[�h�f�o�C�X���쐬 */
	if (FAILED(lpdi->CreateDevice(GUID_SysKeyboard, &lpkbd, NULL))) {
		return FALSE;
	}

	/* �L�[�{�[�h�f�[�^�`����ݒ� */
	if (FAILED(lpkbd->SetDataFormat(&c_dfDIKeyboard))) {
		return FALSE;
	}

	/* �������x����ݒ� */
	if (FAILED(lpkbd->SetCooperativeLevel(hWnd,
						DISCL_BACKGROUND | DISCL_NONEXCLUSIVE))) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	�|�[�����O
 */
void FASTCALL PollKbd(void)
{
	HRESULT hr;
	BYTE buf[256];
	int i;
	BYTE fm7;
	BOOL bFlag;

	/* DirectInput�`�F�b�N */
	if (!lpkbd) {
		return;
	}

	/* �A�N�e�B�x�[�g�`�F�b�N */
	if (!bActivate) {
		return;
	}

	/* ���ԃ`�F�b�N */
	if ((dwExecTotal - keytime) < 20000) {
		return;
	}
	keytime = dwExecTotal;

	/* �f�o�C�X�̃A�N�Z�X�����擾(����擾���Ă��悢) */
	hr = lpkbd->Acquire();
	if ((hr != DI_OK) && (hr != S_FALSE)) {
		return;
	}

	/* �f�o�C�X��Ԃ��擾 */
	if (lpkbd->GetDeviceState(sizeof(buf), buf) != DI_OK) {
		return;
	}

	/* �t���Ooff */
	bFlag = FALSE;

	/* ���܂ł̏�ԂƔ�r���āA���ɕϊ����� */
	for (i=0; i<sizeof(buf); i++) {
		if ((buf[i] & 0x80) != (kbd_map[i] & 0x80)) {
			if (buf[i] & 0x80) {
				/* �L�[���� */
				fm7 = kbd_table[i];
				if (fm7 > 0) {
					if (!bMenuLoop) {
						keyboard_make(fm7);
						bFlag = TRUE;
					}
				}
			}
			else {
				/* �L�[������ */
				fm7 = kbd_table[i];
				if (fm7 > 0) {
					keyboard_break(fm7);
					bFlag = TRUE;
				}
			}

			/* �f�[�^���R�s�[ */
			kbd_map[i] = buf[i];

			/* �P�ł��L�[������������A������ */
			if (bFlag) {
				break;
			}
		}
	}
}

/*
 *	�|�[�����O���L�[���擾
 *	��VM�̃��b�N�͍s���Ă��Ȃ��̂Œ���
 */
BOOL FASTCALL GetKbd(BYTE *pBuf)
{
	HRESULT hr;

	ASSERT(pBuf);

	/* �������N���A */
	memset(pBuf, 0, 256);

	/* DirectInput�`�F�b�N */
	if (!lpkbd) {
		return FALSE;
	}

	/* �f�o�C�X�̃A�N�Z�X�����擾(����擾���Ă��悢) */
	hr = lpkbd->Acquire();
	if ((hr != DI_OK) && (hr != S_FALSE)) {
		return FALSE;
	}

	/* �f�o�C�X��Ԃ��擾 */
	if (lpkbd->GetDeviceState(256, pBuf) != DI_OK) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	�W���C�X�e�B�b�N
 *	�f�o�C�X���ǂݍ���
 */
static BYTE FASTCALL GetJoy(int index)
{
	int num;
	JOYINFOEX jiex;
	BYTE ret;

	/* assert */
	ASSERT((index == 0) || (index == 1));

	/* �T�|�[�g�W���C�X�e�B�b�N�����擾 */
	num = joyGetNumDevs();
	if (index >= num) {
		return 0;
	}

	/* �f�[�^�N���A */
	ret = 0;

	/* �f�[�^�擾 */
	memset(&jiex, 0, sizeof(jiex));
	jiex.dwSize = sizeof(jiex);
	jiex.dwFlags = JOY_RETURNX | JOY_RETURNY | JOY_RETURNBUTTONS |
					JOY_RETURNCENTERED;
	if (joyGetPosEx(index, &jiex) == JOYERR_NOERROR) {

		/* �f�[�^�]��(����) */
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

		/* �f�[�^�]��(�{�^��) */
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
 *	�W���C�X�e�B�b�N
 *	�A�˃J�E���^�e�[�u��(�Б��A20ms�P��)
 */
static const JoyRapidCounter[] = {
	0,			/* �Ȃ� */
	25,			/* 1�V���b�g */
	12,			/* 2�V���b�g */
	8,			/* 3�V���b�g */
	6,			/* 4�V���b�g */
	5,			/* 5�V���b�g */
	4,			/* 6�V���b�g */
	3,			/* 8�V���b�g */
	2,			/* 12�V���b�g */
	1			/* 25�V���b�g */
};

/*
 *	�W���C�X�e�B�b�N
 *	�f�o�C�X���ǂݍ���(�A�˂�)
 */
static BYTE FASTCALL GetRapidJoy(int index)
{
	int i;
	BYTE bit;
	BYTE dat;

	/* assert */
	ASSERT((index == 0) || (index == 1));

	/* �f�[�^�擾 */
	dat = GetJoy(index);

	/* �{�^���`�F�b�N */
	bit = 0x10;
	for (i=0; i<2; i++) {
		if ((dat & bit) && (nJoyRapid[index][i] > 0)) {
			/* �A�˂���ŉ�����Ă���B�J�E���^�`�F�b�N */
			if (joyrapid[index][i] == 0) {
				/* �����J�E���^���� */
				joyrapid[index][i] = JoyRapidCounter[nJoyRapid[index][i]];
			}
			else {
				/* �J�E���^�f�N�������g */
				joyrapid[index][i]--;
				if ((joyrapid[index][i] & 0xff) == 0) {
					/* ���]�^�C�~���O�Ȃ̂ŁA���Ԃ����Z���Ĕ��] */
					joyrapid[index][i] += JoyRapidCounter[nJoyRapid[index][i]];
					joyrapid[index][i] ^= 0x100;
				}
			}
			/* �{�^����������Ă��Ȃ��悤�ɐU�镑�� */
			if (joyrapid[index][i] >= 0x100) {
				dat &= (BYTE)(~bit);
			}
		}
		else {
			/* �{�^����������ĂȂ��̂ŁA�A�˃J�E���^�N���A */
			joyrapid[index][i] = 0;
		}
		/* ���̃r�b�g�� */
		bit <<= 1;
	}

	return dat;
}

/*
 *	�W���C�X�e�B�b�N
 *	�R�[�h�ϊ�
 */
static BYTE FASTCALL PollJoyCode(int code)
{
	/* 0x70�����͖��� */
	if (code < 0x70) {
		return 0;
	}

	/* 0x70����㉺���E */
	switch (code) {
		/* �� */
		case 0x70:
			return 0x01;
		/* �� */
		case 0x71:
			return 0x02;
		/* �� */
		case 0x72:
			return 0x04;
		/* �E */
		case 0x73:
			return 0x08;
		/* A�{�^�� */
		case 0x74:
			return 0x10;
		/* B�{�^�� */
		case 0x75:
			return 0x20;
		/* ����ȊO */
		default:
			ASSERT(FALSE);
			break;
	}

	return 0;
}

/*
 *	�W���C�X�e�B�b�N
 *	�|�[�����O(�W���C�X�e�B�b�N)
 */
static BYTE FASTCALL PollJoySub(int index, BYTE dat)
{
	int i;
	BYTE ret;
	BYTE bit;

	/* assert */
	ASSERT((index == 0) || (index == 1));

	/* �I���f�[�^�N���A */
	ret = 0;

	/* ���� */
	bit = 0x01;
	for (i=0; i<4; i++) {
		/* �{�^����������Ă��邩 */
		if (dat & bit) {
			/* �R�[�h�ϊ� */
			ret |= PollJoyCode(nJoyCode[index][i]);
		}
		bit <<= 1;
	}

	/* �Z���^�[�`�F�b�N */
	if ((dat & 0x0f) == 0) {
		if ((joybk[index] & 0x0f) != 0) {
			ret |= PollJoyCode(nJoyCode[index][4]);
		}
	}

	/* �{�^�� */
	if (dat & 0x10) {
		ret |= PollJoyCode(nJoyCode[index][5]);
	}
	if (dat & 0x20) {
		ret |= PollJoyCode(nJoyCode[index][6]);
	}

	return ret;
}

/*
 *	�W���C�X�e�B�b�N
 *	�|�[�����O(�L�[�{�[�h)
 */
static void FASTCALL PollJoyKbd(int index, BYTE dat)
{
	BYTE bit;
	int i;

	/* �㉺���E */
	bit = 0x01;
	for (i=0; i<4; i++) {
		if (dat & bit) {
			/* ���߂ĉ�������Amake���s */
			if ((joybk[index] & bit) == 0) {
				if ((nJoyCode[index][i] > 0) && (nJoyCode[index][i] <= 0x66)) {
					keyboard_make((BYTE)nJoyCode[index][i]);
				}
			}
		}
		else {
			/* ���߂ė����ꂽ��Abreak���s */
			if ((joybk[index] & bit) != 0) {
				if ((nJoyCode[index][i] > 0) && (nJoyCode[index][i] <= 0x66)) {
					keyboard_break((BYTE)nJoyCode[index][i]);
				}
			}
		}
		bit <<= 1;
	}

	/* �Z���^�[�`�F�b�N */
	if ((dat & 0x0f) == 0) {
		if ((joybk[index] & 0x0f) != 0) {
			/* make/break�𑱂��ďo�� */
			if ((nJoyCode[index][4] > 0) && (nJoyCode[index][4] <= 0x66)) {
				keyboard_make((BYTE)nJoyCode[index][4]);
				keyboard_break((BYTE)nJoyCode[index][4]);
			}
		}
	}

	/* �{�^�� */
	bit = 0x10;
	for (i=0; i<2; i++) {
		if (dat & bit) {
			/* ���߂ĉ�������Amake���s */
			if ((joybk[index] & bit) == 0) {
				if ((nJoyCode[index][i + 5] > 0) && (nJoyCode[index][i + 5] <= 0x66)) {
					keyboard_make((BYTE)nJoyCode[index][i + 5]);
				}
			}
		}
		else {
			/* ���߂ė����ꂽ��Abreak���s */
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
 *	�W���C�X�e�B�b�N
 *	�|�[�����O
 */
void FASTCALL PollJoy(void)
{
	BYTE dat;
	int i;

	/* �Ԋu�`�F�b�N */
	if ((dwExecTotal - joytime) < 20000) {
		return;
	}
	joytime = dwExecTotal;

	/* �f�[�^���N���A */
	joydat[0] = 0;
	joydat[1] = 0;

	/* �����`�F�b�N */
	if ((nJoyType[0] == 0) && (nJoyType[1] == 0)) {
		return;
	}

	/* �f�o�C�X���[�v */
	for (i=0; i<2; i++) {
		/* �f�[�^�擾(�A�˂�) */
		dat = GetRapidJoy(i);

		/* �^�C�v�� */
		switch (nJoyType[i]) {
			/* �W���C�X�e�B�b�N�|�[�g1 */
			case 1:
				joydat[0] = PollJoySub(i, dat);
				break;
			/* �W���C�X�e�B�b�N�|�[�g2 */
			case 2:
				joydat[1] = PollJoySub(i, dat);
				break;
			/* �L�[�{�[�h */
			case 3:
				PollJoyKbd(i, dat);
				break;
		}

		/* �f�[�^�X�V */
		joybk[i] = dat;
	}
}

/*
 *	�W���C�X�e�B�b�N
 *	�f�[�^���N�G�X�g
 */
BYTE FASTCALL joy_request(BYTE no)
{
	ASSERT((no == 0) || (no == 1));

	return joydat[no];
}

#endif	/* _WIN32 */
