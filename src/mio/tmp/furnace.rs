/// Furnace
use serde::{Serialize, Deserialize};
use crate::Result;
use crate::brocker;
use std::path::{Path,PathBuf};

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct Furnace {
    pub open:  bool,
    pub state: bool,
    pub path: PathBuf,
}


impl Furnace {
    pub fn new() -> Furnace {
        let open = false;
        let state = false;
        
        Furnace{ open, state }
    }
    pub fn on(&mut self ) -> Result<()> {
        brocker::furnace_on()?;
        self.state = true;
        Ok(())
    }
    pub fn off(&mut self ) -> Result<()> {
        brocker::furnace_off()?;
        self.state = true;
        Ok(())
    }
    pub fn open(&mut self ) -> Result<()> {
        brocker::furnace_open()?;
        self.open = true;
        Ok(())
    }
    pub fn close(&mut self ) -> Result<()> {
        brocker::furnace_close()?;
        self.open = false;
        Ok(())
    }
}