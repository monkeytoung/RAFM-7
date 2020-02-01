/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API �E�C���h�E�\�� ]
 */

#ifdef _WIN32

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdlib.h>
#include "xm7.h"
#include "subctrl.h"
#include "display.h"
#include "multipag.h"
#include "ttlpalet.h"
#include "apalet.h"
#include "w32.h"
#include "w32_draw.h"
#include "w32_gdi.h"

/*
 *	�O���[�o�� ���[�N
 */
WORD rgbTTLGDI[8];							/* �f�W�^���p���b�g */
WORD rgbAnalogGDI[4096];					/* �A�i���O�p���b�g */
BYTE *pBitsGDI;								/* �r�b�g�f�[�^ */

/*
 *	�X�^�e�B�b�N ���[�N
 */
static BOOL bAnalog;						/* �A�i���O���[�h�t���O */
static HBITMAP hBitmap;						/* �r�b�g�}�b�v �n���h�� */
static UINT nDrawTop;						/* �`��͈͏� */
static UINT nDrawBottom;					/* �`��͈͉� */
static BOOL bPaletFlag;						/* �p���b�g�ύX�t���O */
static BOOL bClearFlag;						/* �N���A�t���O */

/*
 *	�A�Z���u���֐��̂��߂̃v���g�^�C�v�錾
 */
#ifdef __cplusplus
extern "C" {
#endif
void Render640GDI(int first, int last);
void Render320GDI(int first, int last);
#ifdef __cplusplus
}
#endif

/*
 *	������
 */
void FASTCALL InitGDI(void)
{
	/* ���[�N�G���A������ */
	bAnalog = FALSE;
	hBitmap = NULL;
	pBitsGDI = NULL;
	memset(rgbTTLGDI, 0, sizeof(rgbTTLGDI));
	memset(rgbAnalogGDI, 0, sizeof(rgbAnalogGDI));

	nDrawTop = 0;
	nDrawBottom = 200;
	bPaletFlag = FALSE;
}

/*
 *	�N���[���A�b�v
 */
void FASTCALL CleanGDI(void)
{
	if (hBitmap) {
		DeleteObject(hBitmap);
		hBitmap = NULL;
		pBitsGDI = NULL;
	}
}

/*
 *	GDI�Z���N�g����
 */
static BOOL FASTCALL SelectSub(HWND hWnd)
{
	BITMAPINFOHEADER *pbmi;
	HDC hDC;

	ASSERT(hWnd);
	ASSERT(!hBitmap);
	ASSERT(!pBitsGDI);

	/* �������m�� */
	pbmi = (BITMAPINFOHEADER*)malloc(sizeof(BITMAPINFOHEADER) +
									 sizeof(RGBQUAD));
	if (!pbmi) {
		return FALSE;
	}
	memset(pbmi, 0, sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD));

	/* �r�b�g�}�b�v���쐬 */
	pbmi->biSize = sizeof(BITMAPINFOHEADER);
	pbmi->biWidth = 640;
	pbmi->biHeight = -400;
	pbmi->biPlanes = 1;
	pbmi->biBitCount = 16;
	pbmi->biCompression = BI_RGB;

	/* DC�擾�ADIB�Z�N�V�����쐬 */
	hDC = GetDC(hWnd);
	hBitmap = CreateDIBSection(hDC, (BITMAPINFO*)pbmi, DIB_RGB_COLORS,
								(void**)&pBitsGDI, NULL, 0);
	ReleaseDC(hWnd, hDC);
	free(pbmi);
	if (!hBitmap) {
		return FALSE;
	}

	/* �S�G���A���A��x�N���A */
	bClearFlag = TRUE;
	return TRUE;
}

/*
 *  640x200�A�f�W�^�����[�h
 *	�Z���N�g
 */
static BOOL FASTCALL Select640(HWND hWnd)
{
	if (!pBitsGDI) {
		if (!SelectSub(hWnd)) {
			return FALSE;
		}
	}

	/* �S�̈斳�� */
	nDrawTop = 0;
	nDrawBottom = 200;
	bPaletFlag = TRUE;

	/* �f�W�^�����[�h */
	bAnalog = FALSE;

	return TRUE;
}

/*
 *  320x200�A�A�i���O���[�h
 *	�Z���N�g
 */
