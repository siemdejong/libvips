# Zarr Data Types Support

This document describes the data type support in libvips zarrsave.

## Overview

The libvips Zarr implementation supports **12 out of 13** core Zarr v3 data types (92% coverage), including all integer, floating-point, and complex number types that VIPS can natively handle.

## Supported Data Types

### Unsigned Integers

| Type | Size | Range | VIPS Format | Zarr Type |
|------|------|-------|-------------|-----------|
| **uint8** | 8-bit | [0, 255] | `VIPS_FORMAT_UCHAR` | `uint8` |
| **uint16** | 16-bit | [0, 65535] | `VIPS_FORMAT_USHORT` | `uint16` |
| **uint32** | 32-bit | [0, 4294967295] | `VIPS_FORMAT_UINT` | `uint32` |
| **uint64*** | 64-bit | [0, 2^64-1] | N/A | `uint64` |

\* uint64 is supported in the Zarr layer but VIPS doesn't have a native 64-bit unsigned integer format

### Signed Integers

| Type | Size | Range | VIPS Format | Zarr Type |
|------|------|-------|-------------|-----------|
| **int8** | 8-bit | [-128, 127] | `VIPS_FORMAT_CHAR` | `int8` |
| **int16** | 16-bit | [-32768, 32767] | `VIPS_FORMAT_SHORT` | `int16` |
| **int32** | 32-bit | [-2147483648, 2147483647] | `VIPS_FORMAT_INT` | `int32` |
| **int64*** | 64-bit | [-2^63, 2^63-1] | N/A | `int64` |

\* int64 is supported in the Zarr layer but VIPS doesn't have a native 64-bit signed integer format

### Floating Point

| Type | Size | Precision | VIPS Format | Zarr Type |
|------|------|-----------|-------------|-----------|
| **float32** | 32-bit | Single (7 digits) | `VIPS_FORMAT_FLOAT` | `float32` |
| **float64** | 64-bit | Double (15 digits) | `VIPS_FORMAT_DOUBLE` | `float64` |

### Complex Numbers

| Type | Size | Components | VIPS Format | Zarr Type |
|------|------|------------|-------------|-----------|
| **complex64** | 64-bit | 2×float32 | `VIPS_FORMAT_COMPLEX` | `complex64` |
| **complex128** | 128-bit | 2×float64 | `VIPS_FORMAT_DPCOMPLEX` | `complex128` |

## Unsupported Types

| Type | Reason |
|------|--------|
| **bool** | VIPS doesn't have a native boolean format |
| **float16** | VIPS doesn't have half-precision float support |
| **r\*** | Raw bytes not applicable to image processing |

## Usage Examples

### Basic Type Conversion

```bash
# Create and save as different types
vips black test.v 256 256

# uint8 (default)
vips zarrsave test.v output_uint8.zarr

# uint16
vips cast test.v test_uint16.v ushort
vips zarrsave test_uint16.v output_uint16.zarr

# int8
vips cast test.v test_int8.v char
vips zarrsave test_int8.v output_int8.zarr

# int16
vips cast test.v test_int16.v short
vips zarrsave test_int16.v output_int16.zarr

# int32
vips cast test.v test_int32.v int
vips zarrsave test_int32.v output_int32.zarr

# uint32
vips cast test.v test_uint32.v uint
vips zarrsave test_uint32.v output_uint32.zarr

# float32
vips cast test.v test_float32.v float
vips zarrsave test_float32.v output_float32.zarr

# float64
vips cast test.v test_float64.v double
vips zarrsave test_float64.v output_float64.zarr

# complex64
vips cast test.v test_complex64.v complex
vips zarrsave test_complex64.v output_complex64.zarr

# complex128
vips cast test.v test_complex128.v dpcomplex
vips zarrsave test_complex128.v output_complex128.zarr
```

### Type-Specific Use Cases

#### Scientific Data (int16/int32)
Useful for microscopy, medical imaging, and scientific sensors:
```bash
# 16-bit microscopy data
vips zarrsave microscopy.v output.zarr
# Automatically preserves int16 if input is int16
```

#### High Dynamic Range (float32/float64)
For HDR images and scientific measurements:
```bash
vips cast hdr_image.v hdr_float.v float
vips zarrsave hdr_float.v output.zarr --compression zstd
```

