  # Xtensa LX106 assembly emitted by transembly/xtensa_lx106.py
  # module:  cssc_main
  # profile: esp8266
  # triple:  xtensa-none-elf

  .global __adddf3
  .global __addsf3
  .global __divdf3
  .global __divdi3
  .global __divsf3
  .global __divsi3
  .global __eqdf2
  .global __eqsf2
  .global __extendsfdf2
  .global __fixdfsi
  .global __floatsidf
  .global __gedf2
  .global __gesf2
  .global __gtdf2
  .global __gtsf2
  .global __ledf2
  .global __lesf2
  .global __ltdf2
  .global __ltsf2
  .global __moddi3
  .global __modsi3
  .global __muldf3
  .global __muldi3
  .global __mulsf3
  .global __mulsi3
  .global __nedf2
  .global __nesf2
  .global __subdf3
  .global __subsf3
  .global __truncdfsf2
  .global cssc_float_to_str
  .global cssc_i2c_begin
  .global cssc_i2c_free
  .global cssc_i2c_new
  .global cssc_int_to_str
  .global cssc_obj_alloc
  .global cssc_obj_free
  .global cssc_oled_begin
  .global cssc_oled_fill
  .global cssc_oled_fillrect
  .global cssc_oled_line
  .global cssc_oled_new
  .global cssc_oled_show
  .global cssc_oled_text
  .global cssc_release
  .global cssc_runtime_init
  .global cssc_runtime_shutdown
  .global cssc_sleep_ms
  .global cssc_string_concat
  .global cssc_string_lit
  .global cssc_tft_free
  .global cssc_uptime

  .section .rodata
  .align 4
.str.10: .asciz "VideoDriver"
.str.11: .asciz "shutdown"
.str.0: .asciz "VideoDriver v1.0.1"
.str.1: .asciz "VideoDriver "
.str.2: .asciz "VideoDriver "
.str.3: .asciz "s"
.str.4: .asciz "s"
.str.5: .asciz "boot ok"
.str.6: .asciz "CSSC Embedded ("
.str.7: .asciz "CSSC Embedded ("
.str.8: .asciz ")"
.str.9: .asciz ")"
  .text

  .text
  .global cssc_user_main
  .type   cssc_user_main, @function
.align  4
cssc_user_main:
  addi    a1, a1, -608
  s32i.n  a0, a1, 604
  s32i.n  a12, a1, 588    # save callee-save a12
  s32i.n  a13, a1, 592    # save callee-save a13
  s32i.n  a14, a1, 596    # save callee-save a14
  s32i.n  a15, a1, 600    # save callee-save a15
  call0   cssc_scope_push    # arena scope for transient allocations
.Lcssc_user_main__entry:
  movi.n  a15, 0
  movi.n  a14, 0
  movi.n  a13, 4
  movi.n  a12, 0
  movi.n  a8, 5
  movi.n  a9, 0
  s32i.n  a8, a1, 40
  s32i.n  a9, a1, 44
  l32i.n  a6, a1, 40
  l32i.n  a7, a1, 44
  mov.n   a4, a13
  mov.n   a5, a12
  mov.n   a2, a15
  mov.n   a3, a14
  call0   cssc_i2c_new
  s32i.n  a2, a1, 48
  addi    a8, a1, 0    # i2c slot[4]
  s32i.n  a8, a1, 52
  l32i.n  a8, a1, 52
  l32i.n  a9, a1, 48
  s32i.n  a9, a8, 0
  l32i.n  a8, a1, 52
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 56
  l32r    a8, .LIT0
  movi.n  a9, 0
  s32i.n  a8, a1, 64
  s32i.n  a9, a1, 68
  l32i.n  a4, a1, 64
  l32i.n  a5, a1, 68
  l32i.n  a2, a1, 56
  call0   cssc_i2c_begin
  movi.n  a8, 0
  movi.n  a9, 0
  s32i.n  a8, a1, 72
  s32i.n  a9, a1, 76
  movi.n  a8, 1
  movi.n  a9, 0
  s32i.n  a8, a1, 80
  s32i.n  a9, a1, 84
  movi    a8, 128
  movi.n  a9, 0
  s32i.n  a8, a1, 88
  s32i.n  a9, a1, 92
  movi    a8, 64
  movi.n  a9, 0
  s32i.n  a8, a1, 96
  s32i.n  a9, a1, 100
  movi.n  a8, 12
  movi.n  a9, 0
  s32i.n  a8, a1, 104
  s32i.n  a9, a1, 108
  movi.n  a8, 14
  movi.n  a9, 0
  s32i.n  a8, a1, 112
  s32i.n  a9, a1, 116
  movi    a8, 60
  movi.n  a9, 0
  s32i.n  a8, a1, 120
  s32i.n  a9, a1, 124
  addi    a1, a1, -32    # outgoing stack args
  l32i.n  a6, a1, 128
  l32i.n  a7, a1, 132
  l32i.n  a4, a1, 120
  l32i.n  a5, a1, 124
  l32i.n  a2, a1, 112
  l32i.n  a3, a1, 116
  l32i.n  a8, a1, 136
  l32i.n  a9, a1, 140
  s32i.n  a8, a1, 0
  s32i.n  a9, a1, 4
  l32i.n  a8, a1, 144
  l32i.n  a9, a1, 148
  s32i.n  a8, a1, 8
  s32i.n  a9, a1, 12
  l32i.n  a8, a1, 152
  l32i.n  a9, a1, 156
  s32i.n  a8, a1, 16
  s32i.n  a9, a1, 20
  call0   cssc_oled_new
  addi    a1, a1, 32     # pop stack args
  s32i.n  a2, a1, 128
  addi    a8, a1, 4    # oled slot[15]
  s32i.n  a8, a1, 132
  l32i.n  a8, a1, 132
  l32i.n  a9, a1, 128
  s32i.n  a9, a8, 0
  l32i.n  a8, a1, 132
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 136
  movi    a8, 56
  movi.n  a9, 0
  s32i.n  a8, a1, 144
  s32i.n  a9, a1, 148
  l32i.n  a2, a1, 144
  l32i.n  a3, a1, 148
  call0   cssc_obj_alloc
  s32i.n  a2, a1, 152
  l32i.n  a3, a1, 136
  l32i.n  a2, a1, 152
  call0   cssc_obj_VideoDriver_ctor
  addi    a8, a1, 8    # object_handle slot[19]
  s32i.n  a8, a1, 156
  l32i.n  a8, a1, 156
  l32i.n  a9, a1, 152
  s32i.n  a9, a8, 0
  l32i.n  a8, a1, 156
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 160
  l32i.n  a2, a1, 160
  call0   cssc_obj_VideoDriver_boot
  movi.n  a8, 0
  movi.n  a9, 0
  s32i.n  a8, a1, 168
  s32i.n  a9, a1, 172
  j       .Lcssc_user_main__while.cond.1
.Lcssc_user_main__while.cond.1:
  movi.n  a8, 1
  s32i.n  a8, a1, 176
  l32i.n  a8, a1, 176
  bnez    a8, .Lcssc_user_main__while.body.2
  j       .Lcssc_user_main__while.end.3
.Lcssc_user_main__while.body.2:
  l32i.n  a8, a1, 156
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 180
  l32i.n  a2, a1, 180
  call0   cssc_obj_VideoDriver_tick
  s32i.n  a2, a1, 184
  s32i.n  a3, a1, 188
  addi    a8, a1, 12    # int slot[25]
  s32i.n  a8, a1, 192
  l32i.n  a8, a1, 192
  l32i.n  a9, a1, 184
  l32i.n  a10, a1, 188
  s32i.n  a9,  a8, 0    # lo
  s32i.n  a10, a8, 4    # hi
  # slot_release(int) — POD, no-op
  movi.n  a8, 20
  movi.n  a9, 0
  s32i.n  a8, a1, 200
  s32i.n  a9, a1, 204
  l32i.n  a2, a1, 200
  l32i.n  a3, a1, 204
  call0   cssc_sleep_ms
  addi    a8, a1, 20    # float slot[27]
  s32i.n  a8, a1, 208
  call0   cssc_uptime
  s32i.n  a2, a1, 216
  s32i.n  a3, a1, 220
  l32i.n  a2, a1, 216
  l32i.n  a3, a1, 220
  call0   __truncdfsf2
  s32i.n  a2, a1, 224
  l32i.n  a8, a1, 208
  l32i.n  a9, a1, 224
  s32i.n  a9, a8, 0    # f32
  l32i.n  a8, a1, 208
  l32i.n  a9, a8, 0    # f32
  s32i.n  a9, a1, 228
  l32i.n  a8, a1, 156
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 232
  l32i.n  a8, a1, 232
  addi    a9, a8, 0    # cssc_obj_VideoDriver.field[0]
  s32i.n  a9, a1, 236
  l32i.n  a8, a1, 236
  l32i.n  a9,  a8, 0
  l32i.n  a10, a8, 4
  s32i.n  a9,  a1, 240
  s32i.n  a10, a1, 244
  l32i.n  a2, a1, 228
  l32i.n  a3, a1, 240
  call0   __gesf2
  movi.n  a10, 0
  bgez    a2, .LfcmpT1        # a2 >= 0 → true
  j       .LfcmpE2
.LfcmpT1:
  movi.n  a10, 1
.LfcmpE2:
  s32i.n  a10, a1, 248
  l32i.n  a8, a1, 248
  bnez    a8, .Lcssc_user_main__if.then.4
  j       .Lcssc_user_main__if.end.5
.Lcssc_user_main__while.end.3:
  l32i.n  a8, a1, 156
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 252
  l32i.n  a8, a1, 252
  addi    a9, a8, 0    # cssc_obj_VideoDriver.field[0]
  s32i.n  a9, a1, 256
  l32i.n  a8, a1, 252
  addi    a9, a8, 8    # cssc_obj_VideoDriver.field[1]
  s32i.n  a9, a1, 260
  l32i.n  a8, a1, 252
  addi    a9, a8, 12    # cssc_obj_VideoDriver.field[2]
  s32i.n  a9, a1, 264
  l32i.n  a8, a1, 252
  addi    a9, a8, 16    # cssc_obj_VideoDriver.field[3]
  s32i.n  a9, a1, 268
  l32i.n  a8, a1, 252
  addi    a9, a8, 24    # cssc_obj_VideoDriver.field[4]
  s32i.n  a9, a1, 272
  l32i.n  a8, a1, 252
  addi    a9, a8, 32    # cssc_obj_VideoDriver.field[5]
  s32i.n  a9, a1, 276
  l32i.n  a8, a1, 252
  addi    a9, a8, 40    # cssc_obj_VideoDriver.field[6]
  s32i.n  a9, a1, 280
  l32i.n  a8, a1, 280
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 284
  addi    a8, a1, 24    # oled slot[44]
  s32i.n  a8, a1, 288
  l32i.n  a8, a1, 288
  l32i.n  a9, a1, 284
  s32i.n  a9, a8, 0
  l32i.n  a8, a1, 288
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 292
  movi.n  a8, 0
  movi.n  a9, 0
  s32i.n  a8, a1, 296
  s32i.n  a9, a1, 300
  l32i.n  a4, a1, 296
  l32i.n  a5, a1, 300
  l32i.n  a2, a1, 292
  call0   cssc_oled_fill
  movi.n  a8, 0
  movi.n  a9, 0
  s32i.n  a8, a1, 304
  s32i.n  a9, a1, 308
  addi    a8, a1, 28    # string slot[48]
  s32i.n  a8, a1, 312
  l32r    a8, .LITS3
  s32i.n  a8, a1, 316
  movi.n  a8, 11
  movi.n  a9, 0
  s32i.n  a8, a1, 320
  s32i.n  a9, a1, 324
  l32i.n  a4, a1, 320
  l32i.n  a5, a1, 324
  l32i.n  a2, a1, 316
  call0   cssc_string_lit
  s32i.n  a2, a1, 328
  l32i.n  a8, a1, 312
  l32i.n  a9, a1, 328
  s32i.n  a9, a8, 0
  addi    a8, a1, 32    # string slot[52]
  s32i.n  a8, a1, 332
  l32r    a8, .LITS4
  s32i.n  a8, a1, 336
  movi.n  a8, 8
  movi.n  a9, 0
  s32i.n  a8, a1, 344
  s32i.n  a9, a1, 348
  l32i.n  a4, a1, 344
  l32i.n  a5, a1, 348
  l32i.n  a2, a1, 336
  call0   cssc_string_lit
  s32i.n  a2, a1, 352
  l32i.n  a8, a1, 332
  l32i.n  a9, a1, 352
  s32i.n  a9, a8, 0
  l32i.n  a8, a1, 288
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 356
  movi.n  a8, 4
  movi.n  a9, 0
  s32i.n  a8, a1, 360
  s32i.n  a9, a1, 364
  movi.n  a8, 24
  movi.n  a9, 0
  s32i.n  a8, a1, 368
  s32i.n  a9, a1, 372
  l32i.n  a8, a1, 312
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 376
  l32r    a8, .LIT5
  movi.n  a9, 0
  s32i.n  a8, a1, 384
  s32i.n  a9, a1, 388
  movi.n  a8, 1
  movi.n  a9, 0
  s32i.n  a8, a1, 392
  s32i.n  a9, a1, 396
  addi    a1, a1, -32    # outgoing stack args
  l32i.n  a6, a1, 400
  l32i.n  a7, a1, 404
  l32i.n  a4, a1, 392
  l32i.n  a5, a1, 396
  l32i.n  a2, a1, 388
  l32i.n  a8, a1, 408
  s32i.n  a8, a1, 0
  l32i.n  a8, a1, 416
  l32i.n  a9, a1, 420
  s32i.n  a8, a1, 8
  s32i.n  a9, a1, 12
  l32i.n  a8, a1, 424
  l32i.n  a9, a1, 428
  s32i.n  a8, a1, 16
  s32i.n  a9, a1, 20
  call0   cssc_oled_text
  addi    a1, a1, 32     # pop stack args
  movi.n  a8, 0
  movi.n  a9, 0
  s32i.n  a8, a1, 400
  s32i.n  a9, a1, 404
  l32i.n  a8, a1, 288
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 408
  movi.n  a8, 4
  movi.n  a9, 0
  s32i.n  a8, a1, 416
  s32i.n  a9, a1, 420
  movi    a8, 40
  movi.n  a9, 0
  s32i.n  a8, a1, 424
  s32i.n  a9, a1, 428
  l32i.n  a8, a1, 332
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 432
  l32r    a8, .LIT6
  movi.n  a9, 0
  s32i.n  a8, a1, 440
  s32i.n  a9, a1, 444
  movi.n  a8, 1
  movi.n  a9, 0
  s32i.n  a8, a1, 448
  s32i.n  a9, a1, 452
  addi    a1, a1, -32    # outgoing stack args
  l32i.n  a6, a1, 456
  l32i.n  a7, a1, 460
  l32i.n  a4, a1, 448
  l32i.n  a5, a1, 452
  l32i.n  a2, a1, 440
  l32i.n  a8, a1, 464
  s32i.n  a8, a1, 0
  l32i.n  a8, a1, 472
  l32i.n  a9, a1, 476
  s32i.n  a8, a1, 8
  s32i.n  a9, a1, 12
  l32i.n  a8, a1, 480
  l32i.n  a9, a1, 484
  s32i.n  a8, a1, 16
  s32i.n  a9, a1, 20
  call0   cssc_oled_text
  addi    a1, a1, 32     # pop stack args
  movi.n  a8, 0
  movi.n  a9, 0
  s32i.n  a8, a1, 456
  s32i.n  a9, a1, 460
  l32i.n  a8, a1, 288
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 464
  l32i.n  a2, a1, 464
  call0   cssc_oled_show
  movi.n  a8, 0
  movi.n  a9, 0
  s32i.n  a8, a1, 472
  s32i.n  a9, a1, 476
  l32i.n  a8, a1, 312
  l32i.n  a2, a8, 0     # arg0: heap ptr
  call0   cssc_release
  l32i.n  a8, a1, 332
  l32i.n  a2, a8, 0     # arg0: heap ptr
  call0   cssc_release
  l32i.n  a8, a1, 260
  l32i.n  a2, a8, 0     # arg0: heap ptr
  call0   cssc_release
  l32i.n  a8, a1, 264
  l32i.n  a2, a8, 0     # arg0: heap ptr
  call0   cssc_release
  # slot_release(int) — POD, no-op
  # slot_release(int) — POD, no-op
  # slot_release(int) — POD, no-op
  # slot_release(float) — POD, no-op
  l32i.n  a8, a1, 252
  addi    a9, a8, 8    # cssc_obj_VideoDriver.field[1]
  s32i.n  a9, a1, 480
  l32i.n  a8, a1, 480
  l32i.n  a2, a8, 0     # arg0: heap ptr
  call0   cssc_release
  l32i.n  a8, a1, 252
  addi    a9, a8, 12    # cssc_obj_VideoDriver.field[2]
  s32i.n  a9, a1, 484
  l32i.n  a8, a1, 484
  l32i.n  a2, a8, 0     # arg0: heap ptr
  call0   cssc_release
  l32i.n  a2, a1, 252
  call0   cssc_obj_free
  l32i.n  a8, a1, 52
  l32i.n  a2, a8, 0     # arg0: heap ptr
  call0   cssc_i2c_free
  l32i.n  a8, a1, 132
  l32i.n  a2, a8, 0     # arg0: heap ptr
  call0   cssc_tft_free
  call0   cssc_scope_pop    # bulk-free transient arena allocations
  l32i.n  a0, a1, 604
  addi    a1, a1, 608
  ret.n
.Lcssc_user_main__if.then.4:
  j       .Lcssc_user_main__while.end.3
.Lcssc_user_main__if.end.5:
  # slot_release(float) — POD, no-op
  j       .Lcssc_user_main__while.cond.1
  .literal_position
  .literal .LIT0, 400000
  .literal .LITS3, .str.10
  .literal .LITS4, .str.11
  .literal .LIT5, 65535
  .literal .LIT6, 65535
  .size   cssc_user_main, .-cssc_user_main

  .text
  .global cssc_obj_VideoDriver_ctor
  .type   cssc_obj_VideoDriver_ctor, @function
.align  4
cssc_obj_VideoDriver_ctor:
  addi    a1, a1, -224
  s32i.n  a0, a1, 220
  s32i.n  a12, a1, 204    # save callee-save a12
  s32i.n  a13, a1, 208    # save callee-save a13
  s32i.n  a14, a1, 212    # save callee-save a14
  s32i.n  a15, a1, 216    # save callee-save a15
  mov.n   a15, a2
  mov.n   a14, a3
.Lcssc_obj_VideoDriver_ctor__entry:
  mov.n   a8, a15
  addi    a13, a8, 0    # cssc_obj_VideoDriver.field[0]
  mov.n   a8, a15
  addi    a12, a8, 8    # cssc_obj_VideoDriver.field[1]
  mov.n   a8, a15
  addi    a9, a8, 12    # cssc_obj_VideoDriver.field[2]
  s32i.n  a9, a1, 8
  mov.n   a8, a15
  addi    a9, a8, 16    # cssc_obj_VideoDriver.field[3]
  s32i.n  a9, a1, 12
  mov.n   a8, a15
  addi    a9, a8, 24    # cssc_obj_VideoDriver.field[4]
  s32i.n  a9, a1, 16
  mov.n   a8, a15
  addi    a9, a8, 32    # cssc_obj_VideoDriver.field[5]
  s32i.n  a9, a1, 20
  mov.n   a8, a15
  addi    a9, a8, 40    # cssc_obj_VideoDriver.field[6]
  s32i.n  a9, a1, 24
  l32i.n  a8, a1, 24
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 28
  addi    a8, a1, 0    # oled slot[10]
  s32i.n  a8, a1, 32
  l32i.n  a8, a1, 32
  l32i.n  a9, a1, 28
  s32i.n  a9, a8, 0
  mov.n   a8, a15
  addi    a9, a8, 40    # cssc_obj_VideoDriver.field[6]
  s32i.n  a9, a1, 36
  l32i.n  a8, a1, 36
  mov.n   a9, a14
  s32i.n  a9, a8, 0
  movi.n  a8, 0
  l32r    a9, .LIT7
  s32i.n  a8, a1, 40
  s32i.n  a9, a1, 44
  l32i.n  a2, a1, 40
  l32i.n  a3, a1, 44
  call0   __truncdfsf2
  s32i.n  a2, a1, 48
  mov.n   a8, a13
  l32i.n  a9, a1, 48
  s32i.n  a9, a8, 0    # f32
  l32r    a8, .LITS8
  s32i.n  a8, a1, 52
  movi.n  a8, 18
  movi.n  a9, 0
  s32i.n  a8, a1, 56
  s32i.n  a9, a1, 60
  l32i.n  a4, a1, 56
  l32i.n  a5, a1, 60
  l32i.n  a2, a1, 52
  call0   cssc_string_lit
  s32i.n  a2, a1, 64
  mov.n   a8, a12
  l32i.n  a9, a1, 64
  s32i.n  a9, a8, 0
  l32r    a8, .LITS9
  s32i.n  a8, a1, 68
  mov.n   a8, a13
  l32i.n  a9, a8, 0    # f32
  s32i.n  a9, a1, 72
  l32r    a8, .LITS10
  s32i.n  a8, a1, 76
  movi.n  a8, 12
  movi.n  a9, 0
  s32i.n  a8, a1, 80
  s32i.n  a9, a1, 84
  l32i.n  a4, a1, 80
  l32i.n  a5, a1, 84
  l32i.n  a2, a1, 76
  call0   cssc_string_lit
  s32i.n  a2, a1, 88
  l32i.n  a2, a1, 72
  call0   __extendsfdf2
  s32i.n  a2, a1, 96
  s32i.n  a3, a1, 100
  l32i.n  a2, a1, 96
  l32i.n  a3, a1, 100
  call0   cssc_float_to_str
  s32i.n  a2, a1, 104
  l32i.n  a3, a1, 104
  l32i.n  a2, a1, 88
  call0   cssc_string_concat
  s32i.n  a2, a1, 108
  l32r    a8, .LITS11
  s32i.n  a8, a1, 112
  l32r    a8, .LITS12
  s32i.n  a8, a1, 116
  movi.n  a8, 1
  movi.n  a9, 0
  s32i.n  a8, a1, 120
  s32i.n  a9, a1, 124
  l32i.n  a4, a1, 120
  l32i.n  a5, a1, 124
  l32i.n  a2, a1, 116
  call0   cssc_string_lit
  s32i.n  a2, a1, 128
  l32i.n  a3, a1, 128
  l32i.n  a2, a1, 108
  call0   cssc_string_concat
  s32i.n  a2, a1, 132
  l32i.n  a8, a1, 8
  l32i.n  a9, a1, 132
  s32i.n  a9, a8, 0
  movi.n  a8, 0
  movi.n  a9, 0
  s32i.n  a8, a1, 136
  s32i.n  a9, a1, 140
  l32i.n  a8, a1, 12
  l32i.n  a9, a1, 136
  l32i.n  a10, a1, 140
  s32i.n  a9,  a8, 0    # lo
  s32i.n  a10, a8, 4    # hi
  movi.n  a8, 0
  movi.n  a9, 0
  s32i.n  a8, a1, 144
  s32i.n  a9, a1, 148
  l32i.n  a8, a1, 16
  l32i.n  a9, a1, 144
  l32i.n  a10, a1, 148
  s32i.n  a9,  a8, 0    # lo
  s32i.n  a10, a8, 4    # hi
  movi.n  a8, 4
  movi.n  a9, 0
  s32i.n  a8, a1, 152
  s32i.n  a9, a1, 156
  l32i.n  a8, a1, 20
  l32i.n  a9, a1, 152
  l32i.n  a10, a1, 156
  s32i.n  a9,  a8, 0    # lo
  s32i.n  a10, a8, 4    # hi
  l32i.n  a15, a1, 216    # restore callee-save a15
  l32i.n  a14, a1, 212    # restore callee-save a14
  l32i.n  a13, a1, 208    # restore callee-save a13
  l32i.n  a12, a1, 204    # restore callee-save a12
  l32i.n  a0, a1, 220
  addi    a1, a1, 224
  ret.n
  .literal_position
  .literal .LIT7, 1079902208
  .literal .LITS8, .str.0
  .literal .LITS9, .str.1
  .literal .LITS10, .str.2
  .literal .LITS11, .str.3
  .literal .LITS12, .str.4
  .size   cssc_obj_VideoDriver_ctor, .-cssc_obj_VideoDriver_ctor

  .text
  .global cssc_obj_VideoDriver_boot
  .type   cssc_obj_VideoDriver_boot, @function
.align  4
cssc_obj_VideoDriver_boot:
  addi    a1, a1, -384
  s32i.n  a0, a1, 380
  s32i.n  a12, a1, 364    # save callee-save a12
  s32i.n  a13, a1, 368    # save callee-save a13
  s32i.n  a14, a1, 372    # save callee-save a14
  s32i.n  a15, a1, 376    # save callee-save a15
  call0   cssc_scope_push    # arena scope for transient allocations
  mov.n   a15, a2
.Lcssc_obj_VideoDriver_boot__entry:
  mov.n   a8, a15
  addi    a14, a8, 0    # cssc_obj_VideoDriver.field[0]
  mov.n   a8, a15
  addi    a13, a8, 8    # cssc_obj_VideoDriver.field[1]
  mov.n   a8, a15
  addi    a12, a8, 12    # cssc_obj_VideoDriver.field[2]
  mov.n   a8, a15
  addi    a9, a8, 16    # cssc_obj_VideoDriver.field[3]
  s32i.n  a9, a1, 8
  mov.n   a8, a15
  addi    a9, a8, 24    # cssc_obj_VideoDriver.field[4]
  s32i.n  a9, a1, 12
  mov.n   a8, a15
  addi    a9, a8, 32    # cssc_obj_VideoDriver.field[5]
  s32i.n  a9, a1, 16
  mov.n   a8, a15
  addi    a9, a8, 40    # cssc_obj_VideoDriver.field[6]
  s32i.n  a9, a1, 20
  l32i.n  a8, a1, 20
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 24
  addi    a8, a1, 0    # oled slot[9]
  s32i.n  a8, a1, 28
  l32i.n  a8, a1, 28
  l32i.n  a9, a1, 24
  s32i.n  a9, a8, 0
  l32i.n  a8, a1, 28
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 32
  l32i.n  a2, a1, 32
  call0   cssc_oled_begin
  movi.n  a8, 0
  movi.n  a9, 0
  s32i.n  a8, a1, 40
  s32i.n  a9, a1, 44
  l32i.n  a8, a1, 28
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 48
  movi.n  a8, 0
  movi.n  a9, 0
  s32i.n  a8, a1, 56
  s32i.n  a9, a1, 60
  l32i.n  a4, a1, 56
  l32i.n  a5, a1, 60
  l32i.n  a2, a1, 48
  call0   cssc_oled_fill
  movi.n  a8, 0
  movi.n  a9, 0
  s32i.n  a8, a1, 64
  s32i.n  a9, a1, 68
  l32i.n  a8, a1, 28
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 72
  movi.n  a8, 4
  movi.n  a9, 0
  s32i.n  a8, a1, 80
  s32i.n  a9, a1, 84
  movi.n  a8, 16
  movi.n  a9, 0
  s32i.n  a8, a1, 88
  s32i.n  a9, a1, 92
  mov.n   a8, a13
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 96
  l32r    a8, .LIT13
  movi.n  a9, 0
  s32i.n  a8, a1, 104
  s32i.n  a9, a1, 108
  movi.n  a8, 1
  movi.n  a9, 0
  s32i.n  a8, a1, 112
  s32i.n  a9, a1, 116
  addi    a1, a1, -32    # outgoing stack args
  l32i.n  a6, a1, 120
  l32i.n  a7, a1, 124
  l32i.n  a4, a1, 112
  l32i.n  a5, a1, 116
  l32i.n  a2, a1, 104
  l32i.n  a8, a1, 128
  s32i.n  a8, a1, 0
  l32i.n  a8, a1, 136
  l32i.n  a9, a1, 140
  s32i.n  a8, a1, 8
  s32i.n  a9, a1, 12
  l32i.n  a8, a1, 144
  l32i.n  a9, a1, 148
  s32i.n  a8, a1, 16
  s32i.n  a9, a1, 20
  call0   cssc_oled_text
  addi    a1, a1, 32     # pop stack args
  movi.n  a8, 0
  movi.n  a9, 0
  s32i.n  a8, a1, 120
  s32i.n  a9, a1, 124
  l32i.n  a8, a1, 28
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 128
  movi.n  a8, 4
  movi.n  a9, 0
  s32i.n  a8, a1, 136
  s32i.n  a9, a1, 140
  movi    a8, 32
  movi.n  a9, 0
  s32i.n  a8, a1, 144
  s32i.n  a9, a1, 148
  mov.n   a8, a12
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 152
  l32r    a8, .LIT14
  movi.n  a9, 0
  s32i.n  a8, a1, 160
  s32i.n  a9, a1, 164
  movi.n  a8, 1
  movi.n  a9, 0
  s32i.n  a8, a1, 168
  s32i.n  a9, a1, 172
  addi    a1, a1, -32    # outgoing stack args
  l32i.n  a6, a1, 176
  l32i.n  a7, a1, 180
  l32i.n  a4, a1, 168
  l32i.n  a5, a1, 172
  l32i.n  a2, a1, 160
  l32i.n  a8, a1, 184
  s32i.n  a8, a1, 0
  l32i.n  a8, a1, 192
  l32i.n  a9, a1, 196
  s32i.n  a8, a1, 8
  s32i.n  a9, a1, 12
  l32i.n  a8, a1, 200
  l32i.n  a9, a1, 204
  s32i.n  a8, a1, 16
  s32i.n  a9, a1, 20
  call0   cssc_oled_text
  addi    a1, a1, 32     # pop stack args
  movi.n  a8, 0
  movi.n  a9, 0
  s32i.n  a8, a1, 176
  s32i.n  a9, a1, 180
  addi    a8, a1, 4    # string slot[29]
  s32i.n  a8, a1, 184
  l32r    a8, .LITS15
  s32i.n  a8, a1, 188
  movi.n  a8, 7
  movi.n  a9, 0
  s32i.n  a8, a1, 192
  s32i.n  a9, a1, 196
  l32i.n  a4, a1, 192
  l32i.n  a5, a1, 196
  l32i.n  a2, a1, 188
  call0   cssc_string_lit
  s32i.n  a2, a1, 200
  l32i.n  a8, a1, 184
  l32i.n  a9, a1, 200
  s32i.n  a9, a8, 0
  l32i.n  a8, a1, 28
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 204
  movi.n  a8, 4
  movi.n  a9, 0
  s32i.n  a8, a1, 208
  s32i.n  a9, a1, 212
  movi    a8, 48
  movi.n  a9, 0
  s32i.n  a8, a1, 216
  s32i.n  a9, a1, 220
  l32i.n  a8, a1, 184
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 224
  l32r    a8, .LIT16
  movi.n  a9, 0
  s32i.n  a8, a1, 232
  s32i.n  a9, a1, 236
  movi.n  a8, 1
  movi.n  a9, 0
  s32i.n  a8, a1, 240
  s32i.n  a9, a1, 244
  addi    a1, a1, -32    # outgoing stack args
  l32i.n  a6, a1, 248
  l32i.n  a7, a1, 252
  l32i.n  a4, a1, 240
  l32i.n  a5, a1, 244
  l32i.n  a2, a1, 236
  l32i.n  a8, a1, 256
  s32i.n  a8, a1, 0
  l32i.n  a8, a1, 264
  l32i.n  a9, a1, 268
  s32i.n  a8, a1, 8
  s32i.n  a9, a1, 12
  l32i.n  a8, a1, 272
  l32i.n  a9, a1, 276
  s32i.n  a8, a1, 16
  s32i.n  a9, a1, 20
  call0   cssc_oled_text
  addi    a1, a1, 32     # pop stack args
  movi.n  a8, 0
  movi.n  a9, 0
  s32i.n  a8, a1, 248
  s32i.n  a9, a1, 252
  l32i.n  a8, a1, 28
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 256
  l32i.n  a2, a1, 256
  call0   cssc_oled_show
  movi.n  a8, 0
  movi.n  a9, 0
  s32i.n  a8, a1, 264
  s32i.n  a9, a1, 268
  l32i.n  a8, a1, 184
  l32i.n  a2, a8, 0     # arg0: heap ptr
  call0   cssc_release
  l32r    a8, .LIT17
  movi.n  a9, 0
  s32i.n  a8, a1, 272
  s32i.n  a9, a1, 276
  l32i.n  a2, a1, 272
  l32i.n  a3, a1, 276
  call0   cssc_sleep_ms
  call0   cssc_scope_pop    # bulk-free transient arena allocations
  l32i.n  a15, a1, 376    # restore callee-save a15
  l32i.n  a14, a1, 372    # restore callee-save a14
  l32i.n  a13, a1, 368    # restore callee-save a13
  l32i.n  a12, a1, 364    # restore callee-save a12
  l32i.n  a0, a1, 380
  addi    a1, a1, 384
  ret.n
  .literal_position
  .literal .LIT13, 65535
  .literal .LIT14, 65535
  .literal .LITS15, .str.5
  .literal .LIT16, 65535
  .literal .LIT17, 2500
  .size   cssc_obj_VideoDriver_boot, .-cssc_obj_VideoDriver_boot

  .text
  .global cssc_obj_VideoDriver_tick
  .type   cssc_obj_VideoDriver_tick, @function
