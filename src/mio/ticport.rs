/// TicPort
use serde::{Serialize, Deserialize};
use crate::Result;
use crate::brocker;

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct TicPort {
    pub open:  bool,
}


impl TicPort {
    pub fn new() -> TicPort {
        let open = false;
        TicPort{ open }
    }
    pub fn open(&mut self ) -> Result<()> {
        brocker::ticport_open()?;
        self.open = true;
        Ok(())
    }
    pub fn close(&mut self ) -> Result<()> {
        brocker::ticport_close()?;
        self.open = false;
        Ok(())
    }
}