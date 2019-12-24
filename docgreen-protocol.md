## Packet layout
all packages start with `55 AA` (same as with Xiaomi M365 scooters).
The packet header looks like this:
55 AA length address command arg payload[length - 2] checksumLow checksumHigh.

On the ESA 5000 there are three different packets. The following lists these
packets exluding the `55 AA` header and the checksum.

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
- bytes 0-3 `07 25 60 07`: is the packet header (length, address, command, arg)
- bytes 4-8: same as in the packet to address `25`
- byte 9: ?
- byte 10: ?
- the packet to address `25` is sent four times, then the packet to address `27`
is sent once, resulting in five packets one of them to `27`.