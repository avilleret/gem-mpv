This project moved here : https://gitlab.com/antoine.villeret/pd-mpv

# gem-mpv

Bring the power of `libmpv` into `Gem`.

## Building

You need a recent version of Gem installed locally and the libmpv at least version 0.27.2.

    git clone https://github.com/avilleret/gem-mpv.git
    mkdir build
    cd build
    cmake ../gem-mpv
    cmake --build .

Then you will find a `gem-mpv.pd_linux` in the `build` folder.
Put it in your Pd search path and start playing around.

Feel free to report issue on the tracker : https://github.com/avilleret/gem-mpv/issues
