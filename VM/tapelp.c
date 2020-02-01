/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ カセットテープ＆プリンタ ]
 */

#include <assert.h>
#include <string.h>
#include "xm7.h"
#include "tapelp.h"
#include "mainetc.h"
#include "device.h"

/*
 *	グローバル ワーク
 */
BOOL tape_in;							/* テープ 入力データ */
BOOL tape_out;							/* テープ 出力データ */
BOOL tape_motor;						/* テープ モータ */
BOOL tape_rec;							/* テープ RECフラグ */
BOOL tape_writep;						/* テープ 書き込み禁止 */
WORD tape_count;						/* テープ サイクルカウンタ */
BYTE tape_subcnt;						/* テープ サブカウンタ */
int tape_fileh;							/* テープ ファイルハンドル */
DWORD tape_offset;						/* テープ ファイルオフセット */
char tape_fname[128+1];					/* テープ ファイルネーム */

WORD tape_incnt;						/* テープ 読み込みカウンタ */
DWORD tape_fsize;						/* テープ ファイルサイズ */

BYTE lp_data;							/* プリンタ 出力データ */
BOOL lp_busy;							/* プリンタ BUSYフラグ */
BOOL lp_error;							/* プリンタ エラーフラグ */
BOOL lp_pe;								/* プリンタ PEフラグ */
BOOL lp_ackng;							/* プリンタ ACKフラグ */
BOOL lp_online;							/* プリンタ オンライン */
BOOL lp_strobe;							/* プリンタ ストローブ */
int lp_fileh;							/* プリンタ ファイルハンドル */

char lp_fname[128+1];					/* プリンタ ファイルネーム */

/*
 *	カセットテープ＆プリンタ
 *	初期化
 */
BOOL FASTCALL tapelp_init(void)
{
	/* テープ */
	tape_fileh = -1;
	tape_fname[0] = '\0';
	tape_offset = 0;
	tape_fsize = 0;
	tape_writep = FALSE;

	/* プリンタ */
	lp_fileh = -1;
	lp_fname[0] = '\0';

	return TRUE;
}

/*
 *	カセットテープ＆プリンタ
 *	クリーンアップ
 */
void FASTCALL tapelp_cleanup(void)
{
	/* ファイルを開いていれば、閉じる */
	if (tape_fileh != -1) {
		file_close(tape_fileh);
		tape_fileh = -1;
	}

	/* モータOFF */
	tape_motor = FALSE;

	/* ファイルを開いていれば、閉じる */
	if (lp_fileh != -1) {
		file_close(lp_fileh);
		lp_fileh = -1;
	}
}

/*
 *	カセットテープ＆プリンタ
 *	リセット
 */
void FASTCALL tapelp_reset(void)
{
	tape_motor = FALSE;
	tape_rec = FALSE;
	tape_count = 0;
	tape_in = FALSE;
	tape_out = FALSE;
	tape_incnt = 0;
	tape_subcnt = 0;

	lp_busy = FALSE;
	lp_error = FALSE;
	lp_ackng = TRUE;
	lp_pe = FALSE;
	lp_online = FALSE;
	lp_strobe = FALSE;
}

/*-[ プリンタ ]-------------------------------------------------------------*/

/*
 *	プリンタ
 *	データ出力
 */
static void FASTCALL lp_output(BYTE dat)
{
	/* オープンしていなければ，開く */
	if (lp_fileh == -1) {
		if (lp_fname[0] != '\0') {
			lp_fileh = file_open(lp_fname, OPEN_W);
		}
	}

	/* オープンチェック */
	if (lp_fileh == -1) {
		return;
	}

	/* アペンド */
	file_write(lp_fileh, &dat, 1);
}

/*
 *	プリンタ
 *	ファイル名設定
 */
void FASTCALL lp_setfile(char *fname)
{
	/* 一度開いていれば、閉じる */
	if (lp_fileh != -1) {
		file_close(lp_fileh);
		lp_fileh = -1;
	}

	/* ファイル名セット */
	if (fname == NULL) {
		lp_fname[0] = '\0';
		return;
	}

	if (strlen(fname) < sizeof(lp_fname)) {
		strcpy(lp_fname, fname);
	}
	else {
		lp_fname[0] = '\0';
	}
}

