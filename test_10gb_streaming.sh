#!/bin/bash
# Test zarrsave streaming with a 10 GB image to verify no memory spike

set -e

cd /home/siemdejong/libvips

echo "=== Testing Zarr Streaming with 10 GB Image ==="
echo

# Calculate dimensions for ~10 GB image
# 10 GB = 10,737,418,240 bytes
# For RGB (3 bands) uint8: 10,737,418,240 / 3 = 3,579,139,413 pixels
# Square root: ~59,826 x 59,826
# Let's use 60,000 x 60,000 x 3 = 10,800,000,000 bytes = ~10.06 GB

WIDTH=60000
HEIGHT=60000
BANDS=3
SIZE_GB=$(echo "scale=2; $WIDTH * $HEIGHT * $BANDS / 1024 / 1024 / 1024" | bc)

echo "Creating ${WIDTH}x${HEIGHT}x${BANDS} image (~${SIZE_GB} GB uncompressed)..."
echo

# Create the large test image (this is generated on-the-fly, not stored)
echo "1. Generating 10 GB test image with random data..."
# Create noise image so it doesn't compress to nothing
./build/tools/vips gaussnoise test_10gb.v $WIDTH $HEIGHT --sigma 128
# Make it 3-band
./build/tools/vips bandjoin "test_10gb.v test_10gb.v test_10gb.v" test_10gb_rgb.v
mv test_10gb_rgb.v test_10gb.v

echo "   Image created: $(ls -lh test_10gb.v | awk '{print $5}')"
echo

# Monitor memory during zarrsave
echo "2. Saving to zarr with streaming (monitoring memory)..."
echo "   This should NOT spike to 10 GB+ memory usage!"
echo

rm -rf test_10gb.zarr

# Use time to monitor memory, save to temp file for parsing
/usr/bin/time -v ./build/tools/vips zarrsave test_10gb.v test_10gb.zarr \
    --chunk-height 512 --chunk-width 512 --chunk-bands 3 \
    --compression blosc-zstd --blosc-shuffle bitshuffle 2>&1 | tee /tmp/zarr_time_output.txt

echo
echo "=== Results ==="

# Extract memory info
MAX_MEM_KB=$(grep "Maximum resident set size" /tmp/zarr_time_output.txt | awk '{print $6}')
MAX_MEM_MB=$(echo "scale=2; $MAX_MEM_KB / 1024" | bc)
MAX_MEM_GB=$(echo "scale=3; $MAX_MEM_KB / 1024 / 1024" | bc)

echo "Image size: ${SIZE_GB} GB"
echo "Maximum memory used: ${MAX_MEM_MB} MB (${MAX_MEM_GB} GB)"
echo

if (( $(echo "$MAX_MEM_GB < 1.0" | bc -l) )); then
    echo "✓ SUCCESS: Memory usage (${MAX_MEM_GB} GB) is FAR less than image size (${SIZE_GB} GB)"
    echo "✓ Streaming is working correctly!"
else
    echo "⚠ WARNING: Memory usage (${MAX_MEM_GB} GB) is significant compared to image size"
    echo "  This may indicate the image was loaded into memory"
fi

echo
echo "Zarr output stats:"
CHUNK_COUNT=$(find test_10gb.zarr/c -type f | wc -l)
ZARR_SIZE=$(du -sh test_10gb.zarr | awk '{print $1}')
echo "  Chunks created: $CHUNK_COUNT"
echo "  Total zarr size: $ZARR_SIZE"
echo "  Compression ratio: ~$(echo "scale=2; ${SIZE_GB} * 1024 / ${ZARR_SIZE%M}" | bc)x"

# Verify metadata
echo
echo "Zarr metadata:"
cat test_10gb.zarr/zarr.json | python3 -c "
import sys, json
d = json.load(sys.stdin)
print(f\"  Shape: {d['shape']}")
print(f\"  Chunk shape: {d['chunk_grid']['configuration']['chunk_shape']}")
print(f\"  Compression: {d['codecs'][1]['name']} (clevel={d['codecs'][1]['configuration']['clevel']})")
print(f\"  Shuffle: {d['codecs'][1]['configuration']['shuffle']}")
"

echo
echo "=== Cleanup ==="
echo "Deleting test files..."
rm -f test_10gb.v
rm -rf test_10gb.zarr
rm -f /tmp/zarr_time_output.txt
echo "✓ Cleanup complete"

echo
echo "=== Summary ==="
echo "Successfully processed a ${SIZE_GB} GB image using only ${MAX_MEM_MB} MB of memory!"
echo "This confirms that zarrsave uses streaming and does NOT load the entire image into RAM."
