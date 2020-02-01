/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API �T�u�E�B���h�E�P ]
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
 *	�O���[�o�� ���[�N
 */
HWND hSubWnd[SWND_MAXNUM];				/* �T�u�E�C���h�E */

/*
 *	�X�^�e�B�b�N ���[�N
 */
static WORD AddrHistory[16];			/* �A�h���X�q�X�g�� */
static WORD AddrBuf;					/* �A�h���X�o�b�t�@ */
static int AddrNum;						/* �A�h���X�q�X�g���� */
static BYTE *pBreakPoint;				/* �u���[�N�|�C���g Draw�o�b�t�@ */
static HMENU hBreakPoint;				/* �u���[�N�|�C���g ���j���[�n���h�� */
static POINT PosBreakPoint;				/* �u���[�N�|�C���g �}�E�X�ʒu */
static BYTE *pScheduler;				/* �X�P�W���[�� Draw�o�b�t�@ */
static BYTE *pCPURegisterMain;			/* CPU���W�X�^ Draw�o�b�t�@ */
static BYTE *pCPURegisterSub;			/* CPU���W�X�^ Draw�o�b�t�@ */
static BYTE *pDisAsmMain;				/* �t�A�Z���u�� Draw�o�b�t�@ */
static BYTE *pDisAsmSub;				/* �t�A�Z���u�� Draw�o�b�t�@ */
static WORD wDisAsmMain;				/* �t�A�Z���u�� �A�h���X */
static WORD wDisAsmSub;					/* �t�A�Z���u�� �A�h���X */
static HMENU hDisAsmMain;				/* �t�A�Z���u�� ���j���[�n���h�� */
static HMENU hDisAsmSub;				/* �t�A�Z���u�� ���j���[�n���h�� */
static BYTE *pMemoryMain;				/* �������_���v Draw�o�b�t�@ */
static BYTE *pMemorySub;				/* �������_���v Draw�o�b�t�@ */
static WORD wMemoryMain;				/* �������_���v �A�h���X */
static WORD wMemorySub;					/* �������_���v �A�h���X */
static HMENU hMemoryMain;				/* �������_���v ���j���[�n���h�� */
static HMENU hMemorySub;				/* �������_���v ���j���[�n���h�� */

/*-[ �A�h���X���̓_�C�A���O ]------------------------------------------------*/

/*
 *	�A�h���X���̓_�C�A���O
 *	�_�C�A���O������
 */
static BOOL FASTCALL AddrDlgInit(HWND hDlg)
{
	HWND hWnd;
	RECT prect;
	RECT drect;
	int i;
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

	/* �R���{�{�b�N�X���� */
	hWnd = GetDlgItem(hDlg, IDC_ADDRCOMBO);
	ASSERT(hWnd);

	/* �q�X�g����}�� */
	ComboBox_ResetContent(hWnd);
	for (i=AddrNum; i>0; i--) {
		sprintf(string, "%04X", AddrHistory[i - 1]);
		ComboBox_AddString(hWnd, string);
	}

	/* �A�h���X��ݒ� */
	sprintf(string, "%04X", AddrBuf);
	ComboBox_SetText(hWnd, string);

	return TRUE;
}

/*
 *	�A�h���X���̓_�C�A���O
 *	�_�C�A���OOK
 */
static void FASTCALL AddrDlgOK(HWND hDlg)
{
	HWND hWnd;
	char string[128];
	int i;

	ASSERT(hDlg);

	/* �R���{�{�b�N�X���� */
	hWnd = GetDlgItem(hDlg, IDC_ADDRCOMBO);
	ASSERT(hWnd);

	/* ���݂̒l���擾 */
	ComboBox_GetText(hWnd, string, sizeof(string) - 1);
	AddrBuf = (WORD)strtol(string, NULL, 16);

	/* �q�X�g�����V�t�g�A�}���A�J�E���g�A�b�v */
	for (i=14; i>=0; i--) {
		AddrHistory[i + 1] = AddrHistory[i];
	}
	AddrHistory[0] = AddrBuf;
	if (AddrNum < 16) {
		AddrNum++;
	}
}

/*
 *	�A�h���X���̓_�C�A���O
 *	�_�C�A���O�v���V�[�W��
 */
static BOOL CALLBACK AddrDlgProc(HWND hDlg, UINT iMsg,
									WPARAM wParam, LPARAM lParam)
{
	switch (iMsg) {
		/* �_�C�A���O������ */
		case WM_INITDIALOG:
			return AddrDlgInit(hDlg);

		/* �R�}���h���� */
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				/* OK */
				case IDOK:
					AddrDlgOK(hDlg);
					EndDialog(hDlg, IDOK);
					return TRUE;

				/* �L�����Z�� */
				case IDCANCEL:
					EndDialog(hDlg, IDCANCEL);
					return TRUE;
			}
			break;
	}

	/* ����ȊO�́AFALSE */
	return FALSE;
}

/*
 *	�A�h���X����
 */
static BOOL FASTCALL AddrDlg(HWND hWnd, WORD *pAddr)
{
	int ret;

	ASSERT(hWnd);
	ASSERT(pAddr);

	/* �A�h���X���Z�b�g */
	AddrBuf = *pAddr;

	/* ���[�_���_�C�A���O���s */
	ret = DialogBox(hAppInstance, MAKEINTRESOURCE(IDD_ADDRDLG), hWnd, AddrDlgProc);
	if (ret != IDOK) {
		return FALSE;
	}

	/* �A�h���X���Z�b�g���A�A�� */
	*pAddr = AddrBuf;
	return TRUE;
}

/*-[ �u���[�N�|�C���g ]------------------------------------------------------*/

/*
 *	�u���[�N�|�C���g�E�C���h�E
 *	�Z�b�g�A�b�v
 */
static void FASTCALL SetupBreakPoint(BYTE *p, int x, int y)
{
	int i;
	char string[128];

	ASSERT(p);
	ASSERT(x > 0);
	ASSERT(y > 0);

	/* ��U�X�y�[�X�Ŗ��߂� */
	memset(p, 0x20, x * y);

	/* �u���[�N�|�C���g���[�v */
	for (i=0; i<BREAKP_MAXNUM; i++) {
		/* ������쐬 */
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

		/* �R�s�[ */
		memcpy(&p[x * i], string, strlen(string));
	}
}

/*
 *	�u���[�N�|�C���g�E�C���h�E
 *	�`��
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

	/* �E�C���h�E�W�I���g���𓾂� */
	GetClientRect(hWnd, &rect);
	x = rect.right / lCharWidth;
	y = rect.bottom / lCharHeight;
	if ((x == 0) || (y == 0)) {
		return;
	}

	/* �Z�b�g�A�b�v */
	p = pBreakPoint;
	if (!p) {
		return;
	}
	SetupBreakPoint(p, x, y);

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
 *	�u���[�N�|�C���g�E�C���h�E
 *	���t���b�V��
 */
void FASTCALL RefreshBreakPoint(void)
{
	HWND hWnd;
	HDC hDC;

	/* ��ɌĂ΂��̂ŁA���݃`�F�b�N���邱�� */
	if (hSubWnd[SWND_BREAKPOINT] == NULL) {
		return;
	}

	/* �`�� */
	hWnd = hSubWnd[SWND_BREAKPOINT];
	hDC = GetDC(hWnd);
	DrawBreakPoint(hWnd, hDC);
	ReleaseDC(hWnd, hDC);
}

/*
 *	�u���[�N�|�C���g�E�C���h�E
 *	�ĕ`��
 */
static void FASTCALL PaintBreakPoint(HWND hWnd)
{
	HDC hDC;
	PAINTSTRUCT ps;
	RECT rect;
	BYTE *p;
	int x, y;

	ASSERT(hWnd);

	/* �|�C���^��ݒ� */
	p = pBreakPoint;

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
	DrawBreakPoint(hWnd, hDC);
	EndPaint(hWnd, &ps);
}

/*
 *	�u���[�N�|�C���g
 *	�R�}���h
 */
static void FASTCALL CmdBreakPoint(HWND hWnd, WORD wID)
{
	int num;
	POINT point;

	ASSERT(hWnd);

	/* �C���f�b�N�X�ԍ��擾 */
	point = PosBreakPoint;
	num = point.y / lCharHeight;
	if ((num < 0) || (num >= BREAKP_MAXNUM)) {
		return;
	}

	/* �R�}���h�� */
	switch (wID) {
		/* �W�����v */
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

		/* �C�l�[�u�� */
		case IDM_BREAKP_ENABLE:
			if (breakp[num].flag == BREAKP_DISABLED) {
				breakp[num].flag = BREAKP_ENABLED;
				InvalidateRect(hWnd, NULL, FALSE);
				RefreshDisAsm();
			}
			break;

		/* �f�B�Z�[�u�� */
		case IDM_BREAKP_DISABLE:
			if ((breakp[num].flag == BREAKP_ENABLED) || (breakp[num].flag == BREAKP_STOPPED)) {
				breakp[num].flag = BREAKP_DISABLED;
				InvalidateRect(hWnd, NULL, FALSE);
				RefreshDisAsm();
			}
			break;

		/* �N���A */
		case IDM_BREAKP_CLEAR:
			breakp[num].flag = BREAKP_NOTUSE;
			InvalidateRect(hWnd, NULL, FALSE);
			RefreshDisAsm();
			break;

		/* �S�ăN���A */
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
 *	�u���[�N�|�C���g�E�C���h�E
 *	�E�C���h�E�v���V�[�W��
 */
static LRESULT CALLBACK BreakPointProc(HWND hWnd, UINT message,
								 WPARAM wParam, LPARAM lParam)
{
	POINT point;
	int i;
	HMENU hMenu;

	/* ���b�Z�[�W�� */
	switch (message) {
		/* �E�C���h�E�ĕ`�� */
		case WM_PAINT:
			/* ���b�N���K�v */
			LockVM();
			PaintBreakPoint(hWnd);
			UnlockVM();
			return 0;

		/* ���N���b�N */
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
			/* �J�[�\���ʒu����A���� */
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

		/* �R���e�L�X�g���j���[ */
		case WM_RBUTTONDOWN:
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);
			PosBreakPoint = point;
			hMenu = GetSubMenu(hBreakPoint, 0);
			ClientToScreen(hWnd, &point);
			TrackPopupMenu(hMenu, 0, point.x, point.y, 0, hWnd, NULL);
			return 0;

		/* �E�C���h�E�폜 */
		case WM_DESTROY:
			/* ���j���[�폜 */
			DestroyMenu(hBreakPoint);

			/* ���C���E�C���h�E�֎����ʒm */
			for (i=0; i<SWND_MAXNUM; i++) {
				if (hSubWnd[i] == hWnd) {
					hSubWnd[i] = NULL;
					/* Draw�o�b�t�@�폜 */
					free(pBreakPoint);
					pBreakPoint = NULL;
				}
			}
			break;

		/* �R�}���h */
		case WM_COMMAND:
			CmdBreakPoint(hWnd, LOWORD(wParam));
			break;
	}

	/* �f�t�H���g �E�C���h�E�v���V�[�W�� */
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/*
 *	�u���[�N�|�C���g�E�C���h�E
 *	�E�C���h�E�쐬
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

	/* �E�C���h�E��`���v�Z */
	rect.left = lCharWidth * index;
	rect.top = lCharHeight * index;
	if (rect.top >= 380) {
		rect.top -= 380;
	}
	rect.right = lCharWidth * 20;
	rect.bottom = lCharHeight * BREAKP_MAXNUM;

	/* �E�C���h�E�^�C�g��������A�o�b�t�@�m�� */
	LoadString(hAppInstance, IDS_SWND_BREAKPOINT,
				szWndName, sizeof(szWndName));
	pBreakPoint = malloc(2 * (rect.right / lCharWidth) *
								(rect.bottom / lCharHeight));

	/* ���j���[�����[�h */
	hBreakPoint = LoadMenu(hAppInstance, MAKEINTRESOURCE(IDR_BREAKPOINTMENU));

	/* �E�C���h�E�N���X�̓o�^ */
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

/*-[ �X�P�W���[���E�C���h�E ]------------------------------------------------*/

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
 *	�X�P�W���[���E�C���h�E
 *	�Z�b�g�A�b�v
 */
static void FASTCALL SetupScheduler(BYTE *p, int x, int y)
{
	int i;
	char string[128];

	ASSERT(p);
	ASSERT(x > 0);
	ASSERT(y > 0);

	/* ��U�X�y�[�X�Ŗ��߂� */
	memset(p, 0x20, x * y);

	/* ���[�v */
	for (i=0; i<EVENT_MAXNUM; i++) {
		/* �^�C�g�� */
		memcpy(&p[x * i], pszSchedulerTitle[i], strlen(pszSchedulerTitle[i]));

		/* �J�����g�A�����[�h */
		if (event[i].flag != EVENT_NOTUSE) {
			sprintf(string, "%4d.%03dms",   event[i].current / 1000,
											event[i].current % 1000);
			memcpy(&p[x * i + 12], string, strlen(string));

			sprintf(string, "(%4d.%03dms)", event[i].reload / 1000,
											event[i].reload % 1000);
			memcpy(&p[x * i + 23], string, strlen(string));
		}

		/* �X�e�[�^�X */
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
 *	�X�P�W���[���E�C���h�E
 *	�`��
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

	/* �E�C���h�E�W�I���g���𓾂� */
	GetClientRect(hWnd, &rect);
	x = rect.right / lCharWidth;
	y = rect.bottom / lCharHeight;
	if ((x == 0) || (y == 0)) {
		return;
	}

	/* �Z�b�g�A�b�v */
	p = pScheduler;
	if (!p) {
		return;
	}
	SetupScheduler(p, x, y);

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
 *	�X�P�W���[���E�C���h�E
 *	���t���b�V��
 */
void FASTCALL RefreshScheduler(void)
{
	HWND hWnd;
	HDC hDC;

	/* ��ɌĂ΂��̂ŁA���݃`�F�b�N���邱�� */
	if (hSubWnd[SWND_SCHEDULER] == NULL) {
		return;
	}

	/* �`�� */
	hWnd = hSubWnd[SWND_SCHEDULER];
	hDC = GetDC(hWnd);
	DrawScheduler(hWnd, hDC);
	ReleaseDC(hWnd, hDC);
}

/*
 *	�X�P�W���[���E�C���h�E
 *	�ĕ`��
 */
static void FASTCALL PaintScheduler(HWND hWnd)
{
	HDC hDC;
	PAINTSTRUCT ps;
	RECT rect;
	BYTE *p;
	int x, y;

	ASSERT(hWnd);

	/* �|�C���^��ݒ� */
	p = pScheduler;

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
	DrawScheduler(hWnd, hDC);
	EndPaint(hWnd, &ps);
}

/*
 *	�X�P�W���[���E�C���h�E
 *	�E�C���h�E�v���V�[�W��
 */
static LRESULT CALLBACK SchedulerProc(HWND hWnd, UINT message,
								 WPARAM wParam, LPARAM lParam)
{
	int i;

	/* ���b�Z�[�W�� */
	switch (message) {
		/* �E�C���h�E�ĕ`�� */
		case WM_PAINT:
			/* ���b�N���K�v */
			LockVM();
			PaintScheduler(hWnd);
			UnlockVM();
			return 0;

		/* �E�C���h�E�폜 */
		case WM_DESTROY:
			/* ���C���E�C���h�E�֎����ʒm */
			for (i=0; i<SWND_MAXNUM; i++) {
				if (hSubWnd[i] == hWnd) {
					hSubWnd[i] = NULL;
					/* Draw�o�b�t�@�폜 */
					free(pScheduler);
					pScheduler = NULL;
				}
			}
			break;
	}

	/* �f�t�H���g �E�C���h�E�v���V�[�W�� */
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/*
 *	�X�P�W���[���E�C���h�E
 *	�E�C���h�E�쐬
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

	/* �E�C���h�E��`���v�Z */
	rect.left = lCharWidth * index;
	rect.top = lCharHeight * index;
	if (rect.top >= 380) {
		rect.top -= 380;
	}
	rect.right = lCharWidth * 44;
	rect.bottom = lCharHeight * EVENT_MAXNUM;

	/* �E�C���h�E�^�C�g��������A�o�b�t�@�m�� */
	LoadString(hAppInstance, IDS_SWND_SCHEDULER,
				szWndName, sizeof(szWndName));
	pScheduler = malloc(2 * (rect.right / lCharWidth) *
								(rect.bottom / lCharHeight));

	/* �E�C���h�E�N���X�̓o�^ */
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

/*-[ CPU���W�X�^�E�C���h�E ]-------------------------------------------------*/

/*
 *	CPU���W�X�^�E�C���h�E
 *	�Z�b�g�A�b�v
 */
static void FASTCALL SetupCPURegister(BOOL bMain, BYTE *p, int x, int y)
{
	char buf[128];
	cpu6809_t *pReg;

	ASSERT((bMain == TRUE) || (bMain == FALSE));
	ASSERT(p);
	ASSERT(x > 0);
	ASSERT(y > 0);

	/* ��U�X�y�[�X�Ŗ��߂� */
	memset(p, 0x20, x * y);

	/* ���W�X�^�o�b�t�@�𓾂� */
	if (bMain) {
		pReg = &maincpu;
	}
	else {
		pReg = &subcpu;
	}

	/* �Z�b�g */
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
 *	CPU���W�X�^�E�C���h�E
 *	�`��
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

	/* Draw�o�b�t�@�𓾂� */
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

	/* �E�C���h�E�W�I���g���𓾂� */
	GetClientRect(hWnd, &rect);
	x = rect.right / lCharWidth;
	y = rect.bottom / lCharHeight;
	if ((x == 0) || (y == 0)) {
		return;
	}

	/* �Z�b�g�A�b�v */
	SetupCPURegister(bMain, p, x, y);

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
 *	CPU���W�X�^�E�C���h�E
 *	���t���b�V��
 */
void FASTCALL RefreshCPURegister(void)
{
	HWND hWnd;
	HDC hDC;

	/* ���C��CPU */
	if (hSubWnd[SWND_CPUREG_MAIN]) {
		hWnd = hSubWnd[SWND_CPUREG_MAIN];
		hDC = GetDC(hWnd);
		DrawCPURegister(hWnd, hDC);
		ReleaseDC(hWnd, hDC);
	}

	/* �T�uCPU */
	if (hSubWnd[SWND_CPUREG_SUB]) {
		hWnd = hSubWnd[SWND_CPUREG_SUB];
		hDC = GetDC(hWnd);
		DrawCPURegister(hWnd, hDC);
		ReleaseDC(hWnd, hDC);
	}
}

/*
 *	CPU���W�X�^�E�C���h�E
 *	�ĕ`��
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

	/* Draw�o�b�t�@�𓾂� */
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
	DrawCPURegister(hWnd, hDC);
	EndPaint(hWnd, &ps);
}

/*
 *	CPU���W�X�^�E�C���h�E
 *	�E�C���h�E�v���V�[�W��
 */
static LRESULT CALLBACK CPURegisterProc(HWND hWnd, UINT message,
								 WPARAM wParam, LPARAM lParam)
{
	int i;

	/* ���b�Z�[�W�� */
	switch (message) {
		/* �E�C���h�E�ĕ`�� */
		case WM_PAINT:
			/* ���b�N���K�v */
			LockVM();
			PaintCPURegister(hWnd);
			UnlockVM();
			return 0;

		/* �E�C���h�E�폜 */
		case WM_DESTROY:
			/* ���C���E�C���h�E�֎����ʒm */
			for (i=0; i<SWND_MAXNUM; i++) {
				if (hSubWnd[i] == hWnd) {
					hSubWnd[i] = NULL;
					/* Draw�o�b�t�@�폜 */
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

	/* �f�t�H���g �E�C���h�E�v���V�[�W�� */
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/*
 *	CPU���W�X�^�E�C���h�E
 *	�E�C���h�E�쐬
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

	/* �E�C���h�E��`���v�Z */
	rect.left = lCharWidth * index;
	rect.top = lCharHeight * index;
	if (rect.top >= 380) {
		rect.top -= 380;
	}
	rect.right = lCharWidth * 17;
	rect.bottom = lCharHeight * 5;

	/* �E�C���h�E�^�C�g��������A�o�b�t�@�m�� */
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

	/* �E�C���h�E�N���X�̓o�^ */
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

/*-[ �t�A�Z���u���E�C���h�E ]------------------------------------------------*/

/*
 *	�t�A�Z���u���E�C���h�E
 *	�A�h���X�ݒ�
 */
void FASTCALL AddrDisAsm(BOOL bMain, WORD wAddr)
{
	SCROLLINFO sif;

	memset(&sif, 0, sizeof(sif));
	sif.cbSize = sizeof(sif);
	sif.fMask = SIF_POS;
	sif.nPos = (int)wAddr;

	/* ���݃`�F�b�N���ݒ� */
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

	/* ���t���b�V�� */
	RefreshDisAsm();
}

/*
 *	�t�A�Z���u���E�C���h�E
 *	�Z�b�g�A�b�v
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

	/* ��U�X�y�[�X�Ŗ��߂� */
	memset(p, 0x20, x * y);

	/* �����ݒ� */
	if (bMain) {
		cpu = MAINCPU;
		addr = (int)wDisAsmMain;
	}
	else {
		cpu = SUBCPU;
		addr = (int)wDisAsmSub;
	}

	for (i=0; i<y; i++) {
		 /* �t�A�Z���u�� */
		ret = disline(cpu, (WORD)addr, string);

		/* �Z�b�g */
		memcpy(&p[x * i + 2], string, strlen(string));

		/* �}�[�N */
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

			/* �u���[�N�|�C���g */
			p[x * i + 0] = (BYTE)('1' + j);

			/* ���] */
			for (k=0; k<x; k++) {
				 p[x * i + k] = (BYTE)(p[x * i + k] | 0x80);
			}
		}

		/* ���Z�A�I�[�o�[�`�F�b�N */
		addr += ret;
		if (addr >= 0x10000) {
			break;
		}
	}
}

/*
 *	�t�A�Z���u���E�C���h�E
 *	�`��
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

	/* Draw�o�b�t�@�𓾂� */
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

	/* �E�C���h�E�W�I���g���𓾂� */
	GetClientRect(hWnd, &rect);
	x = rect.right / lCharWidth;
	y = rect.bottom / lCharHeight;
	if ((x == 0) || (y == 0)) {
		return;
	}

	/* �Z�b�g�A�b�v */
	SetupDisAsm(bMain, p, x, y);

	/* �t�H���g�Z���N�g */
	hFont = CreateTextFont();
	hBackup = SelectObject(hDC, hFont);

	/* ��r�`�� */
	q = &p[x * y];
	for (yy=0; yy<y; yy++) {
		for (xx=0; xx<x; xx++) {
			if (*p != *q) {
				if (*p >= 0x80) {
					/* ���]�\�� */
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
					/* �ʏ�\�� */
					TextOut(hDC, xx * lCharWidth, yy * lCharHeight,
						(LPCTSTR)p, 1);
				}
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
 *	�t�A�Z���u���E�C���h�E
 *	���t���b�V��
 */
void FASTCALL RefreshDisAsm(void)
{
	HWND hWnd;
	HDC hDC;

	/* ���C��CPU */
	if (hSubWnd[SWND_DISASM_MAIN]) {
		hWnd = hSubWnd[SWND_DISASM_MAIN];
		hDC = GetDC(hWnd);
		DrawDisAsm(hWnd, hDC);
		ReleaseDC(hWnd, hDC);
	}

	/* �T�uCPU */
	if (hSubWnd[SWND_DISASM_SUB]) {
		hWnd = hSubWnd[SWND_DISASM_SUB];
		hDC = GetDC(hWnd);
		DrawDisAsm(hWnd, hDC);
		ReleaseDC(hWnd, hDC);
	}
}

/*
 *	�t�A�Z���u���E�C���h�E
 *	�ĕ`��
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

	/* Draw�o�b�t�@�𓾂� */
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
	DrawDisAsm(hWnd, hDC);
	EndPaint(hWnd, &ps);
}

/*
 *	�t�A�Z���u���E�C���h�E
 *	���N���b�N
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

	/* �s�J�E���g�𓾂� */
	y = point.y / lCharHeight;

	/* ���ۂɋt�A�Z���u�����Ă݂� */
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

	/* �t�A�Z���u�� ���[�v */
	for (i=0; i<y; i++) {
		ret = disline(cpu, (WORD)addr, string);
		addr += ret;
		if (addr >= 0x10000) {
			return;
		}
	}

	/* �u���[�N�|�C���g on/off */
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

	/* �ĕ`�� */
	InvalidateRect(hWnd, NULL, FALSE);

	/* �u���[�N�|�C���g�E�C���h�E���A�ĕ`�悷�� */
	if (hSubWnd[SWND_BREAKPOINT]) {
		InvalidateRect(hSubWnd[SWND_BREAKPOINT], NULL, FALSE);
	}
}

/*
 *	�t�A�Z���u���E�C���h�E
 *	�R�}���h����
 */
static FASTCALL void CmdDisAsm(HWND hWnd, WORD wID, BOOL bMain)
{
	WORD target;
	cpu6809_t *cpu;

	/* CPU�\���̌��� */
	if (bMain) {
		cpu = &maincpu;
	}
	else {
		cpu = &subcpu;
	}

	/* �^�[�Q�b�g�A�h���X���� */
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

	/* �ݒ聕�X�V */
	AddrDisAsm(bMain, target);
}

/*
 *	�t�A�Z���u���E�C���h�E
 *	�E�C���h�E�v���V�[�W��
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

	/* ���b�Z�[�W�� */
	switch (message) {
		/* �E�C���h�E�ĕ`�� */
		case WM_PAINT:
			/* ���b�N���K�v */
			LockVM();
			PaintDisAsm(hWnd);
			UnlockVM();
			return 0;

		/* �E�C���h�E�폜 */
		case WM_DESTROY:
			/* ���C���E�C���h�E�֎����ʒm */
			for (i=0; i<SWND_MAXNUM; i++) {
				if (hSubWnd[i] == hWnd) {
					hSubWnd[i] = NULL;
					/* Draw�o�b�t�@�폜 */
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

		/* �R���e�L�X�g���j���[ */
		case WM_CONTEXTMENU:
			/* �R���e�L�X�g���j���[�����[�h */
			if (hSubWnd[SWND_DISASM_MAIN] == hWnd) {
				hMenu = GetSubMenu(hDisAsmMain, 0);
			}
			else {
				hMenu = GetSubMenu(hDisAsmSub, 0);
			}

			/* �R���e�L�X�g���j���[�����s */
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);
			TrackPopupMenu(hMenu, 0, point.x, point.y, 0, hWnd, NULL);
			return 0;

		/* ���N���b�N�E�_�u���N���b�N */
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);
			LButtonDisAsm(hWnd, point);
			break;

		/* �R�}���h */
		case WM_COMMAND:
			if (hSubWnd[SWND_DISASM_MAIN] == hWnd) {
				CmdDisAsm(hWnd, LOWORD(wParam), TRUE);
			}
			else {
				CmdDisAsm(hWnd, LOWORD(wParam), FALSE);
			}
			break;

		/* �����X�N���[���o�[ */
		case WM_VSCROLL:
			/* �^�C�v���� */
			if (hSubWnd[SWND_DISASM_MAIN] == hWnd) {
				bMain = TRUE;
				wAddr = wDisAsmMain;
			}
			else {
				bMain = FALSE;
				wAddr = wDisAsmSub;
			}

			/* �A�N�V������ */
			switch (LOWORD(wParam)) {
				/* �g�b�v */
				case SB_TOP:
					wAddr = 0;
					break;
				/* �I�[ */
				case SB_BOTTOM:
					wAddr = 0xffff;
					break;
				/* �P�s�� */
				case SB_LINEUP:
					if (wAddr > 0) {
						wAddr--;
					}
					break;
				/* �P�s��(�����͍H�v) */
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
				/* �y�[�W�A�b�v */
				case SB_PAGEUP:
					if (wAddr < 0x80) {
						wAddr = 0;
					}
					else {
						wAddr -= (WORD)0x80;
					}
					break;
				/* �y�[�W�_�E�� */
				case SB_PAGEDOWN:
					if (wAddr >= 0xff80) {
						wAddr = 0xffff;
					}
					else {
						wAddr += (WORD)0x80;
					}
					break;
				/* ���ڎw�� */
				case SB_THUMBTRACK:
					wAddr = HIWORD(wParam);
					break;
			}
			AddrDisAsm(bMain, wAddr);
			RefreshDisAsm();
			break;
	}

	/* �f�t�H���g �E�C���h�E�v���V�[�W�� */
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/*
 *	�t�A�Z���u���E�C���h�E
 *	�E�C���h�E�쐬
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

	/* �E�C���h�E��`���v�Z */
	rect.left = lCharWidth * index;
	rect.top = lCharHeight * index;
	if (rect.top >= 380) {
		rect.top -= 380;
	}
	rect.right = lCharWidth * 47;
	rect.bottom = lCharHeight * 8;

	/* �E�C���h�E�^�C�g��������A�o�b�t�@�m�ہA���j�����[�h */
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

	/* �E�C���h�E�N���X�̓o�^ */
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

	/* �E�C���h�E�쐬 */
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

	/* �L���Ȃ�A�T�C�Y�␳���Ď�O�ɒu�� */
	if (hWnd) {
		GetWindowRect(hWnd, &wrect);
		GetClientRect(hWnd, &crect);
		wrect.right += (rect.right - crect.right);
		wrect.bottom += (rect.bottom - crect.bottom);
		SetWindowPos(hWnd, HWND_TOP, wrect.left, wrect.top,
			wrect.right - wrect.left, wrect.bottom - wrect.top, SWP_NOMOVE);

		/* �X�N���[���o�[�̐ݒ肪�K�v */
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

	/* ���ʂ������A�� */
	return hWnd;
}

/*-[ �������_���v�E�C���h�E ]------------------------------------------------*/

/*
 *	�������_���v�E�C���h�E
 *	�A�h���X�ݒ�
 */
void FASTCALL AddrMemory(BOOL bMain, WORD wAddr)
{
	SCROLLINFO sif;

	wAddr &= (WORD)(0xfff0);

	memset(&sif, 0, sizeof(sif));
	sif.cbSize = sizeof(sif);
	sif.fMask = SIF_POS;
	sif.nPos = (int)(wAddr >> 4);

	/* ���݃`�F�b�N���ݒ� */
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

	/* ���t���b�V�� */
	RefreshMemory();
}

/*
 *	�������_���v�E�C���h�E
 *	�Z�b�g�A�b�v
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

	/* ��U�X�y�[�X�Ŗ��߂� */
	memset(p, 0x20, x * y);

	/* �A�h���X�擾 */
	if (bMain) {
		addr = (WORD)(wMemoryMain & 0xfff0);
	}
	else {
		addr = (WORD)(wMemorySub & 0xfff0);
	}

	/* ���[�v */
	for (i=0; i<8; i++) {
		/* �쐬 */
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

		/* �R�s�[ */
		memcpy(&p[x * i], string, strlen(string));

		/* ���� */
		addr += (WORD)0x0010;
		if (addr == 0) {
			break;
		}
	}
}

/*
 *	�������_���v�E�C���h�E
 *	�`��
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

	/* Draw�o�b�t�@�𓾂� */
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

	/* �E�C���h�E�W�I���g���𓾂� */
	GetClientRect(hWnd, &rect);
	x = rect.right / lCharWidth;
	y = rect.bottom / lCharHeight;
	if ((x == 0) || (y == 0)) {
		return;
	}

	/* �Z�b�g�A�b�v */
	SetupMemory(bMain, p, x, y);

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
 *	�������_���v�E�C���h�E
 *	���t���b�V��
 */
void FASTCALL RefreshMemory(void)
{
	HWND hWnd;
	HDC hDC;

	/* ���C��CPU */
	if (hSubWnd[SWND_MEMORY_MAIN]) {
		hWnd = hSubWnd[SWND_MEMORY_MAIN];
		hDC = GetDC(hWnd);
		DrawMemory(hWnd, hDC);
		ReleaseDC(hWnd, hDC);
	}

	/* �T�uCPU */
	if (hSubWnd[SWND_MEMORY_SUB]) {
		hWnd = hSubWnd[SWND_MEMORY_SUB];
		hDC = GetDC(hWnd);
		DrawMemory(hWnd, hDC);
		ReleaseDC(hWnd, hDC);
	}
}

/*
 *	�������_���v�E�C���h�E
 *	�ĕ`��
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

	/* Draw�o�b�t�@�𓾂� */
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
	DrawMemory(hWnd, hDC);
	EndPaint(hWnd, &ps);
}

/*
 *	�������_���v�E�C���h�E
 *	�R�}���h����
 */
static FASTCALL void CmdMemory(HWND hWnd, WORD wID, BOOL bMain)
{
	WORD target;
	cpu6809_t *cpu;

	/* CPU�\���̌��� */
	if (bMain) {
		cpu = &maincpu;
	}
	else {
		cpu = &subcpu;
	}

	/* �^�[�Q�b�g�A�h���X���� */
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

	/* �ݒ聕�X�V */
	AddrMemory(bMain, target);
}

/*
 *	�������_���v�E�C���h�E
 *	�E�C���h�E�v���V�[�W��
 */
static LRESULT CALLBACK MemoryProc(HWND hWnd, UINT message,
								 WPARAM wParam, LPARAM lParam)
{
	HMENU hMenu;
	POINT point;
	BOOL bMain;
	WORD wAddr;
	int i;

	/* ���b�Z�[�W�� */
	switch (message) {
		/* �E�C���h�E�ĕ`�� */
		case WM_PAINT:
			/* ���b�N���K�v */
			LockVM();
			PaintMemory(hWnd);
			UnlockVM();
			return 0;

		/* �E�C���h�E�폜 */
		case WM_DESTROY:
			/* ���C���E�C���h�E�֎����ʒm */
			for (i=0; i<SWND_MAXNUM; i++) {
				if (hSubWnd[i] == hWnd) {
					hSubWnd[i] = NULL;
					/* Draw�o�b�t�@�폜�A���j���[�폜 */
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

		/* �R���e�L�X�g���j���[ */
		case WM_CONTEXTMENU:
			/* �T�u���j���[�����o�� */
			if (hSubWnd[SWND_MEMORY_MAIN] == hWnd) {
				hMenu = GetSubMenu(hMemoryMain, 0);
			}
			else {
				hMenu = GetSubMenu(hMemorySub, 0);
			}

			/* �R���e�L�X�g���j���[�����s */
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);
			TrackPopupMenu(hMenu, 0, point.x, point.y, 0, hWnd, NULL);
			return 0;

		/* �R�}���h */
		case WM_COMMAND:
			if (hSubWnd[SWND_MEMORY_MAIN] == hWnd) {
				CmdMemory(hWnd, LOWORD(wParam), TRUE);
			}
			else {
				CmdMemory(hWnd, LOWORD(wParam), FALSE);
			}
			break;

		/* �����X�N���[���o�[ */
		case WM_VSCROLL:
			/* �^�C�v���� */
			if (hSubWnd[SWND_MEMORY_MAIN] == hWnd) {
				bMain = TRUE;
				wAddr = wMemoryMain;
			}
			else {
				bMain = FALSE;
				wAddr = wMemorySub;
			}

			/* �A�N�V������ */
			switch (LOWORD(wParam)) {
				/* �g�b�v */
				case SB_TOP:
					wAddr = 0;
					break;
				/* �I�[ */
				case SB_BOTTOM:
					wAddr = 0xfff0;
					break;
				/* �P�s�� */
				case SB_LINEUP:
					if (wAddr >= 0x0010) {
						wAddr -= (WORD)0x0010;
					}
					break;
				/* �P�s�� */
				case SB_LINEDOWN:
					if (wAddr < 0xfff0) {
						wAddr += (WORD)0x0010;
					}
					break;
				/* �y�[�W�A�b�v */
				case SB_PAGEUP:
					if (wAddr < 0x100) {
						wAddr = 0;
					}
					else {
						wAddr -= (WORD)0x100;
					}
					break;
				/* �y�[�W�_�E�� */
				case SB_PAGEDOWN:
					if (wAddr >= 0xfef0) {
						wAddr = 0xfff0;
					}
					else {
						wAddr += (WORD)0x100;
					}
					break;
				/* ���ڎw�� */
				case SB_THUMBTRACK:
					wAddr = (WORD)(HIWORD(wParam) * 16);
					break;
			}
			wAddr &= (WORD)0xfff0;
			AddrMemory(bMain, wAddr);
			RefreshMemory();
			break;
	}

	/* �f�t�H���g �E�C���h�E�v���V�[�W�� */
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/*
 *	�������_���v�E�C���h�E
 *	�E�C���h�E�쐬
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

	/* �E�C���h�E��`���v�Z */
	rect.left = lCharWidth * index;
	rect.top = lCharHeight * index;
	if (rect.top >= 380) {
		rect.top -= 380;
	}
	rect.right = lCharWidth * 53;
	rect.bottom = lCharHeight * 8;

	/* �E�C���h�E�^�C�g��������A�o�b�t�@�m�ہA���j���[���[�h */
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

	/* �E�C���h�E�N���X�̓o�^ */
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

	/* �E�C���h�E�쐬 */
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

	/* �L���Ȃ�A�T�C�Y�␳���Ď�O�ɒu�� */
	if (hWnd) {
		GetWindowRect(hWnd, &wrect);
		GetClientRect(hWnd, &crect);
		wrect.right += (rect.right - crect.right);
		wrect.bottom += (rect.bottom - crect.bottom);
		SetWindowPos(hWnd, HWND_TOP, wrect.left, wrect.top,
			wrect.right - wrect.left, wrect.bottom - wrect.top, SWP_NOMOVE);

		/* �X�N���[���o�[�̐ݒ肪�K�v */
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

	/* ���ʂ������A�� */
	return hWnd;
}

#endif	/* _WIN32 */
