/// Simple wrapper around zarrs to expose C-compatible functions for libvips
/// This allows libvips (C/C++) to use the zarrs library (Rust)

mod ome_zarr;

use std::ffi::CStr;
use std::os::raw::c_char;
use std::path::Path;
use std::sync::Arc;
use std::fs;

use zarrs::array::{Array, ArrayBuilder, DataType, FillValue, ChunkShape};
use zarrs::array::codec::{GzipCodec, ZstdCodec, BloscCodec};
use zarrs::array_subset::ArraySubset;
use zarrs_filesystem::FilesystemStore;
use serde_json::json;
use num_complex::{Complex32, Complex64};

/// Context for streaming writes to a zarr array
struct ZarrWriteContext {
    array: Array<FilesystemStore>,
    width: u64,
    height: u64,
    bands: u64,
    data_type_code: i32,
    ome_zarr: bool,
    path: String,
}

/// Check if zarrs is available and working
/// Returns 1 if zarrs is available, 0 otherwise
#[no_mangle]
pub extern "C" fn vips_zarr_available() -> i32 {
    // Simple check that zarrs can be used
    // This function just verifies the library is linked correctly
    1
}

/// Get the zarrs version string
/// Returns a pointer to a static string containing the version
#[no_mangle]
pub extern "C" fn vips_zarr_version() -> *const c_char {
    // Return a static string with version info
    b"zarrs 0.22 (via Rust FFI)\0".as_ptr() as *const c_char
}

/// Simple test function to verify zarrs functionality
/// Creates a minimal array specification and returns success/failure
#[no_mangle]
pub extern "C" fn vips_zarr_test() -> i32 {
    // Try to use a basic zarrs type to ensure the library works
    // This is a simple compilation/linking test
    use zarrs::array::DataType;
    
    // Just verify we can create a data type
    let _dtype = DataType::UInt8;
    
    1 // Success
}

/// Create a new Zarr v3 array and write data to it
/// 
/// # Arguments
/// * `path` - Path to the zarr store directory (null-terminated C string)
/// * `width` - Width of the image
/// * `height` - Height of the image
/// * `bands` - Number of bands/channels
/// * `data_type` - Data type: 0=uint8, 1=uint16, 2=uint32, 3=float32, 4=float64
/// * `data` - Pointer to the image data
/// * `data_len` - Length of data in bytes
/// * `ome_zarr` - If 1, write OME-Zarr compatible metadata
/// * `chunk_height` - Chunk height (0 for full image height)
/// * `chunk_width` - Chunk width (0 for full image width)
/// * `chunk_bands` - Chunk bands (0 for all bands)
/// * `shard_height` - Shard height (0 for no sharding)
/// * `shard_width` - Shard width (0 for no sharding)
/// * `shard_bands` - Shard bands (0 for no sharding)
/// * `compression` - Compression codec: 0=gzip, 1=zstd, 2-7=blosc variants
/// * `gzip_level` - Gzip compression level (1-9, 0 for default of 5)
/// * `zstd_level` - Zstd compression level (1-22, 0 for default of 3)
/// * `blosc_clevel` - Blosc compression level (0-9)
/// * `blosc_shuffle` - Blosc shuffle mode (0=noshuffle, 1=shuffle, 2=bitshuffle)
/// * `blosc_typesize` - Blosc typesize (0 for automatic)
/// * `blosc_blocksize` - Blosc blocksize (0 for automatic)
/// 
/// # Returns
/// * 0 on success, -1 on error
#[no_mangle]
pub extern "C" fn vips_zarr_write_array(
    path: *const c_char,
    width: u64,
    height: u64,
    bands: u64,
    data_type: i32,
    data: *const u8,
    data_len: usize,
    ome_zarr: i32,
    chunk_height: i32,
    chunk_width: i32,
    chunk_bands: i32,
    shard_height: i32,
    shard_width: i32,
    shard_bands: i32,
    compression: i32,
    gzip_level: i32,
    zstd_level: i32,
    blosc_clevel: i32,
    blosc_shuffle: i32,
    blosc_typesize: i32,
    blosc_blocksize: i32,
) -> i32 {
    // Convert C string to Rust string
    let path_str = unsafe {
        match CStr::from_ptr(path).to_str() {
            Ok(s) => s,
            Err(_) => return -1,
        }
    };
    
    // Convert data pointer to slice
    let data_slice = unsafe {
        std::slice::from_raw_parts(data, data_len)
    };
    
    let use_ome_zarr = ome_zarr != 0;
    
    // Convert chunk parameters (0 means use full dimension)
    let chunk_shape = if chunk_height > 0 && chunk_width > 0 && chunk_bands > 0 {
        Some((chunk_height as u64, chunk_width as u64, chunk_bands as u64))
    } else {
        None
    };
    
    // Convert shard parameters (0 means no sharding)
    let shard_shape = if shard_height > 0 && shard_width > 0 && shard_bands > 0 {
        Some((shard_height as u64, shard_width as u64, shard_bands as u64))
    } else {
        None
    };
    
    // Call the actual implementation
    match write_zarr_array(
        path_str, width, height, bands, data_type, data_slice, use_ome_zarr, 
        chunk_shape, shard_shape, compression, gzip_level, zstd_level,
        blosc_clevel, blosc_shuffle, blosc_typesize, blosc_blocksize
    ) {
        Ok(_) => 0,
        Err(_) => -1,
    }
}

