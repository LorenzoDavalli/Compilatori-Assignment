define dso_local i32 @foo(i32 noundef %0, i32 noundef %1) #0 {
  %3 = add nsw i32 %1, 1
  %4 = mul nsw i32 %3, 4
  %5 = shl i32 %0, 1
  %6 = sdiv i32 %5, 4
  %7 = mul nsw i32 %4, %6
  %8 = mul nsw i32 4, %3
  %9 = mul nsw i32 %3, 7
  %10 = mul nsw i32 %3, 9
  %11 = mul nsw i32 7, %3
  %12 = mul nsw i32 9, %3
  ret i32 %7
}
