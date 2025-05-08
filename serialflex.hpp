#include <vector>
#include <cstdint>
#include <string>
#include <memory>
#include <type_traits>
#include <stdexcept>
#include <algorithm>
#include <array>
#include <unordered_map>
#include <cstring>

namespace serialflex {

//! --------------------------------
//! CRC IMPLEMENTATION
//! --------------------------------

class CRC {
public:
    //! Calculate CRC-16 (CCITT) - standard implementation
    static uint16_t calculateCRC16(const uint8_t* data, size_t length) {
        uint16_t crc = 0xFFFF; //! Initial value
        
        for (size_t i = 0; i < length; i++) {
            crc ^= static_cast<uint16_t>(data[i]) << 8;
            
            for (uint8_t j = 0; j < 8; j++) {
                if (crc & 0x8000) {
                    crc = (crc << 1) ^ 0x1021; //! CCITT polynomial: x^16 + x^12 + x^5 + 1
                } else {
                    crc = crc << 1;
                }
            }
        }
        
        return crc;
    }
    
    //! Calculate CRC-32 (IEEE 802.3)
    static uint32_t calculateCRC32(const uint8_t* data, size_t length) {
        uint32_t crc = 0xFFFFFFFF; //! Initial value
        
        for (size_t i = 0; i < length; i++) {
            crc ^= data[i];
            
            for (uint8_t j = 0; j < 8; j++) {
                if (crc & 1) {
                    crc = (crc >> 1) ^ 0xEDB88320; //! IEEE 802.3 polynomial (reversed)
                } else {
                    crc = crc >> 1;
                }
            }
        }
        
        return ~crc; //! Final XOR
    }
    
    //! Calculate CRC-8
    static uint8_t calculateCRC8(const uint8_t* data, size_t length) {
        uint8_t crc = 0xFF; //! Initial value
        
        for (size_t i = 0; i < length; i++) {
            crc ^= data[i];
            
            for (uint8_t j = 0; j < 8; j++) {
                if (crc & 0x80) {
                    crc = (crc << 1) ^ 0x31; //! Polynomial: x^8 + x^5 + x^4 + 1
                } else {
                    crc = crc << 1;
                }
            }
        }
        
        return crc;
    }
};

//! --------------------------------
//! TYPE TRAITS (simplified)
//! --------------------------------

//! Check if type has a serialize method
template<typename T>
class has_serialize_method {
    typedef char one;
    typedef long two;

    template<typename C> static one test(decltype(&C::serialize));
    template<typename C> static two test(...);

public:
    enum { value = sizeof(test<T>(0)) == sizeof(char) };
};

//! Helper functions to detect container operations
namespace detail {
    //! Test for begin/end
    template<typename T>
    auto has_begin_end_impl(int) 
        -> decltype(std::begin(std::declval<T>()), 
                   std::end(std::declval<T>()), 
                   std::true_type{});
                   
    template<typename T>
    std::false_type has_begin_end_impl(...);
    
    //! Test for size
    template<typename T>
    auto has_size_impl(int) 
        -> decltype(std::declval<T>().size(), std::true_type{});
        
    template<typename T>
    std::false_type has_size_impl(...);
    
    //! Test for reserve
    template<typename T>
    auto has_reserve_impl(int) 
        -> decltype(std::declval<T>().reserve(std::size_t{}), std::true_type{});
        
    template<typename T>
    std::false_type has_reserve_impl(...);
    
    //! Test for push_back with specific value type
    template<typename T, typename V>
    auto has_push_back_impl(int) 
        -> decltype(std::declval<T>().push_back(std::declval<V>()), std::true_type{});
        
    template<typename T, typename V>
    std::false_type has_push_back_impl(...);
}

//! Public type traits
template<typename T>
using has_begin_end = decltype(detail::has_begin_end_impl<T>(0));

template<typename T>
using has_size = decltype(detail::has_size_impl<T>(0));

template<typename T>
struct is_container {
    static constexpr bool value = has_begin_end<T>::value && has_size<T>::value;
};

template<typename T>
using has_reserve = decltype(detail::has_reserve_impl<T>(0));

template<typename T, typename V>
using has_push_back = decltype(detail::has_push_back_impl<T, V>(0));

//! --------------------------------
//! SERIALIZATION IMPLEMENTATION
//! --------------------------------

//! Forward declaration
template<typename T>
std::vector<uint8_t> serialize(const T& data);

//! Main serialization function
template<typename T>
std::vector<uint8_t> serialize(const T& data) {
    if constexpr (std::is_trivially_copyable_v<T>) {
        //! For POD types, direct memory copy
        const uint8_t* begin = reinterpret_cast<const uint8_t*>(&data);
        return std::vector<uint8_t>(begin, begin + sizeof(T));
    } 
    else if constexpr (is_container<T>::value) {
        //! For containers like vector, string, etc.
        std::vector<uint8_t> result;
        
        //! Serialize container size (32-bit)
        uint32_t size = static_cast<uint32_t>(data.size());
        const uint8_t* sizePtr = reinterpret_cast<const uint8_t*>(&size);
        result.insert(result.end(), sizePtr, sizePtr + sizeof(size));
        
        //! Serialize each element
        for (const auto& element : data) {
            auto elementBytes = serialize(element);
            result.insert(result.end(), elementBytes.begin(), elementBytes.end());
        }
        return result;
    } 
    else if constexpr (has_serialize_method<T>::value) {
        //! For custom types with their own serialize method
        return data.serialize();
    } 
    else {
        //! Fallback for complex types without a serialize method
        static_assert(std::is_trivially_copyable_v<T> || 
                     is_container<T>::value || 
                     has_serialize_method<T>::value,
            "Type must be trivially copyable, a container, or have a serialize method");
        return {}; //! Unreachable, but makes compiler happy
    }
}

//! --------------------------------
//! PACKET FRAMING
//! --------------------------------

class PacketFramer {
public:
    //! Constants for packet structure
    static constexpr uint8_t START_BYTE = 0x7E;
    static constexpr uint8_t END_BYTE = 0x7D;
    static constexpr uint8_t ESCAPE_BYTE = 0x7C;
    
