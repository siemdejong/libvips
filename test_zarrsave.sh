#!/bin/bash
# Test script for zarrsave functionality

cd build

echo "=== Testing zarrsave with various image types ==="
echo ""

# Test 1: Single-band uint8
echo "Test 1: Single-band uint8 (white image)"
./tools/vips black test1_black.v 100 100
./tools/vips invert test1_black.v test1_white.v
./tools/vips copy test1_white.v test1_uint8.zarr
echo "  Result: test1_uint8.zarr"
./tools/vipsheader test1_white.v
echo ""

# Test 2: Three-band uint8 (RGB)
echo "Test 2: Three-band uint8 (RGB white image)"
./tools/vips black test2_black.v 100 100 --bands 3
./tools/vips invert test2_black.v test2_white.v
./tools/vips copy test2_white.v test2_rgb_uint8.zarr
echo "  Result: test2_rgb_uint8.zarr"
./tools/vipsheader test2_white.v
echo ""

# Test 3: Single-band float32
echo "Test 3: Single-band float32 (grey gradient)"
./tools/vips grey test3_grey.v 100 100
./tools/vips copy test3_grey.v test3_float32.zarr
echo "  Result: test3_float32.zarr"
./tools/vipsheader test3_grey.v
echo ""

echo "=== Verifying Zarr v3 format ==="
echo ""
for dir in test*.zarr; do
    if [ -d "$dir" ]; then
        echo "Directory: $dir"
        echo "  Metadata: $(cat $dir/zarr.json | jq -r '.zarr_format, .data_type, .shape' | xargs)"
        echo "  Chunk file: $(find $dir -name '0' | head -1)"
        echo ""
    fi
done

echo "=== Tests complete ==="
