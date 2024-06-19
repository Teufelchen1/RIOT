// Copyright (C) 2020 Christian Amsüss
//
// This file is subject to the terms and conditions of the GNU Lesser
// General Public License v2.1. See the file LICENSE in the top level
// directory for more details.
#![no_std]

use coap_handler::Handler;
use coap_handler_implementations::{
    new_dispatcher, HandlerBuilder, ReportingHandlerBuilder, SimpleRenderable, SimpleRendered,
};
use riot_sys::macro_PWM_DEV;
use riot_sys::pwm_mode_t_PWM_LEFT;
use riot_sys::pwm_t;
use riot_wrappers::gpio::{InputMode, OutputMode, GPIO};
use riot_wrappers::ztimer::Clock;
use riot_wrappers::{gcoap, gnrc, thread, ztimer};
use riot_wrappers::{println, riot_main};

extern crate rust_riotmodules;

riot_main!(main);

const POEM_TEXT: &str = "Aurea\n";

pub const POEM_TEXT_LEN: usize = POEM_TEXT.len();

#[derive(Copy, Clone)]
pub struct Poem;
impl coap_handler_implementations::SimpleRenderable for Poem {
    fn render<W: core::fmt::Write>(&mut self, writer: &mut W) {
        writer.write_str(POEM_TEXT).unwrap()
    }
}
pub static POEM: SimpleRendered<Poem> = SimpleRendered(Poem);

fn main() {
    //let mut audio: [u8; 0x8000] = [0; 0x8000];
    extern "C" {
        static mut audio_raw: [u16; 78430];
    }
    extern "C" {
        fn pwm_init_auto_rust(freq: u32, res: u16) -> u32;
    }
    unsafe {
        let freq = pwm_init_auto_rust(31372, 255);
        println!("Set frequency {:}", freq);
    }
    let mut index = 0;
    let mut rotation: u32 = 0;

    let values500: &[u8] = &[
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
        128, 167, 202, 230, 248, 255, 248, 230, 202, 167, 128, 89, 54, 26, 8, 1, 8, 26, 54, 89,
    ];
    let mut p_in = GPIO::from_port_and_pin(0, 11)
        .expect("In pin does not exist")
        .configure_as_input(InputMode::InPullUp)
        .expect("In pin could not be configured");

    let handler = new_dispatcher().at(&["ps"], POEM).with_wkc();
    let mut handler = riot_wrappers::coap_handler::v0_2::GcoapHandler(handler);

    let mut listener = gcoap::SingleHandlerListener::new_catch_all(&mut handler);

    gcoap::scope(|greg| {
        greg.register(&mut listener);

        println!(
            "CoAP server ready; waiting for interfaces to settle before reporting addresses..."
        );

        let sectimer = ztimer::Clock::sec();
        sectimer.sleep_ticks(2);

        for netif in gnrc::Netif::all() {
            println!(
                "Active interface from PID {:?} ({:?})",
                netif.pid(),
                netif.pid().get_name().unwrap_or("unnamed")
            );
            match netif.ipv6_addrs() {
                Ok(addrs) => {
                    for a in &addrs {
                        println!("    Address {:?}", a);
                    }
                }
                _ => {
                    println!("    Does not support IPv6.");
                }
            }
        }

        // Sending main thread to sleep; can't return or the Gcoap handler would need to be
        // deregistered (which it can't).
        loop {
            let value = p_in.is_high();
            println!("Read GPIO value {}, writing it to the out port", value);
            let clock = Clock::msec();
            println!("Rotation value {}", rotation);
            clock.sleep_ticks(10);
            rotation += 15;
            rotation %= 360;
            index += 1;
            index %= 78000;
            let fac = 2.0 * 3.141 * (80.0 + rotation as f32);
            if value {
                unsafe {
                    for i in 0..16384 {
                        let value = fac * i as f32 / 8000.0;
                        let value = (value % (2.0 * 3.141)) * 255.0 / (2.0 * 3.141);
                        //let value = libm::sin(value as f64);
                        //let value = value * 127.0 + 128.0;
                        let value = value as u16;
                        audio_raw[i] = value;
                    }
                    //audio_raw[index as usize] = 0x00;
                }
            } else {
                unsafe {
                    for i in 16384..32768 {
                        let value = fac * i as f32 / 8000.0;
                        let value = (value % (2.0 * 3.141)) * 255.0 / (2.0 * 3.141);
                        //let value = libm::sin(value as f64);
                        //let value = value * 127.0 + 128.0;
                        let value = value as u16;
                        audio_raw[i] = value;
                    }
                }
                //audio_raw[index as usize] = 0x00;
            }
        }
    })
}
