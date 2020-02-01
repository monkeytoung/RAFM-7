;
; FM-7 EMULATOR "XM7"
;
; Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
; [ Win32API レンダリング ]
;

;
; 外部定義
;
		section	.data data align=4 use32
		extern	_vram_c
		extern	_vram_dptr
		extern	_rgbTTLDD
		extern	_rgbAnalogDD
		extern	_pBitsGDI
		extern	_rgbTTLGDI
		extern	_rgbAnalogGDI

		section	.text code align=16 use32
		global	_Render640DD
		global	_Render320DD
		global	_Render640GDI
		global	_Render320GDI

;
; 640x200、デジタルモード
; DirectDrawレンダリング
;
; static void Render640(LPVOID lpSurface, LONG lPitch, int first, int last)
; lpSurface	ebp+8
; lPitch	ebp+12
; first		ebp+16
; last		ebp+20
;
_Render640DD:
		push	ebp
		mov	ebp,esp
		push	ebx
		push	esi
		push	edi
; サーフェイス設定
		mov	edi,[ebp+8]
		mov	eax,[ebp+12]
		imul	eax,[ebp+16]
		add	edi,eax
		add	edi,eax
; ピッチを前もって計算
		mov	esi,[ebp+12]
		sub	esi,1280
		mov	[ebp+12],esi
; VRAMアドレス設定
		mov	esi,[_vram_dptr]
		add	esi,0x4000
		mov	eax,[ebp+16]
		imul	eax,80
		add	esi,eax
; ライン数設定
		mov	edx,[ebp+20]
		sub	edx,[ebp+16]
; １ラインループ
		mov	ebx,_rgbTTLDD
.line_loop:
		push	edx
		mov	dl,80
; １バイトループ
.byte_loop:
		mov	al,[esi+0x4000]
		mov	ah,[esi]
		mov	dh,[esi-0x4000]
; bit7
		xor	ecx,ecx
		add	al,al
		adc	ecx,ecx
		add	ah,ah
		adc	ecx,ecx
		add	dh,dh
		adc	ecx,ecx
		mov	cx,[ebx+ecx*2]
		mov	[edi],cx
; bit6
		xor	ecx,ecx
		add	al,al
		adc	ecx,ecx
		add	ah,ah
		adc	ecx,ecx
		add	dh,dh
		adc	ecx,ecx
		mov	cx,[ebx+ecx*2]
		mov	[edi+2],cx
; bit5
		xor	ecx,ecx
		add	al,al
		adc	ecx,ecx
		add	ah,ah
		adc	ecx,ecx
		add	dh,dh
		adc	ecx,ecx
		mov	cx,[ebx+ecx*2]
		mov	[edi+4],cx
; bit4
		xor	ecx,ecx
		add	al,al
		adc	ecx,ecx
		add	ah,ah
		adc	ecx,ecx
		add	dh,dh
		adc	ecx,ecx
		mov	cx,[ebx+ecx*2]
		mov	[edi+6],cx
; bit3
		xor	ecx,ecx
		add	al,al
		adc	ecx,ecx
		add	ah,ah
		adc	ecx,ecx
		add	dh,dh
		adc	ecx,ecx
		mov	cx,[ebx+ecx*2]
		mov	[edi+8],cx
; bit2
		xor	ecx,ecx
		add	al,al
		adc	ecx,ecx
		add	ah,ah
		adc	ecx,ecx
		add	dh,dh
		adc	ecx,ecx
		mov	cx,[ebx+ecx*2]
		mov	[edi+10],cx
; bit1
		xor	ecx,ecx
		add	al,al
		adc	ecx,ecx
		add	ah,ah
		adc	ecx,ecx
		add	dh,dh
		adc	ecx,ecx
		mov	cx,[ebx+ecx*2]
		mov	[edi+12],cx
; bit0
		xor	ecx,ecx
		add	al,al
		adc	ecx,ecx
		add	ah,ah
		adc	ecx,ecx
		add	dh,dh
		adc	ecx,ecx
		mov	cx,[ebx+ecx*2]
		mov	[edi+14],cx
; 次のバイトへ
		add	edi,16
		inc	esi
		dec	dl
		jnz	near .byte_loop
; インターレース分をスキップ
		add	edi,[ebp+12]
		add	edi,1280
; 次のラインへ
		add	edi,[ebp+12]
		pop	edx
		dec	edx
		jnz	near .line_loop
; 終了
		pop	edi
		pop	esi
		pop	ebx
		pop	ebp
		ret

;
; 320x200、アナログモード
; DirectDrawレンダリング
;
; static void Render320(LPVOID lpSurface, LONG lPitch, int first, int last)
; lpSurface	ebp+8
; lPitch	ebp+12
; first		ebp+16
; last		ebp+20
;
_Render320DD:
		push	ebp
		mov	ebp,esp
		push	ebx
		push	esi
		push	edi
