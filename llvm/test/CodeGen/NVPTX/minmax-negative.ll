; RUN: llc < %s -mtriple=nvptx64 -O0 | FileCheck %s
; RUN: %if ptxas %{ llc < %s -mtriple=nvptx64 -O0 | %ptxas-verify %}

define i16 @test1(ptr %sur1) {
; CHECK-NOT: mov.u16 %rs{{[0-9]+}}, 32767
  %_tmp21.i = icmp sle i16 0, 0
  %_tmp22.i = select i1 %_tmp21.i, i16 0, i16 32767
  store i16 %_tmp22.i, ptr %sur1
  ret i16 0
}
