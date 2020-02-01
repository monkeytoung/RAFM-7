/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API �\�� ]
 */

#ifdef _WIN32

#ifndef _w32_draw_h_
#define _w32_draw_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	��v�G���g��
 */
void FASTCALL InitDraw(void);
										/* ������ */
void FASTCALL CleanDraw(void);
										/* �N���[���A�b�v */
BOOL FASTCALL SelectDraw(HWND hWnd);
										/* �Z���N�g */
void FASTCALL ModeDraw(HWND hWnd, BOOL bFullScreen);
										/* �`�惂�[�h�ύX */
void FASTCALL OnPaint(HWND hWnd);
										/* �ĕ`�� */
void FASTCALL OnDraw(HWND hWnd, HDC hDC);
										/* �����`�� */

/*
 *	��v���[�N
 */
extern BOOL bFullScreen;
										/* �t���X�N���[�� */
extern BOOL bFullScan;
										/* �t���X�L���� */
#ifdef __cplusplus
}
#endif

#endif	/* _w32_draw_h_ */
#endif	/* _WIN32 */
