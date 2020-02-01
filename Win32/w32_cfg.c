/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API �R���t�B�M�����[�V���� ]
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
 *	�ݒ�f�[�^��`
 */
typedef struct {
	int fm7_ver;						/* �n�[�h�E�F�A�o�[�W���� */
	BOOL cycle_steel;					/* �T�C�N���X�`�[���t���O */
	DWORD main_speed;					/* ���C��CPU�X�s�[�h */
	DWORD mmr_speed;					/* ���C��CPU(MMR)�X�s�[�h */
	BOOL bTapeFull;						/* �e�[�v���[�^���̑��x�t���O */

	int nSampleRate;					/* �T���v�����O���[�g */
	int nSoundBuffer;					/* �T�E���h�o�b�t�@�T�C�Y */
	int nBeepFreq;						/* BEEP���g�� */

	BYTE KeyMap[256];					/* �L�[�ϊ��e�[�u�� */

	int nJoyType[2];					/* �W���C�X�e�B�b�N�^�C�v */
	int nJoyRapid[2][2];				/* �W���C�X�e�B�b�N�A�� */
	int nJoyCode[2][7];					/* �W���C�X�e�B�b�N�R�[�h */

	BOOL bDD480Line;					/* �S��ʎ�640x480��D�� */
	BOOL bFullScan;						/* 400���C�����[�h */
	BOOL bDD480Status;					/* 640x480�㉺�X�e�[�^�X */

	BOOL bWHGEnable;					/* WHG�L���t���O */
	BOOL bDigitizeEnable;				/* �f�B�W�^�C�Y�L���t���O */
} configdat_t;

/*
 *	�X�^�e�B�b�N ���[�N
 */
static UINT uPropertyState;				/* �v���p�e�B�V�[�g�i�s�� */
static UINT uPropertyHelp;				/* �w���vID */
static UINT KbdPageSelectID;			/* �L�[�{�[�h�_�C�A���O */
static UINT KbdPageCurrentKey;			/* �L�[�{�[�h�_�C�A���O */
static BYTE KbdPageMap[256];			/* �L�[�{�[�h�_�C�A���O */
static UINT JoyPageIdx;					/* �W���C�X�e�B�b�N�y�[�W */ 
static configdat_t configdat;			/* �R���t�B�O�p�f�[�^ */
static configdat_t propdat;				/* �v���p�e�B�V�[�g�p�f�[�^ */
static char szIniFile[_MAX_PATH];		/* INI�t�@�C���� */
static char *pszSection;				/* �Z�N�V������ */

/*
 *	�v���g�^�C�v�錾
 */
static void FASTCALL SheetInit(HWND hDlg);

/*
 *	�R�����R���g���[���ւ̃A�N�Z�X�}�N��
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

/*-[ �ݒ�f�[�^ ]-----------------------------------------------------------*/

/*
 *	�ݒ�f�[�^
 *	�t�@�C�����w��
 */
static void FASTCALL SetCfgFile(void)
{
	char path[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];

	/* INI�t�@�C�����ݒ� */
	GetModuleFileName(NULL, path, sizeof(path));
	_splitpath(path, drive, dir, fname, NULL);
	strcpy(path, drive);
	strcat(path, dir);
	strcat(path, fname);
	strcat(path, ".INI");

	strcpy(szIniFile, path);
}

/*
 *	�ݒ�f�[�^
 *	�Z�N�V�������w��
 */
static void FASTCALL SetCfgSection(char *section)
{
	ASSERT(section);

	/* �Z�N�V�������ݒ� */
	pszSection = section;
}

/*
 *	�ݒ�f�[�^
 *	���[�h(int)
 */
static int LoadCfgInt(char *key, int def)
{
	ASSERT(key);

	return (int)GetPrivateProfileInt(pszSection, key, def, szIniFile);
}

/*
 *	�ݒ�f�[�^
 *	���[�h(BOOL)
 */
static BOOL FASTCALL LoadCfgBool(char *key, BOOL def)
{
	int dat;

	ASSERT(key);

	/* �ǂݍ��� */
	if (def) {
		dat = LoadCfgInt(key, 1);
	}
	else {
		dat = LoadCfgInt(key, 0);
	}

	/* �]�� */
	if (dat != 0) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

/*
 *	�ݒ�f�[�^
 *	���[�h
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

	/* General�Z�N�V���� */
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

	/* Sound�Z�N�V���� */
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

	/* Keyboard�Z�N�V���� */
	SetCfgSection("Keyboard");
	flag = FALSE;
	for (i=0; i<256; i++) {
		sprintf(string, "Key%d", i);
		configdat.KeyMap[i] = (BYTE)LoadCfgInt(string, 0);
		/* �ǂꂩ��ł����[�h�ł�����Aok */
		if (configdat.KeyMap[i] != 0) {
			flag = TRUE;
		}
	}
	/* �t���O���~��Ă���΁A�f�t�H���g�̃}�b�v�����炤 */
	if (!flag) {
		GetDefMapKbd(configdat.KeyMap, 0);
	}

	/* JoyStick�Z�N�V���� */
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
		/* �����W�G���[�Ȃ珉���l�ݒ� */
		if (!flag) {
			for (j=0; j<7; j++) {
				configdat.nJoyCode[i][j] = JoyTable[j];
			}
		}
	}

	/* Screen�Z�N�V���� */
	SetCfgSection("Screen");
	configdat.bDD480Line = LoadCfgBool("DD480Line", FALSE);
	configdat.bFullScan = LoadCfgBool("FullScan", FALSE);
	configdat.bDD480Status = LoadCfgBool("DD480Status", TRUE);

	/* Option�Z�N�V���� */
	SetCfgSection("Option");
	configdat.bWHGEnable = LoadCfgBool("WHGEnable", TRUE);
	configdat.bDigitizeEnable = LoadCfgBool("DigitizeEnable", TRUE);
}

/*
 *	�ݒ�f�[�^
 *	�Z�[�u(������)
 */
static void FASTCALL SaveCfgString(char *key, char *string)
{
	ASSERT(key);
	ASSERT(string);

	WritePrivateProfileString(pszSection, key, string, szIniFile);
}

/*
 *	�ݒ�f�[�^
 *	�Z�[�u(�S�o�C�gint)
 */
static void FASTCALL SaveCfgInt(char *key, int dat)
{
	char string[128];

	ASSERT(key);

	sprintf(string, "%d", dat);
	SaveCfgString(key, string);
}

/*
 *	�ݒ�f�[�^
 *	�Z�[�u(BOOL)
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
 *	�ݒ�f�[�^
 *	�Z�[�u
 */
void FASTCALL SaveCfg(void)
{
	int i;
	int j;
	char string[128];

	SetCfgFile();

	/* General�Z�N�V���� */
	SetCfgSection("General");
	SaveCfgInt("Version", configdat.fm7_ver);
	SaveCfgBool("CycleSteel", configdat.cycle_steel);
	SaveCfgInt("MainSpeed", configdat.main_speed);
	SaveCfgInt("MMRSpeed", configdat.mmr_speed);
	SaveCfgBool("TapeFullSpeed", configdat.bTapeFull);

	/* Sound�Z�N�V���� */
	SetCfgSection("Sound");
	SaveCfgInt("SampleRate", configdat.nSampleRate);
	SaveCfgInt("SoundBuffer", configdat.nSoundBuffer);
	SaveCfgInt("BeepFreq", configdat.nBeepFreq);

	/* Keyboard�Z�N�V���� */
	SetCfgSection("Keyboard");
	for (i=0; i<256; i++) {
		if (configdat.KeyMap[i] != 0) {
			sprintf(string, "Key%d", i);
			SaveCfgInt(string, configdat.KeyMap[i]);
		}
	}

	/* JoyStick�Z�N�V���� */
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

	/* Screen�Z�N�V���� */
	SetCfgSection("Screen");
	SaveCfgBool("DD480Line", configdat.bDD480Line);
	SaveCfgBool("FullScan", configdat.bFullScan);
	SaveCfgBool("DD480Status", configdat.bDD480Status);

	/* Option�Z�N�V���� */
	SetCfgSection("Option");
	SaveCfgBool("WHGEnable", configdat.bWHGEnable);
	SaveCfgBool("DigitizeEnable", configdat.bDigitizeEnable);
}

