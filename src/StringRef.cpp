// This file is distributed under the MIT license.
// See the LICENSE file for details.

#include "Support/StringRef.h"

#include <algorithm>
#include <ostream>

using namespace support;

void StringRef::write(std::ostream& Stream) const
{
    Stream.write(data(), size());
}

size_t StringRef::find(char_type Ch, size_t From) const
{
    if (empty())
        return npos;

    From = Min(From, size());

    if (auto I = traits_type::find(data() + From, size() - From, Ch))
        return I - data();

    return npos;
}

size_t StringRef::find(StringRef Str, size_t From) const
{
    if (Str.size() == 1)
        return find(Str[0], From);

    if (empty() || Str.empty())
        return npos;

    if (From > size())
        return npos;

    auto I = std::search(begin() + From, end(), Str.begin(), Str.end());
    auto x = static_cast<size_t>(I - begin());

    return x + Str.size() <= size() ? x : npos;
}

size_t StringRef::find_first_of(StringRef Chars, size_t From) const
{
    if (Chars.size() == 1)
        return find(Chars[0], From);

    From = Min(From, size());

    for (auto I = From; I != size(); ++I)
        if (traits_type::find(Chars.data(), Chars.size(), data()[I]))
            return I;

    return npos;
}

size_t StringRef::find_first_not_of(StringRef Chars, size_t From) const
{
    From = Min(From, size());

    for (auto I = From; I != size(); ++I)
        if (!traits_type::find(Chars.data(), Chars.size(), data()[I]))
            return I;

    return npos;
}

size_t StringRef::find_last_of(StringRef Chars, size_t From) const
{
    From = Min(From, size());

    for (auto I = From; I != 0; --I)
        if (traits_type::find(Chars.data(), Chars.size(), data()[I - 1]))
            return I - 1;

    return npos;
}

size_t StringRef::find_last_not_of(StringRef Chars, size_t From) const
{
    From = Min(From, size());

    for (auto I = From; I != 0; --I)
        if (!traits_type::find(Chars.data(), Chars.size(), data()[I - 1]))
            return I - 1;

    return npos;
}

StringRef StringRef::trim_left(StringRef Chars) const
{
    return drop_front(find_first_not_of(Chars));
}

StringRef StringRef::trim_right(StringRef Chars) const
{
    auto I = find_last_not_of(Chars);
    return front(I == npos ? npos : I + 1); // return front(Max(I, I + 1));
}

StringRef StringRef::trim(StringRef Chars) const
{
    return trim_left(Chars).trim_right(Chars);
}
