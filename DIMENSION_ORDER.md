# Dimension Order Parameter

## Overview

The `dimension_order` parameter provides an intuitive string-based interface for specifying dimension reordering in Zarr arrays, replacing the need for numeric transpose indices.

## Usage

Instead of specifying numeric indices like `--transpose-order="2 0 1"`, users can now use dimension letters:

```bash
# Old way (still works for backward compatibility with numeric arrays)
vips zarrsave input.jpg output.zarr --transpose-order="2 0 1"

# New way (intuitive string-based)
vips zarrsave input.jpg output.zarr --dimension-order="cyx"
```

## Dimension Characters

### 3D Arrays (Standard Images)
- **y** = height
- **x** = width  
- **c** = channels/bands

Default canonical order: `yxc` (height, width, bands)

### 5D Arrays (OME-Zarr with time/depth)
- **t** = time
- **z** = depth
- **y** = height
- **x** = width
- **c** = channels/bands

Default canonical order: `tzyxc` (time, depth, height, width, bands)

## Examples

### 3D Arrays

```bash
# Default order (no transpose)
vips zarrsave input.jpg output.zarr

# Identity (explicitly specify default - no transpose codec added)
vips zarrsave input.jpg output.zarr --dimension-order="yxc"

# Bands first (common for analysis tools)
vips zarrsave input.jpg output.zarr --dimension-order="cyx"

# Swap X and Y
vips zarrsave input.jpg output.zarr --dimension-order="xyc"

# Width, bands, height
vips zarrsave input.jpg output.zarr --dimension-order="xcy"

# Height, bands, width
vips zarrsave input.jpg output.zarr --dimension-order="ycx"

# Bands, width, height
vips zarrsave input.jpg output.zarr --dimension-order="cxy"
```

### 5D Arrays

```bash
# 5D with depth dimension, bands first
vips zarrsave input.jpg output.zarr --depth=10 --dimension-order="ctzyx"

# 5D with time dimension, spatial dimensions first
vips zarrsave input.jpg output.zarr --time=5 --dimension-order="yxctz"

# 5D identity (no transpose)
vips zarrsave input.jpg output.zarr --depth=10 --dimension-order="tzyxc"
```

### Pyramids

```bash
# Pyramid with bands first (applies to all levels)
vips zarrsave input.jpg output.zarr --pyramid --dimension-order="cyx"

# OME-Zarr pyramid with custom order
vips zarrsave input.jpg output.zarr --pyramid --ome-zarr --dimension-order="cyx"
```

## Mapping Examples

### 3D (y, x, c)

| String | Transpose Indices | Meaning |
|--------|-------------------|---------|
| `yxc` | [0, 1, 2] | Identity (no transpose) |
| `cyx` | [2, 0, 1] | Bands, height, width |
| `xyc` | [1, 0, 2] | Width, height, bands |
| `xcy` | [1, 2, 0] | Width, bands, height |
| `ycx` | [0, 2, 1] | Height, bands, width |
| `cxy` | [2, 1, 0] | Bands, width, height |

### 5D (t, z, y, x, c)

| String | Transpose Indices | Meaning |
|--------|-------------------|---------|
| `tzyxc` | [0, 1, 2, 3, 4] | Identity (no transpose) |
| `ctzyx` | [4, 0, 1, 2, 3] | Bands first |
| `yxctz` | [2, 3, 4, 0, 1] | Spatial first, then time/depth |
| `zyxct` | [1, 2, 3, 4, 0] | Depth first, time last |

## Features

✅ **Case insensitive**: `"CYX"`, `"cyx"`, and `"CyX"` all work  
✅ **Validation**: Checks for valid characters, correct length, and no duplicates  
✅ **Identity optimization**: If order is identity, no transpose codec is added  
✅ **Error messages**: Clear, descriptive errors for invalid inputs  
✅ **Pyramid support**: Applies same order to all pyramid levels  
✅ **5D support**: Automatically detects when time or depth dimensions are present  

## Error Handling

### Invalid Characters
```bash
$ vips zarrsave input.jpg output.zarr --dimension-order="abc"
Error: Invalid dimension character 'a' in 'abc'. Valid characters for 3D: yxc
```

### Wrong Length
```bash
$ vips zarrsave input.jpg output.zarr --dimension-order="yx"
Error: Dimension order 'yx' must have 3 characters for 3D arrays
```

### Duplicate Characters
```bash
$ vips zarrsave input.jpg output.zarr --dimension-order="ycc"
Error: Duplicate dimension character 'c' in 'ycc'
```

## Implementation Details

- **Rust layer**: Parses string and converts to transpose indices
- **C layer**: Passes string directly to Rust FFI
- **Optimization**: Identity permutations don't create transpose codec
- **Zarr v3 spec compliant**: Generates proper transpose codec configuration

## Backward Compatibility

The old numeric `transpose_order` parameter has been replaced entirely by the more intuitive `dimension_order` string parameter. Users should migrate to using dimension strings:

**Before:**
```bash
vips zarrsave input.jpg output.zarr --transpose-order="2 0 1"
```

**After:**
```bash
vips zarrsave input.jpg output.zarr --dimension-order="cyx"
```

## Performance

- No performance overhead - string parsing happens once at array creation
- Identity permutations are detected and skip transpose codec entirely
- Same underlying zarrs transpose codec implementation

## Related

- [Zarr v3 Transpose Codec Spec](https://zarr-specs.readthedocs.io/en/latest/v3/codecs/transpose/index.html)
- `TRANSPOSE_CODEC_IMPLEMENTATION.md` - Original numeric implementation
- `test_dimension_order.sh` - Comprehensive test suite
