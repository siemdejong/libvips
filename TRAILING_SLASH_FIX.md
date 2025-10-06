# Trailing Slash Support for .zarr Directories

## Problem
Users might add a trailing slash when specifying zarr paths (e.g., `test.zarr/`), which is common when tab-completing directory names in shells. Without this fix, the loader would fail with "is a directory" error.

## Solution

### 1. Updated Directory Check in foreign.c
**File:** `/home/siemdejong/libvips/libvips/foreign/foreign.c` (lines 622-629)

```c
/* Zarr files are directories, so skip the directory check for them.
 * Handle both .zarr and .zarr/ (with trailing slash).
 */
if (vips_isdirf("%s", filename) &&
    !g_str_has_suffix(filename, ".zarr") &&
    !g_str_has_suffix(filename, ".zarr/")) {
    vips_error("VipsForeignLoad",
        _("\"%s\" is a directory"), name);
    return NULL;
}
```

This prevents the generic "is a directory" error for both `.zarr` and `.zarr/` paths.

### 2. Updated is_a Check in zarrload.c
**File:** `/home/siemdejong/libvips/libvips/foreign/zarrload.c` (lines 100-108)

```c
static int
vips_foreign_load_zarr_is_a(const char *filename)
{
    /* Check if it's a directory ending in .zarr or .zarr/
     */
    return g_file_test(filename, G_FILE_TEST_IS_DIR) &&
        (g_str_has_suffix(filename, ".zarr") ||
         g_str_has_suffix(filename, ".zarr/"));
}
```

This ensures the zarrload is_a() method matches both formats during loader discovery.

## Testing

All tests pass with both formats:

```bash
# Both commands work identically:
vips copy test.zarr output.png      ✅
vips copy test.zarr/ output.png     ✅

# OME-Zarr with trailing slash:
vips copy test_ome.zarr/ output.png ✅

# Explicit zarrload command:
vips zarrload test.zarr/ output.png ✅
```

### Verification
- File sizes identical with and without trailing slash
- No "is a directory" errors
- Works for both simple zarr arrays and OME-Zarr pyramids
- Shell tab-completion now works seamlessly

## User Experience Impact

This is a quality-of-life improvement that makes zarrload work the same way users expect:
- **Tab completion friendly**: Users can tab-complete directory names and the trailing slash won't cause issues
- **Consistent behavior**: Works like other directory-based formats (e.g., some users may be familiar with directory-based image formats)
- **Less frustrating**: No need to remember to remove trailing slashes
- **Shell script friendly**: Works in automated scripts that may add trailing slashes

## Technical Notes

The fix is minimal and only affects:
1. The generic directory check in loader discovery (foreign.c)
2. The zarrload-specific is_a() method

Both changes use simple suffix checking with no performance impact. The trailing slash is preserved through the loading process and doesn't affect the actual zarr opening logic, which handles it correctly.
