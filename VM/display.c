/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ ディスプレイ ]
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "xm7.h"
#include "display.h"
#include "subctrl.h"
#include "device.h"
#include "ttlpalet.h"
#include "multipag.h"
#include "mainetc.h"
#include "aluline.h"
#include "keyboard.h"
#include "event.h"

/*
 *	グローバル ワーク
 */
BOOL crt_flag;							/* CRT ONフラグ */
BOOL vrama_flag;						/* VRAMアクセスフラグ */
WORD vram_offset[2];					/* VRAMオフセットレジスタ */
WORD crtc_offset[2];					/* CRTCオフセット */
BOOL vram_offset_flag;					/* 拡張VRAMオフセットレジスタフラグ */
BOOL subnmi_flag;						/* サブNMIイネーブルフラグ */
BOOL vsync_flag;						/* VSYNCフラグ */
BOOL blank_flag;						/* ブランキングフラグ */

BYTE vram_active;						/* アクティブページ */
BYTE *vram_aptr;						/* VRAMアクティプポインタ */
BYTE vram_display;						/* 表示ページ */
BYTE *vram_dptr;						/* VRAM表示ポインタ */

/*
 *	スタティック ワーク
 */
static BYTE *vram_buf;					/* VRAMスクロールバッファ */
static BYTE vram_offset_count[2];		/* VRAMオフセット設定カウンタ */
static WORD hsync_count;				/* HSYNCカウンタ */

/*
 *	プロトタイプ宣言
 */
static BOOL FASTCALL subcpu_event(void); /* サブCPUタイマイベント */
static BOOL FASTCALL display_vsync(void);/* VSYNCイベント */
static BOOL FASTCALL display_blank(void);/* VBLANK,HBLANKイベント */

/*
 *	ディスプレイ
 *	初期化
 */