/// Internal function to write a Zarr array
fn write_zarr_array(
    path: &str,
    width: u64,
    height: u64,
    bands: u64,
    data_type_code: i32,
    data: &[u8],
    ome_zarr: bool,
    chunk_shape: Option<(u64, u64, u64)>,
    shard_shape: Option<(u64, u64, u64)>,
    compression: i32,
    gzip_level: i32,
    zstd_level: i32,
    blosc_clevel: i32,
    blosc_shuffle: i32,
    blosc_typesize: i32,
    blosc_blocksize: i32,
) -> Result<(), Box<dyn std::error::Error>> {
    // Map data type code to zarrs DataType
    let data_type = match data_type_code {
        0 => DataType::UInt8,
        1 => DataType::UInt16,
        2 => DataType::UInt32,
        3 => DataType::Float32,
        4 => DataType::Float64,
        5 => DataType::Int8,
        6 => DataType::Int16,
        7 => DataType::Int32,
        8 => DataType::UInt64,
        9 => DataType::Int64,
        10 => DataType::Complex64,
        11 => DataType::Complex128,
        _ => return Err("Unsupported data type".into()),
    };
    
    // Create filesystem store
    let store = Arc::new(FilesystemStore::new(Path::new(path))?);
    
    // Array shape: [height, width, bands] for interleaved data
    let array_shape = vec![height, width, bands];
    
    // Determine chunk shape and outer chunk shape (for sharding)
    // If chunk_shape is not provided, default to full image size
    let inner_chunk_dims = chunk_shape.unwrap_or((height, width, bands));
    
    // Outer chunk shape (shard boundaries in the chunk grid)
    // If sharding is enabled, this is the shard size
    // If sharding is not enabled, this is the same as inner chunk size
    let outer_chunk_dims = if let Some((sh, sw, sb)) = shard_shape {
        // Validate that shard dimensions are multiples of chunk dimensions
        if inner_chunk_dims.0 > 0 && sh % inner_chunk_dims.0 != 0 {
            eprintln!("Warning: shard_height ({}) is not a multiple of chunk_height ({})", sh, inner_chunk_dims.0);
        }
        if inner_chunk_dims.1 > 0 && sw % inner_chunk_dims.1 != 0 {
            eprintln!("Warning: shard_width ({}) is not a multiple of chunk_width ({})", sw, inner_chunk_dims.1);
        }
        if inner_chunk_dims.2 > 0 && sb % inner_chunk_dims.2 != 0 {
            eprintln!("Warning: shard_bands ({}) is not a multiple of chunk_bands ({})", sb, inner_chunk_dims.2);
        }
        (sh, sw, sb)
    } else {
        inner_chunk_dims
    };
    
    let outer_chunk_shape = vec![outer_chunk_dims.0, outer_chunk_dims.1, outer_chunk_dims.2];
    
    // Create array builder
    // API: ArrayBuilder::new(shape, chunk_grid_metadata, data_type, fill_value)
    // Fill value must match the data type
    let fill_value = match data_type_code {
        0 => FillValue::from(0u8),
        1 => FillValue::from(0u16),
        2 => FillValue::from(0u32),
        3 => FillValue::from(0.0f32),
        4 => FillValue::from(0.0f64),
        5 => FillValue::from(0i8),
        6 => FillValue::from(0i16),
        7 => FillValue::from(0i32),
        8 => FillValue::from(0u64),
        9 => FillValue::from(0i64),
        10 => FillValue::from(Complex32::new(0.0, 0.0)),
        11 => FillValue::from(Complex64::new(0.0, 0.0)),
        _ => return Err("Unsupported data type".into()),
    };
    
    // Determine the array path based on OME-Zarr mode
    // OME-Zarr stores the array in a subdirectory (typically "0")
    let array_path = if ome_zarr { "/0" } else { "/" };
    
    // Determine typesize for blosc (bytes per element)
    let element_size = match data_type_code {
        0 | 5 => 1,      // uint8, int8
        1 | 6 => 2,      // uint16, int16
        2 | 3 | 7 => 4,  // uint32, float32, int32
        4 | 8 | 9 => 8,  // float64, uint64, int64
        10 => 8,         // complex64 (2 x float32)
        11 => 16,        // complex128 (2 x float64)
        _ => 1,
    };
    
    // Create compression codec based on the compression parameter
    // 0 = gzip, 1 = zstd, 2-7 = blosc variants
    let compression_codec: Arc<dyn zarrs::array::codec::BytesToBytesCodecTraits> = match compression {
        1 => {
            // zstd: level 1-22, default 3, always use checksum
            let level = if zstd_level > 0 { zstd_level } else { 3 };
            Arc::new(ZstdCodec::new(level, true))
        },
        2..=7 => {
            // blosc variants: 2=lz4, 3=lz4hc, 4=blosclz, 5=zstd, 6=snappy, 7=zlib
            use zarrs::array::codec::{BloscCompressor, BloscCompressionLevel, BloscShuffleMode};
            
            let compressor = match compression {
                2 => BloscCompressor::LZ4,
                3 => BloscCompressor::LZ4HC,
                4 => BloscCompressor::BloscLZ,
                5 => BloscCompressor::Zstd,
                6 => BloscCompressor::Snappy,
                7 => BloscCompressor::Zlib,
                _ => BloscCompressor::LZ4, // fallback
            };
            
            // Blosc compression level (0-9, default 5)
            let clevel = if blosc_clevel > 0 { 
                BloscCompressionLevel::try_from(blosc_clevel as u8).unwrap_or(BloscCompressionLevel::try_from(5).unwrap())
            } else { 
                BloscCompressionLevel::try_from(5).unwrap()
            };
            
            // Blosc shuffle mode: 0=noshuffle, 1=shuffle, 2=bitshuffle
            let shuffle = match blosc_shuffle {
                0 => BloscShuffleMode::NoShuffle,
                2 => BloscShuffleMode::BitShuffle,
                _ => BloscShuffleMode::Shuffle, // 1 or default
            };
            
            // Blosc typesize (0 for automatic based on data type)
            let typesize = if blosc_typesize > 0 { 
                Some(blosc_typesize as usize)
            } else if shuffle != BloscShuffleMode::NoShuffle {
                Some(element_size)
            } else {
                None
            };
            
            // Blosc blocksize (0 for automatic)
            let blocksize = if blosc_blocksize > 0 { 
                Some(blosc_blocksize as usize)
            } else { 
                None
            };
            
            // Note: BloscCodec::new signature is (cname, clevel, blocksize, shuffle_mode, typesize)
            Arc::new(BloscCodec::new(compressor, clevel, blocksize, shuffle, typesize)?)
        },
        _ => {
            // gzip: level 1-9, default 5
            let level = if gzip_level > 0 { gzip_level as u32 } else { 5 };
            Arc::new(GzipCodec::new(level)?)
        },
    };
    
    // Build array with optional sharding
    // In Zarr v3:
    // - The chunk_grid defines the outer chunks (shards if sharding is used)
    // - The ShardingCodec's inner_chunk_shape defines the logical chunks within shards
    // - Without sharding, chunks are stored as individual files
    // - With sharding, multiple chunks are grouped into shard files
    let array = if shard_shape.is_some() {
        // With sharding: use ShardingCodec as array_to_bytes codec
        // Outer chunks (shards) contain multiple inner chunks
        use zarrs::array::codec::ShardingCodecBuilder;
        
        // Inner chunk shape within each shard
        let inner_chunk_shape: ChunkShape = vec![inner_chunk_dims.0, inner_chunk_dims.1, inner_chunk_dims.2]
            .try_into()
            .map_err(|e| format!("Invalid inner chunk shape: {:?}", e))?;
        
        let sharding_codec = ShardingCodecBuilder::new(inner_chunk_shape)
            .bytes_to_bytes_codecs(vec![compression_codec])
            .build();
        
        ArrayBuilder::new(
            array_shape.clone(),
            outer_chunk_shape.as_slice(),  // Shard boundaries in the chunk grid
            data_type.clone(),
            fill_value,
        )
        .array_to_bytes_codec(Arc::new(sharding_codec))
        .build(store.clone(), array_path)?
    } else {
        // Without sharding: chunks are stored as individual files with selected compression
        ArrayBuilder::new(
            array_shape.clone(),
            outer_chunk_shape.as_slice(),  // Chunk boundaries
            data_type.clone(),
            fill_value,
        )
        .bytes_to_bytes_codecs(vec![compression_codec])
        .build(store.clone(), array_path)?
    };
    
    // Store array metadata
    array.store_metadata()?;
    
    // If OME-Zarr mode, we need to ensure the group metadata exists before writing OME metadata
    // The store creates the zarr.json for the array in /0, but we also need one at the root
    if ome_zarr {
        // Create a minimal group metadata at the root if it doesn't exist
        let zarr_json_path = Path::new(path).join("zarr.json");
        if !zarr_json_path.exists() {
            let group_metadata = json!({
                "zarr_format": 3,
                "node_type": "group",
                "attributes": {}
            });
            fs::write(&zarr_json_path, serde_json::to_string_pretty(&group_metadata)?)?;
        }
    }
    
    // Convert data to the appropriate format and write
    // Create subset covering the entire array
    let subset = ArraySubset::new_with_shape(array_shape.clone());
    
    // Write the raw bytes directly
    // store_array_subset expects bytes in native order
    array.store_array_subset(&subset, data)?;
    
    // Explicitly drop the array to ensure all writes are flushed
    drop(array);
    
    // If OME-Zarr mode, write the group-level metadata
    if ome_zarr {
        write_ome_zarr_metadata(path, width, height, bands, data_type_code)?;
    }
    
    Ok(())
}

