/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ スケジューラ ]
 */

#include <string.h>
#include <assert.h>
#include "xm7.h"
#include "subctrl.h"
#include "display.h"
#include "mmr.h"
#include "device.h"

/*
 *	グローバル ワーク
 */
BOOL run_flag;							/* 動作中フラグ */
BOOL stopreq_flag;						/* 停止要求フラグ */
breakp_t breakp[BREAKP_MAXNUM];			/* ブレークポイント データ */
event_t event[EVENT_MAXNUM];			/* イベント データ */
DWORD main_speed;						/* メインCPUスピード */
DWORD mmr_speed;						/* メイン(MMR)スピード */
BOOL cycle_steel;						/* サイクルスチールフラグ */

/*
 *	プロトタイプ宣言
 */
static BOOL FASTCALL schedule_runbreak(void);

/*
 *	スケジューラ
 *	初期化
 */
BOOL FASTCALL schedule_init(void)
{
	run_flag = FALSE;
	stopreq_flag = FALSE;
	memset(breakp, 0, sizeof(breakp));
	memset(event, 0, sizeof(event));

	/* CPU速度初期設定 */
	main_speed = 17520;
	mmr_speed = 14660;
	cycle_steel = TRUE;

	return TRUE;
}

/*
 *	スケジューラ
 *	クリーンアップ
 */
void FASTCALL schedule_cleanup(void)
{
}

/*
 *	スケジューラ
 *	リセット
 */
void FASTCALL schedule_reset(void)
{
	/* 全てのスケジュールをクリア */
	memset(event, 0, sizeof(event));

	/* ブレークポイント再設定 */
	schedule_runbreak();
}

/*-[ イベント ]-------------------------------------------------------------*/

/*
 *	スケジューラ
 *	イベント設定
 */
BOOL FASTCALL schedule_setevent(int id, DWORD microsec, BOOL FASTCALL (*func)(void))
{
	DWORD exec;

	ASSERT((id >= 0) && (id < EVENT_MAXNUM));
	ASSERT(func);

	if ((id < 0) || (id >= EVENT_MAXNUM)) {
		return FALSE;
	}
	if (microsec == 0) {
		event[id].flag = EVENT_NOTUSE;
		return FALSE;
	}

	/* 登録 */
	event[id].current = microsec;
	event[id].reload = microsec;
	event[id].callback = func;
	event[id].flag = EVENT_ENABLED;

	/* 実行中なら、時間を足しておく必要がある(後で引くため) */
	if (run_flag) {
		if (mmr_flag || twr_flag) {
			exec = (DWORD)maincpu.total;
			exec *= 10000;
			exec /= mmr_speed;
			event[id].current += exec;
		}
		else {
			exec = (DWORD)maincpu.total;
			exec *= 10000;
			exec /= main_speed;
			event[id].current += exec;
		}
	}

	return TRUE;
}

/*
 *	スケジューラ
 *	イベント削除
 */
BOOL FASTCALL schedule_delevent(int id)
{
	ASSERT((id >= 0) && (id < EVENT_MAXNUM));

	if ((id < 0) || (id >= EVENT_MAXNUM)) {
		return FALSE;
	}

	/* 未使用に */
	event[id].flag = EVENT_NOTUSE;

	return TRUE;
}

/*
 *	スケジューラ
 *	イベントハンドラ設定
 */
void FASTCALL schedule_handle(int id, BOOL FASTCALL (*func)(void))
{
	ASSERT((id >= 0) && (id < EVENT_MAXNUM));
	ASSERT(func);

	/* コールバック関数を登録するのみ。それ以外は触らない */
	event[id].callback = func;
}

/*
 *	スケジューラ
 *	最短実行時間調査
 */
static DWORD FASTCALL schedule_chkevent(DWORD microsec)
{
	DWORD exectime;
	int i;

	/* 初期設定 */
	exectime = microsec;

	/* イベントを回って調査 */
	for (i=0; i<EVENT_MAXNUM; i++) {
		if (event[i].flag == EVENT_NOTUSE) {
			continue;
		}

		ASSERT(event[i].current > 0);
		ASSERT(event[i].reload > 0);

		if (event[i].current < exectime) {
			exectime = event[i].current;
		}
	}

	return exectime;
}

/*
 *	スケジューラ
 *	進行処理
 */
static void FASTCALL schedule_doevent(DWORD microsec)
{
	int i;

	for (i=0; i<EVENT_MAXNUM; i++) {
		if (event[i].flag == EVENT_NOTUSE) {
			continue;
		}

		ASSERT(event[i].current > 0);
		ASSERT(event[i].reload > 0);

		/* 実行時間を引き */
		if (event[i].current < microsec) {
			event[i].current = 0;
		}
		else {
			event[i].current -= microsec;
		}

		/* カウンタが0なら */
		if (event[i].current == 0) {
			/* 時間はENABLE,DISABLEにかかわらずリロード */
			event[i].current = event[i].reload;
			/* コールバック実行 */
			if (event[i].flag == EVENT_ENABLED) {
				if (!event[i].callback()) {
					event[i].flag = EVENT_DISABLED;
				}
			}
		}
	}
}

