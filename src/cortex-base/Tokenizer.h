// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <cortex-base/Api.h>
#include <cortex-base/Buffer.h>
#include <string>
#include <vector>

namespace cortex {

/**
 * General purpose string tokenizer.
 */
template <typename T, typename U = T>
class CORTEX_API Tokenizer {
 private:
  const U& input_;
  T token_;

  mutable size_t lastPos_, charPos_;
  size_t wordPos_;
  std::string delimiter_;

 public:
  explicit Tokenizer(const U& input, const std::string& delimiter = " \t\r\n");

  bool end();
  const T& nextToken();
  const T& token() const { return token_; }

  std::vector<T> tokenize();
  static std::vector<T> tokenize(const U& input,
                                 const std::string& delimiter = " \t\r\n");

  size_t charPosition() const { return charPos_; }
  size_t wordPosition() const { return lastPos_; }

  T gap() {
    end();
    return charPos_ != lastPos_ ? substr(lastPos_, charPos_ - lastPos_) : T();
  }
  T remaining() { return !end() ? substr(charPos_) : T(); }

  Tokenizer<T, U>& operator++() {
    nextToken();
    return *this;
  }
  bool operator!() const { return const_cast<Tokenizer<T, U>*>(this)->end(); }

 private:
  void consumeDelimiter();

  T substr(size_t offset) const;
  T substr(size_t offset, size_t size) const;
};

// {{{ impl
template <typename T, typename U>
inline Tokenizer<T, U>::Tokenizer(const U& input, const std::string& delimiter)
    : input_(input),
      token_(),
      lastPos_(0),
      charPos_(0),
      wordPos_(0),
      delimiter_(delimiter) {}

template <typename T, typename U>
const T& Tokenizer<T, U>::nextToken() {
  if (end()) {
    static const T eos;
    return eos;
  }

  size_t isize = input_.size();
  lastPos_ = charPos_;

  while (charPos_ < isize && delimiter_.find(input_[charPos_]) == T::npos)
    ++charPos_;

  token_ = substr(lastPos_, charPos_ - lastPos_);

  ++wordPos_;
  lastPos_ = charPos_;

  return token_;
}

template <typename T, typename U>
void Tokenizer<T, U>::consumeDelimiter() {
  size_t isize = input_.size();

  while (charPos_ < isize && delimiter_.find(input_[charPos_]) != T::npos) {
    ++charPos_;
  }
}

template <typename T, typename U>
bool Tokenizer<T, U>::end() {
  consumeDelimiter();

  if (charPos_ >= input_.size()) return true;

  return false;
}

template <typename T, typename U>
std::vector<T> Tokenizer<T, U>::tokenize() {
  std::vector<T> tokens;

  while (!end()) tokens.push_back(nextToken());

  return std::move(tokens);
}

template <typename T, typename U>
inline std::vector<T> Tokenizer<T, U>::tokenize(const U& input,
                                                const std::string& delimiter) {
  Tokenizer<T, U> t(input, delimiter);
  return t.tokenize();
}

template <typename T, typename U>
inline T Tokenizer<T, U>::substr(size_t offset, size_t size) const {
  return input_.ref(offset, size);
}

template <typename T, typename U>
inline T Tokenizer<T, U>::substr(size_t offset) const {
  return input_.ref(offset);
}
// }}}

}  // namespace cortex
