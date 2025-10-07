/// OME-Zarr metadata generation
/// 
/// This module implements the OME-Zarr (Open Microscopy Environment - Next Generation File Format)
/// v0.5 specification for Zarr metadata.
/// 
/// Spec: https://ngff.openmicroscopy.org/0.5/

use serde_json::{json, Value};
use std::error::Error;

/// Generate OME-Zarr compliant axes metadata
/// 
/// For 5D OME-Zarr data, we generate axes in order: [t, z, y, x, c]
/// For 3D data without t/z, we generate: [y, x, c]
/// - "t" for time axis (if time > 0)
/// - "z" for depth axis (if depth > 0)
/// - "y", "x" for spatial axes (always present)
/// - "c" for channel axis (always present per OME-NGFF spec)
///
/// According to OME-Zarr spec:
/// - Spatial axes should use type "space"
/// - Channel axis should use type "channel"
/// - Time axis should use type "time"
/// - Axes must be ordered: time, channel/space (in practice: t, z, y, x, c)
/// - Channel axis should always be present for consistency
fn generate_axes(_width: u64, _height: u64, _bands: u64, depth: u64, time: u64) -> Vec<Value> {
    let mut axes = Vec::new();
    
    // Add time axis if specified (time > 0)
    if time > 0 {
        axes.push(json!({
            "name": "t",
            "type": "time",
            "unit": "millisecond"
        }));
    }
    
    // Add depth (z) axis if specified (depth > 0)
    if depth > 0 {
        axes.push(json!({
            "name": "z",
            "type": "space",
            "unit": "micrometer"
        }));
    }
    
    // Add spatial axes (y, x order for images) - always present
    axes.push(json!({
        "name": "y",
        "type": "space",
        "unit": "micrometer"
    }));
    
    axes.push(json!({
        "name": "x", 
        "type": "space",
        "unit": "micrometer"
    }));
    
    // Add channel axis - always present per OME-NGFF spec
    axes.push(json!({
        "name": "c",
        "type": "channel"
    }));
    
    axes
}

/// Generate coordinate transformations for a dataset
///
/// Returns a scale transformation. For images without physical pixel size,
/// we default to 1.0 (identity scale). Scale order matches axes order: [t, z, y, x, c]
/// Channel scale is always included per OME-NGFF spec
fn generate_coordinate_transformations(_bands: u64, depth: u64, time: u64) -> Vec<Value> {
    let mut scale = Vec::new();
    
    // Scale for time axis (if present) - always 1.0
    if time > 0 {
        scale.push(1.0);
    }
    
    // Scale for depth axis (if present) - default to 1.0 micrometer
    if depth > 0 {
        scale.push(1.0);
    }
    
    // Scale for spatial axes (y, x) - default to 1.0 micrometer - always present
    scale.push(1.0);
    scale.push(1.0);
    
    // Scale for channel axis - always 1.0, always present
    scale.push(1.0);
    
    vec![json!({
        "type": "scale",
        "scale": scale
    })]
}

/// Generate coordinate transformations for a pyramid level
///
/// Each pyramid level is downsampled by 2^level from the base resolution.
/// Level 0 has scale 1.0, level 1 has scale 2.0, level 2 has scale 4.0, etc.
/// Only spatial dimensions (y, x) are scaled; time, depth, and channels remain at 1.0
/// Channel scale is always included per OME-NGFF spec
fn generate_pyramid_coordinate_transformations(_bands: u64, depth: u64, time: u64, level: u32) -> Vec<Value> {
    let mut scale = Vec::new();
    
    // Scale for time axis (if present) - always 1.0
    if time > 0 {
        scale.push(1.0);
    }
    
    // Scale for depth axis (if present) - always 1.0 (no downsampling in z)
    if depth > 0 {
        scale.push(1.0);
    }
    
    // Scale for spatial axes (y, x) - 2^level
    let level_scale = 2.0_f64.powi(level as i32);
    scale.push(level_scale);
    scale.push(level_scale);
    
    // Scale for channel axis - always 1.0, always present
    scale.push(1.0);
    
    vec![json!({
        "type": "scale",
        "scale": scale
    })]
}

