/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API �\�� ]
 */

#ifdef _WIN32

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include "xm7.h"
#include "device.h"
#include "subctrl.h"
#include "display.h"
#include "w32.h"
#include "w32_gdi.h"
#include "w32_dd.h"
#include "w32_draw.h"

/*
 *	�O���[�o�� ���[�N
 */
BOOL bFullScreen;							/* �t���X�N���[���t���O */
BOOL bFullScan;								/* �t���X�L�����t���O */

/*
 *	������
 */
void FASTCALL InitDraw(void)
{
	/* ���[�N�G���A������ */
	bFullScreen = FALSE;
	bFullScan = FALSE;

	/* �N�����GDI�Ȃ̂ŁAGDI�������� */
	InitGDI();
}

/*
 *	�N���[���A�b�v
 */
void FASTCALL CleanDraw(void)
{
	/* �t���X�N���[���t���O�ɉ����āA�N���[���A�b�v */
	if (bFullScreen) {
		CleanDD();
	}
	else {
		CleanGDI();
	}
}

/*
 *  �Z���N�g
 */
BOOL FASTCALL SelectDraw(HWND hWnd)
{
	ASSERT(hWnd);

	/* �N�����GDI��I�� */
	return SelectGDI(hWnd);
}

/*
 *	���[�h�؂�ւ�
 */
void FASTCALL ModeDraw(HWND hWnd, BOOL bFullFlag)
{
	ASSERT(hWnd);

	/* ����ƈ�v���Ă���΁A�ς���K�v�Ȃ� */
	if (bFullFlag == bFullScreen) {
		return;
	}

	if (bFullFlag) {
		/* �t���X�N���[���� */
		CleanGDI();
		InitDD();
		bFullScreen = TRUE;
		if (!SelectDD()) {
			/* �t���X�N���[�����s */
			bFullScreen = FALSE;
			CleanDD();
			InitGDI();
			SelectGDI(hWnd);
		}
	}
	else {
		/* �E�C���h�E�� */
		bFullScreen = FALSE;
		CleanDD();
		InitGDI();
		if (!SelectGDI(hWnd)) {
			/* �E�C���h�E���s */
			CleanGDI();
			bFullScreen = TRUE;
			InitDD();
			SelectDD();
		}
	}
}

/*
 *	�`��(�ʏ�)
 */
void FASTCALL OnDraw(HWND hWnd, HDC hDC)
{
	ASSERT(hWnd);
	ASSERT(hDC);

	/* �G���[�Ȃ牽�����Ȃ� */
	if (nErrorCode == 0) {
		if (bFullScreen) {
			DrawDD();
		}
		else {
			DrawGDI(hWnd, hDC);
		}
	}
}

/*
 *	�`��(WM_PAINT)
 */
void FASTCALL OnPaint(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hDC;

	ASSERT(hWnd);

	hDC = BeginPaint(hWnd, &ps);

	/* �G���[�Ȃ牽�����Ȃ� */
	if (nErrorCode == 0) {
		/* �ĕ`��w�� */
		if (bFullScreen) {
			ReDrawDD();
		}
		else {
			ReDrawGDI();
		}

		/* �`�� */
		OnDraw(hWnd, hDC);
	}

	EndPaint(hWnd, &ps);
}

/*-[ VM�Ƃ̐ڑ� ]-----------------------------------------------------------*/

/*
 *	VRAM�������ݒʒm
 */
void FASTCALL vram_notify(WORD addr, BYTE dat)
{
	/* �A�N�e�B�u�y�[�W���f�B�X�v���C�y�[�W�ƈقȂ�΁A���Ȃ� */
	if (!mode320) {
		if (vram_active != vram_display) {
			return;
		}
	}

	if (bFullScreen) {
		VramDD(addr);
	}
	else {
		VramGDI(addr);
	}
}

/*
 *	TTL�p���b�g�ʒm
 */
void FASTCALL ttlpalet_notify(void)
{
	if (bFullScreen) {
		DigitalDD();
	}
	else {
		DigitalGDI();
	}
}

/*
 *	�A�i���O�p���b�g�ʒm
 */
void FASTCALL apalet_notify(void)
{
	if (bFullScreen) {
		AnalogDD();
	}
	else {
		AnalogGDI();
	}
}

/*
 *  �ĕ`��v���ʒm
 */
void FASTCALL display_notify(void)
{
	if (bFullScreen) {
		/* ReDraw�͖��ʂȃN���A���܂ނ̂ŁA�����׍H */
		AnalogDD();
	}
	else {
		AnalogGDI();
	}
}

/*
 *	�f�B�W�^�C�Y�v���ʒm
 */
void FASTCALL digitize_notify(void)
{
}

#endif	/* _WIN32 */
