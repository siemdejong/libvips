#!/bin/bash
# Test dimension_order string parameter
# Tests the new intuitive string-based dimension ordering

set -e

cd "$(dirname "$0")/build"

echo "=== Dimension Order String Parameter Test ==="
echo

# Create test image
echo "Creating test image..."
./tools/vips black test_dimorder.v 256 256 --bands=3
echo

# Test 1: Default order (no parameter - should be identity yxc)
echo "1. Testing without dimension-order (default behavior)..."
rm -rf test_default.zarr
./tools/vips zarrsave test_dimorder.v test_default.zarr
if ! grep -q '"transpose"' test_default.zarr/zarr.json; then
    echo "   ✓ Default (no transpose) PASSED"
else
    echo "   ✗ Unexpected transpose in default mode"
    exit 1
fi
echo

# Test 2: yxc order (should be identity, no transpose)
echo "2. Testing 'yxc' order (identity - no transpose)..."
rm -rf test_yxc.zarr
./tools/vips zarrsave test_dimorder.v test_yxc.zarr --dimension-order="yxc"
if ! grep -q '"transpose"' test_yxc.zarr/zarr.json; then
    echo "   ✓ 'yxc' identity PASSED (no transpose codec)"
else
    echo "   ✗ Unexpected transpose for identity"
    exit 1
fi
echo

# Test 3: cyx order (bands first)
echo "3. Testing 'cyx' order (bands, height, width)..."
rm -rf test_cyx.zarr
./tools/vips zarrsave test_dimorder.v test_cyx.zarr --dimension-order="cyx"
order_json=$(grep -A 5 '"order"' test_cyx.zarr/zarr.json | grep -E '^\s*[0-9]' | tr -d ' \n,')
if [ "$order_json" = "201" ]; then
    echo "   ✓ 'cyx' correctly mapped to [2, 0, 1]"
else
    echo "   ✗ 'cyx' mapping incorrect (got: $order_json)"
    exit 1
fi
echo

# Test 4: xyc order (swap x and y)
echo "4. Testing 'xyc' order (width, height, bands)..."
rm -rf test_xyc.zarr
./tools/vips zarrsave test_dimorder.v test_xyc.zarr --dimension-order="xyc"
order_json=$(grep -A 5 '"order"' test_xyc.zarr/zarr.json | grep -E '^\s*[0-9]' | tr -d ' \n,')
if [ "$order_json" = "102" ]; then
    echo "   ✓ 'xyc' correctly mapped to [1, 0, 2]"
else
    echo "   ✗ 'xyc' mapping incorrect (got: $order_json)"
    exit 1
fi
echo

# Test 5: xcy order (width, bands, height)
echo "5. Testing 'xcy' order (width, bands, height)..."
rm -rf test_xcy.zarr
./tools/vips zarrsave test_dimorder.v test_xcy.zarr --dimension-order="xcy"
order_json=$(grep -A 5 '"order"' test_xcy.zarr/zarr.json | grep -E '^\s*[0-9]' | tr -d ' \n,')
if [ "$order_json" = "120" ]; then
    echo "   ✓ 'xcy' correctly mapped to [1, 2, 0]"
else
    echo "   ✗ 'xcy' mapping incorrect (got: $order_json)"
    exit 1
fi
echo

# Test 6: 5D with tzyxc (identity for 5D)
echo "6. Testing 5D with 'tzyxc' order (identity)..."
rm -rf test_5d_identity.zarr
./tools/vips zarrsave test_dimorder.v test_5d_identity.zarr --depth=5 --dimension-order="tzyxc"
if ! grep -q '"transpose"' test_5d_identity.zarr/zarr.json; then
    echo "   ✓ 'tzyxc' identity for 5D PASSED (no transpose codec)"
else
    echo "   ✗ Unexpected transpose for 5D identity"
    exit 1
fi
echo

