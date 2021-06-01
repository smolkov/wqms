/// Monitor gear pump normally used for solution sampling.
///
///
use crate::Result;
use serde::{Deserialize, Serialize};
use crate::brocker;
#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
enum State{
    Start,
    Stop
}

#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
pub struct Pump {
    state: State,
    name: String,
}

impl Pump {
    pub fn new(name:&str) -> Pump {
        Pump { 
            state: State::Stop,
            name:name.to_owned(),
           }
    }
    pub fn start(&mut self) -> Result<()> {
        self.state = State::Start;
        brocker::pump_start(&self.name,0)?;
        Ok(())
    }
    pub fn stop(&mut self) -> Result<()> {
        self.state = State::Stop;
        brocker::pump_stop(&self.name)?;
        Ok(())
    }
}


