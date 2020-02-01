/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API DirectDraw ]
 */

#ifdef _WIN32

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define DIRECTDRAW_VERSION		0x300	/* DirectX3���w�� */
#include <ddraw.h>
#include <assert.h>
#include "xm7.h"
#include "subctrl.h"
#include "display.h"
#include "multipag.h"
#include "ttlpalet.h"
#include "apalet.h"
#include "fdc.h"
#include "tapelp.h"
#include "keyboard.h"
#include "w32.h"
#include "w32_bar.h"
#include "w32_res.h"
#include "w32_draw.h"
#include "w32_dd.h"

/*
 *	�O���[�o�� ���[�N
 */
WORD rgbTTLDD[8];						/* 640x200 �p���b�g */
WORD rgbAnalogDD[4096];					/* 320x200 �p���b�g */
BOOL bDD480Line;						/* 640x480�D��t���O */
BOOL bDD480Status;						/* 640x480�X�e�[�^�X�t���O */

/*
 *	�X�^�e�B�b�N ���[�N
 */
static BOOL bAnalog;					/* �A�i���O���[�h�t���O */
static RECT BkRect;						/* �E�C���h�E���̋�` */
static BOOL bMouseCursor;				/* �}�E�X�J�[�\���t���O */
static LPDIRECTDRAW2 lpdd2;				/* DirectDraw2 */
static LPDIRECTDRAWSURFACE lpdds[2];	/* DirectDrawSurface3 */
static LPDIRECTDRAWCLIPPER lpddc;		/* DirectDrawClipper */
static UINT nPixelFormat;				/* 320x200 �s�N�Z���t�H�[�}�b�g */
static UINT nDrawTop;					/* �`��͈͏� */
static UINT nDrawBottom;				/* �`��͈͉� */
static BOOL bPaletFlag;					/* �p���b�g�ύX�t���O */
static BOOL bDD480Flag;					/* 640x480 �t���O */
static BOOL bClearFlag;					/* �㉺�N���A�t���O */

static char szRunMessage[128];			/* RUN���b�Z�[�W */
static char szStopMessage[128];			/* STOP���b�Z�[�W */
static char szCaption[128];				/* �L���v�V���� */
static int nCAP;						/* CAP�L�[ */
static int nKANA;						/* ���ȃL�[ */
static int nINS;						/* INS�L�[ */
static int nDrive[2];					/* �t���b�s�[�h���C�u */
static char szDrive[2][16 + 1];			/* �t���b�s�[�h���C�u */
static int nTape;						/* �e�[�v */

/*
 *	�A�Z���u���֐��̂��߂̃v���g�^�C�v�錾
 */
#ifdef __cplusplus
extern "C" {
#endif
void Render640DD(LPVOID lpSurface, LONG lPitch, int first, int last);
void Render320DD(LPVOID lpSurface, LONG lPitch, int first, int last);
#ifdef __cplusplus
}
#endif

/*
 *	������
 */
void FASTCALL InitDD(void)
{
	/* ���[�N�G���A������(�ݒ胏�[�N�͕ύX���Ȃ�) */
	bAnalog = FALSE;
	bMouseCursor = TRUE;
	lpdd2 = NULL;
	memset(lpdds, 0, sizeof(lpdds));
	lpddc = NULL;
	bClearFlag = TRUE;

	/* �X�e�[�^�X���C�� */
	szCaption[0] = '\0';
	nCAP = -1;
	nKANA = -1;
	nINS = -1;
	nDrive[0] = -1;
	nDrive[1] = -1;
	szDrive[0][0] = '\0';
	szDrive[1][0] = '\0';
	nTape = -1;

	/* ���b�Z�[�W�����[�h */
	if (LoadString(hAppInstance, IDS_RUNCAPTION,
					szRunMessage, sizeof(szRunMessage)) == 0) {
		szRunMessage[0] = '\0';
	}
	if (LoadString(hAppInstance, IDS_STOPCAPTION,
					szStopMessage, sizeof(szStopMessage)) == 0) {
		szStopMessage[0] = '\0';
	}
}

/*
 *	�N���[���A�b�v
 */
void FASTCALL CleanDD(void)
{
	DWORD dwStyle;
	RECT brect;
	int i;

	/* DirectDrawClipper */
	if (lpddc) {
		lpddc->Release();
		lpddc = NULL;
	}

	/* DirectDrawSurface3 */
	for (i=0; i<2; i++) {
		if (lpdds[i]) {
			lpdds[i]->Release();
			lpdds[i] = NULL;
		}
	}

	/* DirectDraw2 */
	if (lpdd2) {
		lpdd2->Release();
		lpdd2 = NULL;
	}

	/* �E�C���h�E�X�^�C����߂� */
	dwStyle = GetWindowLong(hMainWnd, GWL_STYLE);
	dwStyle &= ~WS_POPUP;
	dwStyle |= (WS_CAPTION | WS_BORDER | WS_SYSMENU);
	SetWindowLong(hMainWnd, GWL_STYLE, dwStyle);
	dwStyle = GetWindowLong(hMainWnd, GWL_EXSTYLE);
	dwStyle |= WS_EX_WINDOWEDGE;
	SetWindowLong(hMainWnd, GWL_EXSTYLE, dwStyle);

	/* �E�C���h�E�ʒu��߂� */
	SetWindowPos(hMainWnd, HWND_NOTOPMOST, BkRect.left, BkRect.top,
		(BkRect.right - BkRect.left), (BkRect.bottom - BkRect.top),
		SWP_DRAWFRAME);

	MoveWindow(hDrawWnd, 0, 0, 640, 400, TRUE);
	if (hStatusBar) {
		GetWindowRect(hStatusBar, &brect);
		MoveWindow(hStatusBar, 0, 400,
					(brect.right - brect.left),
					(brect.bottom - brect.top),
					TRUE);
	}
}

