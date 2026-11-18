#include "plain_text.hpp"

#include "font.hpp"
#include "syntax.hpp"
#include "text_string.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <format>
#include <numbers>

namespace walk_pdf {

namespace {

// A transformation [a b 0, c d 0, e f 1], applied to row vectors as PDF does
struct Matrix {
  double a = 1, b = 0, c = 0, d = 1, e = 0, f = 0;

  // This one, then other
  Matrix then(Matrix const& o) const {
    return {a * o.a + b * o.c,       a * o.b + b * o.d,       c * o.a + d * o.c,
            c * o.b + d * o.d,       e * o.a + f * o.c + o.e, e * o.b + f * o.d + o.f};
  }
  std::pair<double, double> apply(double x, double y) const { return {a * x + c * y + e, b * x + d * y + f}; }
};

Matrix translation(double x, double y) { return {1, 0, 0, 1, x, y}; }

// The state that q saves and Q restores (the text state is part of it)
struct State {
  Matrix ctm;
  std::shared_ptr<Font const> font;
  double size = 0;
  double char_spacing = 0; // Tc
  double word_spacing = 0; // Tw
  double scale = 100;      // Tz, percent
  double leading = 0;      // TL
  double rise = 0;         // Ts
};

double number(std::string_view token) { return std::strtod(std::string{token}.c_str(), nullptr); }

class Interpreter {
public:
  explicit Interpreter(Document const& document) : document_(document) {}

  std::vector<TextRun> page(std::size_t index) {
    walk(Path{}.child(Segment::pages()).child(Segment::ordinal(Kind::page, index)), 0);
    return std::move(runs_);
  }

private:
  // The operations of a page, or of a form drawn by a Do, in order
  void walk(Path const& parent, int depth) {
    for (auto const& child : document_.children(parent)) {
      if (child.content.kind() == Kind::operation) {
        operation(child.path, child.content, depth);
      }
    }
  }

  void operation(Path const& path, Content const& content, int depth) {
    auto const bytes = content.bytes();
    auto const parsed = next_operation(bytes, 0, bytes.size());
    if (!parsed) {
      return;
    }
    std::vector<Value> operands;
    for (std::size_t pos = 0; auto const value = value_at(bytes, pos, parsed->operator_begin);) {
      pos = value->end;
      operands.push_back(*value);
    }
    auto const token = [&](std::size_t i) { return bytes.substr(operands[i].begin, operands[i].end - operands[i].begin); };
    auto const n = [&](std::size_t i) { return i < operands.size() ? number(token(i)) : 0.0; };
    auto const name = parsed->name;

    if (name == "q") {
      saved_.push_back(state_);
    } else if (name == "Q") {
      if (!saved_.empty()) {
        state_ = saved_.back();
        saved_.pop_back();
      }
    } else if (name == "cm" && operands.size() == 6) {
      state_.ctm = Matrix{n(0), n(1), n(2), n(3), n(4), n(5)}.then(state_.ctm);
    } else if (name == "BT") {
      text_matrix_ = line_matrix_ = Matrix{};
    } else if (name == "Tf" && operands.size() == 2 && operands[0].kind == Kind::name) {
      auto const font = document_.font(path, decode_name(token(0)));
      state_.font = font ? document_.load_font(*font) : nullptr;
      state_.size = n(1);
    } else if (name == "Tc") {
      state_.char_spacing = n(0);
    } else if (name == "Tw") {
      state_.word_spacing = n(0);
    } else if (name == "Tz") {
      state_.scale = n(0);
    } else if (name == "TL") {
      state_.leading = n(0);
    } else if (name == "Ts") {
      state_.rise = n(0);
    } else if (name == "Td") {
      move(n(0), n(1));
    } else if (name == "TD") {
      state_.leading = -n(1);
      move(n(0), n(1));
    } else if (name == "Tm" && operands.size() == 6) {
      text_matrix_ = line_matrix_ = Matrix{n(0), n(1), n(2), n(3), n(4), n(5)};
    } else if (name == "T*") {
      move(0, -state_.leading);
    } else if (name == "Tj" && !operands.empty()) {
      show(token(0));
    } else if (name == "'" && !operands.empty()) {
      move(0, -state_.leading);
      show(token(0));
    } else if (name == "\"" && operands.size() == 3) {
      state_.word_spacing = n(0);
      state_.char_spacing = n(1);
      move(0, -state_.leading);
      show(token(2));
    } else if (name == "TJ" && !operands.empty() && operands[0].kind == Kind::array) {
      for (auto at = first_child_at(operands[0]); auto const item = next_child(bytes, operands[0], at);) {
        at = item->value.end;
        auto const item_token = bytes.substr(item->value.begin, item->value.end - item->value.begin);
        if (item->value.kind == Kind::string) {
          show(item_token);
        } else {
          // A number moves the next glyph back (positive) or on, in thousandths of the font size
          advance(-number(item_token) / 1000 * state_.size * state_.scale / 100);
        }
      }
    } else if (name == "BMC") {
      marked_.push_back({std::nullopt, runs_.size()});
    } else if (name == "BDC") {
      marked_.push_back({actual_text(path, bytes, operands), runs_.size()});
    } else if (name == "EMC") {
      end_marked_content();
    } else if (name == "Do" && depth < 8) {
      if (auto const form = document_.form_at(path)) {
        saved_.push_back(state_);
        std::vector<double> m;
        if (auto const matrix = document_.follow(form->child(Segment::entry("Matrix")))) {
          for (auto const& item : document_.children(*matrix)) {
            m.push_back(number(item.content.bytes()));
          }
        }
        if (m.size() == 6) {
          state_.ctm = Matrix{m[0], m[1], m[2], m[3], m[4], m[5]}.then(state_.ctm);
        }
        walk(path, depth + 1);
        state_ = saved_.back();
        saved_.pop_back();
      }
    }
  }

