# Zarr Chunking and Sharding Implementation

## Overview

This implementation provides proper support for both **chunking** and **sharding** in Zarr v3 format, correctly distinguishing between these two concepts.

## Concepts

### Chunking
- **Purpose**: Divides arrays into regular blocks for efficient I/O
- **Storage**: Each chunk is stored as a separate file
- **Access**: Enables reading/writing specific regions without loading entire array
- **Example**: 512×512×3 image with 128×128×3 chunks = 16 separate chunk files

### Sharding  
- **Purpose**: Groups multiple logical chunks into larger physical files
- **Storage**: Multiple chunks stored together in a single "shard" file with an index
- **Access**: Maintains chunk-level access while reducing file count
- **Benefit**: Improves cloud storage performance (fewer HTTP requests, better caching)
- **Example**: 64×64×3 chunks grouped into 128×128×3 shards = 4 chunks per shard file

## Parameters

### Chunk Parameters
- `chunk_height`: Height of each chunk (0 = full image height)
- `chunk_width`: Width of each chunk (0 = full image width)  
- `chunk_bands`: Bands per chunk (0 = all bands)

### Shard Parameters
- `shard_height`: Height of each shard (0 = no sharding)
- `shard_width`: Width of each shard (0 = no sharding)
- `shard_bands`: Bands per shard (0 = no sharding)

**Important**: When using sharding, shard dimensions should be multiples of chunk dimensions.

## Usage Examples

### 1. Default (No chunking, no sharding)
```bash
vips copy input.tif output.zarr
```
- Result: Single file containing entire image
- Use case: Small images, simple storage

### 2. Chunking Only
```bash
vips copy input.tif output.zarr[chunk_height=128,chunk_width=128,chunk_bands=3]
```
- Result: Multiple chunk files (e.g., 16 files for 512×512 image)
- Use case: Enable partial reads, parallel processing

### 3. Chunking + Sharding
```bash
vips copy input.tif output.zarr[chunk_height=64,chunk_width=64,chunk_bands=3,shard_height=256,shard_width=256,shard_bands=3]
```
- Result: Fewer shard files, each containing multiple chunks
- Use case: Cloud storage optimization, reduce HTTP requests

### 4. OME-Zarr with Chunking
```bash
vips copy input.tif output.zarr[ome_zarr=1,chunk_height=128,chunk_width=128,chunk_bands=3]
```
- Result: OME-Zarr compatible with chunked storage
- Use case: Microscopy data with metadata

### 5. OME-Zarr with Chunking + Sharding
```bash
vips copy input.tif output.zarr[ome_zarr=1,chunk_height=64,chunk_width=64,chunk_bands=3,shard_height=256,shard_width=256,shard_bands=3]
```
- Result: OME-Zarr with optimized cloud storage
- Use case: Large microscopy datasets on cloud platforms

## Configuration Examples

### Small Image (256×256×3)
```bash
# Basic chunking
chunk_height=64,chunk_width=64,chunk_bands=3  # 16 chunk files

# With sharding
chunk_height=32,chunk_width=32,chunk_bands=3,shard_height=64,shard_width=64,shard_bands=3  # 16 shards, 4 chunks each
```

### Medium Image (1024×1024×3)
```bash
# Basic chunking
chunk_height=256,chunk_width=256,chunk_bands=3  # 16 chunk files

# With sharding
chunk_height=128,chunk_width=128,chunk_bands=3,shard_height=256,shard_width=256,shard_bands=3  # 16 shards, 4 chunks each
```

### Large Image (4096×4096×3)
```bash
# Basic chunking
chunk_height=512,chunk_width=512,chunk_bands=3  # 64 chunk files

# With sharding
chunk_height=256,chunk_width=256,chunk_bands=3,shard_height=1024,shard_width=1024,shard_bands=3  # 16 shards, 16 chunks each
```

## Metadata Structure

### Chunks Only
```json
{
  "chunk_grid": {
    "configuration": {
      "chunk_shape": [128, 128, 3]
    }
  },
  "codecs": [
    {"name": "bytes"},
    {"name": "gzip", "configuration": {"level": 5}}
  ]
}
```

### Chunks + Shards
```json
{
  "chunk_grid": {
    "configuration": {
      "chunk_shape": [256, 256, 3]  // Shard boundaries
    }
  },
  "codecs": [
    {
      "name": "sharding_indexed",
      "configuration": {
        "chunk_shape": [64, 64, 3],  // Inner chunks
        "codecs": [
          {"name": "bytes"},
          {"name": "gzip", "configuration": {"level": 5}}
        ],
        "index_codecs": [
          {"name": "bytes"},
          {"name": "crc32c"}
        ],
        "index_location": "end"
      }
    }
  ]
}
```

## Performance Considerations

### When to Use Chunking
- Large arrays that don't fit in memory
- Need for partial reads/writes
- Parallel processing workflows
- Random access patterns

### When to Use Sharding
- Cloud storage (S3, GCS, Azure Blob)
- Many small chunks (100s or 1000s)
- High latency storage
- Want to reduce metadata overhead

### Recommended Chunk Sizes
- **Small chunks (64-128)**: Fine-grained access, more files
- **Medium chunks (256-512)**: Balanced performance
- **Large chunks (1024+)**: Fewer files, coarser access

### Recommended Shard/Chunk Ratios
- **2:1 or 4:1 ratio**: Shard contains 4-16 chunks
- **Example**: 64×64 chunks in 128×128 shards (4 chunks per shard)
- **Example**: 128×128 chunks in 256×256 shards (4 chunks per shard)

## Implementation Details

### Validation
- Shard dimensions should be multiples of chunk dimensions
- Warnings are emitted if this constraint is violated
- If chunks are not specified, they default to full image dimensions

### Defaults
- `chunk_*=0`: Use full image dimensions (single chunk)
- `shard_*=0`: No sharding (chunks stored as individual files)

### OME-Zarr Mode
- Chunking and sharding work seamlessly with OME-Zarr metadata
- Array stored in `/0/` subdirectory
- Group metadata includes OME-Zarr v0.5 specification
- All chunking/sharding features available

## Testing

See `test_chunk_vs_shard.sh` for comprehensive test cases demonstrating:
1. Default configuration
2. Chunking only
3. Chunking + sharding
4. Different chunk sizes
5. OME-Zarr with chunking
6. OME-Zarr with chunking + sharding

Run tests with:
```bash
./test_chunk_vs_shard.sh
```
