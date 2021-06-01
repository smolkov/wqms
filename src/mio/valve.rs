/// Valve I/O
use serde::{Deserialize, Serialize};
use crate::Result;
use crate::brocker;

#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)] 
pub struct Valve {
    open: bool,
    name: String,
}


/// Single digital push-pull output pin
impl Valve {
    pub fn new(name:&str) ->Self {
        Self {
            open: false,
            name:name.to_owned() ,
        }
    }
    /// Open valve
    pub fn open(&mut self) -> Result<()> {
        self.open = true;
        brocker::valve_open(&self.name)?;
        Ok(())
    }
    /// Close valve
    pub fn close(&mut self) -> Result<()> {
        self.open = false;
        brocker::valve_close(&self.name)?;
        Ok(())
    }
}