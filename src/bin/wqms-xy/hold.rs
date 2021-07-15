
use anyhow::Result;
use wqms::mio::Xy as Hal;
use wqms::config::XyConfig as Config;


pub fn init(xy:&mut Hal,cfg: &Config) -> Result<()> {
    let xmax = cfg.xmax;
    let ymax = cfg.ymax;
    let zmax = cfg.zmax;
    let xcurrent = cfg.xcurrent;
    let ycurrent = cfg.ycurrent;
    let zcurrent = cfg.zcurrent;

    let stirrer_current =  cfg.stirrer_current;
    let stirrer_delay   =  cfg.stirrer_delay;

    xy.x.set_current(xcurrent)?;
    xy.x.set_max(xmax)?;
    xy.x.set_velocity(1)?;
    xy.y.set_current(ycurrent)?;
    xy.y.set_max(ymax)?;
    xy.y.set_velocity(1)?;
    xy.inj.set_current(zcurrent)?;
    xy.inj.set_max(zmax)?;
    xy.inj.set_velocity(1)?;
    xy.stirrer.stop()?;
    xy.stirrer.set_current(stirrer_current)?;
    xy.stirrer.set_delay(stirrer_delay)?;
    xy.stirrer.start()?;
    Ok(())
}

pub fn hold(xy:&mut Hal,cfg: &Config) -> Result<()> {
    let xhold = cfg.xhold;
    let yhold = cfg.yhold;
    // let z_hold = 10;
    xy.condensat_pump.start()?;
    for gp in xy.sample_pump.iter_mut() {
        gp.stop()?;
    }
    xy.y.to_sensor()?;
    xy.y.set_velocity(1)?;
    xy.y.to_pos(xhold)?;
    xy.x.to_sensor()?;
    xy.x.set_velocity(1)?;
    xy.x.to_pos(yhold)?;
    xy.furnace.close()?;
    xy.furnace.heat_on()?;
    xy.stirrer.start()?;
    Ok(())
}
pub fn start(xy:&mut Hal,cfg:&Config) -> Result<()>{
    init(xy,cfg)?;
    hold(xy,cfg)?;
    Ok(())
}
