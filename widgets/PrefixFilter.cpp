#include "widgets/PrefixFilter.hpp"

#include <QHash>
#include <QRegularExpression>

namespace
{
  // Compiled patterns, keyed by the text between the slashes.  Single threaded
  // (GUI) use only; QRegularExpression is implicitly shared so the copy out is
  // cheap.
  QRegularExpression compiled (QString const & pattern)
  {
    static QHash<QString, QRegularExpression> cache;
    auto it = cache.find (pattern);
    if (cache.end () == it)
      {
        it = cache.insert (pattern, QRegularExpression {pattern});
      }
    return it.value ();
  }
}

namespace PrefixFilter
{
  bool entry_matches (QString const & raw_entry, QString const & call, QString const & entity)
  {
    auto const entry = raw_entry.trimmed ();
    if (entry.isEmpty ()) return false;

    if (entry.startsWith ('+'))
      {
        return entity.contains (entry.mid (1), Qt::CaseInsensitive);
      }

    if (entry.size () > 2 && entry.startsWith ('/') && entry.endsWith ('/'))
      {
        auto const regexp = compiled (entry.mid (1, entry.size () - 2));
        return regexp.isValid () && regexp.match (call).hasMatch ();
      }

    return call.startsWith (entry);
  }
}
