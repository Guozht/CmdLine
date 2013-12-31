// This file is distributed under the MIT license.
// See the LICENSE file for details.

#include "Support/StringRef.h"

#include <algorithm>
#include <ostream>

using namespace support;

size_t const StringRef::npos = static_cast<size_t>(-1);

size_t StringRef::find(char_type Ch, size_t From) const
{
    if (From >= size())
        return npos;

    if (auto I = traits_type::find(data() + From, size() - From, Ch))
        return I - data();

    return npos;
}

size_t StringRef::find(StringRef Str, size_t From) const
{
    if (Str.size() == 1)
        return find(Str[0], From);

    if (From > size())
        return npos;

    if (Str.empty())
        return From;

    auto I = std::search(begin() + From, end(), Str.begin(), Str.end(), traits_type::eq);
    auto x = static_cast<size_t>(I - begin());

    return x + Str.size() <= size() ? x : npos;
}

size_t StringRef::find_first_of(StringRef Chars, size_t From) const
{
    if (From >= size() || Chars.empty())
        return npos;

    for (auto I = From; I != size(); ++I)
        if (traits_type::find(Chars.data(), Chars.size(), data()[I]))
            return I;

    return npos;
}

size_t StringRef::find_first_not_of(StringRef Chars, size_t From) const
{
    if (From >= size())
        return npos;

    for (auto I = From; I != size(); ++I)
        if (!traits_type::find(Chars.data(), Chars.size(), data()[I]))
            return I;

    return npos;
}

size_t StringRef::find_last_of(StringRef Chars, size_t From) const
{
    if (Chars.empty())
        return npos;

    if (From < size())
        From++;
    else
        From = size();

    for (auto I = From; I != 0; --I)
        if (traits_type::find(Chars.data(), Chars.size(), data()[I - 1]))
            return I - 1;

    return npos;
}

size_t StringRef::find_last_not_of(StringRef Chars, size_t From) const
{
    if (From < size())
        From++;
    else
        From = size();

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

std::ostream& support::operator<<(std::ostream& Stream, StringRef Str)
{
    //
    // 27.7.3.6.1 Common requirements [ostream.formatted.reqmts]
    //
    // 1    Each formatted output function begins execution by constructing an
    //      object of class sentry. If this object returns true when converted
    //      to a value of type bool, the function endeavors to generate the
    //      requested output. If the generation fails, then the formatted output
    //      function does setstate(ios_base::failbit), which might throw an
    //      exception. If an exception is thrown during output, then ios::badbit
    //      is turned on in *this's error state. If (exceptions()&badbit) != 0
    //      then the exception is rethrown. Whether or not an exception is
    //      thrown, the sentry object is destroyed before leaving the formatted
    //      output function. If no exception is thrown, the result of the
    //      formatted output function is *this.
    // 2    [...]
    // 3    If a formatted output function of a stream os determines padding, it
    //      does so as follows. Given a charT character sequence seq where charT
    //      is the character type of the stream, if the length of seq is less
    //      than os.width(), then enough copies of os.fill() are added to this
    //      sequence as necessary to pad to a width of os.width() characters.
    //      If (os.flags() & ios_base::adjustfield) == ios_base::left is true,
    //      the fill characters are placed after the character sequence;
    //      otherwise, they are placed before the character sequence.
    //
    // 27.7.3.6.4 Character inserter function templates [ostream.inserters.character]
    //
    // 4    Effects: Behaves like a formatted inserter (as described in 27.7.3.6.1)
    //      of out. [...] Creates a character sequence seq of n characters
    //      starting at s [...]. Determines padding for seq as described in
    //      27.7.3.6.1. Inserts seq into out. Calls width(0).
    // 5    Returns: out.
    //

    using traits = StringRef::traits_type;

    std::ostream::sentry ok(Stream);
    if (ok)
    {
        bool failed = false;
        try
        {
            std::streamsize size = Str.size();
            std::streamsize fill = Stream.width() <= 0 || Stream.width() <= size
                    ? 0
                    : Stream.width() - size;

            bool left = (Stream.flags() & std::ios_base::adjustfield) == std::ios_base::left;

            if (left)
            {
                failed = Stream.rdbuf()->sputn(Str.data(), size) != size;
            }

            for ( ; !failed && fill > 0; --fill)
            {
                failed = Stream.rdbuf()->sputc(Stream.fill()) == traits::eof();
            }

            if (!failed && !left)
            {
                failed = Stream.rdbuf()->sputn(Str.data(), size) != size;
            }
        }
        catch (...)
        {
            failed = true;
        }

        if (failed)
            Stream.setstate(std::ios_base::badbit | std::ios_base::failbit);
    }

    Stream.width(0);

    return Stream;
}
