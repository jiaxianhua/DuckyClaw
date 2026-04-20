# Car Control Feature

DuckyClaw can control an ESP32-based car via HTTP commands.

## Setup

1. Ensure your ESP32 car is running the WebServer with the following endpoints:
   - `POST /command` - Execute single command
   - `POST /sequence` - Execute command sequence

2. The car should be on the same network as DuckyClaw.

## Available Tools

### 1. car_set_ip

Set the car's IP address (default: 192.168.3.111).

**Example usage:**
- "小车IP为192.168.3.100"
- "Set car IP to 192.168.3.100"

### 2. car_command

Send a single command to the car.

**Supported actions:**
- `forward` (前进) - Move forward
- `backward` (后退) - Move backward
- `left` (左平移) - Move left
- `right` (右平移) - Move right
- `rotate_left` (左转) - Rotate left
- `rotate_right` (右转) - Rotate right
- `stop` (停止) - Stop

**Example usage:**
- "小车前进1000ms" → forward for 1000ms
- "小车后退500" → backward for 500ms
- "小车左转90" → rotate left for 90ms
- "小车停止" → stop

### 3. car_sequence

Send a sequence of commands to execute in order.

**Example usage:**
- "小车先前进100ms，然后左转90度，再前进50ms"

This will send:
```json
[
  {"action":"forward","value":100},
  {"action":"rotate_left","value":90},
  {"action":"forward","value":50}
]
```

## HTTP API Format

### Single Command
```bash
curl -X POST http://192.168.3.111/command \
  -H "Content-Type: application/json" \
  -d '{"action":"forward","value":1000}'
```

### Command Sequence
```bash
curl -X POST http://192.168.3.111/sequence \
  -H "Content-Type: application/json" \
  -d '[{"action":"forward","value":100},{"action":"rotate_left","value":90}]'
```

## Troubleshooting

1. **Connection failed**: Check that the car IP is correct and the car is powered on
2. **Network error**: Ensure DuckyClaw and the car are on the same WiFi network
3. **Command not working**: Verify the ESP32 WebServer is running and responding to HTTP requests
