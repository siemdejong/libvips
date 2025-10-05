#!/bin/bash
# Test script for Zarr sharding functionality

set -e

echo "=== Testing Zarr Sharding ==="

# Clean up any previous test output
rm -rf test_zarr_sharded.zarr test_zarr_sharded_ome.zarr

# Create a test image (RGB, 512x512)
echo "Creating test image..."
./build/tools/vips black test_rgb.v 512 512 --bands 3
./build/tools/vips linear test_rgb.v test_rgb.v "1 2 3" "0 0 0"

# Test 1: Regular Zarr without sharding
echo -e "\n=== Test 1: Regular Zarr without sharding ==="
./build/tools/vips copy test_rgb.v test_zarr_regular.zarr
echo "Created test_zarr_regular.zarr"
ls -lh test_zarr_regular.zarr/
find test_zarr_regular.zarr -type f -name "*.zarr" | head -5

# Test 2: Regular Zarr with sharding (256x256x3 shards)
echo -e "\n=== Test 2: Regular Zarr with sharding ==="
./build/tools/vips copy test_rgb.v "test_zarr_sharded.zarr[shard_height=256,shard_width=256,shard_bands=3]"
echo "Created test_zarr_sharded.zarr with sharding"
ls -lh test_zarr_sharded.zarr/
find test_zarr_sharded.zarr -type f | head -10

# Test 3: OME-Zarr without sharding
echo -e "\n=== Test 3: OME-Zarr without sharding ==="
./build/tools/vips copy test_rgb.v "test_zarr_ome.zarr[ome_zarr=1]"
echo "Created test_zarr_ome.zarr"
ls -lh test_zarr_ome.zarr/
cat test_zarr_ome.zarr/ome.zarr

# Test 4: OME-Zarr with sharding (256x256x3 shards)
echo -e "\n=== Test 4: OME-Zarr with sharding ==="
./build/tools/vips copy test_rgb.v "test_zarr_sharded_ome.zarr[ome_zarr=1,shard_height=256,shard_width=256,shard_bands=3]"
echo "Created test_zarr_sharded_ome.zarr with sharding"
ls -lh test_zarr_sharded_ome.zarr/
cat test_zarr_sharded_ome.zarr/ome.zarr

# Test 5: Different shard sizes
echo -e "\n=== Test 5: Different shard sizes (128x128x3) ==="
./build/tools/vips copy test_rgb.v "test_zarr_small_shards.zarr[shard_height=128,shard_width=128,shard_bands=3]"
echo "Created test_zarr_small_shards.zarr with small shards"
find test_zarr_small_shards.zarr -type f | wc -l
echo "Number of files created ^"

# Compare file structures
echo -e "\n=== Comparing file structures ==="
echo "Regular (no sharding):"
find test_zarr_regular.zarr -type f | wc -l
echo "Sharded (256x256):"
find test_zarr_sharded.zarr -type f | wc -l
echo "Small shards (128x128):"
find test_zarr_small_shards.zarr -type f | wc -l

# Check zarr.json metadata
echo -e "\n=== Checking zarr.json metadata ==="
echo "Regular:"
cat test_zarr_regular.zarr/zarr.json | python3 -m json.tool | head -30
echo -e "\nSharded:"
cat test_zarr_sharded.zarr/zarr.json | python3 -m json.tool | head -30

echo -e "\n=== Tests completed ==="
echo "All test files created successfully!"