static BOOL FASTCALL Select320(HWND hWnd)
{
	if (!pBitsGDI) {
		if (!SelectSub(hWnd)) {
			return FALSE;
		}
	}

	/* �S�̈斳�� */
	nDrawTop = 0;
	nDrawBottom = 200;
	bPaletFlag = TRUE;

	/* �A�i���O���[�h */
	bAnalog = TRUE;

	return TRUE;
}

/*
 *	�Z���N�g
 */
BOOL FASTCALL SelectGDI(HWND hWnd)
{
	ASSERT(hWnd);

	/* ���������Ȃ� */
	if (!pBitsGDI) {
		if (mode320) {
			return Select320(hWnd);
		}
		else {
			return Select640(hWnd);
		}
	}

	/* ���[�h����v���Ă���΁A�������Ȃ� */
	if (mode320) {
		if (bAnalog) {
			return TRUE;
		}
	}
	else {
		if (!bAnalog) {
			return TRUE;
		}
	}

	/* �Z���N�g */
	if (mode320) {
		return Select320(hWnd);
	}
	else {
		return Select640(hWnd);
	}
}

/*-[ �`�� ]-----------------------------------------------------------------*/

/*
 *	�I�[���N���A
 */
static void FASTCALL AllClear(void)
{
	/* ���ׂăN���A */
	memset(pBitsGDI, 0, 640 * 400 * 2);

	/* �S�̈�������_�����O�ΏۂƂ��� */
	nDrawTop = 0;
	nDrawBottom = 200;

	bClearFlag = FALSE;
}

/*
 *	�t���X�L����
 */
static void FASTCALL RenderFullScan(void)
{
	BYTE *p;
	BYTE *q;
	UINT u;

	/* �|�C���^������ */
	p = &pBitsGDI[nDrawTop * 2 * 640 * 2];
	q = p + 640 * 2;

	/* ���[�v */
	for (u=nDrawTop; u<nDrawBottom; u++) {
		memcpy(q, p, 640 * 2);
		p += 640 * 2 * 2;
		q += 640 * 2 * 2;
	}
}

/*
 *	640x400�A�f�W�^�����[�h
 *	�p���b�g�ݒ�
 */
static void FASTCALL Palet640(void)
{
	int i;
	int vpage;

	/* �p���b�g�e�[�u�� */
	static const WORD rgbTable[] = {
		0x0000 | 0x0000 | 0x0000,
		0x0000 | 0x0000 | 0x001f,
		0x7c00 | 0x0000 | 0x0000,
		0x7c00 | 0x0000 | 0x001f,
		0x0000 | 0x03e0 | 0x0000,
		0x0000 | 0x03e0 | 0x001f,
		0x7c00 | 0x03e0 | 0x0000,
		0x7c00 | 0x03e0 | 0x001f
	};

	/* �}���`�y�[�W���A�\���v���[�����𓾂� */
	vpage = (~(multi_page >> 4)) & 0x07;

	/* 640x200�A�f�W�^���p���b�g */
	for (i=0; i<8; i++) {
		if (crt_flag) {
			/* CRT ON */
			rgbTTLGDI[i] = rgbTable[ttl_palet[i & vpage] + 0];
		}
		else {
			/* CRT OFF */
			rgbTTLGDI[i] = 0;
		}
	}
}

/*
 *	640x200�A�f�W�^�����[�h
 *	�`��
 */
static void FASTCALL Draw640(HWND hWnd, HDC hDC)
{
	HDC hMemDC;
	HBITMAP hDefBitmap;

	ASSERT(hWnd);
	ASSERT(hDC);

	/* �p���b�g�ݒ� */
	if (bPaletFlag) {
		Palet640();
	}

	/* �N���A���� */
	if (bClearFlag) {
		AllClear();
	}

	/* �����_�����O */
	if (nDrawTop >= nDrawBottom) {
		return;
	}
	Render640GDI(nDrawTop, nDrawBottom);
	if (bFullScan) {
		RenderFullScan();
	}

	/* ������DC�쐬�A�I�u�W�F�N�g�I�� */
	hMemDC = CreateCompatibleDC(hDC);
	ASSERT(hMemDC);
	hDefBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);

	/* �f�X�N�g�b�v�̏󋵂�����ł́A���ۂ����蓾��l */
	if (hDefBitmap) {
		BitBlt(hDC, 0, nDrawTop * 2, 640, (nDrawBottom - nDrawTop) * 2,
				hMemDC, 0, nDrawTop * 2, SRCCOPY);
		SelectObject(hMemDC, hDefBitmap);

		/* ����ɔ����A���[�N���Z�b�g */
		nDrawTop = 200;
		nDrawBottom = 0;
		bPaletFlag = FALSE;
	}
	else {
		/* �ĕ`����N���� */
		InvalidateRect(hWnd, NULL, FALSE);
	}

	/* ������DC�폜 */
	DeleteDC(hMemDC);
}

