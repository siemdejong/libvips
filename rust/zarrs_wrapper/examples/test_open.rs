use std::path::Path;
use std::sync::Arc;
use zarrs::array::Array;
use zarrs_filesystem::FilesystemStore;

fn main() {
    println!("Testing zarr array opening...");
    
    // Test 1: Root array (test.zarr)
    println!("\n1. Opening test.zarr (root array):");
    let store1 = Arc::new(FilesystemStore::new(Path::new("../../test.zarr")).unwrap());
    
    // Try different path options
    for path in ["", "/", "."] {
        print!("  Trying path '{}': ", path);
        match Array::open(store1.clone(), path) {
            Ok(arr) => println!("✓ Success! Shape: {:?}", arr.shape()),
            Err(e) => println!("✗ Failed: {}", e),
        }
    }
    
    // Test 2: OME-Zarr level 0
    println!("\n2. Opening test_ome.zarr/0:");
    let store2 = Arc::new(FilesystemStore::new(Path::new("../../test_ome.zarr/0")).unwrap());
    
    for path in ["", "/", "."] {
        print!("  Trying path '{}': ", path);
        match Array::open(store2.clone(), path) {
            Ok(arr) => println!("✓ Success! Shape: {:?}", arr.shape()),
            Err(e) => println!("✗ Failed: {}", e),
        }
    }
    
    // Test 3: Store at root, access subdirectory
    println!("\n3. Opening test_ome.zarr with path '0':");
    let store3 = Arc::new(FilesystemStore::new(Path::new("../../test_ome.zarr")).unwrap());
    
    print!("  Trying path '0': ");
    match Array::open(store3.clone(), "0") {
        Ok(arr) => println!("✓ Success! Shape: {:?}", arr.shape()),
        Err(e) => println!("✗ Failed: {}", e),
    }
}
