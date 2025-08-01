//===- LinalgReleayoutOps.td - Linalg relayout ops ---------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines Pack + Unpack Ops that have been moved from the Tensor
// dialect. As such, these are defined as memory-effect-free and only accept
// "tensors" as inputs.
//
// TODO: Once a good motivating example is identified, relax these
// restrictions.
//
//===----------------------------------------------------------------------===//

#ifndef LINALG_RELEAYOUT_OPS
#define LINALG_RELEAYOUT_OPS

include "mlir/Dialect/Linalg/IR/LinalgBase.td"
include "mlir/Interfaces/DestinationStyleOpInterface.td"
include "mlir/Interfaces/SideEffectInterfaces.td"
include "mlir/Interfaces/InferTypeOpInterface.td"
include "mlir/Dialect/Linalg/IR/LinalgInterfaces.td"
include "mlir/Dialect/Linalg/IR/RelayoutOpInterface.td"
include "mlir/IR/OpAsmInterface.td"

//===----------------------------------------------------------------------===//
// RelayoutOp
//===----------------------------------------------------------------------===//

class Linalg_RelayoutOp<string mnemonic, list<Trait> traits = []> :
      Op<Linalg_Dialect, mnemonic, !listconcat(traits, [
        DeclareOpInterfaceMethods<OpAsmOpInterface, ["getAsmResultNames"]>,
        DestinationStyleOpInterface, LinalgRelayoutOpInterface,
        ConditionallySpeculatable, NoMemoryEffect,
        DeclareOpInterfaceMethods<ReifyRankedShapedTypeOpInterface>,
        TypesMatchWith<"result type matches type of dest",
                   "dest", "result",
                   "$_self">])> {

  code commonExtraClassDeclaration = [{
    size_t getSourceRank() { return getSourceType().getRank(); };
    size_t getDestRank() { return getDestType().getRank(); };
    RankedTensorType getSourceType() {
      return ::llvm::cast<RankedTensorType>(getSource().getType()); };
    RankedTensorType getDestType() {
      return ::llvm::cast<RankedTensorType>(getDest().getType()); };

    MutableOperandRange getDpsInitsMutable() { return getDestMutable(); }

    /// Interface method for ConditionallySpeculatable.
    Speculation::Speculatability getSpeculatability();

    /// Return a mapping from positions `inner_dims_pos` to their
    /// tile factors.
    DenseMap<int64_t, OpFoldResult> getDimAndTileMapping();

    /// Return the tile sizes as OpFoldResult.
    SmallVector<OpFoldResult> getMixedTiles();

    /// Return the tile sizes as `int64_t`. If a tile size is dynamic
    /// a sentinel `kDynamic` is introduced at that position in
    /// the returned vector.
    SmallVector<int64_t> getStaticTiles();

    /// Retrieve all outer dims for this Pack/UnPack Op, i.e. all the leading
    /// dims excluding the trailing dims corresponding to `innerTiles`. Note
    /// that this will include both tiled and non-tiled dimensions. The order
    /// of the output dimensions is consistent with the shape of the packed
    /// tensor.
    ArrayRef<int64_t> getAllOuterDims();

    /// Similar to `getAllOuterDims`, but only retrieve the outer dims that
    /// have been tiled. Also, the order of the output dimensions is consistent
    /// with `inner_dims_pos` rather than the packed tensor.
    SmallVector<int64_t> getTiledOuterDims();
  }];

  let hasVerifier = 1;
}

//===----------------------------------------------------------------------===//
// PackOp
//===----------------------------------------------------------------------===//