#### Complex Analysis (complex64/complex128)
For Fourier transforms and signal processing:
```bash
# After FFT or complex operations
vips zarrsave frequency_domain.v output.zarr
# Preserves complex64 format
```

## Type Storage

### Memory Layout

All types use **little-endian** byte order in Zarr storage, regardless of the host system's endianness.

### Fill Values

Each data type has an appropriate default fill value:

| Type | Fill Value |
|------|-----------|
| Unsigned integers | `0` |
| Signed integers | `0` |
| Floating point | `0.0` |
| Complex | `0.0 + 0.0i` |

### Storage Efficiency

Typical sizes for a 1024×1024 single-band image:

| Type | Uncompressed | GZIP | ZSTD |
|------|--------------|------|------|
| uint8 | ~1 MB | ~varies | ~varies |
| uint16 | ~2 MB | ~varies | ~varies |
| uint32 | ~4 MB | ~varies | ~varies |
| int16 | ~2 MB | ~varies | ~varies |
| float32 | ~4 MB | ~varies | ~varies |
| float64 | ~8 MB | ~varies | ~varies |
| complex64 | ~8 MB | ~varies | ~varies |
| complex128 | ~16 MB | ~varies | ~varies |

*Actual compressed sizes depend heavily on image content*

## Type Preservation

When processing images through VIPS before saving to Zarr:

✅ **Preserved**: The data type is automatically detected and preserved  
⚠️ **Conversion**: Use `vips cast` to change types before saving  
❌ **Not Supported**: bool and float16 cannot be saved

## Metadata

Data types are stored in the Zarr metadata (`zarr.json`):

```json
{
  "zarr_format": 3,
  "node_type": "array",
  "shape": [1024, 1024, 1],
  "data_type": "int16",
  "chunk_grid": { ... },
  ...
}
```

## Compatibility

### Python zarr library
All supported types are fully compatible:
```python
import zarr
arr = zarr.open('output.zarr', mode='r')
print(arr.dtype)  # numpy dtype
data = arr[:]  # Read as numpy array
```

### NumPy Equivalents
| Zarr Type | NumPy dtype |
|-----------|-------------|
| uint8 | `uint8` |
| uint16 | `uint16` |
| uint32 | `uint32` |
| uint64 | `uint64` |
| int8 | `int8` |
| int16 | `int16` |
| int32 | `int32` |
| int64 | `int64` |
| float32 | `float32` |
| float64 | `float64` |
| complex64 | `complex64` |
| complex128 | `complex128` |

## Best Practices

1. **Choose the smallest type that fits your data**
   - Use uint8 for standard images (0-255)
   - Use int16 for scientific sensors
   - Use float32 for HDR (double precision usually unnecessary)

2. **Consider compression**
   - Integer types often compress better than float
   - Use zstd for best compression ratios

3. **Type conversions**
   - Always use `vips cast` for explicit conversions
   - Check value ranges before downcasting

4. **Complex numbers**
   - Rarely needed for standard images
   - Essential for frequency domain analysis
   - Use complex64 unless you need extreme precision

## Limitations

1. **No bool support**: Use uint8 with 0/1 values instead
2. **No float16**: Cast to float32 if needed
3. **No uint64/int64 in VIPS**: Can only be used via direct FFI calls
4. **No raw bytes**: Not applicable to image data

## Future Enhancements

Potential additions:
- Boolean type support (if VIPS adds native bool format)
- Half-precision float (float16) support
- Extended precision types
- Custom fill value specification

## Testing

Run the comprehensive type test:
```bash
./test_all_datatypes.sh
```

This tests all 10 natively-supported VIPS data types plus compression, chunking, and OME-Zarr combinations.

## See Also

- [ZARR_COMPRESSION.md](ZARR_COMPRESSION.md) - Compression options
- [CHUNKING_AND_SHARDING.md](CHUNKING_AND_SHARDING.md) - Chunking strategies
- [ZARR_V3_UNSUPPORTED_FEATURES.md](ZARR_V3_UNSUPPORTED_FEATURES.md) - Feature support status
- [Zarr v3 Data Types](https://zarr-specs.readthedocs.io/en/latest/v3/core/v3.0.html#data-types)
