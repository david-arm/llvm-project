// REQUIRES: webassembly-registered-target

// RUN: %clang -E -dM %s -target wasm32-unknown-unknown -fwasm-exceptions | FileCheck %s -check-prefix PREPROCESSOR
// PREPROCESSOR: #define __WASM_EXCEPTIONS__ 1

// RUN: %clang_cc1 %s -triple wasm32-unknown-unknown -fms-extensions -fexceptions -fcxx-exceptions -mllvm -wasm-enable-eh -exception-model=wasm -target-feature +exception-handling -emit-llvm -o - -std=c++11 | FileCheck %s
// RUN: %clang_cc1 %s -triple wasm64-unknown-unknown -fms-extensions -fexceptions -fcxx-exceptions -mllvm -wasm-enable-eh -exception-model=wasm -target-feature +exception-handling -emit-llvm -o - -std=c++11 | FileCheck %s

// Test code generation for Wasm EH using WebAssembly EH proposal.
// (https://github.com/WebAssembly/exception-handling/blob/main/proposals/exception-handling/Exceptions.md)

void may_throw();
void dont_throw() noexcept;

struct Cleanup {
  ~Cleanup() { dont_throw(); }
};

// Multiple catch clauses w/o catch-all
void multiple_catches_wo_catch_all() {
  try {
    may_throw();
  } catch (int) {
    dont_throw();
  } catch (double) {
    dont_throw();
  }
}

// CHECK-LABEL: define void @_Z29multiple_catches_wo_catch_allv() {{.*}} personality ptr @__gxx_wasm_personality_v0

// CHECK:   %[[INT_ALLOCA:.*]] = alloca i32
// CHECK:   invoke void @_Z9may_throwv()
// CHECK-NEXT:           to label %[[NORMAL_BB:.*]] unwind label %[[CATCHDISPATCH_BB:.*]]

// CHECK: [[CATCHDISPATCH_BB]]:
// CHECK-NEXT:   %[[CATCHSWITCH:.*]] = catchswitch within none [label %[[CATCHSTART_BB:.*]]] unwind to caller

// CHECK: [[CATCHSTART_BB]]:
// CHECK-NEXT:   %[[CATCHPAD:.*]] = catchpad within %[[CATCHSWITCH]] [ptr @_ZTIi, ptr @_ZTId]
// CHECK-NEXT:   %[[EXN:.*]] = call ptr @llvm.wasm.get.exception(token %[[CATCHPAD]])
// CHECK-NEXT:   store ptr %[[EXN]], ptr %exn.slot
// CHECK-NEXT:   %[[SELECTOR:.*]] = call i32 @llvm.wasm.get.ehselector(token %[[CATCHPAD]])
// CHECK-NEXT:   %[[TYPEID:.*]] = call i32 @llvm.eh.typeid.for.p0(ptr @_ZTIi) #7
// CHECK-NEXT:   %[[MATCHES:.*]] = icmp eq i32 %[[SELECTOR]], %[[TYPEID]]
// CHECK-NEXT:   br i1 %[[MATCHES]], label %[[CATCH_INT_BB:.*]], label %[[CATCH_FALLTHROUGH_BB:.*]]

// CHECK: [[CATCH_INT_BB]]:
// CHECK-NEXT:   %[[EXN:.*]] = load ptr, ptr %exn.slot
// CHECK-NEXT:   %[[ADDR:.*]] = call ptr @__cxa_begin_catch(ptr %[[EXN]]) {{.*}} [ "funclet"(token %[[CATCHPAD]]) ]
// CHECK-NEXT:   %[[INT_VAL:.*]] = load i32, ptr %[[ADDR]]
// CHECK-NEXT:   store i32 %[[INT_VAL]], ptr %[[INT_ALLOCA]]
// CHECK-NEXT:   call void @_Z10dont_throwv() {{.*}} [ "funclet"(token %[[CATCHPAD]]) ]
// CHECK-NEXT:   call void @__cxa_end_catch() {{.*}} [ "funclet"(token %[[CATCHPAD]]) ]
// CHECK-NEXT:   catchret from %[[CATCHPAD]] to label %[[CATCHRET_DEST_BB0:.*]]

