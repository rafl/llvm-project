; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py UTC_ARGS: --version 5
; RUN: llc --mtriple=loongarch64 --mattr=+lsx < %s | FileCheck %s

define <4 x i32> @xor_shl_splat_vec_one(i32 %x, <4 x i32> %y) nounwind {
; CHECK-LABEL: xor_shl_splat_vec_one:
; CHECK:       # %bb.0: # %entry
; CHECK-NEXT:    vreplgr2vr.w $vr1, $a0
; CHECK-NEXT:    vsll.w $vr0, $vr1, $vr0
; CHECK-NEXT:    vbitrevi.w $vr0, $vr0, 0
; CHECK-NEXT:    ret
entry:
  %ins = insertelement <4 x i32> poison, i32 %x, i64 0
  %splat = shufflevector <4 x i32> %ins, <4 x i32> poison, <4 x i32> zeroinitializer
  %shl = shl <4 x i32> %splat, %y
  %xor = xor <4 x i32> %shl, splat (i32 1)
  ret <4 x i32> %xor
}
