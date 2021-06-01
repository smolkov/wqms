pub fn setup(level:femme::LevelFilter){
    femme::with_level(level);
}
pub fn debug() {
    femme::with_level(femme::LevelFilter::Debug);
}
pub fn trace() {
    femme::with_level(femme::LevelFilter::Trace);
}