/// Write OME-Zarr metadata to the root zarr.json
fn write_ome_zarr_metadata(
    path: &str,
    width: u64,
    height: u64,
    bands: u64,
    data_type_code: i32,
) -> Result<(), Box<dyn std::error::Error>> {
    // Generate the OME-Zarr metadata
    let ome_metadata = ome_zarr::generate_ome_zarr_metadata(width, height, bands, data_type_code)?;
    
    // Build the path to zarr.json at the root
    let zarr_json_path = Path::new(path).join("zarr.json");
    
    // Read the existing zarr.json at the root
    let existing_content = fs::read_to_string(&zarr_json_path)?;
    let mut root_metadata: serde_json::Value = serde_json::from_str(&existing_content)?;
    
    // Add the OME metadata to attributes.ome
    if let Some(obj) = root_metadata.as_object_mut() {
        if let Some(attrs) = obj.get_mut("attributes") {
            if let Some(attrs_obj) = attrs.as_object_mut() {
                attrs_obj.insert("ome".to_string(), json!({
                    "version": "0.5",
                    "multiscales": ome_metadata["multiscales"]
                }));
            }
        } else {
            // If no attributes exist, create them
            obj.insert("attributes".to_string(), json!({
                "ome": {
                    "version": "0.5",
                    "multiscales": ome_metadata["multiscales"]
                }
            }));
        }
    }
    
    // Write back the modified zarr.json
    let updated_json = serde_json::to_string_pretty(&root_metadata)?;
    fs::write(&zarr_json_path, updated_json)?;
    
    Ok(())
}