/*
 *	�Z���N�g
 */
BOOL FASTCALL SelectDD(void)
{
	DWORD dwStyle;
	LPDIRECTDRAW lpdd;
	DDSURFACEDESC ddsd;
	DDPIXELFORMAT ddpf;
	RECT brect;

	/* assert */
	ASSERT(hMainWnd);

	/* �E�C���h�E��`���L������ */
	GetWindowRect(hMainWnd, &BkRect);

	/* �E�C���h�E�X�^�C����ύX */
	dwStyle = GetWindowLong(hMainWnd, GWL_STYLE);
	dwStyle &= ~(WS_CAPTION | WS_BORDER | WS_SYSMENU);
	dwStyle |= WS_POPUP;
	SetWindowLong(hMainWnd, GWL_STYLE, dwStyle);
	dwStyle = GetWindowLong(hMainWnd, GWL_EXSTYLE);
	dwStyle &= ~WS_EX_WINDOWEDGE;
	SetWindowLong(hMainWnd, GWL_EXSTYLE, dwStyle);

	/* DirectDraw�I�u�W�F�N�g���쐬 */
	if (FAILED(DirectDrawCreate(NULL, &lpdd, NULL))) {
		return FALSE;
	}

	/* �������[�h��ݒ� */
	if (FAILED(lpdd->SetCooperativeLevel(hMainWnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN))) {
		lpdd->Release();
		return FALSE;
	}

	/* DirectDraw2�C���^�t�F�[�X���擾 */
	if (FAILED(lpdd->QueryInterface(IID_IDirectDraw2, (LPVOID*)&lpdd2))) {
		lpdd->Release();
		return FALSE;
	}

	/* �����܂ŗ���΁ADirectDraw�͂��͂�K�v�Ȃ� */
	lpdd->Release();

	/* ��ʃ��[�h��ݒ� */
	if (bDD480Line) {
		bDD480Flag = TRUE;
		if (FAILED(lpdd2->SetDisplayMode(640, 480, 16, 0, 0))) {
			bDD480Flag = FALSE;
			if (FAILED(lpdd2->SetDisplayMode(640, 400, 16, 0, 0))) {
				return FALSE;
			}
		}
	}
	else {
		bDD480Flag = FALSE;
		if (FAILED(lpdd2->SetDisplayMode(640, 400, 16, 0, 0))) {
			bDD480Flag = TRUE;
			if (FAILED(lpdd2->SetDisplayMode(640, 480, 16, 0, 0))) {
				return FALSE;
			}
		}
	}

	/* �v���C�}���T�[�t�F�C�X���쐬 */
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	if (FAILED(lpdd2->CreateSurface(&ddsd, &lpdds[0], NULL))) {
		return FALSE;
	}

	/* ���[�N�T�[�t�F�C�X���쐬(DDSCAPS_SYSTEMMEMORY���w�肷��) */
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
	ddsd.dwWidth = 640;
	if (bDD480Flag) {
		ddsd.dwHeight = 480;
	}
	else {
		ddsd.dwHeight = 400;
	}
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	if (FAILED(lpdd2->CreateSurface(&ddsd, &lpdds[1], NULL))) {
		return FALSE;
	}

	/* �s�N�Z���t�H�[�}�b�g�𓾂� */
	memset(&ddpf, 0, sizeof(ddpf));
	ddpf.dwSize = sizeof(ddpf);
	if (FAILED(lpdds[1]->GetPixelFormat(&ddpf))) {
		return FALSE;
	}

	/* �s�N�Z���t�H�[�}�b�g���`�F�b�N�BHEL�ŋK�肳��Ă���Q�^�C�v�̂ݑΉ� */
	if (!(ddpf.dwFlags & DDPF_RGB)) {
		return FALSE;
	}
	nPixelFormat = 0;
	if ((ddpf.dwRBitMask == 0xf800) &&
		(ddpf.dwGBitMask == 0x07e0) &&
		(ddpf.dwBBitMask == 0x001f)) {
		nPixelFormat = 1;
	}
	if ((ddpf.dwRBitMask == 0x7c00) &&
		(ddpf.dwGBitMask == 0x03e0) &&
		(ddpf.dwBBitMask == 0x001f)) {
		nPixelFormat = 2;
	}
	if (nPixelFormat == 0) {
		return FALSE;
	}

	/* �N���b�p�[���쐬�A���蓖�� */
	if (FAILED(lpdd2->CreateClipper(NULL, &lpddc, NULL))) {
		return FALSE;
	}
	if (FAILED(lpddc->SetHWnd(NULL, hDrawWnd))) {
		return FALSE;
	}

	/* �E�C���h�E�T�C�Y��ύX(640x480��) */
	if (bDD480Flag) {
		if (hStatusBar) {
			GetWindowRect(hStatusBar, &brect);
		}
		else {
			brect.top = 0;
			brect.bottom = 0;
		}
		MoveWindow(hMainWnd, 0, 0, 640, 480 + (brect.bottom - brect.top), TRUE);
		MoveWindow(hDrawWnd, 0, 0, 640, 480, TRUE);
		if (hStatusBar) {
			MoveWindow(hStatusBar, 0, 480, 0, (brect.bottom - brect.top), TRUE);
		}
	}

	/* ���[�N�Z�b�g�A���� */
	bAnalog = FALSE;
	ReDrawDD();
	return TRUE;
}

