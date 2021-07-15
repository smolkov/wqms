

use serde::{Serialize, Deserialize};

use crate::Result;

use std::path::PathBuf;
use std::fs;
use super::interface::*;
#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
pub struct Cooler {
    path: PathBuf,
}



impl From<Interface> for Cooler{
    fn from(drv:Interface) -> Cooler {
        Cooler{
            path: drv.path.to_path_buf()
        }
    }
}

impl Cooler {
    pub fn on(&mut self ) -> Result<()> {
        fs::write(self.path.join("state"), b"1")?;
        Ok(())
    }
    pub fn off(&mut self ) -> Result<()> {
        fs::write(self.path.join("state"), b"0")?;
        Ok(())
    }
}

