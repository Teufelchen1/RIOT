#![no_std]
use riot_wrappers::cstr::cstr;
use riot_wrappers::riot_sys;
use riot_wrappers::{mutex::Mutex, println};

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
use riot_wrappers::thread;


fn write_byte(uart: &UARTDevice, byte: u8) {
    unsafe { uart_write(uart.dev, [byte].as_ptr(), 1) };
}

fn write_escaping_bytes(uart: &UARTDevice, bytes: &[u8]) {
    for byte in bytes {
        match *byte {
            END => {
                write_byte(uart, ESC);
                write_byte(uart, ESC_END);
            }
            ESC => {
                write_byte(uart, ESC);
                write_byte(uart, ESC_ESC);
            }
            _ => {
                write_byte(uart, *byte);
            }
        }
    }
}

#[derive(Debug)]
pub struct UARTDevice {
    dev: uart_t,
}

impl UARTDevice {
    pub fn new(dev: uart_t) -> Self {
        UARTDevice { dev }
    }
}

static SLIPUART: static_cell::StaticCell<UARTDevice> = static_cell::StaticCell::new();
static SLIPMUX: static_cell::StaticCell<Decoder> = static_cell::StaticCell::new();
static HANDLER: static_cell::StaticCell<RiotSlipmuxFramehandler> = static_cell::StaticCell::new();
static mut STDOUT: Option<&'static UARTDevice> = None;
static WRITE_SYNC: Mutex<()> = Mutex::new(());
static mut COAPBUFFER: [u8; 2048] = [0; 2048];
static mut NETBUFFER: [u8; 2048] = [0; 2048];


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
    let uart = unsafe { STDOUT };
    // unsafe: data is initialized per API
    let data = unsafe { core::slice::from_raw_parts(buffer as *const u8, len as _) };
    if let Some(uart) = uart {
        write_byte(uart, DIAGNOSTIC_START);
        write_escaping_bytes(uart, data);
        write_byte(uart, END);
        len
    } else {
        0
    }
}

extern "C" fn cb(arg: *mut c_void, byte: u8) {
    let decoder = unsafe { &mut *(arg as *mut Decoder) };
    decoder.decode(byte, HANDLER);

    // todo!
}

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
    pub const fn new(
        diagnostic_buffer: &'a mut [u8],
        configuration_buffer: &'a mut [u8],
        packet_buffer: &'a mut [u8],
    ) -> Self {
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

    fn end_frame(&mut self, _: Option<Error>) {
        self.frame_type = None;
        // fix me
        self.index = 0;
    }
}


fn init() -> Result<(), &'static str> {
    let uart = SLIPUART.init(UARTDevice::new(0));
    unsafe { STDOUT = Some(uart) };

    let context: &mut _ = SLIPMUX.init(Decoder::new());
    let handler: &mut _ = HANDLER.init(RiotSlipmuxFramehandler::new(
        &mut COAPBUFFER,
        &mut NETBUFFER,
    ));

    let _result = unsafe {
        uart_init(
            uart.dev,
            115200,
            Some(cb),
            (context as *mut Decoder) as *mut c_void,
        )
    };



    Ok(())
}
