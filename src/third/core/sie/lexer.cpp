#include "core/sie/lexer.hpp"

#include <format>
#include <utility>

namespace cratchit::sie {

namespace {

bool is_blank(charset::unicode::CodePoint code_point) {
  return code_point == U' ' || code_point == U'\t';
}

} // namespace

std::optional<Lexed> Lexer::push(charset::unicode::CodePoint code_point) {
  if (code_point == U'\r') {
    return std::nullopt;
  }
  if (code_point == U'\n') {
    return end_line();
  }
  switch (state_) {
  case State::line_start:
    if (is_blank(code_point)) {
      break;
    }
    line_.number = number_;
    if (code_point == U'#') {
      line_.kind = Line::Kind::record;
      state_ = State::label;
    } else if (code_point == U'{') {
      line_.kind = Line::Kind::block_begin;
      state_ = State::block;
    } else if (code_point == U'}') {
      line_.kind = Line::Kind::block_end;
      state_ = State::block;
    } else {
      fail("not a record (does not start with #), { or }");
    }
    break;
  case State::label:
    if (is_blank(code_point)) {
      if (text_.empty()) {
        fail("# without a label");
        break;
      }
      line_.label = std::exchange(text_, {});
      state_ = State::between;
    } else {
      append(code_point);
    }
    break;
  case State::between:
    if (is_blank(code_point)) {
      break;
    }
    if (code_point == U'"') {
      state_ = State::quoted;
    } else if (code_point == U'{') {
      if (in_list_) {
        fail("{ inside an object list");
        break;
      }
      in_list_ = true;
    } else if (code_point == U'}') {
      if (!in_list_) {
        fail("} outside an object list");
        break;
      }
      line_.fields.push_back(std::exchange(list_, {}));
      in_list_ = false;
    } else {
      append(code_point);
      state_ = State::unquoted;
    }
    break;
  case State::unquoted:
    if (is_blank(code_point)) {
      end_field();
      state_ = State::between;
    } else if (code_point == U'}' && in_list_) {
      end_field();
      line_.fields.push_back(std::exchange(list_, {}));
      in_list_ = false;
      state_ = State::between;
    } else {
      append(code_point);
    }
    break;
  case State::quoted:
    if (code_point == U'\\') {
      state_ = State::escape;
    } else if (code_point == U'"') {
      state_ = State::after_quote;
    } else {
      append(code_point);
    }
    break;
  case State::escape:
    if (code_point != U'"') {
      append(U'\\');
    }
    append(code_point);
    state_ = State::quoted;
    break;
  case State::after_quote:
    if (is_blank(code_point)) {
      end_field();
      state_ = State::between;
    } else if (code_point == U'}' && in_list_) {
      end_field();
      line_.fields.push_back(std::exchange(list_, {}));
      in_list_ = false;
      state_ = State::between;
    } else {
      fail("text right after a closing quote");
    }
    break;
  case State::block:
    if (!is_blank(code_point)) {
      fail("text after a lone { or }");
    }
    break;
  case State::failed:
    break;
  }
  return std::nullopt;
}

std::optional<Lexed> Lexer::finish() {
  return end_line();
}

std::optional<Lexed> Lexer::end_line() {
  std::optional<Lexed> result;
  switch (state_) {
  case State::line_start:
    break;
  case State::label:
    if (text_.empty()) {
      fail("# without a label");
    } else {
      line_.label = std::exchange(text_, {});
    }
    break;
  case State::unquoted:
  case State::after_quote:
    end_field();
    break;
  case State::quoted:
  case State::escape:
    fail("quote not closed");
    break;
  case State::between:
  case State::block:
  case State::failed:
    break;
  }
  if (state_ != State::failed && in_list_) {
    fail("object list not closed");
  }
  if (state_ == State::failed) {
    result = std::unexpected(std::format("line {}: {}", number_, error_));
  } else if (state_ != State::line_start) {
    result = std::move(line_);
  }
  state_ = State::line_start;
  line_ = {};
  text_.clear();
  in_list_ = false;
  list_ = {};
  error_.clear();
  ++number_;
  return result;
}

void Lexer::fail(std::string what) {
  if (state_ != State::failed) {
    error_ = std::move(what);
    state_ = State::failed;
  }
}

void Lexer::append(charset::unicode::CodePoint code_point) {
  for (auto const byte : to_utf8_.push(code_point)) {
    text_.push_back(static_cast<char>(byte));
  }
}

void Lexer::end_field() {
  if (in_list_) {
    list_.items.push_back(std::exchange(text_, {}));
  } else {
    line_.fields.push_back(std::exchange(text_, {}));
  }
}

} // namespace cratchit::sie