/*-[ ブレークポイント ]-----------------------------------------------------*/

/*
 *	スケジューラ
 *	ブレークポイントセット(すでにセットしてあれば消去)
 */
BOOL FASTCALL schedule_setbreak(int cpu, WORD addr)
{
	int i;

	/* まず、全てのブレークポイントを検索し、見つかるか */
	for (i=0; i<BREAKP_MAXNUM; i++) {
		if (breakp[i].flag != BREAKP_NOTUSE) {
			if ((breakp[i].cpu == cpu) && (breakp[i].addr == addr)) {
				break;
			}
		}
	}
	/* 見つかれば、削除 */
	if (i != BREAKP_MAXNUM) {
		breakp[i].flag = BREAKP_NOTUSE;
		return TRUE;
	}	

	/* 空きを調査 */
	for (i=0; i<BREAKP_MAXNUM; i++) {
		if (breakp[i].flag == BREAKP_NOTUSE) {
			break;
		}
	}
	/* すべて埋まっているか */
	if (i == BREAKP_MAXNUM) {
		return FALSE;
	}

	/* セット */
	breakp[i].flag = BREAKP_ENABLED;
	breakp[i].cpu = cpu;
	breakp[i].addr = addr;

	return TRUE;
}

/*
 *	スケジューラ
 *	Stopフラグクリア
 */
static BOOL FASTCALL schedule_runbreak(void)
{
	int i;
	BOOL flag;

	/* 停止中なら、有効に */
	for (i=0; i<BREAKP_MAXNUM; i++) {
		if (breakp[i].flag == BREAKP_STOPPED) {
			breakp[i].flag = BREAKP_ENABLED;
		}
	}

	/* １つでも有効があれば */
	flag = FALSE;
	for (i=0; i<BREAKP_MAXNUM; i++) {
		if (breakp[i].flag == BREAKP_ENABLED) {
			flag = TRUE;
		}
	}

	return flag;
}

/*
 *	スケジューラ
 *	ブレークチェック
 */
