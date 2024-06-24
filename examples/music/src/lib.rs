// Copyright (C) 2020 Christian Amsüss
//
// This file is subject to the terms and conditions of the GNU Lesser
// General Public License v2.1. See the file LICENSE in the top level
// directory for more details.
#![no_std]

//use btoi::btoi;
use coap_handler::Handler;
use coap_handler::Reporting;

use coap_handler_implementations::wkc;

use coap_handler_implementations::{
    new_dispatcher, HandlerBuilder, ReportingHandlerBuilder, SimpleRenderable, SimpleRendered,
};
use coap_message::{
    Code as _, MessageOption, MinimalWritableMessage, MutableWritableMessage, ReadableMessage,
};

use coap_message_utils::option_value::Block2RequestData;
use coap_message_utils::Error;
use coap_numbers::code::{CONTENT, GET};
use coap_numbers::option::{get_criticality, Criticality, ACCEPT, BLOCK2};
use riot_sys::macro_PWM_DEV;
use riot_sys::pwm_mode_t_PWM_LEFT;
use riot_sys::pwm_t;
use riot_wrappers::gpio::{InputMode, OutputMode, GPIO};
use riot_wrappers::ztimer::Clock;
use riot_wrappers::{gcoap, gnrc, thread, ztimer};
use riot_wrappers::{println, riot_main};

use core::ptr::addr_of_mut;
extern crate rust_riotmodules;

riot_main!(main);

static mut global_rotation: u32 = 0;
static mut global_pitch: u32 = 800;

// const ROTATION_TEXT: &str = "Ok\n";

// pub const ROTATION_TEXT_LEN: usize = ROTATION_TEXT.len();

// #[derive(Copy, Clone)]
// pub struct Rotation;
// impl coap_handler_implementations::SimpleRenderable for Rotation {
//     fn render<W: core::fmt::Write>(&mut self, writer: &mut W) {
//         unsafe {
//             global_rotation += 15;
//         }
//         writer.write_str(ROTATION_TEXT).unwrap()
//     }
// }
// pub static ROTATION: SimpleRendered<Rotation> = SimpleRendered(Rotation);

pub struct Rotation;
impl coap_handler::Handler for Rotation {
    type RequestData = u32;
    type ExtractRequestError = Error;
    type BuildResponseError<M: MinimalWritableMessage> = M::UnionError;

    fn extract_request_data<M: ReadableMessage>(
        &mut self,
        request: &M,
    ) -> Result<Self::RequestData, Error> {
        let expected_accept = coap_numbers::content_format::from_str("text/plain; charset=utf-8");

        let mut block2 = None;

        for o in request.options() {
            match o.number() {
                ACCEPT => {
                    if expected_accept.is_some() && o.value_uint() != expected_accept {
                        return Err(Error::bad_option(ACCEPT));
                    }
                }
                BLOCK2 => {
                    block2 = match block2 {
                        Some(_) => return Err(Error::bad_request()),
                        None => Block2RequestData::from_option(&o)
                            .map(Some)
                            // Unprocessed CoAP Option is also for "is aware but could not process"
                            .map_err(|_| Error::bad_option(BLOCK2))?,
                    }
                }
                o if get_criticality(o) == Criticality::Critical => {
                    return Err(Error::bad_option(o));
                }
                _ => (),
            }
        }

        let stringify = core::str::from_utf8(request.payload()).unwrap_or("0");
        let number: u32 = stringify.parse().unwrap_or(0);
        let reqdata = match request.code().into() {
            POST => number,
            _ => return Err(Error::method_not_allowed()),
        };
        Ok(reqdata)
    }

    fn estimate_length(&mut self, _request: &Self::RequestData) -> usize {
        1280 - 40 - 4 // does this correclty calculate the IPv6 minimum MTU?
    }

