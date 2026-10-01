#include "core/bus_trace.h"

#include <charconv>
#include <string_view>

namespace ps2 {

namespace {

constexpr std::string_view kVersion   = "T1";
constexpr std::string_view kLength    = " n=";
constexpr std::string_view kCompleted = " k=";
constexpr std::string_view kOut       = " out=";
constexpr std::string_view kIn        = " in=";
constexpr std::string_view kElapsed   = " us=";
constexpr std::string_view kAbsentIn  = "--";
constexpr std::string_view kAbsentUs  = "-";
constexpr char             kItemSep   = ',';

constexpr std::string_view kHexDigits  = "0123456789ABCDEF";
constexpr unsigned         kNibbleBits = 4;
constexpr unsigned         kNibbleMask = 0x0F;

// Appends to a caller's buffer and remembers whether anything fell off its end.
class LineWriter {
public:
    explicit LineWriter( std::span<char> line ) : m_line( line ) {}

    void put_char( char c ) {
        if ( m_size >= m_line.size() ) {
            m_is_overflow = true;
            return;
        }
        m_line[ m_size++ ] = c;
    }

    void put_text( std::string_view text ) {
        for ( const char c : text ) {
            put_char( c );
        }
    }

    void put_dec( std::size_t value ) {
        char*      first  = m_line.data() + m_size;
        char*      last   = m_line.data() + m_line.size();
        const auto result = std::to_chars( first, last, value );
        if ( result.ec != std::errc{} ) {
            m_is_overflow = true;
            return;
        }
        m_size = static_cast<std::size_t>( result.ptr - m_line.data() );
    }

    void put_hex( std::uint8_t value ) {
        put_char( kHexDigits[ ( value >> kNibbleBits ) & kNibbleMask ] );
        put_char( kHexDigits[ value & kNibbleMask ] );
    }

    // The separator before item `i` of a list: nothing before the first.
    void put_sep( std::size_t i ) {
        if ( i > 0 ) {
            put_char( kItemSep );
        }
    }

    [[nodiscard]] std::size_t length() const {
        return m_is_overflow ? 0 : m_size;
    }

private:
    std::span<char> m_line;
    std::size_t     m_size        = 0;
    bool            m_is_overflow = false;
};

}  // namespace

std::size_t
format_trace_line( std::span<const WireByte> frame, std::size_t completed, std::span<char> line ) {
    if ( completed > frame.size() ) {
        return 0;
    }
    LineWriter writer( line );
    writer.put_text( kVersion );
    writer.put_text( kLength );
    writer.put_dec( frame.size() );
    writer.put_text( kCompleted );
    writer.put_dec( completed );
    writer.put_text( kOut );
    for ( std::size_t i = 0; i < frame.size(); ++i ) {
        writer.put_sep( i );
        writer.put_hex( frame[ i ].out );
    }
    writer.put_text( kIn );
    for ( std::size_t i = 0; i < frame.size(); ++i ) {
        writer.put_sep( i );
        if ( i < completed ) {
            writer.put_hex( frame[ i ].in );
        } else {
            writer.put_text( kAbsentIn );
        }
    }
    writer.put_text( kElapsed );
    for ( std::size_t i = 0; i < frame.size(); ++i ) {
        writer.put_sep( i );
        // The byte that failed was attempted too: its time is how long the master waited.
        const bool is_attempted = i <= completed;
        if ( is_attempted ) {
            writer.put_dec( frame[ i ].elapsed_us );
        } else {
            writer.put_text( kAbsentUs );
        }
    }
    return writer.length();
}

}  // namespace ps2
