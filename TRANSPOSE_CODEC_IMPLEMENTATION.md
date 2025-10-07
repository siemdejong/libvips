# Transpose Codec Implementation

## Summary

The [Zarr v3 transpose codec](https://zarr-specs.readthedocs.io/en/latest/v3/codecs/transpose/index.html) has been successfully implemented in libvips. This codec allows dimension reordering in Zarr arrays.

## Implementation Details

### Files Modified

1. **rust/zarrs_wrapper/Cargo.toml**
   - Added `"transpose"` to the features list to enable the transpose codec from the zarrs crate

2. **rust/zarrs_wrapper/src/lib.rs**
   - Imported `TransposeCodec` and `TransposeOrder` from zarrs
   - Added `transpose_order` and `transpose_len` parameters to:
     - `vips_zarr_write_array()`
     - `vips_zarr_init_array()`
   - Implemented codec creation: `TransposeCodec::new(TransposeOrder::new(order)?)`
   - Integrated into array builder chain with `.array_to_array_codecs(vec![tc])`
   - Applied to both streaming (pyramid) and non-streaming writes

3. **rust/zarrs_wrapper.h**
   - Updated FFI function declarations with transpose parameters

4. **libvips/foreign/zarrsave.c**
   - Added `VipsArrayInt *transpose_order` field to `VipsForeignSaveZarr` struct
   - Added `VIPS_ARG_BOXED` parameter (ID 37) for `transpose_order`
   - Extracted array values and passed to Rust FFI layer
   - Updated both `vips_zarr_write_array()` and `vips_zarr_init_array()` calls

5. **ZARR_V3_UNSUPPORTED_FEATURES.md**
   - Marked transpose codec as ✅ Supported
   - Updated statistics: 1/6 array-to-array codecs now supported

## Usage

### Command Line

```bash
# Basic transpose
vips zarrsave input.jpg output.zarr --transpose-order="2 0 1"

# With pyramid (OME-Zarr)
vips zarrsave input.jpg output.zarr --pyramid --transpose-order="2 0 1"

# Different permutations
vips zarrsave input.jpg output.zarr --transpose-order="0 2 1"
```

### C API

```c
#include <vips/vips.h>

VipsImage *image;
VipsArrayInt *transpose = vips_array_int_newv(3, 2, 0, 1);

vips_zarrsave(image, "output.zarr", 
    "transpose_order", transpose, 
    NULL);
```

### Rust FFI

```c
const int32_t order[] = {2, 0, 1};
vips_zarr_write_array(
    /* ... other params ... */
    order, 3,  // transpose_order and length
    /* ... */
);
```

## Verification

The implementation has been tested with:

1. ✅ Basic single-level zarr saves
2. ✅ Pyramid (OME-Zarr) multi-resolution saves
3. ✅ Different permutation orders
4. ✅ Default behavior (no transpose when not specified)
5. ✅ Correct codec chain ordering (transpose before bytes codec)

All tests pass successfully. See `test_transpose_complete.sh` for the test suite.

## Codec Chain

The transpose codec is correctly placed in the codec chain:
1. **transpose** - dimension reordering
2. **bytes** - endianness handling
3. **gzip/zstd/blosc** - compression

This matches the Zarr v3 specification where array-to-array codecs come before array-to-bytes codecs.

## Example Output

```json
{
  "codecs": [
    {
      "name": "transpose",
      "configuration": {
        "order": [2, 0, 1]
      }
    },
    {
      "name": "bytes",
      "configuration": {
        "endian": "little"
      }
    },
    {
      "name": "gzip",
      "configuration": {
        "level": 5
      }
    }
  ]
}
```

## Pyramid Support

The transpose codec is applied to **all levels** of an OME-Zarr pyramid. When `--pyramid` is used with `--transpose-order`, every resolution level (0, 1, 2, etc.) will have the same transpose codec configuration.

## Notes

- The transpose order must be a valid permutation of dimensions
- For 3D images (y, x, c): common order is `[2, 0, 1]` to reorder to (c, y, x)
- The zarrs library validates the permutation automatically
- Empty/NULL transpose_order means no transpose codec is added (default behavior)
