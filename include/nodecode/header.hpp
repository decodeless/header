// Copyright (c) 2024 Pyarelal Knowles, MIT License

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <nodecode/offset_ptr.hpp>
#include <nodecode/offset_span.hpp>
#include <string>

namespace nodecode {

struct Version {
    static constexpr uint32_t InvalidValue = 0xffffffff;
    uint32_t major = InvalidValue;
    uint32_t minor = InvalidValue;
    uint32_t patch = InvalidValue;

    static bool binaryCompatible(const Version& supported,
                                 const Version& loaded) {
        return loaded.major != InvalidValue &&
               supported.major == loaded.major &&
               supported.minor >= loaded.minor;
    }
};

static constexpr Version VersionSupported{0, 1, 0};

using GitHash = std::array<char, 40>;

struct Magic : public std::array<char, 16> {
    bool operator<(const Magic& other) const {
        return std::string(begin(), end()) <
               std::string(other.begin(), other.end());
    }
};

struct Header {
    // Must have a static Magic HeaderIdentifier;

    Magic identifier;
    Version version;
    GitHash gitHash;

    bool operator<(const Header& other) const {
        return identifier < other.identifier;
    }
};

struct RootHeader {

    using HeaderList = offset_span<offset_ptr<Header>>;

    // Find and cast a specific header
    template <class HeaderType>
        requires std::is_base_of_v<Header, HeaderType>
    inline HeaderType* find() {
        constexpr Magic headerIdentifier =
            HeaderType::HeaderIdentifier;
        HeaderList::iterator result;
        if (headers.size() < 16) {
            result = std::find_if(
                headers.begin(), headers.end(),
                [&headerIdentifier](const offset_ptr<Header>& ptr) {
                    return ptr->identifier == headerIdentifier;
                });
        } else {
            result = std::lower_bound(headers.begin(), headers.end(),
                                      headerIdentifier, HeaderPtrComp());
            if (result != headers.end() &&
                (*result)->identifier != headerIdentifier) {
                result = headers.end();
            }
        }
        return result == headers.end()
                   ? nullptr
                   : reinterpret_cast<HeaderType*>(result->get());
    }

    bool magicValid() const { return nodecodeMagic == NodecodeMagic; }
    bool binaryCompatible() const {
        return Version::binaryCompatible(VersionSupported, nodecodeVersion);
    }

    static constexpr Magic NodecodeMagic = {'N', 'O', 'D', 'E', 'C', 'O',
                                            'D', 'E', ' ', 'F', 'I', 'L',
                                            'E', '>', '>', '>'};

    // Identifies nodecode_header compatible files
    Magic nodecodeMagic = NodecodeMagic;

    // User's magic string for file contents, e.g. matching the app headers
    Magic identifier;

    // Version of this top level header
    Version nodecodeVersion = VersionSupported;

    // Sorted contiguous array of sub-headers
    HeaderList headers;

    // Comparison functor to facilitate find() and sorting headers
    struct HeaderPtrComp {
        inline bool operator()(const offset_ptr<Header>& a,
                               const offset_ptr<Header>& b) {
            return a->identifier < b->identifier;
        }
        inline bool operator()(const offset_ptr<Header>& a,
                               const Magic& identifier) {
            return a->identifier < identifier;
        }
        inline bool operator()(const Magic& identifier,
                               const offset_ptr<Header>& b) {
            return identifier < b->identifier;
        }
    };
};

} // namespace nodecode