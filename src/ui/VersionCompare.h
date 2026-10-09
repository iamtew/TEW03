#pragma once

#include <cctype>
#include <string>
#include <string_view>
#include <vector>

namespace tew
{

// Tags that are not vN.N.N... (optional leading v, digits and dots only) are ignored.
inline bool parseVersion (std::string_view s, std::vector<int>& parts)
{
    parts.clear();
    if (! s.empty() && (s[0] == 'v' || s[0] == 'V'))
        s.remove_prefix (1);
    if (s.empty() || ! std::isdigit (static_cast<unsigned char> (s[0])))
        return false;

    size_t i = 0;
    while (i < s.size())
    {
        if (! std::isdigit (static_cast<unsigned char> (s[i])))
            return false;
        int n = 0;
        while (i < s.size() && std::isdigit (static_cast<unsigned char> (s[i])))
        {
            n = n * 10 + (s[i] - '0');
            ++i;
        }
        parts.push_back (n);
        if (i == s.size())
            return true;
        if (s[i] != '.')
            return false;
        ++i;
        if (i == s.size())
            return false;
    }
    return ! parts.empty();
}

inline bool versionNewer (std::string_view remote, std::string_view local)
{
    std::vector<int> pr, pl;
    if (! parseVersion (remote, pr) || ! parseVersion (local, pl))
        return false;
    const size_t n = pr.size() > pl.size() ? pr.size() : pl.size();
    pr.resize (n, 0);
    pl.resize (n, 0);
    for (size_t i = 0; i < n; ++i)
    {
        if (pr[i] > pl[i])
            return true;
        if (pr[i] < pl[i])
            return false;
    }
    return false;
}

inline std::string newestNewerTag (const std::vector<std::string>& names, std::string_view local)
{
    std::string best;
    for (const auto& name : names)
    {
        if (! versionNewer (name, local))
            continue;
        if (best.empty() || versionNewer (name, best))
            best = name;
    }
    return best;
}

} // namespace tew
