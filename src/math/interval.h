#ifndef INTERVAL_H
#define INTERVAL_H
//==============================================================================================
// To the extent possible under law, the author(s) have dedicated all copyright
// and related and neighboring rights to this software to the public domain
// worldwide. This software is distributed without any warranty.
//
// You should have received a copy (see file COPYING.txt) of the CC0 Public
// Domain Dedication along with this software. If not, see
// <http://creativecommons.org/publicdomain/zero/1.0/>.
//==============================================================================================

class interval {
public:
  double min, max;

  interval();
  interval(double min, double max);
  double size() const;
  bool contains(double x) const;
  bool surrounds(double x) const;
  double clamp(double x) const;

  static const interval empty, universe;
};

#endif
