// This file is distributed under the MIT license.
// See the LICENSE file for details.

#pragma once

#include <cassert>
#include <iosfwd>
#include <string>

namespace support
{

class StringRef
{
public:
    using char_type = char;

    using const_reference   = char_type const&;
    using const_pointer     = char_type const*;
    using const_iterator    = char_type const*;

    using traits_type = std::char_traits<char_type>;

private:
    // The string data - an external buffer
    const_pointer Data;
    // The length of the string
    size_t Length;

    // Workaround: traits_type::compare is undefined for null pointers even if MaxCount is 0.
    static int Compare(const_pointer LHS, const_pointer RHS, size_t MaxCount) {
        return MaxCount == 0 ? 0 : traits_type::compare(LHS, RHS, MaxCount);
    }

    static size_t Min(size_t x, size_t y) { return x < y ? x : y; }
    static size_t Max(size_t x, size_t y) { return x > y ? x : y; }

public:
    static size_t const npos;

public:
    // Construct an empty StringRef.
    StringRef()
        : Data(nullptr)
        , Length(0)
    {
    }

    // Construct a StringRef from a pointer and a length.
    StringRef(const_pointer Data, size_t Length)
        : Data(Data)
        , Length(Length)
    {
        assert((Data || Length == 0) && "constructing from a nullptr and a non-zero length");
    }

    // Construct a StringRef from a C-string.
    StringRef(const_pointer Str)
        : Data(Str)
        , Length(Str ? traits_type::length(Str) : 0)
    {
    }

    // Construct from two iterators
    StringRef(const_iterator Begin, const_iterator End)
        : Data(Begin)
        , Length(End - Begin)
    {
        assert((Begin ? Begin <= End : !End) && "invalid iterators");
    }

    // Construct a StringRef from a std::string.
    StringRef(std::string const& Str)
        : Data(Str.data())
        , Length(Str.size())
    {
    }

    // Returns a pointer to the start of the string.
    // Note: The string may not be null-terminated.
    const_pointer data() const {
        return Data;
    }

    // Returns the length of the string.
    size_t size() const {
        return Length;
    }

    // Returns whether this string is null or empty.
    bool empty() const {
        return Length == 0;
    }

    // Returns whether this string is null.
    bool null() const {
        return Data == nullptr;
    }

    // Returns an iterator to the first element of the string.
    const_iterator begin() const {
        return Data;
    }

    // Returns an iterator to one element past the last element of the string.
    const_iterator end() const {
        return Data + Length;
    }

    // Array access.
    const_reference operator [](size_t Index) const
    {
        assert(Index < size() && "index out of range");
        return Data[Index];
    }

    // Returns the first character of the string.
    const_reference front() const
    {
        assert(!empty() && "index out of range");
        return data()[0];
    }

    // Returns the last character of the string.
    const_reference back() const
    {
        assert(!empty() && "index out of range");
        return data()[size() - 1];
    }

    // Returns the first N characters of the string.
    StringRef front(size_t N) const
    {
        N = Min(N, size());
        return { data(), N };
    }

    // Removes the first N characters from the string.
    StringRef drop_front(size_t N) const
    {
        N = Min(N, size());
        return { data() + N, size() - N };
    }

    // Returns the last N characters of the string.
    StringRef back(size_t N) const
    {
        N = Min(N, size());
        return { data() + (size() - N), N };
    }

    // Removes the last N characters from the string.
    StringRef drop_back(size_t N) const
    {
        N = Min(N, size());
        return { data(), size() - N };
    }

    // Returns the substring [First, Last).
    StringRef slice(size_t First, size_t Last = npos) const {
        return front(Last).drop_front(First);
    }

    // Returns the sub-string [First, First + Count).
    StringRef substr(size_t First, size_t Count = npos) const {
        return drop_front(First).front(Count);
    }

    // Removes substr(Pos, N) from the current string S.
    // Returns a pair (A,B) such that S == A + substr(Pos, N) + B.
    std::pair<StringRef, StringRef> split(size_t Pos, size_t N = 0) const {
        return { front(Pos), drop_front(Pos).drop_front(N) };
    }

