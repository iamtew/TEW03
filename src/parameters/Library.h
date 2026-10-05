#pragma once

#include "../dsp/Sequencer.h"
#include "ParameterIDs.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace tew
{

inline bool storeInPatch (const char* id)
{
    return std::strcmp (id, ParamID::seqBank) != 0
        && std::strcmp (id, ParamID::seqPattern) != 0;
}

struct PatchParam
{
    std::string id;
    float value = 0.f;
};

inline bool readXmlAttr (const std::string& tag, const char* key, std::string& out)
{
    const std::string k = std::string (key) + "=\"";
    const auto pos = tag.find (k);
    if (pos == std::string::npos)
        return false;
    const auto start = pos + k.size();
    const auto end = tag.find ('"', start);
    if (end == std::string::npos)
        return false;
    out.assign (tag, start, end - start);
    return true;
}

inline std::string writePatchXml (const std::vector<PatchParam>& params)
{
    std::string o = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<TEW03PATCH>\n";
    for (const auto& p : params)
    {
        if (! storeInPatch (p.id.c_str()))
            continue;
        char line[192];
        std::snprintf (line, sizeof (line), "  <PARAM id=\"%s\" value=\"%.9g\"/>\n",
                       p.id.c_str(), (double) p.value);
        o += line;
    }
    o += "</TEW03PATCH>\n";
    return o;
}

inline bool readPatchXml (const std::string& xml, std::vector<PatchParam>& out)
{
    if (xml.find ("<TEW03PATCH") == std::string::npos)
        return false;

    out.clear();
    std::size_t pos = 0;
    while ((pos = xml.find ("<PARAM", pos)) != std::string::npos)
    {
        const auto end = xml.find ('>', pos);
        if (end == std::string::npos)
            return false;
        const auto tag = xml.substr (pos, end - pos);
        pos = end + 1;

        std::string id, val;
        if (! readXmlAttr (tag, "id", id) || ! readXmlAttr (tag, "value", val))
            continue;
        if (! storeInPatch (id.c_str()))
            continue;
        out.push_back ({ std::move (id), (float) std::atof (val.c_str()) });
    }
    return true;
}

inline std::string writeBankXml (
    const Sequencer::Step steps[Sequencer::numBanks][Sequencer::patternsPerBank][Sequencer::maxSteps])
{
    std::string o = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<TEW03BANK>\n";
    for (int b = 0; b < Sequencer::numBanks; ++b)
        for (int p = 0; p < Sequencer::patternsPerBank; ++p)
            for (int i = 0; i < Sequencer::maxSteps; ++i)
            {
                const auto s = steps[b][p][i];
                char line[128];
                std::snprintf (line, sizeof (line),
                               "  <STEP b=\"%d\" p=\"%d\" i=\"%d\" note=\"%d\" accent=\"%d\" slide=\"%d\"/>\n",
                               b, p, i, s.note, s.accent ? 1 : 0, s.slide ? 1 : 0);
                o += line;
            }
    o += "</TEW03BANK>\n";
    return o;
}

inline bool readBankXml (
    const std::string& xml,
    Sequencer::Step steps[Sequencer::numBanks][Sequencer::patternsPerBank][Sequencer::maxSteps])
{
    if (xml.find ("<TEW03BANK") == std::string::npos)
        return false;

    for (int b = 0; b < Sequencer::numBanks; ++b)
        for (int p = 0; p < Sequencer::patternsPerBank; ++p)
            for (int i = 0; i < Sequencer::maxSteps; ++i)
                steps[b][p][i] = { -1, false, false };

    std::size_t pos = 0;
    while ((pos = xml.find ("<STEP", pos)) != std::string::npos)
    {
        const auto end = xml.find ('>', pos);
        if (end == std::string::npos)
            return false;
        const auto tag = xml.substr (pos, end - pos);
        pos = end + 1;

        std::string b, p, i, note, acc, sld;
        if (! readXmlAttr (tag, "b", b) || ! readXmlAttr (tag, "p", p)
            || ! readXmlAttr (tag, "i", i) || ! readXmlAttr (tag, "note", note))
            continue;

        const int bi = std::atoi (b.c_str());
        const int pi = std::atoi (p.c_str());
        const int ii = std::atoi (i.c_str());
        if (bi < 0 || bi >= Sequencer::numBanks
            || pi < 0 || pi >= Sequencer::patternsPerBank
            || ii < 0 || ii >= Sequencer::maxSteps)
            continue;

        readXmlAttr (tag, "accent", acc);
        readXmlAttr (tag, "slide", sld);
        steps[bi][pi][ii] = { std::atoi (note.c_str()), acc == "1", sld == "1" };
    }
    return true;
}

} // namespace tew
