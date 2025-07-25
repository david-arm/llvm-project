; RUN: llc -mtriple=amdgcn-amd-amdhsa -mcpu=gfx900 < %s | FileCheck -check-prefix=GCN %s

; TII::areLoadsFromSameBasePtr failed because the offset for atomics
; is different from a normal load due to the data operand.

; GCN-LABEL: {{^}}are_loads_from_same_base_ptr_ds_atomic:
; GCN: global_load_dword
; GCN: ds_min_u32
; GCN: ds_max_u32
define amdgpu_kernel void @are_loads_from_same_base_ptr_ds_atomic(ptr addrspace(1) %arg0, ptr addrspace(3) noalias %ptr0) #0 {
  %tmp1 = load volatile i32, ptr addrspace(1) %arg0
  %tmp2 = atomicrmw umin ptr addrspace(3) %ptr0, i32 %tmp1 seq_cst
  %tmp3 = atomicrmw umax ptr addrspace(3) %ptr0, i32 %tmp1 seq_cst
  ret void
}

attributes #0 = { nounwind }