/*-[ テープ ]---------------------------------------------------------------*/

/*
 *	テープ
 *	データ入力
 */
static void FASTCALL tape_input(void)
{
	BYTE high;
	BYTE low;
	WORD dat;

	/* モータが回っているか */
	if (tape_motor == FALSE) {
		return;
	}

	/* 録音されていれば入力できない */
	if (tape_rec) {
		return;
	}

	/* シングルカウンタが入力カウンタを越えていれば、0にする */
	while (tape_count >= tape_incnt) {
		tape_count -= tape_incnt;
		tape_incnt = 0;

		/* データフェッチ */
		tape_in = FALSE;

		if (tape_fileh == -1) {
			return;
		}

		if (tape_offset >= tape_fsize) {
			return;
		}

		if (!file_read(tape_fileh, &high, 1)) {
			return;
		}
		if (!file_read(tape_fileh, &low, 1)) {
			return;
		}

		/* オフセット更新 */
		tape_offset += 2;

		/* データ、カウンタ設定 */
		dat = (WORD)(high * 256 + low);
		if (dat > 0x7fff) {
			tape_in = TRUE;
		}
		tape_incnt = (WORD)(dat & 0x7fff);

		/* カウンタを先繰りする */
		if (tape_count > tape_incnt) {
			tape_count -= tape_incnt;
			tape_incnt = 0;
		}
		else {
			tape_incnt -= tape_count;
			tape_count = 0;
		}
	}
}

/*
 *	テープ
 *	データ出力
 */
static void FASTCALL tape_output(BOOL flag)
{
	WORD dat;
	BYTE high, low;

	/* テープが回っているか */
	if (tape_motor == FALSE) {
		return;
	}

	/* 録音中か */
	if (tape_rec == FALSE) {
		return;
	}

	/* カウンタが回っているか */
	if (tape_count == 0) {
		return;
	}

	/* 書き込み可能か */
	if (tape_writep) {
		return;
	}

	/* ファイルがオープンされていれば、データ書き込み */
	dat = tape_count;
	if (dat >= 0x8000) {
		dat = 0x7fff;
	}
	if (flag) {
		dat |= 0x8000;
	}
	high = (BYTE)(dat >> 8);
	low = (BYTE)(dat & 0xff);
	if (tape_fileh != -1) {
		if (file_write(tape_fileh, &high, 1)) {
			if (file_write(tape_fileh, &low, 1)) {
				tape_offset += 2;
				if (tape_offset >= tape_fsize) {
					tape_fsize = tape_offset;
				}
			}
		}
	}

	/* カウンタをリセット */
	tape_count = 0;
	tape_subcnt = 0;
}

/*
 *	テープ
 *	マーカ出力
 */
static void FASTCALL tape_mark(void)
{
	BYTE dat;

	/* テープが回っているか */
	if (tape_motor == FALSE) {
		return;
	}

	/* 録音中か */
	if (tape_rec == FALSE) {
		return;
	}

	/* 書き込み可能か */
	if (tape_writep) {
		return;
	}

	/* ファイルがオープンされていれば、データ書き込み */
	if (tape_fileh != -1) {
		dat = 0;
		if (file_write(tape_fileh, &dat, 1)) {
			if (file_write(tape_fileh, &dat, 1)) {
				tape_offset += 2;
				if (tape_offset >= tape_fsize) {
					tape_fsize = tape_offset;
				}
			}
		}
	}
}

/*
 *	テープ
 *	巻き戻し
 */
void FASTCALL tape_rew(void)
{
	WORD dat;

	/* 条件判定 */
	if (tape_fileh == -1) {
		return;
	}

	/* assert */
	ASSERT(tape_fsize >= 16);
	ASSERT(tape_offset >= 16);
	ASSERT(!(tape_fsize & 0x01));
	ASSERT(!(tape_offset & 0x01));

	while (tape_offset > 16) {
		/* ２バイト前に戻り、読み込み */
		tape_offset -= 2;
		if (!file_seek(tape_fileh, tape_offset)) {
			return;
		}
		file_read(tape_fileh, (BYTE *)&dat, 2);

		/* $0000なら、そこに設定 */
		if (dat == 0) {
			file_seek(tape_fileh, tape_offset);
			return;
		}

		/* いま読み込んだ分だけ戻す */
		if (!file_seek(tape_fileh, tape_offset)) {
			return;
		}
	}
}

