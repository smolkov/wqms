pub mod hold;
// pub mod flow;
// pub mod calibration;
// pub mod measurement;
// pub mod check;
// pub mod online;
pub mod cli;

use cli::Args;
use cli::Cmd;
use structopt::*;

use anyhow::Result;

use wqms::mio::Uv as Hal;
use wqms::config::UvConfig as Config;

fn main() -> Result<()> {

    wqms::logger::debug();
    // let conf = tocxy::config::read()?;
    // let mut xy = tocxy::mio::simulation()?;
    // let mut tocxy = tocxy::TocXY::new(xy,conf);
    let args = Args::from_args();
    let mut mio = wqms::mio::Mio::new(".test")?;
    let mut hal = Hal::new(&mut mio)?;
    let cfg = Config::default();
    // let stream = Stream::default();
    match args.cmd {
       Cmd::Hold => hold::start(&mut hal,&cfg)?,
       _ => (()),
    //    Cmd::Measurement => measurement::start(&mut xy,&stream)?,
    //    Cmd::Calibration => calibration::start(&mut xy,&stream)?,
    //    Cmd::Online => online::start(&mut xy)?,
    }
    Ok(())
}