/// Generate the complete OME-Zarr metadata structure
///
/// This creates the group-level metadata that must be stored in zarr.json
/// at the root of the Zarr group.
pub fn generate_ome_zarr_metadata(
    width: u64,
    height: u64,
    bands: u64,
    depth: u64,
    time: u64,
    data_type_code: i32,
) -> Result<Value, Box<dyn Error>> {
    let axes = generate_axes(width, height, bands, depth, time);
    let coord_transforms = generate_coordinate_transformations(bands, depth, time);
    
    // Build the dataset shape to match axes order: [t?, z?, y, x, c]
    // Note: This is for documentation; actual shape is in the array metadata
    let mut shape = Vec::new();
    if time > 0 {
        shape.push(time);
    }
    if depth > 0 {
        shape.push(depth);
    }
    shape.push(height);
    shape.push(width);
    // Channel dimension is always included to match axes
    shape.push(bands);
    
    // Get data type string for zarr (for future use)
    let _data_type_str = match data_type_code {
        0 => "uint8",
        1 => "uint16",
        2 => "uint32",
        3 => "float32",
        4 => "float64",
        5 => "int8",
        6 => "int16",
        7 => "int32",
        8 => "uint64",
        9 => "int64",
        10 => "complex64",
        11 => "complex128",
        _ => return Err("Unsupported data type".into()),
    };
    
    // Create the multiscales metadata according to OME-Zarr 0.5 spec
    let metadata = json!({
        "multiscales": [{
            "version": "0.5",
            "name": "libvips-zarr",
            "axes": axes,
            "datasets": [{
                "path": "0",
                "coordinateTransformations": coord_transforms
            }],
            "type": "none",
            "metadata": {
                "description": "Created by libvips zarrsave",
                "method": "single resolution"
            }
        }]
    });
    
    Ok(metadata)
}

/// Generate OME-Zarr pyramid metadata for multiple resolution levels
///
/// This creates the group-level metadata for a pyramid with multiple resolution levels.
/// Each level is downsampled by 2x from the previous level.
///
/// Parameters:
/// - num_levels: Number of pyramid levels (1 = no pyramid, 2+ = pyramid)
/// - width, height, bands: Dimensions of the full-resolution level (level 0)
/// - depth, time: Z-slices and time points
/// - data_type_code: Data type code for the array
pub fn generate_ome_zarr_pyramid_metadata(
    num_levels: u32,
    width: u64,
    height: u64,
    bands: u64,
    depth: u64,
    time: u64,
    data_type_code: i32,
) -> Result<Value, Box<dyn Error>> {
    if num_levels == 0 {
        return Err("num_levels must be at least 1".into());
    }
    
    let axes = generate_axes(width, height, bands, depth, time);
    
    // Build datasets array for all pyramid levels
    let mut datasets = Vec::new();
    for level in 0..num_levels {
        let coord_transforms = generate_pyramid_coordinate_transformations(bands, depth, time, level);
        datasets.push(json!({
            "path": level.to_string(),
            "coordinateTransformations": coord_transforms
        }));
    }
    
    // Get data type string for zarr (for future use)
    let _data_type_str = match data_type_code {
        0 => "uint8",
        1 => "uint16",
        2 => "uint32",
        3 => "float32",
        4 => "float64",
        5 => "int8",
        6 => "int16",
        7 => "int32",
        8 => "uint64",
        9 => "int64",
        10 => "complex64",
        11 => "complex128",
        _ => return Err("Unsupported data type".into()),
    };
    
    // Create the multiscales metadata according to OME-Zarr 0.5 spec
    let metadata = json!({
        "multiscales": [{
            "version": "0.5",
            "name": "libvips-zarr",
            "axes": axes,
            "datasets": datasets,
            "type": if num_levels > 1 { "gaussian" } else { "none" },
            "metadata": {
                "description": if num_levels > 1 { 
                    "Created by libvips zarrsave with pyramid" 
                } else { 
                    "Created by libvips zarrsave" 
                },
                "method": if num_levels > 1 { 
                    "shrink by factor 2" 
                } else { 
                    "single resolution" 
                }
            }
        }]
    });
    
    Ok(metadata)
}