/*-[ �`�� ]-----------------------------------------------------------------*/

/*
 *	�S�̈�N���A
 */
static void FASTCALL AllClear()
{
	DDSURFACEDESC ddsd;
	HRESULT hResult;
	RECT rect;
	BYTE *p;
	int i;

	/* �t���O�`�F�b�N */
	if (!bClearFlag) {
		return;
	}

	/* �T�[�t�F�C�X�����b�N */
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	hResult = lpdds[1]->Lock(NULL, &ddsd,
							 DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);
	if (hResult != DD_OK) {
		/* �T�[�t�F�C�X�����X�g���Ă���΁A���X�g�A */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[1]->Restore();
		}
		/* ����͑S�̈�X�V */
		ReDrawDD();
		return;
	}

	/* �I�[���N���A */
	p = (BYTE *)ddsd.lpSurface;
	if (bDD480Flag) {
		for (i=0; i<480; i++) {
			memset(p, 0, 640 * 2);
			p += ddsd.lPitch;
		}
	}
	else {
		for (i=0; i<400; i++) {
			memset(p, 0, 640 * 2);
			p += ddsd.lPitch;
		}
	}

	/* �T�[�t�F�C�X���A�����b�N */
	lpdds[1]->Unlock(ddsd.lpSurface);

	/* ���[�N���Z�b�g */
	bClearFlag = FALSE;
	nDrawTop = 0;
	nDrawBottom = 200;

	/* �X�e�[�^�X���C�����N���A */
	szCaption[0] = '\0';
	nCAP = -1;
	nKANA = -1;
	nINS = -1;
	nDrive[0] = -1;
	nDrive[1] = -1;
	szDrive[0][0] = '\0';
	szDrive[1][0] = '\0';
	nTape = -1;

	/* 640x480�A�X�e�[�^�X�Ȃ����̂�Blt */
	if (!bDD480Flag || bDD480Status) {
		return;
	}

	/* �����������΁ABlt */
	rect.top = 0;
	rect.left = 0;
	rect.right = 640;
	rect.bottom = 40;
	hResult = lpdds[0]->Blt(&rect, lpdds[1], &rect, DDBLT_WAIT, NULL);
	if (hResult != DD_OK) {
		/* �T�[�t�F�C�X�����X�g���Ă���΁A���X�g�A */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[0]->Restore();
			lpdds[1]->Restore();
		}
		/* ����͑S�̈�X�V */
		ReDrawDD();
		return;
	}
	rect.top = 440;
	rect.left = 0;
	rect.right = 640;
	rect.bottom = 480;
	hResult = lpdds[0]->Blt(&rect, lpdds[1], &rect, DDBLT_WAIT, NULL);
	if (hResult != DD_OK) {
		/* �T�[�t�F�C�X�����X�g���Ă���΁A���X�g�A */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[0]->Restore();
			lpdds[1]->Restore();
		}
		/* ����͑S�̈�X�V */
		ReDrawDD();
		return;
	}
}

/*
 *	�X�L�������C���`��
 */
static void FASTCALL RenderFullScan(BYTE *pSurface, LONG lPitch)
{
	UINT u;

	/* �t���O�`�F�b�N */
	if (!bFullScan) {
		return;
	}

	/* �����ݒ� */
	pSurface += nDrawTop * 2 * lPitch;

	/* ���[�v */
	for (u=nDrawTop; u<nDrawBottom; u++) {
		memcpy(&pSurface[lPitch], pSurface, 640 * 2);
		pSurface += (lPitch * 2);
	}
}

/*
 *	�X�e�[�^�X���C��(�L���v�V����)�`��
 */
