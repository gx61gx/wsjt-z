#include <QtTest/QtTest>

#include "widgets/AutoBandHop.hpp"

// Locks in the behaviour of the ported WSJT-Y propagation model.  These are
// characterisation tests: if the band table or the scoring is deliberately
// retuned, the expectations here have to be updated on purpose rather than
// drifting.
class TestAutoBandHop final : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void band_table_is_sane ();
  void winter_night_picks_low_band ();
  void spring_midday_picks_20m ();
  void summer_night_picks_40m ();
  void reason_names_the_band ();
  void result_is_always_in_the_table ();
};

namespace
{
  QDateTime utc (int year, int month, int day, int hour)
  {
    return QDateTime {QDate {year, month, day}, QTime {hour, 0}, Qt::UTC};
  }

  double best (int month, int hour)
  {
    return AutoBandHop::bestFrequencyMHz (utc (2026, month, 15, hour));
  }
}

void TestAutoBandHop::band_table_is_sane ()
{
  QCOMPARE (AutoBandHop::bandCount (), 9);
  auto const * bands = AutoBandHop::bands ();
  QVERIFY (bands);
  for (int i = 0; i < AutoBandHop::bandCount (); ++i)
    {
      QVERIFY2 (bands[i].name && *bands[i].name, "band without a name");
      QVERIFY2 (bands[i].freqMHz > 1.0 && bands[i].freqMHz < 30.0, bands[i].name);
      // month masks are 12 bits and never empty
      QVERIFY2 (bands[i].monthMask > 0 && bands[i].monthMask <= 0xFFF, bands[i].name);
      QVERIFY2 (bands[i].startUTC >= 0 && bands[i].startUTC < 24, bands[i].name);
      QVERIFY2 (bands[i].endUTC > 0 && bands[i].endUTC <= 24, bands[i].name);
      if (i) QVERIFY2 (bands[i].freqMHz > bands[i - 1].freqMHz, "table must ascend by frequency");
    }
}

// January 02:00 UTC - 160m/80m territory; the model prefers 80m (base
// priority 4 beats 160m's 3 inside the same tier).
void TestAutoBandHop::winter_night_picks_low_band ()
{
  QCOMPARE (best (1, 2), 3.573);
}

// March midday: 20m's window and season both match.
void TestAutoBandHop::spring_midday_picks_20m ()
{
  QCOMPARE (best (3, 12), 14.074);
}

// June 00:00 UTC: no daytime band matches, so a permanently-open band wins -
// and 40m's higher priority beats 30m.
void TestAutoBandHop::summer_night_picks_40m ()
{
  QCOMPARE (best (6, 0), 7.074);
}

void TestAutoBandHop::reason_names_the_band ()
{
  QString reason;
  double const freq = AutoBandHop::bestFrequencyMHz (utc (2026, 3, 15, 12), &reason);
  QVERIFY (!reason.isEmpty ());
  QVERIFY2 (reason.contains ("20m"), qPrintable (reason));
  QVERIFY2 (reason.contains ("score"), qPrintable (reason));
  QCOMPARE (freq, 14.074);
}

void TestAutoBandHop::result_is_always_in_the_table ()
{
  for (int month = 1; month <= 12; ++month)
    {
      for (int hour = 0; hour < 24; ++hour)
        {
          double const freq = best (month, hour);
          bool found = false;
          for (int i = 0; i < AutoBandHop::bandCount (); ++i)
            {
              if (qFuzzyCompare (AutoBandHop::bands ()[i].freqMHz, freq)) found = true;
            }
          QVERIFY2 (found, qPrintable (QString {"%1/%2 -> %3"}.arg (month).arg (hour).arg (freq)));
        }
    }
}

QTEST_GUILESS_MAIN (TestAutoBandHop)

#include "test_auto_band_hop.moc"