  // The /ActualText of a BDC's properties: a dictionary in the operation, or
  // a name for one in the resources' /Properties
  std::optional<std::string> actual_text(Path const& path, std::string_view bytes, std::vector<Value> const& operands) {
    if (operands.size() < 2) {
      return std::nullopt;
    }
    auto const& properties = operands[1];
    std::optional<std::string> token;
    if (properties.kind == Kind::dictionary) {
      for (auto at = first_child_at(properties); auto const entry = next_child(bytes, properties, at);) {
        at = entry->value.end;
        if (entry->key == "ActualText" && entry->value.kind == Kind::string) {
          token = std::string{bytes.substr(entry->value.begin, entry->value.end - entry->value.begin)};
        }
      }
    } else if (properties.kind == Kind::name) {
      auto const resources = document_.resources(path);
      auto const list = resources ? document_.follow(resources->child(Segment::entry("Properties"))) : std::nullopt;
      auto const name = decode_name(bytes.substr(properties.begin, properties.end - properties.begin));
      auto const dictionary = list ? document_.follow(list->child(Segment::entry(name))) : std::nullopt;
      if (dictionary) {
        if (auto const text = document_.find(dictionary->child(Segment::entry("ActualText")))) {
          auto decrypted = document_.decrypt(text->path(), string_bytes(text->bytes()));
          if (decrypted) {
            return text_to_utf8(*decrypted).value_or(*decrypted);
          }
          token = std::string{text->bytes()};
        }
      }
    }
    if (!token) {
      return std::nullopt;
    }
    // A text string: UTF-16 or UTF-8 when marked, else PDFDocEncoding (here taken as it is)
    auto const raw = string_bytes(*token);
    return text_to_utf8(raw).value_or(raw);
  }

  // EMC: the runs since its BDC are replaced by one run of the actual text
  void end_marked_content() {
    if (marked_.empty()) {
      return;
    }
    auto const [text, first] = marked_.back();
    marked_.pop_back();
    if (!text || first >= runs_.size()) {
      return;
    }
    TextRun run = runs_[first];
    run.text = *text;
    run.end_x = runs_.back().end_x;
    run.end_y = runs_.back().end_y;
    runs_.resize(first);
    runs_.push_back(std::move(run));
  }

  // Td: to the start of a line, moved from the start of the current one
  void move(double x, double y) {
    line_matrix_ = translation(x, y).then(line_matrix_);
    text_matrix_ = line_matrix_;
  }

  void advance(double x) { text_matrix_ = translation(x, 0).then(text_matrix_); }

  // A string's glyphs: their text, and how far they move the text position
  void show(std::string_view token) {
    if (!state_.font) {
      return; // No font: no text walk_pdf can read
    }
    auto const page = text_matrix_.then(state_.ctm);
    TextRun run;
    std::tie(run.x, run.y) = page.apply(0, state_.rise);
    run.size = state_.size * std::hypot(page.c, page.d);
    for (auto const& glyph : state_.font->glyphs(string_bytes(token))) {
      run.text += glyph.text;
      auto width = glyph.width * state_.size + state_.char_spacing;
      if (glyph.code.size() == 1 && glyph.code[0] == ' ') {
        width += state_.word_spacing; // Tw applies to the one-byte code 32 only
      }
      advance(width * state_.scale / 100);
    }
    std::tie(run.end_x, run.end_y) = text_matrix_.then(state_.ctm).apply(0, state_.rise);
    runs_.push_back(std::move(run));
  }

