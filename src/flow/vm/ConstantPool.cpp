// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/flow/vm/ConstantPool.h>
#include <iostream>
#include <iomanip>
#include <inttypes.h>

namespace xzero {
namespace flow {
namespace vm {

// {{{ helpers
template <typename T, typename S>
std::vector<T>&& convert(const std::vector<S>& source) {
  std::vector<T> target(source.size());

  for (S value : source) target[value->id()] = value->get();

  return std::move(target);
}

template <typename T, typename U>
inline bool equals(const std::vector<T>& a, const std::vector<U>& b) {
  if (a.size() != b.size()) return false;

  for (size_t i = 0, e = a.size(); i != e; ++i)
    if (a[i] != b[i]) return false;

  return true;
}

template <typename T, typename U>
inline size_t ensureValue(std::vector<std::vector<T>>& vv, const U& array) {
  for (size_t i = 0, e = vv.size(); i != e; ++i) {
    const auto& test = vv[i];

    if (test.size() != array.size()) continue;

    if (!equals(test, array)) continue;

    return i;
  }

  // we hand-add each element seperately because
  // it might be, that the source element type is not the same as the target
  // element type
  // (such as std::string -> Buffer)

  vv.push_back(std::vector<T>(array.size()));
  auto& target = vv.back();

  for (size_t i = 0, e = array.size(); i != e; ++i) target[i] = array[i];

  return vv.size() - 1;
}

template <typename T, typename U>
inline size_t ensureValue(std::vector<T>& table, const U& literal) {
  for (size_t i = 0, e = table.size(); i != e; ++i)
    if (table[i] == literal) return i;

  table.push_back(literal);
  return table.size() - 1;
}
// }}}

ConstantPool::ConstantPool() {}

ConstantPool::ConstantPool(ConstantPool&& from)
    : numbers_(std::move(from.numbers_)),
      strings_(std::move(from.strings_)),
      ipaddrs_(std::move(from.ipaddrs_)),
      cidrs_(std::move(from.cidrs_)),
      regularExpressions_(std::move(from.regularExpressions_)),
      intArrays_(std::move(from.intArrays_)),
      stringArrays_(std::move(from.stringArrays_)),
      ipaddrArrays_(std::move(from.ipaddrArrays_)),
      cidrArrays_(std::move(from.cidrArrays_)),
      modules_(std::move(from.modules_)),
      handlers_(std::move(from.handlers_)),
      matchDefs_(std::move(from.matchDefs_)),
      nativeHandlerSignatures_(std::move(from.nativeHandlerSignatures_)),
      nativeFunctionSignatures_(std::move(from.nativeFunctionSignatures_)) {}

ConstantPool& ConstantPool::operator=(ConstantPool&& v) { return *this; }

ConstantPool::~ConstantPool() {}

size_t ConstantPool::makeInteger(FlowNumber value) {
  return ensureValue(numbers_, value);
}

size_t ConstantPool::makeString(const std::string& value) {
  return ensureValue(strings_, value);
}

size_t ConstantPool::makeIPAddress(const IPAddress& value) {
  return ensureValue(ipaddrs_, value);
}

size_t ConstantPool::makeCidr(const Cidr& value) {
  return ensureValue(cidrs_, value);
}

size_t ConstantPool::makeRegExp(const RegExp& value) {
  return ensureValue(regularExpressions_, value);
}

size_t ConstantPool::makeIntegerArray(const std::vector<FlowNumber>& elements) {
  return ensureValue(intArrays_, elements);
}

size_t ConstantPool::makeStringArray(const std::vector<std::string>& elements) {
  for (size_t i = 0, e = stringArrays_.size(); i != e; ++i) {
    const auto& test = stringArrays_[i];

    if (test.second.size() != elements.size()) continue;

    if (!equals(test.second, elements)) continue;

    return i;
  }

  stringArrays_.push_back(
      std::make_pair(std::vector<Buffer>(elements.size()),
                     std::vector<BufferRef>(elements.size())));

  auto& target = stringArrays_.back();

  for (size_t i = 0, e = elements.size(); i != e; ++i) {
    target.first[i] = elements[i];
    target.second[i] = target.first[i].ref();
  }

  return stringArrays_.size() - 1;
}

size_t ConstantPool::makeIPaddrArray(const std::vector<IPAddress>& elements) {
  return ensureValue(ipaddrArrays_, elements);
}

size_t ConstantPool::makeCidrArray(const std::vector<Cidr>& elements) {
  return ensureValue(cidrArrays_, elements);
}

size_t ConstantPool::makeMatchDef() {
  matchDefs_.push_back(MatchDef());
  return matchDefs_.size() - 1;
}

size_t ConstantPool::makeNativeHandler(const std::string& sig) {
  return ensureValue(nativeHandlerSignatures_, sig);
}

size_t ConstantPool::makeNativeFunction(const std::string& sig) {
  return ensureValue(nativeFunctionSignatures_, sig);
}

size_t ConstantPool::makeHandler(const std::string& name) {
  size_t i = 0;
  size_t e = handlers_.size();

  while (i != e) {
    if (handlers_[i].first == name) {
      return i;
    }

    ++i;
  }

  handlers_.resize(i + 1);
  handlers_[i].first = name;
  return i;
}

void dump(const std::vector<std::vector<Buffer>>& vv, const char* name) {
  if (vv.empty()) return;

  std::cout << "\n; Constant " << name << " Arrays\n";
  for (size_t i = 0, e = vv.size(); i != e; ++i) {
    const auto& array = vv[i];
    std::cout << ".const array<" << name << "> " << std::setw(3) << i << " = [";
    for (size_t k = 0, m = array.size(); k != m; ++k) {
      if (k) std::cout << ", ";
      std::cout << '"' << array[k] << '"';
    }
    std::cout << "];\n";
  }
}

template <typename T>
void dump(const std::vector<std::vector<T>>& vv, const char* name) {
  if (vv.empty()) return;

  std::cout << "\n; Constant " << name << " Arrays\n";
  for (size_t i = 0, e = vv.size(); i != e; ++i) {
    const auto& array = vv[i];
    std::cout << ".const array<" << name << "> " << std::setw(3) << i << " = [";
    for (size_t k = 0, m = array.size(); k != m; ++k) {
      if (k) std::cout << ", ";
      std::cout << array[k];
    }
    std::cout << "];\n";
  }
}

void ConstantPool::dump() const {
  if (!modules_.empty()) {
    printf("\n; Modules\n");
    for (size_t i = 0, e = modules_.size(); i != e; ++i) {
      if (modules_[i].second.empty())
        printf(".module '%s'\n", modules_[i].first.c_str());
      else
        printf(".module '%s' from '%s'\n", modules_[i].first.c_str(),
               modules_[i].second.c_str());
    }
  }

  if (!nativeFunctionSignatures_.empty()) {
    printf("\n; External Functions\n");
    for (size_t i = 0, e = nativeFunctionSignatures_.size(); i != e; ++i) {
      printf(".extern function %3zu = %-20s\n", i,
             nativeFunctionSignatures_[i].c_str());
    }
  }

  if (!nativeHandlerSignatures_.empty()) {
    printf("\n; External Handlers\n");
    for (size_t i = 0, e = nativeHandlerSignatures_.size(); i != e; ++i) {
      printf(".extern handler %4zu = %-20s\n", i,
             nativeHandlerSignatures_[i].c_str());
    }
  }

  if (!numbers_.empty()) {
    printf("\n; Integer Constants\n");
    for (size_t i = 0, e = numbers_.size(); i != e; ++i) {
      printf(".const integer %5zu = %" PRIi64 "\n", i, (FlowNumber)numbers_[i]);
    }
  }

  if (!strings_.empty()) {
    printf("\n; String Constants\n");
    for (size_t i = 0, e = strings_.size(); i != e; ++i) {
      printf(".const string %6zu = '%s'\n", i, strings_[i].str().c_str());
    }
  }

  if (!ipaddrs_.empty()) {
    printf("\n; IP Constants\n");
    for (size_t i = 0, e = ipaddrs_.size(); i != e; ++i) {
      printf(".const ipaddr %6zu = %s\n", i, ipaddrs_[i].str().c_str());
    }
  }

  if (!cidrs_.empty()) {
    printf("\n; CIDR Constants\n");
    for (size_t i = 0, e = cidrs_.size(); i != e; ++i) {
      printf(".const cidr %8zu = %s\n", i, cidrs_[i].str().c_str());
    }
  }

  if (!regularExpressions_.empty()) {
    printf("\n; Regular Expression Constants\n");
    for (size_t i = 0, e = regularExpressions_.size(); i != e; ++i) {
      printf(".const regex %7zu = /%s/\n", i, regularExpressions_[i].c_str());
    }
  }

  flow::vm::dump(intArrays_, "Integer");

  if (!stringArrays_.empty()) {
    std::cout << "\n; Constant String Arrays\n";
    for (size_t i = 0, e = stringArrays_.size(); i != e; ++i) {
      const auto& array = stringArrays_[i].first;
      std::cout << ".const array<string> " << std::setw(3) << i << " = [";
      for (size_t k = 0, m = array.size(); k != m; ++k) {
        if (k) std::cout << ", ";
        std::cout << '"' << array[k] << '"';
      }
      std::cout << "];\n";
    }
  }

  flow::vm::dump(ipaddrArrays_, "IPAddress");
  flow::vm::dump(cidrArrays_, "Cidr");

  if (!matchDefs_.empty()) {
    printf("\n; Match Table\n");
    for (size_t i = 0, e = matchDefs_.size(); i != e; ++i) {
      const MatchDef& def = matchDefs_[i];
      printf(".const match %7zu = handler %zu, op %s, elsePC %" PRIu64
             " ; %s\n",
             i, def.handlerId, tos(def.op).c_str(), def.elsePC,
             handlers_[def.handlerId].first.c_str());

      for (size_t k = 0, m = def.cases.size(); k != m; ++k) {
        const MatchCaseDef& one = def.cases[k];

        printf("                       case %3zu = label %2" PRIu64
               ", pc %4" PRIu64 " ; ",
               k, one.label, one.pc);

        if (def.op == MatchClass::RegExp) {
          printf("/%s/\n", regularExpressions_[one.label].c_str());
        } else {
          printf("'%s'\n", strings_[one.label].str().c_str());
        }
      }
    }
  }
}

}  // namespace vm
}  // namespace flow
}  // namespace xzero
