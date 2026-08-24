# csi

A cross-platform C++ terminal gradient renderer.

## Build

macOS and Linux:

```sh
./make.sh
```

Windows:

```bat
make.bat
```

The scripts use CMake and select the platform backend at link time.

## Run

```sh
./build/csi fill --space RGB
./build/csi fill --space 256 --fill c --color white
./build/csi fill --space 256 --mode cube --fill c
./build/csi fill --space 16 --fill bg --color '#336699'
./build/csi lava
./build/csi lava --fill c --space 256
./build/csi 256
./build/csi fill -h
./build/csi -v
```

`lava` fills the screen and animates it at 30 frames per second with Quake's turbulent
surface effect: a lava texture sampled through a sine warp that displaces each axis by
the turbulence read at the other one, which makes the surface roll rather than slide. It
runs until Ctrl-C, or until `--seconds <n>` elapses, and takes the same `--fill` option
as `fill`. Quake sampled the warp at whole texels because a pixel was about a texel
wide; a terminal cell is much larger, so the table and the texture are read with
interpolation and the warp is stepped continuously rather than 20 times a second.

`256` charts the whole 256-color palette on one screen as a reference, in four
labelled sections: the 16 system colors down the left, split into a `Dark` and a
`Bright` half; the color cube; the grey ramp; and the cube's own greys. Each cell is
painted with the color it names. System and grey-ramp cells carry their index as
`0xXX`; cube cells carry their `(r,g,b)` coordinates instead, so a cell can be read
as a position in the 6x6x6 grid rather than a bare number. The cube is flattened into
six 6x6 green-by-blue blocks, one per red level, two across and three down, which
keeps it 12 cells wide rather than 36 and leaves neighbouring cells differing in a
single coordinate.

The cube contains six greys of its own, the cells where red, green and blue all
match. That flattening sits the diagonal at a different spot inside every block, so
the six never line up and are easy to miss; the last section gathers them together,
labelled `(0,0,0)` through `(5,5,5)`. Each is placed on the row of the ramp step
nearest it in brightness, so the two scales can be read against each other: six
coarse steps against twenty-four fine ones.

The chart addresses palette slots directly rather than going through the RGB mapping,
since the point is to show what each index actually paints, so `--space` and `--mode`
do not affect it. It needs a window of at least 137 columns and 28 rows.

`--space` is a global option consumed by `main`; it selects the terminal that is handed to the command: `16`, `256`, or `RGB`. `--mode` is a `fill` option and applies only with `--space 256`: it picks which slice of the palette a nearest match may choose from. `all` uses every entry, `known` skips the sixteen system colors whose values depend on the terminal theme, and `cube` uses only the 6×6×6 color cube with no grey ramp. `all` is the default. `--color` selects the upper-right corner using `black`, `white`, `red`, `green`, `blue`, or `#RRGGBB`; it defaults to black. `--fill bg` uses colored terminal backgrounds and spaces. `--fill c` uses the `█` character and colored foreground text. Background mode is the default.
