/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API �T�u�E�B���h�E�Q ]
 */

#ifdef _WIN32

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdlib.h>
#include "xm7.h"
#include "fdc.h"
#include "opn.h"
#include "whg.h"
#include "subctrl.h"
#include "w32.h"
#include "w32_res.h"
#include "w32_snd.h"
#include "w32_sub.h"

/*
 *	�X�^�e�B�b�N ���[�N
 */
static BYTE *pFDC;						/* FDC Draw�o�b�t�@ */
static BYTE *pOPNReg;					/* OPN���W�X�^ Draw�o�b�t�@ */
static BYTE *pOPNDisp;					/* OPN�f�B�X�v���C �r�b�g�}�b�v�o�b�t�@ */
static HBITMAP hOPNDisp;				/* OPN�f�B�X�v���C �r�b�g�}�b�v�n���h�� */
static RECT rOPNDisp;					/* OPN�f�B�X�v���C ��` */
static int knOPNDisp[12];				/* OPN�f�B�X�v���C ���Ճ��[�N */
static int ktOPNDisp[12];				/* OPN�f�B�X�v���C ���Ճ��[�N */
static BYTE cnOPNDisp[12][49 * 2];		/* OPN�f�B�X�v���C �������[�N */
static BYTE ctOPNDisp[12][49 * 2];		/* OPN�f�B�X�v���C �������[�N */
static int lnOPNDisp[12];				/* OPN�f�B�X�v���C ���x�����[�N */
static int ltOPNDisp[12];				/* OPN�f�B�X�v���C ���x�����[�N */
static BYTE *pSubCtrl;					/* �T�u�R���g���[�� Draw�o�b�t�@ */

/*
 *	�p���b�g�e�[�u��
 */
static const RGBQUAD rgbOPNDisp[] = {
	/*  B     G     R   Reserve */
	{ 0x00, 0x00, 0x00, 0x00 },
	{ 0xff, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0xff, 0x00 },
	{ 0xff, 0x00, 0xff, 0x00 },
	{ 0x00, 0xff, 0x00, 0x00 },
	{ 0xff, 0xff, 0x00, 0x00 },
	{ 0x00, 0xff, 0xff, 0x00 },
	{ 0xff, 0xff, 0xff, 0x00 },

	{ 0x3f, 0x3f, 0x3f, 0x00 },	/* �ÊD */
	{ 0xff, 0xbf, 0x00, 0x00 },	/* ���F */
	{ 0x00, 0xdf, 0xff, 0x00 },	/* ���F */
	{ 0x00, 0xaf, 0x7f, 0x00 },	/* ���� */
	{ 0xbf, 0x9f, 0xff, 0x00 },	/* �Î� */
	{ 0xbf, 0xbf, 0x00, 0x00 },
	{ 0x00, 0xbf, 0xbf, 0x00 },
	{ 0xcf, 0xcf, 0xcf, 0x00 },	/* ���D */
};

/*-[ �T�uCPU�R���g���[���E�C���h�E ]-----------------------------------------*/

/*
 *	�T�uCPU�R���g���[���E�C���h�E
 *	�Z�b�g�A�b�v(���LRAM)
 */
static void FASTCALL SetupSubCtrlShared(BYTE *p, int cx, int y)
{
	char string[128];
	char temp[4];
	int i, j;

	ASSERT(p);
	ASSERT(cx > 0);

	/* �^�C�g�� */
	strcpy(string, "Shared RAM:");
	memcpy(&p[cx * y], string, strlen(string));
	y++;

	/* ���[�v */
	for (i=0; i<8; i++) {
		/* ������쐬 */
		sprintf(string, "+%02X:", i * 16);
		for (j=0; j<16; j++) {
			sprintf(temp, " %02X", shared_ram[i * 16 + j]);
			strcat(string, temp);
		}

		/* �Z�b�g */
		memcpy(&p[cx * y], string, strlen(string));
		y++;
	}
}

/*
 *	�T�uCPU�R���g���[���E�C���h�E
 *	�Z�b�g�A�b�v(�t���O)
 */
static void FASTCALL SetupSubCtrlFlag(BYTE *p, int cx, int y,
									char *title, BOOL flag)
{
	char string[64];

	ASSERT(p);
	ASSERT(cx > 0);

	/* ������ */
	memset(string, 0x20, sizeof(string));

	/* �^�C�g���Z�b�g */
	memcpy(string, title, strlen(title));

	/* On�܂���Off */
	if (flag) {
		string[19] = 'O';
		string[20] = 'n';
	}
	else {
		string[18] = 'O';
		string[19] = 'f';
		string[20] = 'f';
	}
	string[21] = '\0';

	/* �Z�b�g */
	memcpy(&p[cx * y], string, strlen(string));
}

/*
 *	�T�uCPU�R���g���[���E�C���h�E
 *	�Z�b�g�A�b�v
 */
static void FASTCALL SetupSubCtrl(BYTE *p, int x, int y)
{
	char string[128];

	ASSERT(p);
	ASSERT(x > 0);
	ASSERT(y > 0);

	/* ��U�X�y�[�X�Ŗ��߂� */
	memset(p, 0x20, x * y);

	/* FM-7�݊��t���O */
	SetupSubCtrlFlag(p, x, 0, "Sub Halt", subhalt_flag);
	SetupSubCtrlFlag(p, x, 1, "Busy Flag", subhalt_flag);
	SetupSubCtrlFlag(p, x, 2, "Cancel IRQ", subcancel_flag);
	SetupSubCtrlFlag(p, x, 3, "Attention FIRQ", subattn_flag);

	/* �T�u���j�^ROM */
	strcpy(string, "Sub ROM");
	memcpy(&p[x * 0 + 30], string, strlen(string));
	switch (subrom_bank) {
		case 0:
			strcpy(string, "Type-C");
			break;
		case 1:
			strcpy(string, "Type-A");
			break;
		case 2:
			strcpy(string, "Type-B");
			break;
		default:
			ASSERT(FALSE);
			break;
	}
	memcpy(&p[x * 0 + 46], string, strlen(string));

	/* CG�o���N */
	strcpy(string, "CG ROM");
	memcpy(&p[x * 1 + 30], string, strlen(string));
	sprintf(string, "Bank %1d", cgrom_bank);
	memcpy(&p[x * 1 + 46], string, strlen(string));

	/* �T�u���Z�b�g */
	strcpy(string, "Sub Reset");
	memcpy(&p[x * 2 + 30], string, strlen(string));
	if (subreset_flag) {
		strcpy(string, "Software");
	}
	else {
		strcpy(string, "Hardware");
	}
	memcpy(&p[x * 2 + 44], string, strlen(string));

	/* 320���[�h */
	strcpy(string, "Display Mode");
	memcpy(&p[x * 3 + 30], string, strlen(string));
	if (mode320) {
		strcpy(string, "320x200");
	}
	else {
		strcpy(string, "640x200");
	}
	memcpy(&p[x * 3 + 45], string, strlen(string));

	/* ���LRAM */
	SetupSubCtrlShared(p, x, 5);
}

/*
 *	�T�uCPU�R���g���[���E�C���h�E
 *	�`��
 */
static void FASTCALL DrawSubCtrl(HWND hWnd, HDC hDC)
{
	RECT rect;
	BYTE *p, *q;
	int x, y;
	int xx, yy;
	HFONT hFont;
	HFONT hBackup;

	ASSERT(hWnd);
	ASSERT(hDC);

	/* �E�C���h�E�W�I���g���𓾂� */
	GetClientRect(hWnd, &rect);
	x = rect.right / lCharWidth;
	y = rect.bottom / lCharHeight;
	if ((x == 0) || (y == 0)) {
		return;
	}

	/* �Z�b�g�A�b�v */
	p = pSubCtrl;
	if (!p) {
		return;
	}
	SetupSubCtrl(p, x, y);

	/* �t�H���g�Z���N�g */
	hFont = CreateTextFont();
	hBackup = SelectObject(hDC, hFont);

	/* ��r�`�� */
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

	/* �I�� */
	SelectObject(hDC, hBackup);
	DeleteObject(hFont);
}

/*
 *	�T�uCPU�R���g���[���E�C���h�E
 *	���t���b�V��
 */