    //! Frame a payload with start/end bytes and byte stuffing
    static std::vector<uint8_t> framePacket(uint8_t messageId, const std::vector<uint8_t>& payload) {
        std::vector<uint8_t> packet;
        packet.reserve(payload.size() + 10); //! Reserve space for overhead
        
        packet.push_back(START_BYTE);
        packet.push_back(messageId);
        
        //! Add length (16-bit, little endian)
        uint16_t dataSize = static_cast<uint16_t>(payload.size());
        packet.push_back(static_cast<uint8_t>(dataSize & 0xFF));
        packet.push_back(static_cast<uint8_t>((dataSize >> 8) & 0xFF));
        
        //! Add data with byte stuffing
        for (uint8_t byte : payload) {
            if (byte == START_BYTE || byte == END_BYTE || byte == ESCAPE_BYTE) {
                packet.push_back(ESCAPE_BYTE);
                packet.push_back(byte ^ 0x20); //! XOR for escaping
            } else {
                packet.push_back(byte);
            }
        }
        
        //! Add CRC-16 (CCITT)
        uint16_t crc = CRC::calculateCRC16(packet.data() + 1, packet.size() - 1);
        packet.push_back(static_cast<uint8_t>(crc & 0xFF));
        packet.push_back(static_cast<uint8_t>((crc >> 8) & 0xFF));
        
        //! Add end byte
        packet.push_back(END_BYTE);
        
        return packet;
    }
    
    //! Structure to hold deframed packet data
    struct DeframedPacket {
        uint8_t messageId;
        std::vector<uint8_t> payload;
        bool valid;
        std::string errorReason;
    };
    
    //! Process a complete framed packet
    static DeframedPacket deframePacket(const std::vector<uint8_t>& packet) {
        DeframedPacket result;
        result.valid = false;
        
        //! Basic validation
        if (packet.size() < 7) { //! Minimum packet size (START + ID + LEN[2] + CRC[2] + END)
            result.errorReason = "Packet too small";
            return result;
        }
        
        if (packet[0] != START_BYTE || packet.back() != END_BYTE) {
            result.errorReason = "Invalid frame markers";
            return result;
        }
        
        //! Extract message ID
        result.messageId = packet[1];
        
        //! Extract length
        uint16_t length = static_cast<uint16_t>(packet[2]) | 
                         (static_cast<uint16_t>(packet[3]) << 8);
                         
        //! Verify packet size matches expected length
        size_t expectedPacketSize = length + 7; //! START + ID + LEN[2] + DATA[length] + CRC[2] + END
        if (packet.size() != expectedPacketSize) {
            result.errorReason = "Length mismatch";
            return result;
        }
        
        //! Verify CRC
        size_t crcPos = packet.size() - 3;
        uint16_t receivedCrc = static_cast<uint16_t>(packet[crcPos]) | 
                              (static_cast<uint16_t>(packet[crcPos + 1]) << 8);
        uint16_t calculatedCrc = CRC::calculateCRC16(packet.data() + 1, crcPos - 1);
        
        if (receivedCrc != calculatedCrc) {
            result.errorReason = "CRC mismatch";
            return result;
        }
        
        //! Extract data portion (excluding header, CRC, and end byte)
        result.payload = std::vector<uint8_t>(packet.begin() + 4, packet.begin() + crcPos);
        result.valid = true;
        return result;
    }
    
    //! Stateful packet receiver to process byte-by-byte
    class PacketReceiver {
    public:
        PacketReceiver() : inPacket_(false), escapeNext_(false) {}
        
