## Packet layout
all packages start with `55 AA` (same as with Xiaomi M365 scooters).
The packet header looks like this:
55 AA length address command arg payload[length - 2] checksumLow checksumHigh.

On the ESA 5000 there are three different packets. The following lists these
packets exluding the `55 AA` header and the checksum.

## Address `22` set option/tuning
- arg `7C`: turn eco mode on/off
    - `04 22 01 7C 01 00`
    - byte 4 `01` for on, `00` for off
- arg `7D`: turn lock on/off
    - `04 22 01 7D 01 00`
    - byte 4 `01` for on, `00` for off
- arg `F0`: turn light on/off
    - `04 22 01 F0 01 00`
    - byte 4 `01` for on, `00` for off
- arg `F2`: set max speed
    - `04 22 01 F2 F3 02`
    - bytes 0-3 `04 22 01 F2`: is the packet header (length, address, command, arg)
    - bytes 4-5 `F3 02` maximum allowed rounds per minute (little endian uint16_t)
    - with the default 8.5" wheel `0x02f3` translates to ~30.7km/h
    - `(0x02f3 * 0.2159 * pi * 60) / 1000 = 30.7256`

## Address `23` request detailed information
- `00 01 02 03 04` (index in decimal)
- `03 23 00 31 18`
- bytes 0-3 `03 23 00 31`: is the packet header (length, address, command, arg)
- byte 4: ? probably location or length of data to read

## Address `11` detailed information
- arg `00`
    - `34 11 33 00 E6 6A 00 00 00 00 00 00 00 00 00 00 00 00 00 00 0B 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 98 0E 01 00 00 00 00 00`
    - bytes 0-3 `34 11 33 00`: is the packet header (length, address, command, arg)
    - bytes 4-7 `E6 6A 00 00`: total operation time in seconds (little endian uint32_t)
    - bytes 20-23 `0B 00 00 00`: (unsure) operation time since boot in seconds (little endian uint16_t/uint32_t)
    - bytes 46-47 `98 0E`: voltage in cV (little endian uint16_t), e.g. `98 0E` = 0x0E98 = 3736cV = 37.36V
    - bytes 48-49 `01 00`: (unsure) current in cA (little endian uint16_t), e.g. `1C 00` = 0x001C = 28cA = 0.28A
- arg `28`
    - `34 11 33 28 78 80 00 00 00 00 7D 02 03 00 00 00 00 00 00 00 35 00 00 00 00 00 00 00 89 13 00 00 00 00 B0 49 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 3F 6B`
    - bytes 0-3 `34 11 33 28`: is the packet header (length, address, command, arg)
    - bytes 10-13 `7D 02 03 00`: mainboard version, should be 0003027d (little endian uint32_t)
    - byte 20 `35`: state of charge in %
    - bytes 28-29 `89 13`: speed in meters/hour (little endian uint16_t)
    - bytes 34-37 `B0 49 00 00`: odometer in m (little endian uint32_t), `B0 49 00 00` = 0x000049B0 = 18864 m = 18.864 km

## Address `25` input information
- `00 01 02 03 04 05 06 07 08` (index in decimal)
- `07 25 60 05 04 2C 2C 00 00`
- bytes 0-3 `07 25 60 05`: is the packet header (length, address, command, arg)
- `04 2C 2C 00 00` (bytes 4-8) is the payload
  - byte 4: ?
  - byte 5: acceleration lever (min 2C, max C5)
  - byte 6: electric brake lever (min 2C, max B5)
  - byte 7: ?
  - byte 8: ?
- when the mechaical brake is pulled bytes 5&6 are `2C B5`

## Address `27` input information 2
- `00 01 02 03 04 05 06 07 08 09 10` (index in decimal)
- `09 27 63 07 06 2C 2C 00 00 00 04`
- bytes 0-3 `09 27 63 07`: is the packet header (length, address, command, arg)
- bytes 4-8: same as in the packet to address `25`
- byte 9: ?
- byte 10: ?
- the packet to address `25` is sent four times, then the packet to address `27`
is sent once, resulting in five packets one of them to `27`.

## Address `28` motor controller information
- `00 01 02 03 04 05 06 07 08 09 10 11 12` (index in decimal)
- `0B 28 6D 09 00 07 00 00 00 00 00 00 61`
- bytes 0-3 `0B 28 6D 09`: is the packet header (length, address, command, arg)
- `00 07 00 00 00 00 00 00 61` (bytes 4-12) is the payload
  - byte 4: `00` for normal mode and `02` for eco mode
  - byte 5: `07` running, `08` before shutting down
  - byte 6: `00` when lights are off, `01` when lights are on
  - byte 7: ?
  - byte 8-9: speed in meters/hour (little endian uint16_t), max speed is `07 4E` = 19975 m/h = 19.975 km/h
  - byte 10: `01` after the button was pressed, `00` otherwise
  - byte 11: error code (see e.g. https://elewheels.com/error-code-xiaomi-m365-scooter/)
  - byte 12: state of charge in %