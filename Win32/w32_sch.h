/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API �X�P�W���[�� ]
 */

#ifdef _WIN32

#ifndef _w32_sch_h_
#define _w32_sch_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	��v�G���g��
 */
void FASTCALL InitSch(void);
										/* ������ */
void FASTCALL CleanSch(void);
										/* �N���[���A�b�v */
BOOL FASTCALL SelectSch(void);
										/* �Z���N�g */
void FASTCALL ResetSch(void);
										/* ���s���Z�b�g */

/*
 *	��v���[�N
 */
extern DWORD dwExecTotal;
										/* ���s���ԃg�[�^�� */
extern BOOL bTapeFullSpeed;
										/* �e�[�v�������[�h */
#ifdef __cplusplus
}
#endif

#endif	/* _w32_sch_h_ */
#endif	/* _WIN32 */
