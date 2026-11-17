#ifndef PREFIX_FILTER_HPP_
#define PREFIX_FILTER_HPP_

#include <QString>

// One entry of the per-band prefix filter (the "Filtering / Prefixes" panel).
//
// Three forms, matching the WSJT-Z 2.19.b7 behaviour:
//
//   SP          literal prefix, matched with startsWith against the callsign
//   +poland     case-insensitive substring of the DXCC entity name
//   /3Z|HF/     slash-delimited regular expression matched against the call
//
// Entries are comma separated per band line ("40m:+poland,/3Z|HF/"), so a
// regular expression must not contain a comma.  Patterns are compiled once and
// cached: the filter text is re-parsed for every decode, which makes
// recompiling per decode measurable on a busy band.

namespace PrefixFilter
{
  // An empty entry never matches; an invalid expression never matches.
  bool entry_matches (QString const & entry, QString const & call, QString const & entity);
}

#endif