static BOOL FASTCALL schedule_chkbreak(void)
{
	int i;

	for (i=0; i<BREAKP_MAXNUM; i++) {
		if (breakp[i].flag == BREAKP_ENABLED) {
			if (breakp[i].cpu == MAINCPU) {
				if (breakp[i].addr == maincpu.pc) {
					return TRUE;
				}
			}
			else {
				if (breakp[i].addr == subcpu.pc) {
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

/*
 *	スケジューラ
 *	Stopフラグ設定
 */
static BOOL FASTCALL schedule_stopbreak(void)
{
	int i;
	int ret;

	/* フラグを落としておく */
	ret = FALSE;

	for (i=0; i<BREAKP_MAXNUM; i++) {
		if (breakp[i].flag == BREAKP_ENABLED) {
			if (breakp[i].cpu == MAINCPU) {
				if (breakp[i].addr == maincpu.pc) {
					breakp[i].flag = BREAKP_STOPPED;
					ret = TRUE;
				}
			}
			else {
				if (breakp[i].addr == subcpu.pc) {
					breakp[i].flag = BREAKP_STOPPED;
					ret = TRUE;
				}
			}
		}
	}

	return ret;
}

/*-[ 実行部 ]---------------------------------------------------------------*/

/*
 *	トレース
 */
void FASTCALL schedule_trace(void)
{
	schedule_runbreak();

	/* １命令実行 */
	maincpu_execline();
	if (!subhalt_flag && (cycle_steel || !vrama_flag || blank_flag)) {
		subcpu_execline();
	}

	schedule_stopbreak();
}

/*
 *	実行
 */
DWORD FASTCALL schedule_exec(DWORD microsec)
{
	DWORD exec;
	DWORD count;
	WORD main;

	ASSERT(run_flag);
	ASSERT(!stopreq_flag);

	/* 最短の実行時間を得る */
	exec = schedule_chkevent(microsec);

	/* CPU時間に換算 */
	if (mmr_flag || twr_flag) {
		count = mmr_speed;
		count *= exec;
		count /= 10000;
	}
	else {
		count = main_speed;
		count *= exec;
		count /= 10000;
	}
	main = (WORD)count;

	/* カウンタクリア */
	maincpu.total = 0;
	subcpu.total = 0;

	if (cycle_steel) {
		if (schedule_runbreak()) {
			/* ブレークポイントあり */
			schedule_stopbreak();

			/* 実行 */
			while (maincpu.total < main) {
				if (schedule_chkbreak()) {
					stopreq_flag = TRUE;
				}
				if (stopreq_flag) {
					break;
				}

				/* メインCPU実行 */
				maincpu_exec();

				/* サブCPU実行 */
				if (!subhalt_flag) {
					subcpu_exec();
				}
			}

			/* ブレークした場合の処理 */
			schedule_stopbreak();
			if (stopreq_flag == TRUE) {
				/* exec分時間を進める(あえて補正しない) */
				run_flag = FALSE;
			}

			/* イベント処理 */
			maincpu.total = 0;
			subcpu.total = 0;
			schedule_doevent(exec);
		}
		else {
			/* ブレークポイントなし */
			/* 実行 */
			while (maincpu.total < main) {
				/* メインCPU実行 */
				maincpu_exec();

				/* サブCPU実行 */
				if (!subhalt_flag) {
					subcpu_exec();
				}
			}

			/* イベント処理 */
			maincpu.total = 0;
			subcpu.total = 0;
			schedule_doevent(exec);
		}
	}
	else {
		if (schedule_runbreak()) {
			/* ブレークポイントあり */
			schedule_stopbreak();

			/* 実行 */
			while (maincpu.total < main) {
				if (schedule_chkbreak()) {
					stopreq_flag = TRUE;
				}
				if (stopreq_flag) {
					break;
				}

				/* メインCPU実行 */
				maincpu_exec();

				/* サブCPU実行 */
				if (!subhalt_flag && (!vrama_flag || blank_flag)) {
					subcpu_exec();
				}
			}

			/* ブレークした場合の処理 */
			schedule_stopbreak();
			if (stopreq_flag == TRUE) {
				/* exec分時間を進める(あえて補正しない) */
				run_flag = FALSE;
			}

			/* イベント処理 */
			maincpu.total = 0;
			subcpu.total = 0;
			schedule_doevent(exec);
		}
		else {
			/* ブレークポイントなし */
			/* 実行 */
			while (maincpu.total < main) {
				/* メインCPU実行 */
				maincpu_exec();

				/* サブCPU実行 */
				if (!subhalt_flag && (!vrama_flag || blank_flag)) {
					subcpu_exec();
				}
			}

			/* イベント処理 */
			maincpu.total = 0;
			subcpu.total = 0;
			schedule_doevent(exec);
		}
	}

	return exec;
}

/*-[ ファイルI/O ]----------------------------------------------------------*/

/*
 *	スケジューラ
 *	セーブ
 */
BOOL FASTCALL schedule_save(int fileh)
{
	int i;

	if (!file_bool_write(fileh, run_flag)) {
		return FALSE;
	}

	if (!file_bool_write(fileh, stopreq_flag)) {
		return FALSE;
	}

	/* ブレークポイント */
	for (i=0; i<BREAKP_MAXNUM; i++) {
		if (!file_byte_write(fileh, (BYTE)breakp[i].flag)) {
			return FALSE;
		}
		if (!file_byte_write(fileh, (BYTE)breakp[i].cpu)) {
			return FALSE;
		}
		if (!file_word_write(fileh, breakp[i].addr)) {
			return FALSE;
		}
	}

	/* イベント */
	for (i=0; i<EVENT_MAXNUM; i++) {
		/* コールバック以外を保存 */
		if (!file_byte_write(fileh, (BYTE)event[i].flag)) {
			return FALSE;
		}
		if (!file_dword_write(fileh, event[i].current)) {
			return FALSE;
		}
		if (!file_dword_write(fileh, event[i].reload)) {
			return FALSE;
		}
	}

	return TRUE;
}

/*
 *	スケジューラ
 *	ロード
 */
BOOL FASTCALL schedule_load(int fileh, int ver)
{
	int i;
	BYTE tmp;

	/* バージョンチェック */
	if (ver < 2) {
		return FALSE;
	}

	if (!file_bool_read(fileh, &run_flag)) {
		return FALSE;
	}

	if (!file_bool_read(fileh, &stopreq_flag)) {
		return FALSE;
	}

	/* ブレークポイント */
	for (i=0; i<BREAKP_MAXNUM; i++) {
		if (!file_byte_read(fileh, &tmp)) {
			return FALSE;
		}
		breakp[i].flag = (int)tmp;
		if (!file_byte_read(fileh, &tmp)) {
			return FALSE;
		}
		breakp[i].cpu = (int)tmp;
		if (!file_word_read(fileh, &breakp[i].addr)) {
			return FALSE;
		}
	}

	/* イベント */
	for (i=0; i<EVENT_MAXNUM; i++) {
		/* コールバック以外を設定 */
		if (!file_byte_read(fileh, &tmp)) {
			return FALSE;
		}
		event[i].flag = (int)tmp;
		if (!file_dword_read(fileh, &event[i].current)) {
			return FALSE;
		}
		if (!file_dword_read(fileh, &event[i].reload)) {
			return FALSE;
		}
	}

	return TRUE;
}