static void FASTCALL DrawCaption(void)
{
	HDC hDC;
	HRESULT hResult;
	RECT rect;

	ASSERT(bDD480Flag);
	ASSERT(bDD480Status);

	/* DC�擾 */
	hResult = lpdds[1]->GetDC(&hDC);
	if (hResult != DD_OK) {
		/* �T�[�t�F�C�X�����X�g���Ă���΁A���X�g�A */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[1]->Restore();
		}
		/* ����͑S�̈�X�V */
		ReDrawDD();
		return;
	}

	/* ExtTextOut���g���A��x�ŕ`�� */
	SetBkColor(hDC, RGB(0, 0, 0));
	SetTextColor(hDC, RGB(255, 255, 255));
	rect.top = 0;
	rect.left = 0;
	rect.right = 640;
	rect.bottom = 40;
	ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rect,
							 szCaption, strlen(szCaption), NULL);

	/* DC����� */
	hResult = lpdds[1]->ReleaseDC(hDC);
	if (hResult != DD_OK) {
		/* �T�[�t�F�C�X�����X�g���Ă���΁A���X�g�A */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[1]->Restore();
		}
		/* ����͑S�̈�X�V */
		ReDrawDD();
		return;
	}

	/* Blt */
	hResult = lpdds[0]->Blt(&rect, lpdds[1], &rect, DDBLT_WAIT, NULL);
	if (hResult != DD_OK) {
		/* �T�[�t�F�C�X�����X�g���Ă���΁A���X�g�A */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[0]->Restore();
			lpdds[1]->Restore();
		}
		/* ����͑S�̈�X�V */
		ReDrawDD();
		return;
	}
}

/*
 *	�X�e�[�^�X���C��(�L���v�V����)
 */
static BOOL FASTCALL StatusCaption(void)
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

	/* ��r */
	string[127] = '\0';
	if (memcmp(szCaption, string, strlen(string) + 1) != 0) {
		strcpy(szCaption, string);
		return TRUE;
	}

	/* �O��Ɠ����Ȃ̂ŁA�`�悵�Ȃ��Ă悢 */
	return FALSE;
}

/*
 *	�X�e�[�^�X���C��(CAP)
 */
static BOOL FASTCALL StatusCAP(void)
{
	HDC hDC;
	HRESULT hResult;
	RECT rect;
	int num;

	/* �l�擾�A��r */
	if (caps_flag) {
		num = 1;
	}
	else {
		num = 0;
	}
	if (num == nCAP) {
		return FALSE;
	}

	/* DC�擾 */
	hResult = lpdds[1]->GetDC(&hDC);
	if (hResult != DD_OK) {
		/* �T�[�t�F�C�X�����X�g���Ă���΁A���X�g�A */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[1]->Restore();
		}
		/* ����͑S�̈�X�V */
		ReDrawDD();
		return FALSE;
	}

	/* -1�Ȃ�A�S�̈�N���A */
	rect.left = 0;
	rect.right = 640;
	rect.top = 440;
	rect.bottom = 480;
	SetBkColor(hDC, RGB(0, 0, 0));
	SetTextColor(hDC, RGB(255, 255, 255));
	if (nCAP == -1) {
		/* �N���A */
		ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);

		/* "CAP"�̕����`�� */
		TextOut(hDC, 500, 444, "CAP", 3);

		/* ���N��`�� */
		rect.left = 500;
		rect.right = rect.left + 30;
		rect.top = 465;
		rect.bottom = rect.top + 10;
		FrameRect(hDC, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
	}

	/* �����ŃR�s�[ */
	nCAP = num;

	/* ���C���F�`�� */
	rect.left = 501;
	rect.right = rect.left + 28;
	rect.top = 466;
	rect.bottom = rect.top + 8;
	if (nCAP == 1) {
		SetBkColor(hDC, RGB(255, 0, 0));
	}
	else {
		SetBkColor(hDC, RGB(0, 0, 0));
	}
	ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);

	/* DC��� */
	hResult = lpdds[1]->ReleaseDC(hDC);
	if (hResult != DD_OK) {
		/* �T�[�t�F�C�X�����X�g���Ă���΁A���X�g�A */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[1]->Restore();
		}
		/* ����͑S�̈�X�V */
		ReDrawDD();
		return FALSE;
	}

	return TRUE;
}

/*
 *	�X�e�[�^�X���C��(����)
 */
static BOOL FASTCALL StatusKANA(void)
{
	HDC hDC;
	HRESULT hResult;
	RECT rect;
	int num;

	/* �l�擾�A��r */
	if (kana_flag) {
		num = 1;
	}
	else {
		num = 0;
	}
	if (num == nKANA) {
		return FALSE;
	}

	/* DC�擾 */
	hResult = lpdds[1]->GetDC(&hDC);
	if (hResult != DD_OK) {
		/* �T�[�t�F�C�X�����X�g���Ă���΁A���X�g�A */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[1]->Restore();
		}
		/* ����͑S�̈�X�V */
		ReDrawDD();
		return FALSE;
	}

	SetBkColor(hDC, RGB(0, 0, 0));
	SetTextColor(hDC, RGB(255, 255, 255));
	if (nKANA == -1) {
		/* "����"�̕����`�� */
		TextOut(hDC, 546, 444, "����", 4);

		/* ���N��`�� */
		rect.left = 546;
		rect.right = rect.left + 30;
		rect.top = 465;
		rect.bottom = rect.top + 10;
		FrameRect(hDC, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
	}

	/* �����ŃR�s�[ */
	nKANA = num;

	/* ���C���F�`�� */
	rect.left = 547;
	rect.right = rect.left + 28;
	rect.top = 466;
	rect.bottom = rect.top + 8;
	if (nKANA == 1) {
		SetBkColor(hDC, RGB(255, 0, 0));
	}
	else {
		SetBkColor(hDC, RGB(0, 0, 0));
	}
	ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);

	/* DC��� */
	hResult = lpdds[1]->ReleaseDC(hDC);
	if (hResult != DD_OK) {
		/* �T�[�t�F�C�X�����X�g���Ă���΁A���X�g�A */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[1]->Restore();
		}
		/* ����͑S�̈�X�V */
		ReDrawDD();
		return FALSE;
	}

	return TRUE;
}

