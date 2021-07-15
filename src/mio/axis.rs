/// XY-System autosampler
///
///
///
// use std::prelude::*;
// use std::io;
use super::Interface;
use std::fs;
// use std::stream::Stream;
use std::path::PathBuf;
use serde::{Deserialize, Serialize};
use crate::Result;

// use std::time::Duration;
// use std::prelude::*;

pub const VELOCITY: &'static str = "velocity";
pub const CURRENT: &'static str = "current";
pub const POSITION: &'static str = "position";
pub const HOLDPOS: &'static str = "hold_position";
pub const MAXPOS: &'static str = "max";
pub const FINALPOS: &'static str = "final_position";
pub const INVERTFP: &'static str = "invert_final_position";
pub const COMMAND: &'static str = "command";
pub const GOTO: &'static str = "goto";


/// ✓
#[derive(Clone,Deserialize, Serialize, Debug)]
pub struct Axis {
    pub path: PathBuf,
}

impl From<Interface> for Axis {
    #[inline]
    fn from(device: Interface) -> Axis {
        Axis {
            path: device.path.to_path_buf(),
        }
    }
}

/// Axis interface..
impl Axis {
    // type Velocity;
    pub fn position(&self) -> Result<u32> {
        let position = fs::read_to_string(self.path.join(POSITION))?.parse::<u32>()?;
        Ok(position)
    }
    /// read current mAh defaul :1200
    pub fn current(&mut self) -> Result<u32> {
        let max = fs::read_to_string(self.path.join(CURRENT))?.parse::<u32>()?;
        Ok(max)
    }
    pub fn max(&self) -> Result<u32> {
        let max = fs::read_to_string(self.path.join(MAXPOS))?.parse::<u32>()?;
        Ok(max)
    }
    pub fn velocity(&self) -> Result<u64> {
        let velocity = fs::read_to_string(self.path.join(VELOCITY))?.parse::<u64>()?;
        Ok(velocity)
    }
    pub fn final_position(&self) -> Result<bool> {
        match fs::read_to_string(self.path.join(FINALPOS))?.as_str() {
            "1" => Ok(true),
            _ => Ok(false),
        }
    }

    pub fn to_pos(&self, goto: u32) -> Result<()> {
        fs::write(self.path.join(GOTO), format!("{}", goto).as_bytes())?;
        // let pos  =  self.position()?;
        // let velocity = self.velocity()?;
        // let  diff =  if pos > goto {
        //     pos - goto
        // }else {
        //     goto - pos
        // };
        // let now = std::time::SystemTime::now();
        // let ten_millis = Duration::from_millis(velocity);
        // for _ in diff..0 {
        //     if goto == self.position()? {
        //         break;
        //     }
        //     std::thread::sleep(ten_millis);
        // }
        // if goto != self.position()? {
        //     let label = Interface::from(self).label()?;
        //     let msg = format!("axis:{} move from {} to {} pos:{} error - timeout in {} millis",label,pos,goto,self.position()?,
        //     now.elapsed().unwrap_or(Duration::from_millis(999999u64)).as_millis());
        //     log::warn!("{}",msg.as_str());
        //     return Err(error::driver_timeout(msg))
        // }
        Ok(())
    }
    pub fn to_sensor(&self) -> Result<()> {
        fs::write(self.path.join(COMMAND), b"1")?;
        // let pos  =  self.position()?;
        // let velocity = self.velocity()?;
        // let now = std::time::SystemTime::now();
        // let ten_millis = Duration::from_millis(velocity);
        // for _ in pos..0 {
        //     if self.final_position()? {
        //         break;
        //     }
        //     std::thread::sleep(ten_millis);
        // }
        // if !self.final_position()? {
        //     let label = Interface::from(self).label()?;
        //     let msg = format!("axis:{} command go to sensor fail pos:{} final:{} - timeout in {} millis",label,self.position()?,self.final_position()?,
        //     now.elapsed().unwrap_or(Duration::from_millis(999999u64)).as_millis());
        //     log::warn!("{}",msg.as_str());
        //     return Err(error::driver_timeout(msg))
        // }
        Ok(())
    }
    pub fn add_pos(&mut self, pos: u32) -> Result<u32> {
        let current = self.position()?;
        let max = self.max()?;
        let goto = if current + pos < max {
            current + pos
        } else {
            max
        };
        self.to_pos(goto)?;
        self.position()
    }
    pub fn odd_pos(&mut self, pos: u32) -> Result<u32> {
        let current = self.position()?;
        let goto = if current > pos { current - pos } else { 0 };
        self.to_pos(goto)?;
        self.position()
    }
    pub fn set_invert_final_position(&self, invert: u8) -> Result<()> {
        fs::write(self.path.join(INVERTFP), format!("{}", invert).as_bytes())?;
        Ok(())
    }
    pub fn set_max(&mut self, max: u32) -> Result<()> {
        fs::write(self.path.join(MAXPOS), format!("{}", max).as_bytes())?;
        Ok(())
    }
    pub fn set_current(&self, current: u32) -> Result<()> {
        fs::write(self.path.join(CURRENT), format!("{}", current).as_bytes())?;
        Ok(())
    }
    pub fn set_velocity(&self, velocity: u64) -> Result<()> {
        fs::write(self.path.join(VELOCITY), format!("{}", velocity).as_bytes())?;
        Ok(())
    }
}
