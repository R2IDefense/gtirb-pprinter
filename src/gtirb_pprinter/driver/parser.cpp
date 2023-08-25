#include "parser.hpp"
#include <iomanip>

using namespace std::literals;

namespace gtirb_pprint_parser {

std::string quote(char C) {
  static const std::string NeedEscapingForRegex("^$\\.*+?()[]{}|");
  if (NeedEscapingForRegex.find(C) != std::string::npos) {
    return "\\"s + C;
  } else
    return ""s + C;
}

std::optional<fs::path>
getOutputFilePath(const std::vector<FileTemplateRule>& Subs,
                  const std::string& ModuleName) {
  for (const auto& Sub : Subs) {
    if (auto Path = Sub.substitute(ModuleName)) {
      return fs::path(*Path);
    }
  }
  return {};
}

/**
 * @brief Parse a string into a list of substitutions to be made
 *
 * Substitution patterns are presumed to be separated by commas;
 * literal commas need to be escaped
 *
 *
 * Grammar for substitutions:
 * INPUT := SUB | SUB,SUBS
 * SUB := FILE | MODULE=FILE
 *
 */
std::vector<FileTemplateRule> parseInput(const std::string& Input) {
  std::vector<FileTemplateRule> Subs;
  bool Escaped = false;
  auto Start = Input.begin();
  for (auto In = Input.begin(); In != Input.end(); In++) {
    if (Escaped) {
      Escaped = false;
      continue;
    }
    if (*In == '\\') {
      Escaped = true;
      continue;
    }
    if (*In == ',') {
      Subs.emplace_back(Start, In);
      Start = In + 1;
    }
  }
  Subs.emplace_back(Start, Input.end());
  return Subs;
}

FileTemplateRule::FileTemplateRule(std::string::const_iterator SpecBegin,
                                   std::string::const_iterator SpecEnd)
    : MPattern{".*", {{"name", 0}, {"n", 0}}} {
  bool Escape = false;
  for (auto SpecIter = SpecBegin; SpecIter != SpecEnd; ++SpecIter) {
    if (Escape) {
      Escape = false;
      continue;
    }
    if (*SpecIter == '\\') {
      Escape = true;
    }

    if (*SpecIter == '=') {
      MPattern = makePattern(SpecBegin, SpecIter);
      FileTemplate = makeFileTemplate(++SpecIter, SpecEnd);
      return;
    }
  }
  FileTemplate = makeFileTemplate(SpecBegin, SpecEnd);
}

std::string
FileTemplateRule::makeFileTemplate(std::string::const_iterator PBegin,
                                   std::string::const_iterator PEnd) {
  /*
   * Grammar for file patterns:
   *
   * FILE := TERM | TERM TERMS
   * TERM := {NAME} | LITERAL
   * NAME := any alphanumeric character or `_`
   * LITERAL := \{ | \, | \= | \\ | any unescaped character
   *
   * `\` only needs to be escaped when it would otherwise form an escape
   * sequence
   */
  std::string SpecialChars{"{\\,="};
  std::string Pattern;
  std::string GroupName;
  for (auto I = PBegin; I != PEnd; I++) {
    switch (*I) {
    case '{':
      I++;
      if (auto J = std::find(I, PEnd, '}'); J != PEnd) {
        GroupName = std::string(I, J);
        auto GroupIndexesIter = MPattern.GroupIndexes.find(GroupName);
        if (GroupIndexesIter == MPattern.GroupIndexes.end()) {
          throw parse_error("Undefined group: {"s + GroupName + "}");
        }
        auto GI = GroupIndexesIter->second;
        Pattern.push_back('$');
        if (GI == 0) {
          Pattern.push_back('&');
        } else if (GI < 10) {
          Pattern.append("0"s + std::to_string(GI));
        } else {
          Pattern.append(std::to_string(GI));
        }
        I = J;
      } else {
        throw parse_error("Unclosed `{` in file template");
      }
      break;
    case '\\':
      I++;
      if (I != PEnd && SpecialChars.find(*I) != std::string::npos) {
        Pattern.push_back(*I);
      } else {
        Pattern.push_back('\\');
        --I;
      }
      break;
    case '$':
      Pattern.append("$$");
      break;
    case ',':
    case '=':
      throw parse_error("Character "s + *I + " must be escaped");
    default:
      Pattern.push_back(*I);
      break;
    }
  }
  return Pattern;
}

std::optional<std::string>
FileTemplateRule::substitute(const std::string& P) const {
  if (auto M = matches(P)) {
    return M->format(FileTemplate);
  }
  return {};
}

ModulePattern makePattern(std::string::const_iterator FieldBegin,
                          std::string::const_iterator FieldEnd) {
  /*
  Grammar for module patterns:

  MODULE ::= GLOB | GLOB GLOBS

  GLOB ::= NAMEDGLOB | ANONYMOUSGLOB

  NAMEDGLOB ::= '{' NAME ':' ANONYMOUSGLOB '}'

  NAME ::= alpha numeric characters, plus `_`

  ANONYMOUSGLOB ::= EXPR | EXPR EXPRS

  EXPR ::= '*' | '?' | LITERAL

  LITERAL ::= '\\' | '\*' | '\?' | '\=' | '\,' | '\{' | '\}' | '\[' | '\]'
            | any unescaped character except the special characters above

  */
  ModulePattern Pattern;
  Pattern.GroupIndexes["name"] = 0;
  Pattern.GroupIndexes["n"] = 0;

  std::string SpecialChars{"\\=,{}:*?[]"};
  std::vector<std::string> GroupNames;
  std::regex WordChars("^\\w+", std::regex::optimize);
  bool OpenGroup = false;
  for (auto i = FieldBegin; i != FieldEnd; i++) {
    std::smatch M;
    switch (*i) {
    case '{':
      if (OpenGroup) {
        throw parse_error("Invalid character in pattern: "s + *i);
      }
      OpenGroup = true;
      Pattern.RegexStr.push_back('(');
      ++i;
      // We know at this point that `i <= FieldEnd`. If `i == FieldEnd`, then
      // std::regex_search will scan an empty sequence and won't dereference
      // `i`, and M.str().length() will be 0 so `i` won't advance further
      std::regex_search(i, FieldEnd, M, WordChars);
      // M.str().length() can never be greater than `FieldEnd - i`
      i += M.str().length();
      if (i == FieldEnd) {
        throw parse_error("Unclosed '{' in group "s + GroupNames.back());
      }
      if (*i != ':') {
        throw parse_error("Invalid character in group name: '"s + *i + "'");
      }
      if (M.str().length() == 0) {
        throw parse_error("All groups must be named");
      }
      GroupNames.push_back(M.str());
      break;
    case '}':
      if (OpenGroup) {
        Pattern.RegexStr.push_back(')');
        OpenGroup = false;
      } else {
        Pattern.RegexStr.append("\\}");
      }
      break;
    case '*':
      Pattern.RegexStr.append(".*?");
      break;
    case '?':
      Pattern.RegexStr.push_back('.');
      break;
    case '\\':
      ++i;
      if (i != FieldEnd && SpecialChars.find(*i) != std::string::npos) {
        Pattern.RegexStr.append(quote(*i));
      } else {
        Pattern.RegexStr.append("\\\\");
        --i;
      }
      break;
    default:
      Pattern.RegexStr.append(quote(*i));
    }
  }
  if (OpenGroup) {
    throw parse_error("Unclosed '{' in group"s + GroupNames.back());
  }
  for (size_t s = 0; s < GroupNames.size(); s++) {
    auto& Name = GroupNames[s];
    Pattern.GroupIndexes[Name] = s + 1;
  }
  return Pattern;
}

} // namespace gtirb_pprint_parser
