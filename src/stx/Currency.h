/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_CURRENCY_H
#define _STX_CURRENCY_H
#include "stx/stdtypes.h"
#include "stx/Currency.pb.h"

namespace stx {

Currency currencyFromString(const String& string);
String currencyToString(Currency lang);

struct Money {
  Money(uint64_t cents, Currency currency);
  const uint64_t cents;
  const Currency currency;
};

class CurrencyConverter {
public:
  // <FROM, TO, RATE>
  typedef Tuple<Currency, Currency, double> ConversionRate;
  typedef Vector<ConversionRate> ConversionTable;

  CurrencyConverter(const ConversionTable& conversion_table);

  Money convert(Money amount, Currency target_currency) const;

protected:
  const ConversionTable conv_table_;
};


} // namespace stx

#endif