/*
 *	�X�e�[�^�X���C��(INS)
 */
static BOOL FASTCALL StatusINS(void)
{
	HDC hDC;
	HRESULT hResult;
	RECT rect;
	int num;

	/* �l�擾�A��r */
	if (ins_flag) {
		num = 1;
	}
	else {
		num = 0;
	}
	if (num == nINS) {
		return FALSE;
	}

	/* DC�擾 */
	hResult = lpdds[1]->GetDC(&hDC);
	if (hResult != DD_OK) {
		/* �T�[�t�F�C�X�����X�g���Ă���΁A���X�g�A */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[1]->Restore();
		}
		/* ����͑S�̈�X�V */
		ReDrawDD();
		return FALSE;
	}

	SetBkColor(hDC, RGB(0, 0, 0));
	SetTextColor(hDC, RGB(255, 255, 255));
	if (nINS == -1) {
		/* "INS"�̕����`�� */
		TextOut(hDC, 593 + 2, 444, "INS", 3);

		/* ���N��`�� */
		rect.left = 593;
		rect.right = rect.left + 30;
		rect.top = 465;
		rect.bottom = rect.top + 10;
		FrameRect(hDC, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
	}

	/* �����ŃR�s�[ */
	nINS = num;

	/* ���C���F�`�� */
	rect.left = 594;
	rect.right = rect.left + 28;
	rect.top = 466;
	rect.bottom = rect.top + 8;
	if (nINS == 1) {
		SetBkColor(hDC, RGB(255, 0, 0));
	}
	else {
		SetBkColor(hDC, RGB(0, 0, 0));
	}
	ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);

	/* DC��� */
	hResult = lpdds[1]->ReleaseDC(hDC);
	if (hResult != DD_OK) {
		/* �T�[�t�F�C�X�����X�g���Ă���΁A���X�g�A */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[1]->Restore();
		}
		/* ����͑S�̈�X�V */
		ReDrawDD();
		return FALSE;
	}

	return TRUE;
}

/*
 *	�X�e�[�^�X���C��(�t���b�s�[�h���C�u)
 */
static BOOL FASTCALL StatusDrive(int drive)
{
	HDC hDC;
	HRESULT hResult;
	RECT rect;
	int num;
	char *name;

	ASSERT((drive == 0) || (drive == 1));

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
			return FALSE;
		}
		if (strcmp(szDrive[drive], name) == 0) {
			return FALSE;
		}
	}

	/* DC�擾 */
	hResult = lpdds[1]->GetDC(&hDC);
	if (hResult != DD_OK) {
		/* �T�[�t�F�C�X�����X�g���Ă���΁A���X�g�A */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[1]->Restore();
		}
		/* ����͑S�̈�X�V */
		ReDrawDD();
		return FALSE;
	}

	/* �����ŃR�s�[ */
	nDrive[drive] = num;
	strcpy(szDrive[drive], name);

	/* ���W�ݒ� */
	SetBkColor(hDC, RGB(0, 0, 0));
	SetTextColor(hDC, RGB(255, 255, 255));
	rect.left = (drive ^ 1) * 160;
	rect.right = ((drive ^ 1) + 1) * 160 - 4;
	rect.top = 444;
	rect.bottom = 474;

	/* �F���� */
	if (nDrive[drive] != 255) {
		SetBkColor(hDC, RGB(63, 63, 63));
	}
	if (nDrive[drive] == FDC_ACCESS_READ) {
		SetBkColor(hDC, RGB(191, 0, 0));
	}
	if (nDrive[drive] == FDC_ACCESS_WRITE) {
		SetBkColor(hDC, RGB(0, 0, 191));
	}

	/* �w�i��h��Ԃ� */
	ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);

	/* DrawText */
	DrawText(hDC, szDrive[drive], strlen(szDrive[drive]), &rect,
		DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

	/* DC��� */
	hResult = lpdds[1]->ReleaseDC(hDC);
	if (hResult != DD_OK) {
		/* �T�[�t�F�C�X�����X�g���Ă���΁A���X�g�A */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[1]->Restore();
		}
		/* ����͑S�̈�X�V */
		ReDrawDD();
		return FALSE;
	}

	return TRUE;
}

/*
 *	�X�e�[�^�X���C��(�e�[�v)
 */
