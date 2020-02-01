/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API �R���g���[���o�[ ]
 */

#ifdef _WIN32

#ifndef _w32_bar_h_
#define _w32_bar_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	��v�G���g��
 */
HWND FASTCALL CreateStatus(HWND hWnd);
										/* �X�e�[�^�X�o�[�쐬 */
void FASTCALL DrawStatus(void);
										/* �`�� */
void FASTCALL PaintStatus(void);
										/* ���ׂčĕ`�� */
void FASTCALL SizeStatus(LONG cx);
										/* �T�C�Y�ύX */
void FASTCALL OwnerDrawStatus(DRAWITEMSTRUCT *pDI);
										/* �I�[�i�[�h���[ */
void FASTCALL OnMenuSelect(WPARAM wParam);
										/* WM_MENUSELECT */
void FASTCALL OnExitMenuLoop(void);
										/* WM_EXITMENULOOP */

/*
 *	��v���[�N
 */
extern HWND hStatusBar;
										/* �X�e�[�^�X�o�[ */
#ifdef __cplusplus
}
#endif

#endif	/* _w32_bar_h_ */
#endif	/* _WIN32 */

