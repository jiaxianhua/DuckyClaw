# Car Control

Control an ESP32-based car via HTTP commands. The car supports movement commands and can execute sequences of actions.

## When to use

When the user asks to control the car, such as:
- "小车前进1000ms" / "car move forward 1000ms"
- "小车后退500" / "car backward 500"
- "小车左转90度" / "car rotate left 90"
- "小车IP为192.168.3.100" / "set car IP to 192.168.3.100"
- Complex sequences like "小车先前进100，然后左转90，再前进50"

## Available tools

1. **car_set_ip** - Set the car's IP address (default: 192.168.3.111)
2. **car_command** - Send a single command
3. **car_sequence** - Send a sequence of commands

## Supported actions

- `forward` (前进) - Move forward
- `backward` (后退) - Move backward
- `left` (左平移) - Move left
- `right` (右平移) - Move right
- `rotate_left` (右转) - Turn RIGHT (hardware reversed)
- `rotate_right` (左转) - Turn LEFT (hardware reversed)
- `stop` (停止) - Stop

## IMPORTANT: Rotation mapping

The hardware rotation is reversed:
- When user says "左转" (turn left) → use `rotate_right`
- When user says "右转" (turn right) → use `rotate_left`

## How to use

1. **Set IP** (if needed): Use car_set_ip(ip_address="192.168.3.xxx")
2. **Single command**: Use car_command(action="forward", value="1000")
3. **Sequence**: Use car_sequence with JSON array:
   [{"action":"forward","value":100},{"action":"rotate_left","value":90}]

## Examples

User: "小车前进1000ms"
-> car_command(action="forward", value="1000")

User: "小车左转90度"
-> car_command(action="rotate_right", value="90")

User: "小车右转90度"
-> car_command(action="rotate_left", value="90")

User: "小车先前进100，左转90，再前进50"
-> car_sequence(sequence="[{\"action\":\"forward\",\"value\":100},{\"action\":\"rotate_right\",\"value\":90},{\"action\":\"forward\",\"value\":50}]")

User: "小车IP为192.168.3.100"
-> car_set_ip(ip_address="192.168.3.100")
