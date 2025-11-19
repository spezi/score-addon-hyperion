# score-addon-hyperion

ossia score addon for Hyperion LED controller output via FlatBuffers protocol.

## Features

- Video output to Hyperion 2.x via FlatBuffers TCP protocol
- Configurable host, port, and priority
- Automatic RGBA to RGB conversion
- Supports Hyperion version 2.0.0 and later

## Requirements

- ossia score 3.x or later
- Hyperion 2.0.0 or later with FlatBuffers server enabled (default port 19400)
- FlatBuffers development headers and compiler

### Installing FlatBuffers

**Ubuntu/Debian:**
```bash
sudo apt install libflatbuffers-dev flatbuffers-compiler
```
on my ubuntu 22 machine i had to compile flatbuffers and install and then copy 
/usr/local/bin/flatc to /usr/bin after that ossia score compiles with addon

**Arch Linux:**
```bash
sudo pacman -S flatbuffers
```

**macOS:**
```bash
brew install flatbuffers
```

## Building

1. Clone this repository into your score addons directory
2. Rebuild score with the addon

```bash
cd score/src/addons
git clone https://github.com/yourname/score-addon-hyperion
cd ../..
cmake --build build
```

## Usage

1. In ossia score, go to Device Explorer
2. Add a new device
3. Select "Hyperion Output" from the Video category
4. Configure:
   - **Host**: Hyperion server address (default: 127.0.0.1)
   - **Port**: FlatBuffers port (default: 19400)
   - **Priority**: LED priority 1-255 (default: 150, lower = higher priority)
   - **Origin**: Source name shown in Hyperion (default: "ossia score")
   - **Width/Height**: Output resolution
   - **Rate**: Frame rate in FPS

5. Connect your video pipeline to the Hyperion output node

## Hyperion Configuration

Make sure the FlatBuffers server is enabled in Hyperion:

1. Open Hyperion web interface
2. Go to Configuration → Network Services
3. Enable "Flatbuffers Server"
4. Note the port (default 19400)

## Protocol Details

This addon uses Hyperion's FlatBuffers protocol:
- Sends Register command on connect with origin and priority
- Sends RawImage commands with RGB data
- Sends Clear command on disconnect

## License

LGPL-3.0, same as ossia score
