#ifndef SHUNTING_YARD_H_
#define SHUNTING_YARD_H_
#include <iostream>

#include <map>
#include <stack>
#include <string>
#include <queue>
#include <list>
#include <vector>
#include <set>
#include <sstream>
#include <memory>
#include <utility>
#include <deque>
#include <unordered_set>

namespace cparse {

/*
 * About tokType enum:
 *
 * The 3 left most bits (0x80, 0x40 and 0x20) of the Token Type
 * are reserved for denoting Numerals, Iterators and References.
 * If you want to define your own type please mind this bits.
 */
typedef uint8_t tokType_t;
typedef uint64_t opID_t;
enum tokType {
  // Internal types:
  NONE_Token, OP_Token, UNARY_Token, VAR_Token,

  // Base types:
  // Note: The mask system accepts at most 29 (32-3) different base types.
  STR_Token, FUNC_Token,

  // Numerals:
  NUM_Token = 0x20,   // Everything with the bit 0x20 set is a number.
  REAL_Token = 0x21,  // == 0x20 + 0x1 => Real numbers.
  INT_Token = 0x22,   // == 0x20 + 0x2 => Integral numbers.
  BOOL_Token = 0x23,  // == 0x20 + 0x3 => Boolean Type.
  POINT_Token = 0x24,

  // Complex types:
  IT_Token = 0x40,      // Everything with the bit 0x40 set are iterators.
  LIST_Token = 0x41,    // == 0x40 + 0x01 => Lists are iterators.
  TUPLE_Token = 0x42,   // == 0x40 + 0x02 => Tuples are iterators.
  STUPLE_Token = 0x43,  // == 0x40 + 0x03 => ArgTuples are iterators.
  MAP_Token = 0x44,     // == 0x40 + 0x04 => Maps are Iterators

  // References are internal tokens used by the calculator:
  REF_Token = 0x80,

  // Mask used when defining operations:
  ANY_TYPE_Token = 0xFF
};

#define ANY_OP ""

struct TokenBase {
  tokType_t type;

  virtual ~TokenBase() {}
  TokenBase() {}
  TokenBase(tokType_t type) : type(type) {}

  virtual TokenBase* clone() const = 0;
};

template<class T> class Token : public TokenBase {
 public:
  T val;
  Token(T t, tokType_t type) : TokenBase(type), val(t) {}
  virtual TokenBase* clone() const {
    return new Token(*this);
  }
};
  
struct TokenNone : public TokenBase {
  TokenNone() : TokenBase(NONE_Token) {}
  virtual TokenBase* clone() const {
    return new TokenNone(*this);
  }
};

struct TokenUnary : public TokenBase {
  TokenUnary() : TokenBase(UNARY_Token) {}
  virtual TokenBase* clone() const {
    return new TokenUnary(*this);
  }
};
  
class packToken;

// Adapt to std::queue<TokenBase*>
class TokenQueue_t: public std::deque<TokenBase*> {
public:
  void push(TokenBase* t) {
    push_back(t);
  }
  void pop() {
    pop_front();
  }
};


class OppMap_t {
  // Set of operators that should be evaluated from right to left:
  std::set<std::string> RtoL;
  // Map of operators precedence:
  std::map<std::string, int> pr_map;

 public:
  OppMap_t() {
    // These operations are hard-coded inside the calculator,
    // thus their precedence should always be defined:
    pr_map["[]"] = -1; pr_map["()"] = -1;
    pr_map["["] = 0x7FFFFFFF; pr_map["("] = 0x7FFFFFFF; pr_map["{"] = 0x7FFFFFFF;
    RtoL.insert("=");
  }

  void add(const std::string& op, int precedence) {
    if (precedence < 0) {
      RtoL.insert(op);
      precedence = -precedence;
    }

    pr_map[op] = precedence;
  }

  void addUnary(const std::string& op, int precedence) {
    add("L"+op, precedence);

    // Also add a binary operator with same precedence so
    // it is possible to verify if an op exists just by checking
    // the binary set of operators:
    if (!exists(op)) {
      add(op, precedence);
    }
  }