// CHECK: [[CATCHRET_DEST_BB0]]:
// CHECK-NEXT:   br label %[[TRY_CONT_BB:.*]]

// CHECK: [[CATCH_FALLTHROUGH_BB]]
// CHECK-NEXT:   %[[TYPEID:.*]] = call i32 @llvm.eh.typeid.for.p0(ptr @_ZTId) #7
// CHECK-NEXT:   %[[MATCHES:.*]] = icmp eq i32 %[[SELECTOR]], %[[TYPEID]]
// CHECK-NEXT:   br i1 %[[MATCHES]], label %[[CATCH_FLOAT_BB:.*]], label %[[RETHROW_BB:.*]]

// CHECK: [[CATCH_FLOAT_BB]]:
// CHECK:   catchret from %[[CATCHPAD]] to label %[[CATCHRET_DEST_BB1:.*]]

// CHECK: [[CATCHRET_DEST_BB1]]:
// CHECK-NEXT:   br label %[[TRY_CONT_BB]]

// CHECK: [[RETHROW_BB]]:
// CHECK-NEXT:   call void @llvm.wasm.rethrow() {{.*}} [ "funclet"(token %[[CATCHPAD]]) ]
// CHECK-NEXT:   unreachable

// Single catch-all
void single_catch_all() {
  try {
    may_throw();
  } catch (...) {
    dont_throw();
  }
}

// CATCH-LABEL: @_Z16single_catch_allv()

// CHECK:   %[[CATCHSWITCH:.*]] = catchswitch within none [label %[[CATCHSTART_BB:.*]]] unwind to caller

// CHECK: [[CATCHSTART_BB]]:
// CHECK-NEXT:   %[[CATCHPAD:.*]] = catchpad within %[[CATCHSWITCH]] [ptr null]
// CHECK:   br label %[[CATCH_ALL_BB:.*]]

// CHECK: [[CATCH_ALL_BB]]:
// CHECK:   catchret from %[[CATCHPAD]] to label

// Multiple catch clauses w/ catch-all
void multiple_catches_w_catch_all() {
  try {
    may_throw();
  } catch (int) {
    dont_throw();
  } catch (...) {
    dont_throw();
  }
}

// CHECK-LABEL: @_Z28multiple_catches_w_catch_allv()

// CHECK:   %[[CATCHSWITCH:.*]] = catchswitch within none [label %[[CATCHSTART_BB:.*]]] unwind to caller

// CHECK: [[CATCHSTART_BB]]:
// CHECK-NEXT:   %[[CATCHPAD:.*]] = catchpad within %[[CATCHSWITCH]] [ptr @_ZTIi, ptr null]
// CHECK:   br i1 %{{.*}}, label %[[CATCH_INT_BB:.*]], label %[[CATCH_ALL_BB:.*]]

// CHECK: [[CATCH_INT_BB]]:
// CHECK:   catchret from %[[CATCHPAD]] to label

// CHECK: [[CATCH_ALL_BB]]:
// CHECK:   catchret from %[[CATCHPAD]] to label

// Cleanup
void cleanup() {
  Cleanup c;
  may_throw();
}

// CHECK-LABEL: @_Z7cleanupv()

// CHECK:   invoke void @_Z9may_throwv()
// CHECK-NEXT:           to label {{.*}} unwind label %[[EHCLEANUP_BB:.*]]

// CHECK: [[EHCLEANUP_BB]]:
// CHECK-NEXT:   %[[CLEANUPPAD:.*]] = cleanuppad within none []
// CHECK-NEXT:   call noundef ptr @_ZN7CleanupD1Ev(ptr {{[^,]*}} %{{.*}}) {{.*}} [ "funclet"(token %[[CLEANUPPAD]]) ]
// CHECK-NEXT:   cleanupret from %[[CLEANUPPAD]] unwind to caller

// Possibly throwing function call within a catch
void catch_int() {
  try {
    may_throw();
  } catch (int) {
    may_throw();
  }
}

// CHECK-LABEL: @_Z9catch_intv()

