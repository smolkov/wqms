/// TicPort
use serde::{Serialize, Deserialize};
use crate::Result;

use std::path::PathBuf;
use std::fs;
use super::Interface;
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct TicPort {
    path:  PathBuf,
}


impl From<Interface> for TicPort{
    fn from(drv:Interface) -> TicPort {
        TicPort{
            path: drv.path.to_path_buf()
        }
    }
}

impl TicPort {
    pub fn new(path:&str) -> TicPort {
        TicPort{ path:PathBuf::from(path) }
    }
    pub fn open(&mut self ) -> Result<()> {
        fs::write(self.path.join("close"), b"0")?;
        fs::write(self.path.join("open"), b"1")?;
        Ok(())
    }
    pub fn close(&mut self ) -> Result<()> {
        fs::write(self.path.join("open"), b"0")?;
        fs::write(self.path.join("close"), b"1")?;
        Ok(())
    }
}