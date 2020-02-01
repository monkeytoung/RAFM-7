/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API スケジューラ ]
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
 *	グローバル ワーク
 */
DWORD dwExecTotal;						/* 実行トータル時間(us) */
BOOL bTapeFullSpeed;					/* テープ高速モードフラグ */

/*
 *	スタティック ワーク
 */
static HANDLE hThread;					/* スレッドハンドル */
static DWORD dwThResult;				/* スレッド戻り値 */
static BOOL bDrawVsync;					/* VSYNCフラグ */
static DWORD dwExecTime;				/* 実行時間(ms) */
static int nFrameSkip;					/* フレームスキップ数(ms) */

/*
 *	プロトタイプ宣言
 */
static DWORD WINAPI ThreadSch(LPVOID);			/* スレッド関数 */

/*
 *	初期化
 */
void FASTCALL InitSch(void)
{
	/* ワークエリア初期化 */
	hThread = NULL;
	dwThResult = 0;
	bDrawVsync = TRUE;

	/* グローバルワーク */
	dwExecTotal = 0;
	bTapeFullSpeed = FALSE;

	return;
}

/*
 *	クリーンアップ
 */
void FASTCALL CleanSch(void)
{
	/* スレッドが万一終了していなければ、終わらせる */
	if (hThread && !dwThResult) {
		bCloseReq = TRUE;
		WaitForSingleObject(hThread, INFINITE);
	}
}

/*
 *	セレクト
 */
BOOL FASTCALL SelectSch(void)
{
	/* ハイプライオリティでスレッド起動 */
	hThread = CreateThread(NULL, 0, ThreadSch, 0, 0, &dwThResult);
	if (hThread == NULL) {
		return FALSE;
	}
	SetPriorityClass(hThread, HIGH_PRIORITY_CLASS);

	return TRUE;
}

/*
 *	VSYNC通知
 */
void FASTCALL vsync_notify(void)
{
	bDrawVsync = TRUE;
}

/*
 *	1ms実行
 */
void FASTCALL ExecSch(void)
{
	DWORD dwCount;

	/* ポーリング */
	PollKbd();
	PollJoy();

	/* サウンド */
	ProcessSnd(FALSE);

	dwCount = 1000;
	while (dwCount > 0) {
		/* 中止要求が上がっていれば、即座にリターン */
		if (stopreq_flag) {
			run_flag = FALSE;
			break;
		}

		/* ここで実行 */
		dwCount -= schedule_exec(dwCount);
	}

	/* トータルタイム増加 */
	dwExecTotal += (1000 - dwCount);
}

/*
 *	描画
 */
static void FASTCALL DrawSch(void)
{
	HDC hDC;

	/* ドローウインドウ */
	hDC = GetDC(hDrawWnd);
	OnDraw(hDrawWnd, hDC);
	ReleaseDC(hDrawWnd, hDC);

	/* サブウインドウ(Sync時のみ) */
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
 *	実行リセット
 *	※VMのロックは行っていないので注意
 */
void FASTCALL ResetSch(void)
{
	nFrameSkip = 0;
	dwExecTime = timeGetTime();
}

/*
 *	スレッド関数
 */
static DWORD WINAPI ThreadSch(LPVOID param)
{
	DWORD dwTempTime;

	/* 初期化 */
	ResetSch();

	/* 無限ループ(クローズ指示があれば終了) */
	while (!bCloseReq) {
		/* いきなりロック */
		LockVM();

		/* 実行指示がなければ、スリープ */
		if (!run_flag) {
			/* 無音を作ってスリープ */
			ProcessSnd(TRUE);
			UnlockVM();
			Sleep(10);
			ResetSch();
			continue;
		}

		/* 時間を取得(49日でのループを考慮) */
		dwTempTime = timeGetTime();
		if (dwTempTime < dwExecTime) {
			dwExecTime = 0;
		}

		/* 時間を比較 */
		if (dwTempTime <= dwExecTime) {
			/* 時間が余っているが、描画できるか */
			if (bDrawVsync) {
				DrawSch();
				nFrameSkip = 0;
				bDrawVsync = FALSE;
			}

			/* 再度、時間を取得(49日でのループを考慮) */
			dwTempTime = timeGetTime();
			if (dwTempTime < dwExecTime) {
				dwExecTime = 0;
			}
			if (dwTempTime > dwExecTime) {
				UnlockVM();
				continue;
			}

			/* 時間に余裕があるので、テープ高速モード判定 */
			if (!tape_motor || !bTapeFullSpeed) {
				Sleep(1);
				UnlockVM();
				continue;
			}

			/* テープ高速モード */
			dwExecTime = dwTempTime - 1;
			if (dwExecTime > dwTempTime) {
				dwExecTime++;
			}
		}

		/* 実行 */
		ExecSch();
		nFrameSkip++;
		dwExecTime++;

		/* 終了対策で、ここで抜ける */
		if (bCloseReq) {
			UnlockVM();
			break;
		}

		/* Break対策 */
		if (!run_flag) {
			DrawSch();
			bDrawVsync = FALSE;
			nFrameSkip = 0;
			UnlockVM();
			continue;
		}

		/* スキップカウンタが500回(500ms)以下なら、続けて実行 */
		if (nFrameSkip < 500) {
			UnlockVM();
			continue;
		}

		/* 無描画が続いているので、ここで一回描画 */
		DrawSch();
		ResetSch();
		bDrawVsync = FALSE;
		UnlockVM();
	}

	/* スレッド終了 */
	ExitThread(TRUE);

	return TRUE;
}

#endif	/* _WIN32 */