// CHECK:   %[[CATCHSWITCH]] = catchswitch within none [label %[[CATCHSTART_BB]]] unwind to caller

// CHECK: [[CATCHSTART_BB]]:
// CHECK:   %[[CATCHPAD:.*]] = catchpad within %[[CATCHSWITCH]] [ptr @_ZTIi]

// CHECK:   invoke void @_Z9may_throwv() [ "funclet"(token %[[CATCHPAD]]) ]
// CHECK-NEXT:           to label %[[INVOKE_CONT_BB:.*]] unwind label %[[EHCLEANUP_BB:.*]]

// CHECK: [[INVOKE_CONT_BB]]:
// CHECK-NEXT:   call void @__cxa_end_catch() {{.*}} [ "funclet"(token %[[CATCHPAD]]) ]
// CHECK-NEXT:   catchret from %[[CATCHPAD]] to label

// CHECK: [[EHCLEANUP_BB]]:
// CHECK-NEXT:   %[[CLEANUPPAD:.*]] = cleanuppad within %[[CATCHPAD]] []
// CHECK-NEXT:   call void @__cxa_end_catch() {{.*}} [ "funclet"(token %[[CLEANUPPAD]]) ]
// CHECK-NEXT:   cleanupret from %[[CLEANUPPAD]] unwind to caller

// Possibly throwing function call within a catch-all
void catch_all() {
  try {
    may_throw();
  } catch (...) {
    may_throw();
  }
}

// CHECK-LABEL: @_Z9catch_allv()

// CHECK:   %[[CATCHSWITCH:.*]] = catchswitch within none [label %[[CATCHSTART_BB]]] unwind to caller

// CHECK: [[CATCHSTART_BB]]:
// CHECK:   %[[CATCHPAD:.*]] = catchpad within %[[CATCHSWITCH]] [ptr null]

// CHECK:   invoke void @_Z9may_throwv() [ "funclet"(token %[[CATCHPAD]]) ]
// CHECK-NEXT:           to label %[[INVOKE_CONT_BB0:.*]] unwind label %[[EHCLEANUP_BB:.*]]

// CHECK: [[INVOKE_CONT_BB0]]:
// CHECK-NEXT:   call void @__cxa_end_catch() [ "funclet"(token %[[CATCHPAD]]) ]
// CHECK-NEXT:   catchret from %[[CATCHPAD]] to label

// CHECK: [[EHCLEANUP_BB]]:
// CHECK-NEXT:   %[[CLEANUPPAD0:.*]] = cleanuppad within %[[CATCHPAD]] []
// CHECK-NEXT:   invoke void @__cxa_end_catch() [ "funclet"(token %[[CLEANUPPAD0]]) ]
// CHECK-NEXT:           to label %[[INVOKE_CONT_BB1:.*]] unwind label %[[TERMINATE_BB:.*]]

// CHECK: [[INVOKE_CONT_BB1]]:
// CHECK-NEXT:   cleanupret from %[[CLEANUPPAD0]] unwind to caller

// CHECK: [[TERMINATE_BB]]:
// CHECK-NEXT:   %[[CLEANUPPAD1:.*]] = cleanuppad within %[[CLEANUPPAD0]] []
// CHECK-NEXT:   call void @_ZSt9terminatev() {{.*}} [ "funclet"(token %[[CLEANUPPAD1]]) ]
// CHECK-NEXT:   unreachable

// Try-catch with cleanups
void try_catch_w_cleanups() {
  Cleanup c1;
  try {
    Cleanup c2;
    may_throw();
  } catch (int) {
    Cleanup c3;
    may_throw();
  }
}

// CHECK-LABEL: @_Z20try_catch_w_cleanupsv()
// CHECK:   invoke void @_Z9may_throwv()
// CHECK-NEXT:           to label %{{.*}} unwind label %[[EHCLEANUP_BB0:.*]]

// CHECK: [[EHCLEANUP_BB0]]:
// CHECK-NEXT:   %[[CLEANUPPAD0:.*]] = cleanuppad within none []
// CHECK-NEXT:   call noundef ptr @_ZN7CleanupD1Ev(ptr {{.*}}) {{.*}} [ "funclet"(token %[[CLEANUPPAD0]]) ]
// CHECK-NEXT:   cleanupret from %[[CLEANUPPAD0]] unwind label %[[CATCH_DISPATCH_BB:.*]]

