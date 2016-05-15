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
#include "cplot/domainprovider.h"

namespace util {
namespace chart {

DomainProvider::DomainProvider(
    AnyDomain* domain /* = nullptr */) :
    domain_(domain),
    free_on_destroy_(false) {};

DomainProvider::~DomainProvider() {
  if (free_on_destroy_) {
    delete domain_;
  }
}

AnyDomain* DomainProvider::get() const {
  return domain_;
}

bool DomainProvider::empty() const {
  return domain_ == nullptr;
}

void DomainProvider::reset(
    AnyDomain* domain,
    bool free_on_destroy /* = false */) {
  if (free_on_destroy_) {
    delete domain_;
  }

  domain_ = domain;
  free_on_destroy_ = free_on_destroy;
}

const std::vector<double> DomainProvider::getTicks() const {
  if (empty()) {
    return std::vector<double>{};
  } else {
    return domain_->getTicks();
  }
}

const std::vector<std::pair<double, std::string>>
    DomainProvider::getLabels() const {
  if (empty()) {
    return std::vector<std::pair<double, std::string>>{};
  } else {
    return domain_->getLabels();
  }
}


}
}
