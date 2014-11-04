// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/io/Selectable.h>
#include <xzero/io/SelectionKey.h>
#include <xzero/io/Selector.h>

namespace xzero {

std::unique_ptr<SelectionKey> Selectable::registerSelectable(unsigned ops) {
  return selector()->createSelectable(this, ops);
}

} // namespace xzero