    // Returns whether this string is equal to another.
    bool equals(StringRef RHS) const
    {
        return size() == RHS.size()
               && 0 == Compare(data(), RHS.data(), RHS.size());
    }

    // Lexicographically compare this string with another.
    bool less(StringRef RHS) const
    {
        int c = Compare(data(), RHS.data(), Min(size(), RHS.size()));
        return c < 0 || (c == 0 && size() < RHS.size());
    }

    // Returns whether the string starts with Prefix
    bool starts_with(StringRef Prefix) const
    {
        return size() >= Prefix.size()
               && 0 == Compare(data(), Prefix.data(), Prefix.size());
    }

    // Returns whether the string ends with Suffix
    bool ends_with(StringRef Suffix) const
    {
        return size() >= Suffix.size()
               && 0 == Compare(data() + (size() - Suffix.size()), Suffix.data(), Suffix.size());
    }

    // Constructs a std::string from this StringRef.
    std::string str() const {
        return empty() ? std::string() : std::string(data(), size());
    }

    // Explicitly convert to a std::string
    explicit operator std::string() const {
        return str();
    }

    // Write this string into the given stream
    void write(std::ostream& Stream) const;

    // Search for the first character Ch in the sub-string [From, Length)
    size_t find(char_type Ch, size_t From = 0) const;

    // Search for the first substring Str in the sub-string [From, Length)
    size_t find(StringRef Str, size_t From = 0) const;

    // Search for the first character in the sub-string [From, Length)
    // which matches any of the characters in Chars.
    size_t find_first_of(StringRef Chars, size_t From = 0) const;

    // Search for the first character in the sub-string [From, Length)
    // which does not match any of the characters in Chars.
    size_t find_first_not_of(StringRef Chars, size_t From = 0) const;

    // Search for the last character in the sub-string [From, Length)
    // which matches any of the characters in Chars.
    size_t find_last_of(StringRef Chars, size_t From = npos) const;

    // Search for the last character in the sub-string [From, Length)
    // which does not match any of the characters in Chars.
    size_t find_last_not_of(StringRef Chars, size_t From = npos) const;

    // Return string with consecutive characters in Chars starting from the left removed.
    StringRef trim_left(StringRef Chars = " \t\n\v\f\r") const;

    // Return string with consecutive characters in Chars starting from the right removed.
    StringRef trim_right(StringRef Chars = " \t\n\v\f\r") const;

    // Return string with consecutive characters in Chars starting from the left and right removed.
    StringRef trim(StringRef Chars = " \t\n\v\f\r") const;
};

inline bool operator ==(StringRef LHS, StringRef RHS) {
    return LHS.equals(RHS);
}

inline bool operator !=(StringRef LHS, StringRef RHS) {
    return !(LHS == RHS);
}

inline bool operator <(StringRef LHS, StringRef RHS) {
    return LHS.less(RHS);
}

inline bool operator <=(StringRef LHS, StringRef RHS) {
    return !(RHS < LHS);
}

inline bool operator >(StringRef LHS, StringRef RHS) {
    return RHS < LHS;
}

inline bool operator >=(StringRef LHS, StringRef RHS) {
    return !(LHS < RHS);
}

inline std::ostream& operator <<(std::ostream& Stream, StringRef Str)
{
    Str.write(Stream);
    return Stream;
}

inline std::string& operator +=(std::string& LHS, StringRef RHS) {
    return LHS.append(RHS.data(), RHS.size());
}

inline std::string operator +(StringRef LHS, std::string RHS)
{
    RHS.insert(0, LHS.data(), LHS.size());
    return std::move(RHS);
}

inline std::string operator +(std::string LHS, StringRef RHS)
{
    LHS.append(RHS.data(), RHS.size());
    return std::move(LHS);
}

// Modified Bernstein hash
inline size_t hashValue(StringRef Str, size_t H = 5381)
{
    for (size_t I = 0, E = Str.size(); I != E; ++I)
        H = 33 * H ^ static_cast<unsigned char>(Str[I]);

    return H;
}

} // namespace support

namespace std
{
    template<>
    struct hash<::support::StringRef>
    {
        size_t operator ()(::support::StringRef Str) const {
            return hashValue(Str);
        }
    };
}
