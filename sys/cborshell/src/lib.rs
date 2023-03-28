#![no_std]
#![deny(elided_lifetimes_in_paths)]
use riot_wrappers::{ println, stdio };
use core::fmt::Write;
use minicbor::{Encoder};

struct CmdData<'a> {
    name: &'a str,
    func: fn(args: Option<&'a [u8; 128]>),
    validate_args: Option<fn(args: Option<&'a [u8; 128]>) -> bool>,
}

const command_list: &[&CmdData<'_>] = &[
    &CmdData {
        name: r"help",
        func: help,
        validate_args: None
    },
    &CmdData {
        name: r"calc",
        func: calc,
        validate_args: Some(calc_validate_args),
    },
];


fn help(_arg: Option<&[u8; 128]>) {
    println!("Help menu");
}

fn calc(_arg: Option<&[u8; 128]>) {
    let res = 1 + 3;
    println!("Result: {res}");
}

fn calc_validate_args(args: Option<&[u8; 128]>) -> bool {
    if args.is_none() {
        println!("Missing args: {{a: i32, b: i32}}");
        return false;
    }
    let raw = args.unwrap();
    false
}


#[no_mangle]
pub extern "C" fn cborshell_run() {
    let mut io = stdio::Stdio {};
    let mut buffer: [u8; 128] = [0; 128];
    let mut cborbuffer: [u8; 128] = [0; 128];
    let mut argbuffer: [u8; 128] = [0; 128];

    loop {
        io.write_str("> ");
        let input_raw = io.read_raw(&mut buffer).unwrap();
        let input_utf8 = core::str::from_utf8(input_raw).unwrap();
        let input = input_utf8.trim();
        if input.is_empty() {
            continue;
        }

        let mut parts = input.splitn(1, ' ');
        let command = parts.next().unwrap_or("help");
        let args_raw = parts.last().unwrap_or("");
        let mut args = None;

        println!("Command: {command:?}, Args: {args:?}");

        if !args_raw.is_empty() {
            let mut parts = args_raw.split(' ');
            {
                let mut encoder = Encoder::new(&mut cborbuffer[..]);
                encoder.begin_array();
                for arg in parts.into_iter() {
                    if arg.contains("=") {
                        encoder.begin_map();
                        let mut map = arg.split('=');
                        encoder.str(map.next().unwrap());
                        encoder.str(map.last().unwrap());
                        encoder.end();
                    } else {
                        encoder.str(arg);
                    }
                }
                encoder.end();
            }
            argbuffer = cborbuffer.clone();
            args = Some(&argbuffer);
        }

        'found: {
            for n in command_list.into_iter() {
                if n.name.eq_ignore_ascii_case(command) {
                    if n.validate_args.is_some() {
                        n.validate_args.unwrap()(args);
                        (n.func)(args);
                    } else {
                        (n.func)(None);
                    }
                    break 'found;
                }
            } 
            println!("Command not found");
        }
    }
}
