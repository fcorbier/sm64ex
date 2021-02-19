# sm64ex for the PocketGo S30

Fork of sm64ex targetting the PocketGo S30.

Feel free to report bugs and contribute, but remember, there must be **no upload of any copyrighted asset**. 
Run `./extract_assets.py --clean && make clean` or `make distclean` to remove ROM-originated content.

## Building

### Google Colaboratory

<a href="https://colab.research.google.com/drive/187ZftyZPQp-UhwlTMI6XSHvBO2qxU0SP?usp=sharing">Here</a> is a Google Colaboratory page to build your own binary that you can copy to the PocketGo S30 SD card.

Note that you will first need to install <a href="https://github.com/retrogamecorps/Simple30">Simple30</a> to your SD card.

### Manual local build

On a Linux system, install the <a href="https://github.com/fcorbier/pocketgo-s30-toolchain/releases/download/1.0/pocketgo-s30-toolchain.tgz">PocketGo S30 toolchain</a>, do a `source env_setup.sh` in the toolchain folder, then you can just run `make` in the `sm64ex` folder.

## New features

 * Options menu with various settings, including button remapping.
 * Optional external data loading (so far only textures and assembled soundbanks), providing support for custom texture packs.
 * Optional analog camera and mouse look (using [Puppycam](https://github.com/FazanaJ/puppycam)).
 * Optional OpenGL1.3-based renderer for older machines, as well as the original GL2.1, D3D11 and D3D12 renderers from Emill's [n64-fast3d-engine](https://github.com/Emill/n64-fast3d-engine/).
 * Option to disable drawing distances.
 * Optional model and texture fixes (e.g. the smoke texture).
 * Skip introductory Peach & Lakitu cutscenes with the `--skip-intro` CLI option
 * Cheats menu in Options (activate with `--cheats` or by pressing L thrice in the pause menu).
 * Support for both little-endian and big-endian save files (meaning you can use save files from both sm64-port and most emulators), as well as an optional text-based save format.

Recent changes in Nightly have moved the save and configuration file path to `%HOMEPATH%\AppData\Roaming\sm64pc` on Windows and `$HOME/.local/share/sm64pc` on Linux. This behaviour can be changed with the `--savepath` CLI option.
For example `--savepath .` will read saves from the current directory (which not always matches the exe directory, but most of the time it does);
   `--savepath '!'` will read saves from the executable directory.

**Make sure you have MXE first before attempting to compile for Windows on Linux and WSL. Follow the guide on the wiki.**
