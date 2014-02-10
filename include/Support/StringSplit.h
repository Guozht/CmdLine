// This file is distributed under the MIT license.
// See the LICENSE file for details.

// http://isocpp.org/files/papers/n3593.html

#pragma once

#include "Support/StringRef.h"

#include <cstddef>
#include <iterator>
#include <utility>

namespace support
{
namespace strings
{

//--------------------------------------------------------------------------------------------------
// Split_iterator
//

template <class RangeT>
class Split_iterator
{
    RangeT* R;

public:
    using iterator_category     = std::input_iterator_tag;
    using value_type            = StringRef;
    using reference             = StringRef const&;
    using pointer               = StringRef const*;
    using difference_type       = ptrdiff_t;

public:
    Split_iterator(RangeT* R_ = nullptr)
        : R(R_)
    {
    }

    reference operator *()
    {
        assert(R && "dereferencing end() iterator");
        return R->Tok;
    }

    pointer operator ->()
    {
        assert(R && "dereferencing end() iterator");
        return &R->Tok;
    }

    Split_iterator& operator ++()
    {
        assert(R && "incrementing end() iterator");

        if (R->next() == false)
            R = nullptr;

        return *this;
    }

    Split_iterator operator ++(int)
    {
        auto t = *this;
        operator ++();
        return t;
    }

    bool operator ==(Split_iterator const& RHS) const {
        return R == RHS.R;
    }

    bool operator !=(Split_iterator const& RHS) const {
        return R != RHS.R;
    }
};

//--------------------------------------------------------------------------------------------------
// Split_range
//

template <class StringT, class DelimiterT, class PredicateT>
class Split_range
{
    template <class> friend class Split_iterator;

    // The string to split
    StringT Str;
    // The delimiter
    DelimiterT Delim;
    // The predicate
    PredicateT Pred;
    // The current token
    StringRef Tok;
    // The start of the rest of the string
    size_t Pos;

public:
    using iterator = Split_iterator<Split_range>;
    using const_iterator = Split_iterator<Split_range>;

public:
    Split_range(StringT Str_, DelimiterT Delim_, PredicateT Pred_)
        : Str(std::move(Str_))
        , Delim(std::move(Delim_))
        , Pred(std::move(Pred_))
        , Tok(Str)
        , Pos(0)
    {
        next();
    }

    iterator begin() {
        return iterator(this);
    }

    iterator end() {
        return iterator();
    }

//#if !STRINGS_N3593
    template <class T> explicit operator T() { return T(begin(), end()); }
//#endif

//#if !STRINGS_N3593
    // Returns the current token and the rest of the string.
    auto operator ()() const -> std::pair<StringRef, StringRef> {
        return { Tok, StringRef(Str).substr(Pos) };
    }
//#endif

private:
    //
    // N3593:
    //
    // The result of a Delimiter's find() member function must be a std::string_view referring to
    // one of the following:
    //
    // -    A substring of find()'s argument text referring to the delimiter/separator that was
    //      found.
    // -    An empty std::string_view referring to find()'s argument's end iterator, (e.g.,
    //      std::string_view(input_text.end(), 0)). This indicates that the delimiter/separator was
    //      not found.
    //
    // [Footnote: An alternative to having a Delimiter's find() function return a std::string_view
    // is to instead have it return a std::pair<size_t, size_t> where the pair's first member is the
    // position of the found delimiter, and the second member is the length of the found delimiter.
    // In this case, Not Found could be prepresented as std::make_pair(std::string_view::npos, 0).
    // ---end footnote]
    //

    void increment(StringRef S, std::pair<size_t, size_t> Sep)
    {
        if (Sep.first == StringRef::npos)
        {
            // There is no further delimiter.
            // The current string is the last token.
            Tok = S;
            Pos = StringRef::npos;
        }
        else
        {
            // Delimiter found.
            Tok = StringRef(S.data(), Sep.first);
            Pos = Pos + Sep.first + Sep.second;
        }
    }

    void increment()
    {
        auto S = StringRef(Str.data() + Pos, Str.size() - Pos);
        increment(S, Delim(S));
    }

