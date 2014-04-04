// This file is distributed under the MIT license.
// See the LICENSE file for details.

#include "Support/CmdLine.h"
#include "Support/CmdLineToArgv.h"
#include "Support/StringSplit.h"

#include <algorithm>
#include <fstream>

using namespace support;
using namespace support::cl;

//--------------------------------------------------------------------------------------------------
// CmdLine
//

CmdLine::CmdLine()
{
}

CmdLine::CmdLine(StringVector args)
    : args_(std::move(args))
{
}

CmdLine::~CmdLine()
{
}

void CmdLine::add(OptionBase& opt)
{
    if (opt.formatting_ == Positional)
    {
        if (opt.name().empty())
            throw std::runtime_error("positional options need a valid name");

        positionals_.push_back(&opt);
        return;
    }

    auto insert = [&](StringRef s)
    {
        // Save the length of the longest prefix option
        if (opt.isPrefix())
        {
            if (maxPrefixLength_ < s.size())
                maxPrefixLength_ = s.size();
        }

        // Insert the option into the map
        if (!options_.insert({ s, &opt }).second)
            throw std::runtime_error("option '" + s + "' already exists");
    };

    if (opt.name().empty())
    {
        auto values = opt.getAllowedValues();

        if (values.empty())
            throw std::runtime_error("option name is empty and option does not provide allowed values");

        for (auto s : values)
            insert(s);
    }
    else
    {
        for (auto s : strings::split(opt.name(), "|"))
            insert(s);
    }
}

void CmdLine::add(OptionGroup& group)
{
    if (!groups_.insert({ group.name_, &group }).second)
        throw std::runtime_error("option group '" + group.name_ + "' already exists");
}

void CmdLine::parse()
{
    auto dashdash = false; // "--" seen?

    // The current positional argument - if any
    currentPositional_ = positionals_.begin();

    // Handle all arguments
    for (index_ = 0; index_ < args_.size(); ++index_)
    {
        handleArg(dashdash);
    }

    check();
}

void CmdLine::parse(StringVector argv)
{
    // Save the list of arguments
    args_ = std::move(argv);

    // Then parse
    parse();
}

void CmdLine::expandAndParse()
{
    // Recursively expand response files
    expandResponseFiles();

    // Then parse the command-line arguments
    parse();
}

void CmdLine::expandAndParse(StringVector argv)
{
    // Save the list of the arguments
    args_ = std::move(argv);

    // Then parse
    expandAndParse();
}

size_t CmdLine::index() const
{
    return index_;
}

StringRef CmdLine::bump()
{
    if (index_ + 1 >= args_.size())
        return {};

    return args_[++index_];
}

OptionBase* CmdLine::findOption(StringRef name) const
{
    auto I = options_.find(name);
    return I == options_.end() ? 0 : I->second;
}

CmdLine::OptionVector CmdLine::getUniqueOptions() const
{
    OptionVector opts;

    // Get the list of all (visible) options
    for (auto const& I : options_)
        opts.emplace_back(I.second);

    // Sort by name
    auto compare = [](OptionBase const* LHS, OptionBase const* RHS) {
        return LHS->name() < RHS->name();
    };

    std::stable_sort(opts.begin(), opts.end(), compare);

    // Remove duplicates
    opts.erase(std::unique(opts.begin(), opts.end()), opts.end());

    return opts;
}

void CmdLine::expandResponseFile(size_t i)
{
    std::ifstream file;

    file.exceptions(std::ios::failbit);
    file.open(args_[i].substr(1));

    // Erase the i-th argument (@file)
    args_.erase(args_.begin() + i);

    // Parse the file while inserting new command line arguments before the erased argument
    auto I = std::istreambuf_iterator<char>(file.rdbuf());
    auto E = std::istreambuf_iterator<char>();

    tokenizeCommandLineUnix(I, E, std::inserter(args_, args_.begin() + i));
}

// Recursively expand response files.
void CmdLine::expandResponseFiles()
{
    // Response file counter to prevent infinite recursion...
    size_t responseFilesLeft = 100;

    for (size_t i = 0; i < args_.size(); /**/)
    {
        if (args_[i][0] != '@')
        {
            i++;
            continue;
        }

        expandResponseFile(i);

        if (--responseFilesLeft == 0)
            throw std::runtime_error("too many response files encountered");
    }
}

