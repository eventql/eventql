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
#include <eventql/sql/runtime/charts/domainconfig.h>
#include <cplot/continuousdomain.h>
#include <cplot/timedomain.h>

namespace csql {

DomainConfig::DomainConfig(
    util::chart::Drawable* drawable,
    util::chart::AnyDomain::kDimension dimension) :
    domain_(drawable->getDomain(dimension)),
    dimension_letter_(util::chart::AnyDomain::kDimensionLetters[dimension]) {}

void DomainConfig::setMin(const SValue& value) {
  auto int_domain = dynamic_cast<
      util::chart::ContinuousDomain<SValue::IntegerType>*>(domain_);
  if (int_domain != nullptr) {
    int_domain->setMin(value.getValue<SValue::IntegerType>());
    return;
  }

  auto float_domain = dynamic_cast<
      util::chart::ContinuousDomain<SValue::FloatType>*>(domain_);
  if (float_domain != nullptr) {
    float_domain->setMin(value.getValue<SValue::FloatType>());
    return;
  }

  auto time_domain =
      dynamic_cast<util::chart::TimeDomain*>(domain_);
  if (time_domain != nullptr) {
    time_domain->setMin(value.getValue<SValue::TimeType>());
    return;
  }

  RAISE(
      kRuntimeError,
      "TypeError: can't set min value for %c domain",
      dimension_letter_);
}

void DomainConfig::setMax(const SValue& value) {
  auto int_domain = dynamic_cast<
      util::chart::ContinuousDomain<SValue::IntegerType>*>(domain_);
  if (int_domain != nullptr) {
    int_domain->setMax(value.getValue<SValue::IntegerType>());
    return;
  }

  auto float_domain = dynamic_cast<
      util::chart::ContinuousDomain<SValue::FloatType>*>(domain_);
  if (float_domain != nullptr) {
    float_domain->setMax(value.getValue<SValue::FloatType>());
    return;
  }

  auto time_domain =
      dynamic_cast<util::chart::TimeDomain*>(domain_);
  if (time_domain != nullptr) {
    time_domain->setMax(value.getValue<SValue::TimeType>());
    return;
  }

  RAISE(
      kRuntimeError,
      "TypeError: can't set max value for %c domain",
      dimension_letter_);
}

void DomainConfig::setInvert(bool invert) {
  if (!invert) {
    return;
  }

  domain_->setInverted(invert);
}

void DomainConfig::setLogarithmic(bool logarithmic) {
  if (!logarithmic) {
    return;
  }

  auto continuous_domain =
      dynamic_cast<util::chart::AnyContinuousDomain*>(domain_);
  if (continuous_domain == nullptr) {
    RAISE(
        kRuntimeError,
        "TypeError: can't set LOGARITHMIC for discrete domain %c",
        dimension_letter_);
  } else {
    continuous_domain->setLogarithmic(logarithmic);
  }
}

}
