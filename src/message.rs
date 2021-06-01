use serde::{Serialize, Deserialize};

#[derive(Clone, Debug, Deserialize, Serialize)]
pub struct MoveMessage{
    pub axis: u8,
    pub pos: u32,
    pub speed: u32,
    pub sensor: bool,
}


/// The Message used to add a new command to the daemon.
#[derive(Clone, Debug, Deserialize, Serialize)]
pub enum Message {
    AxisMove(MoveMessage),
    AxisCurrent(u8,u32),
    AxisMax(u8,u32),
    PumpStart(u8),
    PumpStop(u8),
    ValveOpen(u8),
    ValveClose(u8),
}


