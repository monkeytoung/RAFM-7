/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API ]
 */

#ifdef _WIN32

#ifndef _w32_h_
#define _w32_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	��v�G���g��
 */
HFONT FASTCALL CreateTextFont(void);
										/* �e�L�X�g�t�H���g�쐬 */
void FASTCALL LockVM(void);
										/* VM���b�N */
void FASTCALL UnlockVM(void);
										/* VM�A�����b�N */
void FASTCALL OnCommand(HWND hWnd, WORD wID);
										/* WM_COMMAND */
void FASTCALL OnMenuPopup(HWND hWnd, HMENU hMenu, UINT uPos);
										/* WM_INITMENUPOPUP */
void FASTCALL OnDropFiles(HANDLE hDrop);
										/* WM_DROPFILES */
void FASTCALL OnCmdLine(LPTSTR lpCmdLine);
										/* �R�}���h���C�� */
void FASTCALL OnAbout(HWND hWnd);
										/* �o�[�W������� */

/*
 *	��v���[�N
 */
extern HINSTANCE hAppInstance;
										/* �A�v���P�[�V���� �C���X�^���X */
extern HWND hMainWnd;
										/* ���C���E�C���h�E */
extern HWND hDrawWnd;
										/* �`��E�C���h�E */
extern int nErrorCode;
										/* �G���[�R�[�h */
extern BOOL bMenuLoop;
										/* ���j���[���[�v�� */
extern BOOL bCloseReq;
										/* �I���v���t���O */
extern LONG lCharWidth;
										/* �L�����N�^���� */
extern LONG lCharHeight;
										/* �L�����N�^�c�� */
extern BOOL bSync;
										/* ���s�ɓ��� */
extern BOOL bActivate;
										/* �A�N�e�B�x�[�g�t���O */
#ifdef __cplusplus
}
#endif

#endif	/* _w32_h_ */
#endif	/* _WIN32 */