static BOOL FASTCALL StatusTape(void)
{
	HDC hDC;
	HRESULT hResult;
	RECT rect;
	int num;
	char string[64];

	/* �ԍ��Z�b�g */
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
		return FALSE;
	}

	/* DC�擾 */
	hResult = lpdds[1]->GetDC(&hDC);
	if (hResult != DD_OK) {
		/* �T�[�t�F�C�X�����X�g���Ă���΁A���X�g�A */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[1]->Restore();
		}
		/* ����͑S�̈�X�V */
		ReDrawDD();
		return FALSE;
	}

	/* �����ŃR�s�[ */
	nTape = num;

	/* ���W�ݒ� */
	SetBkColor(hDC, RGB(0, 0, 0));
	SetTextColor(hDC, RGB(255, 255, 255));
	rect.left = 360;
	rect.right = rect.left + 80;
	rect.top = 444;
	rect.bottom = 474;

	/* �F�A�����񌈒� */
	if (nTape >= 30000) {
		string[0] = '\0';
	}
	else {
		sprintf(string, "%04d", nTape % 10000);
		if (nTape >= 10000) {
			if (nTape >= 20000) {
				SetBkColor(hDC, RGB(0, 0, 191));
			}
			else {
				SetBkColor(hDC, RGB(191, 0, 0));
			}
		}
		else {
			SetBkColor(hDC, RGB(63, 63, 63));
		}
	}

	/* �w�i��h��Ԃ� */
	ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);

	/* DrawText */
	DrawText(hDC, string, strlen(string), &rect,
		DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

	/* DC��� */
	hResult = lpdds[1]->ReleaseDC(hDC);
	if (hResult != DD_OK) {
		/* �T�[�t�F�C�X�����X�g���Ă���΁A���X�g�A */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[1]->Restore();
		}
		/* ����͑S�̈�X�V */
		ReDrawDD();
		return FALSE;
	}

	return TRUE;
}

/*
 *	�X�e�[�^�X���C���`��
 */
static void FASTCALL StatusLine(void)
{
	BOOL flag;
	HRESULT hResult;
	RECT rect;

	/* 640x480�A�X�e�[�^�X����̏ꍇ�̂� */
	if (!bDD480Flag || !bDD480Status) {
		return;
	}

	/* �L���v�V���� */
	if (StatusCaption()) {
		DrawCaption();
	}

	flag = FALSE;

	/* �L�[�{�[�h�X�e�[�^�X */
	if (StatusCAP()) {
		flag = TRUE;
	}
	if (StatusKANA()) {
		flag = TRUE;
	}
	if (StatusINS()) {
		flag = TRUE;
	}
	if (StatusDrive(0)) {
		flag = TRUE;
	}
	if (StatusDrive(1)) {
		flag = TRUE;
	}
	if (StatusTape()) {
		flag = TRUE;
	}

	/* �t���O���~��Ă���΁A�`�悷��K�v�Ȃ� */
	if (!flag) {
		return;
	}

	/* Blt */
	rect.left = 0;
	rect.right = 640;
	rect.top = 440;
	rect.bottom = 480;
	hResult = lpdds[0]->Blt(&rect, lpdds[1], &rect, DDBLT_WAIT, NULL);
	if (hResult != DD_OK) {
		/* �T�[�t�F�C�X�����X�g���Ă���΁A���X�g�A */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[0]->Restore();
			lpdds[1]->Restore();
		}
		/* ����͑S�̈�X�V */
		ReDrawDD();
		return;
	}
}

/*
 *	640x200�A�f�W�^�����[�h
 *	�p���b�g�ݒ�
 */
static void FASTCALL Palet640(void)
{
	int i;
	int vpage;

	/* �p���b�g�e�[�u�� */
	static const WORD rgbTable[] = {
		/* nPixelFormat = 1 */
		0x0000 | 0x0000 | 0x0000,
		0x0000 | 0x0000 | 0x001f,
		0xf800 | 0x0000 | 0x0000,
		0xf800 | 0x0000 | 0x001f,
		0x0000 | 0x07e0 | 0x0000,
		0x0000 | 0x07e0 | 0x001f,
		0xf800 | 0x07e0 | 0x0000,
		0xf800 | 0x07e0 | 0x001f,

		/* nPixelFormat = 2 */
		0x0000 | 0x0000 | 0x0000,
		0x0000 | 0x0000 | 0x001f,
		0x7c00 | 0x0000 | 0x0000,
		0x7c00 | 0x0000 | 0x001f,
		0x0000 | 0x03e0 | 0x0000,
		0x0000 | 0x03e0 | 0x001f,
		0x7c00 | 0x03e0 | 0x0000,
		0x7c00 | 0x03e0 | 0x001f
	};

	/* �t���O���Z�b�g����Ă��Ȃ���΁A�������Ȃ� */
	if (!bPaletFlag) {
		return;
	}

	/* �}���`�y�[�W���A�\���v���[�����𓾂� */
	vpage = (~(multi_page >> 4)) & 0x07;

	/* 640x200�A�f�W�^���p���b�g */
	for (i=0; i<8; i++) {
		if (crt_flag) {
			/* CRT ON */
			if (nPixelFormat == 1) {
				rgbTTLDD[i] = rgbTable[ttl_palet[i & vpage] + 0];
			}
			if (nPixelFormat == 2) {
				rgbTTLDD[i] = rgbTable[ttl_palet[i & vpage] + 8];
			}
		}
		else {
			/* CRT OFF */
			rgbTTLDD[i] = 0;
		}
	}

	/* �t���O�~�낷 */
	bPaletFlag = FALSE;
}

