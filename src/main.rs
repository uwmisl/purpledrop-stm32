#![no_std]
#![no_main]
#![feature(lang_items)]

extern crate alloc;
extern crate alloc_cortex_m;
extern crate panic_itm;

use heapless::{
    spsc::{Consumer, Producer, Queue},
};

use cortex_m::{
    iprintln, Peripherals, 
    asm:: {
        delay,
    },
    interrupt,
    interrupt:: {
        Mutex,
    }
};
use cortex_m_rt::entry;

use stm32f4xx_hal::prelude::*;
use stm32f4xx_hal::otg_fs::{USB, UsbBus};
use stm32f4xx_hal::stm32 as target_device;

use typenum::consts::U16;

use usbd_serial::{SerialPort, USB_CLASS_CDC};
use usb_device::prelude::*;

use pd_driver_messages::{
    Parser,
    serialize_msg,
    messages::*
};

static mut EP_MEMORY: [u32; 1024] = [0; 1024];
//static mut RXMSGQ: Mutex<Cell<Queue<Message, U16>>> = Mutex::new(Cell::new(Queue(heapless::i::Queue::new())));
static mut RXMSGQ: Queue<Message, U16> = Queue(heapless::i::Queue::new());
#[global_allocator]
static ALLOCATOR: alloc_cortex_m::CortexMHeap = alloc_cortex_m::CortexMHeap::empty();

fn poll_messages(
    buf: &[u8],
    queue: &mut Producer<'static, Message, U16>,
    parser: &mut Parser,
    itm: &mut cortex_m::peripheral::ITM)
{
    iprintln!(&mut itm.stim[0], "Polling for message data");
    for b in buf {
        match parser.parse(*b) {
            Ok(result) => {
                if let Some(msg) = result {
                    queue.enqueue(msg).ok();
                }
            },
            Err(_err) => iprintln!(&mut itm.stim[0], "Error parsing receive data"),
        }
    }
}


#[entry]
fn main() -> ! {
    // Initialize the alloc-cortex global heap allocator
    let start = cortex_m_rt::heap_start() as usize;
    let size = 1024; // in bytes
    unsafe { ALLOCATOR.init(start, size) }
    
    //static mut RXMSGQ: Queue<Message, U16> = Queue(heapless::i::Queue::new());
    let mut message_queue_producer: Producer<'static, Message, U16>;
    let mut message_queue_consumer: Consumer<'static, Message, U16>;
    unsafe {
        let (p,c) = RXMSGQ.split();
        message_queue_consumer = c;
        message_queue_producer = p;
    }
    // interrupt::free(|cs| { 
    //     let (mut p, mut c) = RXMSGQ.borrow(cs).get_mut().split();
    //     message_queue_consumer = c;
    //     message_queue_producer = p;
    // });
    
    let core = cortex_m::Peripherals::take().unwrap();

    let device = target_device::Peripherals::take().unwrap();
    
    let rcc = device.RCC;
    // Configure PLL settings to run at 100MHz sysclk
    let 
    clocks = rcc.constrain().cfgr
        .use_hse(8.mhz())
        .sysclk(100.mhz())
        .require_pll48clk()
        .freeze();

    let mut itm = core.ITM;
    iprintln!(&mut itm.stim[0], "Hello, world!");

    let gpioa = device.GPIOA.split();
    // Pull-down the DP pin briefly so that host can detect when we reset
    // This is primarily to support debugging
    let mut usb_dp = gpioa
    .pa12
    .into_push_pull_output();
    usb_dp.set_low().ok();
    delay(clocks.sysclk().0 / 100);

    let usb_dm = gpioa.pa11.into_alternate_af10();
    let usb_dp = usb_dp.into_alternate_af10();

    let usb = USB {
        usb_global: device.OTG_FS_GLOBAL,
        usb_device: device.OTG_FS_DEVICE,
        usb_pwrclk: device.OTG_FS_PWRCLK,
        pin_dm: usb_dm,
        pin_dp: usb_dp,
    };
    let usb_bus = UsbBus::new(usb, unsafe { &mut EP_MEMORY });

    let mut serial = SerialPort::new(&usb_bus);

    let mut usb_dev = UsbDeviceBuilder::new(&usb_bus, UsbVidPid(0x16c0, 0x27d0))
        .device_class(USB_CLASS_CDC)
        .device_sub_class(2)
        .manufacturer("UW MISL")
        .product("PurpleDrop")
        .serial_number("A1")
        .build();

    let mut parser = Parser::new();
    loop {
        if !usb_dev.poll(&mut [&mut serial]) {
            continue;
        }
        let mut buf = [0u8; 64];

        match serial.read(&mut buf[..]) {
            Ok(count) => {
                poll_messages(&buf[0..count], &mut message_queue_producer, &mut parser, &mut itm);
            },
            Err(UsbError::WouldBlock) => (),// No data received
            Err(err) => iprintln!(&mut itm.stim[0], "Error on USB serial read: {:?}", err) 
        };

        // match serial.write(&[0x3a, 0x29]) {
        //     Ok(count) => {
        //         // count bytes were written
        //     },
        //     Err(UsbError::WouldBlock) => // No data could be written (buffers full)
        //     Err(err) => // An error occurred
        // };
    }
}

// define how Out Of Memory (OOM) conditions should be handled
// *if* no other crate has already defined `oom`
#[lang = "oom"]
#[no_mangle]
pub fn rust_oom(_layout: core::alloc::Layout) -> ! {
    panic!("Heap allocator ran out of memory");
}
