// use std::fs;
// use std::io;
// use std::io;
use std::path::PathBuf;
use structopt::StructOpt;

/// β init
#[derive(Debug, StructOpt)]
pub struct Init {
    /// π§ replace
    #[structopt(name = "replace", long = "replace")]
    replace: bool,
    /// π§ git
    #[structopt(name = "git", long = "git")]
    git: bool,
}
/// Init
impl Init {
    /// is replace on setup
    #[inline]
    pub fn replace(&self) -> bool {
        self.replace
    }
    /// is git setup
    #[inline]
    pub fn git(&self) -> bool {
        self.git
    }
}

/// β set prop signal
#[derive(Debug, StructOpt)]
pub struct SetProp {
    //β± interval in seconds
    #[structopt(short = "d", long = "name")]
    name: PathBuf,
    ///π hardware connection address
    #[structopt(
        short = "a",
        long = "address",
        default_value = "tcp:host=192.168.66.59,port=6666"
    )]
    value: String,
}

/// β get property value
#[derive(Debug, StructOpt)]
pub struct GetProp {
    /// π§ name
    #[structopt(short = "d", long = "name")]
    name: PathBuf,
}
/// β list show workspace
#[derive(Debug, StructOpt)]
pub struct List {}

///π’ Commands
#[derive(StructOpt, Debug)]
pub enum Cmd {
    /// β Hold
    Hold,
    /// β Measurement
    Measurement,
    /// β Calibration
    Calibration,
    /// Online
    Online,
}

///automata command argument
#[derive(Debug, StructOpt)]
#[structopt(name = "tocxy", about = "π§°tocxy console interface usage.")]
pub struct Args {
    /// Verbose mode (-v, -vv, -vvv)
    #[structopt(short, long, parse(from_occurrences))]
    pub verbose: u8,
    ///π’ commands
    #[structopt(subcommand, about = "π’automata commands list")]
    pub cmd: Cmd,
}