; NOTE: Assertions have been autogenerated by utils/update_test_checks.py UTC_ARGS: --version 4
; RUN: opt < %s -S -passes=instcombine | FileCheck %s

declare void @use64(i64)
declare void @useptr(ptr)

define ptr @test_zero(ptr %base, i64 %a) {
; CHECK-LABEL: define ptr @test_zero(
; CHECK-SAME: ptr [[BASE:%.*]], i64 [[A:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[P2:%.*]] = getelementptr i32, ptr [[BASE]], i64 [[A]]
; CHECK-NEXT:    ret ptr [[P2]]
;
entry:
  %p1 = getelementptr i8, ptr %base, i64 -4
  %index = add i64 %a, 1
  %p2 = getelementptr i32, ptr %p1, i64 %index
  ret ptr %p2
}

define ptr @test_nonzero(ptr %base, i64 %a) {
; CHECK-LABEL: define ptr @test_nonzero(
; CHECK-SAME: ptr [[BASE:%.*]], i64 [[A:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = getelementptr i8, ptr [[BASE]], i64 4
; CHECK-NEXT:    [[P2:%.*]] = getelementptr i32, ptr [[TMP0]], i64 [[A]]
; CHECK-NEXT:    ret ptr [[P2]]
;
entry:
  %p1 = getelementptr i8, ptr %base, i64 -4
  %index = add i64 %a, 2
  %p2 = getelementptr i32, ptr %p1, i64 %index
  ret ptr %p2
}

define ptr @test_or_disjoint(ptr %base, i64 %a) {
; CHECK-LABEL: define ptr @test_or_disjoint(
; CHECK-SAME: ptr [[BASE:%.*]], i64 [[A:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[P2:%.*]] = getelementptr i32, ptr [[BASE]], i64 [[A]]
; CHECK-NEXT:    ret ptr [[P2]]
;
entry:
  %p1 = getelementptr i8, ptr %base, i64 -4
  %index = or disjoint i64 %a, 1
  %p2 = getelementptr i32, ptr %p1, i64 %index
  ret ptr %p2
}

define ptr @test_zero_multiuse_index(ptr %base, i64 %a) {
; CHECK-LABEL: define ptr @test_zero_multiuse_index(
; CHECK-SAME: ptr [[BASE:%.*]], i64 [[A:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[INDEX:%.*]] = add i64 [[A]], 1
; CHECK-NEXT:    call void @use64(i64 [[INDEX]])
; CHECK-NEXT:    [[P2:%.*]] = getelementptr i32, ptr [[BASE]], i64 [[A]]
; CHECK-NEXT:    ret ptr [[P2]]
;
entry:
  %p1 = getelementptr i8, ptr %base, i64 -4
  %index = add i64 %a, 1
  call void @use64(i64 %index)
  %p2 = getelementptr i32, ptr %p1, i64 %index
  ret ptr %p2
}

define ptr @test_zero_multiuse_ptr(ptr %base, i64 %a) {
; CHECK-LABEL: define ptr @test_zero_multiuse_ptr(
; CHECK-SAME: ptr [[BASE:%.*]], i64 [[A:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[P1:%.*]] = getelementptr i8, ptr [[BASE]], i64 -4
; CHECK-NEXT:    call void @useptr(ptr [[P1]])
; CHECK-NEXT:    [[P2:%.*]] = getelementptr i32, ptr [[BASE]], i64 [[A]]
; CHECK-NEXT:    ret ptr [[P2]]
;
entry:
  %p1 = getelementptr i8, ptr %base, i64 -4
  call void @useptr(ptr %p1)
  %index = add i64 %a, 1
  %p2 = getelementptr i32, ptr %p1, i64 %index
  ret ptr %p2
}

define ptr @test_zero_sext_add_nsw(ptr %base, i32 %a) {
; CHECK-LABEL: define ptr @test_zero_sext_add_nsw(
; CHECK-SAME: ptr [[BASE:%.*]], i32 [[A:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[P1:%.*]] = getelementptr i8, ptr [[BASE]], i64 -4
; CHECK-NEXT:    [[TMP0:%.*]] = sext i32 [[A]] to i64
; CHECK-NEXT:    [[TMP1:%.*]] = getelementptr i32, ptr [[P1]], i64 [[TMP0]]
; CHECK-NEXT:    [[P2:%.*]] = getelementptr i8, ptr [[TMP1]], i64 4
; CHECK-NEXT:    ret ptr [[P2]]
;
entry:
  %p1 = getelementptr i8, ptr %base, i64 -4
  %index = add nsw i32 %a, 1
  %p2 = getelementptr i32, ptr %p1, i32 %index
  ret ptr %p2
}

define ptr @test_zero_trunc_add(ptr %base, i128 %a) {
; CHECK-LABEL: define ptr @test_zero_trunc_add(
; CHECK-SAME: ptr [[BASE:%.*]], i128 [[A:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = trunc i128 [[A]] to i64
; CHECK-NEXT:    [[P2:%.*]] = getelementptr i32, ptr [[BASE]], i64 [[TMP0]]
; CHECK-NEXT:    ret ptr [[P2]]
;
entry:
  %p1 = getelementptr i8, ptr %base, i64 -4
  %index = add i128 %a, 1
  %p2 = getelementptr i32, ptr %p1, i128 %index
  ret ptr %p2
}

define ptr @test_non_i8(ptr %base, i64 %a) {
; CHECK-LABEL: define ptr @test_non_i8(
; CHECK-SAME: ptr [[BASE:%.*]], i64 [[A:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[P1:%.*]] = getelementptr i8, ptr [[BASE]], i64 -4
; CHECK-NEXT:    [[TMP0:%.*]] = getelementptr i32, ptr [[P1]], i64 [[A]]
; CHECK-NEXT:    ret ptr [[TMP0]]
;
entry:
  %p1 = getelementptr i16, ptr %base, i64 -4
  %index = add i64 %a, 1
  %p2 = getelementptr i32, ptr %p1, i64 %index
  ret ptr %p2
}

define ptr @test_non_const(ptr %base, i64 %a, i64 %b) {
; CHECK-LABEL: define ptr @test_non_const(
; CHECK-SAME: ptr [[BASE:%.*]], i64 [[A:%.*]], i64 [[B:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[P1:%.*]] = getelementptr i8, ptr [[BASE]], i64 [[B]]
; CHECK-NEXT:    [[TMP0:%.*]] = getelementptr i32, ptr [[P1]], i64 [[A]]
; CHECK-NEXT:    [[P2:%.*]] = getelementptr i8, ptr [[TMP0]], i64 4
; CHECK-NEXT:    ret ptr [[P2]]
;
entry:
  %p1 = getelementptr i8, ptr %base, i64 %b
  %index = add i64 %a, 1
  %p2 = getelementptr i32, ptr %p1, i64 %index
  ret ptr %p2
}

define ptr @test_too_many_indices(ptr %base, i64 %a, i64 %b) {
; CHECK-LABEL: define ptr @test_too_many_indices(
; CHECK-SAME: ptr [[BASE:%.*]], i64 [[A:%.*]], i64 [[B:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[P1:%.*]] = getelementptr i8, ptr [[BASE]], i64 [[B]]
; CHECK-NEXT:    [[INDEX:%.*]] = add i64 [[A]], 1
; CHECK-NEXT:    [[P2:%.*]] = getelementptr [8 x i32], ptr [[P1]], i64 1, i64 [[INDEX]]
; CHECK-NEXT:    ret ptr [[P2]]
;
entry:
  %p1 = getelementptr i8, ptr %base, i64 %b
  %index = add i64 %a, 1
  %p2 = getelementptr [8 x i32], ptr %p1, i64 1, i64 %index
  ret ptr %p2
}

define ptr @test_wrong_op(ptr %base, i64 %a) {
; CHECK-LABEL: define ptr @test_wrong_op(
; CHECK-SAME: ptr [[BASE:%.*]], i64 [[A:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[P1:%.*]] = getelementptr i8, ptr [[BASE]], i64 -4
; CHECK-NEXT:    [[INDEX:%.*]] = xor i64 [[A]], 1
; CHECK-NEXT:    [[P2:%.*]] = getelementptr i32, ptr [[P1]], i64 [[INDEX]]
; CHECK-NEXT:    ret ptr [[P2]]
;
entry:
  %p1 = getelementptr i8, ptr %base, i64 -4
  %index = xor i64 %a, 1
  %p2 = getelementptr i32, ptr %p1, i64 %index
  ret ptr %p2
}

define ptr @test_sext_add_without_nsw(ptr %base, i32 %a) {
; CHECK-LABEL: define ptr @test_sext_add_without_nsw(
; CHECK-SAME: ptr [[BASE:%.*]], i32 [[A:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[P1:%.*]] = getelementptr i8, ptr [[BASE]], i64 -4
; CHECK-NEXT:    [[INDEX:%.*]] = add i32 [[A]], 1
; CHECK-NEXT:    [[TMP0:%.*]] = sext i32 [[INDEX]] to i64
; CHECK-NEXT:    [[P2:%.*]] = getelementptr i32, ptr [[P1]], i64 [[TMP0]]
; CHECK-NEXT:    ret ptr [[P2]]
;
entry:
  %p1 = getelementptr i8, ptr %base, i64 -4
  %index = add i32 %a, 1
  %p2 = getelementptr i32, ptr %p1, i32 %index
  ret ptr %p2
}

define ptr @test_or_without_disjoint(ptr %base, i64 %a) {
; CHECK-LABEL: define ptr @test_or_without_disjoint(
; CHECK-SAME: ptr [[BASE:%.*]], i64 [[A:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[P1:%.*]] = getelementptr i8, ptr [[BASE]], i64 -4
; CHECK-NEXT:    [[INDEX:%.*]] = or i64 [[A]], 1
; CHECK-NEXT:    [[P2:%.*]] = getelementptr i32, ptr [[P1]], i64 [[INDEX]]
; CHECK-NEXT:    ret ptr [[P2]]
;
entry:
  %p1 = getelementptr i8, ptr %base, i64 -4
  %index = or i64 %a, 1
  %p2 = getelementptr i32, ptr %p1, i64 %index
  ret ptr %p2
}

define ptr @test_smul_overflow(ptr %base, i64 %a) {
; CHECK-LABEL: define ptr @test_smul_overflow(
; CHECK-SAME: ptr [[BASE:%.*]], i64 [[A:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[P1:%.*]] = getelementptr i8, ptr [[BASE]], i64 -12
; CHECK-NEXT:    [[TMP0:%.*]] = getelementptr i32, ptr [[P1]], i64 [[A]]
; CHECK-NEXT:    ret ptr [[TMP0]]
;
entry:
  %p1 = getelementptr i8, ptr %base, i64 -4
  %index = add i64 %a, 9223372036854775806
  %p2 = getelementptr i32, ptr %p1, i64 %index
  ret ptr %p2
}

define ptr @test_sadd_overflow(ptr %base, i64 %a) {
; CHECK-LABEL: define ptr @test_sadd_overflow(
; CHECK-SAME: ptr [[BASE:%.*]], i64 [[A:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[P1:%.*]] = getelementptr i8, ptr [[BASE]], i64 -9223372036854775808
; CHECK-NEXT:    [[TMP0:%.*]] = getelementptr i32, ptr [[P1]], i64 [[A]]
; CHECK-NEXT:    ret ptr [[TMP0]]
;
entry:
  %p1 = getelementptr i8, ptr %base, i64 9223372036854775804
  %index = add i64 %a, 1
  %p2 = getelementptr i32, ptr %p1, i64 %index
  ret ptr %p2
}

define ptr @test_nonzero_multiuse_index(ptr %base, i64 %a) {
; CHECK-LABEL: define ptr @test_nonzero_multiuse_index(
; CHECK-SAME: ptr [[BASE:%.*]], i64 [[A:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[P1:%.*]] = getelementptr i8, ptr [[BASE]], i64 -4
; CHECK-NEXT:    [[INDEX:%.*]] = add i64 [[A]], 2
; CHECK-NEXT:    call void @use64(i64 [[INDEX]])
; CHECK-NEXT:    [[P2:%.*]] = getelementptr i32, ptr [[P1]], i64 [[INDEX]]
; CHECK-NEXT:    ret ptr [[P2]]
;
entry:
  %p1 = getelementptr i8, ptr %base, i64 -4
  %index = add i64 %a, 2
  call void @use64(i64 %index)
  %p2 = getelementptr i32, ptr %p1, i64 %index
  ret ptr %p2
}

define ptr @test_nonzero_multiuse_ptr(ptr %base, i64 %a) {
; CHECK-LABEL: define ptr @test_nonzero_multiuse_ptr(
; CHECK-SAME: ptr [[BASE:%.*]], i64 [[A:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[P1:%.*]] = getelementptr i8, ptr [[BASE]], i64 -4
; CHECK-NEXT:    call void @useptr(ptr [[P1]])
; CHECK-NEXT:    [[TMP0:%.*]] = getelementptr i32, ptr [[P1]], i64 [[A]]
; CHECK-NEXT:    [[P2:%.*]] = getelementptr i8, ptr [[TMP0]], i64 8
; CHECK-NEXT:    ret ptr [[P2]]
;
entry:
  %p1 = getelementptr i8, ptr %base, i64 -4
  call void @useptr(ptr %p1)
  %index = add i64 %a, 2
  %p2 = getelementptr i32, ptr %p1, i64 %index
  ret ptr %p2
}

define ptr @test_scalable(ptr %base, i64 %a) {
; CHECK-LABEL: define ptr @test_scalable(
; CHECK-SAME: ptr [[BASE:%.*]], i64 [[A:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[P1:%.*]] = getelementptr i8, ptr [[BASE]], i64 -4
; CHECK-NEXT:    [[INDEX:%.*]] = add i64 [[A]], 1
; CHECK-NEXT:    [[TMP0:%.*]] = call i64 @llvm.vscale.i64()
; CHECK-NEXT:    [[TMP1:%.*]] = shl nuw i64 [[TMP0]], 4
; CHECK-NEXT:    [[P2_IDX:%.*]] = mul i64 [[INDEX]], [[TMP1]]
; CHECK-NEXT:    [[P2:%.*]] = getelementptr i8, ptr [[P1]], i64 [[P2_IDX]]
; CHECK-NEXT:    ret ptr [[P2]]
;
entry:
  %p1 = getelementptr i8, ptr %base, i64 -4
  %index = add i64 %a, 1
  %p2 = getelementptr <vscale x 4 x i32>, ptr %p1, i64 %index
  ret ptr %p2
}

define ptr @test_all_nuw(ptr %base, i64 %a) {
; CHECK-LABEL: define ptr @test_all_nuw(
; CHECK-SAME: ptr [[BASE:%.*]], i64 [[A:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = getelementptr nuw i8, ptr [[BASE]], i64 9
; CHECK-NEXT:    [[P2:%.*]] = getelementptr nuw i32, ptr [[TMP0]], i64 [[A]]
; CHECK-NEXT:    ret ptr [[P2]]
;
entry:
  %p1 = getelementptr nuw i8, ptr %base, i64 1
  %index = add nuw i64 %a, 2
  %p2 = getelementptr nuw i32, ptr %p1, i64 %index
  ret ptr %p2
}

define ptr @test_all_partial_nuw1(ptr %base, i64 %a) {
; CHECK-LABEL: define ptr @test_all_partial_nuw1(
; CHECK-SAME: ptr [[BASE:%.*]], i64 [[A:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = getelementptr i8, ptr [[BASE]], i64 9
; CHECK-NEXT:    [[P2:%.*]] = getelementptr i32, ptr [[TMP0]], i64 [[A]]
; CHECK-NEXT:    ret ptr [[P2]]
;
entry:
  %p1 = getelementptr i8, ptr %base, i64 1
  %index = add nuw i64 %a, 2
  %p2 = getelementptr nuw i32, ptr %p1, i64 %index
  ret ptr %p2
}

define ptr @test_all_partial_nuw2(ptr %base, i64 %a) {
; CHECK-LABEL: define ptr @test_all_partial_nuw2(
; CHECK-SAME: ptr [[BASE:%.*]], i64 [[A:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = getelementptr i8, ptr [[BASE]], i64 9
; CHECK-NEXT:    [[P2:%.*]] = getelementptr i32, ptr [[TMP0]], i64 [[A]]
; CHECK-NEXT:    ret ptr [[P2]]
;
entry:
  %p1 = getelementptr nuw i8, ptr %base, i64 1
  %index = add i64 %a, 2
  %p2 = getelementptr nuw i32, ptr %p1, i64 %index
  ret ptr %p2
}

define ptr @test_all_partial_nuw3(ptr %base, i64 %a) {
; CHECK-LABEL: define ptr @test_all_partial_nuw3(
; CHECK-SAME: ptr [[BASE:%.*]], i64 [[A:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = getelementptr i8, ptr [[BASE]], i64 9
; CHECK-NEXT:    [[P2:%.*]] = getelementptr i32, ptr [[TMP0]], i64 [[A]]
; CHECK-NEXT:    ret ptr [[P2]]
;
entry:
  %p1 = getelementptr nuw i8, ptr %base, i64 1
  %index = add nuw i64 %a, 2
  %p2 = getelementptr i32, ptr %p1, i64 %index
  ret ptr %p2
}

define ptr @test_all_nuw_disjoint(ptr %base, i64 %a) {
; CHECK-LABEL: define ptr @test_all_nuw_disjoint(
; CHECK-SAME: ptr [[BASE:%.*]], i64 [[A:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = getelementptr nuw i8, ptr [[BASE]], i64 9
; CHECK-NEXT:    [[P2:%.*]] = getelementptr nuw i32, ptr [[TMP0]], i64 [[A]]
; CHECK-NEXT:    ret ptr [[P2]]
;
entry:
  %p1 = getelementptr nuw i8, ptr %base, i64 1
  %index = or disjoint i64 %a, 2
  %p2 = getelementptr nuw i32, ptr %p1, i64 %index
  ret ptr %p2
}

define ptr @test_all_inbounds_nuw(ptr %base, i64 %a) {
; CHECK-LABEL: define ptr @test_all_inbounds_nuw(
; CHECK-SAME: ptr [[BASE:%.*]], i64 [[A:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = getelementptr inbounds nuw i8, ptr [[BASE]], i64 9
; CHECK-NEXT:    [[P2:%.*]] = getelementptr inbounds nuw i32, ptr [[TMP0]], i64 [[A]]
; CHECK-NEXT:    ret ptr [[P2]]
;
entry:
  %p1 = getelementptr inbounds nuw i8, ptr %base, i64 1
  %index = add nuw i64 %a, 2
  %p2 = getelementptr inbounds nuw i32, ptr %p1, i64 %index
  ret ptr %p2
}

define ptr @test_all_partial_inbounds1(ptr %base, i64 %a) {
; CHECK-LABEL: define ptr @test_all_partial_inbounds1(
; CHECK-SAME: ptr [[BASE:%.*]], i64 [[A:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = getelementptr nuw i8, ptr [[BASE]], i64 9
; CHECK-NEXT:    [[P2:%.*]] = getelementptr nuw i32, ptr [[TMP0]], i64 [[A]]
; CHECK-NEXT:    ret ptr [[P2]]
;
entry:
  %p1 = getelementptr nuw i8, ptr %base, i64 1
  %index = add nuw i64 %a, 2
  %p2 = getelementptr inbounds nuw i32, ptr %p1, i64 %index
  ret ptr %p2
}

define ptr @test_all_partial_inbounds2(ptr %base, i64 %a) {
; CHECK-LABEL: define ptr @test_all_partial_inbounds2(
; CHECK-SAME: ptr [[BASE:%.*]], i64 [[A:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = getelementptr nuw i8, ptr [[BASE]], i64 9
; CHECK-NEXT:    [[P2:%.*]] = getelementptr nuw i32, ptr [[TMP0]], i64 [[A]]
; CHECK-NEXT:    ret ptr [[P2]]
;
entry:
  %p1 = getelementptr inbounds nuw i8, ptr %base, i64 1
  %index = add nuw i64 %a, 2
  %p2 = getelementptr nuw i32, ptr %p1, i64 %index
  ret ptr %p2
}

define ptr @test_all_inbounds_partial_nuw1(ptr %base, i64 %a) {
; CHECK-LABEL: define ptr @test_all_inbounds_partial_nuw1(
; CHECK-SAME: ptr [[BASE:%.*]], i64 [[A:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = getelementptr i8, ptr [[BASE]], i64 7
; CHECK-NEXT:    [[P2:%.*]] = getelementptr i32, ptr [[TMP0]], i64 [[A]]
; CHECK-NEXT:    ret ptr [[P2]]
;
entry:
  %p1 = getelementptr inbounds i8, ptr %base, i64 -1
  %index = add nuw i64 %a, 2
  %p2 = getelementptr inbounds nuw i32, ptr %p1, i64 %index
  ret ptr %p2
}

define ptr @test_all_inbounds_partial_nuw2(ptr %base, i64 %a) {
; CHECK-LABEL: define ptr @test_all_inbounds_partial_nuw2(
; CHECK-SAME: ptr [[BASE:%.*]], i64 [[A:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = getelementptr i8, ptr [[BASE]], i64 9
; CHECK-NEXT:    [[P2:%.*]] = getelementptr i32, ptr [[TMP0]], i64 [[A]]
; CHECK-NEXT:    ret ptr [[P2]]
;
entry:
  %p1 = getelementptr inbounds nuw i8, ptr %base, i64 1
  %index = add nuw i64 %a, 2
  %p2 = getelementptr inbounds i32, ptr %p1, i64 %index
  ret ptr %p2
}

define ptr @test_all_inbounds_partial_nuw3(ptr %base, i64 %a) {
; CHECK-LABEL: define ptr @test_all_inbounds_partial_nuw3(
; CHECK-SAME: ptr [[BASE:%.*]], i64 [[A:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = getelementptr i8, ptr [[BASE]], i64 9
; CHECK-NEXT:    [[P2:%.*]] = getelementptr i32, ptr [[TMP0]], i64 [[A]]
; CHECK-NEXT:    ret ptr [[P2]]
;
entry:
  %p1 = getelementptr inbounds nuw i8, ptr %base, i64 1
  %index = add i64 %a, 2
  %p2 = getelementptr inbounds nuw i32, ptr %p1, i64 %index
  ret ptr %p2
}

define ptr @test_all_nusw_nuw(ptr %base, i64 %a) {
; CHECK-LABEL: define ptr @test_all_nusw_nuw(
; CHECK-SAME: ptr [[BASE:%.*]], i64 [[A:%.*]]) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = getelementptr nuw i8, ptr [[BASE]], i64 9
; CHECK-NEXT:    [[P2:%.*]] = getelementptr nuw i32, ptr [[TMP0]], i64 [[A]]
; CHECK-NEXT:    ret ptr [[P2]]
;
entry:
  %p1 = getelementptr nusw nuw i8, ptr %base, i64 1
  %index = add nsw nuw i64 %a, 2
  %p2 = getelementptr nusw nuw i32, ptr %p1, i64 %index
  ret ptr %p2
}
