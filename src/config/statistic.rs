use serde::{Deserialize, Serialize};

#[derive(Clone, Deserialize, Serialize, PartialEq, Debug)]
pub struct Statistic {
    pub replicates: u8,
    pub outlier: u8,
    pub cv_max: f32,
    pub jump: f32,
    pub replicate: u8,
}

impl Default for Statistic {
    fn default() -> Self {
        Self {
            replicates: 1,
            outlier: 0,
            cv_max: 0.0,
            jump: 0.0,
            replicate: 0,
        }
    }
}

impl Statistic {
    pub fn new(replicates: u8, outlier: u8) -> Self {
        Self {
            replicates: replicates,
            outlier: outlier,
            cv_max: 0.0,
            jump: 0.0,
            replicate: 0,
        }
    }
}

