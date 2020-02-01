/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API �R���g���[���o�[ ]
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
 *	�O���[�o�� ���[�N
 */
HWND hStatusBar;						/* �X�e�[�^�X�o�[ */

/*
 *	�X�^�e�B�b�N ���[�N
 */
static char szIdleMessage[128];			/* �A�C�h�����b�Z�[�W */
static char szRunMessage[128];			/* RUN���b�Z�[�W */
static char szStopMessage[128];			/* STOP���b�Z�[�W */
static char szCaption[128];				/* �L���v�V���� */
static int nCAP;						/* CAP�L�[ */
static int nKANA;						/* ���ȃL�[ */
static int nINS;						/* INS�L�[ */
static int nDrive[2];					/* �t���b�s�[�h���C�u */
static char szDrive[2][16 + 1];			/* �t���b�s�[�h���C�u */
static int nTape;						/* �e�[�v */

/*-[ �X�e�[�^�X�o�[ ]-------------------------------------------------------*/

/*
 *	�A�N�Z�X�}�N��
 */
#define Status_SetParts(hwnd, nParts, aWidths) \
	SendMessage((hwnd), SB_SETPARTS, (WPARAM) nParts, (LPARAM) (LPINT) aWidths)

#define Status_SetText(hwnd, iPart, uType, szText) \
	SendMessage((hwnd), SB_SETTEXT, (WPARAM) (iPart | uType), (LPARAM) (LPSTR) szText)

/*
 *	�y�C����`
 */
#define PANE_DEFAULT	0
#define PANE_DRIVE1		1
#define PANE_DRIVE0		2
#define PANE_TAPE		3
#define PANE_CAP		4
#define PANE_KANA		5
#define PANE_INS		6

/*
 *	�X�e�[�^�X�o�[�쐬
 */
HWND FASTCALL CreateStatus(HWND hParent)
{
	HWND hWnd;

	ASSERT(hParent);

	/* ���b�Z�[�W�����[�h */
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

	/* �X�e�[�^�X�o�[���쐬 */
	hWnd = CreateStatusWindow(WS_CHILD | WS_VISIBLE | CCS_BOTTOM,
								szIdleMessage, hParent, ID_STATUS_BAR);

	return hWnd;
}

/*
 *	�L���v�V�����`��
 */
static void FASTCALL DrawMainCaption(void)
{
	char string[256];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];

	/* ����󋵂ɉ����āA�R�s�[ */
	if (run_flag) {
		strcpy(string, szRunMessage);
	}
	else {
		strcpy(string, szStopMessage);
	}
	strcat(string, " ");

	/* �t���b�s�[�f�B�X�N�h���C�u 0 */
	if (fdc_ready[0] != FDC_TYPE_NOTREADY) {
		strcat(string, "- ");

		/* �t�@�C���l�[���{�g���q�̂ݎ��o�� */
		_splitpath(fdc_fname[0], drive, dir, fname, ext);
		strcat(string, fname);
		strcat(string, ext);
		strcat(string, " ");
	}

	/* �t���b�s�[�f�B�X�N�h���C�u 1 */
	if (fdc_ready[1] != FDC_TYPE_NOTREADY) {
		if ((strcmp(fdc_fname[0], fdc_fname[1]) != 0) ||
			 (fdc_ready[0] == FDC_TYPE_NOTREADY)) {
			strcat(string, "(");

			/* �t�@�C���l�[���{�g���q�̂ݎ��o�� */
			_splitpath(fdc_fname[1], drive, dir, fname, ext);
			strcat(string, fname);
			strcat(string, ext);
			strcat(string, ") ");
		}
	}

	/* �e�[�v */
	if (tape_fileh != -1) {
		strcat(string, "- ");

		/* �t�@�C���l�[���{�g���q�̂ݎ��o�� */
		_splitpath(tape_fname, drive, dir, fname, ext);
		strcat(string, fname);
		strcat(string, ext);
		strcat(string, " ");
	}

	/* ��r�`�� */
	string[127] = '\0';
	if (memcmp(szCaption, string, strlen(string) + 1) != 0) {
		strcpy(szCaption, string);
		SetWindowText(hMainWnd, szCaption);
	}
}

/*
 *	CAP�L�[�`��
 */
static void FASTCALL DrawCAP(void)
{
	int num;

	/* �ԍ����� */
	if (caps_flag) {
		num = 1;
	}
	else {
		num = 0;
	}

	/* �����Ȃ牽�����Ȃ� */
	if (nCAP == num) {
		return;
	}

	/* �`��A���[�N�X�V */
	nCAP = num;
	Status_SetText(hStatusBar, PANE_CAP, SBT_OWNERDRAW, PANE_CAP);
}

/*
 *	���ȃL�[�`��
 */
static void FASTCALL DrawKANA(void)
{
	int num;

	/* �ԍ����� */
	if (kana_flag) {
		num = 1;
	}
	else {
		num = 0;
	}

	/* �����Ȃ牽�����Ȃ� */
	if (nKANA == num) {
		return;
	}

	/* �`��A���[�N�X�V */
	nKANA = num;
	Status_SetText(hStatusBar, PANE_KANA, SBT_OWNERDRAW, PANE_KANA);
}

/*
 *	INS�L�[�`��
 */
static void FASTCALL DrawINS(void)
{
	int num;

	/* �ԍ����� */
	if (ins_flag) {
		num = 1;
	}
	else {
		num = 0;
	}

	/* �����Ȃ牽�����Ȃ� */
	if (nINS == num) {
		return;
	}

	/* �`��A���[�N�X�V */
	nINS = num;
	Status_SetText(hStatusBar, PANE_INS, SBT_OWNERDRAW, PANE_INS);
}

/*
 *	�h���C�u�`��
 */
static void FASTCALL DrawDrive(int drive, UINT nID)
{
	int num;
	char *name;

	ASSERT((drive >= 0) && (drive <= 1));

	/* �ԍ��Z�b�g */
	if (fdc_ready[drive] == FDC_TYPE_NOTREADY) {
		num = 255;
	}
	else {
		num = fdc_access[drive];
		if (num == FDC_ACCESS_SEEK) {
			num = FDC_ACCESS_READY;
		}
	}

	/* ���O�擾 */
	name = "";
	if (fdc_ready[drive] == FDC_TYPE_D77) {
		name = fdc_name[drive][ fdc_media[drive] ];
	}
	if (fdc_ready[drive] == FDC_TYPE_2D) {
		name = "2D DISK";
	}

	/* �ԍ���r */
	if (nDrive[drive] == num) {
		if (fdc_ready[drive] == FDC_TYPE_NOTREADY) {
			return;
		}
		if (strcmp(szDrive[drive], name) == 0) {
			return;
		}
	}

	/* �`�� */
	nDrive[drive] = num;
	strcpy(szDrive[drive], name);
	Status_SetText(hStatusBar, nID, SBT_OWNERDRAW, nID);
}

/*
 *	�e�[�v�`��
 */
static void FASTCALL DrawTape(void)
{
	int num;

	/* �i���o�[�v�Z */
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

	/* �ԍ���r */
	if (nTape == num) {
		return;
	}

	/* �`�� */
	nTape = num;
	Status_SetText(hStatusBar, PANE_TAPE, SBT_OWNERDRAW, PANE_TAPE);
}

/*
 *	�`��
 */
void FASTCALL DrawStatus(void)
{
	/* �E�C���h�E�`�F�b�N */
	if (!hMainWnd) {
		return;
	}

	DrawMainCaption();

	/* �S��ʁA�X�e�[�^�X�o�[�`�F�b�N */
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
 *	�ĕ`��
 */
void FASTCALL PaintStatus(void)
{
	/* �L�����[�N�����ׂăN���A���� */
	szCaption[0] = '\0';
	nCAP = -1;
	nKANA = -1;
	nINS = -1;
	nDrive[0] = -1;
	nDrive[1] = -1;
	szDrive[0][0] = '\0';
	szDrive[1][0] = '\0';
	nTape = -1;

	/* �`�� */
	DrawStatus();
}

/*
 *	�I�[�i�[�h���[
 */
void FASTCALL OwnerDrawStatus(DRAWITEMSTRUCT *pDI)
{
	HBRUSH hBrush;
	COLORREF fColor;
	COLORREF bColor;
	char string[128];
	int i;

	ASSERT(pDI);

	/* ������A�F������ */
	switch (pDI->itemData) {
		/* �t���b�s�[�f�B�X�N */
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

		/* �e�[�v */
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

		/* ���� */
		case PANE_KANA:
			strcpy(string, "����");
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

		/* ����ȊO */
		default:
			ASSERT(FALSE);
			return;
	}

	/* �u���V�œh�� */
	hBrush = CreateSolidBrush(bColor);
	if (hBrush) {
		FillRect(pDI->hDC, &(pDI->rcItem), hBrush);
		DeleteObject(hBrush);
	}

	/* �e�L�X�g��`�� */
	SetTextColor(pDI->hDC, fColor);
	SetBkColor(pDI->hDC, bColor);
	DrawText(pDI->hDC, string, strlen(string), &(pDI->rcItem),
				DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}

/*
 *	�T�C�Y�ύX
 */
void FASTCALL SizeStatus(LONG cx)
{
	HDC hDC;
	TEXTMETRIC tm;
	LONG cw;
	UINT uPane[7];

	ASSERT(cx > 0);
	ASSERT(hStatusBar);

	/* �e�L�X�g���g���b�N���擾 */
	hDC = GetDC(hStatusBar);
	SelectObject(hDC, GetStockObject(SYSTEM_FONT));
	GetTextMetrics(hDC, &tm);
	ReleaseDC(hStatusBar, hDC);
	cw = tm.tmAveCharWidth;

	/* �y�C���T�C�Y������(�y�C���E�[�̈ʒu��ݒ�) */
	uPane[PANE_INS] = cx;
	uPane[PANE_KANA] = uPane[PANE_INS] - cw * 4;
	uPane[PANE_CAP] = uPane[PANE_KANA] - cw * 4;
	uPane[PANE_TAPE] = uPane[PANE_CAP] - cw * 4;
	uPane[PANE_DRIVE0] = uPane[PANE_TAPE] - cw * 5;
	uPane[PANE_DRIVE1] = uPane[PANE_DRIVE0] - cw * 16;
	uPane[PANE_DEFAULT] = uPane[PANE_DRIVE1] - cw * 16;

	/* �y�C���T�C�Y�ݒ� */
	Status_SetParts(hStatusBar, sizeof(uPane)/sizeof(UINT), uPane);

	/* �ĕ`�� */
	PaintStatus();
}

/*
 *	���j���[�Z���N�g
 */
void FASTCALL OnMenuSelect(WPARAM wParam)
{
	char buffer[128];
	UINT uID;

	ASSERT(hStatusBar);

	/* �X�e�[�^�X�o�[���\������Ă��Ȃ���΁A�������Ȃ� */
	if (!IsWindowVisible(hStatusBar)) {
		return;
	}

	/* ����ID�̕����񃊃\�[�X���[�h�����݂� */
	uID = (UINT)LOWORD(wParam);
	if (LoadString(hAppInstance, uID, buffer, sizeof(buffer)) == 0) {
		buffer[0] = '\0';
	}

	/* �Z�b�g */
	Status_SetText(hStatusBar, 0, 0, buffer);
}

/*
 *	���j���[�I��
 */
void FASTCALL OnExitMenuLoop(void)
{
	ASSERT(hStatusBar);

	/* �X�e�[�^�X�o�[���\������Ă��Ȃ���΁A�������Ȃ� */
	if (!IsWindowVisible(hStatusBar)) {
		return;
	}

	/* �A�C�h�����b�Z�[�W��\�� */
	Status_SetText(hStatusBar, 0, 0, szIdleMessage);
}

#endif	/* _WIN32 */

