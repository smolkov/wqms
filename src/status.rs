use std::time::Duration;
use serde::{Serialize, Deserialize};
use crate::Result;
use std::thread::sleep;


#[derive(Clone, Deserialize, Serialize, PartialEq, Debug)]
pub enum Status {
    Stop,
    Online,
    Calibration,
}

/// Measurement method 
#[derive(Clone, Deserialize, Serialize, Debug)]
pub struct Analyser {
    pub serial: String,
    pub status: Status,
}

impl Analyser {
    pub fn new() -> Analyser {
        Analyser {
            serial: "SERIAL".to_owned(),
            status: Status::Stop,
        }
    }
    pub fn online(&mut self) -> Result<()> {
        self.status = Status::Online;
        Ok(())
    }
    pub fn stop(&mut self) -> Result<()> {
        self.status = Status::Stop;
        Ok(())
    }
    pub fn wait(&mut self,duration:Duration) -> Result<()> {
        sleep(duration);
        Ok(())
    }
}