// CHECK: [[CATCH_DISPATCH_BB]]:
// CHECK-NEXT:  %[[CATCHSWITCH:.*]] = catchswitch within none [label %[[CATCHSTART_BB:.*]]] unwind label %[[EHCLEANUP_BB1:.*]]

// CHECK: [[CATCHSTART_BB]]:
// CHECK-NEXT:   %[[CATCHPAD:.*]] = catchpad within %[[CATCHSWITCH]] [ptr @_ZTIi]
// CHECK:   br i1 %{{.*}}, label %[[CATCH_INT_BB:.*]], label %[[RETHROW_BB:.*]]

// CHECK: [[CATCH_INT_BB]]:
// CHECK:   invoke void @_Z9may_throwv() [ "funclet"(token %[[CATCHPAD]]) ]
// CHECK-NEXT:           to label %[[INVOKE_CONT_BB:.*]] unwind label %[[EHCLEANUP_BB2:.*]]

// CHECK: [[INVOKE_CONT_BB]]:
// CHECK:   catchret from %[[CATCHPAD]] to label %{{.*}}

// CHECK: [[RETHROW_BB]]:
// CHECK-NEXT:   invoke void @llvm.wasm.rethrow() {{.*}} [ "funclet"(token %[[CATCHPAD]]) ]
// CHECK-NEXT:          to label %[[UNREACHABLE_BB:.*]] unwind label %[[EHCLEANUP_BB1:.*]]

// CHECK: [[EHCLEANUP_BB2]]:
// CHECK-NEXT:   %[[CLEANUPPAD2:.*]] = cleanuppad within %[[CATCHPAD]] []
// CHECK-NEXT:   call noundef ptr @_ZN7CleanupD1Ev(ptr {{[^,]*}} %{{.*}}) {{.*}} [ "funclet"(token %[[CLEANUPPAD2]]) ]
// CHECK-NEXT:   cleanupret from %[[CLEANUPPAD2]] unwind label %[[EHCLEANUP_BB3:.*]]

// CHECK: [[EHCLEANUP_BB3]]:
// CHECK-NEXT:   %[[CLEANUPPAD3:.*]] = cleanuppad within %[[CATCHPAD]] []
// CHECK:   cleanupret from %[[CLEANUPPAD3]] unwind label %[[EHCLEANUP_BB1:.*]]

// CHECK: [[EHCLEANUP_BB1]]:
// CHECK-NEXT:   %[[CLEANUPPAD1:.*]] = cleanuppad within none []
// CHECK-NEXT:   call noundef ptr @_ZN7CleanupD1Ev(ptr {{[^,]*}} %{{.*}}) {{.*}} [ "funclet"(token %[[CLEANUPPAD1]]) ]
// CHECK-NEXT:   cleanupret from %[[CLEANUPPAD1]] unwind to caller

// CHECK: [[UNREACHABLE_BB]]:
// CHECK-NEXT:   unreachable

// Nested try-catches within a try with cleanups
void nested_try_catches_with_cleanups() {
  Cleanup c1;
  may_throw();
  try {
    Cleanup c2;
    may_throw();
    try {
      Cleanup c3;
      may_throw();
    } catch (int) {
      may_throw();
    } catch (double) {
      may_throw();
    }
  } catch (int) {
    may_throw();
  } catch (...) {
    may_throw();
  }
}

// CHECK-LABEL: @_Z32nested_try_catches_with_cleanupsv()
// CHECK:   invoke void @_Z9may_throwv()

// CHECK:   invoke void @_Z9may_throwv()

// CHECK:   invoke void @_Z9may_throwv()

// CHECK:   %[[CLEANUPPAD0:.*]] = cleanuppad within none []
// CHECK:   cleanupret from %[[CLEANUPPAD0]] unwind label

// CHECK:   %[[CATCHSWITCH0:.*]] = catchswitch within none