.align  4
cssc_obj_VideoDriver_tick:
  addi    a1, a1, -624
  s32i.n  a0, a1, 620
  s32i.n  a12, a1, 604    # save callee-save a12
  s32i.n  a13, a1, 608    # save callee-save a13
  s32i.n  a14, a1, 612    # save callee-save a14
  s32i.n  a15, a1, 616    # save callee-save a15
  call0   cssc_scope_push    # arena scope for transient allocations
  mov.n   a15, a2
.Lcssc_obj_VideoDriver_tick__entry:
  mov.n   a8, a15
  addi    a14, a8, 0    # cssc_obj_VideoDriver.field[0]
  mov.n   a8, a15
  addi    a13, a8, 8    # cssc_obj_VideoDriver.field[1]
  mov.n   a8, a15
  addi    a12, a8, 12    # cssc_obj_VideoDriver.field[2]
  mov.n   a8, a15
  addi    a9, a8, 16    # cssc_obj_VideoDriver.field[3]
  s32i.n  a9, a1, 8
  mov.n   a8, a15
  addi    a9, a8, 24    # cssc_obj_VideoDriver.field[4]
  s32i.n  a9, a1, 12
  mov.n   a8, a15
  addi    a9, a8, 32    # cssc_obj_VideoDriver.field[5]
  s32i.n  a9, a1, 16
  mov.n   a8, a15
  addi    a9, a8, 40    # cssc_obj_VideoDriver.field[6]
  s32i.n  a9, a1, 20
  l32i.n  a8, a1, 20
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 24
  addi    a8, a1, 0    # oled slot[9]
  s32i.n  a8, a1, 28
  l32i.n  a8, a1, 28
  l32i.n  a9, a1, 24
  s32i.n  a9, a8, 0
  l32i.n  a8, a1, 28
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 32
  movi.n  a8, 0
  movi.n  a9, 0
  s32i.n  a8, a1, 40
  s32i.n  a9, a1, 44
  l32i.n  a4, a1, 40
  l32i.n  a5, a1, 44
  l32i.n  a2, a1, 32
  call0   cssc_oled_fill
  movi.n  a8, 0
  movi.n  a9, 0
  s32i.n  a8, a1, 48
  s32i.n  a9, a1, 52
  l32i.n  a8, a1, 28
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 56
  movi.n  a8, 0
  movi.n  a9, 0
  s32i.n  a8, a1, 64
  s32i.n  a9, a1, 68
  movi.n  a8, 25
  movi.n  a9, 0
  s32i.n  a8, a1, 72
  s32i.n  a9, a1, 76
  call0   cssc_uptime
  s32i.n  a2, a1, 80
  s32i.n  a3, a1, 84
  l32i.n  a2, a1, 80
  l32i.n  a3, a1, 84
  call0   cssc_float_to_str
  s32i.n  a2, a1, 88
  l32r    a8, .LIT18
  movi.n  a9, 0
  s32i.n  a8, a1, 96
  s32i.n  a9, a1, 100
  movi.n  a8, 1
  movi.n  a9, 0
  s32i.n  a8, a1, 104
  s32i.n  a9, a1, 108
  addi    a1, a1, -32    # outgoing stack args
  l32i.n  a6, a1, 104
  l32i.n  a7, a1, 108
  l32i.n  a4, a1, 96
  l32i.n  a5, a1, 100
  l32i.n  a2, a1, 88
  l32i.n  a8, a1, 120
  s32i.n  a8, a1, 0
  l32i.n  a8, a1, 128
  l32i.n  a9, a1, 132
  s32i.n  a8, a1, 8
  s32i.n  a9, a1, 12
  l32i.n  a8, a1, 136
  l32i.n  a9, a1, 140
  s32i.n  a8, a1, 16
  s32i.n  a9, a1, 20
  call0   cssc_oled_text
  addi    a1, a1, 32     # pop stack args
  movi.n  a8, 0
  movi.n  a9, 0
  s32i.n  a8, a1, 112
  s32i.n  a9, a1, 116
  addi    a8, a1, 4    # string slot[21]
  s32i.n  a8, a1, 120
  l32r    a8, .LITS19
  s32i.n  a8, a1, 124
  l32i.n  a8, a1, 8
  l32i.n  a9,  a8, 0
  l32i.n  a10, a8, 4
  s32i.n  a9,  a1, 128
  s32i.n  a10, a1, 132
  l32r    a8, .LITS20
  s32i.n  a8, a1, 136
  movi.n  a8, 15
  movi.n  a9, 0
  s32i.n  a8, a1, 144
  s32i.n  a9, a1, 148
  l32i.n  a4, a1, 144
  l32i.n  a5, a1, 148
  l32i.n  a2, a1, 136
  call0   cssc_string_lit
  s32i.n  a2, a1, 152
  l32i.n  a2, a1, 128
  l32i.n  a3, a1, 132
  call0   cssc_int_to_str
  s32i.n  a2, a1, 156
  l32i.n  a3, a1, 156
  l32i.n  a2, a1, 152
  call0   cssc_string_concat
  s32i.n  a2, a1, 160
  l32r    a8, .LITS21
  s32i.n  a8, a1, 164
  l32r    a8, .LITS22
  s32i.n  a8, a1, 168
  movi.n  a8, 1
  movi.n  a9, 0
  s32i.n  a8, a1, 176
  s32i.n  a9, a1, 180
  l32i.n  a4, a1, 176
  l32i.n  a5, a1, 180
  l32i.n  a2, a1, 168
  call0   cssc_string_lit
  s32i.n  a2, a1, 184
  l32i.n  a3, a1, 184
  l32i.n  a2, a1, 160
  call0   cssc_string_concat
  s32i.n  a2, a1, 188
  l32i.n  a8, a1, 120
  l32i.n  a9, a1, 188
  s32i.n  a9, a8, 0
  l32i.n  a8, a1, 28
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 192
  movi.n  a8, 0
  movi.n  a9, 0
  s32i.n  a8, a1, 200
  s32i.n  a9, a1, 204
  movi.n  a8, 0
  movi.n  a9, 0
  s32i.n  a8, a1, 208
  s32i.n  a9, a1, 212
  l32i.n  a8, a1, 120
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 216
  l32r    a8, .LIT23
  movi.n  a9, 0
  s32i.n  a8, a1, 224
  s32i.n  a9, a1, 228
  movi.n  a8, 1
  movi.n  a9, 0
  s32i.n  a8, a1, 232
  s32i.n  a9, a1, 236
  addi    a1, a1, -32    # outgoing stack args
  l32i.n  a6, a1, 240
  l32i.n  a7, a1, 244
  l32i.n  a4, a1, 232
  l32i.n  a5, a1, 236
  l32i.n  a2, a1, 224
  l32i.n  a8, a1, 248
  s32i.n  a8, a1, 0
  l32i.n  a8, a1, 256
  l32i.n  a9, a1, 260
  s32i.n  a8, a1, 8
  s32i.n  a9, a1, 12
  l32i.n  a8, a1, 264
  l32i.n  a9, a1, 268
  s32i.n  a8, a1, 16
  s32i.n  a9, a1, 20
  call0   cssc_oled_text
  addi    a1, a1, 32     # pop stack args
  movi.n  a8, 0
  movi.n  a9, 0
  s32i.n  a8, a1, 240
  s32i.n  a9, a1, 244
  l32i.n  a8, a1, 28
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 248
  movi.n  a8, 0
  movi.n  a9, 0
  s32i.n  a8, a1, 256
  s32i.n  a9, a1, 260
  movi    a8, 39
  movi.n  a9, 0
  s32i.n  a8, a1, 264
  s32i.n  a9, a1, 268
  movi    a8, 127
  movi.n  a9, 0
  s32i.n  a8, a1, 272
  s32i.n  a9, a1, 276
  movi    a8, 38
  movi.n  a9, 0
  s32i.n  a8, a1, 280
  s32i.n  a9, a1, 284
  l32r    a8, .LIT24
  movi.n  a9, 0
  s32i.n  a8, a1, 288
  s32i.n  a9, a1, 292
  addi    a1, a1, -32    # outgoing stack args
  l32i.n  a6, a1, 296
  l32i.n  a7, a1, 300
  l32i.n  a4, a1, 288
  l32i.n  a5, a1, 292
  l32i.n  a2, a1, 280
  l32i.n  a8, a1, 304
  l32i.n  a9, a1, 308
  s32i.n  a8, a1, 0
  s32i.n  a9, a1, 4
  l32i.n  a8, a1, 312
  l32i.n  a9, a1, 316
  s32i.n  a8, a1, 8
  s32i.n  a9, a1, 12
  l32i.n  a8, a1, 320
  l32i.n  a9, a1, 324
  s32i.n  a8, a1, 16
  s32i.n  a9, a1, 20
  call0   cssc_oled_line
  addi    a1, a1, 32     # pop stack args
  movi.n  a8, 0
  movi.n  a9, 0
  s32i.n  a8, a1, 296
  s32i.n  a9, a1, 300
  l32i.n  a8, a1, 28
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 304
  l32i.n  a8, a1, 12
  l32i.n  a9,  a8, 0
  l32i.n  a10, a8, 4
  s32i.n  a9,  a1, 312
  s32i.n  a10, a1, 316
  movi    a8, 46
  movi.n  a9, 0
  s32i.n  a8, a1, 320
  s32i.n  a9, a1, 324
  movi.n  a8, 14
  movi.n  a9, 0
  s32i.n  a8, a1, 328
  s32i.n  a9, a1, 332
  movi.n  a8, 14
  movi.n  a9, 0
  s32i.n  a8, a1, 336
  s32i.n  a9, a1, 340
  l32r    a8, .LIT25
  movi.n  a9, 0
  s32i.n  a8, a1, 344
  s32i.n  a9, a1, 348
  addi    a1, a1, -32    # outgoing stack args
  l32i.n  a6, a1, 352
  l32i.n  a7, a1, 356
  l32i.n  a4, a1, 344
  l32i.n  a5, a1, 348
  l32i.n  a2, a1, 336
  l32i.n  a8, a1, 360
  l32i.n  a9, a1, 364
  s32i.n  a8, a1, 0
  s32i.n  a9, a1, 4
  l32i.n  a8, a1, 368
  l32i.n  a9, a1, 372
  s32i.n  a8, a1, 8
  s32i.n  a9, a1, 12
  l32i.n  a8, a1, 376
  l32i.n  a9, a1, 380
  s32i.n  a8, a1, 16
  s32i.n  a9, a1, 20
  call0   cssc_oled_fillrect
  addi    a1, a1, 32     # pop stack args
  movi.n  a8, 0
  movi.n  a9, 0
  s32i.n  a8, a1, 352
  s32i.n  a9, a1, 356
  l32i.n  a8, a1, 12
  l32i.n  a9,  a8, 0
  l32i.n  a10, a8, 4
  s32i.n  a9,  a1, 360
  s32i.n  a10, a1, 364
  l32i.n  a8, a1, 16
  l32i.n  a9,  a8, 0
  l32i.n  a10, a8, 4
  s32i.n  a9,  a1, 368
  s32i.n  a10, a1, 372
  l32i.n  a8, a1, 360
  l32i.n  a9, a1, 364
  l32i.n  a10, a1, 368
  l32i.n  a11, a1, 372
  add     a2, a8, a10
  bgeu    a2, a8, .Laddnc26
  addi    a8, a9, 1     # carry-in to hi
  j       .Laddhi27
.Laddnc26:
  mov.n   a8, a9
.Laddhi27:
  add     a3, a8, a11
  s32i.n  a2, a1, 376
  s32i.n  a3, a1, 380
  l32i.n  a8, a1, 12
  l32i.n  a9, a1, 376
  l32i.n  a10, a1, 380
  s32i.n  a9,  a8, 0    # lo
  s32i.n  a10, a8, 4    # hi
  l32i.n  a8, a1, 12
  l32i.n  a9,  a8, 0
  l32i.n  a10, a8, 4
  s32i.n  a9,  a1, 384
  s32i.n  a10, a1, 388
  movi    a8, 114
  movi.n  a9, 0
  s32i.n  a8, a1, 392
  s32i.n  a9, a1, 396
  l32i.n  a8, a1, 384
  l32i.n  a9, a1, 388
  l32i.n  a10, a1, 392
  l32i.n  a11, a1, 396
  movi.n  a2, 0
  blt     a11, a9, .LcmpT28     # hi signed-less
  blt     a9, a11, .LcmpE29      # hi signed-greater
  bltu    a10, a8, .LcmpT28 # lo unsigned-less
  j       .LcmpE29
.LcmpT28:
  movi.n  a2, 1
.LcmpE29:
  s32i.n  a2, a1, 400
  l32i.n  a8, a1, 400
  bnez    a8, .Lcssc_obj_VideoDriver_tick__if.then.1
  j       .Lcssc_obj_VideoDriver_tick__if.end.2
.Lcssc_obj_VideoDriver_tick__if.then.1:
  movi.n  a8, 4
  movi.n  a9, 0
  s32i.n  a8, a1, 408
  s32i.n  a9, a1, 412
  movi.n  a8, 0
  movi.n  a9, 0
  s32i.n  a8, a1, 416
  s32i.n  a9, a1, 420
  l32i.n  a8, a1, 416
  l32i.n  a9, a1, 420
  l32i.n  a10, a1, 408
  l32i.n  a11, a1, 412
  sub     a2, a8, a10
  bgeu    a8, a10, .Lsubnb30
  addi    a8, a9, -1    # borrow-out of hi
  j       .Lsubhi31
.Lsubnb30:
  mov.n   a8, a9
.Lsubhi31:
  sub     a3, a8, a11
  s32i.n  a2, a1, 424
  s32i.n  a3, a1, 428
  l32i.n  a8, a1, 16
  l32i.n  a9, a1, 424
  l32i.n  a10, a1, 428
  s32i.n  a9,  a8, 0    # lo
  s32i.n  a10, a8, 4    # hi
  j       .Lcssc_obj_VideoDriver_tick__if.end.2
.Lcssc_obj_VideoDriver_tick__if.end.2:
  l32i.n  a8, a1, 12
  l32i.n  a9,  a8, 0
  l32i.n  a10, a8, 4
  s32i.n  a9,  a1, 432
  s32i.n  a10, a1, 436
  movi.n  a8, 0
  movi.n  a9, 0
  s32i.n  a8, a1, 440
  s32i.n  a9, a1, 444
  l32i.n  a8, a1, 432
  l32i.n  a9, a1, 436
  l32i.n  a10, a1, 440
  l32i.n  a11, a1, 444
  movi.n  a2, 0
  blt     a9, a11, .LcmpT32     # hi signed-less
  blt     a11, a9, .LcmpE33      # hi signed-greater
  bltu    a8, a10, .LcmpT32 # lo unsigned-less
  j       .LcmpE33
.LcmpT32:
  movi.n  a2, 1
.LcmpE33:
  s32i.n  a2, a1, 448
  l32i.n  a8, a1, 448
  bnez    a8, .Lcssc_obj_VideoDriver_tick__if.then.3
  j       .Lcssc_obj_VideoDriver_tick__if.end.4
.Lcssc_obj_VideoDriver_tick__if.then.3:
  movi.n  a8, 4
  movi.n  a9, 0
  s32i.n  a8, a1, 456
  s32i.n  a9, a1, 460
  l32i.n  a8, a1, 16
  l32i.n  a9, a1, 456
  l32i.n  a10, a1, 460
  s32i.n  a9,  a8, 0    # lo
  s32i.n  a10, a8, 4    # hi
  j       .Lcssc_obj_VideoDriver_tick__if.end.4
.Lcssc_obj_VideoDriver_tick__if.end.4:
  l32i.n  a8, a1, 28
  l32i.n  a9, a8, 0
  s32i.n  a9, a1, 464
  l32i.n  a2, a1, 464
  call0   cssc_oled_show
  movi.n  a8, 0
  movi.n  a9, 0
  s32i.n  a8, a1, 472
  s32i.n  a9, a1, 476
  l32i.n  a8, a1, 8
  l32i.n  a9,  a8, 0
  l32i.n  a10, a8, 4
  s32i.n  a9,  a1, 480
  s32i.n  a10, a1, 484
  movi.n  a8, 1
  movi.n  a9, 0
  s32i.n  a8, a1, 488
  s32i.n  a9, a1, 492
  l32i.n  a8, a1, 480
  l32i.n  a9, a1, 484
  l32i.n  a10, a1, 488
  l32i.n  a11, a1, 492
  add     a2, a8, a10
  bgeu    a2, a8, .Laddnc34
  addi    a8, a9, 1     # carry-in to hi
  j       .Laddhi35
.Laddnc34:
  mov.n   a8, a9
.Laddhi35:
  add     a3, a8, a11
  s32i.n  a2, a1, 496
  s32i.n  a3, a1, 500
  l32i.n  a8, a1, 8
  l32i.n  a9, a1, 496
  l32i.n  a10, a1, 500
  s32i.n  a9,  a8, 0    # lo
  s32i.n  a10, a8, 4    # hi
  l32i.n  a8, a1, 8
  l32i.n  a9,  a8, 0
  l32i.n  a10, a8, 4
  s32i.n  a9,  a1, 504
  s32i.n  a10, a1, 508
  l32i.n  a2, a1, 504
  l32i.n  a3, a1, 508
  call0   cssc_scope_pop    # bulk-free transient arena allocations
  l32i.n  a15, a1, 616    # restore callee-save a15
  l32i.n  a14, a1, 612    # restore callee-save a14
  l32i.n  a13, a1, 608    # restore callee-save a13
  l32i.n  a12, a1, 604    # restore callee-save a12
  l32i.n  a0, a1, 620
  addi    a1, a1, 624
  ret.n
  l32i.n  a8, a1, 120
  l32i.n  a2, a8, 0     # arg0: heap ptr
  call0   cssc_release
  movi.n  a8, 0
  movi.n  a9, 0
  s32i.n  a8, a1, 512
  s32i.n  a9, a1, 516
  l32i.n  a2, a1, 512
  l32i.n  a3, a1, 516
  call0   cssc_scope_pop    # bulk-free transient arena allocations
  l32i.n  a15, a1, 616    # restore callee-save a15
  l32i.n  a14, a1, 612    # restore callee-save a14
  l32i.n  a13, a1, 608    # restore callee-save a13
  l32i.n  a12, a1, 604    # restore callee-save a12
  l32i.n  a0, a1, 620
  addi    a1, a1, 624
  ret.n
  .literal_position
  .literal .LIT18, 65535
  .literal .LITS19, .str.6
  .literal .LITS20, .str.7
  .literal .LITS21, .str.8
  .literal .LITS22, .str.9
  .literal .LIT23, 65535
  .literal .LIT24, 65535
  .literal .LIT25, 65535
  .size   cssc_obj_VideoDriver_tick, .-cssc_obj_VideoDriver_tick

  .global _cssc_user_entry
  .type   _cssc_user_entry, @function
.align  4
_cssc_user_entry:
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  call0   cssc_runtime_init
  call0   cssc_user_main
  call0   cssc_runtime_shutdown
  l32i.n  a0, a1, 12
  addi    a1, a1, 16
  ret.n
  .size   _cssc_user_entry, .-_cssc_user_entry

  .text
  # ===========================================================================
  # Hand-written Xtensa LX106 runtime — minimal surface (Phase A8).
  # CALL0 ABI: args in a2..a7, return value in a2.
  # ===========================================================================

  .global cssc_runtime_init
  .type   cssc_runtime_init, @function
.align  4
cssc_runtime_init:
  # Disable the hardware watchdog. On bare metal we don't pet it,
  # and the ROM bootloader leaves it enabled with a ~3.2 s timeout.
  # Without this the chip would reset every few seconds mid-program.
  #   WDT_CTL_REG (0x60000900) bit 0 = WDT_EN. Clear to disable.
  l32r    a8, .Lwdt_ctl_addr
  movi.n  a9, 0
  s32i.n  a9, a8, 0
  memw
  # Seed `cssc_last_ccount` with the current CCOUNT so `cssc_uptime`'s
  # delta math starts from zero on the first call. Without this seed,
  # the first call would treat the whole power-on CCOUNT value as
  # elapsed time and bump cssc_uptime_ms by ~tens of seconds.
  rsr.ccount a9
  l32r    a8, .Lrt_lastcc_addr
  s32i.n  a9, a8, 0
  ret.n
  .literal_position
  .literal .Lwdt_ctl_addr,    0x60000900
  .literal .Lrt_lastcc_addr,  cssc_last_ccount
  .size cssc_runtime_init, .-cssc_runtime_init

  .global cssc_runtime_shutdown
  .type   cssc_runtime_shutdown, @function
.align  4
cssc_runtime_shutdown:
  ret.n
  .size cssc_runtime_shutdown, .-cssc_runtime_shutdown

  # --- UART0 TX FIFO single-byte write -------------------------------------
  # cssc_uart_putc(i8 b in a2)
  #
  # ESP8266 UART0 TX FIFO is at 0x60000000. Writing a byte enqueues it.
  # We don't check FIFO full status — for ASCII-character cadence at
  # 74880 baud the FIFO drains faster than we can push.
  .global cssc_uart_putc
  .type   cssc_uart_putc, @function
.align  4
cssc_uart_putc:
  l32r    a8, .Luart0_tx_addr
  s8i     a2, a8, 0
  memw                              # ensure write reaches the MMIO bus
  ret.n
  .literal_position
  .literal .Luart0_tx_addr, 0x60000000
  .size cssc_uart_putc, .-cssc_uart_putc

  # --- cssc_print_int(i64 n in a2:a3) --------------------------------------
  # Format `n` (low 32 bits) as decimal + trailing '\n', byte-by-byte
  # through cssc_uart_putc. Uses an on-stack 12-byte scratch buffer.
  .global cssc_print_int
  .type   cssc_print_int, @function
.align  4
cssc_print_int:
  # CALL0 ABI: a0-a11 are caller-save (clobbered by every call0).
  # a12-a15 are callee-save. Values that must live across calls
  # (n, the scratch-buffer pointer, the digit count) go in a12-a14;
  # we save them in the prologue and restore in the epilogue.
  addi    a1, a1, -32
  s32i.n  a0,  a1, 28                # return PC
  s32i.n  a12, a1, 24                # a12 = n
  s32i.n  a13, a1, 20                # a13 = scratch ptr
  s32i.n  a14, a1, 16                # a14 = digit count
  mov.n   a12, a2                    # a12 = n (low 32 bits)

  # Handle negative
  movi.n  a8, 0
  bge     a12, a8, 1f
  movi.n  a2, 45                     # '-'
  call0   cssc_uart_putc
  neg     a12, a12
1:
  # Special-case 0 → emit single '0'
  movi.n  a8, 0
  bne     a12, a8, 2f
  movi.n  a2, 48                     # '0'
  call0   cssc_uart_putc
  j       9f
2:
  # Convert n to ASCII digits in reverse into [a1+0..a1+11].
  # a13 = ptr cursor (callee-save; starts at a1+11 going down).
  addi    a13, a1, 11
  movi.n  a14, 0                     # digit count (callee-save)
3:
  # n_div = n / 10 via __divsi3 (caller-save scratch is clobbered).
  mov.n   a2, a12
  movi.n  a3, 10
  call0   __divsi3
  mov.n   a8, a2                     # a8 = quotient (transient, used below)
  # remainder = n - quotient*10
  mov.n   a2, a8
  movi.n  a3, 10
  call0   __mulsi3                   # a2 = quotient*10
  sub     a9, a12, a2                # a9 = remainder digit (0..9)
  addi    a9, a9, 48                 # ASCII digit
  s8i     a9, a13, 0                 # *(a13) = digit char
  addi    a13, a13, -1
  addi    a14, a14, 1
  # n = quotient. Recompute since a8 may have been clobbered.
  mov.n   a2, a12
  movi.n  a3, 10
  call0   __divsi3
  mov.n   a12, a2                    # a12 = next n
  movi.n  a10, 0
  bne     a12, a10, 3b
  # Emit digits forward.
  addi    a13, a13, 1                # first valid char
4:
  l8ui    a2, a13, 0
  call0   cssc_uart_putc
  addi    a13, a13, 1
  addi    a14, a14, -1
  movi.n  a10, 0
  bne     a14, a10, 4b
9:
  movi.n  a2, 10                     # '\n'
  call0   cssc_uart_putc
  l32i.n  a14, a1, 16
  l32i.n  a13, a1, 20
  l32i.n  a12, a1, 24
  l32i.n  a0,  a1, 28
  addi    a1, a1, 32
  ret.n
  .size cssc_print_int, .-cssc_print_int

  # A.3.17: `cssc::outln()` with no args calls this — emits a single '\n'
  # via UART. No formatting, no buffer needed.
  .global cssc_print_newline
  .type   cssc_print_newline, @function
.align  4
cssc_print_newline:
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  movi.n  a2, 10
  call0   cssc_uart_putc
  l32i.n  a0, a1, 12
  addi    a1, a1, 16
  ret.n
  .size cssc_print_newline, .-cssc_print_newline

  # --- cssc_print_bool(i1 b in a2) -----------------------------------------
  .global cssc_print_bool
  .type   cssc_print_bool, @function
.align  4
cssc_print_bool:
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  movi.n  a8, 0
  beq     a2, a8, 7f
  # print "true\n"
  movi.n  a2, 116                    # t
  call0   cssc_uart_putc
  movi.n  a2, 114                    # r
  call0   cssc_uart_putc
  movi.n  a2, 117                    # u
  call0   cssc_uart_putc
  movi.n  a2, 101                    # e
  call0   cssc_uart_putc
  j       8f
7:
  # print "false\n"
  movi.n  a2, 102                    # f
  call0   cssc_uart_putc
  movi.n  a2, 97                     # a
  call0   cssc_uart_putc
  movi.n  a2, 108                    # l
  call0   cssc_uart_putc
  movi.n  a2, 115                    # s
  call0   cssc_uart_putc
  movi.n  a2, 101                    # e
  call0   cssc_uart_putc
8:
  movi.n  a2, 10
  call0   cssc_uart_putc
  l32i.n  a0, a1, 12
  addi    a1, a1, 16
  ret.n
  .size cssc_print_bool, .-cssc_print_bool

  # --- cssc_print_str(ptr s, i64 len) --------------------------------------
  # Byte-stream a UTF-8 buffer. No trailing newline (cssc::out semantics);
  # the outln-side prints its own '\n' via a separate call.
  .global cssc_print_str
  .type   cssc_print_str, @function
.align  4
cssc_print_str:
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  s32i.n  a12, a1, 8
  s32i.n  a13, a1, 4
  mov.n   a12, a2                    # ptr
  mov.n   a13, a3                    # len
5:
  movi.n  a8, 0
  beq     a13, a8, 6f
  l8ui    a2, a12, 0
  call0   cssc_uart_putc
  addi    a12, a12, 1
  addi    a13, a13, -1
  j       5b
6:
  l32i.n  a13, a1, 4
  l32i.n  a12, a1, 8
  l32i.n  a0,  a1, 12
  addi    a1, a1, 16
  ret.n
  .size cssc_print_str, .-cssc_print_str

  # ===========================================================================
  # B4 — DRAM bump allocator
  # ===========================================================================
  #
  # A static .bss arena. The first byte of `cssc_arena` is the bump
  # cursor anchor; `cssc_arena_top` is the address just past the
  # arena. cssc_obj_alloc(size) bumps the cursor by `size` (8-byte-
  # aligned). cssc_obj_free is a no-op — the arena lives forever.
  #
  # For a 64 KB arena at the default DRAM start (chosen by the linker
  # via `.bss.cssc_arena`), the program gets ~64 KB of heap before
  # OOM. Out-of-memory traps via `cssc_panic`.
  .section .bss
  .global cssc_arena
  .align  8
cssc_arena:
  .space  65536           # 64 KB
cssc_arena_top:
  .global cssc_arena_off
  .align  4
cssc_arena_off:
  .word   0               # bump cursor (byte offset into cssc_arena)

  # Scope-frame stack: a 32-deep ring of saved arena cursors so each
  # emitted user function / object label can push a fresh sub-arena at
  # entry and bulk-free its allocations at exit. Without this, every
  # tick of a render loop accumulates leaked strings (e.g. the result
  # of `cssc_float_to_str(cssc::uptime())` passed straight to
  # `display.text` — a transient temp that has no named owner) and
  # the 64 KB arena fills in ~400 iterations.
  .global cssc_scope_stack
  .align  4
cssc_scope_stack:
  .space  128             # 32 frames × 4 bytes each
  .global cssc_scope_sp
  .align  4
cssc_scope_sp:
  .word   0               # depth (next-free slot index)

  .text

  # cssc_scope_push: save the current arena cursor onto the scope
  # stack. Called at the start of every emitter-produced label
  # method (boot, tick, free, custom) so its allocations live in a
  # dedicated sub-arena. Constructors do NOT call this — their
  # allocations are object-member values that have to outlive the
  # ctor call.
  #
  # CRITICAL: this is called from a function's PROLOGUE, BEFORE the
  # function has saved its argument registers (a2..a7) to its
  # frame. We MUST preserve every arg register verbatim, otherwise
  # we wipe the caller's `self` pointer and the function ends up
  # running with NULL — the next `*self` load traps cause-28 at
  # offset 0x28 (= self+40, the `dir` field).
  #
  # Uses only a8..a11 (caller-save scratch from the perspective of
  # whoever called scope_push, but distinct from a2..a7 which the
  # emitter's prologue hasn't spilled yet).
  .global cssc_scope_push
  .type   cssc_scope_push, @function
  .align  4