BOOL FASTCALL display_init(void)
{
	vram_buf = (BYTE *)malloc(0x4000);

	if (vram_buf == NULL) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	ディスプレイ
 *	クリーンアップ
 */
void FASTCALL display_cleanup(void)
{
	ASSERT(vram_buf);
	if (vram_buf) {
		free(vram_buf);
	}
}

/*
 *	ディスプレイ
 *	リセット
 */
void FASTCALL display_reset(void)
{
	WORD addr;

	/* CRTレジスタ */
	crt_flag = FALSE;
	vrama_flag = FALSE;
	memset(vram_offset, 0, sizeof(vram_offset));
	memset(crtc_offset, 0, sizeof(crtc_offset));
	vram_offset_flag = FALSE;
	memset(vram_offset_count, 0, sizeof(vram_offset_count));

	/* 割り込み、イベント */
	subnmi_flag = TRUE;
	vsync_flag = FALSE;
	blank_flag = TRUE;

	/* アクティブページ、表示ページ初期化 */
	vram_active = 0;
	vram_aptr = vram_c;
	vram_display = 0;
	vram_dptr = vram_c;

	/* 本来、VRAMクリアは必要ない */
	memset(vram_b, 0, 0xc000);
	memset(vram_c, 0, 0xc000);
	for (addr=0; addr<0xc000; addr++) {
		vram_notify(addr, 0);
	}

	/* 20msごとに起こすイベントを追加 */
	schedule_setevent(EVENT_SUBTIMER, 20000, subcpu_event);

	/* VSYNCイベントを作成 */
	schedule_setevent(EVENT_VSYNC, 1520, display_vsync);

	/* V,HBLANKイベントを作成 */
	schedule_setevent(EVENT_BLANK, 3940, display_blank);
	hsync_count = 300;
}

/*
 *	VSYNCイベント
 */
static BOOL FASTCALL display_vsync(void)
{
	if (vsync_flag == FALSE) {
		/* これから垂直同期 0.51ms */
		vsync_flag = TRUE;
		schedule_setevent(EVENT_VSYNC, 510, display_vsync);

		/* ビデオディジタイズ */
		if (digitize_enable) {
			if (digitize_keywait || simpose_mode == 0x03) {
				digitize_notify();
			}
		}
	}
	else {
		/* これから垂直表示 1.91ms + 12.7ms + 1.52ms */
		vsync_flag = FALSE;
		schedule_setevent(EVENT_VSYNC, 1910+12700+1520, display_vsync);
		vsync_notify();
	}

	return TRUE;
}

/*
 *	V/H BLANKイベント
 */
static BOOL FASTCALL display_blank(void)
{
	if (hsync_count >= 200) {
		if (blank_flag == FALSE) {
			/* これからV-BLANK 3.94ms */
			blank_flag = TRUE;
			schedule_setevent(EVENT_BLANK, 3940, display_blank);
			return TRUE;
		}
		else {
			/* これから0ライン目 24μs */
			blank_flag = TRUE;
			schedule_setevent(EVENT_BLANK, 24, display_blank);
			hsync_count = 0;
			return TRUE;
		}
	}

	if (blank_flag == TRUE) {
		/* これから水平表示期間 39μsもしくは40μs */
		blank_flag = FALSE;
		schedule_setevent(EVENT_BLANK, 39 + (hsync_count & 1), display_blank);
		hsync_count++;
		return TRUE;
	}
	else {
		/* 水平同期期間 24μs */
		blank_flag = TRUE;
		schedule_setevent(EVENT_BLANK, 24, display_blank);
		return TRUE;
	}
}

/*
 *	サブCPU
 *	イベント処理
 */	
static BOOL FASTCALL subcpu_event(void)
{
	/* 念のため、チェック */
	if (!subnmi_flag) {
		return FALSE;
	}

	/* NMI割り込みを起こす */
	subcpu_nmi();
	return TRUE;
}

/*
 *	VRAMスクロール
 */
static void FASTCALL vram_scroll(WORD offset)
{
	int i;
	BYTE *vram;

	if (mode320) {
		/* 320時 オフセットマスク */
		offset &= 0x1fff;

		/* ループ */
		for (i=0; i<6; i++) {
			vram = (BYTE *)(vram_aptr + 0x2000 * i);

			/* テンポラリバッファへコピー */
			memcpy(vram_buf, vram, offset);

			/* 前へ詰める */
			memcpy(vram, (vram + offset), 0x2000 - offset);

			/* テンポラリバッファより復元 */
			memcpy(vram + (0x2000 - offset), vram_buf, offset);
		}
	}
	else {
		/* 640時 オフセットマスク */
		offset &= 0x3fff;

		/* ループ */
		for (i=0; i<3; i++) {
			vram = (BYTE *)(vram_aptr + 0x4000 * i);

			/* テンポラリバッファへコピー */
			memcpy(vram_buf, vram, offset);

			/* 前へ詰める */
			memcpy(vram, (vram + offset), 0x4000 - offset);

			/* テンポラリバッファより復元 */
			memcpy(vram + (0x4000 - offset), vram_buf, offset);
		}
	}
}
/*
 *	ディスプレイ
 *	１バイト読み込み
 *	※メイン－サブインタフェース信号線を含む
 */
BOOL FASTCALL display_readb(WORD addr, BYTE *dat)
{
	BYTE ret;

	switch (addr) {
		/* キャンセルIRQ ACK */
		case 0xd402: 
			subcancel_flag = FALSE;
			subcpu_irq();
			*dat = 0xff;
			return TRUE;

		/* BEEP */
		case 0xd403:
			beep_flag = TRUE;
			schedule_setevent(EVENT_BEEP, 205000, mainetc_beep);
			return TRUE;

		/* アテンションIRQ ON */
		case 0xd404:
			subattn_flag = TRUE;
			*dat = 0xff;
			maincpu_firq();
			return TRUE;

		/* CRT ON */
		case 0xd408:
			if (!crt_flag) {
				crt_flag = TRUE;
				/* CRT OFF→ON */
				multipag_writeb(0xfd37, multi_page);
			}
			*dat = 0xff;
			return TRUE;

		/* VRAMアクセス ON */
		case 0xd409:
			vrama_flag = TRUE;
			*dat = 0xff;
			return TRUE;

		/* BUSYフラグ OFF */
		case 0xd40a:
			subbusy_flag = FALSE;
			*dat = 0xff;
			return TRUE;

		/* FM77AV MISCレジスタ */
		case 0xd430:
			if (fm7_ver >= 2) {
				ret = 0xff;

				/* ブランキング */
				if (blank_flag) {
					ret &= (BYTE)~0x80;
				}

				/* 直線補間 */
				if (line_busy) {
					ret &= (BYTE)~0x10;
				}

				/* VSYNC */
				if (!vsync_flag) {
					ret &= (BYTE)~0x04;
				}

				/* サブRESETステータス */
				if (!subreset_flag) {
					ret &= (BYTE)~0x01;
				}

				*dat = ret;
				return TRUE;
			}
	}

	return FALSE;
}

/*
 *	ディスプレイ
 *	１バイト書き込み
 *	※メイン－サブインタフェース信号線を含む
 */
BOOL FASTCALL display_writeb(WORD addr, BYTE dat)
{
	WORD offset;

	switch (addr) {
		/* CRT OFF */
		case 0xd408:
			if (crt_flag) {
				/* CRT ON→OFF */
				crt_flag = FALSE;
				multipag_writeb(0xfd37, multi_page);
			}
			crt_flag = FALSE;
			return TRUE;

		/* VRAMアクセス OFF */
		case 0xd409:
			vrama_flag = FALSE;
			return TRUE;

		/* BUSYフラグ ON */
		case 0xd40a:
			subbusy_flag = TRUE;
			return TRUE;

		/* VRAMオフセットアドレス 上位 */
		case 0xd40e:
			offset = (WORD)(dat & 0x3f);
			offset <<= 8;
			offset |= (WORD)(vram_offset[vram_active] & 0xff);
			vram_offset[vram_active] = offset;
			/* カウントアップ、スクロール */
			vram_offset_count[vram_active]++;
			if ((vram_offset_count[vram_active] & 1) == 0) {
				vram_scroll((WORD)(vram_offset[vram_active] -
									crtc_offset[vram_active]));
				crtc_offset[vram_active] = vram_offset[vram_active];
				display_notify();
			}
			return TRUE;

		/* VRAMオフセットアドレス 下位 */
		case 0xd40f:
			/* 拡張フラグがなければ、下位5bitは無効 */
			if (!vram_offset_flag) {
				dat &= 0xe0;
			}
			offset = (WORD)(vram_offset[vram_active] & 0x3f00);
			offset |= (WORD)dat;
			vram_offset[vram_active] = offset;
			/* カウントアップ、スクロール */
			vram_offset_count[vram_active]++;
			if ((vram_offset_count[vram_active] & 1) == 0) {
				vram_scroll((WORD)(vram_offset[vram_active] -
									crtc_offset[vram_active]));
				crtc_offset[vram_active] = vram_offset[vram_active];
				display_notify();
			}
			return TRUE;

		/* FM77AV MISCレジスタ */
		case 0xd430:
			/* NMIマスク */
			if (dat & 0x80) {
				subnmi_flag = FALSE;
				event[EVENT_SUBTIMER].flag = EVENT_DISABLED;
				subcpu.intr &= ~INTR_NMI;
			}
			else {
				subnmi_flag = TRUE;
				event[EVENT_SUBTIMER].flag = EVENT_ENABLED;
			}

			/* ディスプレイページ */
			if (dat & 0x40) {
				if (vram_display == 0) {
					vram_display = 1;
					vram_dptr = vram_b;
					display_notify();
				}
			}
			else {
				if (vram_display == 1){
					vram_display = 0;
					vram_dptr = vram_c;
					display_notify();
				}
			}

			/* アクティブページ */
			if (dat & 0x20) {
				vram_active = 1;
				vram_aptr = vram_b;
			}
			else {
				vram_active = 0;
				vram_aptr = vram_c;
			}

			/* 拡張VRAMオフセットレジスタ */
			if (dat & 0x04) {
				if (!vram_offset_flag) {
					vram_offset_flag = TRUE;
				}
			}
			else {
				if (vram_offset_flag) {
					vram_offset_flag = FALSE;
				}
			}

			/* CGROMバンク */
			cgrom_bank = (BYTE)(dat & 0x03);
			return TRUE;

	}

	return FALSE;
}

/*
 *	ディスプレイ
 *	セーブ
 */
BOOL FASTCALL display_save(int fileh)
{
	if (!file_bool_write(fileh, crt_flag)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, vrama_flag)) {
		return FALSE;
	}

	if (!file_bool_write(fileh, subnmi_flag)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, vsync_flag)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, blank_flag)) {
		return FALSE;
	}

	if (!file_bool_write(fileh, vram_offset_flag)) {
		return FALSE;
	}
	if (!file_word_write(fileh, vram_offset[0])) {
		return FALSE;
	}
	if (!file_word_write(fileh, vram_offset[1])) {
		return FALSE;
	}
	if (!file_word_write(fileh, crtc_offset[0])) {
		return FALSE;
	}
	if (!file_word_write(fileh, crtc_offset[1])) {
		return FALSE;
	}

	if (!file_byte_write(fileh, vram_active)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, vram_display)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	ディスプレイ
 *	ロード
 */
