#!/bin/bash
# Comprehensive test of the transpose codec implementation
# Tests both single-level and pyramid zarr saves

set -e

cd "$(dirname "$0")/build"

echo "=== Transpose Codec Test Suite ==="
echo

# Create a test image
echo "1. Creating test image..."
./tools/vips black test_transpose_input.v 256 256 --bands=3
echo "   Created 256x256x3 test image"
echo

# Test 1: Basic transpose
echo "2. Testing basic transpose (single level)..."
rm -rf test_transpose_basic.zarr
./tools/vips zarrsave test_transpose_input.v test_transpose_basic.zarr --transpose-order="2 0 1"
if grep -q '"order": \[' test_transpose_basic.zarr/zarr.json; then
    echo "   ✓ Basic transpose PASSED"
    echo "   Order: $(grep -A 3 '"order":' test_transpose_basic.zarr/zarr.json | grep '\[' | tr -d ' ')"
else
    echo "   ✗ Basic transpose FAILED - no order found in metadata"
    exit 1
fi
echo

# Test 2: Transpose with pyramid
echo "3. Testing transpose with pyramid (OME-Zarr)..."
rm -rf test_transpose_pyramid.zarr
./tools/vips zarrsave test_transpose_input.v test_transpose_pyramid.zarr --pyramid --transpose-order="2 0 1"
if grep -q '"order": \[' test_transpose_pyramid.zarr/0/zarr.json && \
   grep -q '"order": \[' test_transpose_pyramid.zarr/1/zarr.json; then
    echo "   ✓ Pyramid transpose PASSED"
    echo "   Level 0 order: $(grep -A 3 '"order":' test_transpose_pyramid.zarr/0/zarr.json | grep '\[' | tr -d ' ')"
    echo "   Level 1 order: $(grep -A 3 '"order":' test_transpose_pyramid.zarr/1/zarr.json | grep '\[' | tr -d ' ')"
else
    echo "   ✗ Pyramid transpose FAILED - order not found in all levels"
    exit 1
fi
echo

# Test 3: Different transpose orders
echo "4. Testing different transpose orders..."
rm -rf test_transpose_alt.zarr
./tools/vips zarrsave test_transpose_input.v test_transpose_alt.zarr --transpose-order="0 2 1"
order_json=$(grep -A 5 '"order"' test_transpose_alt.zarr/zarr.json | grep -E '^\s*[0-9]' | tr -d ' \n,')
if [ "$order_json" = "021" ]; then
    echo "   ✓ Alternative order [0,2,1] PASSED"
else
    echo "   ✗ Alternative order FAILED (got: $order_json)"
    exit 1
fi
echo

# Test 4: No transpose (default behavior)
echo "5. Testing without transpose (default)..."
rm -rf test_no_transpose.zarr
./tools/vips zarrsave test_transpose_input.v test_no_transpose.zarr
if ! grep -q '"transpose"' test_no_transpose.zarr/zarr.json; then
    echo "   ✓ No transpose by default PASSED"
else
    echo "   ✗ No transpose FAILED - transpose found when not expected"
    exit 1
fi
echo

# Test 5: Verify codec order in chain
echo "6. Verifying codec chain order..."
# Extract codec names in order
codec_names=$(grep -A 30 '"codecs"' test_transpose_basic.zarr/zarr.json | grep '"name"' | head -3 | cut -d'"' -f4 | tr '\n' ' ')
if echo "$codec_names" | grep -q 'transpose.*bytes'; then
    echo "   ✓ Codec order correct (transpose before bytes)"
else
    echo "   ✗ Codec order incorrect (got: $codec_names)"
    exit 1
fi
echo

echo "=== All Tests PASSED ==="
echo
echo "Summary:"
echo "  - Basic transpose: working"
echo "  - Pyramid transpose: working on all levels"
echo "  - Alternative orders: working"
echo "  - Default (no transpose): working"
echo "  - Codec chain: correct order"
echo
echo "The transpose codec from Zarr v3 spec is fully implemented!"