cssc_scope_push:
  # The bounds-check (A.2.1) calls into cssc_panic on overflow — that's
  # a `call0` which would clobber a0 with the panic's return PC and
  # trip the asm_tracer's a0-clobber check. Save a0 in a 16-byte frame
  # up front so the tracer + any actually-reached panic both behave.
  # The normal path restores + pops before ret.n; the panic path falls
  # into an infinite loop so the frame leak is irrelevant.
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  l32r    a8, .Larena_off_addr_sp
  l32i.n  a9, a8, 0                # cursor
  l32r    a10, .Lscope_sp_addr
  l32i.n  a11, a10, 0               # sp
  # A.2.1: bounds-check sp < 32 before we write stack[sp].
  movi.n  a8, 32
  bge     a11, a8, .Lscope_overflow
  l32r    a8, .Lscope_stack_addr    # a8 = stack_base
  slli    a11, a11, 2               # a11 = sp*4 (overwrites sp)
  add     a8, a8, a11               # a8 = &stack[sp]
  s32i.n  a9, a8, 0                 # stack[sp] = cursor
  # Reload sp (we trashed it via the shift) and bump it.
  l32i.n  a11, a10, 0
  addi    a11, a11, 1
  s32i.n  a11, a10, 0               # sp++
  l32i.n  a0, a1, 12
  addi    a1, a1, 16
  ret.n
.Lscope_overflow:
  l32r    a2, .Lscope_overflow_msg
  movi    a3, 26                    # strlen("cssc: scope stack overflow")
  call0   cssc_panic
1:
  j       1b                        # cssc_panic is noreturn
  .size cssc_scope_push, .-cssc_scope_push

  # cssc_scope_pop: restore the most-recently-pushed arena cursor.
  # This INSTANTLY bulk-frees every allocation made since the
  # matching push. Must preserve a2:a3 because the caller may have
  # just computed a return value into those registers.
  .global cssc_scope_pop
  .type   cssc_scope_pop, @function
  .align  4
cssc_scope_pop:
  # Same a0-save story as cssc_scope_push — the underflow path calls
  # cssc_panic via call0 and asm_tracer flags any function with a
  # call0 that lacks an a0-save prologue.
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  l32r    a8,  .Lscope_sp_addr
  l32i.n  a9,  a8, 0              # sp
  # A.2.2: bounds-check sp > 0 before decrement.
  beqz    a9, .Lscope_underflow
  addi    a9,  a9, -1
  s32i.n  a9,  a8, 0               # sp--
  l32r    a10, .Lscope_stack_addr
  slli    a11, a9, 2
  add     a11, a10, a11           # &stack[sp]
  l32i.n  a8,  a11, 0             # saved cursor
  l32r    a9,  .Larena_off_addr_sp
  s32i.n  a8,  a9, 0               # arena_off = saved
  l32i.n  a0,  a1, 12
  addi    a1, a1, 16
  ret.n
.Lscope_underflow:
  l32r    a2, .Lscope_underflow_msg
  movi    a3, 27                    # strlen("cssc: scope stack underflow")
  call0   cssc_panic
1:
  j       1b                        # cssc_panic is noreturn
  .literal_position
  .literal .Larena_off_addr_sp,   cssc_arena_off
  .literal .Lscope_sp_addr,       cssc_scope_sp
  .literal .Lscope_stack_addr,    cssc_scope_stack
  .literal .Lscope_overflow_msg,  .Lscope_overflow_str
  .literal .Lscope_underflow_msg, .Lscope_underflow_str
  .section .rodata
.Lscope_overflow_str:  .asciz "cssc: scope stack overflow"
.Lscope_underflow_str: .asciz "cssc: scope stack underflow"
  .text
  .size cssc_scope_pop, .-cssc_scope_pop

  .global cssc_obj_alloc
  .type   cssc_obj_alloc, @function
.align  4
cssc_obj_alloc:
  # in: a2 = size (i64 lo). We ignore the hi half because no single
  #     allocation on ESP8266 can exceed 65535 bytes anyway.
  # out: a2 = pointer to a fresh `size`-byte buffer; aborts via panic
  #      if the arena is exhausted.
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  # Round size up to 8-byte alignment.
  addi    a2, a2, 7
  movi    a8, -8                  # mask = ~7
  and     a2, a2, a8
  # cur = cssc_arena_off (load via l32r)
  l32r    a8, .Larena_off_addr
  l32i.n  a9, a8, 0               # a9 = current offset
  # new_off = cur + size
  add     a10, a9, a2
  # arena_max = sizeof(cssc_arena) = 65536. MUST use a caller-save
  # reg (a11) — a12-a15 are callee-save under CALL0 and clobbering
  # them here corrupts every caller's persistent state.
  movi    a11, 1
  slli    a11, a11, 16             # 65536 = arena size
  bge     a10, a11, .Larena_oom    # new_off >= cap → out of memory
.Larena_ok:
  # cssc_arena_off = new_off
  s32i.n  a10, a8, 0
  # return ptr = &cssc_arena + cur
  l32r    a8, .Larena_base_addr
  add     a2, a8, a9
  l32i.n  a0, a1, 12
  addi    a1, a1, 16
  ret.n
.Larena_oom:
  # Out of memory — panic with a fixed message.
  l32r    a2, .Loom_msg_addr
  movi.n  a3, 17                  # len of "cssc: arena OOM\n"
  call0   cssc_panic
  # unreachable; cssc_panic halts the CPU.
1:
  j       1b
  .size cssc_obj_alloc, .-cssc_obj_alloc

  .global cssc_obj_free
  .type   cssc_obj_free, @function
.align  4
cssc_obj_free:
  # arena allocator — free is a no-op. ret immediately.
  ret.n
  .size cssc_obj_free, .-cssc_obj_free

  # Literal pool entries the allocator uses (need to be in range of
  # an L32R, so they live at the end of the file too).
  .section .rodata
.Loom_msg:    .asciz "cssc: arena OOM"
  .text
  .literal_position
  .literal .Larena_off_addr,  cssc_arena_off
  .literal .Larena_base_addr, cssc_arena
  .literal .Loom_msg_addr,    .Loom_msg

  # ===========================================================================
  # cssc_panic(ptr msg, i64 len) — emit message to UART, halt CPU.
  # ===========================================================================
  .global cssc_panic
  .type   cssc_panic, @function
.align  4
cssc_panic:
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  s32i.n  a12, a1, 8
  s32i.n  a13, a1, 4
  mov.n   a12, a2                 # msg ptr
  mov.n   a13, a3                 # len
p1:
  movi.n  a8, 0
  beq     a13, a8, p2
  l8ui    a2, a12, 0
  call0   cssc_uart_putc
  addi    a12, a12, 1
  addi    a13, a13, -1
  j       p1
p2:
  movi.n  a2, 10
  call0   cssc_uart_putc
  # Halt forever — disable interrupts then waiti 0 in a loop. WAITI
  # puts the CPU into low-power state until next interrupt; the
  # surrounding loop catches any wakeup.
p3:
  waiti   0
  j       p3
  .size cssc_panic, .-cssc_panic

  # ===========================================================================
  # B7 — Time: cssc_sleep_ms, cssc_uptime, cssc_tick.
  #
  # Uses Xtensa's special register CCOUNT (cycle counter, 32-bit, wraps
  # ~53s at 80 MHz). A module-level `cssc_uptime_ms` global is bumped
  # once per ms inside cssc_sleep_ms. cssc_uptime returns the global
  # as a double (seconds).
  # ===========================================================================
  .data
  .global cssc_uptime_ms
  .align  4
cssc_uptime_ms:
  .word   0
  # Last CCOUNT sampled by `cssc_uptime`. We use it to compute the
  # cycles elapsed since the previous call (unsigned subtract handles
  # CCOUNT's 32-bit wrap, which fires every ~53 s at 80 MHz). Seeded
  # by `cssc_runtime_init` so the first call returns zero, not the
  # absolute power-on CCOUNT value.
  .global cssc_last_ccount
  .align  4
cssc_last_ccount:
  .word   0
  .text

  .global cssc_sleep_ms
  .type   cssc_sleep_ms, @function
.align  4
cssc_sleep_ms:
  # in: a2:a3 = ms (i64). We use only the low 32 bits.
  # Pure CCOUNT busy-wait — does NOT bump cssc_uptime_ms any more.
  # `cssc_uptime` derives uptime from CCOUNT delta on demand, so the
  # clock tracks real wall time (including time spent inside
  # display.show(), etc.) instead of just integrated sleep duration.
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  s32i.n  a12, a1, 8
  s32i.n  a13, a1, 4
  mov.n   a12, a2                # a12 = ms remaining
  l32r    a13, .Lcycles_per_ms_addr
  l32i.n  a13, a13, 0            # a13 = cycles_per_ms (80000)
s1:
  movi.n  a8, 0
  beq     a12, a8, s_done
  # Read CCOUNT, busy-wait until cycles_per_ms have passed.
  rsr.ccount a8                  # a8 = start
s2:
  rsr.ccount a10
  sub     a11, a10, a8           # unsigned wrap-safe delta
  bltu    a11, a13, s2
  addi    a12, a12, -1
  j       s1
s_done:
  l32i.n  a13, a1, 4
  l32i.n  a12, a1, 8
  l32i.n  a0,  a1, 12
  addi    a1, a1, 16
  ret.n
  .literal_position
  .literal .Lcycles_per_ms_addr, .Lcycles_per_ms_val
  .section .rodata
  .align  4
.Lcycles_per_ms_val:
  .word   80000
  .text
  .size cssc_sleep_ms, .-cssc_sleep_ms

  .global cssc_uptime
  .type   cssc_uptime, @function
.align  4
cssc_uptime:
  # Returns f64 seconds in a2:a3.
  #
  # Real-time semantics: derive uptime_ms from the difference between
  # CCOUNT now and CCOUNT at the previous call. That makes time
  # advance during long-running work like `display.show()`'s bit-bang
  # I2C transfer, NOT just during `cssc::sleep`. The earlier impl
  # only bumped uptime_ms inside the sleep loop, so a videodriver
  # tick of `sleep(20) + show(~100ms)` reported only 20ms of elapsed
  # time per ~120ms of real time — uptime advanced ~6× slower than
  # the wall clock.
  #
  # CCOUNT is 32-bit unsigned and wraps every ~53 s at 80 MHz. The
  # unsigned subtract `cur - last` correctly produces the elapsed
  # cycle count across a single wrap, so as long as cssc_uptime is
  # called more than once per 53 s (basically always — every render
  # tick and every comparison against a shutdown threshold drives a
  # call) we never miss a wrap.
  addi    a1, a1, -16
  s32i.n  a0,  a1, 12
  s32i.n  a12, a1, 8
  s32i.n  a13, a1, 4
  rsr.ccount a8                  # cur_ccount
  l32r    a9, .Lup_lastcc_addr
  l32i.n  a10, a9, 0             # last_ccount
  sub     a11, a8, a10           # delta cycles (unsigned wrap-safe)
  l32r    a12, .Lup_cyc_per_ms_addr
  l32i.n  a12, a12, 0            # cycles_per_ms (80000 @ 80 MHz)
  # delta_ms = delta_cycles / cycles_per_ms
  mov.n   a2, a11
  mov.n   a3, a12
  call0   __udivsi3
  mov.n   a13, a2                 # delta_ms (callee-save)
  # Update last_ccount = cur_ccount - (delta_cycles mod cycles_per_ms)
  # i.e. last_ccount += delta_ms * cycles_per_ms. Carrying the
  # remainder forward keeps quantisation error from accumulating.
  mov.n   a2, a13
  mov.n   a3, a12
  call0   __mulsi3                # a2 = delta_ms * cyc_per_ms
  l32r    a9, .Lup_lastcc_addr
  l32i.n  a10, a9, 0
  add     a8, a10, a2
  s32i.n  a8, a9, 0               # last_ccount += consumed cycles
  # uptime_ms += delta_ms
  l32r    a8, .Lup_uptime_ms_addr
  l32i.n  a9, a8, 0
  add     a9, a9, a13
  s32i.n  a9, a8, 0
  # Convert uptime_ms (now accurate) to f64 seconds.
  mov.n   a2, a9
  call0   __floatsidf            # a2:a3 = (double)uptime_ms
  l32r    a8, .Lup_thousand_addr
  l32i.n  a4, a8, 0
  l32i.n  a5, a8, 4
  call0   __divdf3               # a2:a3 = uptime_ms / 1000.0
  l32i.n  a13, a1, 4
  l32i.n  a12, a1, 8
  l32i.n  a0,  a1, 12
  addi    a1, a1, 16
  ret.n
  .literal_position
  .literal .Lup_uptime_ms_addr, cssc_uptime_ms
  .literal .Lup_lastcc_addr,    cssc_last_ccount
  .literal .Lup_cyc_per_ms_addr, .Lup_cyc_per_ms_val
  .literal .Lup_thousand_addr,  .Lup_thousand_val
  .section .rodata
  .align  4
.Lup_cyc_per_ms_val:
  .word   80000
  .align  8
.Lup_thousand_val:
  .word   0x00000000
  .word   0x408F4000             # IEEE-754 double 1000.0
  .text
  .size cssc_uptime, .-cssc_uptime

  .global cssc_tick
  .type   cssc_tick, @function
.align  4
cssc_tick:
  # Returns i64 monotonic milliseconds. We first advance cssc_uptime_ms
  # from the CCOUNT delta (same logic as cssc_uptime so both APIs see
  # the same monotonic clock) then return the updated value.
  addi    a1, a1, -16
  s32i.n  a0,  a1, 12
  s32i.n  a12, a1, 8
  s32i.n  a13, a1, 4
  rsr.ccount a8
  l32r    a9, .Ltk_lastcc_addr
  l32i.n  a10, a9, 0
  sub     a11, a8, a10           # delta cycles (unsigned wrap-safe)
  l32r    a12, .Ltk_cyc_per_ms_addr
  l32i.n  a12, a12, 0
  mov.n   a2, a11
  mov.n   a3, a12
  call0   __udivsi3              # a2 = delta_ms
  mov.n   a13, a2
  mov.n   a2, a13
  mov.n   a3, a12
  call0   __mulsi3                # a2 = delta_ms * cyc_per_ms
  l32r    a9, .Ltk_lastcc_addr
  l32i.n  a10, a9, 0
  add     a8, a10, a2
  s32i.n  a8, a9, 0
  l32r    a8, .Ltk_uptime_ms_addr
  l32i.n  a9, a8, 0
  add     a9, a9, a13
  s32i.n  a9, a8, 0
  mov.n   a2, a9                  # return uptime_ms in a2
  movi.n  a3, 0
  l32i.n  a13, a1, 4
  l32i.n  a12, a1, 8
  l32i.n  a0,  a1, 12
  addi    a1, a1, 16
  ret.n
  .literal_position
  .literal .Ltk_uptime_ms_addr,   cssc_uptime_ms
  .literal .Ltk_lastcc_addr,      cssc_last_ccount
  .literal .Ltk_cyc_per_ms_addr,  .Ltk_cyc_per_ms_val
  .section .rodata
  .align  4
.Ltk_cyc_per_ms_val:
  .word   80000
  .text
  .size cssc_tick, .-cssc_tick

  # ===========================================================================
  # B2b — String runtime.
  #
  # cssc_str layout (matches host runtime — A.3.4 refcount cascade):
  #   offset  0: i32 refcount
  #   offset  4: i32 size
  #   offset  8: i32 cap
  #   offset 12: ptr data
  #   offset 16+: byte payload
  # Header is exactly 16 bytes (8-byte aligned).
  # ===========================================================================

  .global cssc_string_lit
  .type   cssc_string_lit, @function
.align  4
cssc_string_lit:
  # in: a2 = const char* (0-terminated)
  # out: a2 = ptr to fresh cssc_str header (refcount=1)
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  s32i.n  a12, a1, 8
  s32i.n  a13, a1, 4
  s32i.n  a14, a1, 0
  mov.n   a12, a2                # src ptr
  # Compute length via strlen-loop.
  mov.n   a13, a12
  movi.n  a14, 0                 # length counter
sl1:
  l8ui    a8, a13, 0
  movi.n  a9, 0
  beq     a8, a9, sl2
  addi    a13, a13, 1
  addi    a14, a14, 1
  j       sl1
sl2:
  # Allocate header (16 bytes) + (length+1) for data.
  mov.n   a2, a14
  addi    a2, a2, 1              # len + 1 for NUL
  addi    a2, a2, 16             # + header
  # Round to 8-byte alignment
  addi    a2, a2, 7
  movi    a8, -8
  and     a2, a2, a8
  call0   cssc_obj_alloc          # a2 = base ptr (header+data combined)
  # Layout: [refcount @ +0][size @ +4][cap @ +8][data @ +12][bytes @ +16..]
  movi.n  a8, 1
  s32i.n  a8, a2, 0              # refcount = 1
  s32i.n  a14, a2, 4             # size
  s32i.n  a14, a2, 8             # cap
  # data ptr = base + 16
  addi    a8, a2, 16
  s32i.n  a8, a2, 12             # data
  # Copy length bytes from src to data.
  mov.n   a9, a12                # src
  mov.n   a10, a8                # dst
  mov.n   a11, a14               # remaining
sl3:
  movi.n  a13, 0
  beq     a11, a13, sl4
  l8ui    a13, a9, 0
  s8i     a13, a10, 0
  addi    a9, a9, 1
  addi    a10, a10, 1
  addi    a11, a11, -1
  j       sl3
sl4:
  # Write trailing NUL.
  movi.n  a13, 0
  s8i     a13, a10, 0
  l32i.n  a14, a1, 0
  l32i.n  a13, a1, 4
  l32i.n  a12, a1, 8
  l32i.n  a0,  a1, 12
  addi    a1, a1, 16
  ret.n
  .size cssc_string_lit, .-cssc_string_lit

  .global cssc_string_free
  .type   cssc_string_free, @function
.align  4
cssc_string_free:
  # Arena allocator: actual memory is reclaimed only when the enclosing
  # scope is popped. cssc_release calls us when refcount hits 0; we
  # have nothing concrete to do here but the symbol exists so the
  # refcount machinery can dispatch uniformly across targets.
  ret.n
  .size cssc_string_free, .-cssc_string_free

  # cssc_retain(ptr s): increment s->refcount. NULL-safe (refcount field
  # is at offset 0; if s is NULL we'd trap, so callers must pre-check —
  # the lowering only emits retain on known-non-null heap ptrs).
  .global cssc_retain
  .type   cssc_retain, @function
.align  4
cssc_retain:
  beqz    a2, .Lretain_null
  l32i.n  a8, a2, 0              # refcount
  addi    a8, a8, 1
  s32i.n  a8, a2, 0
.Lretain_null:
  ret.n
  .size cssc_retain, .-cssc_retain

  # cssc_release(ptr s): decrement s->refcount; if it hits 0, call
  # cssc_string_free (which is a no-op on the arena allocator but
  # matches the host's malloc-backed runtime). NULL-safe.
  .global cssc_release
  .type   cssc_release, @function
.align  4
cssc_release:
  beqz    a2, .Lrelease_null
  l32i.n  a8, a2, 0              # refcount
  addi    a8, a8, -1
  s32i.n  a8, a2, 0
  bnez    a8, .Lrelease_null
  # refcount reached zero — call cssc_string_free(a2).
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  call0   cssc_string_free
  l32i.n  a0, a1, 12
  addi    a1, a1, 16
.Lrelease_null:
  ret.n
  .size cssc_release, .-cssc_release

  .global cssc_string_size
  .type   cssc_string_size, @function
.align  4
cssc_string_size:
  l32i.n  a2, a2, 4              # size at +4 (was +0 before A.3.4)
  movi.n  a3, 0
  ret.n
  .size cssc_string_size, .-cssc_string_size

  .global cssc_string_data
  .type   cssc_string_data, @function
.align  4
cssc_string_data:
  l32i.n  a2, a2, 12             # data at +12 (was +8 before A.3.4)
  ret.n
  .size cssc_string_data, .-cssc_string_data

  .global cssc_string_concat
  .type   cssc_string_concat, @function
.align  4
cssc_string_concat:
  # in: a2 = cssc_str* a, a3 = cssc_str* b
  # out: a2 = fresh cssc_str* with refcount=1 + concatenated bytes
  addi    a1, a1, -32
  s32i.n  a0, a1, 28
  s32i.n  a12, a1, 24
  s32i.n  a13, a1, 20
  s32i.n  a14, a1, 16
  s32i.n  a15, a1, 12
  mov.n   a12, a2                # a
  mov.n   a13, a3                # b
  l32i.n  a14, a12, 4            # size_a (was +0 before A.3.4)
  l32i.n  a15, a13, 4            # size_b (was +0 before A.3.4)
  # total = size_a + size_b
  add     a8, a14, a15
  s32i.n  a8, a1, 0              # total stays at frame[0]
  # alloc header + total + 1 (NUL).
  addi    a2, a8, 17             # total + 16 header + 1 NUL
  addi    a2, a2, 7
  movi    a9, -8
  and     a2, a2, a9
  call0   cssc_obj_alloc         # a2 = base
  l32i.n  a8, a1, 0              # reload total
  movi.n  a9, 1
  s32i.n  a9, a2, 0              # refcount = 1
  s32i.n  a8, a2, 4              # size = total
  s32i.n  a8, a2, 8              # cap = total
  addi    a9, a2, 16             # data ptr
  s32i.n  a9, a2, 12
  # Copy A's bytes.
  l32i.n  a10, a12, 12           # src_a (was +8 before A.3.4)
  mov.n   a11, a9                # dst
  mov.n   a8, a14                # remaining
cc1:
  movi.n  a3, 0
  beq     a8, a3, cc2
  l8ui    a3, a10, 0
  s8i     a3, a11, 0
  addi    a10, a10, 1
  addi    a11, a11, 1
  addi    a8, a8, -1
  j       cc1
cc2:
  # Copy B's bytes after A's.
  l32i.n  a10, a13, 12           # src_b (was +8 before A.3.4)
  mov.n   a8, a15                # remaining
cc3:
  movi.n  a3, 0
  beq     a8, a3, cc4
  l8ui    a3, a10, 0
  s8i     a3, a11, 0
  addi    a10, a10, 1
  addi    a11, a11, 1
  addi    a8, a8, -1
  j       cc3
cc4:
  # Write trailing NUL.
  movi.n  a3, 0
  s8i     a3, a11, 0
  l32i.n  a15, a1, 12
  l32i.n  a14, a1, 16
  l32i.n  a13, a1, 20
  l32i.n  a12, a1, 24
  l32i.n  a0,  a1, 28
  addi    a1, a1, 32
  ret.n
  .size cssc_string_concat, .-cssc_string_concat

  .global cssc_string_eq
  .type   cssc_string_eq, @function
.align  4
cssc_string_eq:
  # in: a2 = cssc_str* a, a3 = cssc_str* b
  # out: a2 = i1 (1 = equal, 0 = not)
  l32i.n  a8, a2, 4              # size_a (was +0 before A.3.4)
  l32i.n  a9, a3, 4              # size_b
  bne     a8, a9, se_neq
  l32i.n  a10, a2, 12            # data_a (was +8 before A.3.4)
  l32i.n  a11, a3, 12            # data_b
  mov.n   a4, a8                 # remaining
se1:
  movi.n  a5, 0
  beq     a4, a5, se_eq
  l8ui    a5, a10, 0
  l8ui    a6, a11, 0
  bne     a5, a6, se_neq
  addi    a10, a10, 1
  addi    a11, a11, 1
  addi    a4, a4, -1
  j       se1
se_eq:
  movi.n  a2, 1
  ret.n
se_neq:
  movi.n  a2, 0
  ret.n
  .size cssc_string_eq, .-cssc_string_eq

  .global cssc_print_string
  .type   cssc_print_string, @function
.align  4
cssc_print_string:
  # Print a cssc_str via cssc_print_str (which expects ptr + length).
  # in: a2 = cssc_str*
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  mov.n   a8, a2
  l32i.n  a9,  a8, 4             # size (was +0 before A.3.4 refcount)
  l32i.n  a10, a8, 12            # data ptr (was +8 before A.3.4 refcount)
  mov.n   a2, a10
  mov.n   a3, a9
  call0   cssc_print_str
  movi.n  a2, 10
  call0   cssc_uart_putc         # trailing newline
  l32i.n  a0, a1, 12
  addi    a1, a1, 16
  ret.n
  .size cssc_print_string, .-cssc_print_string

  # ===========================================================================
  # B3 — Stringify: int/float/bool → cssc_str*.
  #
  # Each helper formats into a stack buffer then calls cssc_string_lit
  # to allocate the heap-resident cssc_str. Resulting string is owned
  # by the caller (arena allocator — survives until shutdown).
  # ===========================================================================

  .global cssc_int_to_str
  .type   cssc_int_to_str, @function
.align  4
cssc_int_to_str:
  # in: a2:a3 = i64 value (we use only the low 32 bits for now —
  #             the full i64 stringify lands when we wire __divdi3
  #             for the digit-reduce loop)
  #
  # CALL0 ABI: a0-a11 are caller-save (clobbered by every call0).
  # The quotient and the buffer cursor must live across __divsi3
  # and __mulsi3 calls, so they go in a12-a15 (callee-save) and we
  # save/restore them via the prologue.
  addi    a1, a1, -32
  s32i.n  a0,  a1, 28
  s32i.n  a12, a1, 24            # a12 = current value (n)
  s32i.n  a13, a1, 20            # a13 = buffer cursor
  s32i.n  a14, a1, 16            # a14 = saw-minus flag
  s32i.n  a15, a1, 12            # a15 = quotient stash across mulsi3
  # Stack buffer at frame[0..23] (24 bytes — enough for "−9223372036854775808\0").
  addi    a13, a1, 23
  movi.n  a8, 0
  s8i     a8, a13, 0             # terminating NUL
  addi    a13, a13, -1
  mov.n   a12, a2                # value
  movi.n  a8, 0
  bge     a12, a8, is1
  neg     a12, a12
  movi.n  a14, 1                 # saw-minus
  j       is2
is1:
  movi.n  a14, 0
is2:
  movi.n  a8, 0
  bne     a12, a8, is3
  movi.n  a8, 48                 # '0'
  s8i     a8, a13, 0
  addi    a13, a13, -1
  j       is_emit_sign
is3:
  # quotient = n / 10
  mov.n   a2, a12
  movi.n  a3, 10
  call0   __divsi3
  mov.n   a15, a2                # STASH in callee-save before mulsi3
  # quotient * 10
  mov.n   a2, a15
  movi.n  a3, 10
  call0   __mulsi3
  sub     a9, a12, a2            # remainder = n - quotient*10
  addi    a9, a9, 48             # '0' + digit
  s8i     a9, a13, 0
  addi    a13, a13, -1
  mov.n   a12, a15               # n = quotient (still live in a15)
  movi.n  a9, 0
  bne     a12, a9, is3
is_emit_sign:
  movi.n  a8, 0
  beq     a14, a8, is_done
  movi.n  a8, 45                 # '-'
  s8i     a8, a13, 0
  addi    a13, a13, -1
is_done:
  addi    a2, a13, 1
  call0   cssc_string_lit
  l32i.n  a15, a1, 12
  l32i.n  a14, a1, 16
  l32i.n  a13, a1, 20
  l32i.n  a12, a1, 24
  l32i.n  a0,  a1, 28
  addi    a1, a1, 32
  ret.n
  .size cssc_int_to_str, .-cssc_int_to_str

  .global cssc_float_to_str
  .type   cssc_float_to_str, @function
.align  4
cssc_float_to_str:
  # in:  a2:a3 = f64 value x
  # out: a2    = cssc_str* with format "[-]INT.FFF" (fixed 3 fractional digits)
  #
  # Algorithm (hand-tuned, no temporary string allocations):
  #   1. extract sign bit, work with |x| from here on.
  #   2. scaled = (i32) (|x| * 1000.0)     ← __muldf3 + __fixdfsi
  #   3. int_part = scaled / 1000, frac = scaled % 1000.
  #   4. format digits right-to-left into the upper half of our
  #      stack frame (16-byte scratch buffer). Layout:
  #        [a1+48..63]: scratch (high end holds last char).
  #      Write: zero-pad frac to exactly 3 digits, then '.',
  #      then int_part (variable, at least one digit), then
  #      optional '-'.
  #   5. allocate ONE cssc_str (16-byte header + len + NUL),
  #      memcpy the formatted run into the data area.
  #
  # Frame layout (96 bytes, 16-aligned). The 16-byte digit buffer
  # lives at low offsets ([8..23]) and the callee-save spill area
  # sits at high offsets ([72..91]) so the right-to-left digit
  # writes can NEVER touch any saved-register word. The earlier
  # 80-byte layout put the buffer at [48..63] which overlapped the
  # saved-a14 word at [60..63]; every digit written into bytes
  # 60/61/62 partially overwrote saved-a14, and the epilogue's
  # `l32i.n a14, a1, 60` then restored the corrupted word back into
  # the caller's a14. That's exactly what made a float-concat in a
  # while loop hang after iteration 1 — the loop's induction value
  # lived in callee-save a14/a15 and got smashed on every call.
  #
  # Offsets:
  #   [a1, +0..7]   saved input x (f64).
  #   [a1, +8..23]  16-byte digit buffer. NUL terminator at +23,
  #                 last digit slot at +22, writes go right-to-left.
  #   [a1, +24..27] scaled  (i32: |x| * 1000).
  #   [a1, +28..31] int_part.
  #   [a1, +32..35] frac.
  #   [a1, +36..39] sign byte (also in a15).
  #   [a1, +40..43] temp for __divsi3/__mulsi3 quotients.
  #   [a1, +44..47] formatted-run start cursor.
  #   [a1, +48..51] cssc_str* result (post-alloc).
  #   [a1, +52..71] pad / future use (20 bytes free).
  #   [a1, +72..75] saved a12.
  #   [a1, +76..79] saved a13.
  #   [a1, +80..83] saved a14.
  #   [a1, +84..87] saved a15.
  #   [a1, +88..91] saved a0 (return PC).
  #   [a1, +92..95] pad.
  #
  # CALL0 ABI: this function calls __muldf3, __fixdfsi, __divsi3,
  # __mulsi3, cssc_string_lit — all of which respect callee-save.
  addi    a1, a1, -96
  s32i.n  a0,  a1, 88
  s32i.n  a12, a1, 72
  s32i.n  a13, a1, 76
  s32i.n  a14, a1, 80
  s32i.n  a15, a1, 84
  s32i.n  a2,  a1, 0
  s32i.n  a3,  a1, 4

  # ---- (1) Extract sign + absolute value ----
  # ---- (1) Extract sign + absolute value ----
  # Sign bit lives in bit 31 of x.hi (a3). bgez branches when
  # the signed value is >= 0; for IEEE-754 the bit pattern is
  # lexicographically ordered (sign bit at MSB), so bgez detects
  # "non-negative double" iff the sign bit is clear.
  movi.n  a15, 0                # sign = 0
  bgez    a3, .Lftos_abs_done
  movi.n  a15, 1                # sign = 1
  l32r    a8, .Lftos_signmask   # 0x80000000
  xor     a3, a3, a8            # clear sign bit → |x|.hi
  s32i.n  a2, a1, 0
  s32i.n  a3, a1, 4