// Process a single command line argument.
void CmdLine::handleArg(bool& dashdash)
{
    StringRef arg = args_[index_];

    // Stop parsing if "--" has been found
    if (arg == "--" && !dashdash)
    {
        dashdash = true;
        return;
    }

    // This argument is considered to be positional if it doesn't start with '-', if it is "-"
    // itself, or if we have seen "--" already.
    if (arg[0] != '-' || arg == "-" || dashdash)
    {
        handlePositional(arg);

        // If the current positional argument has the ConsumeAfter flag set, parse
        // all following command-line arguments as positional options.
        if ((*currentPositional_)->miscFlags_ & ConsumeAfter)
            dashdash = true;

        return;
    }

    // Starts with a dash, must be an argument.

    // Drop the first leading dash
    auto name = arg.substr(1);

    // If the name arguments starts with a single hyphen, this is a short option and
    // might actually be an option group.
    bool short_option = name[0] != '-';

    if (!short_option)
        name = name.substr(1);

    // Try to process this argument as a standard option.
    if (handleOption(name))
        return;

    // If it's not a standard option and there is no equals sign,
    // check for a prefix option.
    if (handlePrefix(name))
        return;

    // If it's not standard or prefix option, check for an option group
    if (short_option && handleGroup(name))
        return;

    // Otherwise it's an unknown option.
    throw std::runtime_error("unknown option '" + arg + "'");
}

void CmdLine::handlePositional(StringRef curr)
{
    if (currentPositional_ == positionals_.end())
        throw std::runtime_error("unhandled positional argument");

    auto opt = *currentPositional_;

    // If the current positional argument does not allow any further
    // occurrence try the next.
    if (!opt->isOccurrenceAllowed())
    {
        ++currentPositional_;
        return handlePositional(curr);
    }

    // The value of a positional option is the argument itself.
    addOccurrence(opt, curr, curr);
}

// If 'arg' is the name of an option, process the option immediately.
// Otherwise looks for an equal sign and try again.
bool CmdLine::handleOption(StringRef curr)
{
    if (auto opt = findOption(curr)) // Standard option?
    {
        addOccurrence(opt, curr);
        return true;
    }

    // Get the index of the first equals sign
    auto I = curr.find('=');

    if (I == StringRef::npos)
        return false;

    // Get the option name
    auto name = curr.substr(0, I);

    if (auto opt = findOption(name))
    {
        // Include the equals sign in the value if this option is a prefix option.
        // Discard otherwise.
        if (!opt->isPrefix())
            I++;

        addOccurrence(opt, name, curr.substr(I));
        return true;
    }

    return false;
}

// Handles prefix options which have an argument specified without an equals sign.
bool CmdLine::handlePrefix(StringRef curr)
{
    assert(!curr.empty());

    for (auto n = std::min(maxPrefixLength_, curr.size()); n != 0; --n)
    {
        auto name = curr.substr(0, n); // curr.front(n);
        auto opt = findOption(name);

        if (opt && opt->isPrefix())
        {
            addOccurrence(opt, name, curr.substr(n));
            return true;
        }
    }

    return false;
}

// Process an option group.
bool CmdLine::handleGroup(StringRef curr)
{
    OptionVector group;

    group.reserve(curr.size());

    // First check if the name consists of single letter options
    for (size_t n = 0; n != curr.size(); ++n)
    {
        auto opt = findOption(curr.substr(n, 1));

        if (opt == 0 || opt->formatting_ != Grouping)
            return false;
        else
            group.push_back(opt);
    }

    assert(!group.empty());

    // The first N - 1 options must not require an argument.
    // The last option might allow an argument.
    for (size_t n = 0; n != group.size() - 1; ++n)
    {
        if (group[n]->numArgs_ == ArgRequired)
        {
            throw std::runtime_error("option '" + group[n]->displayName()
                + "' requires an argument (must be last in '" + curr + "')");
        }
    }

    // Then handle all the options
    for (size_t n = 0; n != group.size(); ++n)
        addOccurrence(group[n], curr.substr(n, 1));

    return true;
}

void CmdLine::addOccurrence(OptionBase* opt, StringRef name)
{
    StringRef arg;

    if (opt->formatting_ != Positional)
    {
        // If the option requires an argument, "steal" the next argument from the
        // command line, so that "-o file" is possible instead of "-o=file"
        if (opt->numArgs_ == ArgRequired)
        {
            if (opt->formatting_ == Prefix || index_ + 1 >= args_.size())
                throw std::runtime_error("option '" + opt->displayName() + "' requires an argument");

            arg = args_[++index_];
        }
    }

    parse(opt, name, arg);
}

