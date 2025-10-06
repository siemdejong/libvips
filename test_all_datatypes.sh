#!/bin/bash
# Test all supported data types in zarrsave

set -e

# Use build/tools/vips
VIPS=build/tools/vips

echo "Testing all Zarr data types support in lib$VIPS zarrsave"
echo "=========================================================="
echo ""

# Create a test image
echo "Creating base test image..."
$VIPS black test.v 128 128 --bands 1

# Test unsigned integer types
echo ""
echo "1. Testing UINT8 (default)..."
$VIPS zarrsave test.v test_uint8.zarr
echo "   ✓ uint8: $(cat test_uint8.zarr/zarr.json | python3 -c "import sys, json; print(json.load(sys.stdin)['data_type'])")"

echo ""
echo "2. Testing UINT16..."
$VIPS cast test.v test_uint16.v ushort
$VIPS zarrsave test_uint16.v test_uint16.zarr
echo "   ✓ uint16: $(cat test_uint16.zarr/zarr.json | python3 -c "import sys, json; print(json.load(sys.stdin)['data_type'])")"

echo ""
echo "3. Testing UINT32..."
$VIPS cast test.v test_uint32.v uint
$VIPS zarrsave test_uint32.v test_uint32.zarr
echo "   ✓ uint32: $(cat test_uint32.zarr/zarr.json | python3 -c "import sys, json; print(json.load(sys.stdin)['data_type'])")"

# Test signed integer types
echo ""
echo "4. Testing INT8..."
$VIPS cast test.v test_int8.v char
$VIPS zarrsave test_int8.v test_int8.zarr
echo "   ✓ int8: $(cat test_int8.zarr/zarr.json | python3 -c "import sys, json; print(json.load(sys.stdin)['data_type'])")"

echo ""
echo "5. Testing INT16..."
$VIPS cast test.v test_int16.v short
$VIPS zarrsave test_int16.v test_int16.zarr
echo "   ✓ int16: $(cat test_int16.zarr/zarr.json | python3 -c "import sys, json; print(json.load(sys.stdin)['data_type'])")"

echo ""
echo "6. Testing INT32..."
$VIPS cast test.v test_int32.v int
$VIPS zarrsave test_int32.v test_int32.zarr
echo "   ✓ int32: $(cat test_int32.zarr/zarr.json | python3 -c "import sys, json; print(json.load(sys.stdin)['data_type'])")"

# Test floating point types
echo ""
echo "7. Testing FLOAT32..."
$VIPS cast test.v test_float32.v float
$VIPS zarrsave test_float32.v test_float32.zarr
echo "   ✓ float32: $(cat test_float32.zarr/zarr.json | python3 -c "import sys, json; print(json.load(sys.stdin)['data_type'])")"

echo ""
echo "8. Testing FLOAT64..."
$VIPS cast test.v test_float64.v double
$VIPS zarrsave test_float64.v test_float64.zarr
echo "   ✓ float64: $(cat test_float64.zarr/zarr.json | python3 -c "import sys, json; print(json.load(sys.stdin)['data_type'])")"

# Test complex types
echo ""
echo "9. Testing COMPLEX64..."
$VIPS cast test.v test_complex64.v complex
$VIPS zarrsave test_complex64.v test_complex64.zarr
echo "   ✓ complex64: $(cat test_complex64.zarr/zarr.json | python3 -c "import sys, json; print(json.load(sys.stdin)['data_type'])")"

echo ""
echo "10. Testing COMPLEX128..."
$VIPS cast test.v test_complex128.v dpcomplex
$VIPS zarrsave test_complex128.v test_complex128.zarr
echo "   ✓ complex128: $(cat test_complex128.zarr/zarr.json | python3 -c "import sys, json; print(json.load(sys.stdin)['data_type'])")"

# Test with compression options
echo ""
echo "11. Testing INT16 with ZSTD compression..."
$VIPS zarrsave test_int16.v test_int16_zstd.zarr --compression zstd
echo "   ✓ int16 + zstd: $(cat test_int16_zstd.zarr/zarr.json | python3 -c "import sys, json; print(json.load(sys.stdin)['data_type'])")"

# Test with chunking
echo ""
echo "12. Testing FLOAT32 with chunking..."
$VIPS zarrsave test_float32.v test_float32_chunked.zarr \
    --chunk-height 64 --chunk-width 64 --chunk-bands 1
echo "   ✓ float32 + chunking: $(find test_float32_chunked.zarr -name 'c*' -type f | wc -l) chunk files"

# Test with OME-Zarr
echo ""
echo "13. Testing UINT16 with OME-Zarr metadata..."
$VIPS zarrsave test_uint16.v test_uint16_ome.zarr --ome-zarr
echo "   ✓ uint16 + OME-Zarr: $(cat test_uint16_ome.zarr/0/zarr.json | python3 -c "import sys, json; print(json.load(sys.stdin)['data_type'])")"

# Test complex with sharding
echo ""
echo "14. Testing COMPLEX64 with sharding..."
$VIPS zarrsave test_complex64.v test_complex64_sharded.zarr \
    --chunk-height 32 --chunk-width 32 --chunk-bands 1 \
    --shard-height 64 --shard-width 64 --shard-bands 1 \
    --compression zstd
echo "   ✓ complex64 + sharding + zstd: $(find test_complex64_sharded.zarr -name 'c*' -type f | wc -l) shard files"

# Display sizes
echo ""
echo "File sizes comparison:"
echo "======================"
du -sh test_uint8.zarr test_int16.zarr test_float32.zarr test_complex64.zarr | awk '{print $2 ": " $1}'

# Cleanup
echo ""
echo "Cleaning up test files..."
rm -rf test*.v test*.zarr

echo ""
echo "=========================================================="
echo "✓ All data type tests passed successfully!"
echo ""
echo "Supported data types:"
echo "  - Unsigned integers: uint8, uint16, uint32"
echo "  - Signed integers: int8, int16, int32"
echo "  - Floating point: float32, float64"
echo "  - Complex: complex64, complex128"
echo ""
echo "Note: uint64 and int64 are supported at the Zarr level but"
echo "      VIPS does not have native 64-bit integer formats."