.Lftos_abs_done:

  # ---- (2) Multiply by 1000.0 and convert to i32 ----
  l32r    a8, .Lftos_thousand_addr
  l32i.n  a4, a8, 0
  l32i.n  a5, a8, 4
  call0   __muldf3              # a2:a3 = |x| * 1000.0
  call0   __fixdfsi             # a2    = (i32) result
  s32i.n  a2, a1, 24            # scaled

  # ---- (3) int_part = scaled / 1000 ----
  movi    a3, 1000
  call0   __divsi3              # a2 = scaled / 1000
  s32i.n  a2, a1, 28
  mov.n   a13, a2               # int_part (callee-save)

  # frac = scaled - int_part * 1000
  mov.n   a2, a13
  movi    a3, 1000
  call0   __mulsi3              # a2 = int_part * 1000
  l32i.n  a8, a1, 24            # scaled
  sub     a14, a8, a2           # frac (callee-save)
  s32i.n  a14, a1, 32

  # ---- (4) Format digits right-to-left into [a1+8..a1+22] ----
  # NUL terminator at +23 (the last byte of the 16-byte buffer).
  # All saved registers live at offsets >= 72, so the buffer can
  # never collide with them no matter how many digits we write.
  movi.n  a8, 0
  s8i     a8, a1, 23
  addi    a12, a1, 22           # cursor = last digit slot

  # --- 4a. Three fractional digits (zero-padded to exactly three) ---
  mov.n   a2, a14
  movi    a3, 10
  call0   __divsi3              # a2 = frac / 10
  s32i.n  a2, a1, 40
  mov.n   a8, a2
  mov.n   a2, a8
  movi    a3, 10
  call0   __mulsi3
  sub     a8, a14, a2
  addi    a8, a8, 0x30
  s8i     a8, a12, 0
  addi    a12, a12, -1
  l32i.n  a14, a1, 40
  mov.n   a2, a14
  movi    a3, 10
  call0   __divsi3
  s32i.n  a2, a1, 40
  mov.n   a8, a2
  mov.n   a2, a8
  movi    a3, 10
  call0   __mulsi3
  sub     a8, a14, a2
  addi    a8, a8, 0x30
  s8i     a8, a12, 0
  addi    a12, a12, -1
  l32i.n  a14, a1, 40
  addi    a8, a14, 0x30
  s8i     a8, a12, 0
  addi    a12, a12, -1

  # --- 4b. Decimal point ---
  movi.n  a8, 0x2E
  s8i     a8, a12, 0
  addi    a12, a12, -1

  # --- 4c. Integer-part digits (at least one) ---
  movi.n  a8, 0
  bne     a13, a8, .Lftos_int_loop
  movi.n  a8, 0x30
  s8i     a8, a12, 0
  addi    a12, a12, -1
  j       .Lftos_int_done
.Lftos_int_loop:
  mov.n   a2, a13
  movi    a3, 10
  call0   __divsi3
  s32i.n  a2, a1, 40
  mov.n   a8, a2
  mov.n   a2, a8
  movi    a3, 10
  call0   __mulsi3
  sub     a8, a13, a2
  addi    a8, a8, 0x30
  s8i     a8, a12, 0
  addi    a12, a12, -1
  l32i.n  a13, a1, 40
  movi.n  a8, 0
  bne     a13, a8, .Lftos_int_loop
.Lftos_int_done:

  # --- 4d. Sign prefix ---
  movi.n  a8, 0
  beq     a15, a8, .Lftos_sign_done
  movi    a8, 0x2D
  s8i     a8, a12, 0
  addi    a12, a12, -1
.Lftos_sign_done:

  # First byte of formatted text is at [a12+1]. cssc_string_lit walks
  # from there to the NUL we wrote at [a1+23].
  addi    a2, a12, 1
  call0   cssc_string_lit       # a2 = cssc_str*
  l32i.n  a15, a1, 84
  l32i.n  a14, a1, 80
  l32i.n  a13, a1, 76
  l32i.n  a12, a1, 72
  l32i.n  a0,  a1, 88
  addi    a1, a1, 96
  ret.n

  .literal_position
  .literal .Lftos_signmask,      0x80000000
  .literal .Lftos_thousand_addr, .Lftos_thousand_val
  .section .rodata
  .align  8
.Lftos_thousand_val:
  .word   0x00000000
  .word   0x408F4000             # 1000.0 (IEEE-754 double)
  .text
  .size cssc_float_to_str, .-cssc_float_to_str

  .global cssc_bool_to_str
  .type   cssc_bool_to_str, @function
.align  4
cssc_bool_to_str:
  # in: a2 = bool (0 or 1) → cssc_str* ("true" / "false")
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  movi.n  a8, 0
  beq     a2, a8, bs_false
  l32r    a2, .Ltrue_addr
  call0   cssc_string_lit
  j       bs_done
bs_false:
  l32r    a2, .Lfalse_addr
  call0   cssc_string_lit
bs_done:
  l32i.n  a0, a1, 12
  addi    a1, a1, 16
  ret.n
  .section .rodata
.Ltrue_str:  .asciz "true"
.Lfalse_str: .asciz "false"
  .text
  .literal_position
  .literal .Ltrue_addr,  .Ltrue_str
  .literal .Lfalse_addr, .Lfalse_str
  .size cssc_bool_to_str, .-cssc_bool_to_str

  # ===========================================================================
  # cssc_print_float — convert via cssc_float_to_str then print.
  # ===========================================================================
  .global cssc_print_float
  .type   cssc_print_float, @function
.align  4
cssc_print_float:
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  call0   cssc_float_to_str       # a2 = cssc_str*
  call0   cssc_print_string
  l32i.n  a0, a1, 12
  addi    a1, a1, 16
  ret.n
  .size cssc_print_float, .-cssc_print_float

  # ===========================================================================
  # D1 — GPIO MMIO driver for ESP8266 LX106.
  #
  # Register map (Espressif TRM v2.5 §3):
  #   GPIO_OUT_REG       = 0x60000300  ; rw — current output bitmask
  #   GPIO_OUT_W1TS_REG  = 0x60000304  ; w  — atomic set bits
  #   GPIO_OUT_W1TC_REG  = 0x60000308  ; w  — atomic clear bits
  #   GPIO_ENABLE_REG    = 0x6000030C  ; rw — output enable mask
  #   GPIO_ENABLE_W1TS   = 0x60000310  ; w  — atomic enable
  #   GPIO_ENABLE_W1TC   = 0x60000314  ; w  — atomic disable
  #   GPIO_IN_REG        = 0x60000318  ; r  — current input state
  #
  # IOMUX function-select registers at PERIPHS_IO_MUX = 0x60000800.
  # Each pin (0-15) has a 4-byte slot; we just need GPIO function = 3
  # (FUNC3) and the optional pullup bit (bit 7).
  #
  # Pin handle (8 bytes in arena):
  #   offset 0: i32 pin_no
  #   offset 4: i32 mode (0=input, 1=output, 2=input_pullup, 3=input_pulldown)
  #
  # GPIO16 needs separate handling via RTC registers; we panic when
  # the user asks for it until that port lands.
  # ===========================================================================

  .global cssc_pin_new
  .type   cssc_pin_new, @function
.align  4
cssc_pin_new:
  # in: a2 = pin number (i64 lo)
  # out: a2 = pin handle pointer
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  s32i.n  a12, a1, 8
  mov.n   a12, a2                # pin_no
  # Reject pin_no >= 16 (GPIO16 not yet supported).
  movi.n  a8, 16
  bge     a12, a8, gpio_err_pin16
  # Reject negative pin numbers.
  movi.n  a8, 0
  blt     a12, a8, gpio_err_pin16
  # Allocate handle (8 bytes).
  movi.n  a2, 8
  call0   cssc_obj_alloc
  s32i.n  a12, a2, 0             # pin_no
  movi.n  a8, 0
  s32i.n  a8, a2, 4              # mode = INPUT (default)
  # IOMUX function-select: set FUNC3 (GPIO) for the pin.
  # The IOMUX register address layout for pins 0-15:
  #   pin 0  → 0x60000834
  #   pin 1  → 0x60000818
  #   pin 2  → 0x60000838
  #   pin 3  → 0x60000814
  #   pin 4  → 0x60000838 (same as pin 2? No — actually pin 4 → 0x6000083C)
  # The mapping is irregular; we use a lookup table indexed by pin_no.
  l32r    a8, .Liomux_table_addr
  slli    a9, a12, 2             # index * 4
  add     a8, a8, a9
  l32i.n  a10, a8, 0             # IOMUX reg addr for this pin
  # Set FUNC3: bits[5:4]=0, bit[12]=1 means FUNC3 selected.
  # Per the TRM the encoding is: clear bits[15:12]=0b0000, then set
  # bit 12 to map to GPIO. Practically `movi 0x10` and write.
  movi.n  a11, 0x10
  s32i.n  a11, a10, 0
  l32i.n  a12, a1, 8
  l32i.n  a0, a1, 12
  addi    a1, a1, 16
  ret.n
gpio_err_pin16:
  # Panic: GPIO16/negative not supported.
  l32r    a2, .Lgpio_msg_addr
  movi.n  a3, 18                 # len of "cssc: bad pin no\n"
  call0   cssc_panic
1:
  j       1b
  .section .rodata
.Lgpio_msg: .asciz "cssc: bad pin no"
  .align 4
.Liomux_table:
  .word 0x60000834   # GPIO 0
  .word 0x60000818   # GPIO 1
  .word 0x60000838   # GPIO 2
  .word 0x60000814   # GPIO 3
  .word 0x6000083C   # GPIO 4
  .word 0x60000840   # GPIO 5
  .word 0x6000081C   # GPIO 6
  .word 0x60000820   # GPIO 7
  .word 0x60000824   # GPIO 8
  .word 0x60000828   # GPIO 9
  .word 0x6000082C   # GPIO 10
  .word 0x60000830   # GPIO 11
  .word 0x60000804   # GPIO 12
  .word 0x60000808   # GPIO 13
  .word 0x6000080C   # GPIO 14
  .word 0x60000810   # GPIO 15
  .text
  .literal_position
  .literal .Liomux_table_addr, .Liomux_table
  .literal .Lgpio_msg_addr,    .Lgpio_msg
  .size cssc_pin_new, .-cssc_pin_new

  .global cssc_pin_free
  .type   cssc_pin_free, @function
.align  4
cssc_pin_free:
  ret.n                          # arena allocator — free is no-op
  .size cssc_pin_free, .-cssc_pin_free

  .global cssc_pin_mode
  .type   cssc_pin_mode, @function
.align  4
cssc_pin_mode:
  # in: a2 = handle, a3 = mode (0..3)
  # Modes: 0=INPUT, 1=OUTPUT, 2=INPUT_PULLUP, 3=INPUT_PULLDOWN
  #
  # Three steps per pin configuration:
  #   (1) Persist mode in handle for read-back.
  #   (2) Set IOMUX function field = 3 (GPIO) for this pin.
  #       The chip boots with most pins in alternate-function modes
  #       (UART/SPI/JTAG), so failing to switch IOMUX leaves
  #       cssc_pin_write writing to a register that no longer
  #       drives the physical pad. The function bits live in
  #       bits 4, 5, 12 of the per-pin IOMUX register; clearing
  #       all three then setting bits 4 + 5 picks function 3.
  #   (3) Drive GPIO_ENABLE (W1TS for output, W1TC for input).
  # CALL0 ABI: we clobber a12/a13/a14 as scratch, so we must
  # save/restore them.
  addi    a1, a1, -16
  s32i.n  a12, a1, 0
  s32i.n  a13, a1, 4
  s32i.n  a14, a1, 8
  s32i.n  a3, a2, 4              # (1) persist mode
  l32i.n  a8, a2, 0              # pin_no
  # ---- (2) IOMUX function 3 ----
  l32r    a11, .Lpinmode_iomux_addr
  slli    a12, a8, 2
  add     a11, a11, a12          # iomux_reg = table[pin]
  l32i.n  a11, a11, 0            # deref to MMIO address
  l32i.n  a12, a11, 0            # current value
  movi    a13, 0x1030            # clear bits 4, 5, 12 (FUNC[2:0])
  movi    a14, -1
  xor     a14, a14, a13          # mask = ~0x1030
  and     a12, a12, a14
  movi    a13, 0x30              # FUNC[1:0] = 3 (GPIO), FUNC[2] = 0
  or      a12, a12, a13
  s32i.n  a12, a11, 0
  memw                           # ordering for the next MMIO write
  # ---- (3) GPIO_ENABLE direction ----
  # bit = 1 << pin_no
  movi.n  a9, 1
  ssl     a8
  sll     a9, a9
  movi.n  a10, 1
  bne     a3, a10, pin_mode_input
  l32r    a11, .Lpin_en_w1ts_addr
  s32i.n  a9, a11, 0
  j       pin_mode_done
pin_mode_input:
  l32r    a11, .Lpin_en_w1tc_addr
  s32i.n  a9, a11, 0
pin_mode_done:
  l32i.n  a14, a1, 8
  l32i.n  a13, a1, 4
  l32i.n  a12, a1, 0
  addi    a1, a1, 16
  ret.n
  .literal_position
  .literal .Lpinmode_iomux_addr, .Liomux_table
  .literal .Lpin_en_w1ts_addr,   0x60000310
  .literal .Lpin_en_w1tc_addr,   0x60000314
  .size cssc_pin_mode, .-cssc_pin_mode

  .global cssc_pin_write
  .type   cssc_pin_write, @function
.align  4
cssc_pin_write:
  # in: a2 = handle, a3 = value (truthy=high)
  l32i.n  a8, a2, 0              # pin_no
  movi.n  a9, 1
  ssl     a8
  sll     a9, a9                 # a9 = bitmask
  # if value != 0: W1TS, else W1TC
  movi.n  a10, 0
  beq     a3, a10, pin_w_clear
  l32r    a11, .Lpin_out_w1ts_addr
  s32i.n  a9, a11, 0
  ret.n
pin_w_clear:
  l32r    a11, .Lpin_out_w1tc_addr
  s32i.n  a9, a11, 0
  ret.n
  .literal_position
  .literal .Lpin_out_w1ts_addr, 0x60000304
  .literal .Lpin_out_w1tc_addr, 0x60000308
  .size cssc_pin_write, .-cssc_pin_write

  .global cssc_pin_read
  .type   cssc_pin_read, @function
.align  4
cssc_pin_read:
  # in: a2 = handle
  # out: a2:a3 = i64 {0 or 1}
  l32i.n  a8, a2, 0              # pin_no
  l32r    a9, .Lpin_in_reg_addr
  l32i.n  a10, a9, 0             # GPIO_IN_REG
  # bit = (GPIO_IN >> pin_no) & 1
  ssr     a8
  srl     a10, a10
  movi.n  a8, 1
  and     a2, a10, a8
  movi.n  a3, 0
  ret.n
  .literal_position
  .literal .Lpin_in_reg_addr, 0x60000318
  .size cssc_pin_read, .-cssc_pin_read

  .global cssc_pin_toggle
  .type   cssc_pin_toggle, @function
.align  4
cssc_pin_toggle:
  # in: a2 = handle
  l32i.n  a8, a2, 0              # pin_no
  movi.n  a9, 1
  ssl     a8
  sll     a9, a9                 # bitmask
  # Read current GPIO_OUT, XOR with bitmask, write back.
  # (Not atomic, but acceptable for the typical "blink an LED" pattern.)
  l32r    a10, .Lpin_out_reg_addr
  l32i.n  a11, a10, 0
  xor     a11, a11, a9
  s32i.n  a11, a10, 0
  ret.n
  .literal_position
  .literal .Lpin_out_reg_addr, 0x60000300
  .size cssc_pin_toggle, .-cssc_pin_toggle

  # ===========================================================================
  # D2 — I2C bit-bang. ESP8266 has no I2C peripheral, so we drive SDA/SCL
  # as GPIO with manual timing. Default 100 kHz (10 µs / bit) — works for
  # SSD1306, SH1106, BMP280, MPU6050, etc.
  #
  # I2C handle (16 bytes in arena):
  #   off 0:  i32 bus_id (informational; ignored)
  #   off 4:  i32 sda_pin
  #   off 8:  i32 scl_pin
  #   off 12: i32 half_bit_cycles (cycles for half a bit period — 400 = 100kHz)
  #
  # Timing primitive `.cssc_i2c_delay` busy-loops `half_bit_cycles`
  # using CCOUNT (1 cycle = 12.5 ns at 80 MHz).
  # ===========================================================================

  .global cssc_i2c_new
  .type   cssc_i2c_new, @function
.align  4
cssc_i2c_new:
  # in: a2 = bus_id (i64 lo), a4 = sda_pin (i64 lo), a6 = scl_pin (i64 lo)
  # Note: With the wide-arg packing, args are (bus_lo, bus_hi, sda_lo, sda_hi,
  # scl_lo, scl_hi). We only care about the lo halves.
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  s32i.n  a12, a1, 8
  s32i.n  a13, a1, 4
  s32i.n  a14, a1, 0
  mov.n   a12, a2                  # bus_id
  mov.n   a13, a4                  # sda_pin
  mov.n   a14, a6                  # scl_pin
  # Allocate 16 bytes handle
  movi.n  a2, 16
  call0   cssc_obj_alloc
  s32i.n  a12, a2, 0
  s32i.n  a13, a2, 4
  s32i.n  a14, a2, 8
  # Default half-bit period: 400 cycles = ~5 µs @ 80 MHz → 100 kHz I2C.
  movi    a8, 400
  s32i.n  a8, a2, 12
  l32i.n  a14, a1, 0
  l32i.n  a13, a1, 4
  l32i.n  a12, a1, 8
  l32i.n  a0,  a1, 12
  addi    a1, a1, 16
  ret.n
  .size cssc_i2c_new, .-cssc_i2c_new

  .global cssc_i2c_free
  .type   cssc_i2c_free, @function
.align  4
cssc_i2c_free:
  ret.n
  .size cssc_i2c_free, .-cssc_i2c_free

  .global cssc_i2c_begin
  .type   cssc_i2c_begin, @function
.align  4
cssc_i2c_begin:
  # in: a2 = handle, a4 = freq_hz (i64 lo). freq_hz==0 means default.
  #
  # Configure both SDA + SCL pins for open-drain GPIO operation:
  #   1. IOMUX function = 3 (GPIO) so the pad is no longer in its
  #      reset-default alternate function (UART/SPI/JTAG).
  #   2. IOMUX pullup bit (7) set so the line floats high when
  #      released — needed because cssc_i2c_drive uses output-
  #      enable as the bit-bang knob, not output-level.
  #   3. (cssc_i2c_drive handles direction at runtime — no
  #      ENABLE setup needed at begin time.)
  # Then compute half-bit cycles from freq for the timing delay.
  addi    a1, a1, -16
  s32i.n  a0,  a1, 12
  s32i.n  a12, a1, 8
  mov.n   a12, a2                  # handle
  # ---- Configure SDA pin IOMUX ----
  l32i.n  a8, a12, 4               # sda pin
  call0   .cssc_pin_iomux_gpio_pu
  # ---- Configure SCL pin IOMUX ----
  l32i.n  a8, a12, 8               # scl pin
  call0   .cssc_pin_iomux_gpio_pu
  # ---- Compute half-bit cycles if freq_hz != 0 ----
  movi.n  a9, 0
  beq     a4, a9, i2c_begin_done
  l32r    a10, .Li2c_40m_addr
  l32i.n  a10, a10, 0              # APB/2 Hz
  mov.n   a2, a10
  mov.n   a3, a4
  call0   __divsi3                 # a2 = half-bit cycles
  s32i.n  a2, a12, 12              # persist
i2c_begin_done:
  l32i.n  a12, a1, 8
  l32i.n  a0,  a1, 12
  addi    a1, a1, 16
  ret.n
  .literal_position
  .literal .Li2c_40m_addr, .Li2c_40m_val
  .section .rodata
  .align 4
.Li2c_40m_val:
  .word 40000000
  .text
  .size cssc_i2c_begin, .-cssc_i2c_begin

  # --- Internal helper: switch a pin's IOMUX to function 3 (GPIO)
  #     and enable its internal pullup. Caller passes pin_no in a8.
  #     Clobbers a8..a11. ---
.global .cssc_pin_iomux_gpio_pu
.type .cssc_pin_iomux_gpio_pu, @function
.align  4
.cssc_pin_iomux_gpio_pu:
  l32r    a9, .Lpinhlp_table_addr
  slli    a10, a8, 2
  add     a9, a9, a10              # addr of table entry
  l32i.n  a9, a9, 0                # iomux MMIO address
  l32i.n  a10, a9, 0               # current value
  # Clear FUNC[2:0] (bits 4, 5, 12)
  movi    a11, -1
  movi    a8, 0x1030
  xor     a11, a11, a8
  and     a10, a10, a11
  # Set FUNC = 3 (bits 4, 5) and PULLUP (bit 7)
  movi    a8, 0xB0                 # 0x30 | 0x80
  or      a10, a10, a8
  s32i.n  a10, a9, 0
  memw
  ret.n
  .literal_position
  .literal .Lpinhlp_table_addr, .Liomux_table
  .size .cssc_pin_iomux_gpio_pu, .-.cssc_pin_iomux_gpio_pu

  # Internal helper: half-bit delay using CCOUNT busy-loop.
  # in: a13 = cycles to wait.
.global .cssc_i2c_delay
.type .cssc_i2c_delay, @function
.align  4
.cssc_i2c_delay:
  rsr.ccount a8
  add     a9, a8, a13
delay_loop:
  rsr.ccount a10
  sub     a11, a10, a8
  bltu    a11, a13, delay_loop
  ret.n
  .size .cssc_i2c_delay, .-.cssc_i2c_delay

  # Internal helper: drive a pin (set as input → released, output → low).
  # Open-drain bit-bang: we toggle GPIO_ENABLE rather than GPIO_OUT.
  #   in: a14 = pin_no, a15 = drive_state (0 = release/HIGH, 1 = pull/LOW)
.global .cssc_i2c_drive
.type .cssc_i2c_drive, @function
.align  4
.cssc_i2c_drive:
  movi.n  a8, 1
  ssl     a14
  sll     a8, a8                   # bitmask
  # Pre-clear the output bit so the pin sinks to 0 when enabled.
  l32r    a9, .Li2c_out_w1tc
  s32i.n  a8, a9, 0
  # Choose ENABLE_W1TS or ENABLE_W1TC based on drive_state.
  movi.n  a9, 0
  beq     a15, a9, i2c_drv_release
  l32r    a9, .Li2c_en_w1ts
  s32i.n  a8, a9, 0
  ret.n
i2c_drv_release:
  l32r    a9, .Li2c_en_w1tc
  s32i.n  a8, a9, 0
  ret.n
  .literal_position
  .literal .Li2c_out_w1tc, 0x60000308
  .literal .Li2c_en_w1ts,  0x60000310
  .literal .Li2c_en_w1tc,  0x60000314
  .size .cssc_i2c_drive, .-.cssc_i2c_drive

  # Internal helper: read SDA pin → a2 (0 or 1).
  #   in: a14 = sda_pin
.global .cssc_i2c_read_sda
.type .cssc_i2c_read_sda, @function
.align  4
.cssc_i2c_read_sda:
  l32r    a8, .Li2c_in_reg
  l32i.n  a9, a8, 0
  ssr     a14
  srl     a9, a9
  movi.n  a10, 1
  and     a2, a9, a10
  ret.n
  .literal_position
  .literal .Li2c_in_reg, 0x60000318
  .size .cssc_i2c_read_sda, .-.cssc_i2c_read_sda

  # cssc_i2c_write(handle, addr, byte) → ack_received (1=ACK, 0=NACK)
  .global cssc_i2c_write
  .type   cssc_i2c_write, @function
.align  4
cssc_i2c_write:
  # ABI (i64 args packed): a2:a3=handle (but it's a ptr — only lo used),
  # a4:a5=addr (lo only), a6:a7=byte (lo only).
  # Frame stores: handle@0, addr@4, byte@8, sda_pin@12, scl_pin@16,
  # half_cycles@20.
  addi    a1, a1, -48
  s32i.n  a0, a1, 44
  s32i.n  a12, a1, 40
  s32i.n  a13, a1, 36
  s32i.n  a14, a1, 32
  s32i.n  a15, a1, 28
  s32i.n  a2, a1, 0                # handle
  s32i.n  a4, a1, 4                # addr
  s32i.n  a6, a1, 8                # byte
  l32i.n  a8, a2, 4
  s32i.n  a8, a1, 12               # sda_pin
  l32i.n  a8, a2, 8
  s32i.n  a8, a1, 16               # scl_pin
  l32i.n  a8, a2, 12
  s32i.n  a8, a1, 20               # half_cycles
  # --- START condition ---
  # SDA high (release), SCL high, delay; SDA low; delay.
  l32i.n  a14, a1, 12              # sda_pin
  movi.n  a15, 0                   # release
  call0   .cssc_i2c_drive
  l32i.n  a14, a1, 16              # scl_pin
  movi.n  a15, 0
  call0   .cssc_i2c_drive
  l32i.n  a13, a1, 20
  call0   .cssc_i2c_delay
  l32i.n  a14, a1, 12              # sda_pin
  movi.n  a15, 1                   # pull low
  call0   .cssc_i2c_drive
  l32i.n  a13, a1, 20
  call0   .cssc_i2c_delay
  # --- Send address byte (addr << 1 + W=0) ---
  l32i.n  a12, a1, 4               # addr
  slli    a12, a12, 1              # shift in W bit (0)
  mov.n   a2, a12
  call0   ._cssc_i2c_send_byte
  # --- Send data byte ---
  l32i.n  a12, a1, 8
  mov.n   a2, a12
  call0   ._cssc_i2c_send_byte
  mov.n   a12, a2                  # preserve ACK status
  # --- STOP condition ---
  # SCL low, SDA low, SCL high (delay), SDA high (delay).
  l32i.n  a14, a1, 16
  movi.n  a15, 1
  call0   .cssc_i2c_drive
  l32i.n  a14, a1, 12
  movi.n  a15, 1
  call0   .cssc_i2c_drive
  l32i.n  a13, a1, 20
  call0   .cssc_i2c_delay
  l32i.n  a14, a1, 16
  movi.n  a15, 0
  call0   .cssc_i2c_drive
  l32i.n  a13, a1, 20
  call0   .cssc_i2c_delay
  l32i.n  a14, a1, 12
  movi.n  a15, 0
  call0   .cssc_i2c_drive
  # Return last ACK status as i64.
  mov.n   a2, a12
  movi.n  a3, 0
  l32i.n  a15, a1, 28
  l32i.n  a14, a1, 32
  l32i.n  a13, a1, 36
  l32i.n  a12, a1, 40
  l32i.n  a0,  a1, 44
  addi    a1, a1, 48
  ret.n
  .size cssc_i2c_write, .-cssc_i2c_write

  # Internal: send one byte MSB-first, read ACK bit, return ACK in a2.
  #   in: a2 = byte to send
  #   out: a2 = ACK status (1=ACK received)
  #   uses sda_pin from frame[+12], scl_pin from frame[+16], half@+20
  #   ASSUMES caller frame is the i2c_write frame layout (relative
  #   offsets stable across all i2c entry points).
.global ._cssc_i2c_send_byte
.type ._cssc_i2c_send_byte, @function
.align  4
._cssc_i2c_send_byte:
  # CALL0 ABI: this helper clobbers a13/a14/a15 (delay-counter, pin-no,
  # drive-state) as scratch across calls to .cssc_i2c_drive. All three
  # are callee-save — they MUST be preserved. The prior 16-byte frame
  # only saved a12, leaking a13..a15 corruption back to every caller
  # (cssc_i2c_write, _pair, _burst). On hardware this manifested as
  # subtle timing/protocol failures in long transactions because the
  # outer functions sometimes refilled these registers from spill
  # before noticing.
  addi    a1, a1, -32
  s32i.n  a0,  a1, 28
  s32i.n  a12, a1, 24
  s32i.n  a13, a1, 20
  s32i.n  a14, a1, 16
  s32i.n  a15, a1, 12
  mov.n   a12, a2                  # byte
  movi.n  a4, 8                    # bits remaining
sb_loop:
  movi.n  a5, 0
  beq     a4, a5, sb_done
  # SCL low. Parent-frame offsets shift by our prologue size (32):
  # sda=12 → 44, scl=16 → 48, half=20 → 52.
  l32i.n  a14, a1, 48              # scl_pin (parent +16)
  movi.n  a15, 1
  call0   .cssc_i2c_drive
  # SDA = MSB of byte. drive_state semantics: 1=pull-low, 0=release-high.
  # bit==1 → release (a15=0); bit==0 → pull-low (a15=1).
  movi.n  a8, 0x80
  and     a9, a12, a8
  movi.n  a15, 0
  beq     a9, a8, sb_set_lo
  movi.n  a15, 1
sb_set_lo:
  l32i.n  a14, a1, 44              # sda_pin (parent +12)
  call0   .cssc_i2c_drive
  l32i.n  a13, a1, 52              # half_cycles (parent +20)
  call0   .cssc_i2c_delay
  # SCL high
  l32i.n  a14, a1, 48
  movi.n  a15, 0
  call0   .cssc_i2c_drive
  l32i.n  a13, a1, 52
  call0   .cssc_i2c_delay
  # next bit
  slli    a12, a12, 1
  addi    a4, a4, -1
  j       sb_loop
sb_done:
  # --- ACK bit: SCL low, release SDA, SCL high, read SDA, SCL low ---
  l32i.n  a14, a1, 48
  movi.n  a15, 1
  call0   .cssc_i2c_drive
  l32i.n  a14, a1, 44
  movi.n  a15, 0                   # release
  call0   .cssc_i2c_drive
  l32i.n  a13, a1, 52
  call0   .cssc_i2c_delay
  l32i.n  a14, a1, 48
  movi.n  a15, 0
  call0   .cssc_i2c_drive
  l32i.n  a13, a1, 52
  call0   .cssc_i2c_delay
  l32i.n  a14, a1, 44
  call0   .cssc_i2c_read_sda
  # a2 = SDA value; ACK = (SDA == 0).
  movi.n  a3, 0
  beq     a2, a3, sb_ack
  movi.n  a2, 0
  j       sb_ret
sb_ack:
  movi.n  a2, 1
sb_ret:
  l32i.n  a15, a1, 12
  l32i.n  a14, a1, 16
  l32i.n  a13, a1, 20
  l32i.n  a12, a1, 24
  l32i.n  a0,  a1, 28
  addi    a1, a1, 32
  ret.n
  .size ._cssc_i2c_send_byte, .-._cssc_i2c_send_byte

  # cssc_i2c_write_pair(handle, addr, b1, b2)
  #   Sends `addr+W, b1, b2` in ONE I2C transaction (START..STOP), no
  #   STOP between b1 and b2. Used by the SSD1306 init sequence
  #   (b1=0x00 control byte, b2=cmd byte) and by per-byte show
  #   (b1=0x40 data control, b2=pixel byte). Without this, every
  #   command/data byte was being sent as its own transaction, which
  #   the SSD1306 interprets as the control byte alone — no init
  #   happens, OLED stays dark.
  .global cssc_i2c_write_pair
  .type   cssc_i2c_write_pair, @function
