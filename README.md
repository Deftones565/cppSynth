# cppSynth

This is a hobby project to learn more about C++, it's a synth that can output sine, square, sawtooth and triangle.

You select the waveform from the dropdown box, Up arrow and down arrow change the octaves. A-; on the keyboard for the notes.
It uses the MIDI standard.

## Clone

```bash
git clone --recurssive https://github.com/Deftones565/cppSynth
```

## Dependencies

### Debian-based distributions (e.g., Ubuntu, Debian)

```bash
sudo apt-get update
sudo apt-get install g++ libportaudio2 libglfw3 libglew-dev libgl1-mesa-dev libxrandr2 libxinerama1 libxcursor1 libxi-dev libasound2-dev libpthread-stubs0-dev
sudo apt-get install libxinerama-dev libxcursor-dev libxi-dev libx randr-dev
```

### Red Hat-based distributions (e.g., Fedora, CentOS)

```bash
sudo dnf install gcc-c++ portaudio-devel glfw-devel glew-devel mesa-libGL-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel alsa-lib-devel
sudo dnf install libXinerama-devel libXcursor-devel libXi-devel libXrandr-devel
```

### Arch Linux and Manjaro

```bash
sudo pacman -Syu gcc portaudio glfw-x11 glew mesa libxrandr libxinerama libxcursor libxi alsa-lib
sudo pacman -Syu libxinerama libxcursor libxi libxrandr
```

# TODO

* Add visual boxes for viewing the waveforms.
* Add more voices, right now it's mono.
