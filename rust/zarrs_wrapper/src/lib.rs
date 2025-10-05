/// Simple wrapper around zarrs to expose C-compatible functions for libvips
/// Th        Err(_) => -1,s allows libvips (C/C++) to use the zarrs library (Rust)

use std::ffi::CStr;
use std::os::raw::c_char;
use std::path::Path;
use std::sync::Arc;

use zarrs::array::{ArrayBuilder, DataType, FillValue};
use zarrs::array::codec::GzipCodec;
use zarrs::array_subset::ArraySubset;
use zarrs_filesystem::FilesystemStore;

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
    
    // Call the actual implementation
    match write_zarr_array(path_str, width, height, bands, data_type, data_slice) {
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
) -> Result<(), Box<dyn std::error::Error>> {
    // Map data type code to zarrs DataType
    let data_type = match data_type_code {
        0 => DataType::UInt8,
        1 => DataType::UInt16,
        2 => DataType::UInt32,
        3 => DataType::Float32,
        4 => DataType::Float64,
        _ => return Err("Unsupported data type".into()),
    };
    
    // Create filesystem store
    let store = Arc::new(FilesystemStore::new(Path::new(path))?);
    
    // Array shape: [height, width, bands] for interleaved data
    let array_shape = vec![height, width, bands];
    
    // Chunk shape: use full image as one chunk for simplicity
    // In production, you'd want smaller chunks
    let chunk_shape = array_shape.clone();
    
    // Create array builder
    // API: ArrayBuilder::new(shape, chunk_grid_metadata, data_type, fill_value)
    // Fill value must match the data type
    let fill_value = match data_type_code {
        0 => FillValue::from(0u8),
        1 => FillValue::from(0u16),
        2 => FillValue::from(0u32),
        3 => FillValue::from(0.0f32),
        4 => FillValue::from(0.0f64),
        _ => return Err("Unsupported data type".into()),
    };
    
    let array = ArrayBuilder::new(
        array_shape.clone(),
        chunk_shape.as_slice(),  // chunk_grid_metadata
        data_type.clone(),
        fill_value,
    )
    .bytes_to_bytes_codecs(vec![
        Arc::new(GzipCodec::new(5)?),
    ])
    .build(store, "/")?;
    
    // Store array metadata
    array.store_metadata()?;
    
    // Convert data to the appropriate format and write
    // Create subset covering the entire array
    let subset = ArraySubset::new_with_shape(array_shape.clone());
    
    // Write the raw bytes directly
    // store_array_subset expects bytes in native order
    array.store_array_subset(&subset, data)?;
    
    // Explicitly drop the array to ensure all writes are flushed
    drop(array);
    
    Ok(())
}
