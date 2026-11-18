#ifndef AUTO_BAND_HOP_HPP_
#define AUTO_BAND_HOP_HPP_

// Auto Band Hop - chooses the best HF FT8 band from the UTC hour, the calendar
// month and a static propagation model, then asks the main window to hop.
//
// Ported from 2E0LXY/wsjt-y (GPL-3.0, the same licence as this tree).  The
// scoring model and band table are unchanged.  The integration point is not:
// WSJT-Y calls band_changed(), this tree goes through MainWindow::switchBand()
// so that stopping TX, clearing the DX/pounce state and auto-tune behave
// exactly as they do for the scheduled band hopper.
//
// Scoring, per band:
//   basePriority
//   +10 when the UTC hour is inside the band's window AND the month is optimal
//   +5  when only the hour matches
//   +3  when only the month matches
//   A band that is open 24/7 year-round gets basePriority + 4: it is always
//   *possible*, never *favoured*, so 40m cannot win every hour of every month.
//
// The table is a coarse, UK-centric model (RSGB 2026 / Ofcom 2024); 60m is
// deliberately absent because it is not a general-use band.

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QTimer>

struct BandEntry
{
  char const * name;
  double freqMHz;          // FT8 dial frequency
  int startUTC;            // optimal window start hour (0-23, UTC)
  int endUTC;              // inclusive/exclusive end; wraps midnight when start > end
  int monthMask;           // bit 0 = January ... bit 11 = December
  int basePriority;        // higher wins ties
  char const * reason;     // shown in the status bar
};

class AutoBandHop final
  : public QObject
{
  Q_OBJECT

public:
  explicit AutoBandHop (QObject * parent = nullptr);

  void setEnabled (bool);
  bool enabled () const {return m_enabled;}

  void setIntervalMinutes (int);
  int intervalMinutes () const {return m_intervalMin;}

  // Which bands may be chosen: bit i selects bands()[i].  Zero means nothing
  // is allowed, in which case no hop is requested.  Default: every band.
  void setAllowedBands (quint16);
  quint16 allowedBands () const {return m_allowed;}

  // Best FT8 dial frequency for a given UTC instant; *reason is set to a
  // human-readable explanation suitable for the status bar.
  // Best FT8 dial frequency for a given UTC instant; *reason is set to a
  // human-readable explanation suitable for the status bar.  Returns 0 when
  // no band is allowed by the mask.
  static double bestFrequencyMHz (QDateTime const & utc, QString * reason = nullptr,
                                  quint16 allowed = 0xFFFF);

  static BandEntry const * bands ();
  static int bandCount ();

signals:
  void hopRequested (double freqMHz, QString reason);

private slots:
  void evaluate ();

private:
  static int scoreFor (BandEntry const &, int utcHour, int monthIndex);
  static bool inWindow (int hour, int start, int end);

  QTimer m_timer;
  bool m_enabled {false};
  int m_intervalMin {5};
  quint16 m_allowed {0xFFFF};
};

#endif
