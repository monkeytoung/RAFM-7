/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API �L�[�{�[�h ]
 */

#ifdef _WIN32

#ifndef _w32_kbd_h_
#define _w32_kbd_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	��v�G���g��
 */
void FASTCALL InitKbd(void);
										/* ������ */
void FASTCALL CleanKbd(void);
										/* �N���[���A�b�v */
BOOL FASTCALL SelectKbd(HWND hWnd);
										/* �Z���N�g */
void FASTCALL PollKbd(void);
										/* �|�[�����O */
void FASTCALL PollJoy(void);
										/* �|�[�����O */
void FASTCALL GetDefMapKbd(BYTE *pMap, int mode);
										/* �f�t�H���g�}�b�v�擾 */
void FASTCALL SetMapKbd(BYTE *pMap);
										/* �}�b�v�ݒ� */
BOOL FASTCALL GetKbd(BYTE *pBuf);
										/* �|�[�����O���L�[���擾 */
/*
 *	��v���[�N
 */
extern int nJoyType[2];
										/* �W���C�X�e�B�b�N�^�C�v */
extern int nJoyRapid[2][2];
										/* �A�˃^�C�v */
extern int nJoyCode[2][7];
										/* �����R�[�h */
#ifdef __cplusplus
}
#endif

#endif	/* _w32_kbd_h_ */
#endif	/* _WIN32 */
