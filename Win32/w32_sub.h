/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API �T�u�E�C���h�E ]
 */

#ifdef _WIN32

#ifndef _w32_sub_h_
#define _w32_sub_h_

/*
 *	�T�u�E�C���h�E��`
 */
#define SWND_BREAKPOINT			0		/* �u���[�N�|�C���g */
#define SWND_SCHEDULER			1		/* �X�P�W���[�� */
#define SWND_CPUREG_MAIN		2		/* CPU���W�X�^ ���C�� */
#define SWND_CPUREG_SUB			3		/* CPU���W�X�^ �T�u */
#define SWND_DISASM_MAIN		4		/* �t�A�Z���u�� ���C�� */
#define SWND_DISASM_SUB			5		/* �t�A�Z���u�� �T�u */
#define SWND_MEMORY_MAIN		6		/* �������_���v ���C�� */
#define SWND_MEMORY_SUB			7		/* �������_���v �T�u */
#define SWND_FDC				8		/* FDC */
#define SWND_OPNREG				9		/* OPN���W�X�^ */
#define SWND_OPNDISP			10		/* OPN�f�B�X�v���C */
#define SWND_SUBCTRL			11		/* �T�uCPU�R���g���[�� */
#define SWND_KEYBOARD			12		/* �L�[�{�[�h */
#define SWND_MMR				13		/* MMR */
#define SWND_MAXNUM				14		/* �ő�T�u�E�C���h�E�� */

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	��v�G���g��
 */
HWND FASTCALL CreateBreakPoint(HWND hParent, int index);
										/* �u���[�N�|�C���g�E�C���h�E �쐬 */
void FASTCALL RefreshBreakPoint(void);
										/* �u���[�N�|�C���g�E�C���h�E ���t���b�V�� */
HWND FASTCALL CreateScheduler(HWND hParent, int index);
										/* �X�P�W���[���E�C���h�E �쐬 */
void FASTCALL RefreshScheduler(void);
										/* �X�P�W���[���E�C���h�E ���t���b�V�� */
HWND FASTCALL CreateCPURegister(HWND hParent, BOOL bMain, int index);
										/* CPU���W�X�^�E�C���h�E �쐬 */
void FASTCALL RefreshCPURegister(void);
										/* CPU���W�X�^�E�C���h�E ���t���b�V�� */
HWND FASTCALL CreateDisAsm(HWND hParent, BOOL bMain, int index);
										/* �t�A�Z���u���E�C���h�E �쐬 */
void FASTCALL RefreshDisAsm(void);
										/* �t�A�Z���u���E�C���h�E ���t���b�V�� */
void FASTCALL AddrDisAsm(BOOL bMain, WORD wAddr);
										/* �t�A�Z���u���E�C���h�E �A�h���X�w�� */
HWND FASTCALL CreateMemory(HWND hParent, BOOL bMain, int index);
										/* �������_���v�E�C���h�E �쐬 */
void FASTCALL RefreshMemory(void);
										/* �������_���v�E�C���h�E ���t���b�V�� */
void FASTCALL AddrMemory(BOOL bMain, WORD wAddr);
										/* �������_���v�E�C���h�E �A�h���X�w�� */

HWND FASTCALL CreateFDC(HWND hParent, int index);
										/* FDC�E�C���h�E �쐬 */
void FASTCALL RefreshFDC(void);
										/* FDC�E�C���h�E ���t���b�V�� */
HWND FASTCALL CreateOPNReg(HWND hParent, int index);
										/* OPN���W�X�^�E�C���h�E �쐬 */
void FASTCALL RefreshOPNReg(void);
										/* OPN���W�X�^�E�C���h�E ���t���b�V�� */
HWND FASTCALL CreateSubCtrl(HWND hParent, int index);
										/* �T�uCPU�R���g���[���E�C���h�E �쐬 */
void FASTCALL RefreshSubCtrl(void);
										/* �T�uCPU�R���g���[���E�C���h�E ���t���b�V�� */
HWND FASTCALL CreateOPNDisp(HWND hParent, int index);
										/* OPN�f�B�X�v���C�E�C���h�E �쐬 */
void FASTCALL RefreshOPNDisp(void);
										/* OPN�f�B�X�v���C�E�C���h�E ���t���b�V�� */
HWND FASTCALL CreateKeyboard(HWND hParent, int index);
										/* �L�[�{�[�h�E�C���h�E �쐬 */
void FASTCALL RefreshKeyboard(void);
										/* �L�[�{�[�h�E�C���h�E ���t���b�V�� */
HWND FASTCALL CreateMMR(HWND hParent, int index);
										/* MMR�E�C���h�E �쐬 */
void FASTCALL RefreshMMR(void);
										/* MMR�E�C���h�E ���t���b�V�� */

/*
 *	��v���[�N
 */
extern HWND hSubWnd[SWND_MAXNUM];
										/* �T�u�E�C���h�E */
#ifdef __cplusplus
}
#endif

#endif	/* _w32_sub_h_ */
#endif	/* _WIN32 */