    fn build_response<M: MutableWritableMessage>(
        &mut self,
        response: &mut M,
        request: Self::RequestData,
    ) -> Result<(), Self::BuildResponseError<M>> {
        let cf = coap_numbers::content_format::from_str("text/plain; charset=utf-8");
        //let block2data = request.0;
        response.set_code(M::Code::new(CONTENT)?);
        //block2_write_with_cf(request, response, |w| self.0.render(w), cf);
        println!("Request: {:?}", request);
        unsafe {
            global_rotation = request % 360;
        }
        Ok(())
    }
}

impl Reporting for Rotation {
    type Record<'a> = wkc::EmptyRecord
    where
        Self: 'a,
    ;
    type Reporter<'a> = core::iter::Once<wkc::EmptyRecord>
    where
        Self: 'a,
    ;

    fn report(&self) -> Self::Reporter<'_> {
        // Using a ConstantSliceRecord instead would be tempting, but that'd need a const return
        // value from self.0.content_format()
        core::iter::once(wkc::EmptyRecord {})
    }
}

pub struct Pitch;
impl coap_handler::Handler for Pitch {
    type RequestData = u32;
    type ExtractRequestError = Error;
    type BuildResponseError<M: MinimalWritableMessage> = M::UnionError;

    fn extract_request_data<M: ReadableMessage>(
        &mut self,
        request: &M,
    ) -> Result<Self::RequestData, Error> {
        let expected_accept = coap_numbers::content_format::from_str("text/plain; charset=utf-8");

        let mut block2 = None;

        for o in request.options() {
            match o.number() {
                ACCEPT => {
                    if expected_accept.is_some() && o.value_uint() != expected_accept {
                        return Err(Error::bad_option(ACCEPT));
                    }
                }
                BLOCK2 => {
                    block2 = match block2 {
                        Some(_) => return Err(Error::bad_request()),
                        None => Block2RequestData::from_option(&o)
                            .map(Some)
                            // Unprocessed CoAP Option is also for "is aware but could not process"
                            .map_err(|_| Error::bad_option(BLOCK2))?,
                    }
                }
                o if get_criticality(o) == Criticality::Critical => {
                    return Err(Error::bad_option(o));
                }
                _ => (),
            }
        }

        let stringify = core::str::from_utf8(request.payload()).unwrap_or("0");
        let number: u32 = stringify.parse().unwrap_or(800);
        let reqdata = match request.code().into() {
            POST => number,
            _ => return Err(Error::method_not_allowed()),
        };
        Ok(reqdata)
    }

    fn estimate_length(&mut self, _request: &Self::RequestData) -> usize {
        1280 - 40 - 4 // does this correclty calculate the IPv6 minimum MTU?
    }

    fn build_response<M: MutableWritableMessage>(
        &mut self,
        response: &mut M,
        request: Self::RequestData,
    ) -> Result<(), Self::BuildResponseError<M>> {
        let cf = coap_numbers::content_format::from_str("text/plain; charset=utf-8");
        //let block2data = request.0;
        response.set_code(M::Code::new(CONTENT)?);
        //block2_write_with_cf(request, response, |w| self.0.render(w), cf);
        println!("Request: {:?}", request);
        unsafe {
            if request > 200 {
                global_pitch = request % 8000;
            } else {
                global_pitch = 200;
            }
        }
        Ok(())
    }
}

impl Reporting for Pitch {
    type Record<'a> = wkc::EmptyRecord
    where
        Self: 'a,
    ;
    type Reporter<'a> = core::iter::Once<wkc::EmptyRecord>
    where
        Self: 'a,
    ;

    fn report(&self) -> Self::Reporter<'_> {
        // Using a ConstantSliceRecord instead would be tempting, but that'd need a const return
        // value from self.0.content_format()
        core::iter::once(wkc::EmptyRecord {})
    }
}

