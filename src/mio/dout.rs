/// Monitor gear pump normally used for solution sampling.
///
///
use crate::Result;
use serde::{Deserialize, Serialize};
use crate::brocker;

#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
pub struct Gpio {
    state: bool,
    name: String,
}

impl Gpio {
    pub fn new(name:&str) -> Gpio {
        Gpio { 
            state: false,
            name:name.to_owned(),
           }
    }
    pub fn start(&mut self) -> Result<()> {
        self.state = true;
        brocker::pump_start(&self.name,0)?;
        Ok(())
    }
    pub fn stop(&mut self) -> Result<()> {
        self.state = false;
        brocker::pump_stop(&self.name)?;
        Ok(())
    }
}