BOOL FASTCALL display_load(int fileh, int ver)
{
	/* バージョンチェック */
	if (ver < 2) {
		return FALSE;
	}

	if (!file_bool_read(fileh, &crt_flag)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &vrama_flag)) {
		return FALSE;
	}

	if (!file_bool_read(fileh, &subnmi_flag)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &vsync_flag)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &blank_flag)) {
		return FALSE;
	}

	if (!file_bool_read(fileh, &vram_offset_flag)) {
		return FALSE;
	}
	if (!file_word_read(fileh, &vram_offset[0])) {
		return FALSE;
	}
	if (!file_word_read(fileh, &vram_offset[1])) {
		return FALSE;
	}
	if (!file_word_read(fileh, &crtc_offset[0])) {
		return FALSE;
	}
	if (!file_word_read(fileh, &crtc_offset[1])) {
		return FALSE;
	}

	if (!file_byte_read(fileh, &vram_active)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &vram_display)) {
		return FALSE;
	}

	/* ポインタを構成 */
	if (vram_active == 0) {
		vram_aptr = vram_c;
	}
	else {
		vram_aptr = vram_b;
	}
	if (vram_display == 0) {
		vram_dptr = vram_c;
	}
	else {
		vram_dptr = vram_b;
	}

	/* イベント */
	schedule_handle(EVENT_SUBTIMER, subcpu_event);
	schedule_handle(EVENT_VSYNC, display_vsync);
	schedule_handle(EVENT_BLANK, display_blank);

	return TRUE;
}