/*
 *	320x200�A�A�i���O���[�h
 *	�p���b�g�ݒ�
 */
static void FASTCALL Palet320(void)
{
	int i, j;
	WORD color;
	BYTE r, g, b;
	int amask;

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
		color = 0;
		if (crt_flag) {
			j = i & amask;
			r = apalet_r[j];
			g = apalet_g[j];
			b = apalet_b[j];
		}
		else {
			r = 0;
			g = 0;
			b = 0;
		}

		/* R */
		r <<= 1;
		if (r > 0) {
			r |= 0x01;
		}
		color |= (WORD)r;
		color <<= 5;

		/* G */
		g <<= 1;
		if (g > 0) {
			g |= 0x01;
		}
		color |= (WORD)g;
		color <<= 5;

		/* B */
		b <<= 1;
		if (b > 0) {
			b |= 0x01;
		}
		color |= (WORD)b;

		/* �Z�b�g */
		rgbAnalogGDI[i] = color;
	}
}

/*
 *	320x200�A�A�i���O���[�h
 *	�`��
 */
static void FASTCALL Draw320(HWND hWnd, HDC hDC)
{
	HDC hMemDC;
	HBITMAP hDefBitmap;

	ASSERT(hWnd);
	ASSERT(hDC);

	/* �p���b�g�ݒ� */
	if (bPaletFlag) {
		Palet320();
	}

	/* �N���A���� */
	if (bClearFlag) {
		AllClear();
	}

	/* �����_�����O */
	if (nDrawTop >= nDrawBottom) {
		return;
	}
	Render320GDI(nDrawTop, nDrawBottom);
	if (bFullScan) {
		RenderFullScan();
	}

	/* ������DC�쐬�A�I�u�W�F�N�g�I�� */
	hMemDC = CreateCompatibleDC(hDC);
	ASSERT(hMemDC);
	hDefBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);

	/* �f�X�N�g�b�v�̏󋵂�����ł́A���ۂ����蓾��l */
	if (hDefBitmap) {
		BitBlt(hDC, 0, nDrawTop * 2, 640, (nDrawBottom - nDrawTop) * 2,
				hMemDC, 0, nDrawTop * 2, SRCCOPY);
		SelectObject(hMemDC, hDefBitmap);

		/* ����ɔ����A���[�N���Z�b�g */
		nDrawTop = 200;
		nDrawBottom = 0;
		bPaletFlag = FALSE;
	}
	else {
		/* �ĕ`����N���� */
		InvalidateRect(hWnd, NULL, FALSE);
	}

	/* ������DC��� */
	DeleteDC(hMemDC);
}

/*
 *	�`��
 */
void FASTCALL DrawGDI(HWND hWnd, HDC hDC)
{
	ASSERT(hWnd);
	ASSERT(hDC);

	/* 640-320 �����؂�ւ� */
	SelectGDI(hWnd);

	/* �ǂ��炩���g���ĕ`�� */
	if (bAnalog) {
		Draw320(hWnd, hDC);
	}
	else {
		Draw640(hWnd, hDC);
	}
}

/*-[ VM�Ƃ̐ڑ� ]-----------------------------------------------------------*/

/*
 *	VRAM�Z�b�g
 */
void FASTCALL VramGDI(WORD addr)
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
void FASTCALL DigitalGDI(void)
{
	bPaletFlag = TRUE;
	nDrawTop = 0;
	nDrawBottom = 200;
}

/*
 *	�A�i���O�p���b�g�Z�b�g
 */
void FASTCALL AnalogGDI(void)
{
	bPaletFlag = TRUE;
	nDrawTop = 0;
	nDrawBottom = 200;
}

/*
 *	�ĕ`��v��
 */
void FASTCALL ReDrawGDI(void)
{
	/* �ĕ`�� */
	nDrawTop = 0;
	nDrawBottom = 200;
	bPaletFlag = TRUE;
	bClearFlag = TRUE;
}

#endif	/* _WIN32 */