; サーフェイス設定
		mov	edi,[ebp+8]
		mov	eax,[ebp+12]
		imul	eax,[ebp+16]
		add	edi,eax
		add	edi,eax
; ピッチを前もって計算
		mov	esi,[ebp+12]
		sub	esi,1280
		mov	[ebp+12],esi
; VRAMアドレス設定
		mov	esi,[_vram_c]
		add	esi,0x0000c000
		mov	eax,[ebp+16]
		imul	eax,40
		add	esi,eax
; ライン数設定
		mov	edx,[ebp+20]
		sub	edx,[ebp+16]
; １ラインループ
.line_loop:
		push	edx
		mov	ebx,40
; １バイトループ
.byte_loop:
		push	ebx
; G
		mov	al,[esi-0x4000]
		mov	ah,[esi-0x2000]
		mov	dl,[esi+0x8000]
		mov	dh,[esi+0xa000]
		mov	ebx,8
; Gループ
.g_loop:
		xor	ecx,ecx
		add	al,al
		adc	ecx,ecx
		add	ah,ah
		adc	ecx,ecx
		add	dl,dl
		adc	ecx,ecx
		add	dh,dh
		adc	ecx,ecx
		mov	[edi],ecx
		add	edi,4
		dec	ebx
		jnz	.g_loop
		sub	edi,32
; R
		mov	al,[esi-0x8000]
		mov	ah,[esi-0x6000]
		mov	dl,[esi+0x4000]
		mov	dh,[esi+0x6000]
		mov	ebx,8
; Rループ
.r_loop:
		mov	ecx,[edi]
		add	al,al
		adc	ecx,ecx
		add	ah,ah
		adc	ecx,ecx
		add	dl,dl
		adc	ecx,ecx
		add	dh,dh
		adc	ecx,ecx
		mov	[edi],ecx
		add	edi,4
		dec	ebx
		jnz	.r_loop
		sub	edi,32
; B
		mov	al,[esi-0xc000]
		mov	ah,[esi-0xa000]
		mov	dl,[esi]
		mov	dh,[esi+0x2000]
		mov	ebx,8
; Bループ
.b_loop:
		mov	ecx,[edi]
		add	al,al
		adc	ecx,ecx
		add	ah,ah
		adc	ecx,ecx
		add	dl,dl
		adc	ecx,ecx
		add	dh,dh
		adc	ecx,ecx
; パレット変換
		and	ecx,4095
		add	ecx,ecx
		add	ecx,_rgbAnalogDD
		mov	cx,[ecx]
		mov	[edi],cx
		mov	[edi+2],cx
		add	edi,4
		dec	ebx
		jnz	.b_loop
; 次のバイトへ
		pop	ebx
		inc	esi
		dec	ebx
		jnz	near .byte_loop
; インターレース分をスキップ
		add	edi,[ebp+12]
		add	edi,1280
; 次のラインへ
		add	edi,[ebp+12]
		pop	edx
		dec	edx
		jnz	near .line_loop
; 終了
		pop	edi
		pop	esi
		pop	ebx
		pop	ebp
		ret

;
; 640x200、デジタルモード
; GDIレンダリング
;
; static void Render640(int first, int last)
; first		ebp+8
; last		ebp+12
;
_Render640GDI:
		push	ebp
		mov	ebp,esp
		push	ebx
		push	esi
		push	edi
; 初期ポインタ、カウンタを計算
		mov	esi,[_vram_dptr]
		mov	eax,[ebp+8]
		imul	eax,80
		add	esi,eax
		mov	edi,[_pBitsGDI]
		mov	eax,[ebp+8]
		imul	eax,10
		shl	eax,8
		add	edi,eax
		mov	edx,[ebp+12]
		sub	edx,[ebp+8]
		mov	ebp,_rgbTTLGDI
; １ラインループ
.line_loop:
		push	edx
		mov	ebx,80
; １バイトループ
.byte_loop:
		push	ebx
		mov	al,[esi+0x8000]
		mov	ah,[esi+0x4000]
		mov	dl,[esi]
; bit 7
		xor	ebx,ebx
		add	al,al
		adc	ebx,ebx
		add	ah,ah
		adc	ebx,ebx
		add	dl,dl
		adc	ebx,ebx
		mov	cx,[ebx*2+ebp]
		mov	[edi],cx
; bit 6
		xor	ebx,ebx
		add	al,al
		adc	ebx,ebx
		add	ah,ah
		adc	ebx,ebx
		add	dl,dl
		adc	ebx,ebx
		mov	cx,[ebx*2+ebp]
		mov	[edi+2],cx
; bit 5
		xor	ebx,ebx
		add	al,al
		adc	ebx,ebx
		add	ah,ah
		adc	ebx,ebx
		add	dl,dl
		adc	ebx,ebx
		mov	cx,[ebx*2+ebp]
		mov	[edi+4],cx