  void addRightUnary(const std::string& op, int precedence) {
    add("R"+op, precedence);

    // Also add a binary operator with same precedence so
    // it is possible to verify if an op exists just by checking
    // the binary set of operators:
    if (!exists(op)) {
      add(op, precedence);
    } else {
      // Note that using a unary and binary operators with
      // the same left operand is ambiguous and that the unary
      // operator will take precedence.
      //
      // So only do it if you know the expected left operands
      // have distinct types.
    }
  }

  int prec(const std::string& op) const { return pr_map.at(op); }
  bool assoc(const std::string& op) const { return RtoL.count(op); }
  bool exists(const std::string& op) const { return pr_map.count(op); }
};

struct TokenMap;
struct TokenList;
class Tuple;  
class STuple;  
class Function;  

  // Encapsulate TokenBase* into a friendlier interface
class packToken {
  TokenBase* base;

 public:
  static const packToken& None();

  typedef std::string (*strFunc_t)(const TokenBase*, uint32_t);
  static strFunc_t& str_custom();

 public:
  packToken() : base(new TokenNone()) {}
  packToken(const TokenBase& t) : base(t.clone()) {}
  packToken(const packToken& t) : base(t.base->clone()) {}
  packToken(packToken&& t) : base(t.base) { t.base = 0; }
  packToken& operator=(const packToken& t);

  template<class C>
  packToken(C c, tokType type) : base(new Token<C>(c, type)) {}
  packToken(int i) : base(new Token<int64_t>(i, INT_Token)) {}
  packToken(int64_t l) : base(new Token<int64_t>(l, INT_Token)) {}
  packToken(bool b) : base(new Token<uint8_t>(b, BOOL_Token)) {}
  packToken(size_t s) : base(new Token<int64_t>(s, INT_Token)) {}
  packToken(float f) : base(new Token<double>(f, REAL_Token)) {}
  packToken(double d) : base(new Token<double>(d, REAL_Token)) {}
  packToken(const void* p) : base(new Token<const void *>(p, POINT_Token)) {}
  packToken(const char* s) : base(new Token<std::string>(s, STR_Token)) {}
  packToken(const std::string& s) : base(new Token<std::string>(s, STR_Token)) {}
  packToken(const TokenMap& map);
  packToken(const TokenList& list);
  ~packToken() { delete base; }

  TokenBase* operator->() const;
  bool operator==(const packToken& t) const;
  bool operator!=(const packToken& t) const;
  packToken& operator[](const std::string& key);
  packToken& operator[](const char* key);
  const packToken& operator[](const std::string& key) const;
  const packToken& operator[](const char* key) const;
  TokenBase* token() { return base; }
  const TokenBase* token() const { return base; }

  bool asBool() const;
  double asDouble() const;
  int64_t asInt() const;
  std::string& asString() const;
  TokenMap& asMap() const;
  TokenList& asList() const;
  Tuple& asTuple() const;
  STuple& asSTuple() const;
  Function* asFunc() const;
  void *asPoint() const;

  // Specialize this template to your types, e.g.:
  // MyType& m = packToken.as<MyType>();
  template<typename T> T& as() const;

  // The nest argument defines how many times
  // it will recursively print nested structures:
  std::string str(uint32_t nest = 3) const;
  static std::string str(const TokenBase* t, uint32_t nest = 3);

 public:
  // This constructor makes sure the TokenBase*
  // will be deleted when the packToken destructor is called.
  //
  // If you still plan to use your TokenBase* use instead:
  //
  // - packToken(token->clone())
  //
  explicit packToken(TokenBase* t) : base(t) {}

 public:
  // Used to recover the original pointer.
  // The intance whose pointer was removed must be an rvalue.
  TokenBase* release() && {
    TokenBase* b = base;
    // Setting base to 0 leaves the class in an invalid state,
    // except for destruction.
    base = 0;
    return b;
  }
};

// To allow cout to print it:
std::ostream& operator<<(std::ostream& os, const packToken& t);
}  // namespace cparse

