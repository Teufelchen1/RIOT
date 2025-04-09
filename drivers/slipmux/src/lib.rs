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

extern "C" {
    fn gnrc_netif_raw_create(
        netif: *mut gnrc_netif_t,
        stack: *mut u8,
        stacksize: i32,
        priority: u8,
        name: *const u8,
        dev: *mut netdev,
    ) -> i32;
}

const DIAGNOSTIC_START: u8 = 0x0a;
const CONFIGURATION_START: u8 = 0xa9;
const END: u8 = 0xc0;
const ESC: u8 = 0xdb;
const ESC_END: u8 = 0xdc;
const ESC_ESC: u8 = 0xdd;

enum FrameState {
    //   PACKET,
    Diagnostic,
    DiagnosticEsc,
    Configuration,
    ConfigurationEsc,
    Idle,
}

use FrameState::{Configuration, ConfigurationEsc, Diagnostic, DiagnosticEsc, Idle};

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

struct Slipmux {
    state: FrameState,
    rxbuf: [u8; 256],
    index: usize,
    netdev: netdev_t,
}

static SLIP: static_cell::StaticCell<Slipmux> = static_cell::StaticCell::new();
static SLIPUART: static_cell::StaticCell<UARTDevice> = static_cell::StaticCell::new();
static mut SLIPCTX: Option<&'static mut Slipmux> = None;
static mut STDOUT: Option<&'static UARTDevice> = None;
static WRITE_SYNC: Mutex<()> = Mutex::new(());
static NETIF: static_cell::StaticCell<gnrc_netif_t> = static_cell::StaticCell::new();
static mut NETSTACK: [u8; 2048] = [0; 2048];
static mut COAPSTACK: [u8; 2048] = [0; 2048];

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
    let slip = unsafe { &mut *(arg as *mut Slipmux) };
    fsm(slip, byte);
}

fn fsm(slip: &mut Slipmux, byte: u8) {
    slip.state = {
        match (&slip.state, byte) {
            (Idle, END) => Idle, /* ignore empty frame */
            (Idle, DIAGNOSTIC_START) => Diagnostic,
            (Idle, CONFIGURATION_START) => Configuration,
            (Idle, _) => Idle,

            (Diagnostic, ESC) => DiagnosticEsc,
            (Diagnostic, END) => Idle,
            (Diagnostic, _) => {
                unsafe { isrpipe_write_one(&raw mut stdin_isrpipe, byte) };
                Diagnostic
            }
            (DiagnosticEsc, ESC_END) => {
                unsafe { isrpipe_write_one(&raw mut stdin_isrpipe, END) };
                Diagnostic
            }
            (DiagnosticEsc, ESC_ESC) => {
                unsafe { isrpipe_write_one(&raw mut stdin_isrpipe, ESC) };
                Diagnostic
            }
            (DiagnosticEsc, _) => {
                unsafe { isrpipe_write_one(&raw mut stdin_isrpipe, byte) };
                Diagnostic
            }

            (Configuration, ESC) => ConfigurationEsc,
            (Configuration, END) => {
                let raw = &mut slip.netdev as *mut netdev_t;
                unsafe { netdev_trigger_event_isr(raw as *mut _) };
                Idle
            }
            (Configuration, _) => {
                slip.rxbuf[slip.index] = byte;
                slip.index += 1;
                Configuration
            }
            (ConfigurationEsc, ESC_END) => {
                slip.rxbuf[slip.index] = END;
                slip.index += 1;
                Configuration
            }
            (ConfigurationEsc, ESC_ESC) => {
                slip.rxbuf[slip.index] = ESC;
                slip.index += 1;
                Configuration
            }
            (ConfigurationEsc, _) => {
                slip.rxbuf[slip.index] = byte;
                slip.index += 1;
                Configuration
            }
        }
    }
}

extern "C" fn _confirm_send(_: *mut riot_wrappers::riot_sys::netdev, _: *mut c_void) -> i32 {
    0
}
extern "C" fn _send(
    _: *mut riot_wrappers::riot_sys::netdev,
    _: *const riot_wrappers::riot_sys::iolist,
) -> i32 {
    0
}

fn _calc_csum(payload: &[u16]) -> u16 {
    let mut csum: u32 = 0;
    for word in payload {
        csum += *word as u32;
    }
    while (csum >> 16) > 0 {
        let carry: u32 = csum >> 16;
        csum = (csum & 0xffff) + carry;
    }
    return csum as u16;
}

