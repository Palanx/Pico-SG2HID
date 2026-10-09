#include "emu/sg_model.h"

#include <charconv>
#include <optional>

namespace ps2 {

namespace {

constexpr std::size_t   kWireIdIndex       = 1;
constexpr std::size_t   kWireReadyIndex    = 2;
constexpr std::size_t   kWirePayloadStart  = 3;
constexpr std::size_t   kWireArgumentIndex = 3;  // a 0x43 / 0x44 command's argument
constexpr std::size_t   kMaxAckByte        = 7;  // the last wire byte an analog frame ACKs
constexpr std::uint32_t kMaxLateUs         = 10000;
constexpr int           kHexBase           = 16;
constexpr int           kDecimalBase       = 10;
constexpr std::size_t   kHexDigits         = 2;

// Splits off the next space-separated word; `rest` keeps what follows it.
[[nodiscard]] std::string_view next_word( std::string_view& rest ) {
    const std::size_t start = rest.find_first_not_of( ' ' );
    if ( start == std::string_view::npos ) {
        rest = {};
        return {};
    }
    rest                        = rest.substr( start );
    const std::size_t      end  = rest.find( ' ' );
    const std::string_view word = rest.substr( 0, end );
    rest = end == std::string_view::npos ? std::string_view{} : rest.substr( end );
    return word;
}

// The whole word as an unsigned number in `base`, or nothing.
[[nodiscard]] std::optional<std::uint32_t> parse_number( std::string_view word, int base ) {
    std::uint32_t value = 0;
    const auto [ end, error ] =
        std::from_chars( word.data(), word.data() + word.size(), value, base );
    if ( word.empty() || error != std::errc{} || end != word.data() + word.size() ) {
        return std::nullopt;
    }
    return value;
}

// Exactly two hex digits, either case.
[[nodiscard]] std::optional<std::uint8_t> parse_hex_byte( std::string_view word ) {
    if ( word.size() != kHexDigits ) {
        return std::nullopt;
    }
    const auto value = parse_number( word, kHexBase );
    if ( !value ) {
        return std::nullopt;
    }
    return static_cast<std::uint8_t>( *value );
}

// `mode digital|analog`: true for analog.
[[nodiscard]] std::expected<bool, std::string_view> parse_mode( std::string_view rest ) {
    const std::string_view mode = next_word( rest );
    if ( !next_word( rest ).empty() || ( mode != "digital" && mode != "analog" ) ) {
        return std::unexpected( "usage: mode digital|analog" );
    }
    return mode == "analog";
}

// `payload h0 h1 h2 h3 h4 h5`: exactly six bytes.
[[nodiscard]] std::expected<Payload, std::string_view> parse_payload( std::string_view rest ) {
    constexpr std::string_view kUsage = "usage: payload h0 h1 h2 h3 h4 h5 (two hex digits each)";
    Payload                    payload{};
    for ( auto& byte : payload ) {
        const auto parsed = parse_hex_byte( next_word( rest ) );
        if ( !parsed ) {
            return std::unexpected( kUsage );
        }
        byte = *parsed;
    }
    if ( !next_word( rest ).empty() ) {
        return std::unexpected( kUsage );
    }
    return payload;
}

// `fault none|ack <n>|late <us>|id <hh>|decline`.
[[nodiscard]] std::expected<Fault, std::string_view> parse_fault( std::string_view rest ) {
    constexpr std::string_view kUsage = "usage: fault none|ack <n>|late <us>|id <hh>|decline";
    const std::string_view     kind   = next_word( rest );
    const std::string_view     arg    = next_word( rest );
    if ( !next_word( rest ).empty() ) {
        return std::unexpected( kUsage );
    }
    if ( kind == "none" && arg.empty() ) {
        return Fault{ .kind = FaultKind::None, .value = 0 };
    }
    if ( kind == "decline" && arg.empty() ) {
        return Fault{ .kind = FaultKind::Decline, .value = 0 };
    }
    if ( kind == "ack" ) {
        const auto n = parse_number( arg, kDecimalBase );
        if ( !n || *n > kMaxAckByte ) {
            return std::unexpected( "usage: fault ack <n>, n from 0 to 7" );
        }
        return Fault{ .kind = FaultKind::Ack, .value = *n };
    }
    if ( kind == "late" ) {
        const auto us = parse_number( arg, kDecimalBase );
        if ( !us || *us == 0 || *us > kMaxLateUs ) {
            return std::unexpected( "usage: fault late <us>, us from 1 to 10000" );
        }
        return Fault{ .kind = FaultKind::Late, .value = *us };
    }
    if ( kind == "id" ) {
        const auto id = parse_hex_byte( arg );
        if ( !id ) {
            return std::unexpected( "usage: fault id <hh>, two hex digits" );
        }
        return Fault{ .kind = FaultKind::Id, .value = *id };
    }
    return std::unexpected( kUsage );
}

}  // namespace

void SgModel::apply_command() {
    const bool is_declined = m_fault.kind == FaultKind::Decline;
    if ( m_command == kCmdConfig && m_argument == kConfigEnter ) {
        m_is_config = true;
    } else if ( m_command == kCmdConfig && m_argument == kConfigLeave ) {
        m_is_config = false;
    } else if ( m_command == kCmdSetMode && m_argument == kModeAnalog && !is_declined ) {
        m_is_analog = true;
    } else if ( m_command == kCmdSetMode && m_argument == kModeDigital ) {
        m_is_analog = false;
    }
}

void SgModel::reset() {
    if ( m_has_argument ) {
        apply_command();
    }
    m_has_argument = false;
    m_index        = 0;
    m_is_broken    = false;
}

ControllerId SgModel::mode_id() const {
    if ( m_is_config ) {
        return ControllerId::Config;
    }
    return m_is_analog ? ControllerId::Analog : ControllerId::Digital;
}

std::size_t SgModel::last_index() const {
    return frame_len( mode_id() );
}

std::uint8_t SgModel::byte_at( std::size_t index ) const {
    if ( index == kWireIdIndex ) {
        if ( m_fault.kind == FaultKind::Id ) {
            return static_cast<std::uint8_t>( m_fault.value );
        }
        return static_cast<std::uint8_t>( mode_id() );
    }
    if ( index == kWireReadyIndex ) {
        return kReadyByte;
    }
    if ( index >= kWirePayloadStart && index <= last_index() ) {
        return m_is_config ? kConfigReplyByte : m_payload[ index - kWirePayloadStart ];
    }
    return kIdleByte;
}

ByteAnswer SgModel::step( std::uint8_t received ) {
    const std::size_t index               = m_index++;
    const bool        is_command_accepted = received == kCmdPoll || received == kCmdConfig ||
                                            ( m_is_config && received == kCmdSetMode );
    if ( ( index == 0 && received != kFrameStart ) ||
         ( index == kWireIdIndex && !is_command_accepted ) ) {
        m_is_broken = true;
    }
    if ( !m_is_broken && index == kWireIdIndex ) {
        m_command = received;
    }
    if ( !m_is_broken && index == kWireArgumentIndex ) {
        m_argument     = received;
        m_has_argument = true;
    }
    bool should_ack = !m_is_broken && index < last_index();
    if ( m_fault.kind == FaultKind::Ack && index == m_fault.value ) {
        should_ack = false;
    }
    // A broken frame is not this controller's: DATA stays released for the rest of it.
    return { .next         = m_is_broken ? kIdleByte : byte_at( index + 1 ),
             .should_ack   = should_ack,
             .ack_delay_us = m_fault.kind == FaultKind::Late ? m_fault.value : kAckDelayUs };
}

std::expected<void, std::string_view> SgModel::apply( std::string_view line ) {
    if ( !line.empty() && line.back() == '\r' ) {
        line.remove_suffix( 1 );
    }
    if ( line.size() > kMaxLineLen ) {
        return std::unexpected( "line too long" );
    }
    std::string_view       rest = line;
    const std::string_view verb = next_word( rest );
    // Each parse returns the new value or a reason; state changes only on success.
    if ( verb == "mode" ) {
        return parse_mode( rest ).transform( [ this ]( bool is_analog ) {
            m_is_analog = is_analog;
            m_is_config = false;
        } );
    }
    if ( verb == "payload" ) {
        return parse_payload( rest ).transform( [ this ]( const Payload& p ) {
            m_payload = p;
        } );
    }
    if ( verb == "fault" ) {
        return parse_fault( rest ).transform( [ this ]( const Fault& f ) {
            m_fault = f;
        } );
    }
    return std::unexpected( "unknown command; try mode, payload or fault" );
}

}  // namespace ps2
