*
* FM-7 EMULATOR "XM7"
*
* Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
* [ ユーティリティIPL ]
*
		ORG	$0100

*
* プログラムスタート
*
* BASICモード時	$0100からブート
* DOSモード時	$0300からブート
* となるため、ポジションインディペンデントに書いておく
*
START		BRA	MAIN
		NOP

*
* DPB (720KB 2DD)
*
		FCC	/MSDOS5.0/
		FCB	$00,$02,$02,$01,$00
		FCB	$02,$70,$00,$A0,$05
		FCB	$F9
		FCB	$03,$00,$09,$00,$02
		FCB	$00,$00,$00,$00,$00
		FCB	$00,$00,$00,$00
		FCB	$00,$00
		FCB	$00,$00
		FCB	$00,$00,$00
		FCC	/NO NAME    /
		FCC	/FAT12   /

*
* メイン
*
MAIN		ORCC	#$50
* 初期化
		LDS	#$0F00
		STA	$FD0F
* 本体読み込み。$1000から$1000バイト
		LDA	#$FD
		TFR	A,DP
		LDB	#$08
LOOP		LEAX	RCB,PCR
		PSHS	B
		JSR	$FE08
		PULS	B
		TSTA
		BNE	ERROR
* 次のセクタへ
		INC	2,X
		INC	2,X
		INC	5,X
		DECB
		BNE	LOOP

*
* 本体へジャンプ
*
		JMP	$1000

*
* 読み込みエラー(BEEPを鳴らす)
*
ERROR		LDA	#$81
		STA	$FD03
		BRA	*

*
* RCB
*
RCB		FCB	$0A,$00		* DREAD
		FDB	$1000		* バッファアドレス
		FCB	$00		* トラック
		FCB	$02		* セクタ
		FCB	$00		* サイド
		FCB	$00		* ドライブ

*
* あまりデータ
*
OMAKE		FCB	$00,$00,$00,$00
		FCB	$00,$00,$00,$00
		FCB	$00,$00
		FCC	/このプログラムはXM7のために/
		FCC	/今年(1999年)に作成したものです。/
		FCC	/特徴として、MS-DOS下位互換の/
		FCC	/簡単なファイルシステムを持っており/
		FCC	/Win9xで互換性のためサポートされている、360KB/
		FCC	/メディアの作成及びファイル書き込み/
		FCC	/ができます。なお/
		FCC	/開発にはMS-DOS上でクロスアセンブラ/
		FCC	/AS63.EXEを使用しました。使いやすい/
		FCC	/アセンブラをFSWで提供頂いた作者/
		FCC	/の方に感謝します。/
		FCC	/1999.7.31 written by ＰＩ．/
		FCC	/(ytanaka@ipc-tokai.or.jp)/

*
* プログラム終了
*
		END
