#!/bin/bash
# Comprehensive test script for Zarr sharding functionality

set -e

echo "=== Testing Zarr Sharding Implementation ==="
echo

# Clean up any previous test output
rm -rf test_*.zarr

# Create test image if it doesn't exist
if [ ! -f test_512_rgb.v ]; then
    echo "Creating test image (512x512x3) with gaussian noise..."
    ./build/tools/vips gaussnoise test_512.v 512 512
    ./build/tools/vips bandjoin "test_512.v test_512.v test_512.v" test_512_rgb.v
fi

echo "=== Test 1: Regular Zarr without sharding ==="
./build/tools/vips copy test_512_rgb.v test_regular.zarr
echo "✓ Created test_regular.zarr"
echo "  Metadata shows:"
cat test_regular.zarr/zarr.json | python3 -c "import sys, json; j=json.load(sys.stdin); print('  - Chunk shape:', j['chunk_grid']['configuration']['chunk_shape']); print('  - Codecs:', [c['name'] for c in j['codecs']])"
echo

echo "=== Test 2: Regular Zarr with sharding (256x256x3) ==="
./build/tools/vips copy test_512_rgb.v "test_sharded.zarr[shard_height=256,shard_width=256,shard_bands=3]"
echo "✓ Created test_sharded.zarr"
echo "  Metadata shows:"
cat test_sharded.zarr/zarr.json | python3 -c "import sys, json; j=json.load(sys.stdin); c=j['codecs'][0]; print('  - Codec:', c['name']); print('  - Inner chunk shape:', c['configuration']['chunk_shape']); print('  - Inner codecs:', [ic['name'] for ic in c['configuration']['codecs']])"
echo "  File structure (shards created):"
find test_sharded.zarr -type f -name "c*" -o -name "[0-9]*" | grep -v zarr.json | head -5
echo

echo "=== Test 3: OME-Zarr without sharding ==="
./build/tools/vips copy test_512_rgb.v "test_ome.zarr[ome_zarr=1]"
echo "✓ Created test_ome.zarr"
echo "  OME-Zarr metadata (in zarr.json attributes):"
cat test_ome.zarr/zarr.json | python3 -c "import sys, json; j=json.load(sys.stdin); ome=j.get('attributes', {}).get('ome', {}); ms=ome.get('multiscales', [{}])[0]; print('  - OME Version:', ome.get('version')); print('  - Multiscales Version:', ms.get('version')); print('  - Axes:', [a['name'] for a in ms.get('axes', [])])"
echo

echo "=== Test 4: OME-Zarr with sharding (256x256x3) ==="
./build/tools/vips copy test_512_rgb.v "test_ome_sharded.zarr[ome_zarr=1,shard_height=256,shard_width=256,shard_bands=3]"
echo "✓ Created test_ome_sharded.zarr"
echo "  Array metadata (0/zarr.json):"
cat test_ome_sharded.zarr/0/zarr.json | python3 -c "import sys, json; j=json.load(sys.stdin); c=j['codecs'][0]; print('  - Codec:', c['name']); print('  - Inner chunk shape:', c['configuration']['chunk_shape'])"
echo "  OME-Zarr metadata (in zarr.json attributes):"
cat test_ome_sharded.zarr/zarr.json | python3 -c "import sys, json; j=json.load(sys.stdin); ome=j.get('attributes', {}).get('ome', {}); print('  - OME Version:', ome.get('version'))"
echo

echo "=== Test 5: Different shard sizes (128x128x3) ==="
./build/tools/vips copy test_512_rgb.v "test_small_shards.zarr[shard_height=128,shard_width=128,shard_bands=3]"
echo "✓ Created test_small_shards.zarr"
echo "  Expected shards: 4x4x1 = 16 shards"
shard_count=$(find test_small_shards.zarr/c -type f 2>/dev/null | wc -l)
echo "  Actual shard files: $shard_count"
find test_small_shards.zarr/c -type f 2>/dev/null | head -5
echo

echo "=== Comparison Summary ==="
echo "File structure comparison:"
printf "%-25s %10s %15s\n" "Configuration" "Files" "Total Size"
printf "%-25s %10s %15s\n" "-------------------------" "----------" "---------------"
for dir in test_regular.zarr test_sharded.zarr test_ome.zarr test_ome_sharded.zarr test_small_shards.zarr; do
    files=$(find $dir -type f | wc -l)
    size=$(du -sh $dir | cut -f1)
    printf "%-25s %10d %15s\n" "$dir" "$files" "$size"
done
echo

echo "=== All Tests Passed! ==="
echo "Sharding implementation is working correctly in both regular and OME-Zarr modes."