/// Write OME-Zarr pyramid metadata to zarr.json
fn write_ome_zarr_pyramid_metadata(
    path: &str,
    num_levels: u32,
    width: u64,
    height: u64,
    bands: u64,
    data_type_code: i32,
) -> Result<(), Box<dyn std::error::Error>> {
    // Generate the OME-Zarr pyramid metadata
    let ome_metadata = ome_zarr::generate_ome_zarr_pyramid_metadata(
        num_levels, width, height, bands, data_type_code
    )?;
    
    // Build the path to zarr.json at the root
    let zarr_json_path = Path::new(path).join("zarr.json");
    
    // Check if zarr.json exists at the root
    let root_metadata = if zarr_json_path.exists() {
        // Read the existing zarr.json
        let existing_content = fs::read_to_string(&zarr_json_path)?;
        let mut metadata: serde_json::Value = serde_json::from_str(&existing_content)?;
        
        // Add the OME metadata to attributes.ome
        if let Some(obj) = metadata.as_object_mut() {
            if let Some(attrs) = obj.get_mut("attributes") {
                if let Some(attrs_obj) = attrs.as_object_mut() {
                    attrs_obj.insert("ome".to_string(), json!({
                        "version": "0.5",
                        "multiscales": ome_metadata["multiscales"]
                    }));
                }
            } else {
                // If no attributes exist, create them
                obj.insert("attributes".to_string(), json!({
                    "ome": {
                        "version": "0.5",
                        "multiscales": ome_metadata["multiscales"]
                    }
                }));
            }
        }
        
        metadata
    } else {
        // Create a new root group with OME metadata
        json!({
            "zarr_format": 3,
            "node_type": "group",
            "attributes": {
                "ome": {
                    "version": "0.5",
                    "multiscales": ome_metadata["multiscales"]
                }
            }
        })
    };
    
    // Write the metadata
    let updated_json = serde_json::to_string_pretty(&root_metadata)?;
    fs::write(&zarr_json_path, updated_json)?;
    
    Ok(())
}