  Document const& document_;
  State state_;
  std::vector<State> saved_;
  Matrix text_matrix_;
  Matrix line_matrix_;
  std::vector<TextRun> runs_;
  // Marked content open now: its /ActualText, if any, and the first run in it
  std::vector<std::pair<std::optional<std::string>, std::size_t>> marked_;
};

// Spaces at the ends of a line: ' ', tab, and U+00A0 (no-break space, C2 A0 in UTF-8)
std::string trimmed(std::string const& text) {
  auto const space_at = [&](std::size_t i) -> std::size_t { // the length of a space at i, or 0
    if (text[i] == ' ' || text[i] == '\t' || text[i] == '\n' || text[i] == '\r') {
      return 1;
    }
    return text.compare(i, 2, "\xC2\xA0") == 0 ? 2 : 0;
  };
  std::size_t begin = 0;
  while (begin < text.size() && space_at(begin) > 0) {
    begin += space_at(begin);
  }
  auto end = text.size();
  while (end > begin) {
    if (text[end - 1] == ' ' || text[end - 1] == '\t' || text[end - 1] == '\n' || text[end - 1] == '\r') {
      --end;
    } else if (end - begin >= 2 && text.compare(end - 2, 2, "\xC2\xA0") == 0) {
      end -= 2;
    } else {
      break;
    }
  }
  return text.substr(begin, end - begin);
}

} // namespace

std::vector<TextRun> text_runs(Document const& document, std::size_t page) {
  return Interpreter{document}.page(page);
}

std::vector<std::string> lines_of(std::vector<TextRun> const& runs) {
  // Each run in the frame of its direction (rounded to a quarter turn), so
  // that text on rotated pages reads along its own lines: u along, v across
  struct Placed {
    TextRun const* run;
    int turn;    // quarter turns
    double u, v; // start
    double end_u;
  };
  std::vector<Placed> placed;
  for (auto const& run : runs) {
    if (run.text.empty()) {
      continue;
    }
    auto const angle = std::atan2(run.end_y - run.y, run.end_x - run.x);
    auto const turn = run.end_x == run.x && run.end_y == run.y
                          ? 0
                          : static_cast<int>(std::lround(angle / (std::numbers::pi / 2))) & 3;
    auto const cosine = std::array{1.0, 0.0, -1.0, 0.0}[static_cast<std::size_t>(turn)];
    auto const sine = std::array{0.0, 1.0, 0.0, -1.0}[static_cast<std::size_t>(turn)];
    auto const u = [&](double x, double y) { return x * cosine + y * sine; };
    auto const v = [&](double x, double y) { return -x * sine + y * cosine; };
    placed.push_back({&run, turn, u(run.x, run.y), v(run.x, run.y), u(run.end_x, run.end_y)});
  }
  // By direction, then top to bottom
  std::ranges::stable_sort(placed, [](Placed const& a, Placed const& b) {
    return a.turn != b.turn ? a.turn < b.turn : a.v > b.v;
  });

  std::vector<std::string> lines;
  std::optional<double> previous_v;
  double previous_size = 0;
  for (std::size_t first = 0; first < placed.size();) {
    // A line: the runs whose baselines are within half a font size of the first's
    auto const& start = placed[first];
    auto const tolerance = std::max(start.run->size, 1.0) / 2;
    auto last = first + 1;
    while (last < placed.size() && placed[last].turn == start.turn && start.v - placed[last].v <= tolerance) {
      ++last;
    }
    std::vector<Placed> line{placed.begin() + static_cast<std::ptrdiff_t>(first),
                             placed.begin() + static_cast<std::ptrdiff_t>(last)};
    std::ranges::stable_sort(line, [](Placed const& a, Placed const& b) { return a.u < b.u; });

    std::string text;
    for (std::size_t i = 0; i < line.size(); ++i) {
      if (i > 0) {
        auto const gap = line[i].u - line[i - 1].end_u;
        auto const size = std::max(line[i].run->size, line[i - 1].run->size);
        if (gap > size / 5 && !text.empty() && text.back() != ' ' && line[i].run->text.front() != ' ') {
          text += ' ';
        }
      }
      text += line[i].run->text;
    }
    if (auto line_text = trimmed(text); !line_text.empty()) {
      // A gap wider than one and a half lines: an empty line between
      if (previous_v && *previous_v - start.v > 1.5 * std::max(previous_size, start.run->size) && !lines.empty()) {
        lines.emplace_back();
      }
      lines.push_back(std::move(line_text));
      previous_v = start.v;
      previous_size = start.run->size;
    }
    first = last;
  }
  return lines;
}

std::expected<PlainText, std::string> plain_text(Document const& document) {
  if (auto const& security = document.security(); security && !security->allowed(5)) {
    return std::unexpected(std::string{"the file's permissions do not allow copying or extracting its text"});
  }
  PlainText text;
  for (std::size_t page = 0; page < document.pages().size(); ++page) {
    text.pages.push_back(lines_of(text_runs(document, page)));
  }
  return text;
}

std::string PlainText::to_string() const {
  std::string result;
  for (std::size_t page = 0; page < pages.size(); ++page) {
    result += std::format("--- page {} ---\n", page + 1);
    for (auto const& line : pages[page]) {
      result += line;
      result += '\n';
    }
  }
  return result;
}

} // namespace walk_pdf
