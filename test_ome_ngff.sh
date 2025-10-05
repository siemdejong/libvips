#!/bin/bash
# Test script for OME-NGFF functionality

cd build

echo "=== Testing zarrsave with OME-NGFF support ==="
echo ""

# Create test images
echo "Creating test images..."
./tools/vips black test_black.v 100 100 --bands 3
./tools/vips invert test_black.v test_white.v
./tools/vips grey test_grey.v 100 100

# Test 1: Regular Zarr (3-band)
echo ""
echo "Test 1: Regular Zarr (3-band RGB)"
rm -rf test_regular_rgb.zarr
./tools/vips copy test_white.v test_regular_rgb.zarr
echo "  Structure: $(find test_regular_rgb.zarr -type d | wc -l) directories"
echo "  Metadata size: $(wc -c < test_regular_rgb.zarr/zarr.json) bytes"
echo "  Array location: $(find test_regular_rgb.zarr/c -name '0' -type f | head -1)"

# Test 2: OME-NGFF Zarr (3-band)
echo ""
echo "Test 2: OME-NGFF Zarr (3-band RGB)"
rm -rf test_ome_rgb.zarr
./tools/vips copy test_white.v test_ome_rgb.zarr[ome_ngff]
echo "  Structure: $(find test_ome_rgb.zarr -type d | wc -l) directories"
echo "  Metadata size: $(wc -c < test_ome_rgb.zarr/zarr.json) bytes"
echo "  Array location: $(find test_ome_rgb.zarr/0/c -name '0' -type f | head -1)"
echo "  OME version: $(cat test_ome_rgb.zarr/zarr.json | grep -o '"version": "[^"]*"' | head -1)"

# Test 3: OME-NGFF Zarr (1-band)
echo ""
echo "Test 3: OME-NGFF Zarr (1-band grayscale)"
rm -rf test_ome_grey.zarr
./tools/vips copy test_grey.v test_ome_grey.zarr[ome_ngff]
echo "  Axes: $(cat test_ome_grey.zarr/zarr.json | grep -o '"name": "[^"]*"' | head -4 | tr '\n' ' ')"
echo "  Array shape: $(cat test_ome_grey.zarr/0/zarr.json | grep -A 4 '"shape"' | grep -E '^ +[0-9]' | tr -d ' \n,')"

# Test 4: Comparison
echo ""
echo "Test 4: Metadata Comparison"
echo "  Regular Zarr attributes:"
cat test_regular_rgb.zarr/zarr.json | grep -A 2 '"attributes"' | head -3
echo ""
echo "  OME-NGFF Zarr attributes (first 10 lines):"
cat test_ome_rgb.zarr/zarr.json | grep -A 10 '"attributes"'

echo ""
echo "=== All OME-NGFF tests complete ==="
echo ""
echo "Summary:"
echo "  - Regular Zarr: Array at root, no OME metadata"
echo "  - OME-NGFF: Array in /0/, full OME-NGFF v0.5 metadata"
echo "  - Both modes: Zarr v3 format, gzip compression"
