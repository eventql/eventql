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
#ifndef _libstx_DISCRETEDOMAIN_H
#define _libstx_DISCRETEDOMAIN_H
#include "eventql/util/stringutil.h"
#include "cplot/domain.h"

namespace util {
namespace chart {

template <typename T>
class DiscreteDomain : public Domain<T> {
public:

  /**
   * Create a new categorical domain
   */
  DiscreteDomain(bool is_inverted = false) : is_inverted_(is_inverted) {}

  std::string label(T value) const {
    return StringUtil::toString(value);
  }

  double scale(T value) const {
    size_t index = categories_.end() - std::find(
        categories_.begin(),
        categories_.end(),
        value);

    if (index < 1) {
      RAISE(kRuntimeError, "can't scale value");
    }

    double cardinality = (double) categories_.size();
    auto scaled = ((double) index - 0.5f) / cardinality;

    if (is_inverted_) {
      return 1.0 - scaled;
    } else {
      return scaled;
    }
  }

  std::pair<double, double> scaleRange(T value) const {
    size_t index = categories_.end() - std::find(
        categories_.begin(),
        categories_.end(),
        value);

    if (index < 1) {
      RAISE(kRuntimeError, "can't scale value");
    }

    auto cardinality = (double) categories_.size();
    auto begin = (double) (index - 1) / cardinality;;
    auto end = (double) index / cardinality;

    if (is_inverted_) {
      return std::make_pair(1.0 - begin, 1.0 - end);
    } else {
      return std::make_pair(begin, end);
    }
  }

  void addValue(const T& value) {
    addCategory(value);
  }

  void addCategory(const T& category) {
    bool insert = std::find(
        categories_.begin(),
        categories_.end(),
        category) == categories_.end();

    if (insert) {
      categories_.emplace_back(category);
    }
  }

  const std::vector<double> getTicks() const {
    std::vector<double> ticks{0.0};

    for (const auto category : categories_) {
      auto range = scaleRange(category);
      ticks.push_back(range.second);
    }

    return ticks;
  }

  const std::vector<std::pair<double, std::string>> getLabels() const {
    std::vector<std::pair<double, std::string>> labels;

    for (const auto category : categories_) {
      auto point = scale(category);
      labels.emplace_back(point, label(category));
    }

    return labels;
  }

  bool contains(T value) const {
    return std::find(
        categories_.begin(),
        categories_.end(),
        value) != categories_.end();
  }

  void setInverted(bool inverted) {
    is_inverted_ = inverted;
  }

  void build() {}

protected:
  bool is_inverted_;
  std::vector<T> categories_;
};

}
}
#endif
