; ModuleID = 'NegativeDistance.m2r.bc'
source_filename = "NegativeDistance.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: mustprogress noinline nounwind uwtable
define dso_local void @_Z3funv() #0 {
  %1 = alloca [20 x i32], align 16
  br label %2

2:                                                ; preds = %7, %0
  %.0 = phi i32 [ 0, %0 ], [ %8, %7 ]
  %3 = icmp slt i32 %.0, 15
  br i1 %3, label %4, label %9

4:                                                ; preds = %2
  %5 = sext i32 %.0 to i64
  %6 = getelementptr inbounds [20 x i32], ptr %1, i64 0, i64 %5
  store i32 1, ptr %6, align 4
  br label %7

7:                                                ; preds = %4
  %8 = add nsw i32 %.0, 1
  br label %2, !llvm.loop !6

9:                                                ; preds = %2
  br label %10

10:                                               ; preds = %19, %9
  %.01 = phi i32 [ 0, %9 ], [ %20, %19 ]
  %11 = icmp slt i32 %.01, 15
  br i1 %11, label %12, label %21

12:                                               ; preds = %10
  %13 = add nsw i32 %.01, 5
  %14 = sext i32 %13 to i64
  %15 = getelementptr inbounds [20 x i32], ptr %1, i64 0, i64 %14
  %16 = load i32, ptr %15, align 4
  %17 = sext i32 %.01 to i64
  %18 = getelementptr inbounds [20 x i32], ptr %1, i64 0, i64 %17
  store i32 %16, ptr %18, align 4
  br label %19

19:                                               ; preds = %12
  %20 = add nsw i32 %.01, 1
  br label %10, !llvm.loop !8

21:                                               ; preds = %10
  ret void
}

attributes #0 = { mustprogress noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Debian clang version 19.1.7 (++20250114103228+cd708029e0b2-1~exp1~20250114103334.78)"}
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
!8 = distinct !{!8, !7}