/*
 *	640x200�A�f�W�^�����[�h
 *	�`��
 */
static void FASTCALL Draw640(void)
{
	DDSURFACEDESC ddsd;
	HRESULT hResult;
	RECT rect;
	BYTE *p;

	/* �I�[���N���A */
	AllClear();

	/* �p���b�g�ݒ� */
	Palet640();

	/* �X�e�[�^�X���C�� */
	StatusLine();

	/* �����_�����O�`�F�b�N */
	if (nDrawTop >= nDrawBottom) {
		return;
	}

	/* �T�[�t�F�C�X�����b�N */
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	hResult = lpdds[1]->Lock(NULL, &ddsd,
							 DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);
	if (hResult != DD_OK) {
		/* �T�[�t�F�C�X�����X�g���Ă���΁A���X�g�A */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[1]->Restore();
		}
		/* ����͑S�̈�X�V */
		ReDrawDD();
		return;
	}

	/* �����_�����O */
	p = (BYTE *)ddsd.lpSurface;
	if (bDD480Flag) {
		p += (ddsd.lPitch * 40);
	}
	Render640DD(p, ddsd.lPitch, nDrawTop, nDrawBottom);
	RenderFullScan(p, ddsd.lPitch);

	/* �T�[�t�F�C�X���A�����b�N */
	lpdds[1]->Unlock(ddsd.lpSurface);

	/* Blt */
	rect.top = nDrawTop * 2;
	rect.left = 0;
	rect.right = 640;
	rect.bottom = nDrawBottom * 2;
	if (bDD480Flag) {
		rect.top += 40;
		rect.bottom += 40;
	}
	hResult = lpdds[0]->Blt(&rect, lpdds[1], &rect, DDBLT_WAIT, NULL);
	if (hResult != DD_OK) {
		/* �T�[�t�F�C�X�����X�g���Ă���΁A���X�g�A */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[0]->Restore();
			lpdds[1]->Restore();
		}
		/* ����͑S�̈�X�V */
		ReDrawDD();
		return;
	}

	/* ����ɔ����A���[�N���Z�b�g */
	nDrawTop = 200;
	nDrawBottom = 0;
}

/*
 *	320x200�A�A�i���O���[�h
 *	�p���b�g�ݒ�
 */
static void FASTCALL Palet320(void)
{
	int i, j;
	WORD r, g, b;
	int amask;

	/* �t���O���Z�b�g����Ă��Ȃ���΁A�������Ȃ� */
	if (!bPaletFlag) {
		return;
	}

	/* �A�i���O�}�X�N���쐬 */
	amask = 0;
	if (!(multi_page & 0x10)) {
		amask |= 0x000f;
	}
	if (!(multi_page & 0x20)) {
		amask |= 0x00f0;
	}
	if (!(multi_page & 0x40)) {
		amask |= 0x0f00;
	}

	for (i=0; i<4096; i++) {
		/* �ŉ��ʂ���5bit�Â�B,G,R */
		if (crt_flag) {
			j = i & amask;
			r = (WORD)apalet_r[j];
			g = (WORD)apalet_g[j];
			b = (WORD)apalet_b[j];
		}
		else {
			r = 0;
			g = 0;
			b = 0;
		}

		/* �s�N�Z���^�C�v�ɉ����AWORD�f�[�^���쐬 */
		if (nPixelFormat == 1) {
			/* R5bit, G6bit, B5bit�^�C�v */
			r <<= 12;
			if (r > 0) {
				r |= 0x0800;
			}

			g <<= 7;
			if (g > 0) {
				g |= 0x0060;
			}

			b <<= 1;
			if (b > 0) {
				b |= 0x0001;
			}
		}
		if (nPixelFormat == 2) {
			/* R5bit, G5bit, B5bit�^�C�v */
			r <<= 11;
			if (r > 0) {
				r |= 0x0400;
			}

			g <<= 6;
			if (g > 0) {
				g |= 0x0020;
			}

			b <<= 1;
			if (b > 0) {
				b |= 0x0001;
			}
		}

		/* �Z�b�g */
		rgbAnalogDD[i] = (WORD)(r | g | b);
	}

	/* �t���O�~�낷 */
	bPaletFlag = FALSE;
}

/*
 *	320x200�A�A�i���O���[�h
 *	�`��
 */
