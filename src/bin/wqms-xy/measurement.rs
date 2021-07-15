
use std::time::{Duration};
use std::thread::sleep;
use tocxy::Result;
use tocxy::config::{XyConfig,XyStream};
use tocxy::mio::Xy;



pub fn measurement(xy: &mut Xy,cfg:&XyCofig, stream:&XyStream) -> Result<()> {
    let mut need_rinsing = true;
    // let mut measurement = Measurement::new(stream.number);
    // while measurement.next_measurement(stream)? {
        xy.y.to_pos(xy.settings.yhold)?;
        xy.x.to_pos(xy.settings.xhold)?;
        xy.x.to_sensor()?;
        xy.y.to_sensor()?;
        xy.y.to_pos(xy.settings.yhold)?;
        xy.x.to_pos(xy.settings.xhold)?;
        if need_rinsing {
           super::flow::rinsing(xy,stream)?;
        }
        let volume = if measurement.is_tic {
            stream.tc_volume
        } else {
            stream.tic_volume
        };

        xy.autosampler.x.to_pos(stream.sample.xpos)?;
        xy.autosampler.z.add_pos(stream.air)?; 
        xy.autosampler.y.to_pos(stream.sample.ypos)?;
        xy.autosampler.z.add_pos(volume)?;
        xy.autosampler.y.to_pos(xy.settings.yhold)?;
        
        if ! measurement.is_tic {
            xy.codo.start(&stream.codo.integration)?;
            xy.tnb.start(&stream.tnb.integration)?;
            xy.ndir1.start(&stream.tc.integration)?;
            xy.ndir2.start(&stream.tc.integration)?;
        }else {
            xy.ndir1.start(&stream.tic.integration)?;
            xy.ndir2.start(&stream.tic.integration)?;
        }
        xy.ndir1.justification()?;
        xy.ndir1.justification()?;
        xy.codo.justification()?;
        xy.tnb.justification()?;

        loop {
            let mut wait = !xy.ndir1.justification_done()?as u8;
            wait += !xy.ndir2.justification_done()? as u8;
            wait += !xy.tnb.justification_done()? as u8;
            wait += !xy.codo.justification_done()?as u8;
            sleep(Duration::from_millis(200));
            if wait == 0  {
                break;
            }
        }        

        if ! measurement.is_tic {
            xy.autosampler.x.to_pos(xy.settings.furnace.xpos)?;
            xy.autosampler.y.to_pos(xy.settings.furnace.needle)?;
            xy.furnace.open()?;
            xy.autosampler.y.to_pos(xy.settings.furnace.ypos)?;
            xy.autosampler.z.odd_pos(volume)?;
            sleep(Duration::from_secs(stream.injection_wait));
            xy.ndir1.integration()?;
            xy.ndir1.integration()?;
            xy.codo.integration()?;
            xy.tnb.integration()?; 
            xy.autosampler.y.to_pos(xy.settings.furnace.needle)?;
            xy.furnace.close()?; 
           
        }else {
            xy.autosampler.x.to_pos(xy.settings.ticport.xpos)?;
            xy.autosampler.y.to_pos(xy.settings.ticport.needle)?;
            xy.ticport.open()?;
            xy.autosampler.y.to_pos(xy.settings.ticport.ypos)?;
            xy.autosampler.z.odd_pos(volume)?;
            xy.ndir1.integration()?;
            xy.ndir1.integration()?;
            xy.autosampler.y.to_pos(xy.settings.ticport.needle)?;
            xy.ticport.close()?; 
        };
        xy.autosampler.y.to_pos(xy.settings.yhold)?;
        xy.autosampler.x.to_pos(xy.settings.xhold)?;
        xy.autosampler.y.to_sensor()?;
        xy.autosampler.x.to_sensor()?;
        xy.autosampler.y.to_pos(xy.settings.yhold)?;
        xy.autosampler.x.to_pos(xy.settings.xhold)?;
        crate::flow::rinsing(xy,stream)?;
        need_rinsing = false; 
        loop {
            let mut wait = !xy.ndir1.integration_done()?as u8;
            wait += !xy.ndir2.integration_done()? as u8;
            wait += !xy.tnb.integration_done()? as u8;
            wait += !xy.codo.integration_done()?as u8;
            sleep(Duration::from_millis(200));
            if wait == 0  {
                break;
            }
        }        
        if ! measurement.is_tic {
            if stream.tc.sensor == xy.ndir2.name {
                measurement.tc.add(xy.ndir2.area()? as f64)?;
            }else {
                measurement.tc.add(xy.ndir1.area()? as f64)?;
            }
            measurement.tc.add(xy.ndir1.area()? as f64)?;
            measurement.tnb.add(xy.tnb.area()? as f64)?;
            measurement.codo.add(xy.codo.area()? as f64)?;
        }else {
            if stream.tic.sensor == xy.ndir2.name {
                measurement.tic.add(xy.ndir2.area()? as f64)?;
            }else {
                measurement.tic.add(xy.ndir1.area()? as f64)?;
            }
        }
    // }
    Ok(measurement)
}

pub fn start(xy:&mut Xy,cfg:&XyConfig,stream: &XyStream) -> Result<()>{

    measurement(xy, cfg,stream)?;

    Ok(())
}