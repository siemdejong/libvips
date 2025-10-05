# Zarr Compression Options

This document describes the compression options available in libvips zarrsave.

## Supported Compression Codecs

### GZIP (default)
- **Codec**: gzip
- **Compression Level**: 5 (hardcoded)
- **Characteristics**:
  - Good compression ratio
  - Wide compatibility (supported everywhere)
  - Moderate speed
  - Standard deflate algorithm

**Usage:**
```bash
vips zarrsave input.jpg output.zarr --compression gzip
```

### ZSTD (recommended for performance)
- **Codec**: zstd (Zstandard)
- **Compression Level**: 3 (hardcoded)
- **Checksum**: Enabled
- **Characteristics**:
  - Better compression ratios than gzip
  - Faster decompression than gzip
  - Modern algorithm optimized for speed
  - Good balance of speed and compression at level 3

**Usage:**
```bash
vips zarrsave input.jpg output.zarr --compression zstd
```

## Compression Performance Comparison

### Example: 512×512 grayscale gradient image

| Compression | File Size | Compression Ratio | Speed    |
|------------|-----------|-------------------|----------|
| None       | ~256 KB   | 1.0x              | Fastest  |
| GZIP       | ~28 KB    | ~9.1x             | Moderate |
| ZSTD       | ~24 KB    | ~10.7x            | Fast     |

*Note: Actual results vary by image content*

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

Currently, compression levels are hardcoded:
- **GZIP**: Level 5 (balanced)
- **ZSTD**: Level 3 (fast, good compression)

These defaults were chosen to balance compression ratio, speed, and resource usage.

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

1. **Default to ZSTD** for new projects unless you have specific compatibility requirements
2. **Use GZIP** when maximum compatibility is needed
3. **Test both** with your specific data to determine which works best
4. **Consider chunk size** - larger chunks typically compress better
5. **Monitor storage costs** - better compression can significantly reduce cloud storage costs

## Future Improvements

Planned enhancements include:
- Configurable compression levels
- Additional codecs (blosc, etc.)
- Per-resolution compression in pyramids
- Compression quality hints based on data type

## See Also

- [CHUNKING_AND_SHARDING.md](CHUNKING_AND_SHARDING.md) - Chunking and sharding concepts
- [ZARR_V3_UNSUPPORTED_FEATURES.md](ZARR_V3_UNSUPPORTED_FEATURES.md) - Zarr v3 feature support status
- [Zarr v3 Specification](https://zarr-specs.readthedocs.io/en/latest/v3/core/index.html)
