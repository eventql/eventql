/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "fnord-base/exception.h"
#include "fnord-base/Currency.h"
#include "fnord-base/stringutil.h"

namespace fnord {

Money::Money(
    uint64_t _cents,
    Currency _currency) :
    cents(_cents),
    currency(_currency) {}

CurrencyConverter::CurrencyConverter(
    const ConversionTable& conversion_table) :
    conv_table_(conversion_table) {}

Money CurrencyConverter::convert(Money amount, Currency target_currency) const {
  if (amount.currency == target_currency) {
    return amount;
  }

  for (const auto& conv : conv_table_) {
    if (std::get<0>(conv) != amount.currency ||
        std::get<1>(conv) != target_currency) {
      continue;
    }

    auto rate = std::get<2>(conv);
    return Money(amount.cents * rate, target_currency);
  }

  RAISEF(
      kRuntimeError,
      "don't know how to convert from currency '$0' to '$1'",
      currencyToString(amount.currency),
      currencyToString(target_currency));
}

Currency currencyFromString(const String& string) {
  String s(string);
  StringUtil::toLower(&s);

  if (s == "eur") return Currency::EUR;
  if (s == "pln") return Currency::PLN;
  if (s == "usd") return Currency::USD;

  return Currency::UNKNOWN;
}

String currencyToString(Currency lang) {
  switch (lang) {
    case Currency::UNKNOWN: return "unknown";
    case Currency::EUR: return "EUR";
    case Currency::PLN: return "PLN";
    case Currency::USD: return "USD";
  }
}

} // namespace fnord
