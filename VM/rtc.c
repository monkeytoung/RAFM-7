/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ 時計 ]
 */

#include <assert.h>
#include <time.h>
#include "xm7.h"
#include "event.h"
#include "rtc.h"
#include "device.h"

/*
 *	グローバル ワーク
 */
BYTE rtc_year;							/* 時計 年(00～99) */
BYTE rtc_month;							/* 時計 月(1～12) */
BYTE rtc_day;							/* 時計 日(0～31) */
BYTE rtc_week;							/* 時計 曜日(0～6) */
BYTE rtc_hour;							/* 時計 時(0～12 or 0～24h) */
BYTE rtc_minute;						/* 時計 分(0～59) */
BYTE rtc_second;						/* 時計 秒(0～59) */
BOOL rtc_24h;							/* 時計 12h/24h切り替え */
BOOL rtc_pm;							/* 時計 AM/PMフラグ */
BYTE rtc_leap;							/* 時計 閏年判定端数 */

/*
 *	月－日数対応テーブル
 */
static BYTE rtc_day_table[] = {			/* 2月は28日にセット */
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

/*
 *	プロトタイプ宣言
 */
static BOOL FASTCALL rtc_event(void);	/* 時計イベント */

/*
 *	時計
 *	初期化
 */
BOOL FASTCALL rtc_init(void)
{
	time_t ltime;
	struct tm *now;

	/* 時計をリセット。12h, 閏年0 */
	rtc_24h = FALSE;
	rtc_leap = 0;

	/* 現在の時間を読み取り、セット */
	ltime = time(NULL);
	now = localtime(&ltime);
	rtc_year = (BYTE)(now->tm_year % 100);
	rtc_month = (BYTE)(now->tm_mon + 1);
	rtc_day = (BYTE)now->tm_mday;
	rtc_week = (BYTE)now->tm_wday;
	rtc_hour = (BYTE)now->tm_hour;
	rtc_minute = (BYTE)now->tm_min;
	rtc_second = (BYTE)now->tm_sec;

	/* AM/PMフラグを設定 */
	if (rtc_hour >= 12) {
		rtc_hour = (BYTE)(rtc_hour - 12);
		rtc_pm = TRUE;
	}

	return TRUE;
}

/*
 *	時計
 *	クリーンアップ
 */
void FASTCALL rtc_cleanup(void)
{
}

/*
 *	時計
 *	リセット
 */
void FASTCALL rtc_reset(void)
{
	/* イベントを設定 */
	schedule_setevent(EVENT_RTC, 1000 * 1000, rtc_event);
}

/*
 *	時計
 *	イベント(1sec)
 */
static BOOL FASTCALL rtc_event(void)
{
	/* 秒アップ */
	rtc_second++;
	if (rtc_second < 60) {
		return TRUE;
	}
	rtc_second = 0;

	/* 分アップ */
	rtc_minute++;
	if (rtc_minute < 60) {
		return TRUE;
	}
	rtc_minute = 0;

	/* 時アップ */
	rtc_hour++;
	if (rtc_24h) {
		/* 24h */
		if (rtc_hour >= 12) {
			rtc_pm = TRUE;
		}
		else {
			rtc_pm = FALSE;
		}
		if (rtc_hour < 24) {
			return TRUE;
		}
	}
	else {
		/* 12h */
		if (rtc_pm) {
			/* PM */
			if (rtc_hour < 12) {
				return TRUE;
			}
		}
		else {
			/* AM */
			if (rtc_hour < 12) {
				return TRUE;
			}
			rtc_hour = 0;
			rtc_pm = TRUE;
			return TRUE;
		}
	}
	rtc_hour = 0;
	rtc_pm = FALSE;

	/* 曜日アップ */
	rtc_week++;
	if (rtc_week > 6) {
		rtc_week = 0;
	}

	/* 日アップ */
	rtc_day++;
	if (rtc_day <= rtc_day_table[rtc_month]) {
		return TRUE;
	}

	/* 閏年のチェックを、ここで入れる */
	if ((rtc_month == 2) && (rtc_day == 29)) {
		/* 2月29日なので、閏年ならreturn TRUE */
		if (rtc_leap == 0) {
			if ((rtc_year % 4) == 0) {
				return TRUE;
			}
		}
		else {
			if ((rtc_year % 4) == (4 - rtc_leap)) {
				return TRUE;
			}
		}
	}

	/* 月アップ */
	rtc_month++;
	if (rtc_month <= 12) {
		return TRUE;
	}
	rtc_month = 0;

	/* 年アップ */
	rtc_year++;
	if (rtc_year > 99) {
		rtc_year = 0;
	}

	return TRUE;
}

/*-[ I/O(キーボードコントローラより) ]--------------------------------------*/

/*
 *	時計
 *	データセット(書き込み)
 */
void FASTCALL rtc_set(BYTE *packet)
{
	BYTE dat;

	ASSERT(packet);

	/* バージョンチェック */
	if (fm7_ver < 2) {
		return;
	}

	/* 年 */
	dat = *packet++;
	rtc_year = (BYTE)((dat >> 4) * 10);
	rtc_year |= (BYTE)(dat & 0x0f);

	/* 月 */
	dat = *packet++;
	rtc_month = (BYTE)((dat >> 4) * 10);
	rtc_month |= (BYTE)(dat & 0x0f);

	/* 日 + 閏年 */
	dat = *packet++;
	rtc_day = (BYTE)(((dat & 0x30) >> 4) * 10);
	rtc_day |= (BYTE)(dat & 0x0f);

	/* 曜日,12/24,時の上位 */
	dat = *packet++;
	rtc_week = (BYTE)((dat >> 4) & 0x07);
	rtc_hour = (BYTE)((dat & 0x03) * 10);
	if (dat & 0x08) {
		rtc_24h = TRUE;
	}
	else {
		rtc_24h = FALSE;
	}
	if (dat & 0x04) {
		rtc_pm = TRUE;
	}
	else {
		rtc_pm = FALSE;
	}

	/* 時の下位、分の上位 */
	dat = *packet++;
	rtc_hour |= (BYTE)(dat >> 4);
	rtc_minute = (BYTE)((dat & 0x0f) * 10);

	/* 分の下位、秒の上位 */
	dat = *packet++;
	rtc_minute |= (BYTE)(dat >> 4);
	rtc_second = (BYTE)((dat & 0x0f) * 10);

	/* 秒の下位 */
	dat = *packet;
	rtc_second |= (BYTE)(dat >> 4);
}

/*
 *	時計
 *	データ取得(読み込み)
 */
void FASTCALL rtc_get(BYTE *packet)
{
	BYTE dat;

	ASSERT(packet);

	/* バージョンチェック */
	if (fm7_ver < 2) {
		return;
	}

	/* 年 */
	dat = (BYTE)((rtc_year / 10) << 4);
	dat |= (BYTE)(rtc_year % 10);
	*packet++ = dat;

	/* 月 */
	dat = (BYTE)((rtc_month / 10) << 4);
	dat |= (BYTE)(rtc_month % 10);
	*packet++ = dat;

	/* 日 + 閏年 */
	dat = (BYTE)((rtc_day / 10) << 4);
	dat |= (BYTE)(rtc_day % 10);
	dat |= (BYTE)(rtc_leap * 64);
	*packet++ = dat;

	/* 曜日,12/24,時の上位 */
	dat = (BYTE)(rtc_week << 4);
	dat |= (BYTE)(rtc_hour / 10);
	if (rtc_24h) {
		dat |= 0x08;
	}
	if (!rtc_24h && rtc_pm) {
		dat |= 0x04;
	}
	*packet++ = dat;

	/* 時の下位、分の上位 */
	dat = (BYTE)((rtc_hour % 10) << 4);
	dat |= (BYTE)(rtc_minute / 10);
	*packet++ = dat;

	/* 分の下位、秒の上位 */
	dat = (BYTE)((rtc_minute % 10) << 4);
	dat |= (BYTE)(rtc_second / 10);
	*packet++ = dat;

	/* 秒の下位 */
	dat = (BYTE)((rtc_second % 10) << 4);
	*packet = dat;
}

/*-[ ファイルI/O ]----------------------------------------------------------*/

/*
 *	時計
 *	セーブ
 */
BOOL FASTCALL rtc_save(int fileh)
{
	if (!file_bool_write(fileh, rtc_24h)) {
		return FALSE;
	}

	if (!file_bool_write(fileh, rtc_pm)) {
		return FALSE;
	}

	if (!file_byte_write(fileh, rtc_leap)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	時計
 *	ロード
 */
BOOL FASTCALL rtc_load(int fileh, int ver)
{
	time_t ltime;
	struct tm *now;

	/* バージョンチェック */
	if (ver < 2) {
		return FALSE;
	}

	if (!file_bool_read(fileh, &rtc_24h)) {
		return FALSE;
	}

	if (!file_bool_read(fileh, &rtc_pm)) {
		return FALSE;
	}

	if (!file_byte_read(fileh, &rtc_leap)) {
		return FALSE;
	}

	/* 現在の時間を読み取り、セット */
	ltime = time(NULL);
	now = localtime(&ltime);
	rtc_year = (BYTE)(now->tm_year % 100);
	rtc_month = (BYTE)(now->tm_mon + 1);
	rtc_day = (BYTE)now->tm_mday;
	rtc_week = (BYTE)now->tm_wday;
	rtc_hour = (BYTE)now->tm_hour;
	rtc_minute = (BYTE)now->tm_min;
	rtc_second = (BYTE)now->tm_sec;

	/* AM/PMフラグを設定 */
	if ((rtc_hour >= 12) && (!rtc_24h)) {
		rtc_hour = (BYTE)(rtc_hour - 12);
		rtc_pm = TRUE;
	}

	/* イベント */
	schedule_handle(EVENT_RTC, rtc_event);

	return TRUE;
}
