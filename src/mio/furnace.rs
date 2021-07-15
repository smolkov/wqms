/// Monitor gear pump normally used for solution sampling.
///
///

// use std::prelude::*;
// use std::io;
use std::fs;
use std::time::Duration;
use std::path::PathBuf;
use serde::{Serialize, Deserialize};
// use async_trait::async_trait;
use super::*;
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct Furnace {
    pub path: PathBuf,
}

impl From<Interface> for Furnace {
    #[inline]
    fn from(device:Interface) -> Furnace {
        Furnace{
            path: device.path.to_path_buf()
        }
    }
}

impl From<&Furnace> for Interface {
    #[inline]
    fn from(furnace:&Furnace) -> Interface {
        Interface{
            path:furnace.path.to_path_buf()
        }
    }
}


impl Furnace{
    pub fn heat_on(&self) -> Result<()> {
        fs::write(self.path.join("heat"), b"1")?;
        Ok(())
    }
    pub fn heat_off(&self) -> Result<()> {
        fs::write(self.path.join("heat"), b"1")?;
        Ok(())
    }
     pub fn is_open(&self)   -> Result<bool> {
        match fs::read_to_string(self.path.join("is_open"))?.as_str() {
            "1" => Ok(true),
            _ => Ok(false),
        }
    }
    pub fn is_close(&self)   -> Result<bool> {
        match fs::read_to_string(self.path.join("is_close"))?.as_str() {
            "1" => Ok(true),
            _ => Ok(false),
        }
    }
    pub fn is_ready(&mut self) -> Result<bool> {
        match fs::read_to_string(self.path.join("ready"))?.as_str() {
            "1" => Ok(true),
            _ => Ok(false),
        }
    }
    pub fn open(&mut self) -> Result<()> {
        fs::write(self.path.join("open"), b"1")?;
        // let now = std::time::SystemTime::now();
        let msec = Duration::from_millis(250);
        for _ in 40..0 {
            if self.is_open()? {
                break;
            }
            std::thread::sleep(msec);
        }
        fs::write(self.path.join("open"), b"0")?;
        // if !self.is_open()? {
        //     let msg = format!("furnace open fail - timeout in {} millis",
        //     now.elapsed().unwrap_or(Duration::from_millis(999999u64)).as_millis());
        //     log::warn!("{}",msg.as_str());
        //     return Err(error::driver_timeout(msg))
        // }
         Ok(())
    }
    pub fn close(&mut self) -> Result<()> {
        fs::write(self.path.join("close"), b"1")?;
        // let now = std::time::SystemTime::now();
        let msec = Duration::from_millis(250);
        for _ in 40..0 {
            if self.is_close()? {
                break;
            }
            std::thread::sleep(msec);
        }
        fs::write(self.path.join("close"), b"0")?;
        // if !self.is_open()? {
        //     let msg = format!("furnace open fail - timeout in {} millis",
        //     now.elapsed().unwrap_or(Duration::from_millis(999999u64)).as_millis());
        //     log::warn!("{}",msg.as_str());
        //     return Err(error::driver_timeout(msg))
        // }
         Ok(())
    }
}
