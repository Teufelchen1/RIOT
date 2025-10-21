#![no_std]
#![feature(type_alias_impl_trait)]
//use riot_wrappers::cstr::cstr;
use riot_wrappers::riot_sys;
use riot_wrappers::{mutex::Mutex, println};
/*
use crate::riot_sys::inline::netdev;
use crate::riot_sys::inline::netdev_register;
use crate::riot_sys::inline::netdev_trigger_event_isr;
use crate::riot_sys::inline::netopt_t;
use crate::riot_sys::inline::NETDEV_EVENT_LINK_UP;
use crate::riot_sys::inline::NETDEV_EVENT_RX_COMPLETE;
use crate::riot_sys::inline::NETDEV_SLIPDEV;

use crate::riot_sys::NETDEV_TYPE_SLIP;
use crate::riot_sys::{
    netopt_t_NETOPT_DEVICE_TYPE as NETOPT_DEVICE_TYPE, netopt_t_NETOPT_IS_WIRED as NETOPT_IS_WIRED,
};

use riot_wrappers::riot_sys::{
    gnrc_netif_t, isrpipe_write_one, libc::c_void, netdev_driver_t, netdev_t, stdin_isrpipe,
    uart_init, uart_t, uart_write,
};
*/
use riot_wrappers::riot_sys::libc::c_void;
use riot_wrappers::riot_sys::{isrpipe_write_one, stdin_isrpipe};
use riot_wrappers::uart::UartDevice;
// use riot_wrappers::thread;
use slipmux;
use slipmux::Constants;
use slipmux::Decoder;
use slipmux::FrameHandler;
use slipmux::FrameType;

fn write_byte(uart: &mut UartDevice, byte: u8) {
    uart.write(&[byte]);
    //unsafe { uart_write(uart.dev, [byte].as_ptr(), 1) };
}

fn write_escaping_bytes(uart: &mut UartDevice, bytes: &[u8]) {
    for byte in bytes {
        match *byte {
            Constants::END => {
                write_byte(uart, Constants::ESC);
                write_byte(uart, Constants::ESC_END);
            }
            Constants::ESC => {
                write_byte(uart, Constants::ESC);
                write_byte(uart, Constants::ESC_ESC);
            }
            _ => {
                write_byte(uart, *byte);
            }
        }
    }
}

// #[derive(Debug)]
// pub struct UARTDevice {
//     dev: uart_t,
// }

// impl UARTDevice {
//     pub fn new(dev: uart_t) -> Self {
//         UARTDevice { dev }
//     }
// }

static SLIPUART: static_cell::StaticCell<UartDevice> = static_cell::StaticCell::new();
static SLIPMUX: static_cell::StaticCell<Decoder> = static_cell::StaticCell::new();
static HANDLER: static_cell::StaticCell<RiotSlipmuxFramehandler> = static_cell::StaticCell::new();
static mut STDOUT: Option<&'static mut UartDevice> = None;
static mut HANDLER_PTR: Option<&'static mut RiotSlipmuxFramehandler> = None;
static WRITE_SYNC: Mutex<()> = Mutex::new(());
static mut COAPBUFFER: [u8; 2048] = [0; 2048];
static mut NETBUFFER: [u8; 2048] = [0; 2048];

fn uart_cb(byte: u8) {
    println!("SLIPMUX received: {}", byte);
}

static mut CB: &'static fn(u8) = &(uart_cb as fn(u8));

#[no_mangle]
pub extern "C" fn auto_init_slipmux() {
    if let Err(e) = init() {
        println!("SLIPMUX init error: {}", e);
    }
}

#[no_mangle]
pub extern "C" fn stdio_init() {
    //let result = unsafe { uart_init(UARTDevice::new(0).dev, 115200, None, core::ptr::null_mut()) };
}

#[no_mangle]
extern "C" fn stdio_write(
    buffer: *const riot_sys::libc::c_void,
    len: riot_sys::size_t,
) -> riot_sys::size_t {
    let _ = WRITE_SYNC.lock();
    let mut uart = unsafe { &mut STDOUT };
    // unsafe: data is initialized per API
    let data = unsafe { core::slice::from_raw_parts(buffer as *const u8, len as _) };
    if let Some(ref mut uart2) = uart {
        write_byte(uart2, Constants::DIAGNOSTIC);
        write_escaping_bytes(uart2, data);
        write_byte(uart2, Constants::END);
        len
    } else {
        0
    }
}

// extern "C" fn cb(arg: *mut c_void, byte: u8) {
//     let decoder = unsafe { &mut *(arg as *mut Decoder) };
//     unsafe {
//         if let Some(ref mut handler) = HANDLER_PTR {
//             decoder.decode(byte, *handler);
//         }
//     }
//     // todo!
// }

pub struct RiotSlipmuxFramehandler<'a> {
    /// Current offset from start of the buffer
    pub index: usize,
    frame_type: Option<FrameType>,
    /// Destination of the current configuration frame, including the FCS checksum
    pub configuration_buffer: &'a mut [u8],
    /// Destination of the current IP packet
    pub packet_buffer: &'a mut [u8],
}

impl<'a> RiotSlipmuxFramehandler<'a> {
    /// Creates a new handler
    #[must_use]
    pub const fn new(configuration_buffer: &'a mut [u8], packet_buffer: &'a mut [u8]) -> Self {
        Self {
            index: 0,
            frame_type: None,
            configuration_buffer,
            packet_buffer,
        }
    }
}

impl FrameHandler for RiotSlipmuxFramehandler<'_> {
    fn begin_frame(&mut self, frame_type: FrameType) {
        assert!(
            self.frame_type.is_none(),
            "Called .begin_frame when a frame was still in progress, .end_frame must be called before a new frame can be started."
        );
        self.frame_type = Some(frame_type);
        self.index = 0;
    }

    fn write_byte(&mut self, byte: u8) {
        match &self.frame_type {
            Some(FrameType::Diagnostic) => {
                unsafe { isrpipe_write_one(&raw mut stdin_isrpipe, byte) };
            }
            Some(FrameType::Configuration) => {
                self.configuration_buffer[self.index] = byte;
            }
            Some(FrameType::Ip) => {
                self.packet_buffer[self.index] = byte;
            }
            None => {
                panic!("Called .write_byte before .begin_frame, frame_type not set.");
            }
        }
        self.index += 1;
    }

    fn end_frame(&mut self, _: Option<slipmux::Error>) {
        self.frame_type = None;
        // fix me
        self.index = 0;
    }
}

static mut cb: fn(u8) = |b| {
    while true {
        println!("Got data\n");
    }
};

fn init() -> Result<(), &'static str> {
    unsafe {
        // let mut uart = UartDevice::new_with_static_cb(0, 115200, &mut CB)
        //     .unwrap_or_else(|e| panic!("Error initializing UART: {e:?}"));
        let mut uart = UartDevice::new_with_static_cb(0, 115200, &mut cb)
            .unwrap_or_else(|e| panic!("Error initializing UART: {e:?}"));
        let uart = SLIPUART.init(uart);
        STDOUT = Some(uart);
    };

    let context: &mut _ = SLIPMUX.init(Decoder::new());
    unsafe {
        let handler: &mut _ = HANDLER.init(RiotSlipmuxFramehandler::new(
            &mut COAPBUFFER,
            &mut NETBUFFER,
        ));
        HANDLER_PTR = Some(handler)
    };

    // let _result = unsafe {
    //     uart_init(
    //         uart.dev,
    //         115200,
    //         Some(cb),
    //         (context as *mut Decoder) as *mut c_void,
    //     )
    // };

    Ok(())
}
