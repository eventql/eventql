#include <xzero/http/HeaderField.h>
#include <xzero/Buffer.h>

namespace xzero {

HeaderField::HeaderField(const std::string& name, const std::string& value)
  : name_(name), value_(value) {
}

bool HeaderField::operator==(const HeaderField& other) const {
  return iequals(name(), other.name()) && iequals(value(), other.value());
}

bool HeaderField::operator!=(const HeaderField& other) const {
  return !(*this == other);
}

} // namespace xzero