; bit 4
		xor	ebx,ebx
		add	al,al
		adc	ebx,ebx
		add	ah,ah
		adc	ebx,ebx
		add	dl,dl
		adc	ebx,ebx
		mov	cx,[ebx*2+ebp]
		mov	[edi+6],cx
; bit 3
		xor	ebx,ebx
		add	al,al
		adc	ebx,ebx
		add	ah,ah
		adc	ebx,ebx
		add	dl,dl
		adc	ebx,ebx
		mov	cx,[ebx*2+ebp]
		mov	[edi+8],cx
; bit 2
		xor	ebx,ebx
		add	al,al
		adc	ebx,ebx
		add	ah,ah
		adc	ebx,ebx
		add	dl,dl
		adc	ebx,ebx
		mov	cx,[ebx*2+ebp]
		mov	[edi+10],cx
; bit 1
		xor	ebx,ebx
		add	al,al
		adc	ebx,ebx
		add	ah,ah
		adc	ebx,ebx
		add	dl,dl
		adc	ebx,ebx
		mov	cx,[ebx*2+ebp]
		mov	[edi+12],cx
; bit 0
		xor	ebx,ebx
		add	al,al
		adc	ebx,ebx
		add	ah,ah
		adc	ebx,ebx
		add	dl,dl
		adc	ebx,ebx
		mov	cx,[ebx*2+ebp]
		mov	[edi+14],cx
; 次のバイトへ
		pop	ebx
		inc	esi
		add	edi,16
		dec	ebx
		jnz	near .byte_loop
; 次のラインへ
		pop	edx
		add	edi,1280
		dec	edx
		jnz	near .line_loop
; 終了
		pop	edi
		pop	esi
		pop	ebx
		pop	ebp
		ret

;
; 320x200、アナログモード
; GDIレンダリング
;
; static void Render320(int first, int last)
; first		ebp+8
; last		ebp+12
;
_Render320GDI:
		push	ebp
		mov	ebp,esp
		push	ebx
		push	esi
		push	edi
; 初期ポインタ、カウンタを計算
		mov	esi,[_vram_c]
		add	esi,0xc000
		mov	eax,[ebp+8]
		imul	eax,40
		add	esi,eax
		mov	edi,[_pBitsGDI]
		mov	eax,[ebp+8]
		imul	eax,10
		shl	eax,8
		add	edi,eax
		mov	edx,[ebp+12]
		sub	edx,[ebp+8]
		mov	ebp,_rgbAnalogGDI
; １ラインループ
.line_loop:
		push	edx
		mov	ebx,40
; １バイトループ
.byte_loop:
		push	ebx
; G
		mov	al,[esi-0x4000]
		mov	ah,[esi-0x2000]
		mov	dl,[esi+0x8000]
		mov	dh,[esi+0xa000]
		mov	ebx,8
; Gループ
.g_loop:
		xor	ecx,ecx
		add	al,al
		adc	ecx,ecx
		add	ah,ah
		adc	ecx,ecx
		add	dl,dl
		adc	ecx,ecx
		add	dh,dh
		adc	ecx,ecx
		mov	[edi],ecx
		add	edi,4
		dec	ebx
		jnz	.g_loop
		sub	edi,32
; R/
		mov	al,[esi-0x8000]
		mov	ah,[esi-0x6000]
		mov	dl,[esi+0x4000]
		mov	dh,[esi+0x6000]
		mov	ebx,8
; Rループ
.r_loop:
		mov	ecx,[edi]
		add	al,al
		adc	ecx,ecx
		add	ah,ah
		adc	ecx,ecx
		add	dl,dl
		adc	ecx,ecx
		add	dh,dh
		adc	ecx,ecx
		mov	[edi],ecx
		add	edi,4
		dec	ebx
		jnz	.r_loop
		sub	edi,32
; B
		mov	al,[esi-0xc000]
		mov	ah,[esi-0xa000]
		mov	dl,[esi]
		mov	dh,[esi+0x2000]
		mov	ebx,8
; Bループ
.b_loop:
		mov	ecx,[edi]
		add	al,al
		adc	ecx,ecx
		add	ah,ah
		adc	ecx,ecx
		add	dl,dl
		adc	ecx,ecx
		add	dh,dh
		adc	ecx,ecx
		and	ecx,0x00000fff
; パレット変換
		mov	cx,[ebp+ecx*2]
		mov	[edi],cx
		mov	[edi+2],cx
		add	edi,4
		dec	ebx
		jnz	.b_loop
; 次のバイトへ
		pop	ebx
		inc	esi
		dec	ebx
		jnz	near .byte_loop
; 次のラインへ
		pop	edx
		add	edi,1280
		dec	edx
		jnz	near .line_loop
; 終了
		pop	edi
		pop	esi
		pop	ebx
		pop	ebp
		ret

;
; プログラム終了
;
		end