// ===== Streaming API =====

/// Initialize a zarr array for streaming writes
#[no_mangle]
pub extern "C" fn vips_zarr_init_array(
    path: *const c_char,
    width: u64,
    height: u64,
    bands: u64,
    data_type: i32,
    ome_zarr: i32,
    chunk_height: i32,
    chunk_width: i32,
    chunk_bands: i32,
    shard_height: i32,
    shard_width: i32,
    shard_bands: i32,
    compression: i32,
    gzip_level: i32,
    zstd_level: i32,
    blosc_clevel: i32,
    blosc_shuffle: i32,
    blosc_typesize: i32,
    blosc_blocksize: i32,
) -> *mut std::ffi::c_void {
    // Convert C string to Rust string
    let path_str = unsafe {
        match CStr::from_ptr(path).to_str() {
            Ok(s) => s.to_string(),
            Err(_) => return std::ptr::null_mut(),
        }
    };
    
    let use_ome_zarr = ome_zarr != 0;
    
    // Convert chunk parameters
    let chunk_shape = if chunk_height > 0 && chunk_width > 0 && chunk_bands > 0 {
        Some((chunk_height as u64, chunk_width as u64, chunk_bands as u64))
    } else {
        None
    };
    
    // Convert shard parameters
    let shard_shape = if shard_height > 0 && shard_width > 0 && shard_bands > 0 {
        Some((shard_height as u64, shard_width as u64, shard_bands as u64))
    } else {
        None
    };
    
    // Initialize the array
    match init_zarr_array(
        &path_str, width, height, bands, data_type, use_ome_zarr,
        chunk_shape, shard_shape, compression, gzip_level, zstd_level,
        blosc_clevel, blosc_shuffle, blosc_typesize, blosc_blocksize
    ) {
        Ok(ctx) => {
            // Box the context and convert to opaque handle
            let boxed = Box::new(ctx);
            let handle = Box::into_raw(boxed) as *mut std::ffi::c_void;
            handle
        },
        Err(_) => std::ptr::null_mut(),
    }
}

/// Write a region of data to the zarr array
#[no_mangle]
pub extern "C" fn vips_zarr_write_region(
    handle: *mut std::ffi::c_void,
    x: u64,
    y: u64,
    width: u64,
    height: u64,
    data: *const u8,
    data_len: usize,
) -> i32 {
    if handle.is_null() {
        return -1;
    }
    
    // Convert handle back to reference
    let ctx = unsafe { &mut *(handle as *mut ZarrWriteContext) };
    
    let data_slice = unsafe {
        std::slice::from_raw_parts(data, data_len)
    };
    
    match write_region(ctx, x, y, width, height, data_slice) {
        Ok(_) => 0,
        Err(_) => -1,
    }
}

/// Finalize the zarr array and free resources
#[no_mangle]
pub extern "C" fn vips_zarr_finalize(handle: *mut std::ffi::c_void) -> i32 {
    if handle.is_null() {
        return -1;
    }
    
    // Convert handle back to Box and take ownership
    let ctx = unsafe { Box::from_raw(handle as *mut ZarrWriteContext) };
    
    match finalize_zarr_array(*ctx) {
        Ok(_) => 0,
        Err(_) => -1,
    }
}

/// Finalize a zarr array without writing OME metadata (for pyramid levels)
#[no_mangle]
pub extern "C" fn vips_zarr_finalize_no_metadata(handle: *mut std::ffi::c_void) -> i32 {
    if handle.is_null() {
        return -1;
    }
    
    // Convert handle back to Box and take ownership
    let ctx = unsafe { Box::from_raw(handle as *mut ZarrWriteContext) };
    
    match finalize_zarr_array_no_metadata(*ctx) {
        Ok(_) => 0,
        Err(_) => -1,
    }
}

