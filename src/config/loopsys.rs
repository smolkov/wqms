
use serde::{Deserialize, Serialize};


#[derive(Clone,Default,Debug, Deserialize, Serialize)]
pub struct LoopConfig {
    pub air: u32,
    pub air_cod: u32,
}
