/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API 識別子定義 ]
 */

#ifdef _WIN32

/* メニュー、アクセラレータ、タイマー、アイコン、ダイアログ */
#define IDR_MAINMENU					101
#define IDR_ACCELERATOR 				102
#define IDR_TIMER						103
#define IDI_APPICON						104
#define IDD_ABOUTDLG					105
#define IDI_WNDICON						106
#define IDR_DISASMMENU					107
#define IDD_ADDRDLG						108
#define IDR_BREAKPOINTMENU				109
#define IDD_TITLEDLG					110
#define IDD_GENERALPAGE					111
#define IDD_SOUNDPAGE					112
#define IDD_KBDPAGE						113
#define IDD_KEYINDLG					114
#define IDD_JOYPAGE						115
#define IDI_FUJICON						116
#define IDD_SCRPAGE						117
#define IDD_OPTPAGE						118

/* チャイルドウインドウ */
#define ID_TOOL_BAR						1000
#define ID_STATUS_BAR					1001
#define ID_CAPTURE_WINDOW				1002

/* バージョン情報ダイアログ */
#define IDC_TITLE						1010
#define IDC_COPYRIGHT					1011
#define IDC_ABOUTICON					1012
#define IDC_FMCOPY						1013
#define IDC_VERSION						1014
#define IDC_URL							1015

/* アドレス入力ダイアログ */
#define IDC_ADDRLABEL					1020
#define IDC_ADDRCOMBO					1021

/* タイトル入力ダイアログ */
#define IDC_TITLELABEL					1030
#define IDC_TITLEEDIT					1031

/* プロパティページ */
#define IDC_GP_FM7						1040
#define IDC_GP_FM77AV					1041
#define IDC_GP_SPEEDMODEG				1042
#define IDC_GP_HIGHSPEED				1043
#define IDC_GP_LOWSPEED					1044
#define IDC_GP_SPEEDG					1045
#define IDC_GP_HELP						1046
#define IDC_GP_TAPESPEED				1047
#define IDC_GP_CPUCOMBO					1048
#define IDC_GP_CPUTEXT					1049
#define IDC_GP_CPUSPIN					1050
#define IDC_GP_CPULABEL					1051
#define IDC_GP_MACHINEG					1052

/* サウンドページ */
#define IDC_SP_HELP						1060
#define IDC_SP_BEEPG					1061
#define IDC_SP_RATEG					1062
#define IDC_SP_44K						1063
#define IDC_SP_22K						1064
#define IDC_SP_NONE						1065
#define IDC_SP_BUFFERG					1066
#define IDC_SP_BUFMS					1067
#define IDC_SP_BUFEDIT					1068
#define IDC_SP_BUFSPIN					1069
#define IDC_SP_BUFTEXT					1070
#define IDC_SP_BEEPEDIT					1071
#define IDC_SP_BEEPTEXT					1072
#define IDC_SP_BEEPHZ					1073
#define IDC_SP_BEEPSPIN					1074

/* キーボードページ */
#define IDC_KP_MAPG						1080
#define IDC_KP_LIST						1081
#define IDC_KP_101B						1082
#define IDC_KP_106B						1083
#define IDC_KP_98B						1084
#define IDC_KP_HELP						1085

/* キー入力ダイアログ */
#define IDC_KEYIN_LABEL					1090
#define IDC_KEYIN_STATIC				1091
#define IDC_KEYIN_KEY					1092
#define IDC_KEYIN_OK					1093
#define IDC_KEYIN_CANCEL				1094

/* ジョイスティックページ */
#define IDC_JP_PORTG					1100
#define IDC_JP_PORT1					1101
#define IDC_JP_PORT2					1102
#define IDC_JP_TYPEG					1103
#define IDC_JP_TYPEC					1104
#define IDC_JP_RAPIDG					1105
#define IDC_JP_RAPIDA					1106
#define IDC_JP_RAPIDB					1107
#define IDC_JP_RAPIDAC					1108
#define IDC_JP_RAPIDBC					1109
#define IDC_JP_CODEG					1110
#define IDC_JP_UP						1111
#define IDC_JP_UPC						1112
#define IDC_JP_DOWN						1113
#define IDC_JP_DOWNC					1114
#define IDC_JP_LEFT						1115
#define IDC_JP_LEFTC					1116
#define IDC_JP_RIGHT					1117
#define IDC_JP_RIGHTC					1118
#define IDC_JP_CENTER					1119
#define IDC_JP_CENTERC					1120
#define IDC_JP_A						1121
#define IDC_JP_AC						1122
#define IDC_JP_B						1123
#define IDC_JP_BC						1124
#define IDC_JP_HELP						1125