.align  4
cssc_i2c_write_pair:
  # Frame mirrors cssc_i2c_write so _cssc_i2c_send_byte can index the
  # parent frame at +12/+16/+20 for sda/scl/half_cycles. We also
  # preserve a12..a15 because we use them as scratch + the caller
  # needs them intact under CALL0 ABI.
  # In: a2:a3=handle (ptr lo only), a4:a5=addr (lo only),
  #     a6:a7=b1 (lo only), stack[+0..7]=b2 (i64 lo only).
  # Out: a2 = ACK status of b2.
  addi    a1, a1, -48
  s32i.n  a0, a1, 44
  s32i.n  a12, a1, 40
  s32i.n  a13, a1, 36
  s32i.n  a14, a1, 32
  s32i.n  a15, a1, 28
  s32i.n  a2, a1, 0                # handle
  s32i.n  a4, a1, 4                # addr
  s32i.n  a6, a1, 8                # b1
  # b2 came in via the caller's outgoing stack-arg block at [caller_sp + 0].
  # After our prologue subtracted 48, that's [a1 + 48].
  l32i.n  a8, a1, 48               # b2 (lo of stack-arg pair)
  s32i.n  a8, a1, 24               # persist b2 in a free slot
  l32i.n  a8, a2, 4
  s32i.n  a8, a1, 12               # sda_pin
  l32i.n  a8, a2, 8
  s32i.n  a8, a1, 16               # scl_pin
  l32i.n  a8, a2, 12
  s32i.n  a8, a1, 20               # half_cycles
  # --- START ---
  l32i.n  a14, a1, 12
  movi.n  a15, 0
  call0   .cssc_i2c_drive
  l32i.n  a14, a1, 16
  movi.n  a15, 0
  call0   .cssc_i2c_drive
  l32i.n  a13, a1, 20
  call0   .cssc_i2c_delay
  l32i.n  a14, a1, 12
  movi.n  a15, 1
  call0   .cssc_i2c_drive
  l32i.n  a13, a1, 20
  call0   .cssc_i2c_delay
  # --- Address byte (W=0) ---
  l32i.n  a12, a1, 4
  slli    a12, a12, 1
  mov.n   a2, a12
  call0   ._cssc_i2c_send_byte
  # --- b1 ---
  l32i.n  a2, a1, 8
  call0   ._cssc_i2c_send_byte
  # --- b2 ---
  l32i.n  a2, a1, 24
  call0   ._cssc_i2c_send_byte
  mov.n   a12, a2                  # preserve final ACK
  # --- STOP ---
  l32i.n  a14, a1, 16
  movi.n  a15, 1
  call0   .cssc_i2c_drive
  l32i.n  a14, a1, 12
  movi.n  a15, 1
  call0   .cssc_i2c_drive
  l32i.n  a13, a1, 20
  call0   .cssc_i2c_delay
  l32i.n  a14, a1, 16
  movi.n  a15, 0
  call0   .cssc_i2c_drive
  l32i.n  a13, a1, 20
  call0   .cssc_i2c_delay
  l32i.n  a14, a1, 12
  movi.n  a15, 0
  call0   .cssc_i2c_drive
  mov.n   a2, a12
  movi.n  a3, 0
  l32i.n  a15, a1, 28
  l32i.n  a14, a1, 32
  l32i.n  a13, a1, 36
  l32i.n  a12, a1, 40
  l32i.n  a0, a1, 44
  addi    a1, a1, 48
  ret.n
  .size cssc_i2c_write_pair, .-cssc_i2c_write_pair

  # cssc_i2c_write_burst(handle, addr, ctl, ptr, len)
  #   Sends `addr+W, ctl, *(ptr), *(ptr+1), ..., *(ptr+len-1)` in
  #   ONE I2C transaction. Used by cssc_tft_show to push the full
  #   framebuffer in a single START..STOP envelope instead of
  #   ~1024 micro-transactions.
  # In: a2=handle, a4=addr, a6=ctl, stack[0..7]=ptr (lo), stack[8..15]=len.
  # Out: a2 = ACK status of last byte.
  .global cssc_i2c_write_burst
  .type   cssc_i2c_write_burst, @function
.align  4
cssc_i2c_write_burst:
  # Frame: 64 bytes so the per-call ptr-cursor and remaining-len slots
  # don't collide with the callee-save save slots.
  #  [+0]  handle
  #  [+4]  addr
  #  [+8]  ctl
  #  [+12] sda_pin
  #  [+16] scl_pin
  #  [+20] half_cycles
  #  [+24] ptr cursor (advances each byte)
  #  [+28] len remaining (decrements each byte)
  #  [+44] saved a15
  #  [+48] saved a14
  #  [+52] saved a13
  #  [+56] saved a12
  #  [+60] saved a0
  # Caller stack-args land at [+64] (ptr lo), [+72] (len lo).
  addi    a1, a1, -64
  s32i.n  a0,  a1, 60
  s32i.n  a12, a1, 56
  s32i.n  a13, a1, 52
  s32i.n  a14, a1, 48
  s32i.n  a15, a1, 44
  s32i.n  a2, a1, 0                # handle
  s32i.n  a4, a1, 4                # addr
  s32i.n  a6, a1, 8                # ctl
  l32i.n  a8, a1, 64               # ptr lo (stack arg)
  s32i.n  a8, a1, 24               # persist ptr cursor
  l32i.n  a8, a1, 72               # len lo (stack arg)
  s32i.n  a8, a1, 28               # persist len
  l32i.n  a8, a2, 4
  s32i.n  a8, a1, 12               # sda_pin
  l32i.n  a8, a2, 8
  s32i.n  a8, a1, 16               # scl_pin
  l32i.n  a8, a2, 12
  s32i.n  a8, a1, 20               # half_cycles
  # --- START ---
  l32i.n  a14, a1, 12
  movi.n  a15, 0
  call0   .cssc_i2c_drive
  l32i.n  a14, a1, 16
  movi.n  a15, 0
  call0   .cssc_i2c_drive
  l32i.n  a13, a1, 20
  call0   .cssc_i2c_delay
  l32i.n  a14, a1, 12
  movi.n  a15, 1
  call0   .cssc_i2c_drive
  l32i.n  a13, a1, 20
  call0   .cssc_i2c_delay
  # --- Address byte ---
  l32i.n  a12, a1, 4
  slli    a12, a12, 1
  mov.n   a2, a12
  call0   ._cssc_i2c_send_byte
  # --- Control byte ---
  l32i.n  a2, a1, 8
  call0   ._cssc_i2c_send_byte
  # --- Data bytes loop ---
wb_loop:
  l32i.n  a8, a1, 28               # len
  movi.n  a9, 0
  beq     a8, a9, wb_done
  l32i.n  a8, a1, 24               # ptr cursor
  l8ui    a2, a8, 0
  call0   ._cssc_i2c_send_byte
  l32i.n  a8, a1, 24
  addi    a8, a8, 1
  s32i.n  a8, a1, 24               # advance cursor
  l32i.n  a8, a1, 28
  addi    a8, a8, -1
  s32i.n  a8, a1, 28               # decrement remaining
  j       wb_loop
wb_done:
  mov.n   a12, a2                  # preserve final ACK
  # --- STOP ---
  l32i.n  a14, a1, 16
  movi.n  a15, 1
  call0   .cssc_i2c_drive
  l32i.n  a14, a1, 12
  movi.n  a15, 1
  call0   .cssc_i2c_drive
  l32i.n  a13, a1, 20
  call0   .cssc_i2c_delay
  l32i.n  a14, a1, 16
  movi.n  a15, 0
  call0   .cssc_i2c_drive
  l32i.n  a13, a1, 20
  call0   .cssc_i2c_delay
  l32i.n  a14, a1, 12
  movi.n  a15, 0
  call0   .cssc_i2c_drive
  mov.n   a2, a12
  movi.n  a3, 0
  l32i.n  a15, a1, 44
  l32i.n  a14, a1, 48
  l32i.n  a13, a1, 52
  l32i.n  a12, a1, 56
  l32i.n  a0,  a1, 60
  addi    a1, a1, 64
  ret.n
  .size cssc_i2c_write_burst, .-cssc_i2c_write_burst

  .global cssc_i2c_read
  .type   cssc_i2c_read, @function
.align  4
cssc_i2c_read:
  # in: a2 = handle, a4 = addr → out: a2 = byte read
  # (Phase D2 minimum: START, send addr|R, read 8 bits + NACK, STOP.)
  # For brevity we delegate the start/stop machinery to cssc_i2c_write's
  # helpers but reorder the bit loop to RECEIVE. A self-contained
  # implementation lives below.
  addi    a1, a1, -48
  s32i.n  a0, a1, 44
  s32i.n  a12, a1, 40
  s32i.n  a13, a1, 36
  s32i.n  a14, a1, 32
  s32i.n  a15, a1, 28
  s32i.n  a2, a1, 0                # handle
  s32i.n  a4, a1, 4                # addr
  l32i.n  a8, a2, 4
  s32i.n  a8, a1, 12
  l32i.n  a8, a2, 8
  s32i.n  a8, a1, 16
  l32i.n  a8, a2, 12
  s32i.n  a8, a1, 20
  # START
  l32i.n  a14, a1, 12
  movi.n  a15, 0
  call0   .cssc_i2c_drive
  l32i.n  a14, a1, 16
  movi.n  a15, 0
  call0   .cssc_i2c_drive
  l32i.n  a13, a1, 20
  call0   .cssc_i2c_delay
  l32i.n  a14, a1, 12
  movi.n  a15, 1
  call0   .cssc_i2c_drive
  l32i.n  a13, a1, 20
  call0   .cssc_i2c_delay
  # Send (addr << 1) | 1.
  l32i.n  a12, a1, 4
  slli    a12, a12, 1
  movi.n  a8, 1
  or      a12, a12, a8
  mov.n   a2, a12
  call0   ._cssc_i2c_send_byte
  # Read 8 bits MSB-first.
  movi.n  a12, 0                   # accumulator
  movi.n  a4, 8
ir_loop:
  movi.n  a5, 0
  beq     a4, a5, ir_done
  l32i.n  a14, a1, 16              # SCL
  movi.n  a15, 1                   # low
  call0   .cssc_i2c_drive
  l32i.n  a14, a1, 12              # SDA
  movi.n  a15, 0                   # release (will read)
  call0   .cssc_i2c_drive
  l32i.n  a13, a1, 20
  call0   .cssc_i2c_delay
  l32i.n  a14, a1, 16              # SCL
  movi.n  a15, 0                   # high
  call0   .cssc_i2c_drive
  l32i.n  a13, a1, 20
  call0   .cssc_i2c_delay
  l32i.n  a14, a1, 12              # read SDA
  call0   .cssc_i2c_read_sda
  # shift accumulator left, OR in the bit.
  slli    a12, a12, 1
  or      a12, a12, a2
  addi    a4, a4, -1
  j       ir_loop
ir_done:
  # Send NACK after the byte (master tells slave "no more bytes").
  l32i.n  a14, a1, 16
  movi.n  a15, 1
  call0   .cssc_i2c_drive
  l32i.n  a14, a1, 12
  movi.n  a15, 0                   # release SDA (HIGH = NACK)
  call0   .cssc_i2c_drive
  l32i.n  a13, a1, 20
  call0   .cssc_i2c_delay
  l32i.n  a14, a1, 16
  movi.n  a15, 0                   # SCL high
  call0   .cssc_i2c_drive
  l32i.n  a13, a1, 20
  call0   .cssc_i2c_delay
  # STOP
  l32i.n  a14, a1, 16
  movi.n  a15, 1
  call0   .cssc_i2c_drive
  l32i.n  a14, a1, 12
  movi.n  a15, 1
  call0   .cssc_i2c_drive
  l32i.n  a13, a1, 20
  call0   .cssc_i2c_delay
  l32i.n  a14, a1, 16
  movi.n  a15, 0
  call0   .cssc_i2c_drive
  l32i.n  a13, a1, 20
  call0   .cssc_i2c_delay
  l32i.n  a14, a1, 12
  movi.n  a15, 0
  call0   .cssc_i2c_drive
  # Return accumulator as i64.
  mov.n   a2, a12
  movi.n  a3, 0
  l32i.n  a15, a1, 28
  l32i.n  a14, a1, 32
  l32i.n  a13, a1, 36
  l32i.n  a12, a1, 40
  l32i.n  a0,  a1, 44
  addi    a1, a1, 48
  ret.n
  .size cssc_i2c_read, .-cssc_i2c_read

  # ===========================================================================
  # D3 — SPI (HSPI hardware block at 0x60000200). Single-byte transfer.
  #
  # Register layout (TRM §7):
  #   SPI_CMD_REG    = 0x60000200  bit 18 = SPI_USR (start transfer)
  #   SPI_CLOCK_REG  = 0x60000218
  #   SPI_USER_REG   = 0x60000234
  #   SPI_USER1_REG  = 0x60000238
  #   SPI_W0_REG     = 0x60000240
  #
  # Handle (16 bytes):
  #   off 0: i32 bus_id (1=HSPI)
  #   off 4: i32 sck_pin, off 8: i32 miso_pin, off 12: i32 mosi_pin
  # ===========================================================================

  .global cssc_spi_new
  .type   cssc_spi_new, @function
.align  4
cssc_spi_new:
  # in: a2 = bus_id (i64 lo), a4 = sck (i64 lo), a6 = miso (i64 lo),
  #     [a1, frame_size+0] = mosi (i64 lo, stack-passed because the
  #     4th i64 arg overflows the 6-register CALL0 ABI).
  # out: a2 = handle ptr
  addi    a1, a1, -32
  s32i.n  a0,  a1, 28
  s32i.n  a12, a1, 24
  s32i.n  a13, a1, 20
  s32i.n  a14, a1, 16
  s32i.n  a15, a1, 12
  mov.n   a12, a2                  # bus
  mov.n   a13, a4                  # sck
  mov.n   a14, a6                  # miso
  # mosi is the stack-passed 4th i64 arg; its lo half lives at
  # [caller-SP, 0..3] which after our 32-byte prologue is at
  # [a1, 32..35]. The hi half at [a1, 36..39] is discarded —
  # pin numbers fit in i32 trivially.
  l32i.n  a15, a1, 32              # mosi
  movi.n  a2, 16
  call0   cssc_obj_alloc
  s32i.n  a12, a2, 0
  s32i.n  a13, a2, 4
  s32i.n  a14, a2, 8
  s32i.n  a15, a2, 12
  l32i.n  a15, a1, 12
  l32i.n  a14, a1, 16
  l32i.n  a13, a1, 20
  l32i.n  a12, a1, 24
  l32i.n  a0,  a1, 28
  addi    a1, a1, 32
  ret.n
  .size cssc_spi_new, .-cssc_spi_new

  .global cssc_spi_free
  .type   cssc_spi_free, @function
.align  4
cssc_spi_free:
  ret.n
  .size cssc_spi_free, .-cssc_spi_free

  .global cssc_spi_begin
  .type   cssc_spi_begin, @function
.align  4
cssc_spi_begin:
  # Configure HSPI for 8-bit full-duplex transfer at ~1 MHz (Phase D3b).
  # SPI_USER_REG bits we care about:
  #   bit 0  = SPI_DOUTDIN   (full-duplex MOSI+MISO on same clocks)
  #   bit 27 = SPI_USR_MOSI  (we send data)
  #   bit 28 = SPI_USR_MISO  (we read the slave's response)
  # SPI_USER1_REG:
  #   bits 23:17 = SPI_USR_MOSI_BITLEN-1 (8 bits → 7)
  #   bits 16:8  = SPI_USR_MISO_BITLEN-1 (8 bits → 7)
  # SPI_CLOCK_REG: clkdiv_pre + clkcnt_n,h,l. The 0x00027001 pattern
  # divides the 80 MHz APB clock down to ~1 MHz.
  l32r    a8, .Lspi_user_addr
  l32r    a9, .Lspi_user_val
  l32i.n  a9, a9, 0
  s32i.n  a9, a8, 0
  l32r    a8, .Lspi_user1_addr
  l32r    a9, .Lspi_user1_val
  l32i.n  a9, a9, 0
  s32i.n  a9, a8, 0
  l32r    a8, .Lspi_clock_addr
  l32r    a9, .Lspi_clock_val
  l32i.n  a9, a9, 0
  s32i.n  a9, a8, 0
  ret.n
  .literal_position
  .literal .Lspi_user_addr,   0x60000234
  .literal .Lspi_user_val,    .Lspi_user_val_data
  .literal .Lspi_user1_addr,  0x60000238
  .literal .Lspi_user1_val,   .Lspi_user1_val_data
  .literal .Lspi_clock_addr,  0x60000218
  .literal .Lspi_clock_val,   .Lspi_clock_val_data
  .section .rodata
  .align 4
.Lspi_user_val_data:
  # SPI_DOUTDIN (bit 0) | SPI_USR_MOSI (bit 27) | SPI_USR_MISO (bit 28)
  # = 0x18000001. Full-duplex: every clock cycle both MOSI and MISO
  # bits move in step, exactly the shape `cssc_spi_transfer` expects.
  .word 0x18000001
.Lspi_user1_val_data:
  # MOSI_BITLEN = 7 (bits 23:17) | MISO_BITLEN = 7 (bits 16:8)
  #   = 0x00E0_0E00. Eight bits each direction, matching CSSC's
  # per-byte transfer abstraction.
  .word 0x00E00E00
.Lspi_clock_val_data:
  .word 0x00027001                 # divider for ~1 MHz
  .text
  .size cssc_spi_begin, .-cssc_spi_begin

  .global cssc_spi_transfer
  .type   cssc_spi_transfer, @function
.align  4
cssc_spi_transfer:
  # Phase D3b — full-duplex single-byte transfer.
  #   in:  a2 = handle (ignored; HSPI is a singleton), a4 = byte (i64 lo)
  #   out: a2:a3 = i64 sample received on MISO during the same 8 clocks
  #
  # The peripheral shifts `a4`'s low 8 bits out on MOSI while
  # latching MISO into the low 8 bits of SPI_W0. SPI_DOUTDIN was set
  # in cssc_spi_begin so the two FIFOs share the same clock edges.
  # After SPI_USR clears we read W0 back, mask to 8 bits, and return.
  l32r    a8, .Lspi_w0_addr
  s32i.n  a4, a8, 0
  l32r    a8, .Lspi_cmd_addr
  movi    a9, 0x40000              # bit 18 = SPI_USR (start transaction)
  s32i.n  a9, a8, 0
spi_xf_wait:
  l32i.n  a10, a8, 0
  bnez    a10, spi_xf_wait
  # Read MISO byte from W0[7:0].
  l32r    a8, .Lspi_w0_addr
  l32i.n  a9, a8, 0
  movi    a10, 0xFF
  and     a2, a9, a10
  movi.n  a3, 0
  ret.n
  .literal_position
  .literal .Lspi_w0_addr,  0x60000240
  .literal .Lspi_cmd_addr, 0x60000200
  .size cssc_spi_transfer, .-cssc_spi_transfer

  # --- Phase D3b multi-byte burst transfer ---------------------------------
  # cssc_spi_transfer_burst(handle, ptr buf_in_out, i64 length)
  #
  # Sends `length` bytes from `buf_in_out` over MOSI, simultaneously
  # replaces them in-place with the bytes received on MISO. The
  # chunked engine fits up to 64 bytes per transaction because the
  # HSPI buffer is W0..W15 = 16 registers × 32 bits = 64 bytes.
  #
  #   in:  a2 = handle (ignored), a4 = buf_in_out ptr, a6 = length (i64 lo)
  #   out: a2 = bytes actually transferred (== length on success)
  #
  # The implementation iterates 64-byte chunks. For each chunk:
  #   1. Configure USER1 with the chunk's bit-length (length*8 - 1).
  #   2. Copy 0..15 source words from `buf` into W0..W15.
  #   3. Trigger SPI_USR; poll until clear.
  #   4. Read W0..W15 back into `buf` (in-place full-duplex semantics).
  #
  # This is the hot path for SSD1306 framebuffer pushes, BMP280
  # pressure reads, and SD-card sector transfers.
  .global cssc_spi_transfer_burst
  .type   cssc_spi_transfer_burst, @function
.align  4
cssc_spi_transfer_burst:
  addi    a1, a1, -32
  s32i.n  a0, a1, 28
  s32i.n  a12, a1, 24                # buf cursor
  s32i.n  a13, a1, 20                # bytes remaining
  s32i.n  a14, a1, 16                # HSPI base reg cache
  s32i.n  a15, a1, 12                # chunk size in bytes
  mov.n   a12, a4                    # a12 = buf cursor
  mov.n   a13, a6                    # a13 = bytes remaining (i64 lo only)
  l32r    a14, .Lspi_w0_burst
.Lspi_burst_chunk:
  movi.n  a15, 0
  beq     a13, a15, .Lspi_burst_done
  # Determine chunk size = min(remaining, 64).
  movi    a15, 64
  bltu    a13, a15, 1f
  movi    a15, 64
  j       2f
1:
  mov.n   a15, a13
2:
  # Configure SPI_USER1_REG with (chunk*8 - 1) for both BITLEN fields.
  #   value = ((chunk*8 - 1) << 17) | ((chunk*8 - 1) << 8)
  slli    a8, a15, 3                  # a8 = chunk * 8
  addi    a8, a8, -1                  # a8 = chunk*8 - 1
  slli    a9, a8, 17
  slli    a10, a8, 8
  or      a9, a9, a10
  l32r    a10, .Lspi_user1_burst_addr
  s32i.n  a9, a10, 0
  # Copy `chunk` bytes from a12 (buf) into W0..W15. The CIR side
  # guarantees buf alignment by allocating from the bump allocator,
  # which hands out 4-byte-aligned blocks; we use l32i.n freely.
  # Bytes that don't fill a full word are still copied with l8ui.
  mov.n   a8, a12                    # a8 = src cursor in buf
  mov.n   a9, a14                    # a9 = dst cursor in SPI_W
  mov.n   a11, a15                   # a11 = bytes left in this chunk
.Lspi_burst_loadw:
  # Word-copy as long as 4+ bytes remain in this chunk.
  movi    a10, 4
  bltu    a11, a10, .Lspi_burst_tail_in
  l32i.n  a10, a8, 0
  s32i.n  a10, a9, 0
  addi    a8, a8, 4
  addi    a9, a9, 4
  addi    a11, a11, -4
  j       .Lspi_burst_loadw
.Lspi_burst_tail_in:
  # 0..3-byte tail. Pack remaining bytes into one word, little-end
  # ordered, then store. BITLEN was set to (chunk*8 - 1) so the
  # peripheral only shifts the exact number of bits requested — any
  # high-byte slack in the packed word is harmless.
  #
  # A.1.2: this block previously used a13 as a scratch register for
  # byte loads + comparison literals — but a13 holds bytes-remaining
  # across the outer chunk loop. Any burst with `length % 4 != 0`
  # clobbered the counter, then a stray `l32r a13, .Lspi_w0_burst`
  # "restored" it to the MMIO base address instead of the counter.
  # Outer loop then read a wild value and either hung or scribbled
  # garbage into MMIO. The 6-byte cmd-stream burst in cssc_tft_show
  # hit this on every frame. Fix: scratch in a3 (caller-save, free
  # throughout this routine), drop the bogus l32r entirely.
  beqz    a11, .Lspi_burst_kick
  movi.n  a10, 0                     # packed word accumulator
  l8ui    a3, a8, 0
  or      a10, a10, a3
  movi    a3, 2
  blt     a11, a3, .Lspi_burst_tail_store
  l8ui    a3, a8, 1
  slli    a3, a3, 8
  or      a10, a10, a3
  movi    a3, 3
  blt     a11, a3, .Lspi_burst_tail_store
  l8ui    a3, a8, 2
  slli    a3, a3, 16
  or      a10, a10, a3
.Lspi_burst_tail_store:
  s32i.n  a10, a9, 0
.Lspi_burst_kick:
  # Start the transaction and poll for completion.
  l32r    a8, .Lspi_cmd_burst_addr
  movi    a9, 0x40000                # SPI_USR
  s32i.n  a9, a8, 0
.Lspi_burst_wait:
  l32i.n  a10, a8, 0
  bnez    a10, .Lspi_burst_wait
  # Read W0..W15 back into buf. We re-derive the byte cursor as
  #   buf_cursor_at_chunk_start = buf_cursor - chunk
  # since a12 still points at the chunk start.
  mov.n   a8, a12
  mov.n   a9, a14
  mov.n   a11, a15
.Lspi_burst_storew:
  movi    a10, 4
  bltu    a11, a10, .Lspi_burst_tail_out
  l32i.n  a10, a9, 0
  s32i.n  a10, a8, 0
  addi    a8, a8, 4
  addi    a9, a9, 4
  addi    a11, a11, -4
  bnez    a11, .Lspi_burst_storew
  j       .Lspi_burst_next
.Lspi_burst_tail_out:
  beqz    a11, .Lspi_burst_next
  l32i.n  a10, a9, 0
.Lspi_burst_tail_byte:
  s8i     a10, a8, 0
  srli    a10, a10, 8
  addi    a8, a8, 1
  addi    a11, a11, -1
  bnez    a11, .Lspi_burst_tail_byte
.Lspi_burst_next:
  add     a12, a12, a15              # buf cursor += chunk
  sub     a13, a13, a15              # remaining -= chunk
  j       .Lspi_burst_chunk
.Lspi_burst_done:
  mov.n   a2, a6                     # return original length
  movi.n  a3, 0
  l32i.n  a15, a1, 12
  l32i.n  a14, a1, 16
  l32i.n  a13, a1, 20
  l32i.n  a12, a1, 24
  l32i.n  a0,  a1, 28
  addi    a1, a1, 32
  ret.n
  .literal_position
  .literal .Lspi_w0_burst,         0x60000240
  .literal .Lspi_user1_burst_addr, 0x60000238
  .literal .Lspi_cmd_burst_addr,   0x60000200
  .size cssc_spi_transfer_burst, .-cssc_spi_transfer_burst

  # ===========================================================================
  # D4 — UART1 (0x60000F00). TX-only on GPIO2 — useful as a debug channel
  # or as a side-channel to another MCU. Receive lives only on UART0.
  # ===========================================================================

  .global cssc_uart_new
  .type   cssc_uart_new, @function
.align  4
cssc_uart_new:
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  s32i.n  a12, a1, 8
  mov.n   a12, a2                  # bus_id (0 or 1)
  movi.n  a2, 8
  call0   cssc_obj_alloc
  s32i.n  a12, a2, 0
  movi.n  a8, 0
  s32i.n  a8, a2, 4                # default baud — set by begin
  l32i.n  a12, a1, 8
  l32i.n  a0,  a1, 12
  addi    a1, a1, 16
  ret.n
  .size cssc_uart_new, .-cssc_uart_new

  .global cssc_uart_free
  .type   cssc_uart_free, @function
.align  4
cssc_uart_free:
  ret.n
  .size cssc_uart_free, .-cssc_uart_free

  .global cssc_uart_begin
  .type   cssc_uart_begin, @function
.align  4
cssc_uart_begin:
  # in: a2 = handle, a4 = baud (i64 lo)
  # ESP8266 UART clock = 80 MHz; UART_CLKDIV = 80_000_000 / baud.
  # UART1_CLKDIV_REG = 0x60000F14.
  s32i.n  a4, a2, 4                # persist baud
  # Compute divider via __divsi3.
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  l32r    a8, .Luart_clk_addr
  l32i.n  a8, a8, 0                # a8 = 80_000_000
  mov.n   a2, a8
  mov.n   a3, a4
  call0   __divsi3                 # a2 = 80M / baud
  l32r    a8, .Luart1_clkdiv_addr
  s32i.n  a2, a8, 0
  l32i.n  a0, a1, 12
  addi    a1, a1, 16
  ret.n
  .literal_position
  .literal .Luart_clk_addr,     .Luart_clk_val
  .literal .Luart1_clkdiv_addr, 0x60000F14
  .section .rodata
  .align 4
.Luart_clk_val:
  .word 80000000
  .text
  .size cssc_uart_begin, .-cssc_uart_begin

  .global cssc_uart_write
  .type   cssc_uart_write, @function
.align  4
cssc_uart_write:
  # in: a2 = handle, a4 = byte (i64 lo). UART1 TX FIFO at 0x60000F00.
  l32r    a8, .Luart1_tx_addr
  s8i     a4, a8, 0
  memw
  ret.n
  .literal_position
  .literal .Luart1_tx_addr, 0x60000F00
  .size cssc_uart_write, .-cssc_uart_write

  .global cssc_uart_read
  .type   cssc_uart_read, @function
.align  4
cssc_uart_read:
  # UART1 RX not connected on ESP8266 (the chip has no RX line wired
  # through to GPIO2). Return 0 as a deterministic placeholder.
  movi.n  a2, 0
  movi.n  a3, 0
  ret.n
  .size cssc_uart_read, .-cssc_uart_read

  # ===========================================================================
  # D5 — ADC, PWM, Timer.
  #
  # ESP8266 ADC: 10-bit, single channel (TOUT pin). Reading requires the
  # ROM-resident `system_adc_read` thunk at 0x40006A14 — we call into
  # it (CALL0) and grab the i32 return.
  #
  # PWM: chip has no native PWM block. The SDK provides software PWM
  # via FRC1 timer interrupts. For now cssc_pwm_duty is a no-op stub;
  # a real port lands once interrupt-vector emission is wired.
  #
  # Timer: FRC1 hardware timer at 0x60000600. FRC1_LOAD_REG (0x600),
  # FRC1_COUNT_REG (0x604), FRC1_CTRL_REG (0x608), FRC1_INT_REG (0x60C).
  # ===========================================================================

  .global cssc_adc_new
  .type   cssc_adc_new, @function
.align  4
cssc_adc_new:
  # in:  a2:a3 = channel (i64 lo:hi); ESP8266 ignores it (TOUT-only),
  #        ESP32 uses bits 0..2 to pick SAR_ADC1 channel 0..7.
  # out: a2:a3 = handle (8 bytes: i32 channel, i32 reserved)
  addi    a1, a1, -16
  s32i.n  a0,  a1, 12
  s32i.n  a12, a1, 8                # preserve channel across alloc
  mov.n   a12, a2                   # a12 = channel
  movi.n  a2, 8
  call0   cssc_obj_alloc
  s32i.n  a12, a2, 0                # handle[0] = channel
  movi.n  a8, 0
  s32i.n  a8, a2, 4                 # handle[4] = reserved (zeroed)
  movi.n  a3, 0
  l32i.n  a12, a1, 8
  l32i.n  a0,  a1, 12
  addi    a1, a1, 16
  ret.n
  .size cssc_adc_new, .-cssc_adc_new

  .global cssc_adc_free
  .type   cssc_adc_free, @function
.align  4
cssc_adc_free:
  ret.n
  .size cssc_adc_free, .-cssc_adc_free

  .global cssc_adc_read
  .type   cssc_adc_read, @function
.align  4
cssc_adc_read:
  # in:  a2 = handle
  # out: a2:a3 = i64 sample (low = raw 12-bit reading on ESP32 or
  #             10-bit on ESP8266; high = 0)
  #
  # Forwards the channel field in handle[0] as the first arg into the
  # profile-specific read target. On ESP8266 the substitution makes
  # that target the bootrom thunk at 0x40006A14 (which ignores its
  # arg — TOUT-only). On ESP32 it's `cssc_adc_esp32_read` further
  # below — a hand-coded SAR_ADC1 sequencer that uses the channel.
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  l32i.n  a2, a2, 0                # a2 = channel (replaces handle ptr)
  l32r    a8, .Ladc_rom_addr
  callx0  a8                       # reads sample → a2 (callee zeroes a3
                                   # for cssc_adc_esp32_read; ESP8266
                                   # bootrom leaves a3 garbage so we
                                   # defensively zero it after).
  movi.n  a3, 0
  l32i.n  a0, a1, 12
  addi    a1, a1, 16
  ret.n
  .literal_position
  .literal .Ladc_rom_addr, 0x40006A14
  .size cssc_adc_read, .-cssc_adc_read

  # --- ESP32 SAR_ADC1 12-bit measurement sequencer ------------------------
  # In:  a2 = channel (0..7 for SAR_ADC1)
  # Out: a2:a3 = i64 sample (low = 12-bit raw, high = 0)
  #
  # Programs SENS_SAR_MEAS_START1_REG (0x3FF48854):
  #   bit 12      EN_PAD_FORCE      — SW controls pad selection
  #   bit 13      START_FORCE       — SW controls start trigger
  #   bit 14      START_SAR         — write 1 to kick a measurement
  #   bit 15      DONE_SAR          — bootrom sets when result ready
  #   bits 11:0   EN_PAD            — channel-select bitmap (1 of 8 set)
  #   bits 31:16  DATA_SAR          — 16-bit raw result (12 useful bits)
  #
  # This is a polling implementation — the SAR finishes in ~20 µs at
  # default clkdiv, well under the bootrom WDT timeout. F1b' adds the
  # RTC_IO MUX setup that switches the channel's GPIO away from the
  # digital subsystem; for now we assume the user hasn't reconfigured
  # the pin to digital mode (which is the freshly-booted state).
  .global cssc_adc_esp32_read
  .type   cssc_adc_esp32_read, @function
