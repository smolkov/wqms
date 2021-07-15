// use wqm::Method;
use std::time::Duration;
use wqms::Result;
use wqms::mio::Xy as Hal;
use wqms::config::XyStream as Stream;
use wqms::config::XyConfig as Config;
// use std::thread::sleep;
use std::thread::sleep;

pub fn prepare_sampling(xy: &mut Hal,stream:&Stream) -> Result<()> {
    let pump = match stream.number {
        1 => &mut xy.sample_pump[0],
        2 => &mut xy.sample_pump[1],
        3 => &mut xy.sample_pump[2],
        4 => &mut xy.sample_pump[3],
        5 => &mut xy.sample_pump[4],
        6 => &mut xy.sample_pump[5],
        _ => &mut xy.sample_pump[0],
 
    };
    pump.start()?;
    sleep(Duration::from_secs(stream.sampling_sec));
    pump.stop()?;
    sleep(Duration::from_secs(stream.striping_sec));
    Ok(())
}

pub fn rinsing(xy: &mut Hal,cfg:&Config,stream:&Stream ) -> Result<()> {
    xy.x.to_pos(stream.drain.xpos)?;
    xy.y.to_pos(stream.drain.ypos)?;
    xy.inj.to_pos(cfg.zhold)?;
    xy.inj.to_sensor()?;
    xy.injection_valve.close()?;
    xy.inj.to_pos(cfg.zhold)?;
    xy.inj.add_pos(cfg.rinse)?;
    xy.injection_valve.open()?;
    xy.inj.to_pos(cfg.zhold)?;
    xy.y.to_pos(cfg.yhold)?;
    Ok(())
}

pub fn dilution(xy: &mut Hal,cfg:&Config,stream:&Stream) -> Result<()> {
    xy.y.to_pos(cfg.yhold)?;
    xy.x.to_pos(stream.sample.xpos)?;
    xy.y.to_pos(stream.sample.ypos)?;
    Ok(())
}