/*
 *	�ݒ�f�[�^�K�p
 *	��VM�̃��b�N�͍s���Ă��Ȃ��̂Œ���
 */
void FASTCALL ApplyCfg(void)
{
	/* General�Z�N�V���� */
	fm7_ver = configdat.fm7_ver;
	cycle_steel = configdat.cycle_steel;
	main_speed = configdat.main_speed * 10;
	mmr_speed = configdat.mmr_speed * 10;
	bTapeFullSpeed = configdat.bTapeFull;

	/* Sound�Z�N�V���� */
	nSampleRate = configdat.nSampleRate;
	nSoundBuffer = configdat.nSoundBuffer;
	nBeepFreq = configdat.nBeepFreq;
	ApplySnd();

	/* Keyboard�Z�N�V���� */
	SetMapKbd(configdat.KeyMap);

	/* JoyStick�Z�N�V���� */
	memcpy(nJoyType, configdat.nJoyType, sizeof(nJoyType));
	memcpy(nJoyRapid, configdat.nJoyRapid, sizeof(nJoyRapid));
	memcpy(nJoyCode, configdat.nJoyCode, sizeof(nJoyCode));

	/* Screen�Z�N�V���� */
	bDD480Line = configdat.bDD480Line;
	bFullScan = configdat.bFullScan;
	bDD480Status = configdat.bDD480Status;
	InvalidateRect(hDrawWnd, NULL, FALSE);

	/* Option�Z�N�V���� */
	whg_enable = configdat.bWHGEnable;
	digitize_enable = configdat.bDigitizeEnable;
}

/*-[ �w���v�T�|�[�g ]-------------------------------------------------------*/

/*
 *	�T�|�[�gID���X�g
 *	(��ɗ�����̂قǗD��)
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
 *	�y�[�W�w���v
 */