// CHECK:   %[[CATCHPAD0:.*]] = catchpad within %[[CATCHSWITCH0]] [ptr @_ZTIi, ptr @_ZTId]

// CHECK:   invoke void @_Z9may_throwv() [ "funclet"(token %[[CATCHPAD0]]) ]

// CHECK:   catchret from %[[CATCHPAD0]] to label

// CHECK:   invoke void @_Z9may_throwv() [ "funclet"(token %[[CATCHPAD0]]) ]

// CHECK:   catchret from %[[CATCHPAD0]] to label

// CHECK:   invoke void @llvm.wasm.rethrow() {{.*}} [ "funclet"(token %[[CATCHPAD0]]) ]

// CHECK:   %[[CLEANUPPAD1:.*]] = cleanuppad within %[[CATCHPAD0]] []
// CHECK:   cleanupret from %[[CLEANUPPAD1]] unwind label

// CHECK:   %[[CLEANUPPAD2:.*]] = cleanuppad within %[[CATCHPAD0]] []
// CHECK:   cleanupret from %[[CLEANUPPAD2]] unwind label

// CHECK:   %[[CLEANUPPAD3:.*]] = cleanuppad within none []
// CHECK:   cleanupret from %[[CLEANUPPAD3]] unwind label

// CHECK:   %[[CATCHSWITCH1:.*]] = catchswitch within none

// CHECK:   %[[CATCHPAD1:.*]] = catchpad within %[[CATCHSWITCH1]] [ptr @_ZTIi, ptr null]

// CHECK:   invoke void @_Z9may_throwv() [ "funclet"(token %[[CATCHPAD1]]) ]

// CHECK:   catchret from %[[CATCHPAD1]] to label

// CHECK:   invoke void @_Z9may_throwv() [ "funclet"(token %[[CATCHPAD1]]) ]

// CHECK:   invoke void @__cxa_end_catch() [ "funclet"(token %[[CATCHPAD1]]) ]

// CHECK:   catchret from %[[CATCHPAD1]] to label

// CHECK:   %[[CLEANUPPAD4:.*]] = cleanuppad within %[[CATCHPAD1]] []
// CHECK:   invoke void @__cxa_end_catch() [ "funclet"(token %[[CLEANUPPAD4]]) ]

// CHECK:   cleanupret from %[[CLEANUPPAD4]] unwind label

// CHECK:   %[[CLEANUPPAD5:.*]] = cleanuppad within %[[CATCHPAD1]] []
// CHECK:   cleanupret from %[[CLEANUPPAD5]] unwind label

// CHECK:   %[[CLEANUPPAD6:.*]] = cleanuppad within none []
// CHECK:   cleanupret from %[[CLEANUPPAD6]] unwind to caller

// CHECK:   unreachable

// CHECK:   %[[CLEANUPPAD7:.*]] = cleanuppad within %[[CLEANUPPAD4]] []
// CHECK:   call void @_ZSt9terminatev() {{.*}} [ "funclet"(token %[[CLEANUPPAD7]]) ]
// CHECK:   unreachable

// Nested try-catches within a catch
void nested_try_catch_within_catch() {
  try {
    may_throw();
  } catch (int) {
    try {
      may_throw();
    } catch (int) {
      may_throw();
    }
  }
}

// CHECK-LABEL: @_Z29nested_try_catch_within_catchv()
// CHECK:   invoke void @_Z9may_throwv()

// CHECK:   %[[CATCHSWITCH0:.*]] = catchswitch within none

// CHECK:   %[[CATCHPAD0:.*]] = catchpad within %[[CATCHSWITCH0]] [ptr @_ZTIi]

// CHECK:   invoke void @_Z9may_throwv() [ "funclet"(token %[[CATCHPAD0]]) ]

// CHECK:   %[[CATCHSWITCH1:.*]] = catchswitch within %[[CATCHPAD0]]

// CHECK:   %[[CATCHPAD1:.*]] = catchpad within %[[CATCHSWITCH1]] [ptr @_ZTIi]

// CHECK:   invoke void @_Z9may_throwv() [ "funclet"(token %[[CATCHPAD1]]) ]

