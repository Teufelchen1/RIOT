#![no_std]
use riot_wrappers::mutex::Mutex;
use riot_wrappers::println;
use riot_wrappers::riot_sys;
use riot_wrappers::uart::UartDevice;

static mut STDOUT: Option<UartDevice> = None;
static WRITE_SYNC: Mutex<()> = Mutex::new(());

#[no_mangle]
pub extern "C" fn auto_init_demouart() {
    if let Err(e) = init() {
        println!("Demouart init error: {}", e);
    }
}

static mut cb: fn(u8) = |b| {
    let _ = write(b"input: ");
    let _ = write(&[b]);
    let _ = write(b"\n");
};

fn init() -> Result<(), &'static str> {
    // use of mutable static is unsafe
    unsafe {
        let mut uart = UartDevice::new_with_static_cb(0, 115200, &mut cb)
            .unwrap_or_else(|e| panic!("Error initializing UART: {e:?}"));
        STDOUT = Some(uart);
    }
    Ok(())
}

#[no_mangle]
pub extern "C" fn stdio_init() {}

#[no_mangle]
extern "C" fn stdio_write(
    buffer: *const riot_sys::libc::c_void,
    len: riot_sys::size_t,
) -> riot_sys::size_t {
    let data = unsafe { core::slice::from_raw_parts(buffer as *const u8, len as _) };
    let _ = WRITE_SYNC.lock();
    write(data)
}

fn write(data: &[u8]) -> u32 {
    let uart = unsafe { &mut STDOUT };
    if let Some(ref mut uart2) = uart {
        write_bytes(uart2, data);
        data.len() as u32
    } else {
        0
    }
}

fn write_bytes(uart: &mut UartDevice, bytes: &[u8]) {
    for b in bytes {
        uart.write(&[b.to_ascii_uppercase()]);
    }
}
