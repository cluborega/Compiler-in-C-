; ModuleID = 'public_samples/myGvar.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-redhat-linux-gnu"

@t = external global i32

define i32 @foo(i32 %x) {
entry:
  %x1 = alloca i32
  br label %next

next:                                             ; preds = %entry
  store i32 %x, i32* %x1
  %t = load i32* @t
  %0 = add i32 %t, 1
  store i32 %0, i32* @t
  %t2 = load i32* @t
  %1 = add i32 %t2, 1
  %t3 = load i32* @t
  ret i32 %t3
}