    bool next()
    {
        do {
            if (Pos == StringRef::npos)
            {
                // The current string is the last token.
                // Set the iterator to the past-the-end iterator.
                return false;
            }
            else
            {
                // Find the next token and adjust the iterator.
                increment();
            }
        } while (!Pred(Tok));

        return true;
    }
};

//--------------------------------------------------------------------------------------------------
// KeepEmpty
//

struct KeepEmpty
{
    bool operator ()(StringRef /*Tok*/) const {
        return true;
    }
};

//--------------------------------------------------------------------------------------------------
// SkipEmpty
//

struct SkipEmpty
{
    bool operator ()(StringRef Tok) const {
        return !Tok.empty();
    }
};

//--------------------------------------------------------------------------------------------------
// SkipSpace
//

struct SkipSpace
{
    bool operator ()(StringRef Tok) const {
        return !Tok.trim().empty();
    }
};

//--------------------------------------------------------------------------------------------------
// Trim
//

struct Trim
{
    bool operator ()(StringRef& Tok) const
    {
        Tok = Tok.trim();
        return !Tok.empty();
    }
};

//--------------------------------------------------------------------------------------------------
// AnyOfDelimiter
//

struct AnyOfDelimiter
{
    StringRef Chars;

    explicit AnyOfDelimiter(StringRef Chars_)
        : Chars(Chars_)
    {
    }

    auto operator ()(StringRef Str) const -> std::pair<size_t, size_t> {
        return { Str.find_first_of(Chars), 1 };
    }
};

//--------------------------------------------------------------------------------------------------
// LiteralDelimiter
//

struct LiteralDelimiter
{
    StringRef Needle;

    explicit LiteralDelimiter(StringRef Needle_)
        : Needle(Needle_)
    {
    }

    auto operator ()(StringRef Str) const -> std::pair<size_t, size_t>
    {
        if (Needle.empty())
        {
#if SUPPORT_STRINGSPLIT_EMPTY_LITERAL_IS_SPECIAL
            //
            // N3593:
            //
            // A delimiter of the empty string results in each character in the input string
            // becoming one element in the output collection. This is a special case. It is done to
            // match the behavior of splitting using the empty string in other programming languages
            // (e.g., perl).
            //
            return { Str.size() <= 1 ? StringRef::npos : 1, 0 };
#else
            //
            // Return the whole string as a token.
            // Makes LiteralDelimiter("") behave exactly as AnyOfDelimiter("").
            //
            return { StringRef::npos, 0 };
#endif
        }

        return { Str.find(Needle), Needle.size() };
    }
};

//--------------------------------------------------------------------------------------------------
// split
//

class Split_string
{
    //
    // N3593:
    //
    // Rvalue support
    //
    // As described so far, std::split() may not work correctly if splitting a std::string_view that
    // refers to a temporary string. In particular, the following will not work:
    //
    //      for (std::string_view s : std::split(GetTemporaryString(), "-")) {
    //          s now refers to a temporary string that is no longer valid.
    //      }
    //
    // To address this, std::split() will move ownership of rvalues into the Range object that is
    // returned from std::split().
    //

    static auto test(std::string&&)
        -> std::string;
    static auto test(std::string const&&)
        -> std::string;
    static auto test(StringRef)
        -> StringRef;
    static auto test(char const*)
        -> StringRef;

public:
    template <class T>
    using type = decltype(test(std::declval<T>()));
};

class Split_delimiter
{
    //
    // N3593:
    //
    // The default delimiter when not explicitly specified is std::literal_delimiter
    //

    template <class T>
    static auto test(T)
        -> T;
    static auto test(std::string)
        -> LiteralDelimiter;
    static auto test(StringRef)
        -> LiteralDelimiter;
    static auto test(char const*)
        -> LiteralDelimiter;

public:
    template <class T>
    using type = decltype(test(std::declval<T>()));
};

//
// N3593:
//
// The std::split() algorithm takes a std::string_view and a Delimiter as arguments, and it returns
// a Range of std::string_view objects as output. The std::string_view objects in the returned Range
// will refer to substrings of the input text. The Delimiter object defines the boundaries between
// the returned substrings.
//
template <class S, class D, class P = KeepEmpty>
auto split(S&& Str, D Delim, P Pred = P())
    -> Split_range<Split_string::type<S>, Split_delimiter::type<D>, P>
{
    return { std::forward<S>(Str), Split_delimiter::type<D>(std::move(Delim)), std::move(Pred) };
}

} // namespace strings
} // namespace support