namespace cparse {

struct TokenMap;
struct TokenList;
class Tuple;
class STuple;
class Function;
struct TokenBase;  

}




// Define the `Function` class
// as well as some built-in functions:
#include <list>
#include <string>
#include<functional>

namespace cparse {


#pragma region Container
#include <map>
#include <list>
#include <vector>
#include <string>
#include <memory>
  
template <typename T>
struct Container {
 protected:
  std::shared_ptr<T> ref;

 public:
  Container() : ref(std::make_shared<T>()) {}
  Container(const T& t) : ref(std::make_shared<T>(t)) {}

 public:
  operator T*() const { return ref.get(); }
  friend bool operator==(Container<T> first, Container<T> second) {
    return first.ref == second.ref;
  }
};

struct Iterator;

struct Iterable : public TokenBase {
  virtual ~Iterable() {}
  Iterable() {}
  Iterable(tokType_t type) : TokenBase(type) {}

  virtual Iterator* getIterator() const = 0;
};

// Iterator super class.
struct Iterator : public Iterable {
  Iterator() : Iterable(IT_Token) {}
  virtual ~Iterator() {}
  // Return the next position of the iterator.
  // When it reaches the end it should return NULL
  // and reset the iterator automatically.
  virtual packToken* next() = 0;
  virtual void reset() = 0;

  Iterator* getIterator() const;
};

struct TokenMap;
typedef std::map<std::string, packToken> TokenMap_t;

struct MapData_t {
  TokenMap_t map;
  TokenMap* parent;
  MapData_t();
  MapData_t(TokenMap* p);
  MapData_t(const MapData_t& other);
  ~MapData_t();

  MapData_t& operator=(const MapData_t& other);
};

struct TokenMap : public Container<MapData_t>, public Iterable {
  // Static factories:
  static TokenMap empty;
  static TokenMap& base_map();
  static TokenMap& default_global();
  static packToken default_constructor(TokenMap scope);

 public:
  // Attribute getters for the `MapData_t` content:
  TokenMap_t& map() const { return ref->map; }
  TokenMap* parent() const { return ref->parent; }

 public:
  // Implement the Iterable Interface:
  struct MapIterator : public Iterator {
    const TokenMap_t& map;
    TokenMap_t::const_iterator it = map.begin();
    packToken last;

    MapIterator(const TokenMap_t& map) : map(map) {}

    packToken* next();
    void reset();

    TokenBase* clone() const {
      return new MapIterator(*this);
    }
  };

  Iterator* getIterator() const {
    return new MapIterator(map());
  }

 public:
  TokenMap(TokenMap* parent = &TokenMap::base_map())
          : Container(parent), Iterable(MAP_Token) {
    // For the TokenBase super class
    this->type = MAP_Token;
  }
  TokenMap(const TokenMap& other) : Container(other) {
    this->type = MAP_Token;
  }

  virtual ~TokenMap() {}

 public:
  // Implement the TokenBase abstract class
  TokenBase* clone() const {
    return new TokenMap(*this);
  }

 public:
  packToken* find(const std::string& key);
  const packToken* find(const std::string& key) const;
  TokenMap* findMap(const std::string& key);
  void assign(std::string key, TokenBase* value);
  void insert(std::string key, TokenBase* value);

  TokenMap getChild();

  packToken& operator[](const std::string& str);

  void erase(std::string key);
};

// Build a TokenMap which is a child of default_global()
struct GlobalScope : public TokenMap {
  GlobalScope() : TokenMap(&TokenMap::default_global()) {}
};

typedef std::vector<packToken> TokenList_t;

struct TokenList : public Container<TokenList_t>, public Iterable {
  static packToken default_constructor(TokenMap scope);

 public:
  // Attribute getter for the `TokenList_t` content:
  TokenList_t& list() const { return *ref; }

 public:
  struct ListIterator : public Iterator {
    TokenList_t* list;
    uint64_t i = 0;

    ListIterator(TokenList_t* list) : list(list) {}

    packToken* next();
    void reset();

    TokenBase* clone() const {
      return new ListIterator(*this);
    }
  };

  Iterator* getIterator() const {
    return new ListIterator(&list());
  }