static void FASTCALL PageHelp(HWND hDlg, UINT uID)
{
	POINT point;
	RECT rect;
	HWND hWnd;
	int i;
	char string[128];

	ASSERT(hDlg);

	/* �|�C���g�쐬 */
	GetCursorPos(&point);

	/* �w���v���X�g�ɍڂ��Ă���ID����� */
	for (i=0; ;i++) {
		/* �I���`�F�b�N */
		if (PageHelpList[i] == 0) {
			break;
		}

		/* �E�C���h�E�n���h���擾 */
		hWnd = GetDlgItem(hDlg, PageHelpList[i]);
		if (!hWnd) {
			continue;
		}
		if (!IsWindowVisible(hWnd)) {
			continue;
		}

		/* ��`�𓾂āAPtInRect�Ń`�F�b�N���� */
		GetWindowRect(hWnd, &rect);
		if (!PtInRect(&rect, point)) {
			continue;
		}

		/* �L���b�V���`�F�b�N */
		if (PageHelpList[i] == uPropertyHelp) {
			return;
		}
		uPropertyHelp = PageHelpList[i];

		/* �����񃊃\�[�X�����[�h�A�ݒ� */
		string[0] = '\0';
		LoadString(hAppInstance, uPropertyHelp, string, sizeof(string));
		hWnd = GetDlgItem(hDlg, uID);
		if (hWnd) {
			SetWindowText(hWnd, string);
		}

		/* �ݒ�I�� */
		return;
	}

	/* �w���v���X�g�͈͊O�̋�`�B������Ȃ� */
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

/*-[ �S�ʃy�[�W ]-----------------------------------------------------------*/

/*
 *	�S�ʃy�[�W
 *	�_�C�A���O������
 */
static void FASTCALL GeneralPageInit(HWND hDlg)
{
	HWND hWnd;
	char string[128];

	ASSERT(hDlg);

	/* �V�[�g������ */
	SheetInit(hDlg);

	/* ����@�� */
	if (propdat.fm7_ver == 1) {
		CheckDlgButton(hDlg, IDC_GP_FM7, BST_CHECKED);
	}
	else {
		CheckDlgButton(hDlg, IDC_GP_FM77AV, BST_CHECKED);
	}

	/* ���샂�[�h */
	if (propdat.cycle_steel == TRUE) {
		CheckDlgButton(hDlg, IDC_GP_HIGHSPEED, BST_CHECKED);
	}
	else {
		CheckDlgButton(hDlg, IDC_GP_LOWSPEED, BST_CHECKED);
	}

	/* CPU�I�� */
	hWnd = GetDlgItem(hDlg, IDC_GP_CPUCOMBO);
	ASSERT(hWnd);
	string[0] = '\0';
	ComboBox_ResetContent(hWnd);
	LoadString(hAppInstance, IDS_GP_MAINCPU, string, sizeof(string));
	ComboBox_AddString(hWnd, string);
	LoadString(hAppInstance, IDS_GP_MAINMMR, string, sizeof(string));
	ComboBox_AddString(hWnd, string);
	ComboBox_SetCurSel(hWnd, 0);

	/* CPU���x */
	hWnd = GetDlgItem(hDlg, IDC_GP_CPUSPIN);
	ASSERT(hWnd);
	UpDown_SetRange(hWnd, 9999, 0);
	UpDown_SetPos(hWnd, propdat.main_speed);

	/* �e�[�v���[�^�t���O */
	if (propdat.bTapeFull) {
		CheckDlgButton(hDlg, IDC_GP_TAPESPEED, BST_CHECKED);
	}
	else {
		CheckDlgButton(hDlg, IDC_GP_TAPESPEED, BST_UNCHECKED);
	}
}

/*
 *	�S�ʃy�[�W
 *	�R�}���h
 */
static void FASTCALL GeneralPageCmd(HWND hDlg, WORD wID, WORD wNotifyCode, HWND hWnd)
{
	int index;
	HWND hSpin;

	ASSERT(hDlg);

	/* ID�� */
	switch (wID) {
		/* CPU�I���R���{�{�b�N�X */
		case IDC_GP_CPUCOMBO:
			/* �I��Ώۂ��ς������A�V�����l�����[�h */
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
 *	�S�ʃy�[�W
 *	�����X�N���[��
 */
static void FASTCALL GeneralPageVScroll(HWND hDlg, WORD wPos, HWND hWnd)
{
	int index;

	/* �`�F�b�N */
	if (hWnd != GetDlgItem(hDlg, IDC_GP_CPUSPIN)) {
		return;
	}

	/* �l���i�[ */
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
 *	�S�ʃy�[�W
 *	�K�p
 */
static void FASTCALL GeneralPageApply(HWND hDlg)
{
	/* �X�e�[�g�ύX */
	uPropertyState = 2;

	/* FM-7�o�[�W���� */
	if (IsDlgButtonChecked(hDlg, IDC_GP_FM7) == BST_CHECKED) {
		propdat.fm7_ver = 1;
	}
	else {
		propdat.fm7_ver = 2;
	}

	/* �T�C�N���X�`�[�� */
	if (IsDlgButtonChecked(hDlg, IDC_GP_HIGHSPEED) == BST_CHECKED) {
		propdat.cycle_steel = TRUE;
	}
	else {
		propdat.cycle_steel = FALSE;
	}

	/* �e�[�v�������[�h */
	if (IsDlgButtonChecked(hDlg, IDC_GP_TAPESPEED) == BST_CHECKED) {
		propdat.bTapeFull = TRUE;
	}
	else {
		propdat.bTapeFull = FALSE;
	}
}

/*
 *	�S�ʃy�[�W
 *	�_�C�A���O�v���V�[�W��
 */
static BOOL CALLBACK GeneralPageProc(HWND hDlg, UINT msg,
									 WPARAM wParam, LPARAM lParam)
{
	LPNMHDR pnmh;

	switch (msg) {
		/* ������ */
		case WM_INITDIALOG:
			GeneralPageInit(hDlg);
			return TRUE;

		/* �R�}���h */
		case WM_COMMAND:
			GeneralPageCmd(hDlg, LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
			return TRUE;

		/* �ʒm */
		case WM_NOTIFY:
			pnmh = (LPNMHDR)lParam;
			if (pnmh->code == PSN_APPLY) {
				GeneralPageApply(hDlg);
				return TRUE;
			}
			break;

		/* �����X�N���[�� */
		case WM_VSCROLL:
			GeneralPageVScroll(hDlg, HIWORD(wParam), (HWND)lParam);
			break;

		/* �J�[�\���ݒ� */
		case WM_SETCURSOR:
			if (HIWORD(lParam) == WM_MOUSEMOVE) {
				PageHelp(hDlg, IDC_GP_HELP);
			}
			break;
	}

	return FALSE;
}

/*
 *	�S�ʃy�[�W
 *	�쐬
 */
static HPROPSHEETPAGE FASTCALL GeneralPageCreate(void)
{
	PROPSHEETPAGE psp;

	/* �\���̂��쐬 */
	memset(&psp, 0, sizeof(PROPSHEETPAGE));
	psp.dwSize = PROPSHEETPAGE_V1_SIZE;
	psp.dwFlags = 0;
	psp.hInstance = hAppInstance;
	psp.u.pszTemplate = MAKEINTRESOURCE(IDD_GENERALPAGE);
	psp.pfnDlgProc = GeneralPageProc;

	return CreatePropertySheetPage(&psp);
}

/*-[ �T�E���h�y�[�W ]-------------------------------------------------------*/

/*
 *	�T�E���h�y�[�W
 *	�����X�N���[��
 */
static void FASTCALL SoundPageVScroll(HWND hDlg, WORD wPos, HWND hWnd)
{
	HWND hBuddyWnd;
	char string[128];

	ASSERT(hDlg);
	ASSERT(hWnd);

	/* �E�C���h�E�n���h�����`�F�b�N */
	if (hWnd != GetDlgItem(hDlg, IDC_SP_BUFSPIN)) {
		return;
	}

	/* �|�W�V��������A�o�f�B�E�C���h�E�ɒl��ݒ� */
	hBuddyWnd = GetDlgItem(hDlg, IDC_SP_BUFEDIT);
	ASSERT(hBuddyWnd);
	sprintf(string, "%d", wPos * 10);
	SetWindowText(hBuddyWnd, string);
}

/*
 *	�T�E���h�y�[�W
 *	�_�C�A���O������
 */
static void FASTCALL SoundPageInit(HWND hDlg)
{
	HWND hWnd;

	/* �V�[�g������ */
	SheetInit(hDlg);

	/* �T���v�����O���[�g */
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

	/* �T�E���h�o�b�t�@ */
	hWnd = GetDlgItem(hDlg, IDC_SP_BUFSPIN);
	ASSERT(hWnd);
	UpDown_SetRange(hWnd, 100, 2);
	UpDown_SetPos(hWnd, propdat.nSoundBuffer / 10);
	SoundPageVScroll(hDlg, LOWORD(UpDown_GetPos(hWnd)), hWnd);

	/* BEEP���g�� */
	hWnd = GetDlgItem(hDlg, IDC_SP_BEEPSPIN);
	ASSERT(hWnd);
	UpDown_SetRange(hWnd, 9999, 100);
	UpDown_SetPos(hWnd, propdat.nBeepFreq);
}

/*
 *	�T�E���h�y�[�W
 *	�K�p
 */
static void FASTCALL SoundPageApply(HWND hDlg)
{
	HWND hWnd;
	UINT uPos;

	/* �X�e�[�g�ύX */
	uPropertyState = 2;

	/* �T���v�����O���[�g */
	if (IsDlgButtonChecked(hDlg, IDC_SP_44K) == BST_CHECKED) {
		propdat.nSampleRate = 44100;
	}
	if (IsDlgButtonChecked(hDlg, IDC_SP_22K) == BST_CHECKED) {
		propdat.nSampleRate = 22050;
	}
	if (IsDlgButtonChecked(hDlg, IDC_SP_NONE) == BST_CHECKED) {
		propdat.nSampleRate = 0;
	}

	/* �T�E���h�o�b�t�@ */
	hWnd = GetDlgItem(hDlg, IDC_SP_BUFSPIN);
	ASSERT(hWnd);
	uPos = LOWORD(UpDown_GetPos(hWnd));
	propdat.nSoundBuffer = uPos * 10;

	/* BEEP���g�� */
	hWnd = GetDlgItem(hDlg, IDC_SP_BEEPSPIN);
	ASSERT(hWnd);
	propdat.nBeepFreq = LOWORD(UpDown_GetPos(hWnd));
}

/*
 *	�T�E���h�y�[�W
 *	�_�C�A���O�v���V�[�W��
 */
static BOOL CALLBACK SoundPageProc(HWND hDlg, UINT msg,
									 WPARAM wParam, LPARAM lParam)
{
	LPNMHDR pnmh;

	switch (msg) {
		/* ������ */
		case WM_INITDIALOG:
			SoundPageInit(hDlg);
			return TRUE;

		/* �ʒm */
		case WM_NOTIFY:
			pnmh = (LPNMHDR)lParam;
			if (pnmh->code == PSN_APPLY) {
				SoundPageApply(hDlg);
				return TRUE;
			}
			break;

		/* �����X�N���[�� */
		case WM_VSCROLL:
			SoundPageVScroll(hDlg, HIWORD(wParam), (HWND)lParam);
			break;

		/* �J�[�\���ݒ� */
		case WM_SETCURSOR:
			if (HIWORD(lParam) == WM_MOUSEMOVE) {
				PageHelp(hDlg, IDC_SP_HELP);
			}
			break;
	}

	return FALSE;
}

/*
 *	�T�E���h�y�[�W
 *	�쐬
 */
static HPROPSHEETPAGE FASTCALL SoundPageCreate(void)
{
	PROPSHEETPAGE psp;

	/* �\���̂��쐬 */
	memset(&psp, 0, sizeof(PROPSHEETPAGE));
	psp.dwSize = PROPSHEETPAGE_V1_SIZE;
	psp.dwFlags = 0;
	psp.hInstance = hAppInstance;
	psp.u.pszTemplate = MAKEINTRESOURCE(IDD_SOUNDPAGE);
	psp.pfnDlgProc = SoundPageProc;

	return CreatePropertySheetPage(&psp);
}

/*-[ �L�[���̓_�C�A���O ]---------------------------------------------------*/

/*
 *	�L�[�{�[�h�y�[�W
 *	DirectInput �L�[�R�[�h�e�[�u��
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
 *	�L�[�{�[�h�y�[�W
 *	FM77AV �L�[�R�[�h�e�[�u��
 */
static char *KbdPageFM77AV[] = {
	NULL, NULL,				/* 0x00 */
	"ESC", NULL,			/* 0x01 */
	"1", "��",				/* 0x02 */
	"2", "��",				/* 0x03 */
	"3", "��",				/* 0x04 */
	"4", "��",				/* 0x05 */
	"5", "��",				/* 0x06 */
	"6", "��",				/* 0x07 */
	"7", "��",				/* 0x08 */
	"8", "��",				/* 0x09 */
	"9", "��",				/* 0x0A */
	"0", "��",				/* 0x0B */
	"-", "��",				/* 0x0C */
	"^", "��",				/* 0x0D */
	"\\", "�[",				/* 0x0E */
	"BS", NULL,				/* 0x0F */
	"TAB", NULL,			/* 0x10 */
	"Q", "��",				/* 0x11 */
	"W", "��",				/* 0x12 */
	"E", "��",				/* 0x13 */
	"R", "��",				/* 0x14 */
	"T", "��",				/* 0x15 */
	"Y", "��",				/* 0x16 */
	"U", "��",				/* 0x17 */
	"I", "��",				/* 0x18 */
	"O", "��",				/* 0x19 */
	"P", "��",				/* 0x1A */
	"@", "�J",				/* 0x1B */
	"[", "�K",				/* 0x1C */
	"RETURN", NULL,			/* 0x1D */
	"A", "��",				/* 0x1E */
	"S", "��",				/* 0x1F */
	"D", "��",				/* 0x20 */
	"F", "��",				/* 0x21 */
	"G", "��",				/* 0x22 */
	"H", "��",				/* 0x23 */
	"J", "��",				/* 0x24 */
	"K", "��",				/* 0x25 */
	"L", "��",				/* 0x26 */
	";", "��",				/* 0x27 */
	":", "��",				/* 0x28 */
	"]", "��",				/* 0x29 */
	"Z", "��",				/* 0x2A */
	"X", "��",				/* 0x2B */
	"C", "��",				/* 0x2C */
	"V", "��",				/* 0x2D */
	"B", "��",				/* 0x2E */
	"N", "��",				/* 0x2F */
	"M", "��",				/* 0x30 */
	",", "��",				/* 0x31 */
	".", "��",				/* 0x32 */
	"/", "��",				/* 0x33 */
	"_", "��",				/* 0x34 */
	"SPACE(�E)", NULL,		/* 0x35 */
	"*", "�e���L�[",		/* 0x36 */
	"/", "�e���L�[",		/* 0x37 */
	"+", "�e���L�[",		/* 0x38 */
	"-", "�e���L�[",		/* 0x39 */
	"7", "�e���L�[",		/* 0x3A */
	"8", "�e���L�[",		/* 0x3B */
	"9", "�e���L�[",		/* 0x3C */
	"=", "�e���L�[",		/* 0x3D */
	"4", "�e���L�[",		/* 0x3E */
	"5", "�e���L�[",		/* 0x3F */
	"6", "�e���L�[",		/* 0x40 */
	",", "�e���L�[",		/* 0x41 */
	"1", "�e���L�[",		/* 0x42 */
	"2", "�e���L�[",		/* 0x43 */
	"3", "�e���L�[",		/* 0x44 */
	"RETURN", "�e���L�[",	/* 0x45 */
	"0", "�e���L�[",		/* 0x46 */
	".", "�e���L�[",		/* 0x47 */
	"INS", NULL,			/* 0x48 */
	"EL", NULL,				/* 0x49 */
	"CLS", NULL,			/* 0x4A */
	"DEL", NULL,			/* 0x4B */
	"DUP", NULL,			/* 0x4C */
	"��", NULL,				/* 0x4D */
	"HOME", NULL,			/* 0x4E */
	"��", NULL,				/* 0x4F */
	"��", NULL,				/* 0x50 */
	"��", NULL,				/* 0x51 */
	"CTRL", NULL,			/* 0x52 */
	"SHIFT(��)", NULL,		/* 0x53 */
	"SHIFT(�E)", NULL,		/* 0x54 */
	"CAP", NULL,			/* 0x55 */
	"GRAPH", NULL,			/* 0x56 */
	"SPACE(��)", NULL,		/* 0x57 */
	"SPACE(��)", NULL,		/* 0x58 */
	NULL, NULL,				/* 0x59 */
	"����", NULL,			/* 0x5A */
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
 *	�C���f�b�N�X��FM77AV �L�[�R�[�h
 */
static int FASTCALL KbdPageIndex2FM77AV(int index)
{
	int i;

	for (i=0; i<sizeof(KbdPageFM77AV)/sizeof(char *)/2; i++) {
		/* NULL�̃L�[�̓X�L�b�v */
		if (KbdPageFM77AV[i * 2] == NULL) {
			continue;
		}

		/* NULL�łȂ���΁A�`�F�b�N���f�N�������g */
		if (index == 0) {
			return i;
		}
		index--;
	}

	/* �G���[ */
	return 0;
}

/*
 *	FM77AV �L�[�R�[�h���C���f�b�N�X
 */
static int FASTCALL KbdPageFM77AV2Index(int keycode)
{
	int i;
	int index;

	index = 0;
	for (i=0; i<sizeof(KbdPageFM77AV)/sizeof(char *)/2; i++) {
		/* �L�[�R�[�h�ɓ��B������I�� */
		if (i == keycode) {
			break;
		}

		/* NULL�̃L�[�̓X�L�b�v */
		if (KbdPageFM77AV[i * 2] == NULL) {
			continue;
		}

		/* NULL�łȂ���΁A�C���N�������g */
		index++;
	}

	return index;
}

/*
 *	FM77AV �L�[�R�[�h��DirectInput �L�[�R�[�h
 */
static int FASTCALL KbdPageFM77AV2DirectInput(int fm)
{
	int i;

	/* ���� */
	for (i=0; i<256; i++) {
		if (propdat.KeyMap[i] == fm) {
			return i;
		}
	}

	/* �G���[ */
	return 0;
}

/*
 *	�L�[���̓_�C�A���O
 *	�_�C�A���O������
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

	/* �e�E�C���h�E�̒����ɐݒ� */
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

	/* �L�[�ԍ��e�L�X�g������ */
	fm = KbdPageIndex2FM77AV(KbdPageSelectID);
	formstr[0] = '\0';
	LoadString(hAppInstance, IDC_KEYIN_LABEL, formstr, sizeof(formstr));
	sprintf(string, formstr, fm);
	hWnd = GetDlgItem(hDlg, IDC_KEYIN_LABEL);
	SetWindowText(hWnd, string);

	/* DirectInput�L�[�e�L�X�g������ */
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

	/* �O���[�o�����[�N������ */
	KbdPageCurrentKey = di;
	GetKbd(KbdPageMap);

	/* �^�C�}�[���X�^�[�g */
	SetTimer(hDlg, IDD_KEYINDLG, 50, NULL);
}

/*
 *	�L�[���̓_�C�A���O
 *	�^�C�}�[
 */
static void FASTCALL KeyInTimer(HWND hDlg)
{
	BYTE buf[256];
	int i;
	HWND hWnd;
	char string[128];

	ASSERT(hDlg);

	/* �L�[���� */
	LockVM();
	GetKbd(buf);
	UnlockVM();

	/* ���񉟂��ꂽ�L�[����� */
	for (i=0; i<256; i++) {
		if ((KbdPageMap[i] < 0x80) && (buf[i] >= 0x80)) {
			break;
		}
	}

	/* �L�[�}�b�v���X�V���A�`�F�b�N */
	memcpy(KbdPageMap, buf, 256);
	if (i >= 256) {
		return;
	}

	/* �L�[�̔ԍ��A�e�L�X�g���Z�b�g */
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
 *	�L�[���̓_�C�A���O
 *	�_�C�A���O�v���V�[�W��
 */
static BOOL CALLBACK KeyInDlgProc(HWND hDlg, UINT iMsg,
									WPARAM wParam, LPARAM lParam)
{
	switch (iMsg) {
		/* �_�C�A���O������ */
		case WM_INITDIALOG:
			KeyInDlgInit(hDlg);
			return TRUE;

		/* �R�}���h */
		case WM_COMMAND:
			if (LOWORD(wParam) != IDCANCEL) {
				return TRUE;
			}
			/* ESC�L�[�����m���邽�߂̍H�v */
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

		/* �^�C�}�[ */
		case WM_TIMER:
			KeyInTimer(hDlg);
			return TRUE;

		/* �E�N���b�N */
		case WM_RBUTTONDOWN:
			EndDialog(hDlg, IDOK);
			return TRUE;

		/* �E�C���h�E�j�� */
		case WM_DESTROY:
			KillTimer(hDlg, IDD_KEYINDLG);
			break;

	}

	/* ����ȊO��FALSE */
	return FALSE;
}

/*-[ �L�[�{�[�h�y�[�W ]-----------------------------------------------------*/

/*
 *	�L�[�{�[�h�y�[�W
 *	�_�C�A���O������(�w�b�_�[�A�C�e��)
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

	/* �e�L�X�g���g���b�N���擾 */
	hDC = GetDC(hWnd);
	GetTextMetrics(hDC, &tm);
	ReleaseDC(hWnd, hDC);

	/* �}�����[�v */
	for (i=0; i<(sizeof(uHeaderTable)/sizeof(UINT)); i++) {
		/* �e�L�X�g�����[�h */
		LoadString(hAppInstance, uHeaderTable[i], string, sizeof(string));

		/* �J�����\���̂��쐬 */
		lvc.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_WIDTH | LVCF_TEXT;
		lvc.iSubItem = i;
		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = (strlen(string) + 1) * tm.tmAveCharWidth;
		lvc.pszText = string;
		lvc.cchTextMax = strlen(string);

		/* �J�����}�� */
		ListView_InsertColumn(hWnd, i, &lvc);
	}
}

/*
 *	�L�[�{�[�h�y�[�W
 *	�T�u�A�C�e���ꊇ�ݒ�
 */
static void FASTCALL KbdPageSubItem(HWND hDlg)
{
	HWND hWnd;
	int i;
	int j;
	int index;

	/* ���X�g�R���g���[���擾 */
	hWnd = GetDlgItem(hDlg, IDC_KP_LIST);
	ASSERT(hWnd);

	/* �A�C�e���}�� */
	for (index=0; ; index++) {
		/* FM77AV�L�[�ԍ��𓾂� */
		i = KbdPageIndex2FM77AV(index);
		if (i == 0) {
			break;
		}

		/* �Y������DirectX�L�[�ԍ��𓾂� */
		j = KbdPageFM77AV2DirectInput(i);

		/* ������Z�b�g */
		if (KbdPageDirectInput[j]) {
			ListView_SetItemText(hWnd, index, 3, KbdPageDirectInput[j]);
		}
		else {
			ListView_SetItemText(hWnd, index, 3, "");
		}
	}
}

/*
 *	�L�[�{�[�h�y�[�W
 *	�_�C�A���O������
 */
static void FASTCALL KbdPageInit(HWND hDlg)
{
	HWND hWnd;
	LV_ITEM lvi;
	int index;
	int i;
	char string[128];

	/* �V�[�g������ */
	SheetInit(hDlg);

	/* ���X�g�R���g���[���擾 */
	hWnd = GetDlgItem(hDlg, IDC_KP_LIST);
	ASSERT(hWnd);

	/* �J�����}�� */
	KbdPageInitColumn(hWnd);

	/* �A�C�e���}�� */
	for (index=0; ; index++) {
		/* FM77AV�L�[�ԍ��𓾂� */
		i = KbdPageIndex2FM77AV(index);
		if (i == 0) {
			break;
		}

		/* �A�C�e���}�� */
		sprintf(string, "%02X", i);
		lvi.mask = LVIF_TEXT;
		lvi.pszText = string;
		lvi.cchTextMax = strlen(string);
		lvi.iItem = index;
		lvi.iSubItem = 0;
		ListView_InsertItem(hWnd, &lvi);

		/* �T�u�A�C�e���~�Q���Z�b�g */
		if (KbdPageFM77AV[i * 2 + 0]) {
			ListView_SetItemText(hWnd, index, 1, KbdPageFM77AV[i * 2 + 0]);
		}
		if (KbdPageFM77AV[i * 2 + 1]) {
			ListView_SetItemText(hWnd, index, 2, KbdPageFM77AV[i * 2 + 1]);
		}
	}

	/* �T�u�A�C�e�� */
	KbdPageSubItem(hDlg);
}

/*
 *	�L�[�{�[�h�y�[�W
 *	�R�}���h
 */
static void FASTCALL KbdPageCmd(HWND hDlg, WORD wID, WORD wNotifyCode, HWND hWnd)
{
	ASSERT(hDlg);

	switch (wID) {
		/* 106�L�[�{�[�h */
		case IDC_KP_106B:
			GetDefMapKbd(propdat.KeyMap, 1);
			KbdPageSubItem(hDlg);
			break;

		/* PC-98�L�[�{�[�h */
		case IDC_KP_98B:
			GetDefMapKbd(propdat.KeyMap, 2);
			KbdPageSubItem(hDlg);
			break;

		/* 101�L�[�{�[�h */
		case IDC_KP_101B:
			GetDefMapKbd(propdat.KeyMap, 3);
			KbdPageSubItem(hDlg);
			break;
	}
}

/*
 *	�L�[�{�[�h�y�[�W
 *	�_�u���N���b�N
 */
static void FASTCALL KbdPageDblClk(HWND hDlg)
{
	HWND hWnd;
	int count;
	int i;
	int j;

	ASSERT(hDlg);

	/* ���X�g�R���g���[���擾 */
	hWnd = GetDlgItem(hDlg, IDC_KP_LIST);
	ASSERT(hWnd);

	/* �I������Ă���A�C�e����������΁A���^�[�� */
	if (ListView_GetSelectedCount(hWnd) == 0) {
		return;
	}

	/* �o�^����Ă���A�C�e���̌����擾 */
	count = ListView_GetItemCount(hWnd);

	/* �Z���N�g����Ă���C���f�b�N�X�𓾂� */
	for (i=0; i<count; i++) {
		if (ListView_GetItemState(hWnd, i, LVIS_SELECTED)) {
			break;
		}
	}

	/* �O���[�o���֋L�� */
	ASSERT(i < count);
	KbdPageSelectID = i;

	/* ���[�_���_�C�A���O���s */
	if (DialogBox(hAppInstance, MAKEINTRESOURCE(IDD_KEYINDLG),
						hDlg, KeyInDlgProc) != IDOK) {
		return;
	}

	/* �}�b�v���C�� */
	j = KbdPageIndex2FM77AV(i);
	for (i=0; i<256; i++) {
		if (propdat.KeyMap[i] == j) {
			propdat.KeyMap[i] = 0;
		}
	}
	propdat.KeyMap[KbdPageCurrentKey] = (BYTE)j;

	/* �ĕ`�� */
	KbdPageSubItem(hDlg);
}

/*
 *	�L�[�{�[�h�y�[�W
 *	�K�p
 */
static void FASTCALL KbdPageApply(HWND hDlg)
{
	/* �X�e�[�g�ύX */
	uPropertyState = 2;
}

/*
 *	�L�[�{�[�h�y�[�W
 *	�_�C�A���O�v���V�[�W��
 */
static BOOL CALLBACK KbdPageProc(HWND hDlg, UINT msg,
									 WPARAM wParam, LPARAM lParam)
{
	LPNMHDR pnmh;
	LV_KEYDOWN *plkd;

	switch (msg) {
		/* ������ */
		case WM_INITDIALOG:
			KbdPageInit(hDlg);
			return TRUE;

		/* �R�}���h */
		case WM_COMMAND:
			KbdPageCmd(hDlg, LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
			return TRUE;

		/* �ʒm */
		case WM_NOTIFY:
			pnmh = (LPNMHDR)lParam;
			/* �y�[�W�I�� */
			if (pnmh->code == PSN_APPLY) {
				KbdPageApply(hDlg);
				return TRUE;
			}
			/* ���X�g�r���[ �_�u���N���b�N */
			if ((pnmh->idFrom == IDC_KP_LIST) && (pnmh->code == NM_DBLCLK)) {
				KbdPageDblClk(hDlg);
				return TRUE;
			}
			/* ���X�g�r���[ ����L�[���� */
			if ((pnmh->idFrom == IDC_KP_LIST) && (pnmh->code == LVN_KEYDOWN)) {
				plkd = (LV_KEYDOWN*)pnmh;
				/* SPACE�����͂��ꂽ��L�[�I�� */
				if (plkd->wVKey == VK_SPACE) {
					KbdPageDblClk(hDlg);
					return TRUE;
				}
			}
			break;

		/* �J�[�\���ݒ� */
		case WM_SETCURSOR:
			if (HIWORD(lParam) == WM_MOUSEMOVE) {
				PageHelp(hDlg, IDC_KP_HELP);
			}
			break;
	}

	return FALSE;
}

/*
 *	�L�[�{�[�h�y�[�W
 *	�쐬
 */
static HPROPSHEETPAGE FASTCALL KbdPageCreate(void)
{
	PROPSHEETPAGE psp;

	/* �\���̂��쐬 */
	memset(&psp, 0, sizeof(PROPSHEETPAGE));
	psp.dwSize = PROPSHEETPAGE_V1_SIZE;
	psp.dwFlags = 0;
	psp.hInstance = hAppInstance;
	psp.u.pszTemplate = MAKEINTRESOURCE(IDD_KBDPAGE);
	psp.pfnDlgProc = KbdPageProc;

	return CreatePropertySheetPage(&psp);
}

/*-[ �W���C�X�e�B�b�N�y�[�W ]-----------------------------------------------*/

/*
 *	�W���C�X�e�B�b�N�y�[�W
 *	�R���{�{�b�N�X�e�[�u��
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
 *	�W���C�X�e�B�b�N�y�[�W
 *	�R�[�h�e�[�u��
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
 *	�W���C�X�e�B�b�N�y�[�W
 *	�R���{�{�b�N�X�Z���N�g
 */
static void FASTCALL JoyPageCombo(HWND hDlg, WORD wID)
{
	int i;
	int j;
	HWND hWnd;
	char string[128];

	ASSERT(hDlg);

	/* ID�`�F�b�N */
	if (wID != IDC_JP_TYPEC) {
		return;
	}

	/* "�g�p���Ȃ�"���b�Z�[�W�����[�h */
	string[0] = '\0';
	LoadString(hAppInstance, IDS_JP_TYPE0, string, sizeof(string));

	/* �R���{�b�N�X���N���A */
	for (i=0; i<sizeof(JoyPageComboTable)/sizeof(UINT); i++) {
		hWnd = GetDlgItem(hDlg, JoyPageComboTable[i]);
		ASSERT(hWnd);
		ComboBox_ResetContent(hWnd);
	}

	/* �^�C�v�擾 */
	hWnd = GetDlgItem(hDlg, IDC_JP_TYPEC);
	ASSERT(hWnd);
	j = ComboBox_GetCurSel(hWnd);

	/* �L�[�{�[�h�̏ꍇ */
	if (j == 3) {
		for (i=0; i<sizeof(JoyPageComboTable)/sizeof(UINT); i++) {
			/* �n���h���擾 */
			hWnd = GetDlgItem(hDlg, JoyPageComboTable[i]);
			ASSERT(hWnd);
			/* �L�[�R�[�h�}�� */
			ComboBox_AddString(hWnd, string);
			for (j=0; j<sizeof(KbdPageFM77AV)/sizeof(char*); j+=2) {
				if (KbdPageFM77AV[j]) {
					ComboBox_AddString(hWnd, KbdPageFM77AV[j]);
				}
			}
			/* �L�[�R�[�h�ɃJ�[�\�����킹 */
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

	/* �W���C�X�e�B�b�N�̏ꍇ */
	for (i=0; i<sizeof(JoyPageComboTable)/sizeof(UINT); i++) {
		/* �n���h���擾 */
		hWnd = GetDlgItem(hDlg, JoyPageComboTable[i]);
		ASSERT(hWnd);

		/* ������}�� */
		for (j=0; j<sizeof(JoyPageCodeTable)/sizeof(UINT); j++) {
			string[0] = '\0';
			LoadString(hAppInstance, JoyPageCodeTable[j], string, sizeof(string));
			ComboBox_AddString(hWnd, string);
		}

		/* �J�[�\���ݒ� */
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
 *	�W���C�X�e�B�b�N�y�[�W
 *	�f�[�^�Z�b�g
 */
static void FASTCALL JoyPageSet(HWND hDlg)
{
	HWND hWnd;

	ASSERT(hDlg);
	ASSERT((JoyPageIdx == 0) || (JoyPageIdx == 1));

	/* �A�˃R���{�{�b�N�X */
	hWnd = GetDlgItem(hDlg, IDC_JP_RAPIDAC);
	ASSERT(hWnd);
	ComboBox_SetCurSel(hWnd, propdat.nJoyRapid[JoyPageIdx][0]);
	hWnd = GetDlgItem(hDlg, IDC_JP_RAPIDBC);
	ASSERT(hWnd);
	ComboBox_SetCurSel(hWnd, propdat.nJoyRapid[JoyPageIdx][1]);

	/* �^�C�v�R���{�{�b�N�X */
	hWnd = GetDlgItem(hDlg, IDC_JP_TYPEC);
	ASSERT(hWnd);
	ComboBox_SetCurSel(hWnd, propdat.nJoyType[JoyPageIdx]);

	/* �R�[�h���� */
	JoyPageCombo(hDlg, IDC_JP_TYPEC);
}

/*
 *	�W���C�X�e�B�b�N�y�[�W
 *	�f�[�^�擾
 */
static void FASTCALL JoyPageGet(HWND hDlg)
{
	HWND hWnd;
	int i;
	int j;

	ASSERT(hDlg);
	ASSERT((JoyPageIdx == 0) || (JoyPageIdx == 1));

	/* �A�˃R���{�{�b�N�X */
	hWnd = GetDlgItem(hDlg, IDC_JP_RAPIDAC);
	ASSERT(hWnd);
	propdat.nJoyRapid[JoyPageIdx][0] = ComboBox_GetCurSel(hWnd);
	hWnd = GetDlgItem(hDlg, IDC_JP_RAPIDBC);
	ASSERT(hWnd);
	propdat.nJoyRapid[JoyPageIdx][1] = ComboBox_GetCurSel(hWnd);

	/* �^�C�v�R���{�{�b�N�X */
	hWnd = GetDlgItem(hDlg, IDC_JP_TYPEC);
	ASSERT(hWnd);
	propdat.nJoyType[JoyPageIdx] = ComboBox_GetCurSel(hWnd);

	/* �R�[�h */
	if (propdat.nJoyType[JoyPageIdx] == 3) {
		/* �L�[�{�[�h */
		for (i=0; i<sizeof(JoyPageComboTable)/sizeof(UINT); i++) {
			/* �n���h���擾 */
			hWnd = GetDlgItem(hDlg, JoyPageComboTable[i]);
			ASSERT(hWnd);

			/* �R�[�h�ϊ��A�Z�b�g */
			j = ComboBox_GetCurSel(hWnd);
			if (j != 0) {
				j = KbdPageIndex2FM77AV(j - 1);
			}
			propdat.nJoyCode[JoyPageIdx][i] = j;
		}
		return;
	}

	/* �W���C�X�e�B�b�N */
	for (i=0; i<sizeof(JoyPageComboTable)/sizeof(UINT); i++) {
		/* �n���h���擾 */
		hWnd = GetDlgItem(hDlg, JoyPageComboTable[i]);
		ASSERT(hWnd);

		/* �R�[�h�ϊ��A�Z�b�g */
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
 *	�W���C�X�e�B�b�N�y�[�W
 *	�{�^�������ꂽ
 */
static void FASTCALL JoyPageButton(HWND hDlg, WORD wID)
{
	ASSERT(hDlg);

	switch (wID) {
		/* �|�[�g1 ��I�� */
		case IDC_JP_PORT1:
			JoyPageGet(hDlg);
			JoyPageIdx = 0;
			JoyPageSet(hDlg);
			break;

		/* �|�[�g2 ��I�� */
		case IDC_JP_PORT2:
			JoyPageGet(hDlg);
			JoyPageIdx = 1;
			JoyPageSet(hDlg);
			break;

		/* ����ȊO */
		default:
			ASSERT(FALSE);
			break;
	}
}

/*
 *	�W���C�X�e�B�b�N�y�[�W
 *	�_�C�A���O������
 */
static void FASTCALL JoyPageInit(HWND hDlg)
{
	HWND hWnd[2];
	int i;
	char string[128];

	/* �V�[�g������ */
	ASSERT(hDlg);
	SheetInit(hDlg);

	/* �^�C�v�R���{�{�b�N�X */
	hWnd[0] = GetDlgItem(hDlg, IDC_JP_TYPEC);
	ASSERT(hWnd[0]);
	ComboBox_ResetContent(hWnd[0]);
	for (i=0; i<4; i++) {
		string[0] = '\0';
		LoadString(hAppInstance, IDS_JP_TYPE0 + i, string, sizeof(string));
		ComboBox_AddString(hWnd[0], string);
	}
	ComboBox_SetCurSel(hWnd[0], 0);

	/* �A�˃R���{�{�b�N�X */
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

	/* �|�[�g�I���O���[�v�{�^�� */
	CheckDlgButton(hDlg, IDC_JP_PORT1, BST_CHECKED);
	JoyPageIdx = 0;
	JoyPageSet(hDlg);
}

/*
 *	�W���C�X�e�B�b�N�y�[�W
 *	�K�p
 */
static void FASTCALL JoyPageApply(HWND hDlg)
{
	ASSERT(hDlg);

	/* �f�[�^�擾 */
	JoyPageGet(hDlg);

	/* �X�e�[�g�ύX */
	uPropertyState = 2;
}

/*
 *	�W���C�X�e�B�b�N�y�[�W
 *	�_�C�A���O�v���V�[�W��
 */
static BOOL CALLBACK JoyPageProc(HWND hDlg, UINT msg,
									 WPARAM wParam, LPARAM lParam)
{
	LPNMHDR pnmh;

	switch (msg) {
		/* ������ */
		case WM_INITDIALOG:
			JoyPageInit(hDlg);
			return TRUE;

		/* �R�}���h */
		case WM_COMMAND:
			switch (HIWORD(wParam)) {
				/* �{�^���N���b�N */
				case BN_CLICKED:
					JoyPageButton(hDlg, LOWORD(wParam));
					break;
				/* �R���{�I�� */
				case CBN_SELCHANGE:
					JoyPageCombo(hDlg, LOWORD(wParam));
					break;
			}
			return TRUE;

		/* �ʒm */
		case WM_NOTIFY:
			pnmh = (LPNMHDR)lParam;
			if (pnmh->code == PSN_APPLY) {
				JoyPageApply(hDlg);
				return TRUE;
			}
			break;

		/* �J�[�\���ݒ� */
		case WM_SETCURSOR:
			if (HIWORD(lParam) == WM_MOUSEMOVE) {
				PageHelp(hDlg, IDC_JP_HELP);
			}
			break;
	}

	return FALSE;
}

/*
 *	�W���C�X�e�B�b�N�y�[�W
 *	�쐬
 */
static HPROPSHEETPAGE FASTCALL JoyPageCreate(void)
{
	PROPSHEETPAGE psp;

	/* �\���̂��쐬 */
	memset(&psp, 0, sizeof(PROPSHEETPAGE));
	psp.dwSize = PROPSHEETPAGE_V1_SIZE;
	psp.dwFlags = 0;
	psp.hInstance = hAppInstance;
	psp.u.pszTemplate = MAKEINTRESOURCE(IDD_JOYPAGE);
	psp.pfnDlgProc = JoyPageProc;

	return CreatePropertySheetPage(&psp);
}

/*-[ �X�N���[���y�[�W ]-----------------------------------------------------*/

/*
 *	�X�N���[���y�[�W
 *	�_�C�A���O������
 */
static void FASTCALL ScrPageInit(HWND hDlg)
{
	/* �V�[�g������ */
	ASSERT(hDlg);
	SheetInit(hDlg);

	/* �S��ʗD�惂�[�h */
	if (propdat.bDD480Line) {
		CheckDlgButton(hDlg, IDC_SCP_480, BST_CHECKED);
	}
	else {
		CheckDlgButton(hDlg, IDC_SCP_400, BST_CHECKED);
	}

	/* �t���X�L����(24k) */
	if (propdat.bFullScan) {
		CheckDlgButton(hDlg, IDC_SCP_24K, BST_CHECKED);
	}
	else {
		CheckDlgButton(hDlg, IDC_SCP_24K, BST_UNCHECKED);
	}

	/* �㉺�X�e�[�^�X */
	if (propdat.bDD480Status) {
		CheckDlgButton(hDlg, IDC_SCP_CAPTIONB, BST_CHECKED);
	}
	else {
		CheckDlgButton(hDlg, IDC_SCP_CAPTIONB, BST_UNCHECKED);
	}
}

/*
 *	�X�N���[���y�[�W
 *	�K�p
 */
static void FASTCALL ScrPageApply(HWND hDlg)
{
	ASSERT(hDlg);

	/* �X�e�[�g�ύX */
	uPropertyState = 2;

	/* �S��ʗD�惂�[�h */
	if (IsDlgButtonChecked(hDlg, IDC_SCP_480) == BST_CHECKED) {
		propdat.bDD480Line = TRUE;
	}
	else {
		propdat.bDD480Line = FALSE;
	}

	/* �t���X�L����(24k) */
	if (IsDlgButtonChecked(hDlg, IDC_SCP_24K) == BST_CHECKED) {
		propdat.bFullScan = TRUE;
	}
	else {
		propdat.bFullScan = FALSE;
	}

	/* �㉺�X�e�[�^�X */
	if (IsDlgButtonChecked(hDlg, IDC_SCP_CAPTIONB) == BST_CHECKED) {
		propdat.bDD480Status = TRUE;
	}
	else {
		propdat.bDD480Status = FALSE;
	}
}

/*
 *	�X�N���[���y�[�W
 *	�_�C�A���O�v���V�[�W��
 */
static BOOL CALLBACK ScrPageProc(HWND hDlg, UINT msg,
									 WPARAM wParam, LPARAM lParam)
{
	LPNMHDR pnmh;

	switch (msg) {
		/* ������ */
		case WM_INITDIALOG:
			ScrPageInit(hDlg);
			return TRUE;

		/* �ʒm */
		case WM_NOTIFY:
			pnmh = (LPNMHDR)lParam;
			if (pnmh->code == PSN_APPLY) {
				ScrPageApply(hDlg);
				return TRUE;
			}
			break;

		/* �J�[�\���ݒ� */
		case WM_SETCURSOR:
			if (HIWORD(lParam) == WM_MOUSEMOVE) {
				PageHelp(hDlg, IDC_SCP_HELP);
			}
			break;
	}

	return FALSE;
}

/*
 *	�X�N���[���y�[�W
 *	�쐬
 */
static HPROPSHEETPAGE FASTCALL ScrPageCreate(void)
{
	PROPSHEETPAGE psp;

	/* �\���̂��쐬 */
	memset(&psp, 0, sizeof(PROPSHEETPAGE));
	psp.dwSize = PROPSHEETPAGE_V1_SIZE;
	psp.dwFlags = 0;
	psp.hInstance = hAppInstance;
	psp.u.pszTemplate = MAKEINTRESOURCE(IDD_SCRPAGE);
	psp.pfnDlgProc = ScrPageProc;

	return CreatePropertySheetPage(&psp);
}

/*-[ �I�v�V�����y�[�W ]-----------------------------------------------------*/

/*
 *	�I�v�V�����y�[�W
 *	�_�C�A���O������
 */
static void FASTCALL OptPageInit(HWND hDlg)
{
	/* �V�[�g������ */
	ASSERT(hDlg);
	SheetInit(hDlg);

	/* WHG */
	if (propdat.bWHGEnable) {
		CheckDlgButton(hDlg, IDC_OP_WHGB, BST_CHECKED);
	}
	else {
		CheckDlgButton(hDlg, IDC_OP_WHGB, BST_UNCHECKED);
	}

	/* �r�f�I�f�B�W�^�C�Y */
	if (propdat.bDigitizeEnable) {
		CheckDlgButton(hDlg, IDC_OP_DIGITIZEB, BST_CHECKED);
	}
	else {
		CheckDlgButton(hDlg, IDC_OP_DIGITIZEB, BST_UNCHECKED);
	}
}

/*
 *	�I�v�V�����y�[�W
 *	�K�p
 */
static void FASTCALL OptPageApply(HWND hDlg)
{
	ASSERT(hDlg);

	/* �X�e�[�g�ύX */
	uPropertyState = 2;

	/* WHG */
	if (IsDlgButtonChecked(hDlg, IDC_OP_WHGB) == BST_CHECKED) {
		propdat.bWHGEnable = TRUE;
	}
	else {
		propdat.bWHGEnable = FALSE;
	}

	/* �r�f�I�f�B�W�^�C�Y */
	if (IsDlgButtonChecked(hDlg, IDC_OP_DIGITIZEB) == BST_CHECKED) {
		propdat.bDigitizeEnable = TRUE;
	}
	else {
		propdat.bDigitizeEnable = FALSE;
	}
}

/*
 *	�I�v�V�����y�[�W
 *	�_�C�A���O�v���V�[�W��
 */
static BOOL CALLBACK OptPageProc(HWND hDlg, UINT msg,
									 WPARAM wParam, LPARAM lParam)
{
	LPNMHDR pnmh;

	switch (msg) {
		/* ������ */
		case WM_INITDIALOG:
			OptPageInit(hDlg);
			return TRUE;

		/* �ʒm */
		case WM_NOTIFY:
			pnmh = (LPNMHDR)lParam;
			if (pnmh->code == PSN_APPLY) {
				OptPageApply(hDlg);
				return TRUE;
			}
			break;

		/* �J�[�\���ݒ� */
		case WM_SETCURSOR:
			if (HIWORD(lParam) == WM_MOUSEMOVE) {
				PageHelp(hDlg, IDC_OP_HELP);
			}
			break;
	}

	return FALSE;
}

/*
 *	�I�v�V�����y�[�W
 *	�쐬
 */
static HPROPSHEETPAGE FASTCALL OptPageCreate(void)
{
	PROPSHEETPAGE psp;

	/* �\���̂��쐬 */
	memset(&psp, 0, sizeof(PROPSHEETPAGE));
	psp.dwSize = PROPSHEETPAGE_V1_SIZE;
	psp.dwFlags = 0;
	psp.hInstance = hAppInstance;
	psp.u.pszTemplate = MAKEINTRESOURCE(IDD_OPTPAGE);
	psp.pfnDlgProc = OptPageProc;

	return CreatePropertySheetPage(&psp);
}

/*-[ �v���p�e�B�V�[�g ]-----------------------------------------------------*/

/*
 *	�v���p�e�B�V�[�g
 *	������
 */
static void FASTCALL SheetInit(HWND hDlg)
{
	RECT drect;
	RECT prect;
	LONG lStyleEx;

	/* �������t���O���`�F�b�N�A�V�[�g�������ς݂ɐݒ� */
	if (uPropertyState > 0) {
		return;
	}
	uPropertyState = 1;

	/* �v���p�e�B�V�[�g���A���C���E�C���h�E�̒����Ɋ񂹂� */
	GetWindowRect(hMainWnd, &prect);
	GetWindowRect(GetParent(hDlg), &drect);
	drect.right -= drect.left;
	drect.bottom -= drect.top;
	drect.left = (prect.right - prect.left) / 2 + prect.left;
	drect.left -= (drect.right / 2);
	drect.top = (prect.bottom - prect.top) / 2 + prect.top;
	drect.top -= (drect.bottom / 2);
	MoveWindow(GetParent(hDlg), drect.left, drect.top, drect.right, drect.bottom, FALSE);

	/* �v���p�e�B�V�[�g����w���v�{�^�������� */
	lStyleEx = GetWindowLong(GetParent(hDlg), GWL_EXSTYLE);
	lStyleEx &= ~WS_EX_CONTEXTHELP;
	SetWindowLong(GetParent(hDlg), GWL_EXSTYLE, lStyleEx);
}

/*
 *	�ݒ�(C)
 */
void FASTCALL OnConfig(HWND hWnd)
{
	PROPSHEETHEADER pshead;
	HPROPSHEETPAGE hpspage[6];
	int i;

	ASSERT(hWnd);

	/* �f�[�^�]�� */
	propdat = configdat;

	/* �v���p�e�B�y�[�W�쐬 */
	hpspage[0] = GeneralPageCreate();
	hpspage[1] = SoundPageCreate();
	hpspage[2] = KbdPageCreate();
	hpspage[3] = JoyPageCreate();
	hpspage[4] = ScrPageCreate();
	hpspage[5] = OptPageCreate();

	/* �v���p�e�B�y�[�W�`�F�b�N */
	for (i=0; i<sizeof(hpspage)/sizeof(HPROPSHEETPAGE); i++) {
		if (!hpspage[i]) {
			return;
		}
	}

	/* �v���p�e�B�V�[�g�쐬 */
	memset(&pshead, 0, sizeof(pshead));
	pshead.dwSize = PROPSHEETHEADER_V1_SIZE;
	pshead.dwFlags = PSH_NOAPPLYNOW;
	pshead.hwndParent = hWnd;
	pshead.hInstance = hAppInstance;
	pshead.pszCaption = MAKEINTRESOURCE(IDS_CONFIGCAPTION);
	pshead.nPages = sizeof(hpspage) / sizeof(HPROPSHEETPAGE);
	pshead.u2.nStartPage = 0;
	pshead.u3.phpage = hpspage;

	/* �v���p�e�B�V�[�g���s */
	uPropertyState = 0;
	uPropertyHelp = 0;
	PropertySheet(&pshead);

	/* ���ʂ�ok�ȊO�Ȃ�I�� */
	if (uPropertyState != 2) {
		return;
	}

	/* ok�Ȃ̂ŁA�f�[�^�]�� */
	configdat = propdat;

	/* �K�p */
	LockVM();
	ApplyCfg();
	UnlockVM();
}

#endif	/* _WIN32 */
