#include <sstream>
#include <string>
#include <iostream>

#include "./shunting-yard.h"


using cparse::packToken;
using cparse::TokenBase;
using cparse::TokenMap;
using cparse::TokenList;
using cparse::Tuple;
using cparse::STuple;
using cparse::Function;

const packToken& packToken::None() {
  static packToken none = packToken(TokenNone());
  return none;
}

packToken::strFunc_t& packToken::str_custom() {
  static strFunc_t func = 0;
  return func;
}

packToken::packToken(const TokenMap& map) : base(new TokenMap(map)) {}
packToken::packToken(const TokenList& list) : base(new TokenList(list)) {}

packToken& packToken::operator=(const packToken& t) {
  delete base;
  base = t.base->clone();
  return *this;
}

bool packToken::operator==(const packToken& token) const {
  if (NUM_Token & token.base->type & base->type) {
    return token.asDouble() == asDouble();
  }

  if (token.base->type != base->type) {
    return false;
  } else {
    // Compare strings to simplify code
    return token.str() == str();
  }
}

bool packToken::operator!=(const packToken& token) const {
  return !(*this == token);
}

TokenBase* packToken::operator->() const {
  return base;
}

std::ostream& cparse::operator<<(std::ostream &os, const packToken& t) {
  return os << t.str();
}

packToken& packToken::operator[](const std::string& key) {
  if (base->type != MAP_Token) {
    None();
  }
  return (*static_cast<TokenMap*>(base))[key];
}
const packToken& packToken::operator[](const std::string& key) const {
  if (base->type != MAP_Token) {
    None();
  }
  return (*static_cast<TokenMap*>(base))[key];
}
packToken& packToken::operator[](const char* key) {
  if (base->type != MAP_Token) {
    None();
  }
  return (*static_cast<TokenMap*>(base))[key];
}
const packToken& packToken::operator[](const char* key) const {
  if (base->type != MAP_Token) {
    None();
  }
  return (*static_cast<TokenMap*>(base))[key];
}

bool packToken::asBool() const {
  switch (base->type) {
    case REAL_Token:
      return static_cast<Token<double>*>(base)->val != 0;
    case INT_Token:
      return static_cast<Token<int64_t>*>(base)->val != 0;
    case BOOL_Token:
      return static_cast<Token<uint8_t>*>(base)->val != 0;
    case STR_Token:
      return static_cast<Token<std::string>*>(base)->val != std::string();
    case MAP_Token:
    case FUNC_Token:
      return true;
    case NONE_Token:
      return false;
    case TUPLE_Token:
    case STUPLE_Token:
      return static_cast<Tuple*>(base)->list().size() != 0;
    default:
      return false;
  }
}

double packToken::asDouble() const {
  switch (base->type) {
  case REAL_Token:
    return static_cast<Token<double>*>(base)->val;
  case INT_Token:
    return static_cast<Token<int64_t>*>(base)->val;
  case BOOL_Token:
    return static_cast<Token<uint8_t>*>(base)->val;
  default:
    return DBL_MAX;
  }
}

int64_t packToken::asInt() const {
  switch (base->type) {
  case REAL_Token:
    return static_cast<Token<double>*>(base)->val;
  case INT_Token:
    return static_cast<Token<int64_t>*>(base)->val;
  case BOOL_Token:
    return static_cast<Token<uint8_t>*>(base)->val;
  default:
    return INT64_MAX;
  }
}

std::string& packToken::asString() const {
  if (base->type != STR_Token && base->type != VAR_Token && base->type != OP_Token) {
    static std::string empty;
    return empty;
  }
  return static_cast<Token<std::string>*>(base)->val;
}

TokenMap& packToken::asMap() const {
  if (base->type != MAP_Token) {
    return TokenMap::empty;
  }
  return *static_cast<TokenMap*>(base);
}

TokenList& packToken::asList() const {
  if (base->type != LIST_Token) {
    static TokenList list;
    return list;
  }
  return *static_cast<TokenList*>(base);
}