void FASTCALL RefreshSubCtrl(void)
{
	HWND hWnd;
	HDC hDC;

	/* ��ɌĂ΂��̂ŁA���݃`�F�b�N���邱�� */
	if (hSubWnd[SWND_SUBCTRL] == NULL) {
		return;
	}

	/* �`�� */
	hWnd = hSubWnd[SWND_SUBCTRL];
	hDC = GetDC(hWnd);
	DrawSubCtrl(hWnd, hDC);
	ReleaseDC(hWnd, hDC);
}

/*
 *	�T�uCPU�R���g���[���E�C���h�E
 *	�ĕ`��
 */
static void FASTCALL PaintSubCtrl(HWND hWnd)
{
	HDC hDC;
	PAINTSTRUCT ps;
	RECT rect;
	BYTE *p;
	int x, y;

	ASSERT(hWnd);

	/* �|�C���^��ݒ� */
	p = pSubCtrl;

	/* �E�C���h�E�W�I���g���𓾂� */
	GetClientRect(hWnd, &rect);
	x = rect.right / lCharWidth;
	y = rect.bottom / lCharHeight;

	/* �㔼�G���A��FF�Ŗ��߂� */
	if ((x > 0) && (y > 0)) {
		memset(&p[x * y], 0xff, x * y);
	}

	/* �`�� */
	hDC = BeginPaint(hWnd, &ps);
	ASSERT(hDC);
	DrawSubCtrl(hWnd, hDC);
	EndPaint(hWnd, &ps);
}

/*
 *	�T�uCPU�R���g���[���E�C���h�E
 *	�E�C���h�E�v���V�[�W��
 */