/// Write OME-Zarr pyramid metadata for multiple resolution levels
#[no_mangle]
pub extern "C" fn vips_zarr_write_pyramid_metadata(
    path: *const c_char,
    num_levels: u32,
    width: u64,
    height: u64,
    bands: u64,
    data_type: i32,
) -> i32 {
    if path.is_null() {
        return -1;
    }
    
    let path_str = match unsafe { CStr::from_ptr(path).to_str() } {
        Ok(s) => s,
        Err(_) => return -1,
    };
    
    match write_ome_zarr_pyramid_metadata(path_str, num_levels, width, height, bands, data_type) {
        Ok(_) => 0,
        Err(_) => -1,
    }
}

/// Internal function to initialize a zarr array
fn init_zarr_array(
    path: &str,
    width: u64,
    height: u64,
    bands: u64,
    data_type_code: i32,
    ome_zarr: bool,
    chunk_shape: Option<(u64, u64, u64)>,
    shard_shape: Option<(u64, u64, u64)>,
    compression: i32,
    gzip_level: i32,
    zstd_level: i32,
    blosc_clevel: i32,
    blosc_shuffle: i32,
    blosc_typesize: i32,
    blosc_blocksize: i32,
) -> Result<ZarrWriteContext, Box<dyn std::error::Error>> {
    // Map data type code to zarrs DataType
    let data_type = match data_type_code {
        0 => DataType::UInt8,
        1 => DataType::UInt16,
        2 => DataType::UInt32,
        3 => DataType::Float32,
        4 => DataType::Float64,
        5 => DataType::Int8,
        6 => DataType::Int16,
        7 => DataType::Int32,
        8 => DataType::UInt64,
        9 => DataType::Int64,
        10 => DataType::Complex64,
        11 => DataType::Complex128,
        _ => return Err("Unsupported data type".into()),
    };
    
    // Create filesystem store
    let store = Arc::new(FilesystemStore::new(Path::new(path))?);
    
    // Array shape: [height, width, bands]
    let array_shape = vec![height, width, bands];
    
    // Determine chunk shape
    let inner_chunk_dims = chunk_shape.unwrap_or((height, width, bands));
    let outer_chunk_dims = shard_shape.unwrap_or(inner_chunk_dims);
    let outer_chunk_shape = vec![outer_chunk_dims.0, outer_chunk_dims.1, outer_chunk_dims.2];
    
    // Fill value
    let fill_value = match data_type_code {
        0 => FillValue::from(0u8),
        1 => FillValue::from(0u16),
        2 => FillValue::from(0u32),
        3 => FillValue::from(0.0f32),
        4 => FillValue::from(0.0f64),
        5 => FillValue::from(0i8),
        6 => FillValue::from(0i16),
        7 => FillValue::from(0i32),
        8 => FillValue::from(0u64),
        9 => FillValue::from(0i64),
        10 => FillValue::from(Complex32::new(0.0, 0.0)),
        11 => FillValue::from(Complex64::new(0.0, 0.0)),
        _ => return Err("Unsupported data type".into()),
    };
    
    let array_path = if ome_zarr { "/0" } else { "/" };
    
    // Determine typesize for blosc
    let element_size = match data_type_code {
        0 | 5 => 1,
        1 | 6 => 2,
        2 | 3 | 7 => 4,
        4 | 8 | 9 => 8,
        10 => 8,
        11 => 16,
        _ => 1,
    };
    
    // Create compression codec
    let compression_codec: Arc<dyn zarrs::array::codec::BytesToBytesCodecTraits> = match compression {
        1 => {
            let level = if zstd_level > 0 { zstd_level } else { 3 };
            Arc::new(ZstdCodec::new(level, true))
        },
        2..=7 => {
            use zarrs::array::codec::{BloscCompressor, BloscCompressionLevel, BloscShuffleMode};
            
            let compressor = match compression {
                2 => BloscCompressor::LZ4,
                3 => BloscCompressor::LZ4HC,
                4 => BloscCompressor::BloscLZ,
                5 => BloscCompressor::Zstd,
                6 => BloscCompressor::Snappy,
                7 => BloscCompressor::Zlib,
                _ => BloscCompressor::LZ4,
            };
            
            let clevel = if blosc_clevel > 0 {
                BloscCompressionLevel::try_from(blosc_clevel as u8).unwrap_or(BloscCompressionLevel::try_from(5).unwrap())
            } else {
                BloscCompressionLevel::try_from(5).unwrap()
            };
            
            let shuffle = match blosc_shuffle {
                0 => BloscShuffleMode::NoShuffle,
                2 => BloscShuffleMode::BitShuffle,
                _ => BloscShuffleMode::Shuffle,
            };
            
            let typesize = if blosc_typesize > 0 {
                Some(blosc_typesize as usize)
            } else if shuffle != BloscShuffleMode::NoShuffle {
                Some(element_size)
            } else {
                None
            };
            
            let blocksize = if blosc_blocksize > 0 {
                Some(blosc_blocksize as usize)
            } else {
                None
            };
            
            Arc::new(BloscCodec::new(compressor, clevel, blocksize, shuffle, typesize)?)
        },
        _ => {
            let level = if gzip_level > 0 { gzip_level as u32 } else { 5 };
            Arc::new(GzipCodec::new(level)?)
        },
    };
    
    // Build array with optional sharding
    let array = if shard_shape.is_some() {
        use zarrs::array::codec::ShardingCodecBuilder;
        
        let inner_chunk_shape: ChunkShape = vec![inner_chunk_dims.0, inner_chunk_dims.1, inner_chunk_dims.2]
            .try_into()
            .map_err(|e| format!("Invalid inner chunk shape: {:?}", e))?;
        
        let sharding_codec = ShardingCodecBuilder::new(inner_chunk_shape)
            .bytes_to_bytes_codecs(vec![compression_codec])
            .build();
        
        ArrayBuilder::new(
            array_shape.clone(),
            outer_chunk_shape.as_slice(),
            data_type.clone(),
            fill_value,
        )
        .array_to_bytes_codec(Arc::new(sharding_codec))
        .build(store.clone(), array_path)?
    } else {
        ArrayBuilder::new(
            array_shape.clone(),
            outer_chunk_shape.as_slice(),
            data_type.clone(),
            fill_value,
        )
        .bytes_to_bytes_codecs(vec![compression_codec])
        .build(store.clone(), array_path)?
    };
    
    // Store array metadata
    array.store_metadata()?;
    
    // Create group metadata for OME-Zarr
    if ome_zarr {
        let zarr_json_path = Path::new(path).join("zarr.json");
        if !zarr_json_path.exists() {
            let group_metadata = json!({
                "zarr_format": 3,
                "node_type": "group",
                "attributes": {}
            });
            fs::write(&zarr_json_path, serde_json::to_string_pretty(&group_metadata)?)?;
        }
    }
    
    Ok(ZarrWriteContext {
        array,
        width,
        height,
        bands,
        data_type_code,
        ome_zarr,
        path: path.to_string(),
    })
}

