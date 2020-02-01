/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API �E�C���h�E�\�� ]
 */

#ifdef _WIN32

#ifndef _w32_gdi_h_
#define _w32_gdi_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	��v�G���g��
 */
void FASTCALL InitGDI(void);
										/* ������ */
void FASTCALL CleanGDI(void);
										/* �N���[���A�b�v */
BOOL FASTCALL SelectGDI(HWND hWnd);
										/* �Z���N�g */
void FASTCALL DrawGDI(HWND hWnd, HDC hDC);
										/* �`�� */
void FASTCALL VramGDI(WORD addr);
										/* VRAM�������ݒʒm */
void FASTCALL DigitalGDI(void);
										/* TTL�p���b�g�ʒm */
void FASTCALL AnalogGDI(void);
										/* �A�i���O�p���b�g�ʒm */
void FASTCALL ReDrawGDI(void);
										/* �ĕ`��ʒm */

/*
 *	��v���[�N
 */
extern WORD rgbTTLGDI[8];
										/* �f�W�^���p���b�g */
extern WORD rgbAnalogGDI[4096];
										/* �A�i���O�p���b�g */
extern BYTE *pBitsGDI;
										/* �r�b�g�f�[�^ */
#ifdef __cplusplus
}
#endif

#endif	/* _w32_gdi_h_ */
#endif	/* _WIN32 */
