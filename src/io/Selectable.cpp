#include <xzero/io/Selectable.h>
#include <xzero/io/SelectionKey.h>
#include <xzero/io/Selector.h>

namespace xzero {

std::unique_ptr<SelectionKey> Selectable::registerSelectable(unsigned ops) {
  return selector()->createSelectable(this, ops);
}

} // namespace xzero
