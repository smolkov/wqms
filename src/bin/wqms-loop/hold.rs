
use anyhow::Result;
use wqms::mio::Loop as Hal;
use wqms::config::LoopConfig as Config;


pub fn init(_hal:&mut Hal,_cfg: &Config) -> Result<()> {
   
    Ok(())
}

pub fn hold(hal:&mut Hal) -> Result<()> {
    hal.injection_block.open()?;//GP1_STD
    hal.furnace.heat_on()?;
    hal.condensat_pump.start()?;
    for gp in hal.sample_pump.iter_mut() {
        gp.stop()?;
    }
    for v in hal.sample_valve.iter_mut() {
        v.open()?;
    }
    for v in hal.stripping_valve.iter_mut() {
        v.open()?;
    }
    hal.y3y9.open()?;
    hal.y3y10.open()?;
    hal.y4y5.open()?;
    hal.y4y6.open()?;
    hal.tic_valve.open()?;//Y4Y7_NO
    hal.y6y1.open()?;
    // #define ALL_STD do{/* @TODO if restarts are needed: ST1_INITIALIZE; */Y1_STD;LOOP_BLOCK_WAIT;Y4Y7_NO;\
    //     FURNACE_ON;\
    //     GP1_STD; ALL_ST_GP_STD;ALL_ST_Y3_NO;ALL_ST_Y5_NO;\
    //     Y3Y10_NO;Y4Y5_NO;Y4Y6_NO;Y4Y7_NO;Y6Y1_NO;LOOP_BLOCK_WAIT;Y5Y9_NO;Y3Y9_NO;\
    //     }while(0)

    Ok(())
}
pub fn start(hal:&mut Hal,cfg:&Config) -> Result<()>{
    init(hal,cfg)?;
    hold(hal)?;
    Ok(())
}
