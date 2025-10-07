# Zarr v3.1 Specification Compliance - Unsupported Features

This document lists all features from the [Zarr v3.1 Core Specification](https://zarr-specs.readthedocs.io/en/latest/v3/core/index.html) that are **not yet supported** by the libvips Zarr implementation.

**Last Updated:** October 5, 2025  
**Zarr Spec Version:** 3.1  
**libvips Branch:** zarr

---

## 1. Data Types

### Supported
- ✅ `uint8` - 8-bit unsigned integer
- ✅ `uint16` - 16-bit unsigned integer  
- ✅ `uint32` - 32-bit unsigned integer
- ✅ `uint64` - 64-bit unsigned integer (via type code 8)*
- ✅ `int8` - 8-bit signed integer
- ✅ `int16` - 16-bit signed integer
- ✅ `int32` - 32-bit signed integer
- ✅ `int64` - 64-bit signed integer (via type code 9)*
- ✅ `float32` - 32-bit floating point
- ✅ `float64` - 64-bit floating point
- ✅ `complex64` - Complex number (64-bit: 2×32-bit float)
- ✅ `complex128` - Complex number (128-bit: 2×64-bit float)

*Note: uint64 and int64 are supported in the Rust/Zarr layer but VIPS doesn't have native 64-bit integer image formats, so these can only be used via direct FFI calls, not through standard VIPS operations.

### Not Supported
- ❌ `bool` - Boolean (1 byte)
- ❌ `float16` - 16-bit floating point
- ❌ `r*` - Raw/opaque bytes (e.g., `r8`, `r16`, `r24`)
- ❌ Extension data types (custom/user-defined types)

## 2. Chunk Grids

### Supported
- ✅ `regular` - Regular rectangular chunk grid with fixed chunk shape

### Not Supported
- ❌ Rectilinear grids (chunks with varying sizes)
- ❌ Extension chunk grids
- ❌ Irregular/adaptive grids

---

## 3. Chunk Key Encodings

### Supported
- ✅ `default` - Default encoding with configurable separator (currently hardcoded to `/`)

### Not Supported
- ❌ Configurable separators (currently fixed to `/`)
- ❌ `v2` - Zarr v2 compatible key encoding
- ❌ Extension chunk key encodings

---

## 4. Codecs

### Array → Array Codecs

#### Supported
- ✅ `transpose` - Array dimension reordering

#### Not Supported
- ❌ Extension array→array codecs

### Array → Bytes Codecs

#### Supported
- ✅ `bytes` - Converts array to bytes with configurable endianness (little, big, native)

#### Not Supported
- ❌ `sharding_indexed` as standalone array→bytes codec (only used internally when sharding is enabled)

### Bytes → Bytes Codecs

#### Supported
- ✅ `gzip` - Gzip compression with configurable level (1-9, default 5)
- ✅ `zstd` - Zstd compression with configurable level (1-22, default 3)
- ✅ `blosc` - Blosc compression with 6 compressor variants (LZ4, LZ4HC, BloscLZ, Zstd, Snappy, Zlib)

#### Not Supported
- ❌ `crc32c` - CRC32C checksum
- ❌ Extension codecs (custom compression/encoding)

### Codec Features

#### Supported
- ✅ Configurable endianness for bytes codec (little, big, native)
- ✅ Multiple bytes→bytes codecs via sharding inner codecs
- ✅ Codec configuration parameters (compression levels, blosc options, endianness)

#### Not Supported
- ❌ Multiple array→array codecs (currently supports at most one transpose codec)
- ❌ Custom codec chains beyond the current pattern
- ❌ `must_understand=false` for optional codecs
- ❌ Partial decoding for codecs (except via sharding)

---

## 5. Fill Values

### Supported
- ✅ Numeric fill values for supported data types (0 for integers, NaN for floats)

### Not Supported
- ❌ Custom fill values (currently hardcoded: 0 for integers, NaN for floats)
- ❌ `null` as fill value
- ❌ Special values beyond NaN for floats
- ❌ Fill values for unsupported data types

---

## 6. Metadata

### Array Metadata - Supported
- ✅ `zarr_format` (always 3)
- ✅ `node_type` (always "array")
- ✅ `shape`
- ✅ `data_type` (limited to 5 core types)
- ✅ `chunk_grid` (regular grid only)
- ✅ `chunk_key_encoding` (default with `/` separator)
- ✅ `fill_value` (hardcoded defaults)
- ✅ `codecs` (limited codec support)

### Array Metadata - Not Supported
- ❌ `attributes` - Custom user attributes
- ❌ `storage_transformers` - Storage transformation layer
- ❌ `dimension_names` - Named dimensions
- ❌ Unknown keys with `must_understand=false`

### Group Metadata - Not Supported
- ❌ **Entire group support** - libvips only creates arrays, not groups
- ❌ Group creation
- ❌ Group attributes
- ❌ Hierarchical structures beyond root array

---

## 7. Storage Features

### Store Interface - Supported
- ✅ `set` - Write key/value pairs
- ✅ Basic filesystem store

### Store Interface - Not Supported
- ❌ `get` - Read operations (write-only implementation)
- ❌ `get_partial_values` - Partial read operations
- ❌ `set_partial_values` - Partial write operations
- ❌ `erase` - Delete operations
- ❌ `erase_values` - Batch delete operations
- ❌ `erase_prefix` - Prefix-based deletion
- ❌ `list` - List all keys
- ❌ `list_prefix` - List keys with prefix
- ❌ `list_dir` - Directory listing

### Store Types - Not Supported
- ❌ S3 / object storage
- ❌ HTTP remote stores
- ❌ Zip file stores
- ❌ Database stores
- ❌ Memory stores
- ❌ Custom store implementations

---

## 8. Storage Transformers

### Not Supported
- ❌ **All storage transformers** - None implemented
- ❌ Caching transformers
- ❌ Encryption transformers
- ❌ Logging/monitoring transformers
- ❌ Custom transformers
- ❌ Transformer stacking

---

## 9. Hierarchy and Node Features

### Not Supported
- ❌ **Groups** - Only arrays at root level
- ❌ Hierarchical paths (e.g., `/foo/bar`)
- ❌ Non-root arrays (e.g., `/foo/array`)
- ❌ Child nodes
- ❌ Node discovery
- ❌ Tree structures
- ❌ Implicit groups

---

## 10. Array Operations

### Supported
- ✅ Create array (write-only)
- ✅ Write chunks

### Not Supported
- ❌ **Read operations** - Cannot read back Zarr arrays
- ❌ Retrieve chunks
- ❌ Partial chunk reads
- ❌ Partial chunk writes
- ❌ Array resizing
- ❌ Array deletion
- ❌ Array updates
- ❌ Append operations

---

## 11. Advanced Features

### Chunking/Sharding - Supported
- ✅ Regular chunk grids
- ✅ Custom chunk shapes
- ✅ Sharding with inner chunks
- ✅ Shard files with index

### Chunking/Sharding - Not Supported
- ❌ Variable chunk shapes
- ❌ Rectilinear chunks
- ❌ Configurable shard index codec (currently crc32c only)
- ❌ Configurable shard index location (currently "end" only)

### OME-NGFF Features - Supported
- ✅ Basic OME-NGFF v0.5 metadata structure
- ✅ `multiscales` with single resolution
- ✅ Axes definitions (c, y, x)
- ✅ Coordinate transformations (scale)

### OME-NGFF Features - Not Supported
- ❌ Multi-resolution pyramids (only single resolution)
- ❌ Multiple datasets in multiscales
- ❌ Translation transformations
- ❌ Complex coordinate systems
- ❌ 5D data (t, z dimensions)
- ❌ Custom axes types
- ❌ Physical units beyond micrometers
- ❌ OME-NGFF plate/well structures
- ❌ Labels (segmentation masks)
- ❌ Tables
- ❌ Points/ROIs

---

## 12. Extension System

### Not Supported
- ❌ **All extension mechanisms**
- ❌ Extension registration
- ❌ Extension discovery
- ❌ Custom extension points
- ❌ Extension versioning
- ❌ `must_understand` field handling
- ❌ Extension configuration objects
- ❌ URI-based extension names

---

## 13. Dimension Features

### Not Supported
- ❌ Dimension names
- ❌ Infinite dimensions
- ❌ Variable-length dimensions
- ❌ Non-finite dimension lengths

---

## 14. Encoding Options

### Not Supported
- ❌ Big-endian encoding
- ❌ Configurable byte order
- ❌ Alternative memory layouts
- ❌ Fortran (column-major) order
- ❌ Custom dimension ordering

---

## 15. Data Organization

### Current Limitations
- **Shape**: Limited to 3D (height, width, bands)
- **Dimension Order**: Fixed to [height, width, bands]
- **Band Interleaving**: Fixed to interleaved (band, y, x order not supported)

### Not Supported
- ❌ 1D or 2D arrays (requires 3D with bands)
- ❌ 4D+ arrays (time, depth, etc.)
- ❌ Planar (non-interleaved) band storage
- ❌ Custom dimension ordering
- ❌ Scalar (0D) arrays

---

## 16. Metadata Flexibility

### Not Supported
- ❌ Custom attributes in array metadata
- ❌ User-defined metadata fields
- ❌ Arbitrary JSON in attributes
- ❌ Metadata-only operations
- ❌ Metadata updates without data writes

---

## 17. Concurrent Access

### Not Supported
- ❌ Concurrent reads
- ❌ Concurrent writes
- ❌ Locking mechanisms
- ❌ Atomic operations
- ❌ Transaction support

---

## 18. Performance Features

### Not Supported
- ❌ Lazy loading
- ❌ Memory-mapped I/O
- ❌ Streaming writes
- ❌ Background compression
- ❌ Parallel chunk writing
- ❌ Prefetching
- ❌ Caching strategies

---

## 19. Validation and Error Handling

### Not Supported
- ❌ Metadata validation
- ❌ Data integrity checks (beyond sharding CRC32C)
- ❌ Checksum verification
- ❌ Partial write recovery
- ❌ Error correction codes

---

## 20. Interoperability

### Not Supported
- ❌ Zarr v2 compatibility mode
- ❌ N5 format compatibility
- ❌ Import from other formats
- ❌ Export to other Zarr implementations
- ❌ Round-trip testing with other libraries

---

## Summary Statistics

**Supported Features:**
- Core data types: 12 of 13 (92%)
- Codecs: 4 of 10+ (40%)
  - transpose (dimension reordering)
  - gzip with configurable levels (1-9)
  - zstd with configurable levels (1-22)
  - bytes codec
- Operations: Write-only (0% read support)
- Metadata: Basic required fields only

**Major Missing Categories:**
- ❌ Read operations (0%)
- ❌ Groups and hierarchies (0%)
- ❌ Storage transformers (0%)
- ❌ Extensions system (0%)
- ❌ Most codecs (60%)
- ❌ Few data types (8%)
- ❌ Advanced OME-NGFF features (70%)

**Implementation Status:**
- **Basic Write**: ✅ Functional
- **Basic Read**: ❌ Not implemented
- **Advanced Features**: ❌ Mostly not implemented
- **Spec Compliance**: ~40-45% of full v3.1 specification

---

## Notes

1. **Write-Only**: The current implementation is write-only. Reading Zarr arrays back into libvips is not supported.

2. **Configurable Compression**: Gzip (levels 1-9, default 5) and zstd (levels 1-22, default 3) compression levels are now fully configurable via `--gzip-level` and `--zstd-level` parameters.

3. **Transpose Codec**: The transpose codec is implemented at the Rust/FFI layer and supports dimension reordering for both 3D and 5D arrays. It validates that the order parameter is a valid permutation of dimension indices. Currently exposed via Rust API only; C API integration is optional.

4. **Limited Data Types**: 12 of 13 core data types are supported, matching common libvips image formats. Bool, float16, and raw bytes are not supported.

5. **No Groups**: Only single root-level arrays are supported. No hierarchical structures.

6. **No Extensions**: The extension system is not implemented, limiting expandability.

7. **Single Resolution**: OME-NGFF support is limited to single-resolution arrays, not multi-resolution pyramids.

8. **Filesystem Only**: Only local filesystem storage is supported. Cloud storage (S3, GCS, Azure) requires external tools.

---

## Future Work Priorities

Based on the spec analysis, high-priority additions would be:

1. **Read Operations** - Essential for round-trip support
2. **Boolean and float16 types** - Complete data type coverage
3. **Additional Codecs** - blosc, lz4, crc32c for checksums
4. **Transpose C API Integration** - Expose transpose codec through vips command-line tool (optional)
5. **Multi-Resolution Pyramids** - Full OME-NGFF support
6. **Groups** - Hierarchical organization
7. **Cloud Storage** - S3/HTTP support

---

**Document Version:** 1.0  
**Specification Reference:** https://zarr-specs.readthedocs.io/en/latest/v3/core/index.html
