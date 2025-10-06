#!/bin/bash
VIPS=build/tools/vips

echo "Testing configurable compression levels..."
echo "==========================================="
echo

# Test gzip levels
echo "GZIP Compression Levels:"
for level in 1 5 9; do
    $VIPS zarrsave test_levels_3band.v test_gzip_level${level}.zarr \
        --compression gzip --gzip-level $level \
        --chunk-height 128 --chunk-width 128 --chunk-bands 3
    size=$(du -sb test_gzip_level${level}.zarr | cut -f1)
    echo "  Level $level: $(du -sh test_gzip_level${level}.zarr | cut -f1)"
    cat test_gzip_level${level}.zarr/zarr.json | grep -A3 '"name": "gzip"' | head -6
done

echo
echo "ZSTD Compression Levels:"
for level in 1 3 10 22; do
    $VIPS zarrsave test_levels_3band.v test_zstd_level${level}.zarr \
        --compression zstd --zstd-level $level \
        --chunk-height 128 --chunk-width 128 --chunk-bands 3
    size=$(du -sb test_zstd_level${level}.zarr | cut -f1)
    echo "  Level $level: $(du -sh test_zstd_level${level}.zarr | cut -f1)"
    cat test_zstd_level${level}.zarr/zarr.json | grep -A3 '"name": "zstd"' | head -6
done

echo
echo "✓ Compression level tests completed!"
