// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/flow/FlowLexer.h>
#include <xzero/net/IPAddress.h>
#include <unordered_map>
#include <string.h>
#include <glob.h>

//#define TRACE(level, msg...) XZERO_DEBUG("FlowLexer", (level), msg)
#define TRACE(level, msg...) do {} while (0)

namespace xzero {
namespace flow {

inline std::string escape(char value)  // {{{
{
  switch (value) {
    case '\t':
      return "<TAB>";
    case '\r':
      return "<CR>";
    case '\n':
      return "<LF>";
    case ' ':
      return "<SPACE>";
    default:
      break;
  }
  if (std::isprint(value)) {
    std::string s;
    s += value;
    return s;
  } else {
    char buf[16];
    snprintf(buf, sizeof(buf), "0x%02X", value);
    return buf;
  }
}  // }}}

static inline std::string unescape(const std::string& value)  // {{{
{
  std::string result;

  for (auto i = value.begin(), e = value.end(); i != e; ++i) {
    if (*i != '\\') {
      result += *i;
    } else if (++i != e) {
      switch (*i) {
        case '\\':
          result += '\\';
          break;
        case 'r':
          result += '\r';
          break;
        case 'n':
          result += '\n';
          break;
        case 't':
          result += '\t';
          break;
        default:
          result += *i;
      }
    }
  }

  return result;
}
// }}}

FlowLexer::FlowLexer()
    : contexts_(),
      currentChar_(EOF),
      ipv6HexDigits_(0),
      location_(),
      token_(FlowToken::Eof),
      stringValue_(),
      ipValue_(),
      numberValue_(0),
      interpolationDepth_(0) {}

FlowLexer::Scope::Scope()
    : filename(), basedir(), stream(), currPos(), nextPos() {}

void FlowLexer::Scope::setStream(const std::string& filename,
                                 std::unique_ptr<std::istream>&& istream) {
  this->filename = filename;
  this->stream = std::move(istream);
  this->basedir = "TODO";
}

FlowLexer::~FlowLexer() {}

bool FlowLexer::open(const std::string& filename) {
  if (enterScope(filename) == nullptr) return false;

  nextToken();
  return true;
}

bool FlowLexer::open(const std::string& filename,
                     std::unique_ptr<std::istream>&& ifs) {
  if (enterScope(filename, std::move(ifs)) == nullptr) return false;

  nextToken();
  return true;
}

FlowLexer::Scope* FlowLexer::enterScope(const std::string& filename,
                                        std::unique_ptr<std::istream>&& ifs) {
  std::unique_ptr<Scope> cx(new Scope());
  if (!cx) return nullptr;

  TRACE(1, "ENTER CONTEXT '%s' (depth: %zu)", filename.c_str(),
        contexts_.size() + 1);

  cx->setStream(filename, std::move(ifs));
  cx->backupChar = currentChar();

  contexts_.push_front(std::move(cx));

  // parse first char in new context
  currentChar_ = '\0';  // something that is not EOF
  nextChar();

  return contexts_.front().get();
}

FlowLexer::Scope* FlowLexer::enterScope(const std::string& filename) {
  auto ifs = std::unique_ptr<std::ifstream>(new std::ifstream());
  if (!ifs) return nullptr;

  ifs->open(filename);
  if (!ifs->good()) return nullptr;

  return enterScope(filename, std::move(ifs));
}

void FlowLexer::leaveScope() {
  TRACE(1, "LEAVE CONTEXT '%s' (depth: %zu)", scope()->filename.c_str(),
        contexts_.size());

  currentChar_ = scope()->backupChar;
  contexts_.pop_front();
}

int FlowLexer::peekChar() { return scope()->stream->peek(); }

bool FlowLexer::eof() const {
  return currentChar_ == EOF || scope()->stream->eof();
}

int FlowLexer::nextChar(bool interscope) {
  if (currentChar_ == EOF) return currentChar_;

  location_.end = scope()->currPos;
  scope()->currPos = scope()->nextPos;

  int ch = scope()->stream->get();
  if (ch == EOF) {
    currentChar_ = ch;

    if (interscope && contexts_.size() > 1) {
      leaveScope();
    }
    return currentChar_;
  }

  currentChar_ = ch;
  // content_ += static_cast<char>(currentChar_);
  scope()->nextPos.offset++;

  if (currentChar_ != '\n') {
    scope()->nextPos.column++;
  } else {
    scope()->nextPos.column = 1;
    scope()->nextPos.line++;
  }

  return currentChar_;
}

bool FlowLexer::consume(char ch) {
  bool result = currentChar() == ch;
  nextChar();
  return result;
}

/**
 * @retval true data pending
 * @retval false EOF reached
 */
bool FlowLexer::consumeSpace() {
  // skip spaces
  for (;; nextChar()) {
    if (eof()) return false;

    if (std::isspace(currentChar_)) continue;

    if (std::isprint(currentChar_)) break;

    // TODO proper error reporting through API callback
    TRACE(1, "%s[%04zu:%02zu]: invalid byte 0x%02X\n",
          location_.filename.c_str(), line(), column(), currentChar() & 0xFF);
  }

  if (eof()) return true;

  if (currentChar() == '#') {
    bool maybeCommand = scope()->currPos.column == 1;
    std::string line;
    nextChar();
    // skip chars until EOL
    for (;;) {
      if (eof()) {
        token_ = FlowToken::Eof;
        if (maybeCommand) processCommand(line);
        return token_ != FlowToken::Eof;
      }

      if (currentChar() == '\n') {
        if (maybeCommand) processCommand(line);
        return consumeSpace();
      }

      line += static_cast<char>(currentChar());
      nextChar();
    }
  }

  if (currentChar() == '/' && peekChar() == '*') {  // "/*" ... "*/"
    // parse multiline comment
    nextChar();

    for (;;) {
      if (eof()) {
        token_ = FlowToken::Eof;
        // reportError(Error::UnexpectedEof);
        return false;
      }

      if (currentChar() == '*' && peekChar() == '/') {
        nextChar();  // skip '*'
        nextChar();  // skip '/'
        break;
      }

      nextChar();
    }

    return consumeSpace();
  }

  return true;
}

void FlowLexer::processCommand(const std::string& line) {
  // `#include "glob"`

  if (strncmp(line.c_str(), "include", 7) != 0) return;

  // TODO this is *very* basic sub-tokenization, but it does well until properly
  // implemented.

  size_t beg = line.find('"');
  size_t end = line.rfind('"');
  if (beg == std::string::npos || end == std::string::npos) {
    TRACE(1, "Malformed #include line\n");
    return;
  }

  std::string pattern = line.substr(beg + 1, end - beg - 1);

  TRACE(1, "Process include: '%s'", pattern.c_str());

  glob_t gl;
  int rv = glob(pattern.c_str(), GLOB_TILDE, nullptr, &gl);
  if (rv != 0) {
    static std::unordered_map<int, const char*> globErrs = {
        {GLOB_NOSPACE, "No space"},
        {GLOB_ABORTED, "Aborted"},
        {GLOB_NOMATCH, "No Match"}, };
    TRACE(1, "glob() error: %s", globErrs[rv]);
    return;
  }

  // put globbed files on stack in reverse order, to be lexed in the right order
  for (size_t i = 0; i < gl.gl_pathc; ++i) {
    const char* filename = gl.gl_pathv[gl.gl_pathc - i - 1];
    enterScope(filename);
  }

  globfree(&gl);
}

FlowToken FlowLexer::nextToken() {
  if (!consumeSpace()) return token_ = FlowToken::Eof;

  lastLocation_ = location_;

  location_.filename = scope()->filename;
  location_.begin = scope()->currPos;

  TRACE(2,
        "nextToken(): currentChar %s curr[%zu:%zu.%zu] curr_tok(%s) "
        "next[%zu:%zu.%zu]",
        escape(currentChar_).c_str(), scope()->currPos.line,
        scope()->currPos.column, scope()->currPos.offset, token().c_str(),
        scope()->nextPos.line, scope()->nextPos.column,
        scope()->nextPos.offset);

  switch (currentChar()) {
    case '~':
      nextChar();
      return token_ = FlowToken::BitNot;
    case '=':
      switch (nextChar()) {
        case '=':
          nextChar();
          return token_ = FlowToken::Equal;
        case '^':
          nextChar();
          return token_ = FlowToken::PrefixMatch;
        case '$':
          nextChar();
          return token_ = FlowToken::SuffixMatch;
        case '~':
          nextChar();
          return token_ = FlowToken::RegexMatch;
        case '>':
          nextChar();
          return token_ = FlowToken::HashRocket;
        default:
          return token_ = FlowToken::Assign;
      }
    case '<':
      switch (nextChar()) {
        case '<':
          nextChar();
          return token_ = FlowToken::Shl;
        case '=':
          nextChar();
          return token_ = FlowToken::LessOrEqual;
        default:
          return token_ = FlowToken::Less;
      }
    case '>':
      switch (nextChar()) {
        case '>':
          nextChar();
          return token_ = FlowToken::Shr;
        case '=':
          nextChar();
          return token_ = FlowToken::GreaterOrEqual;
        default:
          return token_ = FlowToken::Greater;
      }
    case '^':
      nextChar();
      return token_ = FlowToken::BitXor;
    case '|':
      switch (nextChar()) {
        case '|':
          nextChar();
          return token_ = FlowToken::Or;
        case '=':
          nextChar();
          return token_ = FlowToken::OrAssign;
        default:
          return token_ = FlowToken::BitOr;
      }
    case '&':
      switch (nextChar()) {
        case '&':
          nextChar();
          return token_ = FlowToken::And;
        case '=':
          nextChar();
          return token_ = FlowToken::AndAssign;
        default:
          return token_ = FlowToken::BitAnd;
      }
    case '.':
      if (nextChar() == '.') {
        if (nextChar() == '.') {
          nextChar();
          return token_ = FlowToken::Ellipsis;
        }
        return token_ = FlowToken::DblPeriod;
      }
      return token_ = FlowToken::Period;
    case ':':
      if (peekChar() == ':') {
        stringValue_.clear();
        return continueParseIPv6(false);
      } else {
        nextChar();
        return token_ = FlowToken::Colon;
      }
    case ';':
      nextChar();
      return token_ = FlowToken::Semicolon;
    case ',':
      nextChar();
      return token_ = FlowToken::Comma;
    case '{':
      nextChar();
      return token_ = FlowToken::Begin;
    case '}':
      if (interpolationDepth_) {
        return token_ = parseInterpolationFragment(false);
      } else {
        nextChar();
        return token_ = FlowToken::End;
      }
    case '(':
      nextChar();
      return token_ = FlowToken::RndOpen;
    case ')':
      nextChar();
      return token_ = FlowToken::RndClose;
    case '[':
      nextChar();
      return token_ = FlowToken::BrOpen;
    case ']':
      nextChar();
      return token_ = FlowToken::BrClose;
    case '+':
      nextChar();
      return token_ = FlowToken::Plus;
    case '-':
      nextChar();
      return token_ = FlowToken::Minus;
    case '*':
      switch (nextChar()) {
        case '*':
          nextToken();
          return token_ = FlowToken::Pow;
        default:
          return token_ = FlowToken::Mul;
      }
    case '/':
      // if (expectsValue) {
      //   return token_ = parseString('/', FlowToken::RegExp);
      // }

      nextChar();
      return token_ = FlowToken::Div;
    case '%':
      nextChar();
      return token_ = FlowToken::Mod;
    case '!':
      switch (nextChar()) {
        case '=':
          nextChar();
          return token_ = FlowToken::UnEqual;
        default:
          return token_ = FlowToken::Not;
      }
    case '\'':
      return token_ = parseString(true);
    case '"':
      ++interpolationDepth_;
      return token_ = parseInterpolationFragment(true);
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      return parseNumber();
    default:
      if (std::isalpha(currentChar()) || currentChar() == '_')
        return token_ = parseIdent();

      TRACE(1, "nextToken: unknown char %s (0x%02X)\n",
            escape(currentChar()).c_str(), currentChar() & 0xFF);

      nextChar();
      return token_ = FlowToken::Unknown;
  }

  return token_;
}

FlowToken FlowLexer::parseString(bool raw) {
  FlowToken result = parseString(raw ? '\'' : '"', FlowToken::String);

  if (result == FlowToken::String && raw) stringValue_ = unescape(stringValue_);

  return result;
}

FlowToken FlowLexer::parseString(char delimiter, FlowToken result) {
  int delim = currentChar();
  int last = -1;

  nextChar();  // skip left delimiter
  stringValue_.clear();

  while (!eof() && (currentChar() != delim || (last == '\\'))) {
    stringValue_ += static_cast<char>(currentChar());

    last = currentChar();
    nextChar();
  }

  if (currentChar() == delim) {
    nextChar();

    return token_ = result;
  }

  return token_ = FlowToken::Unknown;
}

FlowToken FlowLexer::parseInterpolationFragment(bool start) {
  int last = -1;
  stringValue_.clear();

  // skip either '"' or '}' depending on your we entered
  nextChar();

  for (;;) {
    if (eof()) return token_ = FlowToken::Eof;

    if (currentChar() == '"' && last != '\\') {
      nextChar();
      --interpolationDepth_;
      return token_ =
                 start ? FlowToken::String : FlowToken::InterpolatedStringEnd;
    }

    if (currentChar() == '\\') {
      nextChar();

      if (eof()) return token_ = FlowToken::Eof;

      switch (currentChar()) {
        case 'r':
          stringValue_ += '\r';
          break;
        case 'n':
          stringValue_ += '\n';
          break;
        case 't':
          stringValue_ += '\t';
          break;
        case '\\':
          stringValue_ += '\\';
          break;
        default:
          stringValue_ += '\\';
          stringValue_ += static_cast<char>(currentChar());
          break;
      }
    } else if (currentChar() == '$') {
      nextChar();
      if (currentChar() == '{') {
        nextChar();
        return token_ = FlowToken::InterpolatedStringFragment;
      } else {
        stringValue_ += '$';
      }
      stringValue_ += static_cast<char>(currentChar());
    } else {
      stringValue_ += static_cast<char>(currentChar());
    }

    last = currentChar();
    nextChar();
  }
}

FlowToken FlowLexer::parseNumber() {
  stringValue_.clear();
  numberValue_ = 0;

  while (std::isdigit(currentChar())) {
    numberValue_ *= 10;
    numberValue_ += currentChar() - '0';
    stringValue_ += static_cast<char>(currentChar());
    nextChar();
  }

  // ipv6HexDigit4 *(':' ipv6HexDigit4) ['::' [ipv6HexSeq]]
  if (stringValue_.size() <= 4 && currentChar() == ':')
    return continueParseIPv6(true);

  if (stringValue_.size() < 4 && isHexChar()) return continueParseIPv6(false);

  if (currentChar() != '.') return token_ = FlowToken::Number;

  // 2nd IP component
  stringValue_ += '.';
  nextChar();
  while (std::isdigit(currentChar())) {
    stringValue_ += static_cast<char>(currentChar());
    nextChar();
  }

  // 3rd IP component
  if (!consume('.')) return token_ = FlowToken::Unknown;

  stringValue_ += '.';
  while (std::isdigit(currentChar())) {
    stringValue_ += static_cast<char>(currentChar());
    nextChar();
  }

  // 4th IP component
  if (!consume('.')) return token_ = FlowToken::Unknown;

  stringValue_ += '.';
  while (std::isdigit(currentChar())) {
    stringValue_ += static_cast<char>(currentChar());
    nextChar();
  }

  ipValue_.set(stringValue_.c_str(), IPAddress::V4);

  if (currentChar() != '/') return token_ = FlowToken::IP;

  // IPv4 CIDR
  return continueCidr(32);
}

FlowToken FlowLexer::parseIdent() {
  stringValue_.clear();
  stringValue_ += static_cast<char>(currentChar());
  bool isHex = isHexChar();

  nextChar();

  while (std::isalnum(currentChar()) || currentChar() == '_' ||
         currentChar() == '.') {
    stringValue_ += static_cast<char>(currentChar());
    if (!isHexChar()) isHex = false;

    nextChar();
  }

  if (currentChar() == ':' && !isHex) {
    nextChar();  // skip ':'
    return FlowToken::NamedParam;
  }

  // ipv6HexDigit4 *(':' ipv6HexDigit4) ['::' [ipv6HexSeq]]
  if (stringValue_.size() <= 4 && isHex && currentChar() == ':')
    return continueParseIPv6(true);

  if (stringValue_.size() < 4 && isHex && isHexChar())
    return continueParseIPv6(false);

  static struct {
    const char* symbol;
    FlowToken token;
  } keywords[] = {{"in", FlowToken::In},
                  {"var", FlowToken::Var},
                  {"match", FlowToken::Match},
                  {"on", FlowToken::On},
                  {"do", FlowToken::Do},
                  {"if", FlowToken::If},
                  {"then", FlowToken::Then},
                  {"else", FlowToken::Else},
                  {"unless", FlowToken::Unless},
                  {"import", FlowToken::Import},
                  {"from", FlowToken::From},
                  {"handler", FlowToken::Handler},
                  {"and", FlowToken::And},
                  {"or", FlowToken::Or},
                  {"xor", FlowToken::Xor},
                  {"not", FlowToken::Not},
                  {"shl", FlowToken::Shl},
                  {"shr", FlowToken::Shr},
                  {"bool", FlowToken::BoolType},
                  {"int", FlowToken::NumberType},
                  {"string", FlowToken::StringType},
                  {0, FlowToken::Unknown}};

  for (auto i = keywords; i->symbol; ++i)
    if (strcmp(i->symbol, stringValue_.c_str()) == 0) return token_ = i->token;

  if (stringValue_ == "true" || stringValue_ == "yes") {
    numberValue_ = 1;
    return token_ = FlowToken::Boolean;
  }

  if (stringValue_ == "false" || stringValue_ == "no") {
    numberValue_ = 0;
    return token_ = FlowToken::Boolean;
  }

  return token_ = FlowToken::Ident;
}
// {{{ IPv6 address parser
// IPv6_HexPart ::= IPv6_HexSeq                        # (1)
//                | IPv6_HexSeq "::" [IPv6_HexSeq]     # (2)
//                            | "::" [IPv6_HexSeq]     # (3)
//
bool FlowLexer::ipv6HexPart() {
  bool rv;

  if (currentChar() == ':' && peekChar() == ':') {  // (3)
    stringValue_ = "::";
    nextChar();  // skip ':'
    nextChar();  // skip ':'
    rv = isHexChar() ? ipv6HexSeq() : true;
  } else if (!!(rv = ipv6HexSeq())) {
    if (currentChar() == ':' && peekChar() == ':') {  // (2)
      stringValue_ += "::";
      nextChar();  // skip ':'
      nextChar();  // skip ':'
      rv = isHexChar() ? ipv6HexSeq() : true;
    }
  }

  if (std::isalnum(currentChar_) || currentChar_ == ':') rv = false;

  return rv;
}

// 1*4HEXDIGIT *(':' 1*4HEXDIGIT)
bool FlowLexer::ipv6HexSeq() {
  if (!ipv6HexDigit4()) return false;

  while (currentChar() == ':' && peekChar() != ':') {
    stringValue_ += ':';
    nextChar();

    if (!ipv6HexDigit4()) return false;
  }

  return true;
}

// 1*4HEXDIGIT
bool FlowLexer::ipv6HexDigit4() {
  size_t i = ipv6HexDigits_;

  while (isHexChar()) {
    stringValue_ += currentChar();
    nextChar();
    ++i;
  }

  ipv6HexDigits_ = 0;

  return i >= 1 && i <= 4;
}

bool FlowLexer::continueParseRegEx(char delim) {
  int last = -1;

  stringValue_.clear();

  while (!eof() && (currentChar() != delim || (last == '\\'))) {
    stringValue_ += static_cast<char>(currentChar());
    last = currentChar();
    nextChar();
  }

  if (currentChar() == delim) {
    nextChar();
    token_ = FlowToken::RegExp;
    return true;
  }

  token_ = FlowToken::Unknown;
  return false;
}

// ipv6HexDigit4 *(':' ipv6HexDigit4) ['::' [ipv6HexSeq]]
// where the first component, ipv6HexDigit4 is already parsed
FlowToken FlowLexer::continueParseIPv6(bool firstComplete) {
  bool rv = true;
  if (firstComplete) {
    while (currentChar() == ':' && peekChar() != ':') {
      stringValue_ += ':';
      nextChar();

      if (!ipv6HexDigit4()) return false;
    }

    if (currentChar() == ':' && peekChar() == ':') {
      stringValue_ += "::";
      nextChar();
      nextChar();
      rv = isHexChar() ? ipv6HexSeq() : true;
    }
  } else {
    ipv6HexDigits_ = stringValue_.size();
    rv = ipv6HexPart();
  }

  // parse embedded IPv4 remainer
  while (currentChar_ == '.' && std::isdigit(peekChar())) {
    stringValue_ += '.';
    nextChar();

    while (std::isdigit(currentChar_)) {
      stringValue_ += static_cast<char>(currentChar_);
      nextChar();
    }
  }

  if (!rv)
    // Invalid IPv6
    return token_ = FlowToken::Unknown;

  if (!ipValue_.set(stringValue_.c_str(), IPAddress::V6))
    // Invalid IPv6
    return token_ = FlowToken::Unknown;

  if (currentChar_ != '/') return token_ = FlowToken::IP;

  return continueCidr(128);
}

FlowToken FlowLexer::continueCidr(size_t range) {
  // IPv6 CIDR
  nextChar();  // consume '/'

  if (!std::isdigit(currentChar())) {
    TRACE(1, "%s[%04zu:%02zu]: invalid byte 0x%02X\n",
          location_.filename.c_str(), line(), column(), currentChar() & 0xFF);
    return token_ = FlowToken::Unknown;
  }

  numberValue_ = 0;
  while (std::isdigit(currentChar())) {
    numberValue_ *= 10;
    numberValue_ += currentChar() - '0';
    stringValue_ += static_cast<char>(currentChar());
    nextChar();
  }

  if (numberValue_ > static_cast<decltype(numberValue_)>(range)) {
    TRACE(1, "%s[%04zu:%02zu]: CIDR prefix out of range.\n",
          location_.filename.c_str(), line(), column());
    return token_ = FlowToken::Unknown;
  }

  return token_ = FlowToken::Cidr;
}
// }}}

}  // namespace flow
}  // namespace xzero
