# csi

A cross-platform C++ tool for exercising terminal CSI sequences.

## Build

```sh
./make.sh     # macOS and Linux
make.bat      # Windows
```

Both wrap CMake and pick the platform backend at link time. The binary lands in
`build/csi`.

## Commands

### `fill`

A bilinear gradient across the screen. Runs until Ctrl-C.

```sh
csi fill --space 16
```

![fill in 16 colors](screenshot/fill-16.png)

```sh
csi fill --space 256
```

![fill in 256 colors](screenshot/fill-256.png)

```sh
csi fill --space RGB
```

![fill in truecolor](screenshot/fill-rgb.png)

### `lava`

Quake's turbulent surface effect at 30 fps: a lava texture sampled through a sine
warp that displaces each axis by the turbulence read at the other, so the surface
rolls rather than slides. Runs until Ctrl-C, or `--seconds <n>`.

The frame rate is shown in the top-left corner and a timing breakdown is printed on
exit; `--no-stats` turns the per-frame measurement off.

That rate counts frames *submitted*, not frames you saw. The loop is paced to 30 fps,
so the readout sits at 30 whenever the program is not blocked, even if the terminal
coalesces those updates into fewer repaints -- no terminal acknowledges a repaint, so
nothing in-band can see that. Run `--fps 0` to remove the pacing and let the loop go
as fast as the terminal will take frames; that number is a real ceiling and is what
to compare between terminals.

The other number worth reading is `fence`: flushing a frame only tells
you the kernel accepted the bytes, so instead each frame is followed by `ESC[6n`,
whose reply a terminal cannot send until it has parsed everything queued ahead of
it. That round trip is what the terminal actually spent on the frame. It measures
processing, not pixels -- no terminal acknowledges a present.

```sh
csi lava
```

![lava](screenshot/lava.png)

### `256`

The whole 256-color palette on one screen. Four sections: the system colors split
into `Dark` and `Bright`, the color cube, the grey ramp, and the cube's own greys.
System and ramp cells are labelled with their index, cube cells with their `(r,g,b)`
coordinates. Needs a window of at least 137x28.

```sh
csi 256
```

![256-color palette chart](screenshot/palette256.png)

## Options

```
Usage: csi <command> [options]

Commands:
  fill       Render the gradient
  lava       Animate Quake lava at 30 fps
  256        Chart the 256-color palette with each index

fill options:
  --color  <name|#RRGGBB>     Upper-right corner color
  --fill   <c|bg>             Fill glyph or background
  --mode   <all|known|cube>   Palette subset (--space 256 only)

lava options:
  --fill   <c|bg>             Fill glyph or background
  --seconds <n>               Stop after n seconds (default: Ctrl-C)
  --fps    <n>                Cap the frame rate, 0 for uncapped
  --no-stats                  Skip the per-frame fence timing

Global options:
  --space  <16|256|RGB>       Terminal color space
  -h, --help                  Show this help
  -v, --version               Show version
```

`--mode` picks which slice of the palette a nearest match may draw from. `all` uses
every entry; `known` skips the sixteen system colors, whose values are whatever the
terminal theme paints them; `cube` uses only the 6x6x6 color cube, with no grey ramp.

The `256` chart addresses palette slots directly rather than going through the RGB
mapping, so `--space` and `--mode` do not affect it.
