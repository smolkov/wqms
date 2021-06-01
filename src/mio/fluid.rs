/// Fluid sensor
/// 
use crate::Result;
use serde::{Deserialize, Serialize};



#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)] 
pub struct Fluid {
    status: bool,
    name :String,
}

/// Single digital push-pull output pin
impl Fluid {
    pub fn new(name:&str) ->Self {
        Self {
            status: false,
            name:name.to_owned(),
        }
    }
    /// Check fluid sensor status 
    pub fn status(&mut self) -> Result<bool>{
        Ok(self.status)
    }
}

