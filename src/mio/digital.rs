/// Digital DigIN DigOUT interface 
/// 
// use std::prelude::*;
// use std::io;
use std::fs;
use std::fmt;
// use std::task;
// use sysfs_gpio::{Direction,Pin};
use std::path::{PathBuf};
use super::*;
use serde::{Serialize, Deserialize};
pub const VALUE: &'static str = "value";

// use std::convert::TryFrom;
/// interface transfer
// impl TryFrom<Interface> for DigIN {
//     fn try_from(iface: Interface) -> Result<Self> {
//         iface.set_itype(IType::DigIN)?;
//         Ok(Self{
//             path:iface.path,
//         })
//     }
// }

/// DigIN output
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct DigIN {
    pub path:  PathBuf,
}

impl DigIN{
    /// Check pin 
    pub fn is_low(&self) -> Result<bool> {
        match fs::read_to_string(self.path.join(VALUE))?.as_str() {
            "1" => Ok(false),
            _ => Ok(true),
        }
    }
    pub fn is_high(&self) -> Result<bool>{
        match fs::read_to_string(self.path.join(VALUE))?.as_str() {
            "1" => Ok(true),
            _ => Ok(false),
        }
    }
}

/// Digital output
pub struct DigOUT {
    pub path:  PathBuf,
}

impl From<&Interface> for DigOUT {
    #[inline]
    fn from(device:&Interface) -> DigOUT {
        DigOUT{
            path: device.path.to_path_buf()
        }
    }
}

impl From<&Interface> for DigIN {
    #[inline]
    fn from(device:&Interface) -> DigIN {
        DigIN{
            path: device.path.to_path_buf()
        }
    }
}
impl From<&DigOUT> for DigIN {
    #[inline]
    fn from(out:&DigOUT) -> DigIN {
        DigIN{
            path: out.path.to_path_buf()
        }
    }
}

/// interface transfer
// impl TryFrom<Interface> for DigOUT {
//     fn try_from(iface: Interface) -> Result<Self> {
//         iface.set_itype(IType::DigOUT)?;
//         Ok(Self{
//             path:iface.path,
//         })
//     }
// }


impl DigOUT{
    fn set_low(&self) -> Result<()> {
        fs::write(self.path.join(VALUE), b"0")?;
        Ok(())
    }
    fn set_high(&self) -> Result<()> {
        fs::write(self.path.join(VALUE), b"1")?;
        Ok(())
    }
    pub fn toggle(&self) -> Result<bool> {
        let input = DigIN::from(self);
        if input.is_high()? {
            self.set_low()?;
            Ok(false)
        }else {
            self.set_high()?;
            Ok(true)
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Direction {
    In,
    Out,
    High,
    Low,
}

impl From<u8> for Direction {
    fn from(value: u8) -> Self {
        match value {
            0 => Direction::In,
            1 => Direction::Out,
            2 => Direction::High,
            3 => Direction::Low,
            _ => Direction::In,
        }
    }
}

impl From<String> for Direction {
    fn from(value: String) -> Self {
        match value.trim() {
            "in"   =>  Direction::In,
            "out"  =>  Direction::Out,
            "high" =>  Direction::High,
            "low"  =>  Direction::Low,
            _      =>  Direction::In
        }
    }
}
impl From<&str> for Direction {
    fn from(value: &str) -> Self {
        match value {
            "in"   =>  Direction::In,
            "out"  =>  Direction::Out,
            "high" =>  Direction::High,
            "low"  =>  Direction::Low,
             _     =>  Direction::In
        }
    }
}
impl fmt::Display for Direction {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            Direction::In =>  return write!(f,"in"),
            Direction::Out => return write!(f,"out"),
            Direction::High =>return write!(f,"high"),
            Direction::Low => return write!(f,"low"),
        }
    }
}