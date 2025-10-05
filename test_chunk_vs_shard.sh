#!/bin/bash
# Test script for proper chunking vs sharding

set -e

echo "=== Testing Chunking vs Sharding ==="
echo

# Create test image
if [ ! -f test_512_rgb.v ]; then
    echo "Creating test image (512x512x3)..."
    ./build/tools/vips gaussnoise test_512.v 512 512
    ./build/tools/vips bandjoin "test_512.v test_512.v test_512.v" test_512_rgb.v
fi

# Clean up previous tests
rm -rf test_*.zarr

echo "=== Test 1: Default (no chunking, no sharding) ==="
./build/tools/vips copy test_512_rgb.v test_default.zarr
echo "✓ Created test_default.zarr"
echo "  Chunk shape from metadata:"
cat test_default.zarr/zarr.json | python3 -c "import sys, json; j=json.load(sys.stdin); print('  -', j['chunk_grid']['configuration']['chunk_shape'])"
echo "  Codecs:"
cat test_default.zarr/zarr.json | python3 -c "import sys, json; j=json.load(sys.stdin); print('  -', [c['name'] for c in j['codecs']])"
chunk_files=$(find test_default.zarr/c -type f 2>/dev/null | wc -l || echo "0")
echo "  Chunk files: $chunk_files"
echo

echo "=== Test 2: Chunking only (128x128x3 chunks, no sharding) ==="
./build/tools/vips copy test_512_rgb.v "test_chunks_only.zarr[chunk_height=128,chunk_width=128,chunk_bands=3]"
echo "✓ Created test_chunks_only.zarr"
echo "  Expected: 4x4x1 = 16 chunk files"
echo "  Chunk shape from metadata:"
cat test_chunks_only.zarr/zarr.json | python3 -c "import sys, json; j=json.load(sys.stdin); print('  -', j['chunk_grid']['configuration']['chunk_shape'])"
echo "  Codecs:"
cat test_chunks_only.zarr/zarr.json | python3 -c "import sys, json; j=json.load(sys.stdin); print('  -', [c['name'] for c in j['codecs']])"
chunk_files=$(find test_chunks_only.zarr/c -type f 2>/dev/null | wc -l || echo "0")
echo "  Actual chunk files: $chunk_files"
find test_chunks_only.zarr/c -type f 2>/dev/null | head -5
echo

echo "=== Test 3: Chunking + Sharding (64x64x3 chunks in 128x128x3 shards) ==="
./build/tools/vips copy test_512_rgb.v "test_chunks_and_shards.zarr[chunk_height=64,chunk_width=64,chunk_bands=3,shard_height=128,shard_width=128,shard_bands=3]"
echo "✓ Created test_chunks_and_shards.zarr"
echo "  Expected: 4x4x1 = 16 shard files, each containing 2x2x1 = 4 chunks"
echo "  Outer chunk (shard) shape from metadata:"
cat test_chunks_and_shards.zarr/zarr.json | python3 -c "import sys, json; j=json.load(sys.stdin); print('  -', j['chunk_grid']['configuration']['chunk_shape'])"
echo "  Codec and inner chunk shape:"
cat test_chunks_and_shards.zarr/zarr.json | python3 -c "import sys, json; j=json.load(sys.stdin); c=j['codecs'][0]; print('  - Codec:', c['name']); print('  - Inner chunk shape:', c['configuration']['chunk_shape'])"
shard_files=$(find test_chunks_and_shards.zarr/c -type f 2>/dev/null | wc -l || echo "0")
echo "  Actual shard files: $shard_files"
find test_chunks_and_shards.zarr/c -type f 2>/dev/null | head -5
echo

echo "=== Test 4: Different chunk sizes (256x256x3 chunks) ==="
./build/tools/vips copy test_512_rgb.v "test_large_chunks.zarr[chunk_height=256,chunk_width=256,chunk_bands=3]"
echo "✓ Created test_large_chunks.zarr"
echo "  Expected: 2x2x1 = 4 chunk files"
chunk_files=$(find test_large_chunks.zarr/c -type f 2>/dev/null | wc -l || echo "0")
echo "  Actual chunk files: $chunk_files"
echo

echo "=== Test 5: OME-NGFF with chunking ==="
./build/tools/vips copy test_512_rgb.v "test_ome_chunks.zarr[ome_ngff=1,chunk_height=128,chunk_width=128,chunk_bands=3]"
echo "✓ Created test_ome_chunks.zarr"
chunk_files=$(find test_ome_chunks.zarr/0/c -type f 2>/dev/null | wc -l || echo "0")
echo "  Chunk files in /0/: $chunk_files"
echo

echo "=== Test 6: OME-NGFF with chunking + sharding ==="
./build/tools/vips copy test_512_rgb.v "test_ome_chunks_shards.zarr[ome_ngff=1,chunk_height=64,chunk_width=64,chunk_bands=3,shard_height=256,shard_width=256,shard_bands=3]"
echo "✓ Created test_ome_chunks_shards.zarr"
echo "  Outer chunk (shard) shape:"
cat test_ome_chunks_shards.zarr/0/zarr.json | python3 -c "import sys, json; j=json.load(sys.stdin); print('  -', j['chunk_grid']['configuration']['chunk_shape'])"
echo "  Inner chunk shape:"
cat test_ome_chunks_shards.zarr/0/zarr.json | python3 -c "import sys, json; j=json.load(sys.stdin); c=j['codecs'][0]; print('  -', c['configuration']['chunk_shape'])"
shard_files=$(find test_ome_chunks_shards.zarr/0/c -type f 2>/dev/null | wc -l || echo "0")
echo "  Shard files in /0/: $shard_files"
echo

echo "=== Summary ==="
printf "%-30s %12s %12s %15s\n" "Configuration" "Chunk Files" "Total Files" "Size"
printf "%-30s %12s %12s %15s\n" "------------------------------" "------------" "------------" "---------------"
for dir in test_default.zarr test_chunks_only.zarr test_chunks_and_shards.zarr test_large_chunks.zarr test_ome_chunks.zarr test_ome_chunks_shards.zarr; do
    chunk_files=$(find $dir -type f -path "*/c/*" 2>/dev/null | wc -l || echo "0")
    total_files=$(find $dir -type f 2>/dev/null | wc -l || echo "0")
    size=$(du -sh $dir 2>/dev/null | cut -f1 || echo "N/A")
    printf "%-30s %12d %12d %15s\n" "$dir" "$chunk_files" "$total_files" "$size"
done
echo

echo "=== All Tests Completed! ==="
echo "Key differences demonstrated:"
echo "- Chunking: Divides array into logical chunks, each stored as a file"
echo "- Sharding: Groups multiple chunks into single shard files"
echo "- Sharding reduces file count while maintaining chunk-level access"
