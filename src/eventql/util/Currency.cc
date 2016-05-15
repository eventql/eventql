/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include "eventql/util/exception.h"
#include "eventql/util/Currency.h"
#include "eventql/util/stringutil.h"

namespace stx {

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

  if (s == "eur") return CURRENCY_EUR;
  if (s == "pln") return CURRENCY_PLN;
  if (s == "usd") return CURRENCY_USD;

  return CURRENCY_UNKNOWN;
}

String currencyToString(Currency lang) {
  switch (lang) {
    case CURRENCY_UNKNOWN: return "unknown";
    case CURRENCY_EUR: return "EUR";
    case CURRENCY_PLN: return "PLN";
    case CURRENCY_USD: return "USD";
  }
}

} // namespace stx
