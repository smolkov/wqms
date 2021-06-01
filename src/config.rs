// use std::collections::HashMap;
// use std::fs::{create_dir_all, File};
// use std::io::prelude::*;
use std::path::PathBuf;

use anyhow::{ Result};
use config::Config;
// use log::info;
// use rand::Rng;
use serde::{Serialize, Deserialize};


#[derive(Clone, Debug, Deserialize, Serialize)]
pub struct TocConfig{
    pub serial_number: String,
}


impl TocConfig {
    /// This function creates a new configuration instance and
    /// populates it with default values for every option.
    /// If a local config file already exists it is parsed and
    /// overwrites the default option values.
    /// The local config is located at "~/.config/pueue.yml".
    ///
    /// If `require_config` is `true`, an error will be thrown, if no configuration file can be found.
    pub fn new() -> Result<TocConfig> {
        let mut config = Config::new();
        config.set_default("serial_number", "QU2100000")?;

        Ok(config.try_into()?)
    }

}


pub fn read() -> Result<TocConfig> {
    TocConfig::new()
}