def Linalg_PackOp : Linalg_RelayoutOp<"pack", [
    AttrSizedOperandSegments]> {
  let summary = "linalg.pack operation";
  let description = [{
    The "pack" operation converts a source tensor of rank `n` into a result
    tensor of rank `n + k` with a tiled and packed layout (maybe with padding)
    and optionally transposes the tiled source tensor dimensions.

    `inner_tiles` (mandatory) specifies `k` tile sizes. These tile sizes
    correspond to the least significant ("inner") result tensor dimension sizes,
    in the same order. Tile sizes can be static or dynamic.

    `inner_dims_pos` (mandatory) specifies `k` source tensor dimensions that are
    being tiled, where `0 <= k <= n`.
     - `inner_dims_pos[i]` specifies the source tensor dimension tiled by
    `inner_tiles[i]` where `0 <= i < k`. All the values in `inner_dims_pos` are
    within [0, n).
     - The tiled dimensions (of size `inner_tiles`) are added to the end of the
     result tensor in the order in which they appear, i.e.
     `shape(result)[rank(result) + i] = inner_tiles[i]` for `0 <= i < k`.
     - The following relationship for the tiled dimensions holds:
     `shape(result)[inner_dims_pos[i]] = shape(source)[inner_dims_pos[i]] / inner_tiles[i]`,
     where (⌈/⌉ indicates CeilDiv).


    Example: If `inner_tiles = [16, 32]`, the result tensor has a shape of
    `...x16x32`. If `inner_dims_pos = [0, 1]`, the 0th source dimension is tiled
    by 16 and the 1st source dimension is tiled by 32. Other source dimensions
    (if any) are not tiled. If `inner_dims_pos = [1, 0]`, the 1st dimension is
    tiled by 16 and the 0th dimension is tiled by 32.

    Example:
    ```mlir
    // NC to NCnc
    %0 = linalg.pack %source inner_dims_pos = [0, 1] inner_tiles = [8, 32]
        into %dest : tensor<128x256xf32> -> tensor<16x8 x 8x32 xf32>
    //                                             \  /   \  /
    //                                 Outer Dims: 16x8   Inner Dims: 8x32

    // CHW to CHWhw
    %0 = linalg.pack %source inner_dims_pos = [2, 1] inner_tiles = [4, 2]
        into %dest : tensor<3x20x24xf32> -> tensor<3x10x6 x 4x2 xf32>
    //                                              \  /    \ /
    //                                 Outer Dims: 3x10x6  Inner Dims: 4x2

    // HCW to HCWhw
    %0 = linalg.pack %source inner_dims_pos = [2, 0] inner_tiles = [4, 2]
        into %dest : tensor<18x3x32xf32> -> tensor<9x3x8 x 4x2 xf32>
    //                                              \  /   \ /
    //                                 Outer Dims: 9x3x8  Inner Dims: 4x2
    ```

    `outer_dims_perm` (optional) specifies a permutation for the outer
    dimensions. If specified, it must have `n` elements.

    Example:
    ```mlir
    // CK to KCck
    %0 = linalg.pack %source outer_dims_perm = [1, 0] inner_dims_pos = [0, 1]
        inner_tiles = [8, 32] into %dest
        : tensor<128x256xf32> -> tensor<8x16 x 8x32 xf32>
    //                                  \  /
    //            compare with "NC to NCnc": outer dims are transposed
    ```

    `padding_value` specifies a padding value at the boundary on non-perfectly
    divisible dimensions. Padding is optional:
    - If absent, it is assumed that for all inner tiles,
      `shape(source)[inner_dims_pos[i]] % inner_tiles[i] == 0`, i.e. all inner
      tiles divide perfectly the corresponding outer dimension in the result
      tensor. It is UB if the tile does not perfectly divide the dimension.
    - If present, it will pad along high dimensions (high-padding) to make the
      tile complete. Note that it is not allowed to have artificial padding that
      is not strictly required by linalg.pack (i.e., padding past what is needed
      to complete the last tile along each packed dimension). It is UB if extra
      padding is requested.
    It is not possible to verify the requirements statically with dynamic
    shapes, so they are treated as UB.

    Example:
    ```mlir
    %0 = linalg.pack %arg0 padding_value(%pad : f32) outer_dims_perm = [2, 1, 0]
        inner_dims_pos = [1] inner_tiles = [2] into %arg1
        : tensor<200x127x256xf32> -> tensor<256x64x200x2xf32>
    //                 \
    //                padded and tiled dim
    //
    // Source dimension 1 is tiled. 64 does not divide 127 evenly, so 1 padded
    // element is added at the end.
    //
    // Note: Only tiled dimensions can be padded.
    ```

    Invalid example that has artificial padding:
    ```mlir
    %0 = linalg.pack %src padding_value(%cst : f32) inner_dims_pos = [0]
        inner_tiles = [8] into %dest
        : tensor<9xf32> -> tensor<3x8xf32>
    //                             \
    //            expect tensor<2x8xf32> because CeilDiv(9, 8) = 2
    ```
  }];
  let arguments = (ins AnyRankedTensor:$source,
                       AnyRankedTensor:$dest,
                       Optional<AnyType>:$padding_value,
                       DefaultValuedOptionalAttr<DenseI64ArrayAttr, "{}">:$outer_dims_perm,
                       DenseI64ArrayAttr:$inner_dims_pos,
                       Variadic<Index>:$inner_tiles,
                       DenseI64ArrayAttr:$static_inner_tiles);
  let results = (outs AnyRankedTensor:$result);
  let assemblyFormat = [{
    $source
    (`padding_value` `(` $padding_value^ `:` type($padding_value) `)`)?
    (`outer_dims_perm` `=` $outer_dims_perm^)?
    `inner_dims_pos` `=` $inner_dims_pos
    `inner_tiles` `=`
    custom<DynamicIndexList>($inner_tiles, $static_inner_tiles)
    `into` $dest attr-dict `:` type($source) `->` type($dest)
  }];

  let builders = [
    OpBuilder<(ins "Value":$source, "Value":$dest,
      "ArrayRef<int64_t>":$innerDimsPos,
      "ArrayRef<OpFoldResult>":$innerTiles,
      CArg<"std::optional<Value>", "std::nullopt">:$paddingValue,
      CArg<"ArrayRef<int64_t>", "{}">:$outerDimsPerm)>
  ];

  let extraClassDeclaration = commonExtraClassDeclaration # [{
    // Method to get the shape of the result as `SmallVector<OpFoldResult>`.
    // This is a static method to allow getting the shape of the destination
    // expected while creating a `pack` op.
    static SmallVector<OpFoldResult> getResultShape(OpBuilder &builder,
        Location loc, ArrayRef<OpFoldResult> sourceDims,
        ArrayRef<OpFoldResult> innerTileDims, ArrayRef<int64_t> innerDimsPos,
        ArrayRef<int64_t> outerDimsPerm = {});

    // Method to get the `RankedTensorType` of the result based on the inner
    // tiles, position of the inner tiles (innerDimsPos)  and interchange vector
    // of outer loops (outerDimsPerm).
    static RankedTensorType inferPackedType(RankedTensorType sourceType,
        ArrayRef<int64_t> innerTileSizes, ArrayRef<int64_t> innerDimsPos,
        ArrayRef<int64_t> outerDimsPerm = {});

    // Returns true if we have enough static information to catch undefined
    // behavior when the tile size does not divide perfectly the dimension of
    // the input tensor. Detecting UB requires that the input size and either
    // corresponding tile or output size are static.
    static bool requirePaddingValue(ArrayRef<int64_t> inputShape,
                                    ArrayRef<int64_t> innerDimsPos,
                                    ArrayRef<int64_t> outputShape,
                                    ArrayRef<int64_t> outerDimsPerm,
                                    ArrayRef<OpFoldResult> innerTiles);

    static Value createDestinationTensor(OpBuilder &b, Location loc,
        Value source, ArrayRef<OpFoldResult> innerTileSizes,
        ArrayRef<int64_t> innerDimsPos, ArrayRef<int64_t> outerDimsPerm);

    /// Build and return a new PackOp that is a clone of the current PackOp with
    /// (innerDimsPos, innerTiles) (resp. outerDimsPerm) are permuted by
    /// innerPermutation (resp. outerPermutation).
    /// A new `tensor.empty` of the proper shape is built in the process.
    /// Asserts that:
    ///   - At least one of innerPermutation or outerPermutation is non-empty.
    ///   - If not empty, innerPermutation is a valid permutation of size
    ///     matching innerDimPos.
    ///   - If not empty, outerPermutation is a valid permutation of size
    ///     matching outerDimsPerm.
    PackOp createTransposedClone(OpBuilder &b,
                                 Location loc,
                                 ArrayRef<int64_t> innerPermutation,
                                 ArrayRef<int64_t> outerPermutation);

    /// Check if this PackOp is like a simple pad operation.
    /// In other words, this operation:
    /// 1. adds useless dimensions (dimension of size 1),
    /// 2. pads the other ones, and
    /// 3. doesn't shuffle the dimensions
    bool isLikePad();
  }];

  let hasCanonicalizeMethod = 1;

  let hasFolder = 1;
}

