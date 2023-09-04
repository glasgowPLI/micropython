# Compiling on Morello

I compiled MicroPython directly on the CheriBSD/Morello machine, using the
`llvm-morello` toolchain and `gmake`. The makefiles are not compatible with
BSD `make`.

## Hybrid ABI

To compile MicroPython for the Hybrid ABI, execute the following command in this folder:

```
gmake CC='clang-morello' CXX='clang++-morello'
```

## Purecap ABI

To compile MicroPython for the Purecap ABI, execute the following command in this folder:

```
gmake CC='clang-morello -march=morello+c64 -mabi=purecap' CXX='clang++-morello -march=morello+c64 -mabi=purecap' LIBFFI_LDFLAGS='-L/usr/local/lib -lffi'
```

The `LIBFFI_LDFLAGS` argument can be omitted if FFI has been turned off in `mpconfigport.mk` 

## Coverage variant

To compile the coverage variant, simply append `VARIANT=coverage` to the `gmake` command.
If you get an internal error in Clang, remove the two lines in
`variants/coverage/mpconfigvariant.h` which add the flags`-fprofile-arcs -ftest-coverage`.