Tuple& packToken::asTuple() const {
  if (base->type != TUPLE_Token) {
    static Tuple tuple;
    return tuple;
  }
  return *static_cast<Tuple*>(base);
}

STuple& packToken::asSTuple() const {
  if (base->type != STUPLE_Token) {
    static STuple stuple;
    return stuple;
  }
  return *static_cast<STuple*>(base);
}

Function* packToken::asFunc() const {
  if (base->type != FUNC_Token) {
    return nullptr;
  }
  return static_cast<Function*>(base);
}

void* packToken::asPoint() const
{
  if (base->type != POINT_Token) {
    return nullptr;
  }
  return (static_cast<Token<void *>*>(base))->val;
}

std::string packToken::str(uint32_t nest) const {
  return packToken::str(base, nest);
}

std::string packToken::str(const TokenBase* base, uint32_t nest) {
  std::stringstream ss;
  TokenMap_t* tmap;
  TokenMap_t::iterator m_it;

  TokenList_t* tlist;
  TokenList_t::iterator l_it;
  const Function* func;
  bool first, boolval;
  std::string name;

  if (!base) return "undefined";

  if (base->type & REF_Token) {
    base = static_cast<const RefToken*>(base)->resolve();
    name = static_cast<const RefToken*>(base)->key.str();
  }

  /* * * * * Check for a user defined functions: * * * * */

  if (packToken::str_custom()) {
    std::string result = packToken::str_custom()(base, nest);
    if (result != "") {
      return result;
    }
  }

  /* * * * * Stringify the token: * * * * */

  switch (base->type) {
    case NONE_Token:
      return "None";
    case UNARY_Token:
      return "UnaryToken";
    case OP_Token:
      return static_cast<const Token<std::string>*>(base)->val;
    case VAR_Token:
      return static_cast<const Token<std::string>*>(base)->val;
    case REAL_Token:
      ss << static_cast<const Token<double>*>(base)->val;
      return ss.str();
    case INT_Token:
      ss << static_cast<const Token<int64_t>*>(base)->val;
      return ss.str();
    case BOOL_Token:
      boolval = static_cast<const Token<uint8_t>*>(base)->val;
      return boolval ? "True" : "False";
    case STR_Token:
      return "\"" + static_cast<const Token<std::string>*>(base)->val + "\"";
    case FUNC_Token:
      func = static_cast<const Function*>(base);
      if (func->name().size()) return "[Function: " + func->name() + "]";
      if (name.size()) return "[Function: " + name + "]";
      return "[Function]";
    case TUPLE_Token:
    case STUPLE_Token:
      if (nest == 0) return "[Tuple]";
      ss << "(";
      first = true;
      for (const packToken& token : static_cast<const Tuple*>(base)->list()) {
        if (!first) {
          ss << ", ";
        } else {
          first = false;
        }
        ss << str(token.token(), nest-1);
      }
      if (first) {
        // Its an empty tuple:
        // Add a `,` to make it different than ():
        ss << ",)";
      } else {
        ss << ")";
      }
      return ss.str();
    case MAP_Token:
      if (nest == 0) return "[Map]";
      tmap = &(static_cast<const TokenMap*>(base)->map());
      if (tmap->size() == 0) return "{}";
      ss << "{";
      for (m_it = tmap->begin(); m_it != tmap->end(); ++m_it) {
        ss << (m_it == tmap->begin() ? "" : ",");
        ss << " \"" << m_it->first << "\": " << m_it->second.str(nest-1);
      }
      ss << " }";
      return ss.str();
    case LIST_Token:
      if (nest == 0) return "[List]";
      tlist = &(static_cast<const TokenList*>(base)->list());
      if (tlist->size() == 0) return "[]";
      ss << "[";
      for (l_it = tlist->begin(); l_it != tlist->end(); ++l_it) {
        ss << (l_it == tlist->begin() ? "" : ",");
        ss << " " << l_it->str(nest-1);
      }
      ss << " ]";
      return ss.str();
    default:
      if (base->type & IT_Token) {
        return "[Iterator]";
      }
      return "unknown_type";
  }
}
