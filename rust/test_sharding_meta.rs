// Test program to check sharding metadata
use std::sync::Arc;
use std::path::Path;
use zarrs::array::{ArrayBuilder, DataType, FillValue, ChunkShape};
use zarrs::array::codec::{GzipCodec, ShardingCodecBuilder};
use zarrs_filesystem::FilesystemStore;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let store = Arc::new(FilesystemStore::new(Path::new("test_sharding_metadata.zarr"))?);
    
    let array_shape = vec![512u64, 512, 3];
    let chunk_shape = vec![256u64, 256, 3];
    let inner_chunk_shape: ChunkShape = vec![256u64, 256, 3].try_into()?;
    
    let sharding_codec = ShardingCodecBuilder::new(inner_chunk_shape)
        .bytes_to_bytes_codecs(vec![
            Arc::new(GzipCodec::new(5)?),
        ])
        .build();
    
    let array = ArrayBuilder::new(
        array_shape,
        chunk_shape.as_slice(),
        DataType::UInt8,
        FillValue::from(0u8),
    )
    .array_to_bytes_codec(Arc::new(sharding_codec))
    .build(store, "/")?;
    
    array.store_metadata()?;
    
    println!("Metadata stored! Check test_sharding_metadata.zarr/zarr.json");
    
    Ok(())
}
