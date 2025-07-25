; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py UTC_ARGS: --version 2
; RUN: llc -mtriple=amdgcn-amd-amdhsa -mcpu=gfx942 < %s | FileCheck -enable-var-scope -check-prefixes=GFX942 %s


define protected amdgpu_kernel void @test(ptr addrspace(1) %in, ptr addrspace(1) %out) #0 {
; GFX942-LABEL: test:
; GFX942:       ; %bb.0: ; %entry
; GFX942-NEXT:    s_load_dwordx4 s[0:3], s[4:5], 0x0
; GFX942-NEXT:    v_mov_b32_e32 v0, 0
; GFX942-NEXT:    v_mov_b32_e32 v2, v0
; GFX942-NEXT:    v_mov_b32_e32 v3, v0
; GFX942-NEXT:    v_mov_b32_e32 v1, v0
; GFX942-NEXT:    s_waitcnt lgkmcnt(0)
; GFX942-NEXT:    s_load_dwordx4 s[4:7], s[0:1], 0x0
; GFX942-NEXT:    v_mov_b64_e32 v[10:11], v[2:3]
; GFX942-NEXT:    v_mov_b64_e32 v[8:9], v[0:1]
; GFX942-NEXT:    s_waitcnt lgkmcnt(0)
; GFX942-NEXT:    v_mov_b32_e32 v12, s4
; GFX942-NEXT:    v_mov_b32_e32 v13, s5
; GFX942-NEXT:    v_mov_b32_e32 v4, s6
; GFX942-NEXT:    v_mov_b32_e32 v5, s7
; GFX942-NEXT:    v_mov_b32_e32 v6, s7
; GFX942-NEXT:    v_mov_b32_e32 v7, s7
; GFX942-NEXT:    s_nop 1
; GFX942-NEXT:    v_smfmac_i32_16x16x64_i8 v[8:11], v[12:13], v[4:7], v13
; GFX942-NEXT:    s_nop 6
; GFX942-NEXT:    global_store_dword v0, v11, s[2:3] offset:12
; GFX942-NEXT:    s_endpgm
entry:
  %arrayidx = getelementptr inbounds i32, ptr addrspace(1) %in, i64 0
  %arrayidx1 = getelementptr inbounds i32, ptr addrspace(1) %in, i64 1
  %arrayidx2 = getelementptr inbounds i32, ptr addrspace(1) %in, i64 2
  %arrayidx3 = getelementptr inbounds i32, ptr addrspace(1) %in, i64 3
  %0 = load i32, ptr addrspace(1) %arrayidx
  %1 = load i32, ptr addrspace(1) %arrayidx1
  %2 = load i32, ptr addrspace(1) %arrayidx2
  %3 = load i32, ptr addrspace(1) %arrayidx3
  %src1.0 = insertelement <2 x i32> poison, i32 %0, i64 0
  %src1 = insertelement <2 x i32> %src1.0, i32 %1, i64 1
  %src2.0 = insertelement <4 x i32> poison, i32 %2, i64 0
  %src2.1 = insertelement <4 x i32> %src2.0, i32 %3, i64 1
  %src2.2 = insertelement <4 x i32> %src2.1, i32 %3, i64 2
  %src2 = insertelement <4 x i32> %src2.2, i32 %3, i64 3
  %4 = tail call <4 x i32> @llvm.amdgcn.smfmac.i32.16x16x64.i8(<2 x i32> %src1, <4 x i32> %src2, <4 x i32> zeroinitializer, i32 %1, i32 0, i32 0)
  %vecext = extractelement <4 x i32> %4, i64 0
  %vecext.1 = extractelement <4 x i32> %4, i64 1
  %vecext.2 = extractelement <4 x i32> %4, i64 2
  %vecext.3 = extractelement <4 x i32> %4, i64 3
  %arrayidx4 = getelementptr inbounds i32, ptr addrspace(1) %out, i64 3
  store i32 %vecext.3, ptr addrspace(1) %arrayidx4
  ret void
}
declare <4 x i32> @llvm.amdgcn.smfmac.i32.16x16x64.i8(<2 x i32>, <4 x i32>, <4 x i32>, i32, i32 immarg, i32 immarg)

attributes #0 = { "amdgpu-agpr-alloc"="0" }
