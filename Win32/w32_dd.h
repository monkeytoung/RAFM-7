/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API DirectDraw ]
 */

#ifdef _WIN32

#ifndef _w32_dd_h_
#define _w32_dd_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	��v�G���g��
 */
void FASTCALL InitDD(void);
										/* ������ */
void FASTCALL CleanDD(void);
										/* �N���[���A�b�v */
BOOL FASTCALL SelectDD(void);
										/* �Z���N�g */
void FASTCALL DrawDD(void);
										/* �`�� */
void FASTCALL EnterMenuDD(HWND hWnd);
										/* ���j���[�J�n */
void FASTCALL ExitMenuDD(void);
										/* ���j���[�I�� */
void FASTCALL VramDD(WORD addr);
										/* VRAM�������ݒʒm */
void FASTCALL DigitalDD(void);
										/* TTL�p���b�g�ʒm */
void FASTCALL AnalogDD(void);
										/* �A�i���O�p���b�g�ʒm */
void FASTCALL ReDrawDD(void);
										/* �ĕ`��ʒm */

/*
 *	��v���[�N
 */
extern WORD rgbTTLDD[8];
										/* 640x200 �p���b�g */
extern WORD rgbAnalogDD[4096];
										/* 320x200 �p���b�g */
extern BOOL bDD480Line;
										/* 640x480 �D��t���O */
extern BOOL bDD480Status;
										/* 640x480 �X�e�[�^�X�t���O */
#ifdef __cplusplus
}
#endif

#endif	/* _w32_dd_h_ */
#endif	/* _WIN32 */