// CHECK:   catchret from %[[CATCHPAD1]] to label

// CHECK:   invoke void @llvm.wasm.rethrow() {{.*}} [ "funclet"(token %[[CATCHPAD1]]) ]

// CHECK:   catchret from %[[CATCHPAD0]] to label

// CHECK:   call void @llvm.wasm.rethrow() {{.*}} [ "funclet"(token %[[CATCHPAD0]]) ]
// CHECK:   unreachable

// CHECK:   %[[CLEANUPPAD0:.*]] = cleanuppad within %[[CATCHPAD1]] []
// CHECK:   cleanupret from %[[CLEANUPPAD0]] unwind label

// CHECK:   %[[CLEANUPPAD1:.*]] = cleanuppad within %[[CATCHPAD0]] []
// CHECK:   cleanupret from %[[CLEANUPPAD1]] unwind to caller

// CHECK:   unreachable

void noexcept_throw() noexcept {
  throw 3;
}

// CATCH-LABEL: define void @_Z14noexcept_throwv()
// CHECK: %{{.*}} = cleanuppad within none []
// CHECK-NEXT:  call void @_ZSt9terminatev()


// RUN: %clang_cc1 %s -triple wasm32-unknown-unknown -fms-extensions -fexceptions -fcxx-exceptions -mllvm -wasm-enable-eh -exception-model=wasm -target-feature +exception-handling -emit-llvm -o - -std=c++11 2>&1 | FileCheck %s --check-prefix=WARNING-DEFAULT
// RUN: %clang_cc1 %s -triple wasm32-unknown-unknown -fms-extensions -fexceptions -fcxx-exceptions -mllvm -wasm-enable-eh -exception-model=wasm -target-feature +exception-handling -Wwasm-exception-spec -emit-llvm -o - -std=c++11 2>&1 | FileCheck %s --check-prefix=WARNING-ON
// RUN: %clang_cc1 %s -triple wasm32-unknown-unknown -fms-extensions -fexceptions -fcxx-exceptions -mllvm -wasm-enable-eh -exception-model=wasm -target-feature +exception-handling -Wno-wasm-exception-spec -emit-llvm -o - -std=c++11 2>&1 | FileCheck %s --check-prefix=WARNING-OFF
// RUN: %clang_cc1 %s -triple wasm32-unknown-unknown -fexceptions -fcxx-exceptions -emit-llvm -o - -std=c++11 2>&1 | FileCheck %s --check-prefix=EM-EH-WARNING

// Wasm EH ignores dynamic exception specifications with types at the moment.
// This is controlled by -Wwasm-exception-spec, which is on by default. This
// warning can be suppressed with -Wno-wasm-exception-spec. Checks if a warning
// message is correctly printed or not printed depending on the options.
void exception_spec_warning() throw(int) {
}
// WARNING-DEFAULT: warning: dynamic exception specifications with types are currently ignored in wasm
// WARNING-ON: warning: dynamic exception specifications with types are currently ignored in wasm
// WARNING-OFF-NOT: warning: dynamic exception specifications with types are currently ignored in wasm
// EM-EH-WARNING: warning: dynamic exception specifications with types are currently ignored in wasm

// Wasm currently treats 'throw()' in the same way as 'noexcept'. Check if the
// same warning message is printed as if when a 'noexcept' function throws.
void exception_spec_throw_empty() throw() {
  throw 3;
}
// WARNING-DEFAULT: warning: 'exception_spec_throw_empty' has a non-throwing exception specification but can still throw
// WARNING-DEFAULT: function declared non-throwing here

// Here we only check if the command enables wasm exception handling in the
// backend so that exception handling instructions can be generated in .s file.

// RUN: %clang_cc1 %s -triple wasm32-unknown-unknown -fms-extensions -fexceptions -fcxx-exceptions -mllvm -wasm-enable-eh -exception-model=wasm -target-feature +exception-handling -S -o - -std=c++11 | FileCheck %s --check-prefix=ASSEMBLY

// ASSEMBLY: try
// ASSEMBLY: catch
// ASSEMBLY: rethrow
// ASSEMBLY: end_try
