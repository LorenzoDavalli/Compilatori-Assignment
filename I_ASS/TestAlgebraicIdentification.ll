define dso_local i32 @foo(i32 noundef %0, i32 noundef %1) #0 {
  %3 = mul nsw i32 %1, 1
  %4 = mul nsw i32 1, %1
  %5 = add nsw i32 %3, 0
  %6 = add nsw i32 0, %3
  %7 = mul nsw i32 %4, %6
  %8 = add nsw i32 4, %3
  ret i32 %7
}