/// Write a region of data to the zarr array
fn write_region(
    ctx: &mut ZarrWriteContext,
    x: u64,
    y: u64,
    width: u64,
    height: u64,
    data: &[u8],
) -> Result<(), Box<dyn std::error::Error>> {
    // Create subset for this region: [y..y+height, x..x+width, 0..bands]
    let subset = ArraySubset::new_with_ranges(&[
        y..(y + height),
        x..(x + width),
        0..ctx.bands,
    ]);
    
    // Write the data
    ctx.array.store_array_subset(&subset, data)?;
    
    Ok(())
}

/// Finalize the zarr array
fn finalize_zarr_array(
    ctx: ZarrWriteContext,
) -> Result<(), Box<dyn std::error::Error>> {
    // Drop the array to ensure all writes are flushed
    drop(ctx.array);
    
    // Write OME-Zarr metadata if needed
    if ctx.ome_zarr {
        write_ome_zarr_metadata(&ctx.path, ctx.width, ctx.height, ctx.bands, ctx.data_type_code)?;
    }
    
    Ok(())
}

/// Finalize the zarr array without writing OME metadata (for pyramid levels)
fn finalize_zarr_array_no_metadata(
    ctx: ZarrWriteContext,
) -> Result<(), Box<dyn std::error::Error>> {
    // Drop the array to ensure all writes are flushed
    drop(ctx.array);
    Ok(())
}

// ===== Reading API =====

/// Context for reading from a zarr array
struct ZarrReadContext {
    array: Array<FilesystemStore>,
    width: u64,
    height: u64,
    bands: u64,
    data_type_code: i32,
}

