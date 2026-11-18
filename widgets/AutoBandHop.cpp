#include "widgets/AutoBandHop.hpp"

#include <QTime>

namespace
{
  // monthMask: bit 0 = January ... bit 11 = December
  // startUTC/endUTC: optimal UTC window; start > end wraps midnight (22..4)
  // Masks are hex because this tree still builds as C++11 (no 0b literals):
  //   0xC03 = Nov,Dec,Jan,Feb        0xC07 = Nov..Mar
  //   0xFFF = year-round             0x71C = Mar,Apr,May,Sep,Oct,Nov
  //   0x1E7 = May..Aug plus Sep..Mar
  BandEntry const kBands[] =
  {
    {"160m", 1.840, 22, 4, 0xC03, 3, "160m: winter night, F-layer DX with the D-layer absent"},
    {"80m", 3.573, 20, 6, 0xC07, 4, "80m: winter nocturnal DX, D-layer gone"},
    {"40m", 7.074, 0, 24, 0xFFF, 6, "40m: 24/7 workhorse, best at night"},
    {"30m", 10.136, 0, 24, 0xFFF, 5, "30m: WARC band, 24/7 digital haven"},
    {"20m", 14.074, 6, 22, 0x71C, 7, "20m: primary DX band, equinox peak"},
    {"17m", 18.100, 8, 19, 0x71C, 6, "17m: WARC band, quiet daytime DX"},
    {"15m", 21.074, 8, 18, 0x71C, 5, "15m: F2 DX, needs SFI > 100"},
    {"12m", 24.915, 9, 17, 0x1E7, 4, "12m: summer sporadic-E and solar-max F2"},
    {"10m", 28.074, 10, 18, 0x1E7, 4, "10m: sporadic-E and solar-max F2, strictly diurnal"},
  };

  int const kBandCount = sizeof (kBands) / sizeof (kBands[0]);
}

AutoBandHop::AutoBandHop (QObject * parent)
  : QObject {parent}
{
  m_timer.setInterval (m_intervalMin * 60 * 1000);
  connect (&m_timer, &QTimer::timeout, this, &AutoBandHop::evaluate);
}

BandEntry const * AutoBandHop::bands () {return kBands;}
int AutoBandHop::bandCount () {return kBandCount;}

bool AutoBandHop::inWindow (int hour, int start, int end)
{
  if (end == 24) return true;
  if (start <= end) return hour >= start && hour < end;
  return hour >= start || hour < end;
}

int AutoBandHop::scoreFor (BandEntry const & band, int utcHour, int monthIndex)
{
  // A band coded as open 24/7 and year-round is *unconstrained*, not
  // *favoured*.  Giving it the same +10 a genuinely time-restricted band earns
  // only during its real peak made 40m win every hour of every month.
  // Zeroing the bonus over-corrected (40m and 30m then never won at all), so
  // permanently open bands get a flat bonus between "no credit" and the +3 for
  // a month match: enough to win genuine gaps, not enough to beat a real match.
  constexpr int always_open_bonus = 4;
  bool const always_time = band.startUTC == 0 && band.endUTC == 24;
  bool const always_month = band.monthMask == 0xFFF;

  if (always_time && always_month)
    {
      return band.basePriority + always_open_bonus;
    }

  bool const time_ok = always_time ? false : inWindow (utcHour, band.startUTC, band.endUTC);
  bool const month_ok = always_month ? false : ((band.monthMask >> monthIndex) & 1);

  int score {band.basePriority};
  if (time_ok && month_ok) score += 10;
  else if (time_ok) score += 5;
  else if (month_ok) score += 3;
  return score;
}

double AutoBandHop::bestFrequencyMHz (QDateTime const & utc, QString * reason,
                                      quint16 allowed)
{
  int const hour = utc.time ().hour ();
  int const month = utc.date ().month () - 1;

  int best_score {-1};
  int best_index {-1};
  for (int i = 0; i < kBandCount; ++i)
    {
      if (0 == (allowed & (quint16 {1} << i))) continue;
      int const score = scoreFor (kBands[i], hour, month);
      if (score > best_score)
        {
          best_score = score;
          best_index = i;
        }
    }

  if (best_index < 0)
    {
      if (reason) *reason = QStringLiteral ("Auto band hop: no bands enabled");
      return 0.0;
    }

  if (reason)
    {
      *reason = QStringLiteral ("Auto band hop: %1 - %2 (score %3)")
        .arg (QString::fromLatin1 (kBands[best_index].name),
              QString::fromLatin1 (kBands[best_index].reason))
        .arg (best_score);
    }
  return kBands[best_index].freqMHz;
}

void AutoBandHop::setEnabled (bool on)
{
  m_enabled = on;
  if (on)
    {
      evaluate ();              // hop immediately when switched on
      m_timer.start ();
    }
  else
    {
      m_timer.stop ();
    }
}

void AutoBandHop::setIntervalMinutes (int minutes)
{
  m_intervalMin = qBound (1, minutes, 60);
  m_timer.setInterval (m_intervalMin * 60 * 1000);
}

void AutoBandHop::setAllowedBands (quint16 allowed)
{
  m_allowed = allowed;
}

void AutoBandHop::evaluate ()
{
  if (0 == m_allowed) return;          // nothing enabled: stay put
  QString reason;
  double const freq = bestFrequencyMHz (QDateTime::currentDateTimeUtc (), &reason, m_allowed);
  if (freq > 0.0) Q_EMIT hopRequested (freq, reason);
}