void CmdLine::addOccurrence(OptionBase* opt, StringRef name, StringRef arg)
{
    if (opt->formatting_ != Positional)
    {
        // An argument was specified, check if this is allowed.
        if (opt->numArgs_ == ArgDisallowed)
            throw std::runtime_error("option '" + opt->displayName() + "' doesn't allow an argument");
    }

    parse(opt, name, arg);
}

void CmdLine::parse(OptionBase* opt, StringRef name, StringRef arg)
{
    // Check if an occurrence is allowed
    if (!opt->isOccurrenceAllowed())
        throw std::runtime_error("option '" + opt->displayName() + "' already specified");

    auto add = [&](StringRef name, StringRef arg)
    {
        opt->parse(name, arg);
        opt->count_++;
    };

    // If this option takes a comma-separated list of arguments, split the string
    // and add all arguments separately.
    if (opt->miscFlags_ & CommaSeparated)
    {
        for (auto v : strings::split(arg, ","))
            add(name, v);
    }
    // Otherwise, just parse the option.
    else
    {
        add(name, arg);
    }
}

void CmdLine::check(OptionBase const* opt)
{
    if (opt->isOccurrenceRequired())
        throw std::runtime_error("option '" + opt->displayName() + "' missing");
}

void CmdLine::check()
{
    for (auto& I : getUniqueOptions())
        check(I);

    for (auto& I : positionals_)
        check(I);

    for (auto& I : groups_)
        I.second->check();
}

//--------------------------------------------------------------------------------------------------
// OptionGroup
//

OptionGroup::OptionGroup(std::string name, Type type)
    : name_(std::move(name))
    , type_(type)
{
}

void OptionGroup::add(OptionBase& opt)
{
    // FIXME:
    // Check for duplicates!
    options_.push_back(&opt);
}

void OptionGroup::check() const
{
    if (type_ == OptionGroup::Default)
        return;

    auto pred = [](OptionBase const* x) {
        return x->count() > 0;
    };

    // Count the number of options in this group which have been specified on the command-line
    size_t N = std::count_if(options_.begin(), options_.end(), pred);

    switch (type_)
    {
    case Default: // prevent warning
        break;
    case Zero:
        if (N != 0)
            throw std::runtime_error("no options in group '" + name_ + "' may be specified");
        break;
    case ZeroOrOne:
        if (N != 0 && N != 1)
            throw std::runtime_error("at most one option in group '" + name_ + "' may be specified");
        break;
    case One:
        if (N != 1)
            throw std::runtime_error("exactly one option in group '" + name_ + "' must be specified");
        break;
    case OneOrMore:
        if (N < 1)
            throw std::runtime_error("at least one option in group '" + name_ + "' must be specified");
        break;
    case All:
        if (N != options_.size())
            throw std::runtime_error("all options in group '" + name_ + "' must be specified");
        break;
    case ZeroOrAll:
        if (N != 0 && N != options_.size())
            throw std::runtime_error("none or all options in group '" + name_ + "' must be specified");
        break;
    }
}

//--------------------------------------------------------------------------------------------------
// OptionBase
//

OptionBase::OptionBase()
    : name_()
    , argName_("arg")
    , numOccurrences_(Optional)
    , numArgs_(ArgOptional)
    , formatting_(DefaultFormatting)
    , miscFlags_(None)
    , count_(0)
{
}

OptionBase::~OptionBase()
{
}

StringRef OptionBase::displayName() const
{
    if (name_.empty())
        return argName_;

    return name_;
}

bool OptionBase::isOccurrenceAllowed() const
{
    if (numOccurrences_ == Optional || numOccurrences_ == Required)
        return count_ == 0;

    return true;
}

bool OptionBase::isOccurrenceRequired() const
{
    if (numOccurrences_ == Required || numOccurrences_ == OneOrMore)
        return count_ == 0;

    return false;
}

bool OptionBase::isUnbounded() const
{
    return numOccurrences_ == ZeroOrMore || numOccurrences_ == OneOrMore;
}

bool OptionBase::isRequired() const
{
    return numOccurrences_ == Required || numOccurrences_ == OneOrMore;
}

bool OptionBase::isPrefix() const
{
    return formatting_ == Prefix || formatting_ == MayPrefix;
}