 public:
  TokenList() { this->type = LIST_Token; }
  virtual ~TokenList() {}

  packToken& operator[](const uint64_t idx) const {
    if (list().size() <= idx) {
      // throw std::out_of_range("List index out of range!");
      return list()[0];
    }
    return list()[idx];
  }

  void push(packToken val) const { list().push_back(val); }
  packToken pop() const {
    packToken back = list().back();
    list().pop_back();
    return back;
  }

 public:
  // Implement the TokenBase abstract class
  TokenBase* clone() const {
    return new TokenList(*this);
  }
};

class Tuple : public TokenList {
 public:
  Tuple() { this->type = TUPLE_Token; }
  Tuple(const TokenBase* first) {
    this->type = TUPLE_Token;
    list().push_back(packToken(first->clone()));
  }
  Tuple(const packToken first) : Tuple(first.token()) {}

  Tuple(const TokenBase* first, const TokenBase* second) {
    this->type = TUPLE_Token;
    list().push_back(packToken(first->clone()));
    list().push_back(packToken(second->clone()));
  }
  Tuple(const packToken first, const packToken second)
       : Tuple(first.token(), second.token()) {}

 public:
  // Implement the TokenBase abstract class
  TokenBase* clone() const {
    return new Tuple(*this);
  }
};

// This Special Tuple is to be used only as syntactic sugar, and
// constructed only with the operator `:`, i.e.:
// - passing key-word arguments: func(1, 2, optional_arg:10)
// - slicing lists or strings: my_list[2:10:2] (not implemented)
//
// STuple means one of:
// - Special Tuple, Syntactic Tuple or System Tuple
//
// I haven't decided yet. Suggestions accepted.
class STuple : public Tuple {
 public:
  STuple() { this->type = STUPLE_Token; }
  STuple(const TokenBase* first) {
    this->type = STUPLE_Token;
    list().push_back(packToken(first->clone()));
  }
  STuple(const packToken first) : STuple(first.token()) {}

  STuple(const TokenBase* first, const TokenBase* second) {
    this->type = STUPLE_Token;
    list().push_back(packToken(first->clone()));
    list().push_back(packToken(second->clone()));
  }
  STuple(const packToken first, const packToken second)
       : STuple(first.token(), second.token()) {}

 public:
  // Implement the TokenBase abstract class
  TokenBase* clone() const {
    return new STuple(*this);
  }
};
#pragma endregion

// This struct was created to expose internal toRPN() variables
// to custom parsers, in special to the rWordParser_t functions.
struct rpnBuilder {
  TokenQueue_t rpn;
  std::stack<std::string> opStack;
  uint8_t lastTokenWasOp = true;
  bool lastTokenWasUnary = false;
  TokenMap scope;
  const OppMap_t& opp;

  // Used to make sure the expression won't
  // end inside a bracket evaluation just because
  // found a delimiter like '\n' or ')'
  uint32_t bracketLevel = 0;

  rpnBuilder(TokenMap scope, const OppMap_t& opp) : scope(scope), opp(opp) {}

 public:
  static void cleanRPN(TokenQueue_t* rpn);

 public:
  void handle_op(const std::string& op);
  void handle_token(TokenBase* token);
  void open_bracket(const std::string& bracket);
  void close_bracket(const std::string& bracket);

  // * * * * * Static parsing helpers: * * * * * //

  // Check if a character is the first character of a variable:
  static inline bool isvarchar(const char c) {
    return isalpha(c) || c == '_';
  }

  static inline std::string parseVar(const char* expr, const char** rest = 0) {
    std::stringstream ss;
    ss << *expr;
    ++expr;
    while (rpnBuilder::isvarchar(*expr) || isdigit(*expr)) {
      ss << *expr;
      ++expr;
    }
    if (rest) *rest = expr;
    return ss.str();
  }

 private:
  void handle_opStack(const std::string& op);
  void handle_binary(const std::string& op);
  void handle_left_unary(const std::string& op);
  void handle_right_unary(const std::string& op);
};

class RefToken;
struct opMap_t;
struct evaluationData {
  TokenQueue_t rpn;
  TokenMap scope;
  const opMap_t& opMap;