/// Open a zarr array for reading
/// Returns handle containing array metadata, or NULL on error
#[no_mangle]
pub extern "C" fn vips_zarr_open(
    path: *const c_char,
) -> *mut std::ffi::c_void {
    if path.is_null() {
        return std::ptr::null_mut();
    }
    
    let path_str = match unsafe { CStr::from_ptr(path).to_str() } {
        Ok(s) => s,
        Err(_) => return std::ptr::null_mut(),
    };
    
    match open_zarr_array(path_str) {
        Ok(ctx) => Box::into_raw(Box::new(ctx)) as *mut std::ffi::c_void,
        Err(_) => std::ptr::null_mut(),
    }
}

/// Get dimensions and data type from an open zarr array
#[no_mangle]
pub extern "C" fn vips_zarr_get_metadata(
    handle: *mut std::ffi::c_void,
    width: *mut u64,
    height: *mut u64,
    bands: *mut u64,
    data_type: *mut i32,
) -> i32 {
    if handle.is_null() || width.is_null() || height.is_null() || 
       bands.is_null() || data_type.is_null() {
        return -1;
    }
    
    let ctx = unsafe { &*(handle as *const ZarrReadContext) };
    
    unsafe {
        *width = ctx.width;
        *height = ctx.height;
        *bands = ctx.bands;
        *data_type = ctx.data_type_code;
    }
    
    0
}

/// Read a region from an open zarr array
/// The output buffer must be pre-allocated with size width*height*bands*element_size
#[no_mangle]
pub extern "C" fn vips_zarr_read_region(
    handle: *mut std::ffi::c_void,
    x: u64,
    y: u64,
    width: u64,
    height: u64,
    data: *mut u8,
    data_len: usize,
) -> i32 {
    if handle.is_null() || data.is_null() {
        return -1;
    }
    
    let ctx = unsafe { &*(handle as *const ZarrReadContext) };
    
    // Convert raw pointer to slice
    let buffer = unsafe { std::slice::from_raw_parts_mut(data, data_len) };
    
    match read_zarr_region(ctx, x, y, width, height, buffer) {
        Ok(_) => 0,
        Err(_) => -1,
    }
}

/// Close a zarr array and free resources
#[no_mangle]
pub extern "C" fn vips_zarr_close(handle: *mut std::ffi::c_void) -> i32 {
    if handle.is_null() {
        return -1;
    }
    
    // Convert handle back to Box and drop it
    unsafe { Box::from_raw(handle as *mut ZarrReadContext) };
    
    0
}

/// Internal function to open a zarr array for reading
fn open_zarr_array(
    path: &str,
) -> Result<ZarrReadContext, Box<dyn std::error::Error>> {
    // Create filesystem store
    let store = Arc::new(FilesystemStore::new(Path::new(path))?);
    
    // Open the array at the root (use "/" for root arrays)
    let array = Array::open(store, "/")?;
    
    // Get array shape and data type
    let shape = array.shape();
    
    // Expected shape is [height, width, bands] or [height, width]
    let (height, width, bands) = if shape.len() == 3 {
        (shape[0], shape[1], shape[2])
    } else if shape.len() == 2 {
        (shape[0], shape[1], 1)
    } else {
        return Err("Unsupported array shape".into());
    };
    
    // Map data type to code
    let data_type_code = match array.data_type() {
        DataType::UInt8 => 0,
        DataType::UInt16 => 1,
        DataType::UInt32 => 2,
        DataType::Float32 => 3,
        DataType::Float64 => 4,
        DataType::Int8 => 5,
        DataType::Int16 => 6,
        DataType::Int32 => 7,
        DataType::UInt64 => 8,
        DataType::Int64 => 9,
        DataType::Complex64 => 10,
        DataType::Complex128 => 11,
        _ => return Err("Unsupported data type".into()),
    };
    
    Ok(ZarrReadContext {
        array,
        width,
        height,
        bands,
        data_type_code,
    })
}

/// Internal function to read a region from a zarr array
fn read_zarr_region(
    ctx: &ZarrReadContext,
    x: u64,
    y: u64,
    width: u64,
    height: u64,
    buffer: &mut [u8],
) -> Result<(), Box<dyn std::error::Error>> {
    // Create array subset for the region
    let subset = ArraySubset::new_with_ranges(&[
        y..(y + height),
        x..(x + width),
        0..ctx.bands,
    ]);
    
    // Retrieve the data
    let data = ctx.array.retrieve_array_subset(&subset)?;
    
    // Copy to output buffer
    let bytes = data.into_fixed()?;
    buffer[..bytes.len()].copy_from_slice(&bytes);
    
    Ok(())
}
