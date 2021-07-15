
use anyhow::Result;
use wqms::mio::Uv as Hal;
use wqms::config::UvConfig as Config;


pub fn init(_hal:&mut Hal,_cfg: &Config) -> Result<()> {
   
    Ok(())
}

pub fn hold(_hal:&mut Hal) -> Result<()> {
    Ok(())
}
pub fn start(hal:&mut Hal,cfg:&Config) -> Result<()>{
    init(hal,cfg)?;
    hold(hal)?;
    Ok(())
}
