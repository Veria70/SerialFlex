/**
 * SerialFlex Library Example
 * 
 * This example demonstrates the usage of the SerialFlex library
 * for serializing, framing, and transmitting data structures.
 */

 #include "serialflex.hpp"
 #include <iostream>
 #include <iomanip>
 #include <chrono>
 
 //! Example custom data structure with serialization support
 struct SensorData {
     float temperature;
     float humidity;
     uint32_t timestamp;
     std::string sensorId;
     std::vector<uint16_t> readings;
     
     //! Custom serialization method
     std::vector<uint8_t> serialize() const {
         std::vector<uint8_t> result;
         
         //! Add POD members
         const uint8_t* tempPtr = reinterpret_cast<const uint8_t*>(&temperature);
         result.insert(result.end(), tempPtr, tempPtr + sizeof(temperature));
         
         const uint8_t* humidPtr = reinterpret_cast<const uint8_t*>(&humidity);
         result.insert(result.end(), humidPtr, humidPtr + sizeof(humidity));
         
         const uint8_t* timePtr = reinterpret_cast<const uint8_t*>(&timestamp);
         result.insert(result.end(), timePtr, timePtr + sizeof(timestamp));
         
         //! Add string length and data
         uint32_t strLength = static_cast<uint32_t>(sensorId.size());
         const uint8_t* lenPtr = reinterpret_cast<const uint8_t*>(&strLength);
         result.insert(result.end(), lenPtr, lenPtr + sizeof(strLength));
         
         result.insert(result.end(), sensorId.begin(), sensorId.end());
         
         //! Add readings vector
         uint32_t readingsLength = static_cast<uint32_t>(readings.size());
         const uint8_t* readingsLenPtr = reinterpret_cast<const uint8_t*>(&readingsLength);
         result.insert(result.end(), readingsLenPtr, readingsLenPtr + sizeof(readingsLength));
         
         for (const auto& reading : readings) {
             const uint8_t* readingPtr = reinterpret_cast<const uint8_t*>(&reading);
             result.insert(result.end(), readingPtr, readingPtr + sizeof(reading));
         }
         
         return result;
     }
     
     //! Static deserialization method
     static SensorData deserialize(serialflex::ByteReader& reader) {
         SensorData data;
         data.temperature = reader.read<float>();
         data.humidity = reader.read<float>();
         data.timestamp = reader.read<uint32_t>();
         
         uint32_t strLength = reader.read<uint32_t>();
         auto bytes = reader.readBytes(strLength);
         data.sensorId = std::string(bytes.begin(), bytes.end());
         
         uint32_t readingsLength = reader.read<uint32_t>();
         data.readings.reserve(readingsLength);
         for (uint32_t i = 0; i < readingsLength; i++) {
             data.readings.push_back(reader.read<uint16_t>());
         }
         
         return data;
     }
 };
 
 //! Nested structure example with explicit serialization/deserialization
 struct Command {
     enum class CommandType : uint8_t {
         GET = 1,
         SET = 2,
         RESET = 3,
         UPDATE = 4
     };
     
     CommandType type;
     uint16_t deviceId;
     std::string targetName;
     std::vector<uint8_t> payload;
     
     //! Additional nested structure
     struct Parameter {
         uint16_t paramId;
         float value;
     };
     
     std::vector<Parameter> parameters;
     
     //! Custom serialization method
     std::vector<uint8_t> serialize() const {
         std::vector<uint8_t> result;
         
         //! Serialize command type
         result.push_back(static_cast<uint8_t>(type));
         
         //! Serialize device ID
         const uint8_t* deviceIdPtr = reinterpret_cast<const uint8_t*>(&deviceId);
         result.insert(result.end(), deviceIdPtr, deviceIdPtr + sizeof(deviceId));
         
         //! Serialize target name
         uint32_t nameLength = static_cast<uint32_t>(targetName.size());
         const uint8_t* nameLenPtr = reinterpret_cast<const uint8_t*>(&nameLength);
         result.insert(result.end(), nameLenPtr, nameLenPtr + sizeof(nameLength));
         result.insert(result.end(), targetName.begin(), targetName.end());
         
         //! Serialize payload
         uint32_t payloadLength = static_cast<uint32_t>(payload.size());
         const uint8_t* payloadLenPtr = reinterpret_cast<const uint8_t*>(&payloadLength);
         result.insert(result.end(), payloadLenPtr, payloadLenPtr + sizeof(payloadLength));
         result.insert(result.end(), payload.begin(), payload.end());
         
         //! Serialize parameters
         uint32_t paramCount = static_cast<uint32_t>(parameters.size());
         const uint8_t* paramCountPtr = reinterpret_cast<const uint8_t*>(&paramCount);
         result.insert(result.end(), paramCountPtr, paramCountPtr + sizeof(paramCount));
         
         for (const auto& param : parameters) {
             const uint8_t* paramIdPtr = reinterpret_cast<const uint8_t*>(&param.paramId);
             result.insert(result.end(), paramIdPtr, paramIdPtr + sizeof(param.paramId));
             
             const uint8_t* valuePtr = reinterpret_cast<const uint8_t*>(&param.value);
             result.insert(result.end(), valuePtr, valuePtr + sizeof(param.value));
         }
         
         return result;
     }
     
     //! Static deserialization method
     static Command deserialize(serialflex::ByteReader& reader) {
         Command cmd;
         
         //! Read command type
         cmd.type = static_cast<CommandType>(reader.read<uint8_t>());
         
         //! Read device ID
         cmd.deviceId = reader.read<uint16_t>();
         
         //! Read target name
         uint32_t nameLength = reader.read<uint32_t>();
         auto nameBytes = reader.readBytes(nameLength);
         cmd.targetName = std::string(nameBytes.begin(), nameBytes.end());
         
         //! Read payload
         uint32_t payloadLength = reader.read<uint32_t>();
         cmd.payload = reader.readBytes(payloadLength);
         
         //! Read parameters
         uint32_t paramCount = reader.read<uint32_t>();
         cmd.parameters.reserve(paramCount);
         
         for (uint32_t i = 0; i < paramCount; i++) {
             Parameter param;
             param.paramId = reader.read<uint16_t>();
             param.value = reader.read<float>();
             cmd.parameters.push_back(param);
         }
         
         return cmd;
     }
 };
 
 //! Helper function to print a byte vector as hex
 void printHex(const std::vector<uint8_t>& data, const std::string& label) {
     std::cout << label << " (" << data.size() << " bytes): ";
     for (const auto& byte : data) {
         std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << " ";
     }
     std::cout << std::dec << std::endl;
 }
 
 //! Helper function to simulate data transmission over a channel
 void simulateTransmission(const std::vector<uint8_t>& data) {
     std::cout << "\nSimulating transmission..." << std::endl;
     
     //! Create a receiver to process incoming bytes
     serialflex::PacketReceiver receiver;
     serialflex::DeframedPacket deframedPacket;
     
     //! Process each byte one by one (as would happen in a real serial transmission)
     for (uint8_t byte : data) {
         if (receiver.processByte(byte, deframedPacket)) {
             if (deframedPacket.valid) {
                 std::cout << "Received valid packet with ID: " << static_cast<int>(deframedPacket.messageId) << std::endl;
                 std::cout << "Payload size: " << deframedPacket.payload.size() << " bytes" << std::endl;
             } else {
                 std::cout << "Received invalid packet: " << deframedPacket.errorReason << std::endl;
             }
         }
     }
 }
 
 //! Example 1: Basic serialization of built-in types
 void example1_basicTypes() {
     std::cout << "\n=== Example 1: Basic Types Serialization ===" << std::endl;
     
     //! Serialize an integer
     int32_t testInt = 42;
     auto serializedInt = serialflex::serialize(testInt);
     printHex(serializedInt, "Serialized int32_t");
     
     //! Deserialize back to original type
     int32_t deserializedInt = serialflex::deserialize<int32_t>(serializedInt);
     std::cout << "Original: " << testInt << ", Deserialized: " << deserializedInt << std::endl;
     
     //! Serialize a floating-point number
     double testDouble = 3.14159;
     auto serializedDouble = serialflex::serialize(testDouble);
     printHex(serializedDouble, "Serialized double");
     
     //! Deserialize back to original type
     double deserializedDouble = serialflex::deserialize<double>(serializedDouble);
     std::cout << "Original: " << testDouble << ", Deserialized: " << deserializedDouble << std::endl;
 }
 
 //! Example 2: Container serialization
 void example2_containers() {
     std::cout << "\n=== Example 2: Container Serialization ===" << std::endl;
     
     //! Serialize a string
     std::string testString = "Hello, SerialFlex!";
     auto serializedString = serialflex::serialize(testString);
     printHex(serializedString, "Serialized string");
     
     //! Deserialize back to original type
     std::string deserializedString = serialflex::deserialize<std::string>(serializedString);
     std::cout << "Original: " << testString << ", Deserialized: " << deserializedString << std::endl;
     
     //! Serialize a vector of integers
     std::vector<int> testVector = {1, 2, 3, 4, 5};
     auto serializedVector = serialflex::serialize(testVector);
     printHex(serializedVector, "Serialized vector<int>");
     
     //! Deserialize back to original type
     std::vector<int> deserializedVector = serialflex::deserialize<std::vector<int>>(serializedVector);
     std::cout << "Original: [";
     for (size_t i = 0; i < testVector.size(); i++) {
         std::cout << testVector[i];
         if (i < testVector.size() - 1) std::cout << ", ";
     }
     std::cout << "], Deserialized: [";
     for (size_t i = 0; i < deserializedVector.size(); i++) {
         std::cout << deserializedVector[i];
         if (i < deserializedVector.size() - 1) std::cout << ", ";
     }
     std::cout << "]" << std::endl;
 }
 
 //! Example 3: Custom data structure
 void example3_customType() {
     std::cout << "\n=== Example 3: Custom Data Structure ===" << std::endl;
     
     //! Create sensor data
     SensorData sensorData = {
         22.5f,                                       //! temperature
         65.0f,                                       //! humidity
         static_cast<uint32_t>(time(nullptr)),       //! timestamp
         "SENSOR_001",                                //! sensorId
         {1024, 2048, 4096}                           //! readings
     };
     
     //! Serialize the data
     auto serialized = serialflex::serialize(sensorData);
     printHex(serialized, "Serialized SensorData");
     
     //! Deserialize back to original type
     SensorData deserialized = serialflex::deserialize<SensorData>(serialized);
     
     //! Verify the data
     std::cout << "Original data:" << std::endl;
     std::cout << "  Temperature: " << sensorData.temperature << "°C" << std::endl;
     std::cout << "  Humidity: " << sensorData.humidity << "%" << std::endl;
     std::cout << "  Timestamp: " << sensorData.timestamp << std::endl;
     std::cout << "  Sensor ID: " << sensorData.sensorId << std::endl;
     std::cout << "  Readings: [";
     for (size_t i = 0; i < sensorData.readings.size(); i++) {
         std::cout << sensorData.readings[i];
         if (i < sensorData.readings.size() - 1) std::cout << ", ";
     }
     std::cout << "]" << std::endl;
     
     std::cout << "Deserialized data:" << std::endl;
     std::cout << "  Temperature: " << deserialized.temperature << "°C" << std::endl;
     std::cout << "  Humidity: " << deserialized.humidity << "%" << std::endl;
     std::cout << "  Timestamp: " << deserialized.timestamp << std::endl;
     std::cout << "  Sensor ID: " << deserialized.sensorId << std::endl;
     std::cout << "  Readings: [";
     for (size_t i = 0; i < deserialized.readings.size(); i++) {
         std::cout << deserialized.readings[i];
         if (i < deserialized.readings.size() - 1) std::cout << ", ";
     }
     std::cout << "]" << std::endl;
 }
 
 //! Example 4: Packet framing and CRC validation
 void example4_packetFraming() {
     std::cout << "\n=== Example 4: Packet Framing ===" << std::endl;
     
     //! Create sensor data
     SensorData sensorData = {
         22.5f,                                       //! temperature
         65.0f,                                       //! humidity
         static_cast<uint32_t>(time(nullptr)),       //! timestamp
         "SENSOR_001",                                //! sensorId
         {1024, 2048, 4096}                           //! readings
     };
     
     //! Create a packet with message ID 0x01
     auto packet = serialflex::createPacket(0x01, sensorData);
     printHex(packet, "Framed packet");
     
     //! Deframe the packet
     auto deframed = serialflex::PacketFramer::deframePacket(packet);
     
     if (deframed.valid) {
         std::cout << "Packet is valid." << std::endl;
         std::cout << "Message ID: " << static_cast<int>(deframed.messageId) << std::endl;
         std::cout << "Payload size: " << deframed.payload.size() << " bytes" << std::endl;
         
         //! Deserialize the payload
         SensorData deserialized = serialflex::deserialize<SensorData>(deframed.payload);
         
         //! Verify the data
         std::cout << "Deserialized data:" << std::endl;
         std::cout << "  Temperature: " << deserialized.temperature << "°C" << std::endl;
         std::cout << "  Humidity: " << deserialized.humidity << "%" << std::endl;
         std::cout << "  Timestamp: " << deserialized.timestamp << std::endl;
         std::cout << "  Sensor ID: " << deserialized.sensorId << std::endl;
     } else {
         std::cout << "Packet is invalid: " << deframed.errorReason << std::endl;
     }
     
     //! Simulate a corrupted packet (change a byte in the middle)
     std::vector<uint8_t> corruptedPacket = packet;
     if (corruptedPacket.size() > 10) {
         corruptedPacket[10] ^= 0xFF; //! Flip all bits in a data byte
     }
     
     //! Try to deframe the corrupted packet
     auto deframedCorrupted = serialflex::PacketFramer::deframePacket(corruptedPacket);
     
     if (deframedCorrupted.valid) {
         std::cout << "Corrupted packet is valid (This shouldn't happen)." << std::endl;
     } else {
         std::cout << "Corrupted packet is correctly detected as invalid: " 
                   << deframedCorrupted.errorReason << std::endl;
     }
     
     //! Demonstrate byte-by-byte processing
     simulateTransmission(packet);
 }
 
 //! Example 5: Complex nested structure
 void example5_complexType() {
     std::cout << "\n=== Example 5: Complex Nested Structure ===" << std::endl;
     
     //! Create a command
     Command cmd;
     cmd.type = Command::CommandType::SET;
     cmd.deviceId = 0x1234;
     cmd.targetName = "motor_controller";
     cmd.payload = {0x01, 0x02, 0x03, 0x04};
     cmd.parameters.push_back({1, 3.14f});
     cmd.parameters.push_back({2, 2.71f});
     
     //! Serialize the command
     auto serialized = serialflex::serialize(cmd);
     printHex(serialized, "Serialized Command");
     
     //! Create a packet with message ID 0x02
     auto packet = serialflex::createPacket(0x02, cmd);
     printHex(packet, "Framed Command packet");
     
     //! Deserialize using the convenience wrapper
     auto [success, deserializedCmd] = serialflex::parsePacket<Command>(packet);
     
     if (success) {
         std::cout << "Successfully parsed command packet." << std::endl;
         std::cout << "Command type: " << static_cast<int>(static_cast<uint8_t>(deserializedCmd.type)) << std::endl;
         std::cout << "Device ID: 0x" << std::hex << deserializedCmd.deviceId << std::dec << std::endl;
         std::cout << "Target name: " << deserializedCmd.targetName << std::endl;
         std::cout << "Payload size: " << deserializedCmd.payload.size() << " bytes" << std::endl;
         std::cout << "Parameter count: " << deserializedCmd.parameters.size() << std::endl;
         for (size_t i = 0; i < deserializedCmd.parameters.size(); i++) {
             std::cout << "  Parameter " << i << ": ID=" << deserializedCmd.parameters[i].paramId 
                      << ", Value=" << deserializedCmd.parameters[i].value << std::endl;
         }
     } else {
         std::cout << "Failed to parse command packet." << std::endl;
     }
 }
 
 //! Example 6: Performance test
 void example6_performance() {
     std::cout << "\n=== Example 6: Performance Test ===" << std::endl;
     
     constexpr int testCount = 10000;
     
     //! Create test data
     SensorData sensorData = {
         22.5f,                                       //! temperature
         65.0f,                                       //! humidity
         static_cast<uint32_t>(time(nullptr)),       //! timestamp
         "SENSOR_001",                                //! sensorId
         {1024, 2048, 4096, 8192, 16384}              //! readings
     };
     
     //! Measure serialization performance
     auto startSer = std::chrono::high_resolution_clock::now();
     
     for (int i = 0; i < testCount; i++) {
         auto serialized = serialflex::serialize(sensorData);
     }
     
     auto endSer = std::chrono::high_resolution_clock::now();
     auto serializationTime = std::chrono::duration_cast<std::chrono::microseconds>(endSer - startSer).count();
     
     //! Pre-serialize for deserialization test
     auto serialized = serialflex::serialize(sensorData);
     
     //! Measure deserialization performance
     auto startDeser = std::chrono::high_resolution_clock::now();
     
     for (int i = 0; i < testCount; i++) {
         auto deserialized = serialflex::deserialize<SensorData>(serialized);
     }
     
     auto endDeser = std::chrono::high_resolution_clock::now();
     auto deserializationTime = std::chrono::duration_cast<std::chrono::microseconds>(endDeser - startDeser).count();
     
     //! Measure packet creation performance
     auto startPack = std::chrono::high_resolution_clock::now();
     
     for (int i = 0; i < testCount; i++) {
         auto packet = serialflex::createPacket(0x01, sensorData);
     }
     
     auto endPack = std::chrono::high_resolution_clock::now();
     auto packTime = std::chrono::duration_cast<std::chrono::microseconds>(endPack - startPack).count();
     
     //! Pre-create packet for parsing test
     auto packet = serialflex::createPacket(0x01, sensorData);
     
     //! Measure packet parsing performance
     auto startParse = std::chrono::high_resolution_clock::now();
     
     for (int i = 0; i < testCount; i++) {
         auto [success, deserialized] = serialflex::parsePacket<SensorData>(packet);
     }
     
     auto endParse = std::chrono::high_resolution_clock::now();
     auto parseTime = std::chrono::duration_cast<std::chrono::microseconds>(endParse - startParse).count();
     
     //! Output performance results
     std::cout << "Performance results (" << testCount << " iterations):" << std::endl;
     std::cout << "  Serialization:   " << serializationTime / 1000.0 << " ms (" 
               << serializationTime / static_cast<double>(testCount) << " µs per operation)" << std::endl;
     std::cout << "  Deserialization: " << deserializationTime / 1000.0 << " ms (" 
               << deserializationTime / static_cast<double>(testCount) << " µs per operation)" << std::endl;
     std::cout << "  Packet creation: " << packTime / 1000.0 << " ms (" 
               << packTime / static_cast<double>(testCount) << " µs per operation)" << std::endl;
     std::cout << "  Packet parsing:  " << parseTime / 1000.0 << " ms (" 
               << parseTime / static_cast<double>(testCount) << " µs per operation)" << std::endl;
 }
 
 //! Example 7: CRC calculations
 void example7_crc() {
     std::cout << "\n=== Example 7: CRC Calculations ===" << std::endl;
     
     //! Test data
     const char* testData = "123456789"; //! Standard test string for CRC algorithms
     const uint8_t* data = reinterpret_cast<const uint8_t*>(testData);
     size_t length = strlen(testData);
     
     //! Calculate different CRC values
     uint8_t crc8 = serialflex::CRC::calculateCRC8(data, length);
     uint16_t crc16 = serialflex::CRC::calculateCRC16(data, length);
     uint32_t crc32 = serialflex::CRC::calculateCRC32(data, length);
     
     //! Output CRC values
     std::cout << "Test data: \"" << testData << "\"" << std::endl;
     std::cout << "CRC-8:  0x" << std::hex << std::setw(2) << std::setfill('0') 
               << static_cast<int>(crc8) << std::dec << std::endl;
     std::cout << "CRC-16: 0x" << std::hex << std::setw(4) << std::setfill('0') 
               << crc16 << std::dec << std::endl;
     std::cout << "CRC-32: 0x" << std::hex << std::setw(8) << std::setfill('0') 
               << crc32 << std::dec << std::endl;
     
     //! Expected values (may vary depending on exact polynomial and implementation)
     std::cout << "Expected CRC-8 (x^8 + x^5 + x^4 + 1):  0xF4" << std::endl;
     std::cout << "Expected CRC-16 CCITT (x^16 + x^12 + x^5 + 1): 0x29B1" << std::endl;
     std::cout << "Expected CRC-32 IEEE 802.3 (x^32 + x^26 + ... + 1): 0xCBF43926" << std::endl;
 }
 
 int main() {
     std::cout << "SerialFlex Library Example" << std::endl;
     std::cout << "==========================" << std::endl;
     
     //! Run all examples
     example1_basicTypes();
     example2_containers();
     example3_customType();
     example4_packetFraming();
     example5_complexType();
     example6_performance();
     example7_crc();
     
     return 0;
 }