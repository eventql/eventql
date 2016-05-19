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
#ifndef _libstx_AXISDEFINITION_H
#define _libstx_AXISDEFINITION_H
#include <utility>
#include <string>
#include <vector>
#include "eventql/util/charts/domain.h"
#include "eventql/util/charts/domainprovider.h"

namespace util {
namespace chart {

class AxisDefinition {
public:

  /**
   * The axis positions/placements
   */
  enum kPosition {
    TOP = 0,
    RIGHT = 1,
    BOTTOM = 2,
    LEFT = 3
  };

  /**
   * The axis tick position
   */
  enum kLabelPosition {
    LABELS_INSIDE = 0,
    LABELS_OUTSIDE = 1,
    LABELS_OFF = 2
  };

  /**
   * Create a new axis definition
   *
   * @param axis_position the position of the axis ({TOP,RIGHT,BOTTOM,LEFT})
   */
  AxisDefinition(kPosition axis_position);

  /**
   * Create a new axis definition from a domain
   *
   * @param axis_position the position of the axis ({TOP,RIGHT,BOTTOM,LEFT})
   * @param domain the domain. does not transfer ownership
   */
  AxisDefinition(kPosition axis_position, DomainProvider* domain);

  /**
   * Add a "tick" to this axis
   *
   * @param tick_position the position of the tick (0.0-1.0)
   */
  void addTick(double tick_position);

  /**
   * Returns the ticks of this axis
   */
  const std::vector<double> getTicks() const;

  /**
   * Add a label to this axis
   *
   * @param label_position the position of the label (0.0-1.0)
   * @param label_text the label text
   */
  void addLabel(double label_position, const std::string& label_text);

  /**
   * Removes the labels from this axis
   */
  void removeLabels();

  /**
   * Returns the labels of this axis
   */
  const std::vector<std::pair<double, std::string>> getLabels() const;

  /**
   * Returns true if this axis has labels, false otherwise
   */
  bool hasLabels() const;

  /**
   * Return the position/placement of this axis
   */
  kPosition getPosition() const;

  /**
   * Set the label position for this axis
   */
  void setLabelPosition(kLabelPosition pos);

  /**
   * Return the label position for this axis
   */
  kLabelPosition getLabelPosition() const;

  /**
   * Set the label rotation for this axis
   */
  void setLabelRotation(double deg);

  /**
   * Return the label rotaitoj for this axis
   */
  double getLabelRotation() const;

  /**
   * Set the title for this axis
   */
  void setTitle(const std::string& title);

  /**
   * Get the title for this axis
   */
  const std::string& getTitle();

  /**
   * Returns true if the title of this axis is a string with len > 0 and false
   * otherwise
   */
  bool hasTitle() const;

  /**
   * Set the domain for this axis
   */
  void setDomain(DomainProvider* domain);

protected:
  kPosition position_;
  DomainProvider* domain_;
  std::string title_;
  std::vector<double> ticks_;
  bool has_ticks_;
  std::vector<std::pair<double, std::string>> labels_;
  bool has_labels_;
};

}
}
#endif
