#include <QtTest/QtTest>

#include "widgets/PrefixFilter.hpp"

class TestPrefixFilter final : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  // Literal entries keep their historical exact-prefix semantics.
  void literal_prefix ();

  // "+name" is a case-insensitive substring of the DXCC entity name.
  void entity_name ();

  // "/pattern/" is matched against the callsign, unanchored unless the pattern
  // anchors itself; these are the examples from the 2.19.b7 filter panel.
  void regex_entry ();

  // Bad input must be inert rather than throw or match everything.
  void malformed_input ();
};

void TestPrefixFilter::literal_prefix ()
{
  QVERIFY (PrefixFilter::entry_matches ("SP", "SP9ABC", "Poland"));
  QVERIFY (!PrefixFilter::entry_matches ("SP", "DL1ABC", "Germany"));
  // entries are trimmed before use
  QVERIFY (PrefixFilter::entry_matches ("  SP  ", "SP9ABC", "Poland"));
  // literals match from the start of the call, not anywhere in it
  QVERIFY (!PrefixFilter::entry_matches ("9A", "SP9ABC", "Poland"));
}

void TestPrefixFilter::entity_name ()
{
  QVERIFY (PrefixFilter::entry_matches ("+poland", "SP9ABC", "Poland"));
  QVERIFY (PrefixFilter::entry_matches ("+POLAND", "SP9ABC", "Poland"));
  QVERIFY (!PrefixFilter::entry_matches ("+poland", "DL1ABC", "Germany"));
}

void TestPrefixFilter::regex_entry ()
{
  // "/3Z|HF/" - unanchored alternation: anywhere in the call
  QVERIFY (PrefixFilter::entry_matches ("/3Z|HF/", "3Z1ABC", "Poland"));
  QVERIFY (PrefixFilter::entry_matches ("/3Z|HF/", "HF1ABC", "Poland"));
  QVERIFY (PrefixFilter::entry_matches ("/3Z|HF/", "SP3Z", "Poland"));
  QVERIFY (!PrefixFilter::entry_matches ("/3Z|HF/", "SP9ABC", "Poland"));

  // "/^(\bJ|BJ[0-9])/" - J callsigns and BJ-prefixed calls only
  QVERIFY (PrefixFilter::entry_matches ("/^(\\bJ|BJ[0-9])/", "JA1ABC", "Japan"));
  QVERIFY (PrefixFilter::entry_matches ("/^(\\bJ|BJ[0-9])/", "BJ2XYZ", "Japan"));
  QVERIFY (!PrefixFilter::entry_matches ("/^(\\bJ|BJ[0-9])/", "DL1ABC", "Germany"));

  // "/...[389]$/" - ends in 3, 8 or 9 three characters from the end
  QVERIFY (PrefixFilter::entry_matches ("/...[389]$/", "SP9ABC3", "Poland"));
  QVERIFY (!PrefixFilter::entry_matches ("/...[389]$/", "SP9ABC4", "Poland"));
}

void TestPrefixFilter::malformed_input ()
{
  QVERIFY (!PrefixFilter::entry_matches ("", "SP9ABC", "Poland"));
  QVERIFY (!PrefixFilter::entry_matches ("   ", "SP9ABC", "Poland"));
  QVERIFY (!PrefixFilter::entry_matches ("/(/", "SP9ABC", "Poland"));  // invalid regex
  QVERIFY (!PrefixFilter::entry_matches ("/", "SP9ABC", "Poland"));    // too short to be a regex
}

QTEST_APPLESS_MAIN (TestPrefixFilter)

#include "test_prefix_filter.moc"
