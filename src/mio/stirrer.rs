
use serde::{Deserialize, Serialize};
use crate::Result;

#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
pub struct Stirrer {
    name: String,
    run: bool,
    delay: u32,
    current: u32,
}

impl Stirrer {
    pub fn new(name:&str) -> Stirrer{
        Stirrer{
            name: name.to_owned(),
            run: false,
            delay: 50,
            current: 200,
        }
    }
    pub fn start(&mut self) -> Result<()> {
        self.run = true;
        Ok(())
    }
    pub fn set_delay(&mut self,delay:u32) -> Result<()> {
        self.delay = delay;
        Ok(()) 
    }
    pub fn set_current(&mut self,current:u32) -> Result<()>{
        self.current = current;
        Ok(()) 
    }
    pub fn stop(&mut self) -> Result<()>{
        self.run = false;
        Ok(()) 
    }
}