  std::unique_ptr<RefToken> left;
  std::unique_ptr<RefToken> right;

  std::string op;
  opID_t opID;

  evaluationData(TokenQueue_t rpn, const TokenMap &scope, const opMap_t& opMap)
    : rpn(rpn), scope(scope), opMap(opMap), opID(0)
  {
  }
};

// The reservedWordParser_t is the function type called when
// a reserved word or character is found at parsing time.
typedef void rWordParser_t(const char* expr, const char** rest,
                           rpnBuilder* data);
typedef std::map<std::string, rWordParser_t*> rWordMap_t;
typedef std::map<char, rWordParser_t*> rCharMap_t;

struct parserMap_t {
  rWordMap_t wmap;
  rCharMap_t cmap;

  // Add reserved word:
  void add(const std::string& word, rWordParser_t* parser) {
    wmap[word] = parser;
  }

  // Add reserved character:
  void add(char c, rWordParser_t* parser) {
    cmap[c] = parser;
  }

  rWordParser_t* find(const std::string text) const {
    const auto w_it = wmap.find(text);
    if (w_it != wmap.end()) {
      return w_it->second;
    }
    return nullptr;
  }

  rWordParser_t* find(char c) const {
    const rCharMap_t::const_iterator c_it = cmap.find(c);
    if (c_it != cmap.end()) {
      return c_it->second;
    }
    return nullptr;
  }
};

// The RefToken keeps information about the context
// in which a variable was originally evaluated
// and allow a final value to be correctly resolved
// afterwards.
class RefToken : public TokenBase {
  packToken original_value;

 public:
  packToken key;
  packToken origin;
  RefToken(packToken k, TokenBase* v, packToken m = packToken::None()) :
    TokenBase(v->type | REF_Token), original_value(v), key(std::forward<packToken>(k)), origin(std::forward<packToken>(m)) {}
  RefToken(packToken k = packToken::None(), packToken v = packToken::None(), packToken m = packToken::None()) :
    TokenBase(v->type | REF_Token), original_value(std::forward<packToken>(v)), key(std::forward<packToken>(k)), origin(std::forward<packToken>(m)) {}

  TokenBase* resolve(TokenMap* localScope = 0) const {
    TokenBase* result = 0;

    // Local variables have no origin == NONE,
    // thus, require a localScope to be resolved:
    if (origin->type == NONE_Token && localScope) {
      // Get the most recent value from the local scope:
      packToken* r_value = localScope->find(key.asString());
      if (r_value) {
        result = (*r_value)->clone();
      }
    }

    // In last case return the compilation-time value:
    return result ? result : original_value->clone();
  }

  virtual TokenBase* clone() const {
    return new RefToken(*this);
  }
};

struct opSignature_t {
  tokType_t left; std::string op; tokType_t right;
  opSignature_t(const tokType_t L, const std::string op, const tokType_t R)
               : left(L), op(op), right(R) {}
};

class Operation {
 public:
  typedef packToken (*opFunc_t)(const packToken& left, const packToken& right,
                                evaluationData* data);

 public:
  // Use this exception to reject an operation.
  // Without stoping the operation matching process.
  // struct Reject : public std::exception {};

 public:
  static inline uint32_t mask(tokType_t type);
  static opID_t build_mask(tokType_t left, tokType_t right);

 private:
  opID_t _mask;
  opFunc_t _exec;

 public:
  Operation(opSignature_t sig, opFunc_t func)
           : _mask(build_mask(sig.left, sig.right)), _exec(func) {}

 public:
  opID_t getMask() const { return _mask; }
  packToken exec(const packToken& left, const packToken& right,
                 evaluationData* data) const {
    return _exec(left, right, data);
  }
};

typedef std::map<tokType_t, TokenMap> typeMap_t;
typedef std::vector<Operation> opList_t;
struct opMap_t : public std::map<std::string, opList_t> {
  void add(const opSignature_t sig, Operation::opFunc_t func) {
    (*this)[sig.op].push_back(Operation(sig, func));
  }

