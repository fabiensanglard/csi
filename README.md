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

The frame rate is shown in the bottom-right corner and a timing breakdown is printed on
exit. That rate counts frames *submitted*, not frames you saw: the loop is paced, so
it reads 30 whenever the program is not blocked, even if the terminal coalesces those
updates into fewer repaints. Run `--fps 0` to remove the pacing and measure the real
ceiling; that is the number to compare between terminals.

`--stats` picks how each frame is fenced, weakest to strongest. None of them reach
photons -- no terminal acknowledges a present, so compositing and refresh are outside
all of them.

| mode | what it waits for | what it proves |
| --- | --- | --- |
| `none` | nothing, and no summary | quiet run |
| `flush` | `fwrite` + `fflush` returning | the kernel accepted the bytes |
| `drain` | `tcdrain` | every byte reached the terminal |
| `dsr` | reply to `ESC[6n` | the terminal parsed the frame |
| `da` | reply to `ESC[c` | same, via a different query |
| `sync` | frame wrapped in DEC 2026, then `ESC[6n` | parsed, and presented atomically |

`dsr` is the default. `sync` also removes tearing, since the terminal buffers the
frame and presents it in one go, and lets it skip intermediate repaints -- but
Terminal.app does not implement mode 2026, so there it costs nothing and does
nothing. A terminal that never answers a query retires that fence after one timeout
rather than paying it every frame.

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
  --stats  <none|flush|drain|dsr|da|sync>
                              How each frame is fenced
  --no-stats                  Same as --stats none

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
