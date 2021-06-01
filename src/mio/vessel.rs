use serde::{Deserialize, Serialize};
// use crate::Result;

#[derive(Clone, Deserialize, Serialize, PartialEq, Debug,Default)]
pub struct Vessel {
    pub xpos: u32,
    pub ypos: u32,
    pub needle: u32,
}