  std::string str() const {
    if (this->size() == 0) return "{}";

    std::string result = "{ ";
    for (const auto& pair : (*this)) {
      result += "\"" + pair.first + "\", ";
    }
    result.pop_back();
    result.pop_back();
    return result + " }";
  }
};

struct Config_t {
  parserMap_t parserMap;
  OppMap_t opPrecedence;
  opMap_t opMap;

  Config_t() {}
  Config_t(parserMap_t p, OppMap_t opp, opMap_t opMap)
          : parserMap(p), opPrecedence(opp), opMap(opMap) {}
};

class calculator {
 public:
  static Config_t& Default();

 public:
  static typeMap_t& type_attribute_map();

 public:
  static packToken calculate(const char* expr, const TokenMap &vars = TokenMap::empty,
                             const char* delim = 0, const char** rest = 0);

 public:
  static TokenBase* calculate(const TokenQueue_t& RPN, const TokenMap &scope,
                              const Config_t& config = Default());
  static TokenQueue_t toRPN(const char* expr, const TokenMap &vars,
                            const char* delim = 0, const char** rest = 0,
                            const Config_t &config = Default());

 public:
  // Used to dealloc a TokenQueue_t safely.
  struct RAII_TokenQueue_t;

 protected:
  virtual const Config_t& Config() const { return Default(); }

 private:
  TokenQueue_t RPN;

 public:
  virtual ~calculator();
  calculator() { this->RPN.push(new TokenNone()); }
  calculator(const calculator& calc);
  calculator(const char* expr, TokenMap vars = &TokenMap::empty,
             const char* delim = 0, const char** rest = 0,
             const Config_t& config = Default());
  void compile(const char* expr, TokenMap &vars = TokenMap::empty,
               const char* delim = 0, const char** rest = 0);
  packToken eval(const TokenMap &vars = TokenMap::empty, bool keep_refs = false) const;
  std::unordered_set<std::string> get_variables() const;

  // Serialization:
  std::string str() const;
  static std::string str(TokenQueue_t rpn);

  // Operators:
  calculator& operator=(const calculator& calc);
};

#pragma region Function
#include <list>
#include <string>
#include<functional>
  typedef std::list<std::string> args_t;
  class packToken;

  class Function : public TokenBase {
  public:
    static packToken call(packToken _this, const Function* func,
                          TokenList* args, TokenMap &scope);
  public:
    Function() : TokenBase(FUNC_Token) {}
    virtual ~Function() {}

  public:
    virtual const std::string name() const = 0;
    virtual const args_t args() const = 0;
    virtual packToken exec(TokenMap &scope) const = 0;
    virtual TokenBase* clone() const = 0;
  };

  class CppFunction : public Function {
  public:
    packToken (*func)(TokenMap);
    std::function<packToken(TokenMap)> stdFunc;
    args_t _args;
    std::string _name;
    bool isStdFunc;

    CppFunction();
    CppFunction(packToken (*func)(TokenMap), const args_t args,
                std::string name = "");
    CppFunction(packToken (*func)(TokenMap), unsigned int nargs,
                const char** args, std::string name = "");
    CppFunction(packToken (*func)(TokenMap), std::string name = "");
    CppFunction(std::function<packToken(TokenMap)> func, const args_t args,
                std::string name = "");
    CppFunction(const args_t args, std::function<packToken(TokenMap)> func,
                std::string name = "");
    CppFunction(std::function<packToken(TokenMap)> func, unsigned int nargs,
                const char** args, std::string name = "");
    CppFunction(std::function<packToken(TokenMap)> func, std::string name = "");

    virtual const std::string name() const { return _name; }
    virtual const args_t args() const { return _args; }
    virtual packToken exec(TokenMap &scope) const { return isStdFunc ? stdFunc(scope) : func(scope); }

    virtual TokenBase* clone() const {
      return new CppFunction(static_cast<const CppFunction&>(*this));
    }
  };

#pragma endregion  
}  // namespace cparse

#endif  // SHUNTING_YARD_H_