.align  4
cssc_adc_esp32_read:
  l32r    a8, .Lsar_meas_start1
  # Build config: (1 << channel) | START_FORCE (0x2000) | EN_PAD_FORCE (0x1000)
  movi.n  a9, 1
  ssl     a2                       # SAR = channel
  sll     a10, a9                  # a10 = 1 << channel
  movi    a11, 0x3000              # START_FORCE | EN_PAD_FORCE
  or      a10, a10, a11
  s32i.n  a10, a8, 0               # clear START_SAR, set FORCEs + pad
  # Set START_SAR (bit 14) to kick the measurement.
  movi    a11, 0x4000
  or      a10, a10, a11
  s32i.n  a10, a8, 0
.Lsar_wait:
  l32i.n  a9, a8, 0
  bbci    a9, 15, .Lsar_wait       # loop while DONE_SAR (bit 15) clear
  # Result is in bits 31:16. Shift right by 16 to land it in a2[15:0].
  srli    a2, a9, 16
  movi.n  a3, 0
  ret.n
  .literal_position
  .literal .Lsar_meas_start1, 0x3FF48854
  .size cssc_adc_esp32_read, .-cssc_adc_esp32_read

  .global cssc_pwm_new
  .type   cssc_pwm_new, @function
.align  4
cssc_pwm_new:
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  movi.n  a2, 16
  call0   cssc_obj_alloc
  l32i.n  a0, a1, 12
  addi    a1, a1, 16
  ret.n
  .size cssc_pwm_new, .-cssc_pwm_new

  .global cssc_pwm_free
  .type   cssc_pwm_free, @function
.align  4
cssc_pwm_free:
  ret.n
  .size cssc_pwm_free, .-cssc_pwm_free

  .global cssc_pwm_duty
  .type   cssc_pwm_duty, @function
.align  4
cssc_pwm_duty:
  # Software PWM requires FRC1 interrupt handler — queued for Phase
  # D5b. For now persist the duty value so user code that reads
  # back the configured duty sees it.
  s32i.n  a4, a2, 12
  ret.n
  .size cssc_pwm_duty, .-cssc_pwm_duty

  .global cssc_timer_new
  .type   cssc_timer_new, @function
.align  4
cssc_timer_new:
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  movi.n  a2, 16
  call0   cssc_obj_alloc
  l32i.n  a0, a1, 12
  addi    a1, a1, 16
  ret.n
  .size cssc_timer_new, .-cssc_timer_new

  .global cssc_timer_free
  .type   cssc_timer_free, @function
.align  4
cssc_timer_free:
  ret.n
  .size cssc_timer_free, .-cssc_timer_free

  .global cssc_timer_start
  .type   cssc_timer_start, @function
.align  4
cssc_timer_start:
  # FRC1 ctrl: enable + auto-reload + clock=5MHz divider (DIV16).
  l32r    a8, .Lfrc1_load_addr
  l32r    a9, .Lfrc1_load_val
  l32i.n  a9, a9, 0
  s32i.n  a9, a8, 0
  l32r    a8, .Lfrc1_ctrl_addr
  l32r    a9, .Lfrc1_ctrl_val
  l32i.n  a9, a9, 0
  s32i.n  a9, a8, 0
  ret.n
  .literal_position
  .literal .Lfrc1_load_addr, 0x60000600
  .literal .Lfrc1_load_val,  .Lfrc1_load_val_d
  .literal .Lfrc1_ctrl_addr, 0x60000608
  .literal .Lfrc1_ctrl_val,  .Lfrc1_ctrl_val_d
  .section .rodata
  .align 4
.Lfrc1_load_val_d:
  .word 0x000F4240                 # 1_000_000 µs reload
.Lfrc1_ctrl_val_d:
  .word 0x000000C0                 # enable + auto-reload + 256-prescale
  .text
  .size cssc_timer_start, .-cssc_timer_start

  .global cssc_timer_stop
  .type   cssc_timer_stop, @function
.align  4
cssc_timer_stop:
  l32r    a8, .Lfrc1_ctrl_addr_b
  movi.n  a9, 0
  s32i.n  a9, a8, 0
  ret.n
  .literal_position
  .literal .Lfrc1_ctrl_addr_b, 0x60000608
  .size cssc_timer_stop, .-cssc_timer_stop

  .global cssc_timer_read
  .type   cssc_timer_read, @function
.align  4
cssc_timer_read:
  l32r    a8, .Lfrc1_count_addr
  l32i.n  a2, a8, 0
  movi.n  a3, 0
  ret.n
  .literal_position
  .literal .Lfrc1_count_addr, 0x60000604
  .size cssc_timer_read, .-cssc_timer_read

  # ===========================================================================
  # E2 — video / matrix / framebuffer / console.
  #
  # ESP8266 has no built-in display surface. These types exist on host
  # as Win32 windows / SDL surfaces; on bare metal we keep the handles
  # and persist the configured dimensions so user code observes
  # consistent reads, but no rendering happens. Real chips with
  # attached displays go through #tft / #oled (Phase E1).
  # ===========================================================================

  .global cssc_video_new
  .type   cssc_video_new, @function
.align  4
cssc_video_new:
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  movi.n  a2, 16
  call0   cssc_obj_alloc
  l32i.n  a0, a1, 12
  addi    a1, a1, 16
  ret.n
  .size cssc_video_new, .-cssc_video_new
  .global cssc_video_free
.align  4
cssc_video_free:
ret.n
  .global cssc_video_begin
.align  4
cssc_video_begin:
ret.n
  .global cssc_video_close
.align  4
cssc_video_close:
ret.n
  .global cssc_video_clear
.align  4
cssc_video_clear:
ret.n
  .global cssc_video_present
.align  4
cssc_video_present:
ret.n
  .global cssc_video_is_open
.align  4
cssc_video_is_open:
  movi.n  a2, 0
  movi.n  a3, 0
  ret.n

  .global cssc_matrix_new
.align  4
cssc_matrix_new:
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  movi.n  a2, 12
  call0   cssc_obj_alloc
  l32i.n  a0, a1, 12
  addi    a1, a1, 16
  ret.n
  .global cssc_matrix_free
.align  4
cssc_matrix_free:
ret.n
  .global cssc_matrix_fill
.align  4
cssc_matrix_fill:
ret.n
  .global cssc_matrix_pixel
.align  4
cssc_matrix_pixel:
ret.n
  .global cssc_matrix_show
.align  4
cssc_matrix_show:
ret.n

  .global cssc_framebuffer_new
.align  4
cssc_framebuffer_new:
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  movi.n  a2, 8
  call0   cssc_obj_alloc
  l32i.n  a0, a1, 12
  addi    a1, a1, 16
  ret.n
  .global cssc_framebuffer_free
.align  4
cssc_framebuffer_free:
ret.n
  .global cssc_framebuffer_fill
.align  4
cssc_framebuffer_fill:
ret.n
  .global cssc_framebuffer_pixel
.align  4
cssc_framebuffer_pixel:
ret.n
  .global cssc_framebuffer_present
.align  4
cssc_framebuffer_present:
ret.n

  .global cssc_console_new
.align  4
cssc_console_new:
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  movi.n  a2, 8
  call0   cssc_obj_alloc
  l32i.n  a0, a1, 12
  addi    a1, a1, 16
  ret.n
  .global cssc_console_free
.align  4
cssc_console_free:
ret.n
  .global cssc_console_write
.align  4
cssc_console_write:
  # Forward writes to UART0 stdout (cssc_uart_putc) for visibility.
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  s32i.n  a12, a1, 8
  s32i.n  a13, a1, 4
  # a4 = cssc_str* (string). Load size + data ptr, byte-stream through UART.
  l32i.n  a12, a4, 8               # data ptr
  l32i.n  a13, a4, 0               # size
con_loop:
  movi.n  a8, 0
  beq     a13, a8, con_done
  l8ui    a2, a12, 0
  call0   cssc_uart_putc
  addi    a12, a12, 1
  addi    a13, a13, -1
  j       con_loop
con_done:
  l32i.n  a13, a1, 4
  l32i.n  a12, a1, 8
  l32i.n  a0, a1, 12
  addi    a1, a1, 16
  ret.n
  .global cssc_console_clear
.align  4
cssc_console_clear:
ret.n
  .global cssc_console_close
.align  4
cssc_console_close:
ret.n

  # ===========================================================================
  # E1 — SSD1306 OLED driver (also serves SH1106, ILI9341, ST7735, ST7789
  # via per-controller branches in cssc_tft_begin). Bus: I2C for SSD1306/
  # SH1106; SPI for ILI9341/ST7735/ST7789. The handle records which bus.
  #
  # OLED handle layout (32 bytes):
  #   off 0:  i32 ctrl_id           (1=SSD1306, 2=SH1106, 3=ILI9341, ...)
  #   off 4:  i32 width
  #   off 8:  i32 height
  #   off 12: i32 sda_pin   (I2C) OR sck_pin (SPI)
  #   off 16: i32 scl_pin   (I2C) OR mosi_pin (SPI)
  #   off 20: i32 i2c_addr  (default 0x3C for SSD1306)
  #   off 24: i32 fb_ptr    (pointer to framebuffer in arena)
  #   off 28: i32 fb_size   (bytes — width*height/8 for monochrome)
  #
  # The 5x7 font table at .Lfont5x7_table is embedded as a 96 × 5-byte
  # block (ASCII 0x20..0x7F). Each character is 5 vertical columns of
  # 7 bits (MSB top, bit 7 unused). The default block is all zeros so
  # .text() renders blank rectangles until a font is dropped in — see
  # the rodata block; a follow-up phase replaces it with a real font.
  # ===========================================================================

  .global cssc_tft_new
  .type   cssc_tft_new, @function
.align  4
cssc_tft_new:
  # in:  a2 = ctrl_id (i64 lo), a4 = w (i64 lo), a6 = h (i64 lo)
  # out: a2 = pointer to 32-byte handle
  #
  # Frame layout (locals + saved regs, 32 bytes total):
  #   [a1,  0]: ctrl_id (saved)
  #   [a1,  4]: w
  #   [a1,  8]: h
  #   [a1, 12]: handle ptr (after alloc)
  #   [a1, 16]: saved a12
  #   [a1, 20]: saved a13
  #   [a1, 24]: saved a14
  #   [a1, 28]: saved a0 (return PC)
  addi    a1, a1, -32
  s32i.n  a0,  a1, 28
  s32i.n  a14, a1, 24
  s32i.n  a13, a1, 20
  s32i.n  a12, a1, 16
  s32i.n  a2,  a1, 0
  s32i.n  a4,  a1, 4
  s32i.n  a6,  a1, 8
  # Allocate the 32-byte handle.
  movi.n  a2, 32
  call0   cssc_obj_alloc           # a2 = handle
  s32i.n  a2, a1, 12
  # Populate handle: ctrl, w, h, defaults for I2C fields.
  l32i.n  a8, a1, 0
  s32i.n  a8, a2, 0                # ctrl
  l32i.n  a8, a1, 4
  s32i.n  a8, a2, 4                # w
  l32i.n  a8, a1, 8
  s32i.n  a8, a2, 8                # h
  movi.n  a8, 0
  s32i.n  a8, a2, 12               # sda_pin (caller fills via oled_new)
  s32i.n  a8, a2, 16               # scl_pin
  movi    a8, 0x3C
  s32i.n  a8, a2, 20               # default I2C address (SSD1306)
  # Framebuffer size = (w * h) / 8 bytes.
  l32i.n  a12, a1, 4               # w
  l32i.n  a13, a1, 8               # h
  mov.n   a2, a12
  mov.n   a3, a13
  call0   __mulsi3                 # a2 = w*h
  srli    a2, a2, 3                # / 8
  mov.n   a14, a2                  # fb_size
  l32i.n  a8, a1, 12               # handle
  s32i.n  a14, a8, 28              # persist fb_size in handle
  # Allocate the framebuffer itself.
  mov.n   a2, a14
  call0   cssc_obj_alloc           # a2 = fb ptr
  l32i.n  a8, a1, 12               # handle
  s32i.n  a2, a8, 24               # persist fb_ptr
  # Return the handle.
  mov.n   a2, a8
  l32i.n  a12, a1, 16
  l32i.n  a13, a1, 20
  l32i.n  a14, a1, 24
  l32i.n  a0,  a1, 28
  addi    a1, a1, 32
  ret.n
  .size cssc_tft_new, .-cssc_tft_new

  .global cssc_oled_new
  .type   cssc_oled_new, @function
.align  4
cssc_oled_new:
  # in:  a2=ctrl, a4=w, a6=h  (first 3 i64 args in reg-pairs);
  #      sda, scl, addr  are stack-passed (the caller reserved space
  #      for 3 wide args = 24 bytes at [a1, 0..23] BEFORE we entered).
  # out: a2 = handle ptr (32-byte struct, same layout as cssc_tft_new).
  #
  # Frame (48 bytes — must NOT clobber the caller's stack-arg slots
  # that sit just above our prologue's adjusted SP):
  #   [a1,  0]: handle ptr (after alloc)
  #   [a1,  4]: fb_size
  #   [a1,  8]: sda
  #   [a1, 12]: scl
  #   [a1, 16]: addr
  #   [a1, 20]: w
  #   [a1, 24]: h
  #   [a1, 28]: ctrl
  #   [a1, 32]: saved a12
  #   [a1, 36]: saved a13
  #   [a1, 40]: saved a14
  #   [a1, 44]: saved a0
  # Caller's stack-args (sda, scl, addr) live at [a1, 48], [a1, 56],
  # [a1, 64] respectively — above our prologue's adjusted SP.
  addi    a1, a1, -48
  s32i.n  a0,  a1, 44
  s32i.n  a14, a1, 40
  s32i.n  a13, a1, 36
  s32i.n  a12, a1, 32
  # Save the regs-args first.
  s32i.n  a2, a1, 28              # ctrl
  s32i.n  a4, a1, 20              # w
  s32i.n  a6, a1, 24              # h
  # Pull stack args from the caller's stack-arg area.
  l32i.n  a8, a1, 48              # sda (lo of first stack-arg pair)
  s32i.n  a8, a1, 8
  l32i.n  a8, a1, 56              # scl
  s32i.n  a8, a1, 12
  l32i.n  a8, a1, 64              # addr
  s32i.n  a8, a1, 16
  # Allocate 32-byte handle.
  movi.n  a2, 32
  call0   cssc_obj_alloc          # a2 = handle
  s32i.n  a2, a1, 0
  # Populate.
  l32i.n  a8, a1, 28
  s32i.n  a8, a2, 0                # ctrl
  l32i.n  a8, a1, 20
  s32i.n  a8, a2, 4                # w
  l32i.n  a8, a1, 24
  s32i.n  a8, a2, 8                # h
  l32i.n  a8, a1, 8
  s32i.n  a8, a2, 12               # sda
  l32i.n  a8, a1, 12
  s32i.n  a8, a2, 16               # scl
  l32i.n  a8, a1, 16
  s32i.n  a8, a2, 20               # addr
  # fb_size = (w * h) / 8
  l32i.n  a12, a1, 20
  l32i.n  a13, a1, 24
  mov.n   a2, a12
  mov.n   a3, a13
  call0   __mulsi3
  srli    a2, a2, 3
  s32i.n  a2, a1, 4                # persist fb_size in frame
  l32i.n  a8, a1, 0                # handle
  s32i.n  a2, a8, 28               # persist fb_size in handle
  # Allocate framebuffer.
  l32i.n  a2, a1, 4
  call0   cssc_obj_alloc           # a2 = fb_ptr
  l32i.n  a8, a1, 0                # handle
  s32i.n  a2, a8, 24               # persist fb_ptr in handle
  # Return handle.
  mov.n   a2, a8
  l32i.n  a12, a1, 32
  l32i.n  a13, a1, 36
  l32i.n  a14, a1, 40
  l32i.n  a0,  a1, 44
  addi    a1, a1, 48
  ret.n
  .size cssc_oled_new, .-cssc_oled_new

  .global cssc_tft_free
.align  4
cssc_tft_free:
ret.n
  .global cssc_oled_free
.align  4
cssc_oled_free:
ret.n
  .global cssc_tft_close
.align  4
cssc_tft_close:
ret.n
  .global cssc_oled_close
.align  4
cssc_oled_close:
ret.n

  .global cssc_tft_begin
  .type   cssc_tft_begin, @function
.align  4
cssc_tft_begin:
  # in: a2 = handle
  # Sends the SSD1306 init sequence over I2C to the configured
  # sda/scl pins. Each command byte ships as <addr> <0x00 ctl> <cmd>.
  # The init sequence is 27 commands (right after the table label).
  #
  # CALL0 ABI: we clobber a12, a13, a14, a15 as scratch (handle,
  # cmd-table cursor, count, byte respectively). All four are
  # callee-save and must be preserved across the call.
  addi    a1, a1, -32
  s32i.n  a0, a1, 28
  s32i.n  a12, a1, 24
  s32i.n  a13, a1, 20
  s32i.n  a14, a1, 16
  s32i.n  a15, a1, 12
  mov.n   a12, a2                  # handle
  # Build an inline i2c handle: borrow the cssc_i2c_write machinery by
  # constructing a stack-resident handle that points at our sda/scl.
  l32i.n  a8, a12, 12              # sda
  l32i.n  a9, a12, 16              # scl
  s32i.n  a8, a1, 4                # inline handle: sda @ +4
  s32i.n  a9, a1, 8                # scl @ +8
  movi    a8, 400                  # half_bit cycles (100 kHz default)
  s32i.n  a8, a1, 12
  movi.n  a8, 0
  s32i.n  a8, a1, 0                # bus_id (unused)
  # Configure IOMUX for both SDA + SCL pads → function 3 (GPIO) +
  # internal pullup. Without this the pins stay in their boot-default
  # alternate function (e.g. JTAG/MTDI/MTCK on GPIO12/14) and the
  # subsequent cssc_i2c_drive writes have no observable effect on
  # the physical lines — the OLED never sees the START condition.
  l32i.n  a8, a12, 12              # sda pin no
  call0   .cssc_pin_iomux_gpio_pu
  l32i.n  a8, a12, 16              # scl pin no
  call0   .cssc_pin_iomux_gpio_pu
  # Drive both pads high (release) by setting GPIO_OUT_W1TC + then
  # enabling output. cssc_i2c_drive() handles per-bit transitions.
  # Walk the init table. Each command byte is sent as a 2-byte I2C
  # transaction: [addr+W, 0x00 (cmd-prefix), cmd_byte]. Sending
  # them as separate 1-byte transactions (the old code path) is
  # misinterpreted by the SSD1306 as a control byte alone and the
  # init never takes effect — the OLED stays dark.
  # cssc_i2c_write_pair takes (handle, addr, b1=0x00, b2=cmd) with
  # b2 passed via the outgoing-stack-arg slot at [a1, 0].
  #
  # The init table has 25 bytes that mirror the working --gcc path's
  # sequence in cssc_tft.c (`_ssd_init`). The last byte 0xAF (display
  # ON) MUST be sent — earlier versions had count=27 with a 28-byte
  # table, dropping 0xAF and leaving the panel powered-but-dark.
  l32r    a13, .Lssd1306_init_addr
  movi.n  a14, 25                  # count — MUST match table length
ti1:
  movi.n  a8, 0
  beq     a14, a8, ti_done
  l8ui    a15, a13, 0              # next cmd byte
  # Reserve 16 bytes of outgoing stack args (b2 is a wide arg via the
  # pair-aligned ABI; we only use the lo half but allocate the pair).
  addi    a1, a1, -16
  mov.n   a8, a15
  s32i.n  a8, a1, 0                # b2 lo
  movi.n  a8, 0
  s32i.n  a8, a1, 4                # b2 hi (unused)
  addi    a2, a1, 16               # &inline_handle (caller frame, parent +0)
  l32i.n  a4, a12, 20              # addr
  movi.n  a6, 0x00                 # b1 = control prefix (commands continuous)
  call0   cssc_i2c_write_pair
  addi    a1, a1, 16               # pop stack arg block
  addi    a13, a13, 1
  addi    a14, a14, -1
  j       ti1
ti_done:
  l32i.n  a15, a1, 12
  l32i.n  a14, a1, 16
  l32i.n  a13, a1, 20
  l32i.n  a12, a1, 24
  l32i.n  a0,  a1, 28
  addi    a1, a1, 32
  ret.n
  .section .rodata
  .align 4
.Lssd1306_init:
  # SSD1306 power-on init for 128x64. Order + values mirror the
  # working --gcc path in cssc_tft.c (_ssd_init) which is itself
  # Adafruit-canonical. KEY constraints:
  #   * 0x8D 0x14 (charge pump ON) MUST come BEFORE 0xAF (display ON)
  #     because the OLED segments draw current from the pump.
  #   * 0xD9 0xF1 (precharge for internal Vcc) — value 0x22 used by
  #     the old table is for EXTERNAL Vcc and leaves the panel dark.
  #   * 0xDB 0x40 (VCOMH deselect ≈ 0.77*VCC) — the prior 0x20 is too
  #     low for the charge-pump variant.
  #   * 0x81 0xCF (contrast 0xCF) — 0x7F works but is noticeably
  #     dimmer; matches the legacy path for visual parity.
  #   * 25 bytes total; the count loop in cssc_tft_begin reads
  #     `movi.n a14, 25` — keep these in sync.
  .byte 0xAE              # 1.  display OFF
  .byte 0xD5, 0x80        # 2.  display clock divide ratio / oscillator freq
  .byte 0xA8, 0x3F        # 4.  multiplex ratio (63 → 64 rows)
  .byte 0xD3, 0x00        # 6.  display offset = 0
  .byte 0x40              # 8.  set display start line = 0
  .byte 0x8D, 0x14        # 9.  enable internal charge pump (REQUIRED)
  .byte 0x20, 0x00        # 11. memory addressing mode = horizontal
  .byte 0xA1              # 13. segment remap (col 127 → SEG0)
  .byte 0xC8              # 14. COM scan dir reversed
  .byte 0xDA, 0x12        # 15. COM pins hw config (alt, 128x64)
  .byte 0x81, 0xCF        # 17. contrast control
  .byte 0xD9, 0xF1        # 19. pre-charge period (internal Vcc)
  .byte 0xDB, 0x40        # 21. VCOMH deselect level
  .byte 0xA4              # 23. resume display from RAM
  .byte 0xA6              # 24. normal (non-inverted) mode
  .byte 0xAF              # 25. display ON  ← MUST be the last byte sent
  # --- 5x7 font table (ASCII 0x20..0x7F, 5 columns per glyph) -------
  # Loaded from the project's existing cssc_tft.c so the host and
  # Xtensa runtimes share one source of truth. Embedded in .rodata
  # → maps to flash on ESP8266 via the linker section assignment.
  .global .Lfont5x7_table
.Lfont5x7_table:
  .byte 0x00, 0x00, 0x00, 0x00, 0x00    # 0x20 ' '
  .byte 0x00, 0x00, 0x5F, 0x00, 0x00    # 0x21 '!'
  .byte 0x00, 0x07, 0x00, 0x07, 0x00    # 0x22 '"'
  .byte 0x14, 0x7F, 0x14, 0x7F, 0x14    # 0x23 '#'
  .byte 0x24, 0x2A, 0x7F, 0x2A, 0x12    # 0x24 '$'
  .byte 0x23, 0x13, 0x08, 0x64, 0x62    # 0x25 '%'
  .byte 0x36, 0x49, 0x55, 0x22, 0x50    # 0x26 '&'
  .byte 0x00, 0x05, 0x03, 0x00, 0x00    # 0x27 "'"
  .byte 0x00, 0x1C, 0x22, 0x41, 0x00    # 0x28 '('
  .byte 0x00, 0x41, 0x22, 0x1C, 0x00    # 0x29 ')'
  .byte 0x14, 0x08, 0x3E, 0x08, 0x14    # 0x2A '*'
  .byte 0x08, 0x08, 0x3E, 0x08, 0x08    # 0x2B '+'
  .byte 0x00, 0x50, 0x30, 0x00, 0x00    # 0x2C ','
  .byte 0x08, 0x08, 0x08, 0x08, 0x08    # 0x2D '-'
  .byte 0x00, 0x60, 0x60, 0x00, 0x00    # 0x2E '.'
  .byte 0x20, 0x10, 0x08, 0x04, 0x02    # 0x2F '/'
  .byte 0x3E, 0x51, 0x49, 0x45, 0x3E    # 0x30 '0'
  .byte 0x00, 0x42, 0x7F, 0x40, 0x00    # 0x31 '1'
  .byte 0x42, 0x61, 0x51, 0x49, 0x46    # 0x32 '2'
  .byte 0x21, 0x41, 0x45, 0x4B, 0x31    # 0x33 '3'
  .byte 0x18, 0x14, 0x12, 0x7F, 0x10    # 0x34 '4'
  .byte 0x27, 0x45, 0x45, 0x45, 0x39    # 0x35 '5'
  .byte 0x3C, 0x4A, 0x49, 0x49, 0x30    # 0x36 '6'
  .byte 0x01, 0x71, 0x09, 0x05, 0x03    # 0x37 '7'
  .byte 0x36, 0x49, 0x49, 0x49, 0x36    # 0x38 '8'
  .byte 0x06, 0x49, 0x49, 0x29, 0x1E    # 0x39 '9'
  .byte 0x00, 0x36, 0x36, 0x00, 0x00    # 0x3A ':'
  .byte 0x00, 0x56, 0x36, 0x00, 0x00    # 0x3B ';'
  .byte 0x00, 0x08, 0x14, 0x22, 0x41    # 0x3C '<'
  .byte 0x14, 0x14, 0x14, 0x14, 0x14    # 0x3D '='
  .byte 0x41, 0x22, 0x14, 0x08, 0x00    # 0x3E '>'
  .byte 0x02, 0x01, 0x51, 0x09, 0x06    # 0x3F '?'
  .byte 0x32, 0x49, 0x79, 0x41, 0x3E    # 0x40 '@'
  .byte 0x7E, 0x11, 0x11, 0x11, 0x7E    # 0x41 'A'
  .byte 0x7F, 0x49, 0x49, 0x49, 0x36    # 0x42 'B'
  .byte 0x3E, 0x41, 0x41, 0x41, 0x22    # 0x43 'C'
  .byte 0x7F, 0x41, 0x41, 0x22, 0x1C    # 0x44 'D'
  .byte 0x7F, 0x49, 0x49, 0x49, 0x41    # 0x45 'E'
  .byte 0x7F, 0x09, 0x09, 0x01, 0x01    # 0x46 'F'
  .byte 0x3E, 0x41, 0x41, 0x51, 0x32    # 0x47 'G'
  .byte 0x7F, 0x08, 0x08, 0x08, 0x7F    # 0x48 'H'
  .byte 0x00, 0x41, 0x7F, 0x41, 0x00    # 0x49 'I'
  .byte 0x20, 0x40, 0x41, 0x3F, 0x01    # 0x4A 'J'
  .byte 0x7F, 0x08, 0x14, 0x22, 0x41    # 0x4B 'K'
  .byte 0x7F, 0x40, 0x40, 0x40, 0x40    # 0x4C 'L'
  .byte 0x7F, 0x02, 0x04, 0x02, 0x7F    # 0x4D 'M'
  .byte 0x7F, 0x04, 0x08, 0x10, 0x7F    # 0x4E 'N'
  .byte 0x3E, 0x41, 0x41, 0x41, 0x3E    # 0x4F 'O'
  .byte 0x7F, 0x09, 0x09, 0x09, 0x06    # 0x50 'P'
  .byte 0x3E, 0x41, 0x51, 0x21, 0x5E    # 0x51 'Q'
  .byte 0x7F, 0x09, 0x19, 0x29, 0x46    # 0x52 'R'
  .byte 0x46, 0x49, 0x49, 0x49, 0x31    # 0x53 'S'
  .byte 0x01, 0x01, 0x7F, 0x01, 0x01    # 0x54 'T'
  .byte 0x3F, 0x40, 0x40, 0x40, 0x3F    # 0x55 'U'
  .byte 0x1F, 0x20, 0x40, 0x20, 0x1F    # 0x56 'V'
  .byte 0x7F, 0x20, 0x18, 0x20, 0x7F    # 0x57 'W'
  .byte 0x63, 0x14, 0x08, 0x14, 0x63    # 0x58 'X'
  .byte 0x03, 0x04, 0x78, 0x04, 0x03    # 0x59 'Y'
  .byte 0x61, 0x51, 0x49, 0x45, 0x43    # 0x5A 'Z'
  .byte 0x00, 0x00, 0x7F, 0x41, 0x41    # 0x5B '['
  .byte 0x02, 0x04, 0x08, 0x10, 0x20    # 0x5C '\\'
  .byte 0x41, 0x41, 0x7F, 0x00, 0x00    # 0x5D ']'
  .byte 0x04, 0x02, 0x01, 0x02, 0x04    # 0x5E '^'
  .byte 0x40, 0x40, 0x40, 0x40, 0x40    # 0x5F '_'
  .byte 0x00, 0x01, 0x02, 0x04, 0x00    # 0x60 '`'
  .byte 0x20, 0x54, 0x54, 0x54, 0x78    # 0x61 'a'
  .byte 0x7F, 0x48, 0x44, 0x44, 0x38    # 0x62 'b'
  .byte 0x38, 0x44, 0x44, 0x44, 0x20    # 0x63 'c'
  .byte 0x38, 0x44, 0x44, 0x48, 0x7F    # 0x64 'd'
  .byte 0x38, 0x54, 0x54, 0x54, 0x18    # 0x65 'e'
  .byte 0x08, 0x7E, 0x09, 0x01, 0x02    # 0x66 'f'
  .byte 0x08, 0x14, 0x54, 0x54, 0x3C    # 0x67 'g'
  .byte 0x7F, 0x08, 0x04, 0x04, 0x78    # 0x68 'h'
  .byte 0x00, 0x44, 0x7D, 0x40, 0x00    # 0x69 'i'
  .byte 0x20, 0x40, 0x44, 0x3D, 0x00    # 0x6A 'j'
  .byte 0x00, 0x7F, 0x10, 0x28, 0x44    # 0x6B 'k'
  .byte 0x00, 0x41, 0x7F, 0x40, 0x00    # 0x6C 'l'
  .byte 0x7C, 0x04, 0x18, 0x04, 0x78    # 0x6D 'm'
  .byte 0x7C, 0x08, 0x04, 0x04, 0x78    # 0x6E 'n'
  .byte 0x38, 0x44, 0x44, 0x44, 0x38    # 0x6F 'o'
  .byte 0x7C, 0x14, 0x14, 0x14, 0x08    # 0x70 'p'
  .byte 0x08, 0x14, 0x14, 0x18, 0x7C    # 0x71 'q'
  .byte 0x7C, 0x08, 0x04, 0x04, 0x08    # 0x72 'r'
  .byte 0x48, 0x54, 0x54, 0x54, 0x20    # 0x73 's'
  .byte 0x04, 0x3F, 0x44, 0x40, 0x20    # 0x74 't'
  .byte 0x3C, 0x40, 0x40, 0x20, 0x7C    # 0x75 'u'
  .byte 0x1C, 0x20, 0x40, 0x20, 0x1C    # 0x76 'v'
  .byte 0x3C, 0x40, 0x30, 0x40, 0x3C    # 0x77 'w'
  .byte 0x44, 0x28, 0x10, 0x28, 0x44    # 0x78 'x'
  .byte 0x0C, 0x50, 0x50, 0x50, 0x3C    # 0x79 'y'
  .byte 0x44, 0x64, 0x54, 0x4C, 0x44    # 0x7A 'z'
  .byte 0x00, 0x08, 0x36, 0x41, 0x00    # 0x7B '{'
  .byte 0x00, 0x00, 0x7F, 0x00, 0x00    # 0x7C '|'
  .byte 0x00, 0x41, 0x36, 0x08, 0x00    # 0x7D '}'
  .byte 0x08, 0x04, 0x08, 0x10, 0x08    # 0x7E '~'
  .byte 0x00, 0x00, 0x00, 0x00, 0x00    # 0x7F '?'
  .text
  .literal_position
  .literal .Lssd1306_init_addr, .Lssd1306_init
  .literal .Lfont5x7_addr,      .Lfont5x7_table
  .size cssc_tft_begin, .-cssc_tft_begin

  .global cssc_oled_begin
