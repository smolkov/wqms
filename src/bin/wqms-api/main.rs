// use std::{env, time::Duration};
// use tide::{sessions::SessionMiddleware, Redirect};

// pub mod records;
// mod templates;
// use handlebars::Handlebars;
// use tide_handlebars::prelude::*;
// use async_std::sync::Arc;
// use async_std::task;
// use tide::{sessions::SessionMiddleware, Redirect};

#[derive(Clone)]
pub struct State {
    // db: SqlitePool,
    // client: redis::Client,
    // registry: Arc<Handlebars<'static>>,
}

impl State {
   pub fn new()  -> State {
       State{}
   }
}

pub type Request = tide::Request<State>;

#[async_std::main]
async fn main() -> tide::Result<()> {
    tide::log::start();
    // create tunnel
    let mut app = tide::with_state(State::new());
    app.with(tide::sessions::SessionMiddleware::new(
        tide::sessions::MemoryStore::new(),
        std::env::var("TIDE_SECRET")
            .expect("Please provide a TIDE_SECRET value of at \
                      least 32 bytes in order to run this example",
            )
            .as_bytes(),
    )); 

   // Redirect hackers to YouTube.
    app.at("/.env").get(tide::Redirect::new("https://www.youtube.com/watch?v=dQw4w9WgXcQ"));
    app.at("/public").serve_dir("www/public/")?;
    app.listen("127.0.0.1:8000").await?;
    Ok(())
}