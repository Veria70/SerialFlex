# SerialFlex

A modern, header-only C++ serialization library designed for embedded systems and cross-platform communication.
This library is your new best friend if you're coding with a microcontroller wizard who eats bits for breakfast — slap it in your project and watch your code go from ‘meh’ to ‘heck yeah,’ my dude! 😂😂😂

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)

## Overview

SerialFlex is a lightweight, flexible serialization library that provides:

- Generic serialization/deserialization for any data structure
- CRC checksum calculation (CRC-8, CRC-16, CRC-32)
- Packet framing with start/end markers and byte stuffing
- Zero hardware dependencies

The library is designed with embedded systems in mind but can be used in any C++ application that needs to serialize data for storage or transmission. It's particularly useful for IoT devices, microcontrollers, and any system that needs to communicate structured data over serial interfaces.

## Features

- **Type-safe serialization**: Works with built-in types, STL containers, and custom classes
- **Automatic type handling**: Uses template metaprogramming to select the appropriate serialization method
- **CRC checksum algorithms**: Standard implementations of CRC-8, CRC-16, and CRC-32
- **Packet framing**: Robust message framing with start/end bytes and escaping
- **Byte stuffing**: Ensures binary transparency for packet data
- **Customizable**: Easy to extend for custom data types
- **C++17 compatible**: Uses modern C++ features for clean, efficient code
- **Header-only**: No library dependencies or compilation required
- **Transport-agnostic**: Works with any communication medium (UART, SPI, I2C, network, etc.)

## Requirements

- C++17 compatible compiler
- Standard Template Library (STL)

## Installation

SerialFlex is a header-only library. Simply copy the `serialflex.hpp` file to your project and include it:

```cpp
#include "serialflex.hpp"
```

## Quick Start

### Basic Serialization

```cpp
#include "serialflex.hpp"
#include <vector>
#include <string>

// Serialize built-in types
int value = 42;
std::vector<uint8_t> serialized = serialflex::serialize(value);

// Serialize STL containers
std::vector<float> values = {1.0f, 2.0f, 3.0f};
std::vector<uint8_t> serializedVector = serialflex::serialize(values);

// Deserialize
int deserializedValue = serialflex::deserialize<int>(serialized);
std::vector<float> deserializedVector = serialflex::deserialize<std::vector<float>>(serializedVector);
```

### Creating Framed Packets

```cpp
// Create a message packet with ID 0x01
std::vector<uint8_t> packet = serialflex::createPacket(0x01, value);

// Parse the packet
auto [success, result] = serialflex::parsePacket<int>(packet);
if (success) {
    // Use result...
}
```

### Custom Data Types

```cpp
struct SensorData {
    float temperature;
    float humidity;
    std::string sensorId;
    
    // Custom serialization method
    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> result;
        // Serialization logic...
        return result;
    }
    
    // Static deserialization method
    static SensorData deserialize(serialflex::ByteReader& reader) {
        SensorData data;
        // Deserialization logic...
        return data;
    }
};

// Use like any other type
SensorData data = {22.5f, 65.0f, "SENSOR_001"};
std::vector<uint8_t> serialized = serialflex::serialize(data);
SensorData deserialized = serialflex::deserialize<SensorData>(serialized);
```

### Byte-by-Byte Packet Reception

```cpp
// Create a receiver
serialflex::PacketReceiver receiver;
serialflex::DeframedPacket packet;

// Process bytes as they arrive (e.g., from UART)
for (uint8_t byte : receivedData) {
    if (receiver.processByte(byte, packet)) {
        if (packet.valid) {
            // Process the packet based on messageId
            switch (packet.messageId) {
                case 0x01:
                    auto sensorData = serialflex::deserialize<SensorData>(packet.payload);
                    // Handle sensor data...
                    break;
                // Other message types...
            }
        }
    }
}
```

## Documentation

### Serialization

The library automatically detects the best serialization method based on the type:

1. **Built-in types** (int, float, etc.): Direct memory copy
2. **STL containers** (vector, string, etc.): Container size + each element
3. **Custom types with `serialize()` method**: User-defined serialization

### Deserialization

Deserialization requires specifying the target type:

```cpp
auto result = serialflex::deserialize<TargetType>(serializedData);
```

For custom types, implement a static `deserialize` method:

```cpp
static YourType deserialize(serialflex::ByteReader& reader);
```

### Packet Format

The packet format used by SerialFlex:

```
+------------+----------+---------+----------+--------+----------+
| START_BYTE | MSG_ID   | LENGTH  | PAYLOAD  | CRC    | END_BYTE |
| (1 byte)   | (1 byte) | (2 byte)| (n bytes)| (2 byte)| (1 byte) |
+------------+----------+---------+----------+--------+----------+
```

- **START_BYTE**: Fixed marker (0x7E) indicating the start of a packet
- **MSG_ID**: Message identifier (user-defined)
- **LENGTH**: Length of the payload (16-bit, little-endian)
- **PAYLOAD**: Serialized data (with byte stuffing applied)
- **CRC**: CRC-16 checksum of everything from MSG_ID to the end of PAYLOAD
- **END_BYTE**: Fixed marker (0x7D) indicating the end of a packet

### Byte Stuffing

To ensure binary transparency, special bytes (START_BYTE, END_BYTE, ESCAPE_BYTE) within the payload are "escaped":

1. ESCAPE_BYTE (0x7C) is inserted before the special byte
2. The special byte is XORed with 0x20

### CRC Implementation

Three CRC algorithms are provided:

- **CRC-8**: Polynomial x^8 + x^5 + x^4 + 1 (0x31)
- **CRC-16 CCITT**: Polynomial x^16 + x^12 + x^5 + 1 (0x1021)
- **CRC-32 IEEE 802.3**: Standard Ethernet polynomial (0xEDB88320, reversed)

## Examples

The repository includes a comprehensive example application demonstrating all features:

```bash
# Compile the example
g++ -std=c++17 main.cpp -o serialflex_example

# Run the example
./serialflex_example
```

### Example 1: Basic Types

```cpp
// Serialize an integer
int32_t testInt = 42;
auto serializedInt = serialflex::serialize(testInt);

// Deserialize back to original type
int32_t deserializedInt = serialflex::deserialize<int32_t>(serializedInt);
```

### Example 2: Containers

```cpp
// Serialize a string
std::string testString = "Hello, SerialFlex!";
auto serializedString = serialflex::serialize(testString);

// Deserialize back to original type
std::string deserializedString = serialflex::deserialize<std::string>(serializedString);
```

### Example 3: Custom Data Structures

```cpp
// Create sensor data
SensorData sensorData = {
    22.5f,                         // temperature
    65.0f,                         // humidity
    static_cast<uint32_t>(time(nullptr)), // timestamp
    "SENSOR_001",                  // sensorId
    {1024, 2048, 4096}             // readings
};

// Serialize the data
auto serialized = serialflex::serialize(sensorData);

// Deserialize back to original type
SensorData deserialized = serialflex::deserialize<SensorData>(serialized);
```

### Example 4: Packet Framing

```cpp
// Create a packet with message ID 0x01
auto packet = serialflex::createPacket(0x01, sensorData);

// Deframe the packet
auto deframed = serialflex::PacketFramer::deframePacket(packet);

if (deframed.valid) {
    // Deserialize the payload
    SensorData deserialized = serialflex::deserialize<SensorData>(deframed.payload);
}
```

## Performance Considerations

- Use `reserve()` on vectors when serializing large amounts of data
- Consider implementing move semantics in your custom serialize/deserialize methods
- For very resource-constrained systems, avoid using STL containers and strings
- Benchmark serialization performance for your specific data structures

## Advanced Usage

### Error Handling

```cpp
try {
    auto deserialized = serialflex::deserialize<YourType>(data);
} catch (const serialflex::DeserializationError& e) {
    std::cerr << "Deserialization failed: " << e.what() << std::endl;
}
```

### Binary Inspection

```cpp
// Print byte vector as hex
void printHex(const std::vector<uint8_t>& data) {
    for (const auto& byte : data) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') 
                  << static_cast<int>(byte) << " ";
    }
    std::cout << std::dec << std::endl;
}
```

### Custom Container Support

The library automatically handles any container type that provides:
- `begin()` and `end()` methods
- `size()` method
- `push_back()` method
- A value_type typedef

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This library is released under the MIT License. See the LICENSE file for details.

## Acknowledgments

SerialFlex was inspired by the need for a flexible, modern serialization library for embedded systems that doesn't rely on heavy external dependencies.
