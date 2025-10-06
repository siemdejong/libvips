#!/bin/bash
# Test zarrsave streaming with memory monitoring

set -e

cd /home/siemdejong/libvips

echo "=== Testing Zarr Streaming (no memory spike expected) ==="
echo

# Create a test image (smaller for faster testing)
echo "1. Creating test image (1024×1024×3)..."
./build/tools/vips black test_image_tmp.v 1024 1024 --bands 3
./build/tools/vips invert test_image_tmp.v test_image.v
rm test_image_tmp.v

# Test basic save
echo "2. Testing basic zarrsave..."
rm -rf test_basic.zarr
/usr/bin/time -v ./build/tools/vips zarrsave test_image.v test_basic.zarr 2>&1 | grep "Maximum resident"

# Test with chunking
echo "3. Testing zarrsave with chunking (128×128×3)..."
rm -rf test_chunked.zarr
/usr/bin/time -v ./build/tools/vips zarrsave test_image.v test_chunked.zarr \
    --chunk-height 128 --chunk-width 128 --chunk-bands 3 2>&1 | grep "Maximum resident"

# Test with sharding
echo "4. Testing zarrsave with chunking + sharding..."
rm -rf test_sharded.zarr
/usr/bin/time -v ./build/tools/vips zarrsave test_image.v test_sharded.zarr \
    --chunk-height 64 --chunk-width 64 --chunk-bands 3 \
    --shard-height 256 --shard-width 256 --shard-bands 3 2>&1 | grep "Maximum resident"

# Test with blosc compression
echo "5. Testing zarrsave with blosc-zstd compression..."
rm -rf test_blosc.zarr
/usr/bin/time -v ./build/tools/vips zarrsave test_image.v test_blosc.zarr \
    --compression blosc-zstd --blosc-shuffle bitshuffle \
    --chunk-height 128 --chunk-width 128 --chunk-bands 3 2>&1 | grep "Maximum resident"

echo
echo "=== Verification ==="
echo "Basic zarr chunks:"
find test_basic.zarr/c -type f | wc -l
echo "Chunked zarr chunks:"
find test_chunked.zarr/c -type f | wc -l
echo "Sharded zarr shards:"
find test_sharded.zarr/c -type f | wc -l
echo "Blosc zarr chunks:"
find test_blosc.zarr/c -type f | wc -l

echo
echo "Basic zarr metadata:"
cat test_basic.zarr/zarr.json | python3 -c "import sys,json;d=json.load(sys.stdin);print(f\"Shape: {d['shape']}, Chunk: {d['chunk_grid']['configuration']['chunk_shape']}\")"

echo "Chunked zarr metadata:"
cat test_chunked.zarr/zarr.json | python3 -c "import sys,json;d=json.load(sys.stdin);print(f\"Shape: {d['shape']}, Chunk: {d['chunk_grid']['configuration']['chunk_shape']}\")"

echo "Sharded zarr metadata:"
cat test_sharded.zarr/zarr.json | python3 -c "import sys,json;d=json.load(sys.stdin);print(f\"Shape: {d['shape']}, Chunk: {d['chunk_grid']['configuration']['chunk_shape']}\")"

echo
echo "✓ All streaming tests completed successfully!"

# Clean up
rm -f test_image.v
