; ModuleID = 'loopSequence.m2r.bc'
source_filename = "loopSequence.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @example(i32 noundef %0, i32 noundef %1, i32 noundef %2) #0 {
  br label %4

4:                                                ; preds = %16, %3
  %.01 = phi i32 [ 0, %3 ], [ %13, %16 ]
  %.0 = phi i32 [ undef, %3 ], [ %5, %16 ]
  %5 = add nsw i32 %.0, 1
  %6 = mul nsw i32 %1, %2
  %7 = add nsw i32 %1, 10
  %8 = mul nsw i32 %5, 2
  %9 = add nsw i32 %.01, %5
  %10 = add nsw i32 %6, %7
  %11 = add nsw i32 %10, %8
  %12 = add nsw i32 %11, %9
  %13 = add nsw i32 %.01, %12
  %14 = icmp eq i32 %5, 100
  br i1 %14, label %15, label %16

15:                                               ; preds = %4
  br label %17

16:                                               ; preds = %4
  br label %4

17:                                               ; preds = %15
  br label %18

18:                                               ; preds = %30, %17
  %.12 = phi i32 [ %13, %17 ], [ %27, %30 ]
  %.1 = phi i32 [ %5, %17 ], [ %19, %30 ]
  %19 = add nsw i32 %.1, 1
  %20 = mul nsw i32 %1, %2
  %21 = add nsw i32 %1, 10
  %22 = mul nsw i32 %19, 2
  %23 = add nsw i32 %.12, %19
  %24 = add nsw i32 %20, %21
  %25 = add nsw i32 %24, %22
  %26 = add nsw i32 %25, %23
  %27 = add nsw i32 %.12, %26
  %28 = icmp eq i32 %19, 100
  br i1 %28, label %29, label %30

29:                                               ; preds = %18
  br label %31

30:                                               ; preds = %18
  br label %18

31:                                               ; preds = %29
  ret i32 %27
}

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Debian clang version 19.1.7 (++20250114103228+cd708029e0b2-1~exp1~20250114103334.78)"}
