//! Integration parameters 
//! * Justification - time to emit last zero value.
//! 
use serde::{Deserialize, Serialize};


/// Integration 
/// 
#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
pub struct Integration {
    pub justification: u64,
    pub start_treshold: u32,
    pub stop_treshold: u32,
    pub min_start: u64,
    pub min_stop: u64,
    pub max_stop: u64,
}


impl Default for Integration {
    fn default() -> Self {
        Self {
            justification: 15,
            start_treshold: 2000, 
            stop_treshold: 3000, 
            min_start: 10,
            min_stop: 60,
            max_stop: 210,
        }
    }
}