/* スクリーンページ */
#define IDC_SCP_MODEG					1130
#define IDC_SCP_400						1131
#define IDC_SCP_480						1132
#define IDC_SCP_ETCG					1133
#define IDC_SCP_24K						1134
#define IDC_SCP_CAPTIONB				1135
#define IDC_SCP_HELP					1136

/* オプションページ */
#define IDC_OP_WHGG						1140
#define IDC_OP_WHGB						1141
#define IDC_OP_DIGITIZEG				1142
#define IDC_OP_DIGITIZEB				1143
#define IDC_OP_HELP						1144

/* ファイルメニュー */
#define IDM_OPEN						40001
#define IDM_SAVE						40002
#define IDM_SAVEAS						40003
#define IDM_RESET						40004
#define IDM_BASIC						40005
#define IDM_DOS 						40006
#define IDM_EXIT						40007

/* ドライブ1メニュー */
#define IDM_D1OPEN						40008
#define IDM_DBOPEN						40009
#define IDM_D1EJECT 					40010
#define IDM_D1TEMP						40011
#define IDM_D1WRITE 					40012
#define IDM_D1MEDIA00					40013
#define IDM_D1MEDIA01					40014
#define IDM_D1MEDIA02					40015
#define IDM_D1MEDIA03					40016
#define IDM_D1MEDIA04					40017
#define IDM_D1MEDIA05					40018
#define IDM_D1MEDIA06					40019
#define IDM_D1MEDIA07					40020
#define IDM_D1MEDIA08					40021
#define IDM_D1MEDIA09					40022
#define IDM_D1MEDIA10					40023
#define IDM_D1MEDIA11					40024
#define IDM_D1MEDIA12					40025
#define IDM_D1MEDIA13					40026
#define IDM_D1MEDIA14					40027
#define IDM_D1MEDIA15					40028

/* ドライブ0メニュー */
#define IDM_D0OPEN						40029

#define IDM_D0EJECT						40031
#define IDM_D0TEMP						40032
#define IDM_D0WRITE 					40033
#define IDM_D0MEDIA00					40034
#define IDM_D0MEDIA01					40035
#define IDM_D0MEDIA02					40036
#define IDM_D0MEDIA03					40037
#define IDM_D0MEDIA04					40038
#define IDM_D0MEDIA05					40039
#define IDM_D0MEDIA06					40040
#define IDM_D0MEDIA07					40041
#define IDM_D0MEDIA08					40042
#define IDM_D0MEDIA09					40043
#define IDM_D0MEDIA10					40044
#define IDM_D0MEDIA11					40045
#define IDM_D0MEDIA12					40046
#define IDM_D0MEDIA13					40047
#define IDM_D0MEDIA14					40048
#define IDM_D0MEDIA15					40049

/* テープメニュー */
#define IDM_TOPEN						40050
#define IDM_TEJECT						40051
#define IDM_REW							40052
#define IDM_FF							40053
#define IDM_REC							40054

/* 表示メニュー */
#define IDM_FDC							40055
#define IDM_OPNREG						40056
#define IDM_OPNDISP						40057
#define IDM_SUBCTRL						40058
#define IDM_KEYBOARD					40059
#define IDM_MMR							40060
#define IDM_STATUS						40066
#define IDM_REFRESH						40067
#define IDM_SYNC						40068
#define IDM_FULLSCREEN					40069

/* デバッグメニュー */
#define IDM_EXEC 						40070
#define IDM_BREAK						40071
#define IDM_TRACE						40072
#define IDM_BREAKPOINT					40073
#define IDM_SCHEDULER					40074
#define IDM_CPU_MAIN					40075
#define IDM_CPU_SUB						40076
#define IDM_DISASM_MAIN					40077
#define IDM_DISASM_SUB					40078
#define IDM_MEMORY_MAIN					40079
#define IDM_MEMORY_SUB					40080

/* ツールメニュー */
#define IDM_CONFIG						40090
#define IDM_GRPCAP						40091
#define IDM_WAVCAP						40092
#define IDM_NEWDISK						40093
#define IDM_NEWTAPE						40094
#define IDM_VFD2D77						40095
#define IDM_2D2D77						40096
#define IDM_VTP2T77						40097