.align  4
cssc_oled_begin:
  # Alias: same impl.
  j       cssc_tft_begin

  .global cssc_tft_fill
  .type   cssc_tft_fill, @function
.align  4
cssc_tft_fill:
  # in: a2 = handle, a4 = color (i64 lo: 0=black, anything else=white)
  l32i.n  a8, a2, 24               # fb ptr
  l32i.n  a9, a2, 28               # fb size in bytes
  movi.n  a10, 0
  beq     a4, a10, fl_zero
  movi    a10, 0xFF
fl_zero:
fl_loop:
  movi.n  a11, 0
  beq     a9, a11, fl_done
  s8i     a10, a8, 0
  addi    a8, a8, 1
  addi    a9, a9, -1
  j       fl_loop
fl_done:
  ret.n
  .size cssc_tft_fill, .-cssc_tft_fill

  .global cssc_oled_fill
.align  4
cssc_oled_fill:
j cssc_tft_fill

  .global cssc_tft_pixel
  .type   cssc_tft_pixel, @function
.align  4
cssc_tft_pixel:
  # in: a2 = handle, a4 = x (i64 lo), a6 = y (i64 lo), a8 = color (i64 lo)
  # Set/clear bit (y % 8) at byte index (x + (y/8) * w) in the framebuffer.
  # ABI: a8 is the 4th arg's lo, but a8 is also a scratch register. The
  # CIR caller materializes args into a2..a7; for a 4-arg int call,
  # arg #4 goes to (a8, a9) since 3 wide args ate (a2:a3, a4:a5, a6:a7).
  # That means our 4th arg lo lives at a8 — exactly where wide-arg
  # layout puts it. (See `_arg_reg_layout`.)
  #
  # Frame: we need to save a0 because we call __mulsi3, and we need
  # to preserve a12..a15 because we clobber them as scratch and they
  # are CALL0-callee-save (the caller — typically cssc_tft_text /
  # cssc_tft_line / cssc_tft_fillrect — expects them intact).
  addi    a1, a1, -32
  s32i.n  a0,  a1, 28
  s32i.n  a12, a1, 24
  s32i.n  a13, a1, 20
  s32i.n  a14, a1, 16
  s32i.n  a15, a1, 12
  s32i.n  a8,  a1, 8               # preserve color across __mulsi3
  l32i.n  a10, a2, 24              # fb ptr
  l32i.n  a11, a2, 4               # width
  # Clip y >= height → no-op
  l32i.n  a3, a2, 8                # height
  bge     a6, a3, pix_done
  bltz    a6, pix_done
  bge     a4, a11, pix_done
  bltz    a4, pix_done
  # page = y / 8; bit = y % 8
  srli    a12, a6, 3               # page
  movi.n  a13, 7
  and     a14, a6, a13             # bit
  # Persist x and fb_ptr across the __mulsi3 call.
  s32i.n  a4,  a1, 4               # x
  s32i.n  a10, a1, 0               # fb_ptr
  # index = page * w + x
  mov.n   a2, a12
  mov.n   a3, a11
  call0   __mulsi3                 # a2 = page*w
  l32i.n  a4,  a1, 4               # restore x
  l32i.n  a10, a1, 0               # restore fb_ptr
  l32i.n  a8,  a1, 8               # restore color
  add     a15, a2, a4              # index
  # mask = 1 << bit
  movi.n  a2, 1
  ssl     a14
  sll     a2, a2
  add     a3, a10, a15             # addr of byte
  l8ui    a4, a3, 0
  movi.n  a5, 0
  beq     a8, a5, pix_clear
  or      a4, a4, a2               # set
  j       pix_store
pix_clear:
  # ~mask & current
  movi.n  a5, -1
  xor     a2, a2, a5
  and     a4, a4, a2
pix_store:
  s8i     a4, a3, 0
pix_done:
  l32i.n  a15, a1, 12
  l32i.n  a14, a1, 16
  l32i.n  a13, a1, 20
  l32i.n  a12, a1, 24
  l32i.n  a0,  a1, 28
  addi    a1, a1, 32
  ret.n
  .size cssc_tft_pixel, .-cssc_tft_pixel

  .global cssc_oled_pixel
.align  4
cssc_oled_pixel:
j cssc_tft_pixel

  # Drawing primitives (line / rect / fillrect / circle / text) — these
  # build on cssc_tft_pixel. Their full bodies follow the Bresenham
  # algorithm conventions; for brevity each function calls pixel in a
  # loop. Real performance optimization (column-major writes) is in
  # Phase E1b.
  # ===========================================================================
  # E1b — full OLED drawing primitives.
  #
  # Each function builds on `cssc_tft_pixel` for individual pixel
  # writes, then iterates over the geometry. Frame ABI for the
  # multi-arg shapes (5 int args after handle) means args after the
  # 4th wide come from the stack-arg area: caller wrote
  #   [a1, 0..7]   = x1   (wide)
  #   [a1, 8..15]  = y1
  #   ...
  # immediately before the call. Our prologue reserves frame
  # without touching that area; we read stack args at
  # `[a1, frame_size + 0..]`.
  # ===========================================================================

  .global cssc_tft_line
  .type   cssc_tft_line, @function
.align  4
cssc_tft_line:
  # in:  handle=a2, x0=a4, y0=a6  (in regs)
  #      x1, y1, color  (on stack at [a1, 0..23] before our prologue)
  # Implements Bresenham's classic line algorithm. The end-point
  # check uses x0==x1 && y0==y1 so a single-pixel line also plots
  # the start.
  #
  # A.1.1 FRAME LAYOUT (64-byte frame, saves ABOVE locals so they
  # cannot collide). Previous 48-byte layout had a12-a15 saved at
  # offsets 28..40 — exactly where `dx`/`sy`/`-dy`/`err` were also
  # written. Every line() call corrupted the caller's callee-save
  # regs; this manifested as scrambled text + missing pixels when
  # the caller (tft_text, render loops) kept loop state in a12-a15.
  # New layout: locals @ +0..+43, saves @ +44..+63.
  addi    a1, a1, -64
  s32i.n  a0,  a1, 60
  s32i.n  a12, a1, 56
  s32i.n  a13, a1, 52
  s32i.n  a14, a1, 48
  s32i.n  a15, a1, 44
  s32i.n  a2,  a1, 0                # handle
  s32i.n  a4,  a1, 4                # x0 (mutable)
  s32i.n  a6,  a1, 8                # y0 (mutable)
  # Pull stack args (lives ABOVE our adjusted SP at our frame_size+).
  # Frame is now 64 bytes, so stack args sit at +64..+71/+72..+79/+80..+87.
  l32i    a8,  a1, 64               # x1
  s32i.n  a8,  a1, 12
  l32i    a8,  a1, 72               # y1
  s32i.n  a8,  a1, 16
  l32i    a8,  a1, 80               # color
  s32i.n  a8,  a1, 20
  # dx = abs(x1-x0); sx = (x0<x1)?+1:-1
  l32i.n  a8, a1, 12                # x1
  l32i.n  a9, a1, 4                 # x0
  sub     a10, a8, a9
  movi.n  a11, 0
  bge     a10, a11, ln_dx_pos
  neg     a10, a10                  # dx absolute
  movi    a11, -1
  s32i.n  a11, a1, 24               # sx = -1
  j       ln_dx_done
ln_dx_pos:
  movi.n  a11, 1
  s32i.n  a11, a1, 24               # sx = +1
ln_dx_done:
  s32i.n  a10, a1, 28               # dx
  # dy = abs(y1-y0); sy = ...
  l32i.n  a8, a1, 16                # y1
  l32i.n  a9, a1, 8                 # y0
  sub     a10, a8, a9
  movi.n  a11, 0
  bge     a10, a11, ln_dy_pos
  neg     a10, a10
  movi    a11, -1
  s32i.n  a11, a1, 32               # sy = -1
  j       ln_dy_done
ln_dy_pos:
  movi.n  a11, 1
  s32i.n  a11, a1, 32               # sy = +1
ln_dy_done:
  # dy is stored NEGATED in the standard Bresenham to allow signed
  # comparison with err. Store -dy instead.
  neg     a10, a10
  s32i.n  a10, a1, 36               # -dy (working value)
  # err = dx + (-dy)   (i.e. dx - dy with our convention)
  l32i.n  a8, a1, 28
  l32i.n  a9, a1, 36
  add     a8, a8, a9
  s32i.n  a8, a1, 40                # err
ln_loop:
  # Plot (x0, y0) via cssc_tft_pixel(handle, x0, y0, color).
  l32i.n  a2, a1, 0
  l32i.n  a4, a1, 4
  l32i.n  a6, a1, 8
  l32i.n  a8, a1, 20                # color
  call0   cssc_tft_pixel
  # if (x0 == x1 && y0 == y1) break
  l32i.n  a8, a1, 4
  l32i.n  a9, a1, 12
  bne     a8, a9, ln_step
  l32i.n  a8, a1, 8
  l32i.n  a9, a1, 16
  bne     a8, a9, ln_step
  j       ln_done
ln_step:
  # e2 = 2 * err
  l32i.n  a8, a1, 40
  add     a9, a8, a8                # e2 = err << 1
  # if (e2 >= -dy) { err += -dy; x0 += sx; }
  l32i.n  a10, a1, 36               # -dy (already negated)
  blt     a9, a10, ln_y_part
  # err += -dy
  l32i.n  a11, a1, 40
  add     a11, a11, a10
  s32i.n  a11, a1, 40
  # x0 += sx
  l32i.n  a11, a1, 4
  l32i.n  a12, a1, 24
  add     a11, a11, a12
  s32i.n  a11, a1, 4
ln_y_part:
  # if (e2 <= dx) { err += dx; y0 += sy; }
  l32i.n  a10, a1, 28               # dx
  # (Recompute e2 since the x-part might have updated err.)
  l32i.n  a8, a1, 40
  add     a9, a8, a8
  blt     a10, a9, ln_loop          # e2 > dx → skip y update
  l32i.n  a11, a1, 40
  l32i.n  a12, a1, 28
  add     a11, a11, a12
  s32i.n  a11, a1, 40
  l32i.n  a11, a1, 8
  l32i.n  a12, a1, 32
  add     a11, a11, a12
  s32i.n  a11, a1, 8
  j       ln_loop
ln_done:
  l32i.n  a15, a1, 44
  l32i.n  a14, a1, 48
  l32i.n  a13, a1, 52
  l32i.n  a12, a1, 56
  l32i.n  a0,  a1, 60
  addi    a1, a1, 64
  ret.n
  .size cssc_tft_line, .-cssc_tft_line

  .global cssc_oled_line
.align  4
cssc_oled_line:
j cssc_tft_line

  # --- rect (outline) — 4 line calls ----------------------------------
  .global cssc_tft_rect
  .type   cssc_tft_rect, @function
.align  4
cssc_tft_rect:
  # in: handle, x, y, w, h, color  (4 stack args)
  # Outline = top edge + bottom + left + right.
  addi    a1, a1, -48
  s32i.n  a0,  a1, 44
  s32i.n  a12, a1, 40
  s32i.n  a13, a1, 36
  s32i.n  a14, a1, 32
  # Save in-reg args.
  s32i.n  a2, a1, 0                 # handle
  s32i.n  a4, a1, 4                 # x
  s32i.n  a6, a1, 8                 # y
  # Stack args at [a1, frame+0..]:
  l32i.n  a8, a1, 48                # w
  s32i.n  a8, a1, 12
  l32i.n  a8, a1, 56                # h
  s32i.n  a8, a1, 16
  l32i.n  a8, a1, 64                # color
  s32i.n  a8, a1, 20
  # x_end = x + w - 1; y_end = y + h - 1
  l32i.n  a8, a1, 4
  l32i.n  a9, a1, 12
  add     a8, a8, a9
  addi    a8, a8, -1
  s32i.n  a8, a1, 24                # x_end
  l32i.n  a8, a1, 8
  l32i.n  a9, a1, 16
  add     a8, a8, a9
  addi    a8, a8, -1
  s32i.n  a8, a1, 28                # y_end
  # Emit 4 line(...) calls by reserving 24 bytes of stack-arg
  # space and stuffing (x1, y1, color) into it for each call.
  # Top edge: line(handle, x, y, x_end, y, color)
  addi    a1, a1, -24
  l32i.n  a8, a1, 48                # x_end (was a1+24, now a1+48 after prologue+sub)
  s32i.n  a8, a1, 0
  l32i.n  a8, a1, 32
  s32i.n  a8, a1, 8
  l32i.n  a8, a1, 44
  s32i.n  a8, a1, 16
  l32i.n  a2, a1, 24                # handle
  l32i.n  a4, a1, 28                # x
  l32i.n  a6, a1, 32                # y
  call0   cssc_tft_line
  addi    a1, a1, 24
  # Bottom edge.
  addi    a1, a1, -24
  l32i.n  a8, a1, 48
  s32i.n  a8, a1, 0                 # x_end
  l32i.n  a8, a1, 52
  s32i.n  a8, a1, 8                 # y_end
  l32i.n  a8, a1, 44
  s32i.n  a8, a1, 16                # color
  l32i.n  a2, a1, 24
  l32i.n  a4, a1, 28
  l32i.n  a6, a1, 52                # y_end as start-y
  call0   cssc_tft_line
  addi    a1, a1, 24
  # Left edge.
  addi    a1, a1, -24
  l32i.n  a8, a1, 28
  s32i.n  a8, a1, 0                 # x1 = x
  l32i.n  a8, a1, 52
  s32i.n  a8, a1, 8                 # y1 = y_end
  l32i.n  a8, a1, 44
  s32i.n  a8, a1, 16
  l32i.n  a2, a1, 24
  l32i.n  a4, a1, 28
  l32i.n  a6, a1, 32
  call0   cssc_tft_line
  addi    a1, a1, 24
  # Right edge.
  addi    a1, a1, -24
  l32i.n  a8, a1, 48
  s32i.n  a8, a1, 0                 # x1 = x_end
  l32i.n  a8, a1, 52
  s32i.n  a8, a1, 8                 # y1 = y_end
  l32i.n  a8, a1, 44
  s32i.n  a8, a1, 16
  l32i.n  a2, a1, 24
  l32i.n  a4, a1, 48                # x = x_end
  l32i.n  a6, a1, 32
  call0   cssc_tft_line
  addi    a1, a1, 24
  l32i.n  a14, a1, 32
  l32i.n  a13, a1, 36
  l32i.n  a12, a1, 40
  l32i.n  a0,  a1, 44
  addi    a1, a1, 48
  ret.n
  .size cssc_tft_rect, .-cssc_tft_rect

  .global cssc_oled_rect
.align  4
cssc_oled_rect:
j cssc_tft_rect

  # --- fillrect — nested loop of horizontal line plots ----------------
  .global cssc_tft_fillrect
  .type   cssc_tft_fillrect, @function
.align  4
cssc_tft_fillrect:
  # in: handle, x, y, w, h, color (4 stack args)
  addi    a1, a1, -48
  s32i.n  a0,  a1, 44
  s32i.n  a12, a1, 40
  s32i.n  a13, a1, 36
  s32i.n  a14, a1, 32
  s32i.n  a2, a1, 0                 # handle
  s32i.n  a4, a1, 4                 # x
  s32i.n  a6, a1, 8                 # y
  l32i.n  a8, a1, 48                # w
  s32i.n  a8, a1, 12
  l32i.n  a8, a1, 56                # h
  s32i.n  a8, a1, 16
  l32i.n  a8, a1, 64                # color
  s32i.n  a8, a1, 20
  # Loop j from 0 to h, plotting a horizontal line per row.
  movi.n  a12, 0                    # j
fr_outer:
  l32i.n  a8, a1, 16                # h
  bge     a12, a8, fr_done
  # Inner loop: i from 0 to w → cssc_tft_pixel(handle, x+i, y+j, color)
  movi.n  a13, 0
fr_inner:
  l32i.n  a8, a1, 12                # w
  bge     a13, a8, fr_inext
  l32i.n  a2, a1, 0
  l32i.n  a8, a1, 4
  add     a4, a8, a13               # x + i
  l32i.n  a8, a1, 8
  add     a6, a8, a12               # y + j
  l32i.n  a8, a1, 20                # color
  call0   cssc_tft_pixel
  addi    a13, a13, 1
  j       fr_inner
fr_inext:
  addi    a12, a12, 1
  j       fr_outer
fr_done:
  l32i.n  a14, a1, 32
  l32i.n  a13, a1, 36
  l32i.n  a12, a1, 40
  l32i.n  a0,  a1, 44
  addi    a1, a1, 48
  ret.n
  .size cssc_tft_fillrect, .-cssc_tft_fillrect

  .global cssc_oled_fillrect
.align  4
cssc_oled_fillrect:
j cssc_tft_fillrect

  # --- circle — midpoint algorithm, 8-fold symmetry -------------------
  .global cssc_tft_circle
  .type   cssc_tft_circle, @function
.align  4
cssc_tft_circle:
  # Phase E1c — full midpoint (Bresenham) circle algorithm.
  #
  # in:  handle, cx, cy, r, color
  #        a2 = handle, a4 = cx_lo, a6 = cy_lo,
  #        stack[0..3] = r (i64 lo), stack[8..11] = color (i64 lo)
  # out: 8 octant pixels per iteration plus 4 cardinal pixels at start.
  #
  # Frame layout (80 bytes):
  #   [0]  handle
  #   [4]  cx
  #   [8]  cy
  #   [12] r
  #   [16] color
  #   [20] f          (decision variable: 1 - r initially)
  #   [24] ddF_x      (1 initially)
  #   [28] ddF_y      (-2*r initially)
  #   [32] x          (loop state, starts at 0)
  #   [36] y          (loop state, starts at r)
  #   [60..72] saved a12..a15
  #   [76] saved a0 (return PC)
  addi    a1, a1, -80
  s32i.n  a0,  a1, 76
  s32i    a12, a1, 60
  s32i    a13, a1, 64
  s32i    a14, a1, 68
  s32i    a15, a1, 72
  s32i.n  a2,  a1, 0
  s32i.n  a4,  a1, 4
  s32i.n  a6,  a1, 8
  # Stack args: pixel-call adds 16 bytes for outgoing args; our
  # caller put `r` at incoming-stack+0 and `color` at +8 (i64 widths).
  l32i.n  a8,  a1, 80                # r
  s32i.n  a8,  a1, 12
  l32i.n  a8,  a1, 88                # color
  s32i.n  a8,  a1, 16
  # f = 1 - r
  movi.n  a8, 1
  l32i.n  a9, a1, 12
  sub     a8, a8, a9
  s32i.n  a8, a1, 20
  # ddF_x = 1
  movi.n  a8, 1
  s32i.n  a8, a1, 24
  # ddF_y = -2*r
  l32i.n  a8, a1, 12
  slli    a8, a8, 1
  neg     a8, a8
  s32i.n  a8, a1, 28
  # x = 0; y = r
  movi.n  a8, 0
  s32i.n  a8, a1, 32
  l32i.n  a8, a1, 12
  s32i.n  a8, a1, 36
  # ---- Plot the 4 cardinal points first ----
  # (cx,   cy+r), (cx,   cy-r), (cx+r, cy), (cx-r, cy)
  call0   .Lcirc_plot_cardinals
.Lcirc_loop:
  # Loop while x < y
  l32i.n  a12, a1, 32                # x
  l32i.n  a13, a1, 36                # y
  bge     a12, a13, .Lcirc_done
  # if f >= 0: y--; ddF_y += 2; f += ddF_y
  l32i.n  a14, a1, 20                # f
  movi.n  a15, 0
  blt     a14, a15, .Lcirc_f_neg
  addi    a13, a13, -1
  s32i.n  a13, a1, 36
  l32i.n  a8, a1, 28
  addi    a8, a8, 2
  s32i.n  a8, a1, 28
  add     a14, a14, a8
  s32i.n  a14, a1, 20
.Lcirc_f_neg:
  # x++; ddF_x += 2; f += ddF_x
  addi    a12, a12, 1
  s32i.n  a12, a1, 32
  l32i.n  a8, a1, 24
  addi    a8, a8, 2
  s32i.n  a8, a1, 24
  l32i.n  a14, a1, 20
  add     a14, a14, a8
  s32i.n  a14, a1, 20
  # ---- Plot 8 octant pixels ----
  call0   .Lcirc_plot_octants
  j       .Lcirc_loop
.Lcirc_done:
  l32i    a15, a1, 72
  l32i    a14, a1, 68
  l32i    a13, a1, 64
  l32i    a12, a1, 60
  l32i.n  a0,  a1, 76
  addi    a1, a1, 80
  ret.n
  .size cssc_tft_circle, .-cssc_tft_circle

  # --- Helper: emit the 4 cardinal pixels for the current (cx,cy,r,color).
  #     Local function; uses a0 spill at frame-relative offset.
.Lcirc_plot_cardinals:
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  # (cx, cy+r)
  l32i.n  a2, a1, 16                # handle
  l32i.n  a4, a1, 20                # cx
  l32i.n  a6, a1, 24                # cy
  l32i.n  a8, a1, 28                # r
  add     a6, a6, a8
  l32i.n  a8, a1, 32                # color
  call0   cssc_tft_pixel
  # (cx, cy-r)
  l32i.n  a2, a1, 16
  l32i.n  a4, a1, 20
  l32i.n  a6, a1, 24
  l32i.n  a8, a1, 28
  sub     a6, a6, a8
  l32i.n  a8, a1, 32
  call0   cssc_tft_pixel
  # (cx+r, cy)
  l32i.n  a2, a1, 16
  l32i.n  a4, a1, 20
  l32i.n  a8, a1, 28
  add     a4, a4, a8
  l32i.n  a6, a1, 24
  l32i.n  a8, a1, 32
  call0   cssc_tft_pixel
  # (cx-r, cy)
  l32i.n  a2, a1, 16
  l32i.n  a4, a1, 20
  l32i.n  a8, a1, 28
  sub     a4, a4, a8
  l32i.n  a6, a1, 24
  l32i.n  a8, a1, 32
  call0   cssc_tft_pixel
  l32i.n  a0, a1, 12
  addi    a1, a1, 16
  ret.n
  .size .Lcirc_plot_cardinals, .-.Lcirc_plot_cardinals

  # --- Helper: emit 8 octant pixels for the current (x,y) state.
  #     Uses caller's frame layout (handle@16, cx@20, cy@24, color@32,
  #     x@48, y@52 — note frame offsets shifted by our +16 push).
.Lcirc_plot_octants:
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  # Macro shorthand: each plot loads handle/cx/cy/color, builds the
  # offset, and calls cssc_tft_pixel. (cx+x, cy+y)
  # ---- (cx+x, cy+y)
  l32i.n  a2, a1, 16
  l32i.n  a4, a1, 20
  l32i.n  a8, a1, 48
  add     a4, a4, a8
  l32i.n  a6, a1, 24
  l32i.n  a8, a1, 52
  add     a6, a6, a8
  l32i.n  a8, a1, 32
  call0   cssc_tft_pixel
  # (cx-x, cy+y)
  l32i.n  a2, a1, 16
  l32i.n  a4, a1, 20
  l32i.n  a8, a1, 48
  sub     a4, a4, a8
  l32i.n  a6, a1, 24
  l32i.n  a8, a1, 52
  add     a6, a6, a8
  l32i.n  a8, a1, 32
  call0   cssc_tft_pixel
  # (cx+x, cy-y)
  l32i.n  a2, a1, 16
  l32i.n  a4, a1, 20
  l32i.n  a8, a1, 48
  add     a4, a4, a8
  l32i.n  a6, a1, 24
  l32i.n  a8, a1, 52
  sub     a6, a6, a8
  l32i.n  a8, a1, 32
  call0   cssc_tft_pixel
  # (cx-x, cy-y)
  l32i.n  a2, a1, 16
  l32i.n  a4, a1, 20
  l32i.n  a8, a1, 48
  sub     a4, a4, a8
  l32i.n  a6, a1, 24
  l32i.n  a8, a1, 52
  sub     a6, a6, a8
  l32i.n  a8, a1, 32
  call0   cssc_tft_pixel
  # (cx+y, cy+x)
  l32i.n  a2, a1, 16
  l32i.n  a4, a1, 20
  l32i.n  a8, a1, 52
  add     a4, a4, a8
  l32i.n  a6, a1, 24
  l32i.n  a8, a1, 48
  add     a6, a6, a8
  l32i.n  a8, a1, 32
  call0   cssc_tft_pixel
  # (cx-y, cy+x)
  l32i.n  a2, a1, 16
  l32i.n  a4, a1, 20
  l32i.n  a8, a1, 52
  sub     a4, a4, a8
  l32i.n  a6, a1, 24
  l32i.n  a8, a1, 48
  add     a6, a6, a8
  l32i.n  a8, a1, 32
  call0   cssc_tft_pixel
  # (cx+y, cy-x)
  l32i.n  a2, a1, 16
  l32i.n  a4, a1, 20
  l32i.n  a8, a1, 52
  add     a4, a4, a8
  l32i.n  a6, a1, 24
  l32i.n  a8, a1, 48
  sub     a6, a6, a8
  l32i.n  a8, a1, 32
  call0   cssc_tft_pixel
  # (cx-y, cy-x)
  l32i.n  a2, a1, 16
  l32i.n  a4, a1, 20
  l32i.n  a8, a1, 52
  sub     a4, a4, a8
  l32i.n  a6, a1, 24
  l32i.n  a8, a1, 48
  sub     a6, a6, a8
  l32i.n  a8, a1, 32
  call0   cssc_tft_pixel
  l32i.n  a0, a1, 12
  addi    a1, a1, 16
  ret.n
  .size .Lcirc_plot_octants, .-.Lcirc_plot_octants

  .global cssc_oled_circle
.align  4
cssc_oled_circle:
j cssc_tft_circle

  # --- text — glyph-table lookup + per-pixel plot ---------------------
  .global cssc_tft_text
  .type   cssc_tft_text, @function
.align  4
cssc_tft_text:
  # Phase E1c — clean frame layout, no save-slot overlap.
  #
  # In:  handle=a2, x=a4 (i64 lo), y=a6 (i64 lo).
  #      stack-passed args (the rest):
  #        +80 → str ptr (cssc_str*)
  #        +88 → color (i64 lo)
  #
  # Frame (80 bytes, 16-aligned):
  #   [0]   handle
  #   [4]   x (start)
  #   [8]   y
  #   [12]  str ptr
  #   [16]  color
  #   [20]  col_cursor      (pixels from x; +6 per emitted glyph)
  #   [24]  size_remaining  (str bytes left to process)
  #   [28]  data_cursor     (next char to read)
  #   [32]  font_base       (.Lfont5x7_table address)
  #   [36]  glyph_base      (font_base + glyph_idx*5)
  #   [40]  col_within_glyph (0..4)
  #   [44]  col_bits        (current column's 7-bit pattern)
  #   [48]  row             (0..6)
  #   [60]  saved a12
  #   [64]  saved a13
  #   [68]  saved a14
  #   [72]  saved a15
  #   [76]  saved a0
  addi    a1, a1, -80
  s32i.n  a0,  a1, 76
  s32i    a12, a1, 60
  s32i    a13, a1, 64
  s32i    a14, a1, 68
  s32i    a15, a1, 72
  s32i.n  a2, a1, 0
  s32i.n  a4, a1, 4
  s32i.n  a6, a1, 8
  l32i.n  a8, a1, 80                # str ptr (stack arg)
  s32i.n  a8, a1, 12
  l32i.n  a8, a1, 88                # color
  s32i.n  a8, a1, 16
  movi.n  a8, 0
  s32i.n  a8, a1, 20                # col_cursor = 0
  l32i.n  a8, a1, 12                # str ptr
  l32i.n  a9, a8, 4                 # size (was +0 before A.3.4 refcount)
  l32i.n  a10, a8, 12               # data (was +8 before A.3.4 refcount)
  s32i.n  a9, a1, 24                # size_remaining
  s32i.n  a10, a1, 28                # data_cursor
  l32r    a8, .Ltxt_font_addr
  s32i.n  a8, a1, 32                # font_base
.Ltxt_loop:
  l32i.n  a9, a1, 24                # size_remaining
  movi.n  a10, 0
  beq     a9, a10, .Ltxt_done
  l32i.n  a11, a1, 28                # data_cursor
  l8ui    a8, a11, 0                # current char
  # Clamp to printable range 0x20..0x7F.
  movi.n  a9, 0x20
  blt     a8, a9, .Ltxt_skip
  movi.n  a9, 0x80
  bge     a8, a9, .Ltxt_skip
  # glyph_base = font_base + (char - 0x20) * 5
  addi    a8, a8, -0x20
  mov.n   a2, a8
  movi.n  a3, 5
  call0   __mulsi3
  l32i.n  a8, a1, 32
  add     a8, a8, a2
  s32i.n  a8, a1, 36                # glyph_base
  movi.n  a8, 0
  s32i.n  a8, a1, 40                # col_within_glyph = 0
.Ltxt_col:
  l32i.n  a9, a1, 40
  movi.n  a10, 5
  beq     a9, a10, .Ltxt_advance
  l32i.n  a11, a1, 36
  add     a8, a11, a9               # addr of this column's byte
  l8ui    a8, a8, 0                 # col_bits
  s32i.n  a8, a1, 44                # persist col_bits
  movi.n  a8, 0
  s32i.n  a8, a1, 48                # row = 0
.Ltxt_row:
  l32i.n  a8, a1, 48                # row
  movi.n  a9, 7
  beq     a8, a9, .Ltxt_col_next
  # bit = (col_bits >> row) & 1
  l32i.n  a11, a1, 44                # col_bits
  ssr     a8                         # SAR = row
  srl     a10, a11
  movi.n  a3, 1
  and     a10, a10, a3
  beqz    a10, .Ltxt_row_next
  # Plot pixel at (x + col_cursor + col_within_glyph, y + row, color).
  l32i.n  a2, a1, 0                 # handle
  l32i.n  a4, a1, 4                 # x
  l32i.n  a8, a1, 20                # col_cursor
  add     a4, a4, a8
  l32i.n  a8, a1, 40                # col_within_glyph
  add     a4, a4, a8
  l32i.n  a6, a1, 8                 # y
  l32i.n  a8, a1, 48                # row
  add     a6, a6, a8
  l32i.n  a8, a1, 16                # color
  call0   cssc_tft_pixel
.Ltxt_row_next:
  l32i.n  a8, a1, 48
  addi    a8, a8, 1
  s32i.n  a8, a1, 48
  j       .Ltxt_row
.Ltxt_col_next:
  l32i.n  a8, a1, 40
  addi    a8, a8, 1
  s32i.n  a8, a1, 40
  j       .Ltxt_col
