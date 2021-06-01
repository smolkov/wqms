use serde_derive::{Deserialize, Serialize};
// use std::time::{Duration};

use super::remote::{Remote};


/// Schedule
#[derive(Clone, Deserialize, Serialize, PartialEq,Debug)]
pub struct Schedule {
    /// interval in second
    pub interval  : u64,
}

impl Default for Schedule {
    fn default()->Self {
        Self {
            interval: 0,
        }
    }
}