/*
 *	テープ
 *	早送り
 */
void FASTCALL tape_ff(void)
{
	WORD dat;

	/* 条件判定 */
	if (tape_fileh == -1) {
		return;
	}

	/* assert */
	ASSERT(tape_fsize >= 16);
	ASSERT(tape_offset >= 16);
	ASSERT(!(tape_fsize & 0x01));
	ASSERT(!(tape_offset & 0x01));

	while (tape_offset < tape_fsize) {
		/* 先へ進める */
		tape_offset += 2;
		if (tape_offset >= tape_fsize){
			return;
		}
		if (!file_seek(tape_fileh, tape_offset)) {
			return;
		}
		file_read(tape_fileh, (BYTE *)&dat, 2);

		/* $0000なら、その次に設定 */
		if (dat == 0) {
			tape_offset += 2;
			if (tape_offset >= tape_fsize) {
				tape_fsize = tape_offset;
			}
			return;
		}
	}
}

/*
 *	テープ
 *	ファイル名設定
 */
void FASTCALL tape_setfile(char *fname)
{
	char *header = "XM7 TAPE IMAGE 0";
	char buf[17];

	/* 一度開いていれば、閉じる */
	if (tape_fileh != -1) {
		file_close(tape_fileh);
		tape_fileh = -1;
		tape_writep = FALSE;
	}

	/* ファイル名セット */
	if (fname == NULL) {
		tape_fname[0] = '\0';
	}
	else {
		if (strlen(fname) < sizeof(tape_fname)) {
			strcpy(tape_fname, fname);
		}
		else {
			tape_fname[0] = '\0';
		}
	}

	/* ファイルオープンを試みる */
	if (tape_fname[0] != '\0') {
		tape_fileh = file_open(tape_fname, OPEN_RW);
		if (tape_fileh != -1) {
			tape_writep = FALSE;
		}
		else {
			tape_fileh = file_open(tape_fname, OPEN_R);
			tape_writep = TRUE;
		}
	}

	/* 開けていれば、ヘッダを読み込みチェック */
	if (tape_fileh != -1) {
		memset(buf, 0, sizeof(buf));
		file_read(tape_fileh, (BYTE*)buf, 16);
		if (strcmp(buf, header) != 0) {
			file_close(tape_fileh);
			tape_fileh = -1;
			tape_writep = FALSE;
		}
	}

	/* フラグの処理 */
	tape_setrec(FALSE);
	tape_count = 0;
	tape_incnt = 0;
	tape_subcnt = 0;

	/* ファイルが開けていれば、ファイルサイズ、オフセットを決定 */
	if (tape_fileh != -1) {
		tape_fsize = file_getsize(tape_fileh);
		tape_offset = 16;
	}
}

/*
 *	テープ
 *	録音フラグ設定
 */
void FASTCALL tape_setrec(BOOL flag)
{
	/* モータが回っていれば、マーカを書き込む */
	if (tape_motor && !tape_rec) {
		if (flag == TRUE) {
			tape_rec = TRUE;
			tape_mark();
			return;
		}
	}

	tape_rec = flag;
}

/*-[ メモリR/W ]------------------------------------------------------------*/

/*
 *	カセットテープ＆プリンタ
 *	１バイト読み出し
 */
BOOL FASTCALL tapelp_readb(WORD addr, BYTE *dat)
{
	BYTE ret;

	/* アドレスチェック */
	if (addr != 0xfd02) {
		return FALSE;
	}

	/* プリンタ ステータス作成 */
	ret = 0x30;
	if (lp_busy) {
		ret |= 0x01;
	}
	if (!lp_error) {
		ret |= 0x02;
	}
	if (!lp_ackng) {
		ret |= 0x04;
	}
	if (lp_pe) {
		ret |= 0x08;
	}

	/* WIN32版ではプリンタ未接続 */
#ifdef _WIN32
	ret |= 0x0f;
#endif

	/* カセット データ作成 */
	tape_input();
	if (tape_in) {
		ret |= 0x80;
	}

	/* ｏｋ */
	*dat = ret;
	return TRUE;
}

/*
 *	カセットテープ＆プリンタ
 *	１バイト書き込み
 */
