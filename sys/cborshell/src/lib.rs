#![no_std]

use riot_wrappers::{ println, stdio };
use core::fmt::Write;
use minicbor::{Encoder, Decoder};

struct CmdData<'a> {
    name: &'a str,
    func: fn(args: Option<&[u8; 128]>),
    validate_args: Option<fn(args: Option<&[u8; 128]>) -> bool>,
}

const command_list: &[&CmdData] = &[
    &CmdData {
        name: r"help",
        func: help,
        validate_args: None
    },
    &CmdData {
        name: r"add",
        func: add,
        validate_args: Some(add_validate_args),
    },
];


fn help(_arg: Option<&[u8; 128]>) {
    println!("Help menu");
}

fn add(args: Option<&[u8; 128]>) {
    if args.is_none() {
        println!("Missing args");
        return;
    }
    let raw = args.unwrap();

    let mut decoder = Decoder::new(raw.as_ref());
    decoder.array();
    decoder.map();
    //let a_key = decoder.str().unwrap();
    decoder.skip();
    let a = decoder.str().unwrap().parse::<i32>().unwrap();
    decoder.skip();
    decoder.map();
    //let b_key = decoder.str().unwrap();
    decoder.skip();
    let b = decoder.str().unwrap().parse::<i32>().unwrap();
    let res = a + b;
    println!("Result: {res}");

    let mut cborbuffer: [u8; 20] = [0; 20];
    let mut encoder = Encoder::new(&mut cborbuffer[..]);
    encoder.begin_array();
    encoder.begin_map();
    encoder.str(r"result");
    encoder.i32(res);
    encoder.end();
    encoder.end();
    println!("CBOR: {cborbuffer:x?}");
}

fn add_validate_args(args: Option<&[u8; 128]>) -> bool {
    if args.is_none() {
        println!("Missing args: {{a: i32, b: i32}}");
        return false;
    }
    let raw = args.unwrap();
    let mut decoder = Decoder::new(raw.as_ref());
    if decoder.array().is_err() {
        println!("Arg doesn't start with an array");
        return false;
    }

    if decoder.map().is_err() {
        println!("Arg doesn't contain at least one map");
        return false;
    }
    let first_key = {
        match decoder.str() {
            Ok(val) => val,
            Err(err) => {
                println!("error key first");
                return false;
            }
        }
    };
    let first_value = {
        match decoder.str() {
            Ok(val) => val,
            Err(err) => {
                println!("error name second");
                return false;
            }
        }
    };

    decoder.skip();

    if decoder.map().is_err() { 
        println!("Arg doesn't contain at least two maps");
        return false;
    }
    let second_key = {
        match decoder.str() {
            Ok(val) => val,
            Err(err) => {
                println!("error key first: {err}");
                return false;
            }
        }
    };
    let second_value = {
        match decoder.str() {
            Ok(val) => val,
            Err(err) => {
                println!("error name second: {err}");
                return false;
            }
        }
    };

    let a = {
        if first_key.eq_ignore_ascii_case("a") {
            match first_value.parse::<i32>() {
                Ok(val) => val,
                Err(err) => {
                    println!("Couldn't convert a: {err}");
                    return false;
                }
            }
        } else {
            println!("First argument is not 'a'");
            return false;
        }
    };

    let b = {
        if second_key.eq_ignore_ascii_case("b") {
            match second_value.parse::<i32>() {
                Ok(val) => val,
                Err(err) => {
                    println!("Couldn't convert b: {err}");
                    return false;
                }
            }
        } else {
            println!("Second argument is not 'b'");
            return false;
        }
    };
    println!("[{{a:{a:?}}}, {{b:{b:?}}}]");
    true
}


#[no_mangle]
pub extern "C" fn cborshell_run() {
    let mut io = stdio::Stdio {};
    let mut buffer: [u8; 128] = [0; 128];
    let mut cborbuffer: [u8; 128] = [0; 128];

    loop {
        io.write_str("> ");
        let input_raw = io.read_raw(&mut buffer).unwrap();
        let input_utf8 = core::str::from_utf8(input_raw).unwrap();
        let input = input_utf8.trim();
        if input.is_empty() {
            continue;
        }

        let mut parts = input.splitn(2, ' ');
        let command = parts.next().unwrap_or("help");
        let args_raw = parts.last().unwrap_or("");

        let args: Option<&[u8; 128]> = {
            if !args_raw.is_empty() {
                let parts = args_raw.split(' ');
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
                Some(&cborbuffer)
            } else {
                None
            }
        };

        'found: {
            for n in command_list.into_iter() {
                if n.name.eq_ignore_ascii_case(command) {
                    if n.validate_args.is_some() {
                        if  n.validate_args.unwrap()(args) {
                            (n.func)(args);
                        }
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
