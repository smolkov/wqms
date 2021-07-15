
use serde::{Deserialize, Serialize};
use crate::Result;


use std::path::PathBuf;
use std::fs;
use super::Interface;

#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
pub struct Stirrer {
    path: PathBuf,
}


impl From<Interface> for Stirrer{
    fn from(drv:Interface) -> Stirrer {
        Stirrer{
            path: drv.path.to_path_buf()
        }
    }
}

impl Stirrer {
    pub fn new(path:&str) -> Stirrer{
        Stirrer{
            path: PathBuf::from(path),
        }
    }
    pub fn start(&mut self) -> Result<()> {
        fs::write(self.path.join("state"), b"1")?;
        Ok(())
    }
    pub fn stop(&mut self) -> Result<()>{
        fs::write(self.path.join("state"), b"1")?;
        Ok(()) 
    }
    pub fn set_delay(&mut self,delay:u32) -> Result<()> {
        fs::write(self.path.join("delay"), format!("{}", delay).trim().as_bytes())?;
        Ok(()) 
    }
    pub fn set_current(&mut self,current:u32) -> Result<()>{
        fs::write(self.path.join("current"), format!("{}", current).trim().as_bytes())?;
        Ok(()) 
    }
}



