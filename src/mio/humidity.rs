/// Humidity sensor
/// Anschlus Analog:IN04
///
use serde::{Deserialize, Serialize};
use super::Result;

#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
pub struct Humidity {
    pub value: f32,
    pub broken:bool,

}

impl Humidity {
    pub fn new() -> Humidity{
        Humidity{
            value:0.0,
            broken: true,
        }
    }
}
