/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API �X�P�W���[�� ]
 */

#ifdef _WIN32

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <assert.h>
#include "xm7.h"
#include "tapelp.h"
#include "device.h"
#include "w32.h"
#include "w32_sch.h"
#include "w32_draw.h"
#include "w32_snd.h"
#include "w32_kbd.h"
#include "w32_bar.h"
#include "w32_sub.h"

/*
 *	�O���[�o�� ���[�N
 */
DWORD dwExecTotal;						/* ���s�g�[�^������(us) */
BOOL bTapeFullSpeed;					/* �e�[�v�������[�h�t���O */

/*
 *	�X�^�e�B�b�N ���[�N
 */
static HANDLE hThread;					/* �X���b�h�n���h�� */
static DWORD dwThResult;				/* �X���b�h�߂�l */
static BOOL bDrawVsync;					/* VSYNC�t���O */
static DWORD dwExecTime;				/* ���s����(ms) */
static int nFrameSkip;					/* �t���[���X�L�b�v��(ms) */

/*
 *	�v���g�^�C�v�錾
 */
static DWORD WINAPI ThreadSch(LPVOID);			/* �X���b�h�֐� */

/*
 *	������
 */
void FASTCALL InitSch(void)
{
	/* ���[�N�G���A������ */
	hThread = NULL;
	dwThResult = 0;
	bDrawVsync = TRUE;

	/* �O���[�o�����[�N */
	dwExecTotal = 0;
	bTapeFullSpeed = FALSE;

	return;
}

/*
 *	�N���[���A�b�v
 */
void FASTCALL CleanSch(void)
{
	/* �X���b�h������I�����Ă��Ȃ���΁A�I��点�� */
	if (hThread && !dwThResult) {
		bCloseReq = TRUE;
		WaitForSingleObject(hThread, INFINITE);
	}
}

/*
 *	�Z���N�g
 */
BOOL FASTCALL SelectSch(void)
{
	/* �n�C�v���C�I���e�B�ŃX���b�h�N�� */
	hThread = CreateThread(NULL, 0, ThreadSch, 0, 0, &dwThResult);
	if (hThread == NULL) {
		return FALSE;
	}
	SetPriorityClass(hThread, HIGH_PRIORITY_CLASS);

	return TRUE;
}

/*
 *	VSYNC�ʒm
 */
void FASTCALL vsync_notify(void)
{
	bDrawVsync = TRUE;
}

/*
 *	1ms���s
 */
void FASTCALL ExecSch(void)
{
	DWORD dwCount;

	/* �|�[�����O */
	PollKbd();
	PollJoy();

	/* �T�E���h */
	ProcessSnd(FALSE);

	dwCount = 1000;
	while (dwCount > 0) {
		/* ���~�v�����オ���Ă���΁A�����Ƀ��^�[�� */
		if (stopreq_flag) {
			run_flag = FALSE;
			break;
		}

		/* �����Ŏ��s */
		dwCount -= schedule_exec(dwCount);
	}

	/* �g�[�^���^�C������ */
	dwExecTotal += (1000 - dwCount);
}

/*
 *	�`��
 */
static void FASTCALL DrawSch(void)
{
	HDC hDC;

	/* �h���[�E�C���h�E */
	hDC = GetDC(hDrawWnd);
	OnDraw(hDrawWnd, hDC);
	ReleaseDC(hDrawWnd, hDC);

	/* �T�u�E�C���h�E(Sync���̂�) */
	if (bSync) {
		RefreshBreakPoint();
		RefreshScheduler();
		RefreshCPURegister();
		AddrDisAsm(TRUE, maincpu.pc);
		AddrDisAsm(FALSE, subcpu.pc);
		RefreshDisAsm();
		RefreshMemory();

		RefreshFDC();
		RefreshOPNReg();
		RefreshOPNDisp();
		RefreshSubCtrl();
		RefreshKeyboard();
		RefreshMMR();
	}
}

/*
 *	���s���Z�b�g
 *	��VM�̃��b�N�͍s���Ă��Ȃ��̂Œ���
 */
void FASTCALL ResetSch(void)
{
	nFrameSkip = 0;
	dwExecTime = timeGetTime();
}

/*
 *	�X���b�h�֐�
 */
static DWORD WINAPI ThreadSch(LPVOID param)
{
	DWORD dwTempTime;

	/* ������ */
	ResetSch();

	/* �������[�v(�N���[�Y�w��������ΏI��) */
	while (!bCloseReq) {
		/* �����Ȃ胍�b�N */
		LockVM();

		/* ���s�w�����Ȃ���΁A�X���[�v */
		if (!run_flag) {
			/* ����������ăX���[�v */
			ProcessSnd(TRUE);
			UnlockVM();
			Sleep(10);
			ResetSch();
			continue;
		}

		/* ���Ԃ��擾(49���ł̃��[�v���l��) */
		dwTempTime = timeGetTime();
		if (dwTempTime < dwExecTime) {
			dwExecTime = 0;
		}

		/* ���Ԃ��r */
		if (dwTempTime <= dwExecTime) {
			/* ���Ԃ��]���Ă��邪�A�`��ł��邩 */
			if (bDrawVsync) {
				DrawSch();
				nFrameSkip = 0;
				bDrawVsync = FALSE;
			}

			/* �ēx�A���Ԃ��擾(49���ł̃��[�v���l��) */
			dwTempTime = timeGetTime();
			if (dwTempTime < dwExecTime) {
				dwExecTime = 0;
			}
			if (dwTempTime > dwExecTime) {
				UnlockVM();
				continue;
			}

			/* ���Ԃɗ]�T������̂ŁA�e�[�v�������[�h���� */
			if (!tape_motor || !bTapeFullSpeed) {
				Sleep(1);
				UnlockVM();
				continue;
			}

			/* �e�[�v�������[�h */
			dwExecTime = dwTempTime - 1;
			if (dwExecTime > dwTempTime) {
				dwExecTime++;
			}
		}

		/* ���s */
		ExecSch();
		nFrameSkip++;
		dwExecTime++;

		/* �I���΍�ŁA�����Ŕ����� */
		if (bCloseReq) {
			UnlockVM();
			break;
		}

		/* Break�΍� */
		if (!run_flag) {
			DrawSch();
			bDrawVsync = FALSE;
			nFrameSkip = 0;
			UnlockVM();
			continue;
		}

		/* �X�L�b�v�J�E���^��500��(500ms)�ȉ��Ȃ�A�����Ď��s */
		if (nFrameSkip < 500) {
			UnlockVM();
			continue;
		}

		/* ���`�悪�����Ă���̂ŁA�����ň��`�� */
		DrawSch();
		ResetSch();
		bDrawVsync = FALSE;
		UnlockVM();
	}

	/* �X���b�h�I�� */
	ExitThread(TRUE);

	return TRUE;
}

#endif	/* _WIN32 */

