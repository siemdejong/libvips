# Zarr Compression Options

This document describes the compression options available in libvips zarrsave.

## Supported Compression Codecs

### GZIP (default)
- **Codec**: gzip
- **Compression Level**: 1-9 (default: 5)
- **Characteristics**:
  - Good compression ratio
  - Wide compatibility (supported everywhere)
  - Moderate speed
  - Standard deflate algorithm
  - Level 1: Fastest, less compression
  - Level 5: Balanced (default)
  - Level 9: Best compression, slower

**Usage:**
```bash
# Use default level (5)
vips zarrsave input.jpg output.zarr --compression gzip

# Fast compression (level 1)
vips zarrsave input.jpg output.zarr --compression gzip --gzip-level 1

# Maximum compression (level 9)
vips zarrsave input.jpg output.zarr --compression gzip --gzip-level 9
```

### ZSTD (recommended for performance)
- **Codec**: zstd (Zstandard)
- **Compression Level**: 1-22 (default: 3)
- **Checksum**: Enabled
- **Characteristics**:
  - Better compression ratios than gzip
  - Faster decompression than gzip
  - Modern algorithm optimized for speed
  - Level 1-3: Very fast, good compression
  - Level 3: Balanced (default)
  - Level 10-15: Better compression, moderate speed
  - Level 22: Maximum compression, slowest

**Usage:**
```bash
# Use default level (3)
vips zarrsave input.jpg output.zarr --compression zstd

# Very fast compression (level 1)
vips zarrsave input.jpg output.zarr --compression zstd --zstd-level 1

# Better compression (level 10)
vips zarrsave input.jpg output.zarr --compression zstd --zstd-level 10

# Maximum compression (level 22)
vips zarrsave input.jpg output.zarr --compression zstd --zstd-level 22
```

## Compression Performance Comparison

### Example: 512×512×3 gradient image

| Compression     | Level | File Size | Compression Ratio | Relative Speed |
|-----------------|-------|-----------|-------------------|----------------|
| None            | -     | ~768 KB   | 1.0x              | Fastest        |
| GZIP            | 1     | 2.8 MB    | 0.27x             | Fast           |
| GZIP            | 5     | 1.2 MB    | 0.64x             | Moderate       |
| GZIP            | 9     | 1.2 MB    | 0.64x             | Slower         |
| ZSTD            | 1     | 588 KB    | 1.31x             | Very Fast      |
| ZSTD            | 3     | 556 KB    | 1.38x             | Fast           |
| ZSTD            | 10    | 508 KB    | 1.51x             | Moderate       |
| ZSTD            | 22    | 1.1 MB    | 0.70x             | Slowest        |

*Note: Actual results vary by image content. Higher compression levels may produce larger files for some data types due to overhead.*

### When to Use Each

**Use GZIP when:**
- Maximum compatibility is required
- Working with older Zarr implementations
- Integration with systems that may not support zstd

**Use ZSTD when:**
- Performance is important
- Working with modern Zarr libraries (zarr-python 2.x+, zarrs)
- Creating new datasets
- Cloud storage costs are a concern (smaller files)

## Compression with Chunking

Compression is applied per-chunk. Smaller chunks may compress less efficiently but allow more granular access.

```bash
# Chunked storage with zstd compression
vips zarrsave input.jpg output.zarr \
    --compression zstd \
    --chunk-height 256 \
    --chunk-width 256 \
    --chunk-bands 3
```

## Compression with Sharding

When using sharding, compression is applied to the inner chunks within each shard.

```bash
# Sharded storage with zstd compression
vips zarrsave input.jpg output.zarr \
    --compression zstd \
    --chunk-height 128 \
    --chunk-width 128 \
    --chunk-bands 3 \
    --shard-height 256 \
    --shard-width 256 \
    --shard-bands 3
```

## OME-Zarr with Compression

Compression works with OME-Zarr metadata:

```bash
# OME-Zarr with zstd
vips zarrsave input.tif output.zarr \
    --ome-zarr \
    --compression zstd
```

## Metadata

The compression codec is stored in the Zarr metadata (`zarr.json`):

### GZIP metadata:
```json
{
  "name": "gzip",
  "configuration": {
    "level": 5
  }
}
```

### ZSTD metadata:
```json
{
  "name": "zstd",
  "configuration": {
    "level": 3,
    "checksum": true
  }
}
```

## Compression Levels

Compression levels are now fully configurable:

### GZIP Levels (1-9)
- **Level 1**: Fastest compression, larger files (~2.8 MB for test image)
- **Level 5**: Balanced (default) (~1.2 MB)
- **Level 9**: Best compression, slowest (~1.2 MB)

### ZSTD Levels (1-22)
- **Level 1**: Very fast compression (~588 KB for test image)
- **Level 3**: Fast, good compression (default) (~556 KB)
- **Level 10**: Better compression, moderate speed (~508 KB)
- **Level 15-19**: High compression, slower
- **Level 22**: Maximum compression, slowest (~1.1 MB)*

*Note: Very high ZSTD levels (20-22) may produce larger files for some data due to overhead exceeding gains.

### Choosing a Level

**For fast writes (e.g., real-time processing):**
```bash
vips zarrsave input.jpg output.zarr --compression zstd --zstd-level 1
```

**For balanced performance (recommended):**
```bash
vips zarrsave input.jpg output.zarr --compression zstd  # Uses level 3 by default
```

**For maximum compression (archival):**
```bash
vips zarrsave input.jpg output.zarr --compression zstd --zstd-level 10
```

**For maximum compatibility:**
```bash
vips zarrsave input.jpg output.zarr --compression gzip  # Uses level 5 by default
```

## Compatibility

### GZIP
- ✅ Python zarr library (all versions)
- ✅ zarrs (Rust)
- ✅ tensorstore
- ✅ xtensor
- ✅ All Zarr v2 and v3 implementations

### ZSTD
- ✅ Python zarr library 2.13.0+
- ✅ zarrs (Rust) 0.22+
- ✅ tensorstore
- ⚠️ May require additional dependencies in some environments
- ⚠️ Not available in legacy Zarr v2-only implementations

## Best Practices

1. **Default to ZSTD level 3** for new projects - best balance of speed and compression
2. **Use GZIP** when maximum compatibility is needed
3. **Tune compression level** based on your use case:
   - **Fast writes**: zstd level 1 or gzip level 1
   - **Balanced**: zstd level 3 (default) or gzip level 5 (default)
   - **Archival/storage**: zstd level 10-15 or gzip level 9
4. **Test with your data** - optimal levels vary by content type
5. **Consider chunk size** - larger chunks typically compress better
6. **Monitor storage costs** - better compression can significantly reduce cloud storage costs
7. **Avoid extreme levels** - zstd 20-22 may be slower without better compression

## Future Improvements

Planned enhancements include:
- ✅ Configurable compression levels (implemented!)
- Additional codecs (blosc, lz4, etc.)
- Per-resolution compression in pyramids
- Compression quality hints based on data type

## See Also

- [CHUNKING_AND_SHARDING.md](CHUNKING_AND_SHARDING.md) - Chunking and sharding concepts
- [ZARR_V3_UNSUPPORTED_FEATURES.md](ZARR_V3_UNSUPPORTED_FEATURES.md) - Zarr v3 feature support status
- [Zarr v3 Specification](https://zarr-specs.readthedocs.io/en/latest/v3/core/index.html)