/* ウィンドウメニュー */
#define IDM_CASCADE						40110
#define IDM_TILE						40111
#define IDM_ICONIC						40112
#define IDM_ARRANGEICON					40113
#define IDM_SWND00						40114
#define IDM_SWND01						40115
#define IDM_SWND02						40116
#define IDM_SWND03						40117
#define IDM_SWND04						40118
#define IDM_SWND05						40119
#define IDM_SWND06						40120
#define IDM_SWND07						40121
#define IDM_SWND08						40122
#define IDM_SWND09						40123
#define IDM_SWND10						40124
#define IDM_SWND11						40125
#define IDM_SWND12						40126
#define IDM_SWND13						40127
#define IDM_SWND14						40128
#define IDM_SWND15						40129

/* ヘルプメニュー */
#define IDM_ABOUT						40140

/* コンテキストメニュー(逆アセンブル・ダンプ) */
#define IDM_DIS_ADDR					40200
#define IDM_DIS_PC						40201
#define IDM_DIS_X						40202
#define IDM_DIS_Y						40203
#define IDM_DIS_U						40204
#define IDM_DIS_S						40205
#define IDM_DIS_RESET					40206
#define IDM_DIS_NMI						40207
#define IDM_DIS_SWI						40208
#define IDM_DIS_IRQ						40209
#define IDM_DIS_FIRQ					40210
#define IDM_DIS_SWI2					40211
#define IDM_DIS_SWI3					40212

/* コンテキストメニュー(ブレークポイント) */
#define IDM_BREAKP_JUMP					40300
#define IDM_BREAKP_ENABLE 				40301
#define IDM_BREAKP_DISABLE				40302
#define IDM_BREAKP_CLEAR				40303
#define IDM_BREAKP_ALL					40304

/* メッセージ文字列群 */
#define IDS_IDLEMESSAGE 				41000
#define IDS_VMERROR 					41001
#define IDS_COMERROR					41002
#define IDS_RUNCAPTION					41003
#define IDS_STOPCAPTION					41004
#define IDS_DISKFILTER					41005
#define IDS_DISKOPEN					41006
#define IDS_DISKBOTH					41007
#define IDS_DISKEJECT					41008
#define IDS_DISKPROTECT					41009
#define IDS_DISK2D						41010
#define IDS_TAPEFILTER					41011
#define IDS_TAPEOPEN					41012
#define IDS_TAPEEJECT					41013
#define IDS_TAPEREW						41014
#define IDS_TAPEFF						41015
#define IDS_TAPEREC						41016
#define IDS_NEWTAPEOK					41017
#define IDS_NEWDISKFILTER				41018
#define IDS_NEWDISKOK					41019
#define IDS_GRPCAPFILTER				41020
#define IDS_SWND_CPUREG_MAIN			41021
#define IDS_SWND_CPUREG_SUB				41022
#define IDS_SWND_SCHEDULER				41023
#define IDS_SWND_DISASM_MAIN			41024
#define IDS_SWND_DISASM_SUB				41025
#define IDS_SWND_MEMORY_MAIN			41026
#define IDS_SWND_MEMORY_SUB				41027
#define IDS_STATEFILTER					41028
#define IDS_STATEERROR					41029
#define IDS_SWND_BREAKPOINT				41030
#define IDS_SWND_FDC					41031
#define IDS_VFDFILTER					41032
#define IDS_2DFILTER					41033
#define IDS_VTPFILTER					41034
#define IDS_CONVERTOK					41035
#define IDS_SWND_OPNREG					41036
#define IDS_SWND_SUBCTRL				41037
#define IDS_CONFIGCAPTION				41038
#define IDS_GP_MAINCPU					41039
#define IDS_GP_MAINMMR					41040
#define IDS_SWND_OPNDISP				41041
#define IDS_SWND_KEYBOARD				41042
#define IDS_KP_KEYNO					41043
#define IDS_KP_KEYFM					41044
#define IDS_KP_KEYKANA					41045
#define IDS_KP_KEYDI					41046
#define IDS_DISKTEMP					41047
#define IDS_JP_RAPID0					41048
#define IDS_JP_RAPID1					41049
#define IDS_JP_RAPID2					41050
#define IDS_JP_RAPID3					41051
#define IDS_JP_RAPID4					41052
#define IDS_JP_RAPID5					41053
#define IDS_JP_RAPID6					41054
#define IDS_JP_RAPID7					41055
#define IDS_JP_RAPID8					41056
#define IDS_JP_RAPID9					41057
#define IDS_JP_TYPE0					41058
#define IDS_JP_TYPE1					41059
#define IDS_JP_TYPE2					41060
#define IDS_JP_TYPE3					41061
#define IDS_ABOUTURL					41062
#define IDS_SWND_MMR					41063
#define IDS_WAVCAPFILTER				41064
#define IDS_WAVCAPERROR					41065

#endif	/* _WIN32 */