static void FASTCALL Draw320(void)
{
	DDSURFACEDESC ddsd;
	HRESULT hResult;
	RECT rect;
	BYTE *p;

	/* �I�[���N���A */
	AllClear();

	/* �p���b�g�ݒ� */
	Palet320();

	/* �X�e�[�^�X���C�� */
	StatusLine();

	/* �����_�����O�`�F�b�N */
	if (nDrawTop >= nDrawBottom) {
		return;
	}

	/* �T�[�t�F�C�X�����b�N */
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	hResult = lpdds[1]->Lock(NULL, &ddsd,
							 DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);
	if (hResult != DD_OK) {
		/* �T�[�t�F�C�X�����X�g���Ă���΁A���X�g�A */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[1]->Restore();
		}
		/* ����͑S�̈�X�V */
		ReDrawDD();
		return;
	}

	/* �����_�����O */
	p = (BYTE *)ddsd.lpSurface;
	if (bDD480Flag) {
		p += (ddsd.lPitch * 40);
	}
	Render320DD(p, ddsd.lPitch, nDrawTop, nDrawBottom);
	RenderFullScan(p, ddsd.lPitch);

	/* �T�[�t�F�C�X���A�����b�N */
	lpdds[1]->Unlock(ddsd.lpSurface);

	/* Blt */
	rect.top = nDrawTop * 2;
	rect.left = 0;
	rect.right = 640;
	rect.bottom = nDrawBottom * 2;
	if (bDD480Flag) {
		rect.top += 40;
		rect.bottom += 40;
	}
	hResult = lpdds[0]->Blt(&rect, lpdds[1], &rect, DDBLT_WAIT, NULL);
	if (hResult != DD_OK) {
		/* �T�[�t�F�C�X�����X�g���Ă���΁A���X�g�A */
		if (hResult == DDERR_SURFACELOST) {
			lpdds[0]->Restore();
			lpdds[1]->Restore();
		}
		/* ����͑S�̈�X�V */
		ReDrawDD();
		return;
	}

	/* ����ɔ����A���[�N���Z�b�g */
	nDrawTop = 200;
	nDrawBottom = 0;
}

/*
 *	�`��
 */
void FASTCALL DrawDD(void)
{

	/* �f�W�^���E�A�i���O���� */
	if (mode320) {
		if (!bAnalog) {
			ReDrawDD();
			bAnalog = TRUE;
		}
	}
	else {
		if (bAnalog) {
			ReDrawDD();
			bAnalog = FALSE;
		}
	}

	/* �ǂ��炩���g���ĕ`�� */
	if (bAnalog) {
		Draw320();
	}
	else {
		Draw640();
	}
}

/*
 *	���j���[�J�n
 */
void FASTCALL EnterMenuDD(HWND hWnd)
{
	ASSERT(hWnd);

	/* �N���b�p�[�̗L���Ŕ���ł��� */
	if (!lpddc) {
		return;
	}

	/* �N���b�p�[�Z�b�g */
	LockVM();
	lpdds[0]->SetClipper(lpddc);
	UnlockVM();

	/* �}�E�X�J�[�\��on */
	if (!bMouseCursor) {
		ShowCursor(TRUE);
		bMouseCursor = TRUE;
	}

	/* ���j���[�o�[��`�� */
	DrawMenuBar(hWnd);
}

/*
 *	���j���[�I��
 */
void FASTCALL ExitMenuDD(void)
{
	/* �N���b�p�[�̗L���Ŕ���ł��� */
	if (!lpddc) {
		return;
	}

	/* �N���b�p�[���� */
	LockVM();
	lpdds[0]->SetClipper(NULL);
	UnlockVM();

	/* �}�E�X�J�[�\��OFF */
	if (bMouseCursor) {
		ShowCursor(FALSE);
		bMouseCursor = FALSE;
	}

	/* �ĕ\�� */
	ReDrawDD();
}

/*-[ VM�Ƃ̐ڑ� ]-----------------------------------------------------------*/

/*
 *	VRAM�Z�b�g
 */
void FASTCALL VramDD(WORD addr)
{
	UINT y;

	/* y���W�Z�o */
	if (bAnalog) {
		addr &= 0x1fff;
		y = (UINT)(addr / 40);
	}
	else {
		addr &= 0x3fff;
		y = (UINT)(addr / 80);
	}
	if (y >= 200) {
		return;
	}

	/* �X�V */
	if (nDrawTop > y) {
		nDrawTop = y;
	}
	if (nDrawBottom <= y) {
		nDrawBottom = y + 1;
	}
}

/*
 *	TTL�p���b�g�Z�b�g
 */
void FASTCALL DigitalDD(void)
{
	nDrawTop = 0;
	nDrawBottom = 200;
	bPaletFlag = TRUE;
}

/*
 *	�A�i���O�p���b�g�Z�b�g
 */
void FASTCALL AnalogDD(void)
{
	nDrawTop = 0;
	nDrawBottom = 200;
	bPaletFlag = TRUE;
}

/*
 *	�ĕ`��v��
 */
void FASTCALL ReDrawDD(void)
{
	/* �S�̈惌���_�����O */
	nDrawTop = 0;
	nDrawBottom = 200;
	bPaletFlag = TRUE;
	bClearFlag = TRUE;
}

#endif	/* _WIN32 */