extern "C" fn _recv(
    _netdev: *mut riot_wrappers::riot_sys::netdev,
    buf: *mut c_void,
    len: u32,
    _info: *mut c_void,
) -> i32 {
    let buffer = unsafe { core::slice::from_raw_parts_mut(buf as *mut u8, len as usize) };
    let slipcontext = unsafe { &mut SLIPCTX };
    if let Some(&mut ref mut ctx) = slipcontext {
        if ctx.index <= len as usize {
            buffer[..ctx.index].copy_from_slice(&ctx.rxbuf[..ctx.index]);
            let ret = ctx.index as i32;
            ctx.index = 0;
            ret
        } else {
            ctx.index as i32
        }
    } else {
        todo!();
    }
}

extern "C" fn _init(netdev: *mut riot_wrappers::riot_sys::netdev) -> i32 {
    unsafe {
        if let Some(cb) = (*netdev).event_callback {
            cb(netdev, NETDEV_EVENT_LINK_UP);
        }
    };
    0
}
extern "C" fn _isr(netdev: *mut riot_wrappers::riot_sys::netdev) {
    unsafe {
        if let Some(cb) = (*netdev).event_callback {
            cb(netdev, NETDEV_EVENT_RX_COMPLETE);
        }
    };
}
extern "C" fn _get(
    _: *mut riot_wrappers::riot_sys::netdev,
    option: netopt_t,
    dest: *mut c_void,
    _: u32,
) -> i32 {
    match option {
        NETOPT_IS_WIRED => 1,
        NETOPT_DEVICE_TYPE => {
            let typ = dest as *mut u16;
            unsafe { (*typ) = NETDEV_TYPE_SLIP as u16 };
            2
        }
        //_ => -95,
        // -ENOTSUP
        _ => -134,
    }
}
unsafe extern "C" fn _set(
    _: *mut riot_wrappers::riot_sys::netdev,
    _: u8,
    _: *const c_void,
    _: u32,
) -> i32 {
    -61
}

const DRIVER: netdev_driver_t = netdev_driver_t {
    send: Some(_send),
    recv: Some(_recv),
    init: Some(_init),
    isr: Some(_isr),
    get: Some(_get),
    set: Some(_set),
    confirm_send: Some(_confirm_send),
};

extern "C" {
    fn slip_recv_init();
    fn slip_recv(buff: *mut riot_sys::libc::c_char) -> i32;
}

static mut SECONDTHREAD_MAIN: &'static mut (dyn Send + FnMut()) = &mut || sec_thread();

fn sec_thread() {
    unsafe { slip_recv_init() };
    let mut buffer: [u8; 512] = [0; 512];
    loop {
        let len = unsafe { slip_recv(buffer.as_mut_ptr() as *mut u8) } as usize;
        {
            let _ = WRITE_SYNC.lock();
            let uart = unsafe { STDOUT };
            // unsafe: data is initialized per API
            if let Some(uart) = uart {
                write_byte(uart, CONFIGURATION_START);
                write_escaping_bytes(uart, &buffer[..len]);
                write_byte(uart, END);
            }
        }
    }
}

fn init() -> Result<(), &'static str> {
    let mut nif_ = gnrc_netif_t::default();
    nif_.flags = 0;
    let nif: *mut _ = NETIF.init(nif_);
    let uart = SLIPUART.init(UARTDevice::new(0));
    unsafe { STDOUT = Some(uart) };
    let context: &mut _ = SLIP.init(Slipmux {
        state: Idle,
        rxbuf: [0; 256],
        index: 0,
        netdev: netdev_t::default(),
    });
    (*context).netdev.driver = &DRIVER;

    let _result = unsafe {
        uart_init(
            uart.dev,
            115200,
            Some(cb),
            (context as *mut Slipmux) as *mut c_void,
        )
    };

    unsafe {
        let raw = (&mut (*context).netdev) as *mut netdev_t;
        netdev_register(
            raw as *mut riot_wrappers::riot_sys::inline::netdev,
            NETDEV_SLIPDEV,
            0,
        );
    };
    unsafe {
        let raw = (&mut (*context).netdev) as *mut netdev_t;
        gnrc_netif_raw_create(
            nif,
            NETSTACK.as_mut_ptr(),
            2048,
            2,
            b"foo\0".as_ptr(),
            raw as *mut riot_wrappers::riot_sys::inline::netdev,
        );
    };
    unsafe { SLIPCTX = Some(context) };

    let _secondthread = unsafe {
        thread::spawn(
            &mut COAPSTACK,
            &mut SECONDTHREAD_MAIN,
            cstr!("slp_coap_recv"),
            (riot_sys::THREAD_PRIORITY_MAIN - 2) as _,
            (riot_sys::THREAD_CREATE_STACKTEST) as _,
        )
        .expect("Failed to spawn second thread")
    };

    Ok(())
}
