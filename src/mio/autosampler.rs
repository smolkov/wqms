use std::fmt;
use serde::{Serialize, Deserialize};
use std::cmp::Ordering;
use crate::Result;
use crate::brocker;
// use crate::mio::Valve;




#[derive(Clone, Copy, PartialEq, Eq, Ord, Hash, Serialize, Deserialize)]
pub enum SpeedLevel {
    Value(i32),
    Instant,
}

pub enum Direction{
    Hrizontally,
    Vertically,
    Injection,
}

#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Serialize, Deserialize)]
pub struct Speed(SpeedLevel);

/// The default speed is "normal"
impl Default for Speed {
    fn default() -> Self {
        "normal".into()
    }
}

const MIN_SPEED: i32 = 1;
const MAX_SPEED: i32 = 25;

impl PartialOrd for SpeedLevel {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        use SpeedLevel::*;
        match (*self, *other) {
            (Value(value), Value(ref other_value)) => value.partial_cmp(other_value),
            (Instant, Instant) => Some(Ordering::Equal),
            (Value(_), Instant) => Some(Ordering::Less),
            (Instant, Value(_)) => Some(Ordering::Greater),
        }
    }
}

impl fmt::Display for Speed {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        use SpeedLevel::*;
        match self.0 {
            Value(value) => fmt::Display::fmt(&value, f),
            Instant => write!(f, "\"instant\""),
        }
    }
}

impl fmt::Debug for Speed {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "Speed({})", self)
    }
}

impl PartialEq<i32> for Speed {
    fn eq(&self, other: &i32) -> bool {
        self.eq(&Speed::from(*other))
    }
}

impl PartialOrd<i32> for Speed {
    fn partial_cmp(&self, other: &i32) -> Option<Ordering> {
        self.partial_cmp(&Speed::from(*other))
    }
}

impl<'a> From<&'a str> for Speed {
    fn from(level_name: &'a str) -> Self {
        use SpeedLevel::*;

        Speed(match level_name {
            "slowest" => Value(1),
            "slower" => Value(5),
            "slow" => Value(8),
            "normal" => Value(10),
            "fast" => Value(12),
            "faster" => Value(15),
            "instant" => Instant,
            _ => panic!(
                "Invalid speed specified, use one of the words: \"slowest\", \"slower\", \"slow\", \"normal\", \"fast\", \"faster\", \"instant\""
            ),
        })
    }
}

impl From<i32> for Speed {
    fn from(n: i32) -> Self {
        use SpeedLevel::*;

        Speed(match n {
            // Special error message for 0 because this used to be a valid speed
            0 => panic!("Invalid speed: 0. If you wanted to set the speed to instant, please use the string \"instant\" or Speed::instant()"),
            n if n >= MIN_SPEED && n <= MAX_SPEED => Value(n),
            n => panic!("Invalid speed: {}. Must be a value between {} and {}", n, MIN_SPEED, MAX_SPEED),
        })
    }
}
// We include this implementation because we use f64 in the rest of the library.
// It makes sense to implement this so that they don't get messed up if they accidentally use a
// floating point literal with set_speed.
impl From<f64> for Speed {
    fn from(n: f64) -> Self {
        (n.round() as i32).into()
    }
}
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct Axis {
    speed: u8,
    pos : u32,
    sensor: bool,
    max : u32,
    current: u32,
    name: String,
}

impl Axis{
    pub fn new(axis:&str) -> Axis {
        Axis{
            speed: 1,
            pos: 0,
            sensor: false,
            max: 1800,
            current: 1200,
            name:axis.to_owned(),
        }
    }
    pub fn position(&self) -> Result<u32> {
        Ok(self.pos)
    }
    pub fn sensor(&self) -> Result<bool> {
        Ok(self.sensor)
    }
    pub fn set_speed(&mut self,speed:u8) -> &mut Self{
        self.speed = speed;
        self
    }
    pub fn set_max(&mut self,max:u32) -> Result<()>{
        self.max = max;
        brocker::axis_set_max(&self.name,self.max)?;
        Ok(())
    }
    pub fn set_current(&mut self,current:u32) -> Result<()>{
        self.current = current;
        brocker::axis_set_current(&self.name,self.max)?;
        Ok(())
    }
    pub fn to_pos(&mut self,pos: u32) -> Result<u32> {
        self.sensor = false;
        self.pos = brocker::axis_to_pos(&self.name,pos,self.speed)?;
        Ok(self.pos)
    }
    pub fn add_pos(&mut self,pos: u32) -> Result<u32> {
        self.pos += pos;
        self.sensor = false;
        Ok(self.pos)
    }
    pub fn odd_pos(&mut self,pos: u32) -> Result<u32> {
        self.pos -= pos;
        self.sensor = false;
        Ok(self.pos)
    }
    pub fn to_sensor(&mut self) -> Result<bool> {
        
        self.sensor = true;
        self.pos = brocker::axis_to_sensor(&self.name)?;
        self.sensor = self.pos < 1;
        Ok(self.sensor)
    }
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct Autosampler{
    pub x: Axis,
    pub y: Axis,
    pub z: Axis,
}

impl Autosampler {
    pub fn new()-> Autosampler{
        Autosampler{
            x: Axis::new("x"),
            y: Axis::new("y"),
            z: Axis::new("inj"),
        }
    }
}
#[derive(Debug, Clone, Copy, PartialEq, Serialize, Deserialize)]
pub struct Point {
    /// The x-coordinate of the Point
    pub x: u32,
    /// The y-coordinate of the Point
    pub y: u32,
    /// The z-coordinate of the Point
    pub z: u32,
}

impl Point {
    
}

/// The default speed is "normal"
impl Default for Point {
    fn default() -> Self {
        Point{
            x:0,
            y:0,
            z:0,
        }
    }
}

impl fmt::Display for Point {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "[x:{},y:{},z:{}]",self.x,self.y,self.z)
    }
}