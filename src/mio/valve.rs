/// Valve I/O
use serde::{Deserialize, Serialize};
use crate::Result;
use super::Interface;

use std::path::PathBuf;
use std::fs;

#[derive(Serialize, Deserialize, Clone, Debug)] 
pub struct Valve {
    path: PathBuf
}


impl From<Interface> for Valve{
    fn from(drv:Interface) -> Valve {
        Valve{
            path: drv.path.to_path_buf()
        }
    }
}

/// Single digital push-pull output pin
impl Valve {
    pub fn new(path:&str) ->Self {
        Self{
            path:PathBuf::from(path)
        }
    }
    /// Open valve
    pub fn open(&mut self) -> Result<()> {
        fs::write(self.path.join("state"), b"1")?;
        Ok(())
    }
    /// Close valve
    pub fn close(&mut self) -> Result<()> {
        fs::write(self.path.join("state"), b"0")?;
        Ok(())
    }
}