//===----------------------------------------------------------------------===//
// UnPackOp
//===----------------------------------------------------------------------===//

def Linalg_UnPackOp : Linalg_RelayoutOp<"unpack"> {
  let summary = "linalg.unpack operation";
  let description = [{
    The "unpack" operation converts a source tensor of rank `n` with a tiled and
    packed layout to a result tensor of rank `n - k`.

    `inner_tiles` (mandatory) specifies `k` tile sizes. These tile sizes
    correspond to the least significant ("inner") source tensor dimension sizes.
    The behavior of this op is undefined if:
    - `inner_tiles` do not exactly match with the corresponding source tensor
      dimension sizes.
    - Or, `inner_tiles[i]` does not divide the size of dimension
      `inner_dims_pos[i]` (assuming that `outer_dims_perm` is not specified)
      evenly.

    `inner_dims_pos` (mandatory) specifies `k` result tensor (i.e. unpacked
    tensor) dimensions that were tiled with the `inner_tiles` to create the
    packed source tensor. The source tensor (i.e. packed tensor) dimensions can
    be unpacked given `inner_dims_pos` as follows.
    - For `0 <= i < k` the following relationship holds:
    `shape(result)[inner_dims_pos[i]] <= shape(source)[n-k+i] * shape(source)[inner_dims_pos[i]]`.
    - For `0 <= j < n-k` and `j` not in `inner_dims_pos` the following relationship holds:
    `shape(result)[j] = shape(source)[j]`.

    `outer_dims_perm` (optional) specifies a permutation for the outer
    dimensions. If specified, it must have `n - k` elements. If specified, this
    permutation is applied before combining any dimensions.

    Note, the unpack operation may drop any padding introduced by the pack
    operation and hence the following holds
    `NumElementsOf(source) >= NumElementsOf(result)`.

    Examples:

    ```mlir
    // NCnc to NC:
    %0 = linalg.unpack %source inner_dims_pos = [0, 1] inner_tiles = [8, 32]
        into %dest : tensor<16x8 x 8x32 xf32> -> tensor<128x256xf32>
    //                      \  /   \  /
    //          Outer Dims: 16x8  Inner Dims: 8x32

    // CK to KCck:
    %0 = linalg.unpack %source outer_dims_perm = [1, 0] inner_dims_pos = [0, 1]
        inner_tiles = [8, 32]
        into %dest : tensor<8x16 x 8x32 xf32> -> tensor<128x256xf32>
    //                      \  /   \  /
    //          Outer Dims: 8x16  Inner Dims: 8x32

    // CHW to CHWhw:
    %0 = linalg.unpack %source inner_dims_pos = [2, 1] inner_tiles = [4, 2]
        into %dest : tensor<3x10x6 x 4x2 xf32> -> tensor<3x20x24xf32>
    //                       \  /    \ /
    //          Outer Dims: 3x10x6  Inner Dims: 4x2

    // HCW to HCWhw
    %0 = linalg.unpack %source inner_dims_pos = [2, 0] inner_tiles = [4, 2]
        into %dest : tensor<9x3x8 x 4x2 xf32> -> tensor<18x3x32xf32>
    //                       \  /   \ /
    //          Outer Dims: 9x3x8   Inner Dims: 4x2
    ```
  }];
  let arguments = (ins AnyRankedTensor:$source,
                       AnyRankedTensor:$dest,
                       DefaultValuedOptionalAttr<DenseI64ArrayAttr, "{}">:$outer_dims_perm,
                       DenseI64ArrayAttr:$inner_dims_pos,
                       Variadic<Index>:$inner_tiles,
                       DenseI64ArrayAttr:$static_inner_tiles);
  let results = (outs AnyRankedTensor:$result);
  let assemblyFormat = [{
    $source
    (`outer_dims_perm` `=` $outer_dims_perm^)?
    `inner_dims_pos` `=` $inner_dims_pos
    `inner_tiles` `=`
    custom<DynamicIndexList>($inner_tiles, $static_inner_tiles)
    `into` $dest attr-dict `:` type($source) `->` type($dest)
  }];

  let builders = [
    OpBuilder<(ins "Value":$source, "Value":$dest,
    "ArrayRef<int64_t>":$innerDimsPos,
    "ArrayRef<OpFoldResult>":$innerTiles,
    CArg<"ArrayRef<int64_t>", "{}">:$outerDimsPerm)>
  ];

  let extraClassDeclaration = commonExtraClassDeclaration # [{
    static Value createDestinationTensor(OpBuilder &b, Location loc,
        Value source, ArrayRef<OpFoldResult> innerTileSizes,
        ArrayRef<int64_t> innerDimsPos, ArrayRef<int64_t> outerDimsPerm);

    /// Build and return a new UnPackOp that is a clone of the current UnPackOp
    /// with (innerDimsPos, innerTiles) (resp. outerDimsPerm) are permuted by
    /// innerPermutation (resp. outerPermutation).
    /// Asserts that:
    ///   - At least one of innerPermutation or outerPermutation is non-empty.
    ///   - If not empty, innerPermutation is a valid permutation of size
    ///     matching innerDimPos.
    ///   - If not empty, outerPermutation is a valid permutation of size
    ///     matching outerDimsPerm.
    UnPackOp createTransposedClone(OpBuilder &b,
                                   Location loc,
                                   Value transposedSource,
                                   ArrayRef<int64_t> innerPermutation,
                                   ArrayRef<int64_t> outerPermutation);

    /// Returns true if it is statically known that the `sliceOp` result shape
    /// is compatible with the `unPackOp`. I.e., it does not drop any tile.
    bool canFoldSliceOp(tensor::ExtractSliceOp sliceOp);

    /// Check if this UnPackOp is like a simple unpad operation.
    /// In other words, this operation:
    /// 1. drops useless dimensions (dimension of size 1), and
    /// 2. reduces dimensions in place (i.e., no transpose.)
    bool isLikeUnPad();
  }];

  let hasCanonicalizeMethod = 1;

  let hasFolder = 1;
}

#endif // LINALG_RELEAYOUT_OPS
