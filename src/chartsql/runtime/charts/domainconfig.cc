/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/runtime/charts/domainconfig.h>
#include <cplot/continuousdomain.h>
#include <cplot/timedomain.h>

namespace csql {

DomainConfig::DomainConfig(
    stx::chart::Drawable* drawable,
    stx::chart::AnyDomain::kDimension dimension) :
    domain_(drawable->getDomain(dimension)),
    dimension_letter_(stx::chart::AnyDomain::kDimensionLetters[dimension]) {}

void DomainConfig::setMin(const SValue& value) {
  auto int_domain = dynamic_cast<
      stx::chart::ContinuousDomain<SValue::IntegerType>*>(domain_);
  if (int_domain != nullptr) {
    int_domain->setMin(value.getValue<SValue::IntegerType>());
    return;
  }

  auto float_domain = dynamic_cast<
      stx::chart::ContinuousDomain<SValue::FloatType>*>(domain_);
  if (float_domain != nullptr) {
    float_domain->setMin(value.getValue<SValue::FloatType>());
    return;
  }

  auto time_domain =
      dynamic_cast<stx::chart::TimeDomain*>(domain_);
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
      stx::chart::ContinuousDomain<SValue::IntegerType>*>(domain_);
  if (int_domain != nullptr) {
    int_domain->setMax(value.getValue<SValue::IntegerType>());
    return;
  }

  auto float_domain = dynamic_cast<
      stx::chart::ContinuousDomain<SValue::FloatType>*>(domain_);
  if (float_domain != nullptr) {
    float_domain->setMax(value.getValue<SValue::FloatType>());
    return;
  }

  auto time_domain =
      dynamic_cast<stx::chart::TimeDomain*>(domain_);
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
      dynamic_cast<stx::chart::AnyContinuousDomain*>(domain_);
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
