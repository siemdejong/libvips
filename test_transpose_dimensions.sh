#!/bin/bash
# Test transpose codec with different array dimensions
# Tests that the implementation now supports arbitrary dimensions

set -e

cd "$(dirname "$0")/build"

echo "=== Transpose Codec Dimension Support Test ==="
echo

# Test 1: 3D array (standard vips image: height, width, bands)
echo "1. Testing 3D array (256x256x3)..."
./tools/vips black test_3d.v 256 256 --bands=3
rm -rf test_3d_transpose.zarr
./tools/vips zarrsave test_3d.v test_3d_transpose.zarr --transpose-order="2 0 1"
order_json=$(grep -A 5 '"order"' test_3d_transpose.zarr/zarr.json | grep -E '^\s*[0-9]' | tr -d ' \n,')
if [ "$order_json" = "201" ]; then
    shape=$(grep -A 5 '"shape":' test_3d_transpose.zarr/zarr.json | grep '\[' | head -1)
    echo "   ✓ 3D transpose PASSED"
    echo "   Shape: ${shape//[[:space:]]/}"
    echo "   Order: [2, 0, 1] (bands, height, width)"
else
    echo "   ✗ 3D transpose FAILED (got: $order_json)"
    exit 1
fi
echo

# Test 2: Grayscale (3D with bands=1)
echo "2. Testing grayscale image (256x256x1)..."
./tools/vips black test_gray.v 256 256 --bands=1
rm -rf test_gray_transpose.zarr
./tools/vips zarrsave test_gray.v test_gray_transpose.zarr --transpose-order="0 2 1"
order_json=$(grep -A 5 '"order"' test_gray_transpose.zarr/zarr.json | grep -E '^\s*[0-9]' | tr -d ' \n,')
if [ "$order_json" = "021" ]; then
    echo "   ✓ Grayscale transpose PASSED"
    echo "   Order: [0, 2, 1] (height, bands, width)"
else
    echo "   ✗ Grayscale transpose FAILED (got: $order_json)"
    exit 1
fi
echo

# Test 3: Multi-band (e.g., 4 bands)
echo "3. Testing 4-band image (256x256x4)..."
./tools/vips black test_4band.v 256 256 --bands=4
rm -rf test_4band_transpose.zarr
./tools/vips zarrsave test_4band.v test_4band_transpose.zarr --transpose-order="1 2 0"
order_json=$(grep -A 5 '"order"' test_4band_transpose.zarr/zarr.json | grep -E '^\s*[0-9]' | tr -d ' \n,')
if [ "$order_json" = "120" ]; then
    echo "   ✓ 4-band transpose PASSED"
    echo "   Order: [1, 2, 0] (width, bands, height)"
else
    echo "   ✗ 4-band transpose FAILED (got: $order_json)"
    exit 1
fi
echo

# Test 4: 5D with pyramid (OME-NGFF: t, z, y, x, c)
echo "4. Testing 5D array with depth dimension..."
rm -rf test_5d_depth.zarr
./tools/vips zarrsave test_3d.v test_5d_depth.zarr --depth=10 --transpose-order="4 2 3 0 1"
order_json=$(grep -A 10 '"order"' test_5d_depth.zarr/zarr.json | grep -E '^\s*[0-9]' | tr -d ' \n,')
if [ "$order_json" = "42301" ]; then
    echo "   ✓ 5D (depth) transpose PASSED"
    echo "   Order: [4, 2, 3, 0, 1] (c, y, x, t, z)"
else
    echo "   ✗ 5D (depth) transpose FAILED (got: $order_json)"
    exit 1
fi
echo

# Test 5: 5D with time dimension
echo "5. Testing 5D array with time dimension..."
rm -rf test_5d_time.zarr
./tools/vips zarrsave test_3d.v test_5d_time.zarr --time=5 --transpose-order="1 2 3 4 0"
order_json=$(grep -A 10 '"order"' test_5d_time.zarr/zarr.json | grep -E '^\s*[0-9]' | tr -d ' \n,')
if [ "$order_json" = "12340" ]; then
    echo "   ✓ 5D (time) transpose PASSED"
    echo "   Order: [1, 2, 3, 4, 0] (z, y, x, c, t)"
else
    echo "   ✗ 5D (time) transpose FAILED (got: $order_json)"
    exit 1
fi
echo

# Test 6: Invalid dimension count (should fail gracefully)
echo "6. Testing invalid transpose order (wrong dimension count)..."
rm -rf test_invalid.zarr
if ./tools/vips zarrsave test_3d.v test_invalid.zarr --transpose-order="0 1 2 3" 2>&1 | grep -qi "failed"; then
    echo "   ✓ Invalid dimension count properly rejected"
else
    echo "   ✗ Invalid dimension count not rejected"
    exit 1
fi
echo

# Test 7: Invalid permutation (duplicate indices)
echo "7. Testing invalid permutation (duplicate indices)..."
rm -rf test_invalid_perm.zarr
if ./tools/vips zarrsave test_3d.v test_invalid_perm.zarr --transpose-order="0 0 1" 2>&1 | grep -qi "failed"; then
    echo "   ✓ Invalid permutation properly rejected"
else
    echo "   ✗ Invalid permutation not rejected"
    exit 1
fi
echo

# Test 8: Test that non-transposed arrays still work
echo "8. Verifying backward compatibility (no transpose)..."
rm -rf test_no_transpose.zarr
./tools/vips zarrsave test_3d.v test_no_transpose.zarr
if ! grep -q '"transpose"' test_no_transpose.zarr/zarr.json; then
    echo "   ✓ No transpose by default PASSED"
else
    echo "   ✗ Unexpected transpose in default mode"
    exit 1
fi
echo

echo "=== All Dimension Tests PASSED ==="
echo
echo "Summary:"
echo "  ✓ 3D arrays (standard images): working"
echo "  ✓ Grayscale (bands=1): working"
echo "  ✓ Multi-band (bands=4): working"
echo "  ✓ 5D arrays with depth: working"
echo "  ✓ 5D arrays with time: working"
echo "  ✓ Invalid dimensions: properly rejected"
echo "  ✓ Invalid permutations: properly rejected"
echo "  ✓ Backward compatibility: preserved"
echo
echo "The transpose codec now supports ARBITRARY DIMENSIONS!"
