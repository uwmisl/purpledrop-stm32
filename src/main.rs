#![no_std]
#![no_main]

extern crate panic_itm;

use rtfm;
use cortex_m::{iprintln};


#[rtfm::app(device=stm32f4::stm32f413, peripherals = true, monotonic = rtfm::cyccnt::CYCCNT)]
const APP: () = {
    struct Resources {

    }

    #[init()]
    fn init(cx: init::Context) {
        // Cortex-M peripherals
        let mut core = cx.core;

        let stim = &mut core.ITM.stim[0];
        iprintln!(stim, "Hello, world!");

        panic!("Failed successfully!");

    }
};