fn addBass(start: usize, stop: usize, audio_raw: &mut [u16]) {
    let times = 8192.0 / 60.0;
    let stop_damping = (start as f32 + (stop as f32 - start as f32) * 0.6) as usize;
    for i in start..stop_damping {
        let value = (i as f32 % times) / times;
        let value = value * 128.0;
        let value = value as u16;
        unsafe {
            audio_raw[i] = value;
        }
    }
    for i in stop_damping..stop {
        let value = (i as f32 % times) / times;
        let damping = (stop as f32 - i as f32) / (stop - stop_damping) as f32;
        let value = value * 128.0 * damping;
        let value = value as u16;
        unsafe {
            audio_raw[i] = value;
        }
    }
}

fn addTone(start: usize, stop: usize, freq: f32, audio_raw: &mut [u16]) {
    println!("Adding {freq}");
    let times = 8192.0 / freq;
    let stop_damping = (start as f32 + (stop as f32 - start as f32) * 0.6) as usize;
    for i in start..stop_damping {
        let value = (i as f32 % times) / times;
        let value = value * 64.0;
        let value = value as u16;
        unsafe {
            audio_raw[i] += value;
        }
    }
    for i in stop_damping..stop {
        let value = (i as f32 % times) / times;
        let damping = (stop as f32 - i as f32) / (stop - stop_damping) as f32;
        let value = value * 64.0 * damping;
        let value = value as u16;
        unsafe {
            audio_raw[i] += value;
        }
    }
}

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

    let handler = new_dispatcher()
        .at(&["speed"], Rotation)
        .at(&["pitch"], Pitch)
        .with_wkc();
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
        let clock = Clock::msec();
        let mut bar = 0;
        let mut last_value = false;
        let mut last_global_rotation = 0;
        let mut last_global_pitch = 0;
        loop {
            clock.sleep_ticks(10);
            let value = p_in.is_low();
            let coap_change = unsafe {
                (global_rotation != last_global_rotation) || (global_pitch != last_global_pitch)
            };
            if value != last_value || coap_change {
                last_value = value;
                unsafe {
                    last_global_rotation = global_rotation;
                    last_global_pitch = global_pitch;
                }

                let speed = {
                    println!("Rotation value {last_global_rotation}");
                    ((1.0 + (last_global_rotation as f32 / 360.0)) * 5.0) as u32
                };
                let samplerate = 8192;
                let step = samplerate * 4 / speed;
                println!("Speed: {speed}, Step {step}");

                for slot in 0..speed {
                    let start = slot * step;
                    let stop = (slot + 1) * step;
                    //println!("{start}, {stop}");
                    unsafe {
                        addBass(start as usize, stop as usize, &mut audio_raw);
                    }
                    if bar == 0 || bar == 2 {
                        unsafe {
                            addTone(
                                start as usize,
                                stop as usize,
                                global_pitch as f32,
                                &mut audio_raw,
                            );
                        }
                    }
                    if bar == 1 || bar == 3 || bar == 5 || bar == 7 {
                        unsafe {
                            addTone(
                                start as usize,
                                stop as usize,
                                160.0 + 70.0 * bar as f32,
                                &mut audio_raw,
                            );
                        }
                    }
                    bar += 1;
                    bar %= 8;
                }
            }

            // if value {
            //     unsafe {
            //         for i in 0..16384 {
            //             let value = fac * i as f32 / 8000.0;
            //             let value = (value % (2.0 * 3.141)) * 255.0 / (2.0 * 3.141);
            //             //let value = libm::sin(value as f64);
            //             //let value = value * 127.0 + 128.0;
            //             let value = value as u16;
            //             audio_raw[i] = value;
            //         }
            //         //audio_raw[index as usize] = 0x00;
            //     }
            // } else {
            //     unsafe {
            //         for i in 16384..32768 {
            //             let value = fac * i as f32 / 8000.0;
            //             let value = (value % (2.0 * 3.141)) * 255.0 / (2.0 * 3.141);
            //             //let value = libm::sin(value as f64);
            //             //let value = value * 127.0 + 128.0;
            //             let value = value as u16;
            //             audio_raw[i] = value;
            //         }
            //     }
            //     //audio_raw[index as usize] = 0x00;
            // }
        }
    })
}
