# The cheriot-rtos port

This is a port for CHERIoT systems, currently configured for the lowRISC Sonata board.

## Building MicroPython for CHERIoT

The following dependencies are required to build this port:

- [LLVM for CHERIoT](https://github.com/CHERIoT-Platform/llvm-project)
- [CHERIoT RTOS](https://github.com/microsoft/cheriot-rtos)
- Python 3
- `make`
- `uf2conv` (Can be installed through `cargo`)

To build, run the following (replacing `/path/to/xxx` with the relevant path on your system):

    $ make CLLVM_DIR=/path/to/cheriot-llvm CRTOS_DIR=/path/to/cheriot-rtos UF2CONV=/path/to/uf2conv

- `CLLVM_DIR` should point to the parent directory of the `bin/` directory holding the CHERIoT llvm binaries.
- `CRTOS_DIR` should point to the root of the CHERIoT RTOS git repository.
- `UF2CONV` should point to the `uf2conv` executable.

## Deploying MicroPython on the Sonata board

Follow the instructions under 'Main persistent flow' at [Sonata v0.2](https://github.com/lowRISC/sonata-system/releases/tag/v0.2), then drag the `build/micropython.uf2` file into the `SONATA`  drive.

The MicroPython interpreter can then be accessed over the UART interface:
    $ sudo minicom -c on -D /dev/ttyUSB2

In order to demonstrate the different MicroPython VM compartment entry points, a REPL is not automatically launched. Instead, press `r` to launch a REPL, `s` to execute the test string, `f` to load the test frozen module, or `q` to exit.