static LRESULT CALLBACK SubCtrlProc(HWND hWnd, UINT message,
								 WPARAM wParam, LPARAM lParam)
{
	int i;

	/* ���b�Z�[�W�� */
	switch (message) {
		/* �E�C���h�E�ĕ`�� */
		case WM_PAINT:
			/* ���b�N���K�v */
			LockVM();
			PaintSubCtrl(hWnd);
			UnlockVM();
			return 0;

		/* �E�C���h�E�폜 */
		case WM_DESTROY:
			/* ���C���E�C���h�E�֎����ʒm */
			for (i=0; i<SWND_MAXNUM; i++) {
				if (hSubWnd[i] == hWnd) {
					hSubWnd[i] = NULL;
					/* Draw�o�b�t�@�폜 */
					free(pSubCtrl);
					pSubCtrl = NULL;
				}
			}
			break;
	}

	/* �f�t�H���g �E�C���h�E�v���V�[�W�� */
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/*
 *	�T�uCPU�R���g���[���E�C���h�E
 *	�E�C���h�E�쐬
 */
HWND FASTCALL CreateSubCtrl(HWND hParent, int index)
{
	WNDCLASSEX wcex;
	char szClassName[] = "XM7_SubCtrl";
	char szWndName[128];
	RECT rect;
	RECT crect, wrect;
	HWND hWnd;

	ASSERT(hParent);

	/* �E�C���h�E��`���v�Z */
	rect.left = lCharWidth * index;
	rect.top = lCharHeight * index;
	if (rect.top >= 380) {
		rect.top -= 380;
	}
	rect.right = lCharWidth * 52;
	rect.bottom = lCharHeight * 14;

	/* �E�C���h�E�^�C�g��������A�o�b�t�@�m�� */
	LoadString(hAppInstance, IDS_SWND_SUBCTRL,
				szWndName, sizeof(szWndName));
	pSubCtrl = malloc(2 * (rect.right / lCharWidth) *
								(rect.bottom / lCharHeight));

	/* �E�C���h�E�N���X�̓o�^ */
	memset(&wcex, 0, sizeof(wcex));
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_VREDRAW | CS_HREDRAW;
	wcex.lpfnWndProc = SubCtrlProc;
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

	/* �E�C���h�E�쐬 */
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

	/* �L���Ȃ�A�T�C�Y�␳���Ď�O�ɒu�� */
	if (hWnd) {
		GetWindowRect(hWnd, &wrect);
		GetClientRect(hWnd, &crect);
		wrect.right += (rect.right - crect.right);
		wrect.bottom += (rect.bottom - crect.bottom);
		SetWindowPos(hWnd, HWND_TOP, wrect.left, wrect.top,
			wrect.right - wrect.left, wrect.bottom - wrect.top, SWP_NOMOVE);
	}

	/* ���ʂ������A�� */
	return hWnd;
}

/*-[ OPN�f�B�X�v���C�E�C���h�E ]---------------------------------------------*/

/*
 *	OPN�f�B�X�v���C�E�C���h�E
 *	�h�b�g�`��
 */
static void FASTCALL PSetOPNDisp(int x, int y, int color)
{
	int w;
	BYTE *p;
	BYTE dat;

	ASSERT((x >= 0) && (x < rOPNDisp.right));
	ASSERT((y >= 0) && (y < rOPNDisp.bottom));
	ASSERT((color >= 0) && (color <= 15));

	/* rOPNDisp���A�r�b�g�}�b�v�̉��T�C�Y���v�Z(4�o�C�g�A���C�����g) */
	w = (((rOPNDisp.right / 2) + 3) >> 2) << 2;

	/* �A�h���X�A�f�[�^�擾 */
	if (!pOPNDisp) {
		return;
	}
	p = &pOPNDisp[w * y + (x >> 1)];
	dat = *p;

	/* �Q�ɕ����� */
	if (x & 1) {
		dat &= 0xf0;
		dat |= (BYTE)color;
	}
	else {
		dat &= 0x0f;
		dat |= (BYTE)(color << 4);
	}

	/* �������� */
	*p = dat;
}

/*
 *	OPN�f�B�X�v���C�E�C���h�E
 *	�{�b�N�X�t�B���`��
 */
static void FASTCALL BfOPNDisp(int x, int y, int cx, int cy, int color)
{
	int i;
	int j;

	ASSERT((color >= 0) && (color < 16));

	for (i=0; i<cy; i++) {
		for (j=0; j<cx; j++) {
			PSetOPNDisp(x + j, y, color);
		}
		y++;
	}
}

/*
 *	OPN�f�B�X�v���C�E�C���h�E
 *	�L�����N�^�`��
 */
static void FASTCALL ChrOPNDisp(char c, int x, int y, int color)
{
	int i;
	int j;
	BYTE *p;
	BYTE dat;

	ASSERT((color >= 0) && (color < 16));

	/* �T�uROM(C)�̃t�H���g�A�h���X�𓾂� */
	p = &subrom_c[c * 8];

	/* y���[�v */
	for (i=0; i<8; i++) {
		/* �f�[�^�擾 */
		dat = *p;
		p++;

		/* x���[�v */
		for (j=0; j<8; j++) {
			if (dat & 0x80) {
				PSetOPNDisp(x, y, color);
			}
			else {
				PSetOPNDisp(x, y, 0);
			}
			dat <<= 1;
			x++;
		}

 		/* ����y�� */
		x -= 8;
		y++;
	}
}

/*
 *	OPN�f�B�X�v���C�E�C���h�E
 *	���Օ`��
 *
 *	��ʃj�u��	�I�N�^�[�u 0�`7
 *	���ʃj�u��	C, C#, D, D#, E, F, F#, G, G#, A, A#, B
 *	x, y�̓g���b�N��̊�_�Bcolor=-1�̓f�t�H���g�J���[
 */
static void FASTCALL KbdOPNDisp(int code, int x, int y, int color)
{
	int i;
	int j;

	/* x���W�B��=6dot���A��=5dot���B���Ɣ��̌��Ԃ�1dot�}�� */
	static const int x_table[] = {
		0, 0+5, 7, 7+5, 14, 21, 21+5, 28, 28+5, 35, 35+5, 42
	};
	/* 0:�E���� 1:���E���� 2:������ 3:���� */
	static const int type_table[] = {
		0, 3, 1 ,3, 2, 0, 3, 1, 3, 1, 3, 2
	};

	ASSERT(code <= 0x7f);
	ASSERT((code & 0x0f) <= 0x0b);

	/* x�ʒu���� */
	x = (code >> 4) * 49;
	code &= 0x0f;
	x += x_table[code];

	/* �F�ݒ� */
	if (color < 0) {
		if (type_table[code] < 3) {
			color = 15;
		}
		else {
			color = 8;
		}
	}

	/* �^�C�v�� */
	switch (type_table[code]) {
		/* �����B�E���ɍ������� */
		case 0:
			/* ���Ԃ������� */
			for (i=0; i<16; i++) {
				PSetOPNDisp(x, y, 0);
				y++;
			}
			y -= 16;
			/* ������� */
			for (i=0; i<8; i++) {
				for (j=1; j<=4; j++) {
					PSetOPNDisp(x + j, y, color);
				}
				y++;
			}
			/* ������� */
			for (i=0; i<8; i++) {
				for (j=1; j<=6; j++) {
					PSetOPNDisp(x + j, y, color);
				}
				y++;
			}
			break;

		/* �����B���E�ɍ������� */
		case 1:
			/* ���Ԃ������� */
			for (i=8; i<16; i++) {
				PSetOPNDisp(x, y + i, 0);
			}
			/* ������� */
			for (i=0; i<8; i++) {
				for (j=3; j<=4; j++) {
					PSetOPNDisp(x + j, y, color);
				}
				y++;
			}
			/* ������� */
			for (i=0; i<8; i++) {
				for (j=1; j<=6; j++) {
					PSetOPNDisp(x + j, y, color);
				}
				y++;
			}
			break;

		/* �����B�����ɍ������� */
		case 2:
			/* ���Ԃ������� */
			for (i=8; i<16; i++) {
				PSetOPNDisp(x, y + i, 0);
			}
			/* ������� */
			for (i=0; i<8; i++) {
				for (j=3; j<=6; j++) {
					PSetOPNDisp(x + j, y, color);
				}
				y++;
			}
			/* ������� */
			for (i=0; i<8; i++) {
				for (j=1; j<=6; j++) {
					PSetOPNDisp(x + j, y, color);
				}
				y++;
			}
			break;

		/* ���� */
		case 3:
			for (i=0; i<8; i++) {
				for (j=0; j<5; j++) {
					PSetOPNDisp(x + j, y, color);
				}
				y++;
			}
			break;

		default:
			ASSERT(FALSE);
	}
}

/*
 *	OPN�f�B�X�v���C�E�C���h�E
 *	���ՑS�`��
 *
 *	x, y�̓g���b�N��̊�_
 */
void FASTCALL AllKbdOPNDisp(int x, int y)
{
	int i;
	int j;

	/* �Q�d���[�v */
	for (i=0; i<8; i++) {
		for (j=0; j<12; j++) {
			KbdOPNDisp((i * 16) + j, x, y, -1);
		}
	}
}

/*
 *	OPN�f�B�X�v���C�E�C���h�E
 *	�Z�b�g�A�b�v
 */
static BOOL FASTCALL SetupOPNDisp(int *first, int *end)
{
	int i;
	int j;
	int y;
	BOOL flag[12];

	/* ������ */
	memset(flag, 0, sizeof(flag));

	/* ���� */
	y = 0;
	for (i=0; i<12; i++) {
		for (j=0; j<49; j++) {
			/* ��v�`�F�b�N */
			if (cnOPNDisp[i][j * 2] == ctOPNDisp[i][j * 2]) {
				if (cnOPNDisp[i][j*2+1] == ctOPNDisp[i][j*2+1]) {
					continue;
				}
			}

			/* �`�� */
			ChrOPNDisp(ctOPNDisp[i][j * 2], j * 8, y, ctOPNDisp[i][j * 2 + 1]);

			/* �R�s�[ */
			cnOPNDisp[i][j * 2] = ctOPNDisp[i][j * 2];
			cnOPNDisp[i][j * 2 + 1] = ctOPNDisp[i][j * 2 + 1];

			/* �t���O���� */
			flag[i] = TRUE;
		}

		/* ���� */
		y += (8 * 3 + 1);
	}

	/* ���� */
	y = 8;
	for (i=0; i<12; i++) {
		/* �����ԂȂ�A�L�[�I�t��Ԃɂ���̂��挈 */
		if (knOPNDisp[i] == -2) {
			AllKbdOPNDisp(0, y);
			knOPNDisp[i] = -1;
			flag[i] = TRUE;
		}

		/* �`�F�b�N */
		if (knOPNDisp[i] != ktOPNDisp[i]) {
			/* �L�[�I����ԂȂ�A���� */
			if (knOPNDisp[i] >= 0) {
				KbdOPNDisp(knOPNDisp[i], 0, y, -1);
			}
			/* �L�[�I���ɂ���Ȃ�A���� */
			if (ktOPNDisp[i] >= 0) {
				if ((i % 6) < 3) {
					KbdOPNDisp(ktOPNDisp[i], 0, y, 10);
				}
				else {
					KbdOPNDisp(ktOPNDisp[i], 0, y, 9);
				}
			}
			/* �X�V */
			knOPNDisp[i] = ktOPNDisp[i];
			flag[i] = TRUE;
		}

		/* ���� */
		y += (8 * 3 + 1);
	}

	/* ���x�� */
	y = 0;
	for (i=0; i<12; i++) {
		/* ��v�`�F�b�N */
		if (lnOPNDisp[i] != ltOPNDisp[i]) {
			/* �؂�ւ��_�ƂȂ�x���W�����߂� */
			if (ltOPNDisp[i] >= 500) {
				ltOPNDisp[i] = 499;
			}
			j = 192 * ltOPNDisp[i];
			j /= 500;

			/* �`�� */
			BfOPNDisp(200, y, j, 7, 11);
			BfOPNDisp(200 + j, y, 192 - j, 7, 0);

			/* �X�V */
			lnOPNDisp[i] = ltOPNDisp[i];
			flag[i] = TRUE;
		}

		/* ���� */
		y += (8 * 3 + 1);
	}

	/* flag���������� */
	y = -1;
	j = 12;
	for (i=0; i<12; i++) {
		if (flag[i]) {
			/* ���������� */
			if (i < j) {
				j = i;
			}
			/* ��������� */
			if (y < i) {
				y = i;
			}
		}
	}

	/* j����y�܂ŁA��������΂悢 */
	if (y >= 0) {
		*first = j;
		*end = y;
		return TRUE;
	}

	/* �`��̕K�v�Ȃ� */
	return FALSE;
}

/*
 *	�����e�[�u��
 *	�������Ƃ̂������肵�������łȂ��A���̒��Ԃ̂������l��\��������
 */
static const double pitch_table[] = {
	31.772, 33.661, 35.663, 37.784, 40.030, 42.411, 44.933, 47.605, 50.435, 53.434, 56.612, 59.978,
	63.544, 67.323, 71.326, 75.567, 80.061, 84.822, 89.865, 95.209, 100.870, 106.869, 113.223, 119.956,
	127.089, 134.646, 142.652, 151.135, 160.122, 169.643, 179.731, 190.418, 201.741, 213.737, 226.446, 239.912,
	254.178, 269.292, 285.305, 302.270, 320.244, 339.287, 359.461, 380.836, 403.482, 427.474, 452.893, 479.823,
	508.356, 538.584, 570.610, 604.540, 640.488, 678.573, 718.923, 761.672, 806.964, 854.948, 905.786, 959.647,
	1016.711, 1077.168, 1141.220, 1209.080, 1280.975, 1357.146, 1437.846, 1523.345, 1613.927, 1709.896, 1811.572, 1919.293,
	2033.422, 2154.336, 2282.439, 2418.160, 2561.951, 2714.292, 2875.692, 3046.689, 3227.855, 3419.792, 3623.144, 3838.587,
	4066.845, 4308.672, 4564.878, 4836.319, 5123.901, 5428.584, 5751.384, 6093.378, 6455.709, 6839.585, 7246.287, 7677.173,
	8133.681
};

/*
 *	�������R�[�h�ϊ�
 *	-1�͔͈͊O������
 */
static int FASTCALL ConvOPNDisp(double freq)
{
	int i;
	int ret;

	/* �͈͊O���`�F�b�N */
	if (freq < pitch_table[0]) {
		return -1;
	}
	if (pitch_table[96] <= freq) {
		return -1;
	}

	/* ���[�v�A���� */
	ret = -1;
	for (i=0; i<96; i++) {
		if ((pitch_table[i] <= freq) && (freq <pitch_table[i + 1])) {
			/* i��ϊ� */
			ret = i / 12;
			ret <<= 4;
			ret += (i % 12);
			break;
		}
	}

	return ret;
}

/*
 *	OPN�f�B�X�v���C�E�C���h�E
 *	FM�������R�[�h�ϊ�
 */
static int FASTCALL FMConvOPNDisp(BYTE a4, BYTE a0, int scale)
{
	int oct;
	int fnum;
	double freq;
	double d;

	/* Octave, F-Number�����߂� */
	oct = (int)a4;
	oct >>= 3;
	fnum = (int)a4;
	fnum &= 0x07;
	fnum <<= 8;
	fnum |= (int)a0;

	/* ���g�������߂� */
	freq = 1.2288;
	freq *= 1000000;
	while (oct != 0) {
		freq *= 2;
		oct--;
	}
	freq *= fnum;
	d = (double)(1 << 20);
	d *= 12;
	d *= scale;
	freq /= d;

	/* �ϊ� */
	return ConvOPNDisp(freq);
}

/*
 *	OPN�f�B�X�v���C�E�C���h�E
 *	PSG�������R�[�h�ϊ�
 */
static int FASTCALL PSGConvOPNDisp(BYTE low, BYTE high, int scale)
{
	int pitch;
	double freq;
	double d;

	/* �s�b�`�Z�o */
	pitch = (int)high;
	pitch &= 0x0f;
	pitch <<= 8;
	pitch |= (int)low;

	/* ���g�������߂� */
	freq = 1.2288;
	freq *= 1000000;
	d = 8;
	d *= pitch;
	switch (scale) {
		case 3:
			d *= 2;
			break;
		case 6:
			d *= 4;
			break;
		case 2:
			d *= 1;
			break;
	}
	if (d == 0) {
		freq = 0;
	}
	else {
		freq /= d;
	}

	/* �ϊ� */
	return ConvOPNDisp(freq);
}

/*
 *	OPN�f�B�X�v���C�E�C���h�E
 *	�L�[�{�[�h�X�e�[�^�X
 */
static void FASTCALL StatKbdOPNDisp(void)
{
	int i;

	/* FM���� */
	for (i=0; i<3; i++) {
		if (!opn_key[i]) {
			ktOPNDisp[i + 0] = -1;
		}
		else {
			ktOPNDisp[i + 0] = FMConvOPNDisp(opn_reg[0xa4 + i],
									opn_reg[0xa0 + i], opn_scale);
		}
		if (!whg_key[i]) {
			ktOPNDisp[i + 6] = -1;
		}
		else {
			ktOPNDisp[i + 6] = FMConvOPNDisp(whg_reg[0xa4 + i],
									whg_reg[0xa0 + i], whg_scale);
		}
	}

	/* PSG���� */
	for (i=0; i<3; i++) {
		if (ltOPNDisp[i + 3] > 0) {
			ktOPNDisp[i + 3] = PSGConvOPNDisp(opn_reg[i * 2 + 0],
									opn_reg[i * 2 + 1], opn_scale);
		}
		else {
			ktOPNDisp[i + 3] = -1;
		}

		if (ltOPNDisp[i + 9] > 0) {
			ktOPNDisp[i + 9] = PSGConvOPNDisp(whg_reg[i * 2 + 0],
									whg_reg[i * 2 + 1], whg_scale);
		}
		else {
			ktOPNDisp[i + 9] = -1;
		}
	}
}

/*
 *	OPN�f�B�X�v���C�E�C���h�E
 *	���x���X�e�[�^�X
 */
static void FASTCALL StatLevOPNDisp(void)
{
	BYTE m;
	BYTE p;
	int i;

	/* ��U�擾���� */
	for (i=0; i<12; i++) {
		ltOPNDisp[i] = GetLevelSnd(i);
	}

	/* SSG�}�X�N�`�F�b�N:OPN */
	m = opn_reg[7];
	p = 9;
	for (i=0; i<3; i++) {
		if ((m & p) == p) {
			ltOPNDisp[i + 0 + 3] = 0;
		}
		p <<= 1;
	}

	/* SSG�}�X�N�`�F�b�N:WHG */
	m = whg_reg[7];
	p = 9;
	for (i=0; i<3; i++) {
		if ((m & p) == p) {
			ltOPNDisp[i + 6 + 3] = 0;
		}
		p <<= 1;
	}
}

/*
 *	OPN�f�B�X�v���C�E�C���h�E
 *	������Z�b�g
 */
static void FASTCALL StrOPNDisp(char *string, int x, int y, int color)
{
	char ch;

	ASSERT(string);
	ASSERT((x >= 0) && (x < 49));
	ASSERT((y >= 0) && (y < 12));
	ASSERT((color >= 0) && (color < 16));

	/* �������[�v */
	for (;;) {
		/* �����擾 */
		ch = *string++;
		if (ch == '\0') {
			break;
		}

		/* x�`�F�b�N */
		if (x >= 49) {
			continue;
		}

		/* �Z�b�g */
		ctOPNDisp[y][x * 2 + 0] = ch;
		ctOPNDisp[y][x * 2 + 1] = (BYTE)color;
		x++;
	}
}

/*
 *	OPN�f�B�X�v���C�E�C���h�E
 *	�����f�[�^�Z�b�g
 */
static void FASTCALL StatChrOPNDisp(void)
{
	int i;
	int j;
	char string[128];

	for (i=0; i<12; i++) {
		/* �`�����l�� */
		if (i < 6) {
			sprintf(string, "OPN%1d", (i % 6) + 1);
		}
		else {
			sprintf(string, "WHG%1d", (i % 6) + 1);
		}
		StrOPNDisp(string, 0, i, 7);

		/* ���g�� */
		if ((i % 6) < 3) {
			if (i < 6) {
				j = opn_reg[0xa4 + (i % 6)];
				j <<= 8;
				j |= opn_reg[0xa0 + (i % 6)];
			}
			else {
				j = whg_reg[0xa4 + (i % 6)];
				j <<= 8;
				j |= whg_reg[0xa0 + (i % 6)];
			}
			sprintf(string, "F:$%04X", j);
		}
		else {
			if (i < 6) {
				j = opn_reg[((i % 6) - 3) * 2 + 1];
				j &= 0x0f;
				j <<= 8;
				j |= opn_reg[((i % 6) - 3) * 2 + 0];
			}
			else {
				j = whg_reg[((i % 6) - 3) * 2 + 1];
				j &= 0x0f;
				j <<= 8;
				j |= whg_reg[((i % 6) - 3) * 2 + 0];
			}
			sprintf(string, "P:$%04X", j);
		}
		StrOPNDisp(string, 6, i, 12);

		/* �{�����[�� */
		if ((i % 6) < 3) {
			if (i < 6) {
				j = opn_reg[0x4c + (i % 6)];
			}
			else {
				j = whg_reg[0x4c + (i % 6)];
			}
			/* 7bit�̂ݗL�� */
			j &= 0x7f;
			sprintf(string, "V:%03d", 127 - j);
		}
		else {
			if (i < 6) {
				j = opn_reg[8 + (i % 6) - 3];
			}
			else {
				j = whg_reg[8 + (i % 6) - 3];
			}
			/* 0�`15�ƁA16�ȏ�ɕ����� */
			j &= 0x1f;
			if (j >= 0x10) {
				j = 0x10;
			}
			sprintf(string, "V:%03d", j);
		}
		StrOPNDisp(string, 14, i, 12);

		/* �L�[�I�� */
		if ((i % 6) < 3) {
			if (i < 6) {
				j = opn_key[(i % 6)];
			}
			else {
				j = whg_key[(i % 6)];
			}
			if (j) {
				strcpy(string, "KEYON");
			}
			else {
				strcpy(string, "     ");
			}
			StrOPNDisp(string, 20, i, 12);
		}

		/* �~�L�T */
		if ((i % 6) >= 3) {
			if (i < 6) {
				j = opn_reg[7];
			}
			else {
				j = whg_reg[7];
			}
			if ((i % 6) == 4) {
				j >>= 1;
			}
			if ((i % 6) == 5) {
				j >>= 2;
			}
			j &= 0x09;

			switch (j) {
				case 0:
					strcpy(string, "T + N");
					break;
				case 1:
					strcpy(string, "NOISE");
					break;
				case 8:
					strcpy(string, "TONE ");
					break;
				case 9:
					strcpy(string, "     ");
					break;
			}
			StrOPNDisp(string, 20, i, 12);
		}
	}
}

/*
 *	OPN�f�B�X�v���C�E�C���h�E
 *	�`��
 */
static void FASTCALL DrawOPNDisp(HDC hDC)
{
	HDC hMemDC;
	HBITMAP hBitmap;
	int first, end;

	ASSERT(hDC);

	/* �r�b�g�}�b�v�n���h���������Ȃ�A�������Ȃ� */
	if (!hOPNDisp) {
		return;
	}

	/* �X�e�[�^�X�`�F�b�N */
	StatLevOPNDisp();
	StatKbdOPNDisp();
	StatChrOPNDisp();

	/* �Z�b�g�A�b�v���`��`�F�b�N */
	if (!SetupOPNDisp(&first, &end)) {
		return;
	}

	/* ������DC���쐬���A�r�b�g�}�b�v���Z���N�g */
	hMemDC = CreateCompatibleDC(hDC);
	ASSERT(hMemDC);
	hBitmap = (HBITMAP)SelectObject(hMemDC, hOPNDisp);

	/* �f�X�N�g�b�v�̏󋵂�����ł́A���ۂ����蓾��l */
	if (hBitmap) {
		/* �p���b�g�ݒ�ABitBlt */
		SetDIBColorTable(hMemDC, 0, 16, rgbOPNDisp);
		BitBlt(hDC,
				0, (8 * 3 + 1) * first,
				rOPNDisp.right, (8 * 3 + 1) * (end - first + 1), hMemDC,
				0, (8 * 3 + 1) * first, SRCCOPY);

		/* �I�u�W�F�N�g�ăZ���N�g */
		SelectObject(hMemDC, hBitmap);
	}

	/* ������DC�폜 */
	DeleteDC(hMemDC);
}

/*
 *	OPN�f�B�X�v���C�E�C���h�E
 *	���t���b�V��
 */
void FASTCALL RefreshOPNDisp(void)
{
	HWND hWnd;
	HDC hDC;

	/* ��ɌĂ΂��̂ŁA���݃`�F�b�N���邱�� */
	if (hSubWnd[SWND_OPNDISP] == NULL) {
		return;
	}

	/* �`�� */
	hWnd = hSubWnd[SWND_OPNDISP];
	hDC = GetDC(hWnd);
	DrawOPNDisp(hDC);
	ReleaseDC(hWnd, hDC);
}

/*
 *	OPN�f�B�X�v���C�E�C���h�E
 *	�ĕ`��
 */
static void FASTCALL PaintOPNDisp(HWND hWnd)
{
	HDC hDC;
	PAINTSTRUCT ps;
	int i;

	ASSERT(hWnd);

	/* ���[�N�G���A�����ׂĖ���� */
	for (i=0; i<12; i++) {
		knOPNDisp[i] = -2;
		lnOPNDisp[i] = -1;
	}
	memset(cnOPNDisp, 0xff, sizeof(cnOPNDisp));

	/* �`�� */
	hDC = BeginPaint(hWnd, &ps);
	ASSERT(hDC);
	DrawOPNDisp(hDC);
	EndPaint(hWnd, &ps);
}

/*
 *	OPN�f�B�X�v���C�E�C���h�E
 *	�E�C���h�E�v���V�[�W��
 */
static LRESULT CALLBACK OPNDispProc(HWND hWnd, UINT message,
								 WPARAM wParam, LPARAM lParam)
{
	int i;

	/* ���b�Z�[�W�� */
	switch (message) {
		/* �E�C���h�E�ĕ`�� */
		case WM_PAINT:
			/* ���b�N���K�v */
			LockVM();
			PaintOPNDisp(hWnd);
			UnlockVM();
			return 0;

		/* �w�i�`�� */
		case WM_ERASEBKGND:
			return TRUE;

		/* �E�C���h�E�폜 */
		case WM_DESTROY:
			/* ���C���E�C���h�E�֎����ʒm */
			for (i=0; i<SWND_MAXNUM; i++) {
				if (hSubWnd[i] == hWnd) {
					hSubWnd[i] = NULL;
				}
			}
			/* �r�b�g�}�b�v��� */
			if (hOPNDisp) {
				DeleteObject(hOPNDisp);
				hOPNDisp = NULL;
				pOPNDisp = NULL;
			}
			break;
	}

	/* �f�t�H���g �E�C���h�E�v���V�[�W�� */
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/*
 *	OPN�f�B�X�v���C�E�C���h�E
 *	������
 */
void FASTCALL InitOPNDisp(HWND hWnd)
{
	BITMAPINFOHEADER *pbmi;
	HDC hDC;
	int i;

	/* �S�̃��[�N������ */
	pOPNDisp = NULL;
	hOPNDisp = NULL;

	/* �\���Ǘ����[�N������ */
	for (i=0; i<12; i++) {
		knOPNDisp[i] = -2;
		ktOPNDisp[i] = -1;
		lnOPNDisp[i] = -1;
		ltOPNDisp[i] = 0;
	}
	memset(cnOPNDisp, 0xff, sizeof(cnOPNDisp));
	memset(ctOPNDisp, 0, sizeof(ctOPNDisp));

	/* �r�b�g�}�b�v�w�b�_���� */
	pbmi = (BITMAPINFOHEADER*)malloc(sizeof(BITMAPINFOHEADER)
										 + sizeof(RGBQUAD) * 16);
	if (pbmi) {
		memset(pbmi, 0, sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 16);
		pbmi->biSize = sizeof(BITMAPINFOHEADER);
		pbmi->biWidth = rOPNDisp.right;
		pbmi->biHeight = -rOPNDisp.bottom;
		pbmi->biPlanes = 1;
		pbmi->biBitCount = 4;
		pbmi->biCompression = BI_RGB;

		/* DC�擾�ADIB�Z�N�V�����쐬 */
		hDC = GetDC(hWnd);
		hOPNDisp = CreateDIBSection(hDC, (BITMAPINFO*)pbmi, DIB_RGB_COLORS,
								(void**)&pOPNDisp, NULL, 0);
		ReleaseDC(hWnd, hDC);
		free(pbmi);
	}
}

/*
 *	OPN�f�B�X�v���C�E�C���h�E
 *	�E�C���h�E�쐬
 */
HWND FASTCALL CreateOPNDisp(HWND hParent, int index)
{
	WNDCLASSEX wcex;
	char szClassName[] = "XM7_OPNDisp";
	char szWndName[128];
	RECT rect;
	RECT crect, wrect;
	HWND hWnd;

	ASSERT(hParent);

	/* �E�C���h�E��`���v�Z */
	rect.left = lCharWidth * index;
	rect.top = lCharHeight * index;
	if (rect.top >= 380) {
		rect.top -= 380;
	}

	/* �E�C���h�E�����́A49�h�b�g * 8 �I�N�^�[�u */
	rect.right = 49 * 8;
	/* �E�C���h�E�c���́A8 * 3 + 1�h�b�g * 12 �`�����l�� */
	rect.bottom = (8 * 3 + 1) * 12;

	/* �E�C���h�E�^�C�g�������� */
	LoadString(hAppInstance, IDS_SWND_OPNDISP,
				szWndName, sizeof(szWndName));

	/* �E�C���h�E�N���X�̓o�^ */
	memset(&wcex, 0, sizeof(wcex));
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_VREDRAW | CS_HREDRAW;
	wcex.lpfnWndProc = OPNDispProc;
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

	/* �E�C���h�E�쐬 */
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

	/* �L���Ȃ�A�T�C�Y�␳���Ď�O�ɒu�� */
	if (hWnd) {
		GetWindowRect(hWnd, &wrect);
		GetClientRect(hWnd, &crect);
		wrect.right += (rect.right - crect.right);
		wrect.bottom += (rect.bottom - crect.bottom);
		SetWindowPos(hWnd, HWND_TOP, wrect.left, wrect.top,
			wrect.right - wrect.left, wrect.bottom - wrect.top, SWP_NOMOVE);

		/* ��`��ۑ� */
		rOPNDisp = rect;

		/* ���̑������� */
		InitOPNDisp(hWnd);
	}

	/* ���ʂ������A�� */
	return hWnd;
}

/*-[ OPN���W�X�^�E�C���h�E ]-------------------------------------------------*/

/*
 *	OPN���W�X�^�E�C���h�E
 *	�Z�b�g�A�b�v(���W�X�^�Z�b�g)
 */
static void FASTCALL SetupOPNRegSub(BYTE *p, int cx, BYTE *reg, int x, int y)
{
	int i;
	int j;
	char string[128];

	/* X�����K�C�h�\�� */
	strcpy(string, "+0+1+2+3+4+5+6+7+8+9+A+B+C+D+E+F");
	memcpy(&p[cx * (y + 0) + (x + 0)], string, strlen(string));

	/* ���[�v */
	for (i=0; i<16; i++) {
		for (j=0; j<16; j++) {
			sprintf(string, "%02X", reg[i * 16 + j]);
			memcpy(&p[cx * (y + i + 2) + x + j * 2], string, strlen(string));
		}
	}
}

/*
 *	OPN���W�X�^�E�C���h�E
 *	�Z�b�g�A�b�v
 */
static void FASTCALL SetupOPNReg(BYTE *p, int x, int y)
{
	char string[128];
	int i;

	ASSERT(p);
	ASSERT(x > 0);
	ASSERT(y > 0);

	/* ��U�X�y�[�X�Ŗ��߂� */
	memset(p, 0x20, x * y);

	/* �^�C�g�� */
	strcpy(string, "OPN (Standard)");
	memcpy(&p[x * 0 + 4], string, strlen(string));
	strcpy(string, "WHG (Extension)");
	memcpy(&p[x * 0 + 4 + 32 + 2], string, strlen(string));

	/* Y�����K�C�h�\�� */
	for (i=0; i<16; i++) {
		sprintf(string, "+%02X", i * 16);
		memcpy(&p[x * (i + 4) + 0], string, strlen(string));
	}

	/* OPN */
	SetupOPNRegSub(p, x, opn_reg, 4, 2);

	/* WHG */
	SetupOPNRegSub(p, x, whg_reg, 38, 2);
}

/*
 *	OPN���W�X�^�E�C���h�E
 *	�`��
 */
static void FASTCALL DrawOPNReg(HWND hWnd, HDC hDC)
{
	RECT rect;
	BYTE *p, *q;
	int x, y;
	int xx, yy;
	HFONT hFont;
	HFONT hBackup;

	ASSERT(hWnd);
	ASSERT(hDC);

	/* �E�C���h�E�W�I���g���𓾂� */
	GetClientRect(hWnd, &rect);
	x = rect.right / lCharWidth;
	y = rect.bottom / lCharHeight;
	if ((x == 0) || (y == 0)) {
		return;
	}

	/* �Z�b�g�A�b�v */
	p = pOPNReg;
	if (!p) {
		return;
	}
	SetupOPNReg(p, x, y);

	/* �t�H���g�Z���N�g */
	hFont = CreateTextFont();
	hBackup = SelectObject(hDC, hFont);

	/* ��r�`�� */
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

	/* �I�� */
	SelectObject(hDC, hBackup);
	DeleteObject(hFont);
}

/*
 *	OPN���W�X�^�E�C���h�E
 *	���t���b�V��
 */
void FASTCALL RefreshOPNReg(void)
{
	HWND hWnd;
	HDC hDC;

	/* ��ɌĂ΂��̂ŁA���݃`�F�b�N���邱�� */
	if (hSubWnd[SWND_OPNREG] == NULL) {
		return;
	}

	/* �`�� */
	hWnd = hSubWnd[SWND_OPNREG];
	hDC = GetDC(hWnd);
	DrawOPNReg(hWnd, hDC);
	ReleaseDC(hWnd, hDC);
}

/*
 *	OPN���W�X�^�E�C���h�E
 *	�ĕ`��
 */
static void FASTCALL PaintOPNReg(HWND hWnd)
{
	HDC hDC;
	PAINTSTRUCT ps;
	RECT rect;
	BYTE *p;
	int x, y;

	ASSERT(hWnd);

	/* �|�C���^��ݒ� */
	p = pOPNReg;

	/* �E�C���h�E�W�I���g���𓾂� */
	GetClientRect(hWnd, &rect);
	x = rect.right / lCharWidth;
	y = rect.bottom / lCharHeight;

	/* �㔼�G���A��FF�Ŗ��߂� */
	if ((x > 0) && (y > 0)) {
		memset(&p[x * y], 0xff, x * y);
	}

	/* �`�� */
	hDC = BeginPaint(hWnd, &ps);
	ASSERT(hDC);
	DrawOPNReg(hWnd, hDC);
	EndPaint(hWnd, &ps);
}

/*
 *	OPN���W�X�^�E�C���h�E
 *	�E�C���h�E�v���V�[�W��
 */
static LRESULT CALLBACK OPNRegProc(HWND hWnd, UINT message,
								 WPARAM wParam, LPARAM lParam)
{
	int i;

	/* ���b�Z�[�W�� */
	switch (message) {
		/* �E�C���h�E�ĕ`�� */
		case WM_PAINT:
			/* ���b�N���K�v */
			LockVM();
			PaintOPNReg(hWnd);
			UnlockVM();
			return 0;

		/* �E�C���h�E�폜 */
		case WM_DESTROY:
			/* ���C���E�C���h�E�֎����ʒm */
			for (i=0; i<SWND_MAXNUM; i++) {
				if (hSubWnd[i] == hWnd) {
					hSubWnd[i] = NULL;
					/* Draw�o�b�t�@�폜 */
					free(pOPNReg);
					pOPNReg = NULL;
				}
			}
			break;
	}

	/* �f�t�H���g �E�C���h�E�v���V�[�W�� */
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/*
 *	OPN���W�X�^�E�C���h�E
 *	�E�C���h�E�쐬
 */
HWND FASTCALL CreateOPNReg(HWND hParent, int index)
{
	WNDCLASSEX wcex;
	char szClassName[] = "XM7_OPNReg";
	char szWndName[128];
	RECT rect;
	RECT crect, wrect;
	HWND hWnd;

	ASSERT(hParent);

	/* �E�C���h�E��`���v�Z */
	rect.left = lCharWidth * index;
	rect.top = lCharHeight * index;
	if (rect.top >= 380) {
		rect.top -= 380;
	}
	rect.right = lCharWidth * 70;
	rect.bottom = lCharHeight * 20;

	/* �E�C���h�E�^�C�g��������A�o�b�t�@�m�� */
	LoadString(hAppInstance, IDS_SWND_OPNREG,
				szWndName, sizeof(szWndName));
	pOPNReg = malloc(2 * (rect.right / lCharWidth) *
								(rect.bottom / lCharHeight));

	/* �E�C���h�E�N���X�̓o�^ */
	memset(&wcex, 0, sizeof(wcex));
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_VREDRAW | CS_HREDRAW;
	wcex.lpfnWndProc = OPNRegProc;
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

	/* �E�C���h�E�쐬 */
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

	/* �L���Ȃ�A�T�C�Y�␳���Ď�O�ɒu�� */
	if (hWnd) {
		GetWindowRect(hWnd, &wrect);
		GetClientRect(hWnd, &crect);
		wrect.right += (rect.right - crect.right);
		wrect.bottom += (rect.bottom - crect.bottom);
		SetWindowPos(hWnd, HWND_TOP, wrect.left, wrect.top,
			wrect.right - wrect.left, wrect.bottom - wrect.top, SWP_NOMOVE);
	}

	/* ���ʂ������A�� */
	return hWnd;
}

/*-[ FDC�E�C���h�E ]---------------------------------------------------------*/

/*
 *	FDC�E�C���h�E
 *	�Z�b�g�A�b�v(�R�}���h)
 */
static void FASTCALL SetupFDCCmd(BYTE *p, int x, int cx)
{
	BYTE high, low;
	char buffer[128];
	int y;

	ASSERT(p);
	ASSERT(cx > 0);

	/* ������ */
	y = 0;

	if (fdc_command == 0xff) {
		/* �R�}���h���� */
		strcpy(buffer, "NO COMMAND");
		memcpy(&p[x + y * cx], buffer, strlen(buffer));
		return;
	}

	high = (BYTE)(fdc_command >> 4);
	low = (BYTE)(fdc_command & 0x0f);
	if (high < 8) {
		/* TYPE I */
		switch (high) {
			/* RESTORE */
			case 0:
				strcpy(buffer, "RESTORE");
				break;
			/* SEEK */
			case 1:
				strcpy(buffer, "SEEK");
				break;
			/* STEP */
			case 2:
			case 3:
				strcpy(buffer, "STEP");
				break;
			/* STEP IN */
			case 4:
			case 5:
				strcpy(buffer, "STEP IN");
				break;
			/* STEP IN */
			case 6:
			case 7:
				strcpy(buffer, "STEP OUT");
				break;
		}
		memcpy(&p[x + y * cx], buffer, strlen(buffer));
		y++;

		strcpy(buffer, "(TYPE I)");
		memcpy(&p[x + y * cx], buffer, strlen(buffer));
		y += 3;

		strcpy(buffer, "Step Rate     ");
		switch (low & 0x03) {
			case 0:
				strcat(buffer, " 6ms");
				break;
			case 1:
				strcat(buffer, "12ms");
				break;
			case 2:
				strcat(buffer, "20ms");
				break;
			case 3:
				strcat(buffer, "30ms");
				break;
		}
		memcpy(&p[x + y * cx], buffer, strlen(buffer));
		y++;

		strcpy(buffer, "Verify         ");
		if (low & 0x04) {
			strcat(buffer, " On");
		}
		else {
			strcat(buffer, "Off");
		}
		memcpy(&p[x + y * cx], buffer, strlen(buffer));
		y++;

		strcpy(buffer, "Head        ");
		if (low & 0x08) {
			strcat(buffer, "  Load");
		}
		else {
			strcat(buffer, "Unload");
		}
		memcpy(&p[x + y * cx], buffer, strlen(buffer));
		y++;

		if (high >= 2) {
			strcpy(buffer, "Update Track   ");
			if (low & 0x04) {
				strcat(buffer, " On");
			}
			else {
				strcat(buffer, "Off");
			}
			memcpy(&p[x + y * cx], buffer, strlen(buffer));
		}
		return;
	}

	if ((high & 0x0c) == 0x08) {
		/* TYPE II */
		if (high < 0x0a) {
			strcpy(buffer, "READ DATA");
		} else {
			strcpy(buffer, "WRITE DATA");
		}
		memcpy(&p[x + y * cx], buffer, strlen(buffer));
		y++;

		strcpy(buffer, "(TYPE II)");
		memcpy(&p[x + y * cx], buffer, strlen(buffer));
		y += 2;

		strcpy(buffer, "Multi Sector   ");
		if (high & 0x01) {
			strcat(buffer, " On");
		}
		else {
			strcat(buffer, "Off");
		}
		memcpy(&p[x + y * cx], buffer, strlen(buffer));
		y++;

		strcpy(buffer, "Compare Side   ");
		if (low & 0x02) {
			if (low & 0x08) {
				strcat(buffer, "  0");
			}
			else {
				strcat(buffer, "  1");
			}
		} else {
			strcat(buffer, "Off");
		}
		memcpy(&p[x + y * cx], buffer, strlen(buffer));
		y++;

		strcpy(buffer, "Addr. Mark ");
		if (low & 0x01) {
			strcat(buffer, "Deleted");
		}
		else {
			strcat(buffer, " Normal");
		}
		memcpy(&p[x + y * cx], buffer, strlen(buffer));
		y++;

		sprintf(buffer, "Total Bytes   %04X", fdc_totalcnt);
		memcpy(&p[x + y * cx], buffer, strlen(buffer));
		y++;

		sprintf(buffer, "Xfer. Bytes   %04X", fdc_nowcnt);
		memcpy(&p[x + y * cx], buffer, strlen(buffer));
		y++;
	}

	/* Type I, II�͏I�� */
	if (high < 0x0c) {
		return;
	}

	/* Type IV */
	if (high == 0x0d) {
		strcpy(buffer, "FORCE INTERRUPT");
		memcpy(&p[x + y * cx], buffer, strlen(buffer));
		y++;

		strcpy(buffer, "(TYPE IV)");
		memcpy(&p[x + y * cx], buffer, strlen(buffer));
		y += 3;

		strcpy(buffer, "READY  In      ");
		if (low & 0x01) {
			strcat(buffer, "IRQ");
		}
		else {
			strcat(buffer, "Off");
		}
		memcpy(&p[x + y * cx], buffer, strlen(buffer));
		y++;

		strcpy(buffer, "READY Out      ");
		if (low & 0x02) {
			strcat(buffer, "IRQ");
		}
		else {
			strcat(buffer, "Off");
		}
		memcpy(&p[x + y * cx], buffer, strlen(buffer));
		y++;

		strcpy(buffer, "INDEX          ");
		if (low & 0x04) {
			strcat(buffer, "IRQ");
		}
		else {
			strcat(buffer, "Off");
		}
		memcpy(&p[x + y * cx], buffer, strlen(buffer));
		y++;

		strcpy(buffer, "One Shot       ");
		if (low & 0x08) {
			strcat(buffer, "IRQ");
		}
		else {
			strcat(buffer, "Off");
		}
		memcpy(&p[x + y * cx], buffer, strlen(buffer));
	}
	else {
		/* TYPE III */
		switch (high) {
			case 0x0c:
				strcpy(buffer, "READ ADDRESS");
				break;
			case 0x0e:
				strcpy(buffer, "READ TRACK");
				break;
			case 0x0f:
				strcpy(buffer, "WRITE TRACK");
				break;
		}
		memcpy(&p[x + y * cx], buffer, strlen(buffer));
		y++;

		strcpy(buffer, "(TYPE III)");
		memcpy(&p[x + y * cx], buffer, strlen(buffer));
		y += 5;

		sprintf(buffer, "Total Bytes   %04X", fdc_totalcnt);
		memcpy(&p[x + y * cx], buffer, strlen(buffer));

		sprintf(buffer, "Xfer. Bytes   %04X", fdc_nowcnt);
		memcpy(&p[x + y * cx], buffer, strlen(buffer));
	}

}

/*
 *	FDC�E�C���h�E
 *	�Z�b�g�A�b�v(���W�X�^)
 */
static void FASTCALL SetupFDCReg(BYTE *p, int x, int cx)
{
	int y;
	char buffer[128];

	ASSERT(p);
	ASSERT(cx > 0);

	/* ������ */
	y = 0;

	sprintf(buffer, "Drive   %1d", fdc_drvreg);
	if (fdc_motor) {
		strcat(buffer, "( On)");
	}
	else {
		strcat(buffer, "(Off)");
	}
	memcpy(&p[x + y * cx], buffer, strlen(buffer));
	y++;

	if (fdc_drvreg < FDC_DRIVES) {
		sprintf(buffer, "Track       %02X", fdc_track[fdc_drvreg]);
	}
	else {
		buffer[0] = '\0';
	}
	memcpy(&p[x + y * cx], buffer, strlen(buffer));
	y++;

	sprintf(buffer, "Track  Reg. %02X", fdc_trkreg);
	memcpy(&p[x + y * cx], buffer, strlen(buffer));
	y++;


	sprintf(buffer, "Sector Reg. %02X", fdc_secreg);
	memcpy(&p[x + y * cx], buffer, strlen(buffer));
	y++;

	sprintf(buffer, "Side   Reg. %02X", fdc_sidereg);
	memcpy(&p[x + y * cx], buffer, strlen(buffer));
	y++;

	sprintf(buffer, "Data   Reg. %02X", fdc_datareg);
	memcpy(&p[x + y * cx], buffer, strlen(buffer));
	y++;

	if (fdc_drqirq & 0x80) {
		strcpy(buffer, "DRQ         On");
	}
	else {
		strcpy(buffer, "DRQ        Off");
	}
	memcpy(&p[x + y * cx], buffer, strlen(buffer));
	y++;

	if (fdc_drqirq & 0x40) {
		strcpy(buffer, "IRQ         On");
	}
	else {
		strcpy(buffer, "IRQ        Off");
	}
	memcpy(&p[x + y * cx], buffer, strlen(buffer));
}

/*
 *	FDC�E�C���h�E
 *	�Z�b�g�A�b�v(�X�e�[�^�X)
 */
static void FASTCALL SetupFDCStat(BYTE *p, int x, int cx)
{
	int y;
	int type;
	int i;
	BYTE dat;
	BYTE bit;
	char buffer[128];

	ASSERT(p);
	ASSERT(cx > 0);

	/* ������ */
	y = 0;

	/* �^�C�v���� */
	type = 0;
	if ((fdc_command & 0xe0) == 0x80) {
		/* READ DATA */
		type = 1;
	}
	if ((fdc_command & 0xe0) == 0xa0) {
		/* WRITE DATA */
		type = 2;
	}
	if ((fdc_command & 0xf0) == 0xc0) {
		/* READ ADDRESS */
		type = 1;
	}
	if ((fdc_command & 0xf0) == 0xe0) {
		/* READ TRACK */
		type = 1;
	}
	if ((fdc_command & 0xf0) == 0xf0) {
		/* WRITE TRACK */
		type = 2;
	}

	/* �����ݒ� */
	dat = fdc_status;
	bit = 0x80;

	/* �W�r�b�g���[�v */
	for (i=7; i>=0; i--) {
		sprintf(buffer, "bit%d ", i);
		if (dat & bit) {
			switch (i) {
				/* BUSY */
				case 0:
					strcat(buffer, "BUSY");
					break;
				/* INDEX or DATA REQUEST */
				case 1:
					if (type == 0) {
						strcat(buffer, "INDEX");
					}
					else {
						strcat(buffer, "DATA REQUEST");
					}
					break;
				/* TRACK00 or LOST DATA */
				case 2:
					if (type == 0) {
						strcat(buffer, "TRACK00");
					}
					else {
						strcat(buffer, "LOST DATA");
					}
					break;
				/* CRC ERROR */
				case 3:
					strcat(buffer, "CRC ERROR");
					break;
				/* SEEK ERROR or RECORD NOT FOUND */
				case 4:
					if (type == 0) {
						strcat(buffer, "SEEK ERROR");
					}
					else {
						strcat(buffer, "RECORD NOT FOUND");
					}
					break;
				/* HEAD ENGAGED or RECORD TYPE or WRITE FAULT */
				case 5:
					switch (type) {
						case 0:
							strcat(buffer, "HEAD ENGAGED");
							break;
						case 1:
							strcat(buffer, "RECORD TYPE");
							break;
						case 2:
							strcat(buffer, "WRITE FAULT");
							break;
					}
					break;
				/* WRITE PROTECT */
				case 6:
					strcat(buffer, "WRITE PROTECT");
					break;
				/* NOT READY */
				case 7:
					strcat(buffer, "NOT READY");
					break;
			}
		}
		else {
			strcat(buffer, "----------------");
		}
		bit >>= 1;
		memcpy(&p[x + y * cx], buffer, strlen(buffer));
		y++;
	}
}

/*
 *	FDC�E�C���h�E
 *	�Z�b�g�A�b�v
 */
static void FASTCALL SetupFDC(BYTE *p, int x, int y)
{
	ASSERT(p);
	ASSERT(x > 0);
	ASSERT(y > 0);

	/* ��U�X�y�[�X�Ŗ��߂� */
	memset(p, 0x20, x * y);

	/* �T�u�֐����Ă� */
	SetupFDCCmd(p, 0, x);
	SetupFDCReg(p, 20, x);
	SetupFDCStat(p, 36, x);
}

/*
 *	FDC�E�C���h�E
 *	�`��
 */
static void FASTCALL DrawFDC(HWND hWnd, HDC hDC)
{
	RECT rect;
	BYTE *p, *q;
	int x, y;
	int xx, yy;
	HFONT hFont;
	HFONT hBackup;

	ASSERT(hWnd);
	ASSERT(hDC);

	/* �E�C���h�E�W�I���g���𓾂� */
	GetClientRect(hWnd, &rect);
	x = rect.right / lCharWidth;
	y = rect.bottom / lCharHeight;
	if ((x == 0) || (y == 0)) {
		return;
	}

	/* �Z�b�g�A�b�v */
	p = pFDC;
	if (!p) {
		return;
	}
	SetupFDC(p, x, y);

	/* �t�H���g�Z���N�g */
	hFont = CreateTextFont();
	hBackup = SelectObject(hDC, hFont);

	/* ��r�`�� */
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

	/* �I�� */
	SelectObject(hDC, hBackup);
	DeleteObject(hFont);
}

/*
 *	FDC�E�C���h�E
 *	���t���b�V��
 */
void FASTCALL RefreshFDC(void)
{
	HWND hWnd;
	HDC hDC;

	/* ��ɌĂ΂��̂ŁA���݃`�F�b�N���邱�� */
	if (hSubWnd[SWND_FDC] == NULL) {
		return;
	}

	/* �`�� */
	hWnd = hSubWnd[SWND_FDC];
	hDC = GetDC(hWnd);
	DrawFDC(hWnd, hDC);
	ReleaseDC(hWnd, hDC);
}

/*
 *	FDC�E�C���h�E
 *	�ĕ`��
 */
static void FASTCALL PaintFDC(HWND hWnd)
{
	HDC hDC;
	PAINTSTRUCT ps;
	RECT rect;
	BYTE *p;
	int x, y;

	ASSERT(hWnd);

	/* �|�C���^��ݒ� */
	p = pFDC;

	/* �E�C���h�E�W�I���g���𓾂� */
	GetClientRect(hWnd, &rect);
	x = rect.right / lCharWidth;
	y = rect.bottom / lCharHeight;

	/* �㔼�G���A��FF�Ŗ��߂� */
	if ((x > 0) && (y > 0)) {
		memset(&p[x * y], 0xff, x * y);
	}

	/* �`�� */
	hDC = BeginPaint(hWnd, &ps);
	ASSERT(hDC);
	DrawFDC(hWnd, hDC);
	EndPaint(hWnd, &ps);
}

/*
 *	FDC�E�C���h�E
 *	�E�C���h�E�v���V�[�W��
 */
static LRESULT CALLBACK FDCProc(HWND hWnd, UINT message,
								 WPARAM wParam, LPARAM lParam)
{
	int i;

	/* ���b�Z�[�W�� */
	switch (message) {
		/* �E�C���h�E�ĕ`�� */
		case WM_PAINT:
			/* ���b�N���K�v */
			LockVM();
			PaintFDC(hWnd);
			UnlockVM();
			return 0;

		/* �E�C���h�E�폜 */
		case WM_DESTROY:
			/* ���C���E�C���h�E�֎����ʒm */
			for (i=0; i<SWND_MAXNUM; i++) {
				if (hSubWnd[i] == hWnd) {
					hSubWnd[i] = NULL;
					/* Draw�o�b�t�@�폜 */
					free(pFDC);
					pFDC = NULL;
				}
			}
			break;
	}

	/* �f�t�H���g �E�C���h�E�v���V�[�W�� */
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/*
 *	FDC�E�C���h�E
 *	�E�C���h�E�쐬
 */
HWND FASTCALL CreateFDC(HWND hParent, int index)
{
	WNDCLASSEX wcex;
	char szClassName[] = "XM7_FDC";
	char szWndName[128];
	RECT rect;
	RECT crect, wrect;
	HWND hWnd;

	ASSERT(hParent);

	/* �E�C���h�E��`���v�Z */
	rect.left = lCharWidth * index;
	rect.top = lCharHeight * index;
	if (rect.top >= 380) {
		rect.top -= 380;
	}
	rect.right = lCharWidth * 57;
	rect.bottom = lCharHeight * 8;

	/* �E�C���h�E�^�C�g��������A�o�b�t�@�m�� */
	LoadString(hAppInstance, IDS_SWND_FDC,
				szWndName, sizeof(szWndName));
	pFDC = malloc(2 * (rect.right / lCharWidth) *
								(rect.bottom / lCharHeight));

	/* �E�C���h�E�N���X�̓o�^ */
	memset(&wcex, 0, sizeof(wcex));
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_VREDRAW | CS_HREDRAW;
	wcex.lpfnWndProc = FDCProc;
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

	/* �E�C���h�E�쐬 */
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

	/* �L���Ȃ�A�T�C�Y�␳���Ď�O�ɒu�� */
	if (hWnd) {
		GetWindowRect(hWnd, &wrect);
		GetClientRect(hWnd, &crect);
		wrect.right += (rect.right - crect.right);
		wrect.bottom += (rect.bottom - crect.bottom);
		SetWindowPos(hWnd, HWND_TOP, wrect.left, wrect.top,
			wrect.right - wrect.left, wrect.bottom - wrect.top, SWP_NOMOVE);
	}

	/* ���ʂ������A�� */
	return hWnd;
}

#endif	/* _WIN32 */
