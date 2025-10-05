#!/bin/bash
# Test zstd compression in zarrsave

set -e

# Create a simple test image
echo "Creating test image..."
vips black test.v 512 512 --bands 3

# Test 1: Save with default gzip compression
echo "Test 1: Saving with gzip compression..."
vips zarrsave test.v test_gzip.zarr --compression gzip
echo "Gzip zarr size:"
du -sh test_gzip.zarr

# Test 2: Save with zstd compression
echo ""
echo "Test 2: Saving with zstd compression..."
vips zarrsave test.v test_zstd.zarr --compression zstd
echo "Zstd zarr size:"
du -sh test_zstd.zarr

# Test 3: Save with zstd and chunking
echo ""
echo "Test 3: Saving with zstd compression and chunking..."
vips zarrsave test.v test_zstd_chunked.zarr \
    --compression zstd \
    --chunk-height 128 \
    --chunk-width 128 \
    --chunk-bands 3
echo "Zstd chunked zarr size:"
du -sh test_zstd_chunked.zarr
echo "Number of chunk files:"
find test_zstd_chunked.zarr -name "c*" | wc -l

# Test 4: Save with zstd, chunking, and sharding
echo ""
echo "Test 4: Saving with zstd compression, chunking, and sharding..."
vips zarrsave test.v test_zstd_sharded.zarr \
    --compression zstd \
    --chunk-height 64 \
    --chunk-width 64 \
    --chunk-bands 3 \
    --shard-height 128 \
    --shard-width 128 \
    --shard-bands 3
echo "Zstd sharded zarr size:"
du -sh test_zstd_sharded.zarr
echo "Number of shard files:"
find test_zstd_sharded.zarr -name "c*" | wc -l

# Test 5: OME-Zarr with zstd
echo ""
echo "Test 5: Saving OME-Zarr with zstd compression..."
vips zarrsave test.v test_ome_zstd.zarr --ome-zarr --compression zstd
echo "OME-Zarr with zstd size:"
du -sh test_ome_zstd.zarr

# Check metadata to verify codec
echo ""
echo "Checking metadata for zstd codec..."
if command -v python3 &> /dev/null; then
    python3 << 'EOF'
import json
import sys

try:
    with open('test_zstd.zarr/zarr.json', 'r') as f:
        metadata = json.load(f)
    
    codecs = metadata.get('attributes', {}).get('codecs', [])
    print("Codecs in metadata:")
    print(json.dumps(codecs, indent=2))
    
    # For non-OME zarr, check the array metadata
    with open('test_zstd.zarr/0/zarr.json' if 'test_zstd.zarr/0/zarr.json' else 'test_zstd.zarr/zarr.json', 'r') as f:
        array_metadata = json.load(f)
    
    print("\nArray codecs:")
    print(json.dumps(array_metadata.get('codecs', []), indent=2))
except FileNotFoundError:
    print("Metadata file not found, checking root...")
    with open('test_zstd.zarr/zarr.json', 'r') as f:
        metadata = json.load(f)
    print(json.dumps(metadata, indent=2))
EOF
else
    echo "Python3 not available, skipping metadata check"
    cat test_zstd.zarr/zarr.json | head -20
fi

# Cleanup
echo ""
echo "Cleaning up..."
rm -rf test.v test_gzip.zarr test_zstd.zarr test_zstd_chunked.zarr test_zstd_sharded.zarr test_ome_zstd.zarr

echo ""
echo "All tests completed successfully!"
