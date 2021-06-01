

use serde::{Serialize, Deserialize};

use crate::Result;

#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
pub struct Cooler {
    on: bool,
}

impl Cooler {
    pub fn new() -> Cooler{
        let on = false;
        Cooler{on}
    }
    pub fn on(&mut self ) -> Result<()> {
        self.on = true;
        Ok(())
    }
    pub fn off(&mut self ) -> Result<()> {
        self.on = false;
        Ok(())
    }
}

