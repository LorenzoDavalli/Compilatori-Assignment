define dso_local i32 @foo(i32 noundef %0, i32 noundef %1) #0 {
  %3 = sub nsw i32 %1, 1
  %4 = sub nsw i32 %3, 1
  %5 = add nsw i32 %4, 2
  ret i32 %5
}
