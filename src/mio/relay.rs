/// Relay
use crate::Result;
use serde::{Deserialize, Serialize};
use crate::brocker;


#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)] 
pub struct Relay {
    name: String,
    open: bool,
}


/// Single digital push-pull output pin
impl Relay {
    pub fn new(name:&str) ->Self {
        Self {
            name: name.to_owned(),
            open: false,
        }
    }
    /// Open relay
    pub fn open(&mut self) -> Result<()> {
        brocker::relay_open(&self.name)?;
        self.open = true;
        Ok(())
    }

    /// Close relay
    ///
    pub fn close(&mut self) -> Result<()> {
        brocker::relay_close(&self.name)?;
        self.open = false;
        Ok(())
    }
}