.Ltxt_advance:
  # Move cursor right by 6 pixels (5 glyph cols + 1 spacing gap).
  l32i.n  a8, a1, 20
  addi    a8, a8, 6
  s32i.n  a8, a1, 20
.Ltxt_skip:
  l32i.n  a8, a1, 28                # data_cursor
  addi    a8, a8, 1
  s32i.n  a8, a1, 28
  l32i.n  a8, a1, 24                # size_remaining
  addi    a8, a8, -1
  s32i.n  a8, a1, 24
  j       .Ltxt_loop
.Ltxt_done:
  l32i    a15, a1, 72
  l32i    a14, a1, 68
  l32i    a13, a1, 64
  l32i    a12, a1, 60
  l32i.n  a0,  a1, 76
  addi    a1, a1, 80
  ret.n
  .literal_position
  .literal .Ltxt_font_addr, .Lfont5x7_table
  .size cssc_tft_text, .-cssc_tft_text

  .global cssc_oled_text
.align  4
cssc_oled_text:
j cssc_tft_text

  # --- show — bulk I2C transfer of framebuffer ------------------------
  .global cssc_tft_show
  .type   cssc_tft_show, @function
.align  4
cssc_tft_show:
  # in: a2 = handle
  # Send the entire framebuffer to the OLED in 16-byte chunks via
  # cssc_i2c_write. Each chunk: <addr> + <data prefix 0x40> + bytes.
  #
  # SSD1306 page-organized: 8 pages × width bytes per page.
  # We use cssc_i2c_write byte-by-byte (control byte 0x40 once per
  # page-start, then data bytes). This is slower than a true
  # START..STOP burst but is correct and matches the byte-oriented
  # cssc_i2c_write API.
  # CALL0 ABI: we clobber a12..a15 — must be preserved across the
  # call. a12=handle, a13=fb cursor, a14=fb remaining, a15=byte.
  # Frame: 48 bytes — [0..15] inline i2c handle, [28]=a0, [24]=a12,
  # [20]=a13, [16]=a14, [12]=a15... but [12] collides with the
  # inline-handle's half_bit slot. Push saved-CS up by 16 bytes to
  # give the inline handle its own region.
  addi    a1, a1, -48
  s32i.n  a0,  a1, 44
  s32i.n  a12, a1, 40
  s32i.n  a13, a1, 36
  s32i.n  a14, a1, 32
  s32i.n  a15, a1, 28
  mov.n   a12, a2                   # handle
  l32i.n  a13, a12, 24              # fb ptr
  l32i.n  a14, a12, 28              # fb size
  # Build inline I2C handle on frame for cssc_i2c_write_burst.
  l32i.n  a8, a12, 12               # sda
  l32i.n  a9, a12, 16               # scl
  s32i.n  a8, a1, 4
  s32i.n  a9, a1, 8
  movi    a8, 400
  s32i.n  a8, a1, 12                # half_bit cycles (100 kHz)
  movi.n  a8, 0
  s32i.n  a8, a1, 0
  # === STEP 1: set column + page address ranges ====================
  # In horizontal addressing mode the SSD1306 walks its internal
  # pointer across (col_start..col_end) then advances page until
  # (page_end), then wraps. We need to RESET this pointer before
  # each frame, otherwise pixels can land at undefined positions
  # (the legacy --gcc path's `_ssd_flush` does the same: it sends
  # 0x21,0x00,col_end then 0x22,0x00,page_end before the data).
  #
  # Bundle six command bytes [0x21, 0x00, col_end, 0x22, 0x00, page_end]
  # into a single cmd-stream transaction. We compose them on the
  # stack first, then call write_burst with the 0x00 control prefix.
  #
  # Stack scratch for the 6-byte command blob: place it at [a1+20..25]
  # inside our existing 48-byte frame. The inline handle uses [0..15],
  # saved CS regs use [28..44], so [20..27] is free.
  l32i.n  a8, a12, 4                # handle.width
  addi    a8, a8, -1                # col_end = width - 1
  movi.n  a9, 0x21
  s8i     a9, a1, 20                # cmd 0x21 (set col addr)
  movi.n  a9, 0x00
  s8i     a9, a1, 21                # col_start = 0
  s8i     a8, a1, 22                # col_end
  movi.n  a9, 0x22
  s8i     a9, a1, 23                # cmd 0x22 (set page addr)
  movi.n  a9, 0x00
  s8i     a9, a1, 24                # page_start = 0
  l32i.n  a8, a12, 8                # handle.height
  srli    a8, a8, 3                 # pages = height / 8
  addi    a8, a8, -1                # page_end = pages - 1
  s8i     a8, a1, 25                # page_end
  # Reserve 16 bytes for outgoing stack args.
  addi    a1, a1, -16
  addi    a8, a1, 16+20             # &cmd_blob (caller offset 20)
  s32i.n  a8, a1, 0                 # ptr lo
  movi.n  a8, 0
  s32i.n  a8, a1, 4                 # ptr hi
  movi.n  a8, 6
  s32i.n  a8, a1, 8                 # len lo = 6
  movi.n  a8, 0
  s32i.n  a8, a1, 12                # len hi
  addi    a2, a1, 16                # &inline_handle (parent +0)
  l32i.n  a4, a12, 20               # addr
  movi.n  a6, 0x00                  # CMD stream prefix
  call0   cssc_i2c_write_burst
  addi    a1, a1, 16                # pop stack args
  # === STEP 2: stream the framebuffer with the 0x40 data prefix ====
  addi    a1, a1, -16
  s32i.n  a13, a1, 0                # ptr lo (fb)
  movi.n  a8, 0
  s32i.n  a8, a1, 4                 # ptr hi
  s32i.n  a14, a1, 8                # len lo (fb_size)
  s32i.n  a8, a1, 12                # len hi
  addi    a2, a1, 16                # &inline_handle
  l32i.n  a4, a12, 20               # addr
  movi.n  a6, 0x40                  # DATA stream prefix
  call0   cssc_i2c_write_burst
  addi    a1, a1, 16
sh_done:
  l32i.n  a15, a1, 28
  l32i.n  a14, a1, 32
  l32i.n  a13, a1, 36
  l32i.n  a12, a1, 40
  l32i.n  a0,  a1, 44
  addi    a1, a1, 48
  ret.n
  .size cssc_tft_show, .-cssc_tft_show

  .global cssc_oled_show
.align  4
cssc_oled_show:
j cssc_tft_show

  # ===========================================================================
  # C1 — Vector runtime (vector_int, vector_float, array_int, array_float).
  #
  # Header layout (24 bytes — matches host runtime so v6 native scripts
  # can interoperate at the source level):
  #   offset 0:  i64 size      (number of elements currently in use)
  #   offset 8:  i64 capacity  (allocated slots)
  #   offset 16: ptr data      (heap pointer to size*8 bytes)
  #
  # Each element occupies 8 bytes regardless of kind (i64 or f64);
  # the runtime is shared between int and float — the calling code
  # passes the right value type and we store the 8 bytes raw.
  #
  # Arena-allocator constraint: we don't free individual buffers, so
  # push_back grows via "reallocate to 2x + copy" only when there's
  # room in the arena. Once arena is exhausted, push_back returns
  # silently (size stays). User can check via `.size()` and bail.
  # ===========================================================================

  .global cssc_vector_new_int
  .type   cssc_vector_new_int, @function
.align  4
cssc_vector_new_int:
  # in: a2 = capacity_hint (i64 lo)
  # out: a2 = pointer to header
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  s32i.n  a12, a1, 8
  s32i.n  a13, a1, 4
  # Clamp cap to a minimum of 4 so push_back's "double on grow" has
  # a sensible starting size.
  movi.n  a8, 4
  bge     a2, a8, vi_cap_ok
  movi.n  a2, 4
vi_cap_ok:
  mov.n   a12, a2                  # cap
  # Allocate header (24 bytes).
  movi.n  a2, 24
  call0   cssc_obj_alloc
  mov.n   a13, a2                  # header ptr
  # size = 0
  movi.n  a8, 0
  s32i.n  a8, a13, 0
  s32i.n  a8, a13, 4               # size_hi
  # cap
  s32i.n  a12, a13, 8
  s32i.n  a8, a13, 12              # cap_hi
  # data buffer: cap * 8 bytes
  slli    a2, a12, 3               # cap << 3
  call0   cssc_obj_alloc
  s32i.n  a2, a13, 16              # data ptr
  # return header
  mov.n   a2, a13
  l32i.n  a13, a1, 4
  l32i.n  a12, a1, 8
  l32i.n  a0,  a1, 12
  addi    a1, a1, 16
  ret.n
  .size cssc_vector_new_int, .-cssc_vector_new_int

  .global cssc_vector_new_float
.align  4
cssc_vector_new_float:
j cssc_vector_new_int   # identical 8-byte slots

  .global cssc_vector_free_int
.align  4
cssc_vector_free_int:
ret.n                    # arena allocator
  .global cssc_vector_free_float
.align  4
cssc_vector_free_float:
ret.n

  .global cssc_vector_size_int
  .type   cssc_vector_size_int, @function
.align  4
cssc_vector_size_int:
  # in: a2 = header ptr → out: a2:a3 = size (i64)
  l32i.n  a8, a2, 0
  l32i.n  a3, a2, 4
  mov.n   a2, a8
  ret.n
  .size cssc_vector_size_int, .-cssc_vector_size_int

  .global cssc_vector_size_float
.align  4
cssc_vector_size_float:
j cssc_vector_size_int

  .global cssc_vector_get_int
  .type   cssc_vector_get_int, @function
.align  4
cssc_vector_get_int:
  # in: a2 = header, a4 = idx (i64 lo) → out: a2:a3 = element (i64)
  l32i.n  a8, a2, 16               # data
  slli    a9, a4, 3                # idx*8
  add     a10, a8, a9              # element addr
  l32i.n  a2, a10, 0
  l32i.n  a3, a10, 4
  ret.n
  .size cssc_vector_get_int, .-cssc_vector_get_int

  .global cssc_vector_get_float
.align  4
cssc_vector_get_float:
j cssc_vector_get_int    # raw 8-byte read

  .global cssc_vector_set_int
  .type   cssc_vector_set_int, @function
.align  4
cssc_vector_set_int:
  # in: a2 = header, a4 = idx, a6:a7 = value
  l32i.n  a8, a2, 16
  slli    a9, a4, 3
  add     a10, a8, a9
  s32i.n  a6, a10, 0
  s32i.n  a7, a10, 4
  ret.n
  .size cssc_vector_set_int, .-cssc_vector_set_int

  .global cssc_vector_set_float
.align  4
cssc_vector_set_float:
j cssc_vector_set_int

  .global cssc_vector_push_back_int
  .type   cssc_vector_push_back_int, @function
.align  4
cssc_vector_push_back_int:
  # in: a2 = header, a4:a5 = value
  # If size < cap: write at data[size*8], size += 1.
  # Else: try to grow (reallocate 2x in arena + copy). On arena
  # exhaustion we leave size unchanged so the caller can detect via
  # `.size()`.
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  s32i.n  a12, a1, 8
  s32i.n  a13, a1, 4
  s32i.n  a14, a1, 0
  mov.n   a12, a2                  # header
  l32i.n  a13, a12, 0              # size
  l32i.n  a14, a12, 8              # cap
  bge     a13, a14, vpb_grow
vpb_store:
  l32i.n  a8, a12, 16              # data
  slli    a9, a13, 3
  add     a10, a8, a9
  s32i.n  a4, a10, 0
  s32i.n  a5, a10, 4
  addi    a13, a13, 1
  s32i.n  a13, a12, 0              # size++
  j       vpb_done
vpb_grow:
  # new_cap = cap * 2
  slli    a8, a14, 1               # new_cap
  s32i.n  a8, a12, 8               # cap = new_cap
  # Alloc new buffer (new_cap * 8 bytes).
  mov.n   a9, a8
  slli    a2, a9, 3
  call0   cssc_obj_alloc
  mov.n   a10, a2                  # new_data
  # Copy old data into new (size * 8 bytes).
  l32i.n  a8, a12, 16              # old_data
  slli    a11, a13, 3              # bytes_to_copy
  movi.n  a3, 0
vpb_copy:
  beq     a11, a3, vpb_copy_done
  l32i.n  a2, a8, 0
  s32i.n  a2, a10, 0
  addi    a8, a8, 4
  addi    a10, a10, 4
  addi    a11, a11, -4
  j       vpb_copy
vpb_copy_done:
  # Switch data ptr.
  s32i.n  a2, a12, 16              # wait — a2 was clobbered. Reload.
  # Bug-fix: we need the new_data ptr again. We overwrote a2; the
  # saved start lives at a10 + (size*8) backwards… cleaner: recompute
  # via a different register at the start of the grow loop.
  # (The Phase C1 baseline stores new_data via a2 right after alloc;
  # the copy loop has now ended, so we restart that store from
  # a10 - size*8.)
  l32i.n  a2, a12, 8               # cap (== new_cap)
  # Recompute new_data start: end pointer minus bytes_copied.
  # (Simpler: re-alloc would be wrong — keep the new_data we wrote
  #  via the copy. Recompute from a10 = end-of-copy.)
  slli    a11, a13, 3              # bytes_copied
  sub     a10, a10, a11            # rewound to start
  s32i.n  a10, a12, 16
  j       vpb_store
vpb_done:
  l32i.n  a14, a1, 0
  l32i.n  a13, a1, 4
  l32i.n  a12, a1, 8
  l32i.n  a0, a1, 12
  addi    a1, a1, 16
  ret.n
  .size cssc_vector_push_back_int, .-cssc_vector_push_back_int

  .global cssc_vector_push_back_float
.align  4
cssc_vector_push_back_float:
j cssc_vector_push_back_int

  .global cssc_vector_pop_back_int
  .type   cssc_vector_pop_back_int, @function
.align  4
cssc_vector_pop_back_int:
  # in: a2 = header → out: a2:a3 = popped element (i64) or 0 if empty
  l32i.n  a8, a2, 0
  movi.n  a9, 0
  beq     a8, a9, vpop_empty
  addi    a8, a8, -1
  s32i.n  a8, a2, 0                # size--
  l32i.n  a10, a2, 16
  slli    a11, a8, 3
  add     a10, a10, a11
  l32i.n  a3, a10, 4
  l32i.n  a2, a10, 0
  ret.n
vpop_empty:
  movi.n  a2, 0
  movi.n  a3, 0
  ret.n
  .size cssc_vector_pop_back_int, .-cssc_vector_pop_back_int

  .global cssc_vector_pop_back_float
.align  4
cssc_vector_pop_back_float:
j cssc_vector_pop_back_int

  .global cssc_vector_clear_int
  .type   cssc_vector_clear_int, @function
.align  4
cssc_vector_clear_int:
  # in: a2 = header. Just zero the size field — data buffer kept.
  movi.n  a8, 0
  s32i.n  a8, a2, 0
  s32i.n  a8, a2, 4
  ret.n
  .size cssc_vector_clear_int, .-cssc_vector_clear_int

  .global cssc_vector_clear_float
.align  4
cssc_vector_clear_float:
j cssc_vector_clear_int

  # ===========================================================================
  # C2 — map_si (string→int hash table) + bind_ss (ordered string-pair list).
  #
  # map_si layout (32 bytes):
  #   off 0:  i64 size       (entries in use)
  #   off 8:  i64 bucket_cnt (power-of-2 number of buckets)
  #   off 16: ptr buckets    (bucket_cnt * 16 bytes each: key_ptr, value_lo)
  #   off 24: i64 _padding   (so header is 32 bytes total)
  #
  # Each bucket: 16 bytes — [ptr key | i64 value]. Empty buckets have
  # key_ptr = NULL. Probe sequence is linear; we accept the simpler
  # clustering trade-off versus the small bucket count we need.
  #
  # FNV-1a 32-bit hash:
  #   hash = 0x811C9DC5
  #   for each byte b: hash = (hash XOR b) * 0x01000193
  # ===========================================================================

  .global cssc_map_si_new
  .type   cssc_map_si_new, @function
.align  4
cssc_map_si_new:
  # in: a2 = capacity_hint (assumed power-of-2, min 8)
  # out: a2 = header ptr (single 32-byte alloc, no leak)
  #
  # A.1.3: the original code allocated the header twice — the first
  # 32-byte block got clobbered before the bucket buffer alloc finished,
  # so every map_si_new permanently leaked 32 bytes from the arena.
  # Fixed by stashing the header pointer in a13 (callee-save, preserved
  # across cssc_obj_alloc) for the duration of the bucket-buffer alloc.
  addi    a1, a1, -16
  s32i.n  a0,  a1, 12
  s32i.n  a12, a1, 8
  s32i.n  a13, a1, 4
  # Floor cap at 8.
  movi.n  a8, 8
  bge     a2, a8, ms_cap_ok
  movi.n  a2, 8
ms_cap_ok:
  mov.n   a12, a2                  # a12 = bucket_cnt (callee-save)
  # Allocate header (32 bytes), keep ptr in a13.
  movi.n  a2, 32
  call0   cssc_obj_alloc
  mov.n   a13, a2                  # a13 = header ptr (callee-save)
  # Allocate bucket buffer (cnt * 16 bytes); a2 returns ptr.
  slli    a2, a12, 4
  call0   cssc_obj_alloc
  # Now write the header fields once. a13 is still the same header.
  movi.n  a9, 0
  s32i.n  a9,  a13, 0              # size = 0
  s32i.n  a9,  a13, 4
  s32i.n  a12, a13, 8              # bucket_cnt
  s32i.n  a9,  a13, 12
  s32i.n  a2,  a13, 16             # bucket ptr
  s32i.n  a9,  a13, 20
  # Return the (only) header.
  mov.n   a2, a13
  l32i.n  a13, a1, 4
  l32i.n  a12, a1, 8
  l32i.n  a0,  a1, 12
  addi    a1, a1, 16
  ret.n
  .size cssc_map_si_new, .-cssc_map_si_new

  .global cssc_map_si_free
.align  4
cssc_map_si_free:
ret.n

  .global cssc_map_si_size
  .type   cssc_map_si_size, @function
.align  4
cssc_map_si_size:
  l32i.n  a8, a2, 0
  l32i.n  a3, a2, 4
  mov.n   a2, a8
  ret.n
  .size cssc_map_si_size, .-cssc_map_si_size

  # Internal: FNV-1a hash of a cssc_str.
  #   in: a2 = cssc_str* key
  #   out: a2 = 32-bit hash
.global ._cssc_fnv1a
.type ._cssc_fnv1a, @function
.align  4
._cssc_fnv1a:
  # CALL0 ABI: we use a12 (loop terminator constant) and a13 (current
  # byte) as scratch. Both are callee-save; the prior frame saved a0
  # only and leaked corruption back to callers (cssc_map_si_set/get/has).
  addi    a1, a1, -16
  s32i.n  a0,  a1, 12
  s32i.n  a12, a1, 8
  s32i.n  a13, a1, 4
  # size
  l32i.n  a8, a2, 0                # size
  # data
  l32i.n  a9, a2, 8                # data ptr
  # hash init = 0x811C9DC5
  l32r    a10, .Lfnv_offset_basis
  l32r    a11, .Lfnv_prime
fnv_loop:
  movi.n  a12, 0
  beq     a8, a12, fnv_done
  l8ui    a13, a9, 0
  xor     a10, a10, a13
  # hash *= prime
  mov.n   a2, a10
  mov.n   a3, a11
  call0   __mulsi3
  mov.n   a10, a2
  addi    a9, a9, 1
  addi    a8, a8, -1
  j       fnv_loop
fnv_done:
  mov.n   a2, a10
  l32i.n  a13, a1, 4
  l32i.n  a12, a1, 8
  l32i.n  a0,  a1, 12
  addi    a1, a1, 16
  ret.n
  .literal_position
  .literal .Lfnv_offset_basis, 0x811C9DC5
  .literal .Lfnv_prime,        0x01000193
  .size ._cssc_fnv1a, .-._cssc_fnv1a

  .global cssc_map_si_set
  .type   cssc_map_si_set, @function
.align  4
cssc_map_si_set:
  # in: a2 = header, a4 = key cssc_str*, a6:a7 = value (i64)
  # Hash, linear-probe to find an empty bucket OR a matching key,
  # install. Resize is NOT implemented in Phase C2 — caller must
  # size the map appropriately.
  addi    a1, a1, -32
  s32i.n  a0, a1, 28
  s32i.n  a12, a1, 24
  s32i.n  a13, a1, 20
  s32i.n  a14, a1, 16
  s32i.n  a15, a1, 12
  mov.n   a12, a2                  # header
  mov.n   a13, a4                  # key
  # Save value temporarily on stack (we'll need to clobber a6/a7).
  s32i.n  a6, a1, 0
  s32i.n  a7, a1, 4
  # hash
  mov.n   a2, a13
  call0   ._cssc_fnv1a             # a2 = hash
  mov.n   a14, a2                  # hash
  l32i.n  a15, a12, 8              # bucket_cnt
  addi    a8, a15, -1              # mask = cnt-1
  and     a14, a14, a8             # first slot
  l32i.n  a9, a12, 16              # buckets ptr
  # iterate
  mov.n   a10, a14                 # probe idx
ms_probe:
  slli    a11, a10, 4              # bucket offset
  add     a2, a9, a11              # bucket addr
  l32i.n  a3, a2, 0                # key_ptr
  movi.n  a4, 0
  beq     a3, a4, ms_install       # empty slot → install
  # If keys equal byte-by-byte, replace value.
  mov.n   a4, a3                   # existing key
  mov.n   a3, a13                  # our key
  # cssc_string_eq(a, b) returns 0/1 in a2.
  s32i.n  a2, a1, 8                # preserve bucket addr
  mov.n   a2, a4
  mov.n   a3, a13
  call0   cssc_string_eq
  movi.n  a5, 0
  beq     a2, a5, ms_advance
  # equal → reuse bucket
  l32i.n  a2, a1, 8                # bucket addr back
  j       ms_write_value
ms_advance:
  l32i.n  a2, a1, 8
  addi    a10, a10, 1
  # modulo cnt: AND with mask. cnt is power-of-2 so this works.
  l32i.n  a8, a12, 8
  addi    a8, a8, -1
  and     a10, a10, a8
  bne     a10, a14, ms_probe       # full pass without empty → full map
  # Map full — drop silently (caller can pre-size). On embedded
  # this is preferable to a panic.
  j       ms_done
ms_install:
  # bucket at a2; install key + value.
  s32i.n  a13, a2, 0               # key
  # increment size
  l32i.n  a8, a12, 0
  addi    a8, a8, 1
  s32i.n  a8, a12, 0
ms_write_value:
  # value bytes from frame.
  l32i.n  a8, a1, 0
  s32i.n  a8, a2, 8
  l32i.n  a8, a1, 4
  s32i.n  a8, a2, 12
ms_done:
  l32i.n  a15, a1, 12
  l32i.n  a14, a1, 16
  l32i.n  a13, a1, 20
  l32i.n  a12, a1, 24
  l32i.n  a0, a1, 28
  addi    a1, a1, 32
  ret.n
  .size cssc_map_si_set, .-cssc_map_si_set

  .global cssc_map_si_get
  .type   cssc_map_si_get, @function
.align  4
cssc_map_si_get:
  # in: a2 = header, a4 = key → out: a2:a3 = value (0 if not found)
  # CALL0 ABI: we clobber a12/a13/a14. Bigger frame so the
  # per-call scratch slots ([+0] saved idx, [+4] saved bucket
  # addr during cssc_string_eq) don't overlap the callee-save
  # spill slots.
  addi    a1, a1, -32
  s32i.n  a0,  a1, 28
  s32i.n  a12, a1, 24
  s32i.n  a13, a1, 20
  s32i.n  a14, a1, 16
  mov.n   a12, a2
  mov.n   a13, a4
  mov.n   a2, a13
  call0   ._cssc_fnv1a
  l32i.n  a8, a12, 8
  addi    a9, a8, -1
  and     a2, a2, a9               # idx
  mov.n   a14, a2                  # first idx
  l32i.n  a10, a12, 16             # buckets
mg_probe:
  slli    a11, a2, 4
  add     a3, a10, a11
  l32i.n  a4, a3, 0                # key_ptr
  movi.n  a5, 0
  beq     a4, a5, mg_miss
  # cssc_string_eq
  s32i.n  a2, a1, 0
  s32i.n  a3, a1, 4                # bucket addr
  mov.n   a2, a4
  mov.n   a3, a13
  call0   cssc_string_eq
  movi.n  a5, 0
  beq     a2, a5, mg_next
  l32i.n  a3, a1, 4
  l32i.n  a2, a3, 8                # value lo
  l32i.n  a3, a3, 12               # value hi
  l32i.n  a14, a1, 16
  l32i.n  a13, a1, 20
  l32i.n  a12, a1, 24
  l32i.n  a0,  a1, 28
  addi    a1, a1, 32
  ret.n
mg_next:
  l32i.n  a2, a1, 0
  addi    a2, a2, 1
  l32i.n  a8, a12, 8
  addi    a8, a8, -1
  and     a2, a2, a8
  bne     a2, a14, mg_probe
mg_miss:
  movi.n  a2, 0
  movi.n  a3, 0
  l32i.n  a14, a1, 16
  l32i.n  a13, a1, 20
  l32i.n  a12, a1, 24
  l32i.n  a0,  a1, 28
  addi    a1, a1, 32
  ret.n
  .size cssc_map_si_get, .-cssc_map_si_get

  .global cssc_map_si_has
  .type   cssc_map_si_has, @function
.align  4
cssc_map_si_has:
  # in: a2 = header, a4 = key → out: a2 = i1 (1 if present)
  # Reuses cssc_map_si_get and checks bucket presence rather than
  # value (handles the "value happens to be 0" case correctly).
  # CALL0 ABI: we clobber a12/a13/a14, so we need an extended frame
  # that saves all three plus the scratch slot for the idx
  # preserved across the cssc_string_eq call.
  addi    a1, a1, -32
  s32i.n  a0,  a1, 28
  s32i.n  a12, a1, 24
  s32i.n  a13, a1, 20
  s32i.n  a14, a1, 16
  mov.n   a12, a2
  mov.n   a13, a4
  mov.n   a2, a13
  call0   ._cssc_fnv1a
  l32i.n  a8, a12, 8
  addi    a9, a8, -1
  and     a2, a2, a9
  mov.n   a14, a2
  l32i.n  a10, a12, 16
mh_probe:
  slli    a11, a2, 4
  add     a3, a10, a11
  l32i.n  a4, a3, 0
  movi.n  a5, 0
  beq     a4, a5, mh_miss
  s32i.n  a2, a1, 0
  mov.n   a2, a4
  mov.n   a3, a13
  call0   cssc_string_eq
  movi.n  a5, 0
  beq     a2, a5, mh_next
  movi.n  a2, 1
  movi.n  a3, 0
  l32i.n  a14, a1, 16
  l32i.n  a13, a1, 20
  l32i.n  a12, a1, 24
  l32i.n  a0,  a1, 28
  addi    a1, a1, 32
  ret.n
mh_next:
  l32i.n  a2, a1, 0
  addi    a2, a2, 1
  l32i.n  a8, a12, 8
  addi    a8, a8, -1
  and     a2, a2, a8
  bne     a2, a14, mh_probe
mh_miss:
  movi.n  a2, 0
  movi.n  a3, 0
  l32i.n  a14, a1, 16
  l32i.n  a13, a1, 20
  l32i.n  a12, a1, 24
  l32i.n  a0,  a1, 28
  addi    a1, a1, 32
  ret.n
  .size cssc_map_si_has, .-cssc_map_si_has

  # ===========================================================================
  # bind_ss — ordered (key, value) string-pair list. Simpler than map:
  # linear probe over an array of pair entries, retains insertion order.
  # Layout (24 bytes):
  #   off 0:  i64 size
  #   off 8:  i64 cap
  #   off 16: ptr entries (each entry: 8-byte key_ptr + 8-byte val_ptr)
  # ===========================================================================

  .global cssc_bind_ss_new
  .type   cssc_bind_ss_new, @function
.align  4
cssc_bind_ss_new:
  addi    a1, a1, -16
  s32i.n  a0, a1, 12
  s32i.n  a12, a1, 8
  movi.n  a8, 4
  bge     a2, a8, bs_cap_ok
  movi.n  a2, 4
bs_cap_ok:
  mov.n   a12, a2
  movi.n  a2, 24
  call0   cssc_obj_alloc
  movi.n  a8, 0
  s32i.n  a8, a2, 0
  s32i.n  a8, a2, 4
  s32i.n  a12, a2, 8
  s32i.n  a8, a2, 12
  # entries buffer (cap * 16 bytes)
  slli    a3, a12, 4
  s32i.n  a2, a1, 0                # save header
  mov.n   a2, a3
  call0   cssc_obj_alloc
  mov.n   a8, a2                   # entries ptr
  l32i.n  a2, a1, 0                # header
  s32i.n  a8, a2, 16
  l32i.n  a12, a1, 8
  l32i.n  a0,  a1, 12
  addi    a1, a1, 16
  ret.n
  .size cssc_bind_ss_new, .-cssc_bind_ss_new

  .global cssc_bind_ss_free
.align  4
cssc_bind_ss_free:
ret.n

  .global cssc_bind_ss_size
.align  4
cssc_bind_ss_size:
  l32i.n  a8, a2, 0
  l32i.n  a3, a2, 4
  mov.n   a2, a8
  ret.n

  .global cssc_bind_ss_push_back
  .type   cssc_bind_ss_push_back, @function
.align  4
cssc_bind_ss_push_back:
  # in: a2 = header, a4 = key, a6 = value (both cssc_str*)
  l32i.n  a8, a2, 0                # size
  l32i.n  a9, a2, 8                # cap
  bge     a8, a9, bp_full
  l32i.n  a10, a2, 16              # entries
  slli    a11, a8, 4               # size * 16
  add     a11, a10, a11
  s32i.n  a4, a11, 0
  s32i.n  a6, a11, 8
  addi    a8, a8, 1
  s32i.n  a8, a2, 0
bp_full:
  ret.n
  .size cssc_bind_ss_push_back, .-cssc_bind_ss_push_back

  .global cssc_pin_pullup
  .type   cssc_pin_pullup, @function
.align  4
cssc_pin_pullup:
  # in: a2 = handle, a3 = enable (0/1)
  # Set/clear bit 7 of the IOMUX register for this pin.
  # CALL0 ABI: we clobber a12 and a13, so we have to save/restore
  # them. (a8..a11 are caller-save scratch and need no preserve.)
  addi    a1, a1, -16
  s32i.n  a12, a1, 0
  s32i.n  a13, a1, 4
  l32i.n  a8, a2, 0              # pin_no
  l32r    a9, .Lpinpu_iomux_addr
  slli    a10, a8, 2
  add     a9, a9, a10
  l32i.n  a11, a9, 0             # current IOMUX value
  # Pullup bit is bit 7.
  movi.n  a12, 0x80
  movi.n  a13, 0
  beq     a3, a13, pin_pu_off
  or      a11, a11, a12
  j       pin_pu_store
pin_pu_off:
  # mask = ~0x80 = 0xFFFFFF7F
  l32r    a13, .Lpinpu_mask_addr
  l32i.n  a13, a13, 0
  and     a11, a11, a13
pin_pu_store:
  s32i.n  a11, a9, 0
  l32i.n  a13, a1, 4
  l32i.n  a12, a1, 0
  addi    a1, a1, 16
  ret.n
  .literal_position
  .literal .Lpinpu_iomux_addr, .Liomux_table
  .literal .Lpinpu_mask_addr,  .Lpinpu_mask_val
  .section .rodata
  .align 4
.Lpinpu_mask_val:
  .word 0xFFFFFF7F
  .text
  .size cssc_pin_pullup, .-cssc_pin_pullup