BOOL FASTCALL tapelp_writeb(WORD addr, BYTE dat)
{
	switch (addr) {
		/* カセット制御、プリンタ制御 */
		case 0xfd00:
			/* プリンタ オンライン */
			if (dat & 0x80) {
				lp_online = FALSE;
			}
			else {
				lp_online = TRUE;
			}

			/* プリンタ ストローブ */
			if (dat & 0x40) {
				lp_strobe = TRUE;
			}
			else {
				if (lp_strobe && lp_online) {
					lp_output(lp_data);
					mainetc_lp();
				}
				lp_strobe = FALSE;
			}

			/* テープ 出力データ */
			if (dat & 0x01) {
				if (!tape_out) {
					tape_output(FALSE);
				}
				tape_out = TRUE;
			}
			else {
				if (tape_out) {
					tape_output(TRUE);
				}
				tape_out = FALSE;
			}

			/* テープ モータ */
			if (dat & 0x02) {
				if (tape_motor == FALSE) {
					/* 新規スタート */
					tape_count = 0;
					tape_subcnt = 0;
					tape_motor = TRUE;
					if (tape_rec == TRUE) {
						tape_mark();
					}
				}
			}
			else {
				/* モータ停止 */
				tape_motor = FALSE;
			}

			return TRUE;

		/* プリンタ出力データ */
		case 0xfd01:
			lp_data = dat;
			return TRUE;
	}

	return FALSE;
}

/*
 *	カセットテープ＆プリンタ
 *	セーブ
 */
BOOL FASTCALL tapelp_save(int fileh)
{
	BOOL tmp;

	if (!file_bool_write(fileh, tape_in)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, tape_out)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, tape_motor)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, tape_rec)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, tape_writep)) {
		return FALSE;
	}
	if (!file_word_write(fileh, tape_count)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, tape_subcnt)) {
		return FALSE;
	}

	if (!file_dword_write(fileh, tape_offset)) {
		return FALSE;
	}
	if (!file_write(fileh, (BYTE*)tape_fname, 128 + 1)) {
		return FALSE;
	}
	tmp = (tape_fileh != -1);
	if (!file_bool_write(fileh, tmp)) {
		return FALSE;
	}

	if (!file_word_write(fileh, tape_incnt)) {
		return FALSE;
	}
	if (!file_dword_write(fileh, tape_fsize)) {
		return FALSE;
	}

	if (!file_byte_write(fileh, lp_data)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, lp_busy)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, lp_error)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, lp_pe)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, lp_ackng)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, lp_online)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, lp_strobe)) {
		return FALSE;
	}

	if (!file_write(fileh, (BYTE*)lp_fname, 128 + 1)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	カセットテープ＆プリンタ
 *	ロード
 */
BOOL FASTCALL tapelp_load(int fileh, int ver)
{
	DWORD offset;
	char fname[128 + 1];
	BOOL flag;

	/* バージョンチェック */
	if (ver < 2) {
		return FALSE;
	}

	if (!file_bool_read(fileh, &tape_in)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &tape_out)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &tape_motor)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &tape_rec)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &tape_writep)) {
		return FALSE;
	}
	if (!file_word_read(fileh, &tape_count)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &tape_subcnt)) {
		return FALSE;
	}

	if (!file_dword_read(fileh, &offset)) {
		return FALSE;
	}
	if (!file_read(fileh, (BYTE*)fname, 128 + 1)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &flag)) {
		return FALSE;
	}

	/* マウント */
	tape_setfile(NULL);
	if (flag) {
		tape_setfile(fname);
		if ((tape_fileh != -1) && ((tape_fsize +1 ) >= tape_offset)) {
			file_seek(tape_fileh, offset);
		}
	}

	if (!file_word_read(fileh, &tape_incnt)) {
		return FALSE;
	}
	/* tape_fsizeは無効 */
	if (!file_dword_read(fileh, &offset)) {
		return FALSE;
	}

	if (!file_byte_read(fileh, &lp_data)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &lp_busy)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &lp_error)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &lp_pe)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &lp_ackng)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &lp_online)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &lp_strobe)) {
		return FALSE;
	}

	if (!file_read(fileh, (BYTE*)lp_fname, 128 + 1)) {
		return FALSE;
	}

	return TRUE;
}
