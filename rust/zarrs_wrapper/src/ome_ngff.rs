/// OME-NGFF metadata generation
/// 
/// This module implements the OME-NGFF (Open Microscopy Environment - Next Generation File Format)
/// v0.5 specification for Zarr metadata.
/// 
/// Spec: https://ngff.openmicroscopy.org/0.5/

use serde_json::{json, Value};
use std::error::Error;

/// Generate OME-NGFF compliant axes metadata
/// 
/// For a simple 2D or 3D image with channels, we generate:
/// - "y", "x" for spatial axes (always present)
/// - "c" for channel axis (if bands > 1)
///
/// According to OME-NGFF spec:
/// - Spatial axes should use type "space"
/// - Channel axis should use type "channel"
/// - Axes must be ordered: time (optional), channel (optional), space (z, y, x)
fn generate_axes(_width: u64, _height: u64, bands: u64) -> Vec<Value> {
    let mut axes = Vec::new();
    
    // Add channel axis if more than 1 band
    if bands > 1 {
        axes.push(json!({
            "name": "c",
            "type": "channel"
        }));
    }
    
    // Add spatial axes (y, x order for images)
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
    
    axes
}

/// Generate coordinate transformations for a dataset
///
/// Returns a scale transformation. For images without physical pixel size,
/// we default to 1.0 (identity scale).
fn generate_coordinate_transformations(bands: u64) -> Vec<Value> {
    let mut scale = Vec::new();
    
    // Scale for channel axis (if present) - always 1.0
    if bands > 1 {
        scale.push(1.0);
    }
    
    // Scale for spatial axes (y, x) - default to 1.0 micrometer
    scale.push(1.0);
    scale.push(1.0);
    
    vec![json!({
        "type": "scale",
        "scale": scale
    })]
}

/// Generate the complete OME-NGFF metadata structure
///
/// This creates the group-level metadata that must be stored in zarr.json
/// at the root of the Zarr group.
pub fn generate_ome_ngff_metadata(
    width: u64,
    height: u64,
    bands: u64,
    data_type_code: i32,
) -> Result<Value, Box<dyn Error>> {
    let axes = generate_axes(width, height, bands);
    let coord_transforms = generate_coordinate_transformations(bands);
    
    // Build the dataset shape
    let mut shape = Vec::new();
    if bands > 1 {
        shape.push(bands);
    }
    shape.push(height);
    shape.push(width);
    
    // Get data type string for zarr (for future use)
    let _data_type_str = match data_type_code {
        0 => "uint8",
        1 => "uint16",
        2 => "uint32",
        3 => "float32",
        4 => "float64",
        _ => return Err("Unsupported data type".into()),
    };
    
    // Create the multiscales metadata according to OME-NGFF 0.5 spec
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