        //! Process a single byte, returns true if a complete packet was received
        bool processByte(uint8_t byte, DeframedPacket& outPacket) {
            static const size_t MAX_PACKET_SIZE = 1024;
            
            if (!inPacket_ && byte == START_BYTE) {
                buffer_.clear();
                buffer_.push_back(byte);
                inPacket_ = true;
                escapeNext_ = false;
                return false;
            } 
            else if (inPacket_) {
                if (escapeNext_) {
                    buffer_.push_back(byte ^ 0x20); //! Unescape
                    escapeNext_ = false;
                } 
                else if (byte == ESCAPE_BYTE) {
                    escapeNext_ = true;
                } 
                else if (byte == END_BYTE) {
                    buffer_.push_back(byte);
                    outPacket = deframePacket(buffer_);
                    inPacket_ = false;
                    return true;
                } 
                else {
                    buffer_.push_back(byte);
                }
                
                //! Safety check for buffer overflow
                if (buffer_.size() > MAX_PACKET_SIZE) {
                    inPacket_ = false;
                    outPacket.valid = false;
                    outPacket.errorReason = "Buffer overflow";
                    return true;
                }
            }
            
            return false;
        }
        
    private:
        std::vector<uint8_t> buffer_;
        bool inPacket_;
        bool escapeNext_;
    };
};

//! --------------------------------
//! DESERIALIZATION IMPLEMENTATION
//! --------------------------------

//! Exception for deserialization errors
class DeserializationError : public std::runtime_error {
public:
    DeserializationError(const std::string& msg) : std::runtime_error(msg) {}
};

//! Helper class for tracking deserialization position
class ByteReader {
public:
    ByteReader(const std::vector<uint8_t>& data) : data_(data), pos_(0) {}
    
    template<typename T>
    T read() {
        static_assert(std::is_trivially_copyable_v<T>, "Can only read trivially copyable types directly");
        
        if (pos_ + sizeof(T) > data_.size()) {
            throw DeserializationError("Not enough data to read");
        }
        
        T value;
        std::memcpy(&value, data_.data() + pos_, sizeof(T));
        pos_ += sizeof(T);
        return value;
    }
    
    std::vector<uint8_t> readBytes(size_t count) {
        if (pos_ + count > data_.size()) {
            throw DeserializationError("Not enough data to read");
        }
        
        std::vector<uint8_t> result(data_.begin() + pos_, data_.begin() + pos_ + count);
        pos_ += count;
        return result;
    }
    
    bool hasMore() const {
        return pos_ < data_.size();
    }
    
    size_t remaining() const {
        return data_.size() - pos_;
    }

private:
    const std::vector<uint8_t>& data_;
    size_t pos_;
};

//! Forward declaration
template<typename T>
T deserialize(ByteReader& reader);

//! Main deserialization function
template<typename T>
T deserialize(ByteReader& reader) {
    if constexpr (std::is_trivially_copyable_v<T>) {
        //! For POD types (int, float, etc.)
        return reader.read<T>();
    } 
    else if constexpr (is_container<T>::value) {
        //! For containers like vector, string, etc.
        using ValueType = typename T::value_type;
        
        //! Read container size
        uint32_t size = reader.read<uint32_t>();
        
        T container;
        //! Reserve space if the container supports it
        if constexpr (has_reserve<T>::value) {
            container.reserve(size);
        }
        
        //! Deserialize each element
        for (uint32_t i = 0; i < size; i++) {
            if constexpr (has_push_back<T, typename T::value_type>::value) {
                container.push_back(deserialize<ValueType>(reader));
            } else {
                //! For fixed containers like std::array, this won't compile properly
                static_assert(!is_container<T>::value || 
                              has_push_back<T, typename T::value_type>::value, 
                              "Container must support push_back operation");
            }
        }
        
        return container;
    } 
    else {
        //! For custom types with their own deserialize method
        return T::deserialize(reader);
    }
}

//! Overload for vector of bytes
template<typename T>
T deserialize(const std::vector<uint8_t>& data) {
    ByteReader reader(data);
    return deserialize<T>(reader);
}

//! --------------------------------
//! UTILITY FUNCTIONS
//! --------------------------------

//! Convenience wrapper for serializing and framing in one step
template<typename T>
std::vector<uint8_t> createPacket(uint8_t messageId, const T& data) {
    std::vector<uint8_t> serialized = serialize(data);
    return PacketFramer::framePacket(messageId, serialized);
}

//! Convenience wrapper for deframing and deserializing in one step
template<typename T>
std::pair<bool, T> parsePacket(const std::vector<uint8_t>& packetData) {
    auto deframed = PacketFramer::deframePacket(packetData);
    if (!deframed.valid) {
        return {false, T{}};
    }
    
    try {
        T data = deserialize<T>(deframed.payload);
        return {true, data};
    } catch (const DeserializationError& e) {
        return {false, T{}};
    }
}

//! Type aliases for convenience
using DeframedPacket = PacketFramer::DeframedPacket;
using PacketReceiver = PacketFramer::PacketReceiver;

} //! namespace serialflex