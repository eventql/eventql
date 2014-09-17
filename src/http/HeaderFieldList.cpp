#include <xzero/http/HeaderFieldList.h>
#include <xzero/Buffer.h>
#include <algorithm>

namespace xzero {

HeaderFieldList::HeaderFieldList(
    const std::initializer_list<std::pair<std::string, std::string>>& init) {

  for (const auto& field : init) {
    push_back(field.first, field.second);
  }
}

void HeaderFieldList::push_back(const std::string& name,
                                const std::string& value) {
  entries_.push_back(HeaderField(name, value));
}

void HeaderFieldList::overwrite(const std::string& name,
                                const std::string& value) {
  remove(name);
  push_back(name, value);
}

void HeaderFieldList::append(const std::string& name,
                             const std::string& value) {
  for (HeaderField& field : entries_) {
    if (iequals(field.name(), name)) {
      field.setValue(field.value() + value);
      return;
    }
  }

  push_back(name, value);
}

void HeaderFieldList::remove(const std::string& name) {
  auto start = begin();
  while (start != end()) {
    auto i = std::find_if(begin(), end(), [&](const HeaderField& field) {
      return iequals(field.name(), name);
    });

    if (i != end()) {
      entries_.erase(i);
    }
    start = i;
  }
}

bool HeaderFieldList::contains(const std::string& name) const {
  for (const HeaderField& field : entries_) {
    if (iequals(field.name(), name)) {
      return true;
    }
  }

  return false;
}

bool HeaderFieldList::contains(const std::string& name,
                               const std::string& value) const {
  for (const HeaderField& field : entries_) {
    if (iequals(field.name(), name) && iequals(field.value(), value)) {
      return true;
    }
  }

  return false;
}

const std::string& HeaderFieldList::get(const std::string& name) const {
  for (const HeaderField& field : entries_) {
    if (iequals(field.name(), name)) {
      return field.value();
    }
  }
  static std::string notfound;
  return notfound;
}

void HeaderFieldList::reset() { entries_.clear(); }

}  // namespace xzero
