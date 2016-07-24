/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#ifndef _libstx_DOMAIN_H
#define _libstx_DOMAIN_H
#include <algorithm>
#include <math.h>
#include <stdlib.h>
#include <vector>

// FIXPAUL too many copies T val...
namespace util {
namespace chart {

/**
 * Untyped domain base class
 */
class AnyDomain {
public:
  static const char kDimensionLetters[];

  // FIXPAUL make this configurable
  static const int kDefaultNumTicks;
  static const double kDefaultDomainPadding;

  enum kDimension {
    DIM_X = 0,
    DIM_Y = 1,
    DIM_Z = 2
  };

  virtual ~AnyDomain() {}

  virtual const std::vector<double> getTicks() const = 0;

  virtual const std::vector<std::pair<double, std::string>> getLabels()
      const = 0;

  virtual void setInverted(bool inverted) = 0;

  virtual void build() = 0;

};

/**
 * Polymorphic domain
 */
template <typename T>
class Domain : public AnyDomain {
public:
  virtual ~Domain() {}

  static Domain<T>* mkDomain();

  /**
   * Returns the label at the specified index
   *
   * @param index the index
   */
  virtual std::string label(T value) const = 0;

  virtual double scale(T value) const = 0;

  virtual std::pair<double, double> scaleRange(T value) const = 0;

  virtual void addValue(const T& value) = 0;

  virtual bool contains(T value) const = 0;

};

}
}
#endif