# Test 7: 5D with ctzyx (bands first)
echo "7. Testing 5D with 'ctzyx' order (bands first)..."
rm -rf test_5d_ctzyx.zarr
./tools/vips zarrsave test_dimorder.v test_5d_ctzyx.zarr --depth=5 --dimension-order="ctzyx"
order_json=$(grep -A 10 '"order"' test_5d_ctzyx.zarr/zarr.json | grep -E '^\s*[0-9]' | tr -d ' \n,')
if [ "$order_json" = "40123" ]; then
    echo "   ✓ 'ctzyx' correctly mapped to [4, 0, 1, 2, 3]"
else
    echo "   ✗ 'ctzyx' mapping incorrect (got: $order_json)"
    exit 1
fi
echo

# Test 8: Case insensitive
echo "8. Testing case insensitivity 'CYX'..."
rm -rf test_CYX.zarr
./tools/vips zarrsave test_dimorder.v test_CYX.zarr --dimension-order="CYX"
order_json=$(grep -A 5 '"order"' test_CYX.zarr/zarr.json | grep -E '^\s*[0-9]' | tr -d ' \n,')
if [ "$order_json" = "201" ]; then
    echo "   ✓ 'CYX' correctly handled (case insensitive)"
else
    echo "   ✗ Case insensitivity not working"
    exit 1
fi
echo

# Test 9: Invalid characters
echo "9. Testing invalid dimension order 'abc'..."
if ! ./tools/vips zarrsave test_dimorder.v test_invalid_abc.zarr --dimension-order="abc" 2>&1; then
    echo "   ✓ Invalid characters properly rejected"
else
    echo "   ✗ Invalid characters not rejected"
    exit 1
fi
echo

# Test 10: Wrong length for 3D
echo "10. Testing wrong length 'yx' (too short for 3D)..."
if ! ./tools/vips zarrsave test_dimorder.v test_invalid_yx.zarr --dimension-order="yx" 2>&1; then
    echo "   ✓ Wrong length properly rejected"
else
    echo "   ✗ Wrong length not rejected"
    exit 1
fi
echo

# Test 11: Duplicate characters
echo "11. Testing duplicate characters 'ycc'..."
if ! ./tools/vips zarrsave test_dimorder.v test_invalid_ycc.zarr --dimension-order="ycc" 2>&1; then
    echo "   ✓ Duplicate characters properly rejected"
else
    echo "   ✗ Duplicate characters not rejected"
    exit 1
fi
echo

# Test 12: Pyramid with dimension order
echo "12. Testing pyramid with 'cyx' order (3D)..."
rm -rf test_pyramid_cyx.zarr
./tools/vips zarrsave test_dimorder.v test_pyramid_cyx.zarr --pyramid --dimension-order="cyx"
order_json=$(grep -A 5 '"order"' test_pyramid_cyx.zarr/0/zarr.json | grep -E '^\s*[0-9]' | tr -d ' \n,')
if [ "$order_json" = "201" ]; then
    echo "   ✓ Pyramid with 'cyx' PASSED"
    # Check level 1 as well
    order_json_l1=$(grep -A 5 '"order"' test_pyramid_cyx.zarr/1/zarr.json | grep -E '^\s*[0-9]' | tr -d ' \n,')
    if [ "$order_json_l1" = "201" ]; then
        echo "   ✓ All pyramid levels have correct order"
    else
        echo "   ✗ Pyramid level 1 has incorrect order"
        exit 1
    fi
else
    echo "   ✗ Pyramid 'cyx' mapping incorrect"
    exit 1
fi
echo

echo "=== All Tests PASSED ==="
echo
echo "Summary:"
echo "  ✓ Default (no parameter): no transpose"
echo "  ✓ yxc (identity): no transpose codec"
echo "  ✓ cyx: [2, 0, 1]"
echo "  ✓ xyc: [1, 0, 2]"
echo "  ✓ xcy: [1, 2, 0]"
echo "  ✓ tzyxc (5D identity): no transpose"
echo "  ✓ ctzyx (5D): [4, 0, 1, 2, 3]"
echo "  ✓ Case insensitive: working"
echo "  ✓ Invalid inputs: properly rejected"
echo "  ✓ Pyramids: working on all levels"
echo
echo "The dimension_order string parameter is fully functional!"
echo "Users can now use intuitive strings like 'cyx' instead of numeric indices!"
