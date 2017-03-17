; ModuleID = 'public_samples/myGvar11.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-redhat-linux-gnu"

define i32 @foo(float %x) {
entry:
  %arg0 = alloca float
  %y = alloca i32
  %z = alloca i1
  br label %next

next:                                             ; preds = %entry
  store float %x, float* %arg0
  %0 = load i32* %y
  %1 = add i32 %0, 1
  store i32 %1, i32* %y
  store i1 true, i1* %z
  %2 = load i32* %y
  ret i32 %2
}
