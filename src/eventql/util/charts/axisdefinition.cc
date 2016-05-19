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
#include "axisdefinition.h"
#include "domain.h"

namespace util {
namespace chart {

AxisDefinition::AxisDefinition(
    kPosition axis_position) :
    AxisDefinition(axis_position, nullptr) {}

AxisDefinition::AxisDefinition(
    kPosition axis_position,
    DomainProvider* domain) :
    position_(axis_position),
    domain_(domain),
    has_ticks_(false),
    has_labels_(false) {}

void AxisDefinition::addTick(double tick_position) {
  has_ticks_ = true;
  ticks_.push_back(tick_position);
}

const std::vector<double> AxisDefinition::getTicks() const {
  if (has_ticks_ || domain_ == nullptr) {
    return ticks_;
  }

  return domain_->getTicks();
}

void AxisDefinition::addLabel(
    double label_position,
    const std::string& label_text) {
  has_labels_ = true;
  labels_.emplace_back(label_position, label_text);
}

void AxisDefinition::removeLabels() {
  labels_.clear();
}

const std::vector<std::pair<double, std::string>> AxisDefinition::getLabels()
    const {
  if (has_labels_ || domain_ == nullptr) {
    return labels_;
  }

  return domain_->getLabels();
}

bool AxisDefinition::hasLabels() const {
  return has_labels_ || domain_ != nullptr;
}

AxisDefinition::kPosition AxisDefinition::getPosition() const {
  return position_;
}

void AxisDefinition::setLabelPosition(kLabelPosition pos) {
  printf("set label pos: %i", pos);
}

AxisDefinition::kLabelPosition AxisDefinition::getLabelPosition() const {
  return LABELS_INSIDE;
}

void AxisDefinition::setLabelRotation(double deg) {
  printf("axis label rot: %f\n", deg);
}

double AxisDefinition::getLabelRotation() const {
  return 0.0f;
}

void AxisDefinition::setTitle(const std::string& title) {
  title_ = title;
}

const std::string& AxisDefinition::getTitle() {
  return title_;
}

bool AxisDefinition::hasTitle() const {
  return title_.length() > 0;
}

void AxisDefinition::setDomain(DomainProvider* domain) {
  domain_ = domain;
}

}
}
