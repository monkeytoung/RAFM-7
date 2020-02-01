/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API �T�E���h ]
 */

#ifdef _WIN32

#ifndef _w32_snd_h_
#define _w32_snd_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	��v�G���g��
 */
void FASTCALL InitSnd(void);
										/* ������ */
void FASTCALL CleanSnd(void);
										/* �N���[���A�b�v */
BOOL FASTCALL SelectSnd(HWND hWnd);
										/* �Z���N�g */
void FASTCALL ApplySnd(void);
										/* �K�p */
void FASTCALL PlaySnd(void);
										/* ���t�J�n */
void FASTCALL StopSnd(void);
										/* ���t��~ */
void FASTCALL ProcessSnd(BOOL bZero);
										/* �o�b�t�@�[�U�莞���� */
int FASTCALL GetLevelSnd(int ch);
										/* �T�E���h���x���擾 */
void FASTCALL OpenCaptureSnd(char *fname);
										/* WAV�L���v�`���J�n */
void FASTCALL CloseCaptureSnd(void);
										/* WAV�L���v�`���I�� */

/*
 *	��v���[�N
 */
extern UINT nSampleRate;
										/* �T���v�����[�g(Hz�A0�Ŗ���) */
extern UINT nSoundBuffer;
										/* �T�E���h�o�b�t�@(�_�u���Ams) */
extern UINT nBeepFreq;
										/* BEEP���g��(Hz) */
extern int hWavCapture;
										/* WAV�L���v�`���t�@�C���n���h�� */
extern BOOL bWavCapture;
										/* WAV�L���v�`���J�n�� */
#ifdef __cplusplus
}
#endif

#endif	/* _w32_snd_h_ */
#endif	/* _WIN32 */
