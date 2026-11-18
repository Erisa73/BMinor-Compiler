#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <cctype>
#include <algorithm>

using namespace std;

// -----------------------------------------------------------------------------
// 1. Source positions and tokens
// -----------------------------------------------------------------------------
struct SourceLocation {
    string filename;
    int line = 1, col = 1, offset = 0;
};

enum class TokenType {
    KW_ARRAY, KW_AUTO, KW_ELSE, KW_FOR, KW_FUNCTION, KW_IF, KW_PRINT,
    KW_RETURN, KW_VOID, KW_WHILE, TYPE_INTEGER, TYPE_BOOLEAN, TYPE_CHAR,
    TYPE_STRING, BOOL_LIT, INT_LIT, CHAR_LIT, STRING_LIT, IDENT,
    PLUS, MINUS, STAR, SLASH, PERCENT, CARET, INC, DEC, LT, LE, GT, GE,
    EQ, NEQ, AND, OR, NOT, ASSIGN, LBRACE, RBRACE, LPAREN, RPAREN,
    LBRACKET, RBRACKET, SEMI, COMMA, COLON, COMMENT, INVALID, END_OF_FILE
};

struct Token {
    TokenType type;
    string lexeme;
    SourceLocation loc;
};

string tokenTypeToString(TokenType t) {
    switch (t) {
        case TokenType::KW_ARRAY: return "KW(array)";
        case TokenType::KW_AUTO: return "KW(auto)";
        case TokenType::KW_ELSE: return "KW(else)";
        case TokenType::KW_FOR: return "KW(for)";
        case TokenType::KW_FUNCTION: return "KW(function)";
        case TokenType::KW_IF: return "KW(if)";
        case TokenType::KW_PRINT: return "KW(print)";
        case TokenType::KW_RETURN: return "KW(return)";
        case TokenType::KW_VOID: return "KW(void)";
        case TokenType::KW_WHILE: return "KW(while)";
        case TokenType::TYPE_INTEGER: return "KW(integer)";
        case TokenType::TYPE_BOOLEAN: return "KW(boolean)";
        case TokenType::TYPE_CHAR: return "KW(char)";
        case TokenType::TYPE_STRING: return "KW(string)";
        case TokenType::BOOL_LIT: return "BOOL";
        case TokenType::INT_LIT: return "INT";
        case TokenType::CHAR_LIT: return "CHAR";
        case TokenType::STRING_LIT: return "STRING";
        case TokenType::IDENT: return "IDENT";
        case TokenType::PLUS: case TokenType::MINUS: case TokenType::STAR:
        case TokenType::SLASH: case TokenType::PERCENT: case TokenType::CARET:
        case TokenType::INC: case TokenType::DEC: case TokenType::LT:
        case TokenType::LE: case TokenType::GT: case TokenType::GE:
        case TokenType::EQ: case TokenType::NEQ: case TokenType::AND:
        case TokenType::OR: case TokenType::NOT: return "OP";
        case TokenType::ASSIGN: return "ASSIGN";
        case TokenType::LBRACE: return "LBRACE";
        case TokenType::RBRACE: return "RBRACE";
        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::LBRACKET: return "LBRACKET";
        case TokenType::RBRACKET: return "RBRACKET";
        case TokenType::SEMI: return "SEMI";
        case TokenType::COMMA: return "COMMA";
        case TokenType::COLON: return "COLON";
        case TokenType::COMMENT: return "COMMENT";
        case TokenType::INVALID: return "INVALID";
        case TokenType::END_OF_FILE: return "EOF";
    }
    return "UNKNOWN";
}

// -----------------------------------------------------------------------------
// 2. Lexer
// -----------------------------------------------------------------------------
class Lexer {
    string filename, source;
    size_t pos = 0;
    int line = 1, col = 1;
    vector<string> errors;

    const unordered_map<string, TokenType> keywords = {
        {"array", TokenType::KW_ARRAY}, {"auto", TokenType::KW_AUTO},
        {"else", TokenType::KW_ELSE}, {"for", TokenType::KW_FOR},
        {"function", TokenType::KW_FUNCTION}, {"if", TokenType::KW_IF},
        {"print", TokenType::KW_PRINT}, {"return", TokenType::KW_RETURN},
        {"void", TokenType::KW_VOID}, {"while", TokenType::KW_WHILE},
        {"integer", TokenType::TYPE_INTEGER}, {"boolean", TokenType::TYPE_BOOLEAN},
        {"char", TokenType::TYPE_CHAR}, {"string", TokenType::TYPE_STRING},
        {"true", TokenType::BOOL_LIT}, {"false", TokenType::BOOL_LIT}
    };

    char peek(int n = 0) const {
        return pos + n < source.size() ? source[pos + n] : '\0';
    }

    char advance() {
        char c = peek();
        if (c == '\0') return c;
        ++pos;
        if (c == '\n') { ++line; col = 1; }
        else ++col;
        return c;
    }

    SourceLocation location() const {
        return {filename, line, col, static_cast<int>(pos)};
    }

    void error(const SourceLocation& loc, const string& message) {
        ostringstream out;
        out << "[Lexical Error] " << loc.filename << ':' << loc.line << ':'
            << loc.col << ": " << message;
        errors.push_back(out.str());
    }

public:
    Lexer(string file, string text) : filename(move(file)), source(move(text)) {}
    const vector<string>& getErrors() const { return errors; }

    vector<Token> tokenizeAll() {
        vector<Token> tokens;
        const unordered_map<string, TokenType> pairs = {
            {"<=", TokenType::LE}, {">=", TokenType::GE}, {"==", TokenType::EQ},
            {"!=", TokenType::NEQ}, {"&&", TokenType::AND}, {"||", TokenType::OR},
            {"++", TokenType::INC}, {"--", TokenType::DEC}
        };
        const unordered_map<char, TokenType> singles = {
            {'+', TokenType::PLUS}, {'-', TokenType::MINUS}, {'*', TokenType::STAR},
            {'/', TokenType::SLASH}, {'%', TokenType::PERCENT}, {'^', TokenType::CARET},
            {'<', TokenType::LT}, {'>', TokenType::GT}, {'=', TokenType::ASSIGN},
            {'!', TokenType::NOT}, {'{', TokenType::LBRACE}, {'}', TokenType::RBRACE},
            {'(', TokenType::LPAREN}, {')', TokenType::RPAREN}, {'[', TokenType::LBRACKET},
            {']', TokenType::RBRACKET}, {';', TokenType::SEMI}, {',', TokenType::COMMA},
            {':', TokenType::COLON}
        };

        while (pos < source.size()) {
            char c = peek();
            if (isspace(static_cast<unsigned char>(c))) { advance(); continue; }

            SourceLocation start = location();
            string text;
            auto add = [&](TokenType type) { tokens.push_back({type, text, start}); };

            // Line and block comments
            if (c == '/' && peek(1) == '/') {
                while (peek() && peek() != '\n') text += advance();
                add(TokenType::COMMENT);
                continue;
            }
            if (c == '/' && peek(1) == '*') {
                text += advance(); text += advance();
                bool closed = false;
                while (peek()) {
                    if (peek() == '*' && peek(1) == '/') {
                        text += advance(); text += advance();
                        closed = true;
                        break;
                    }
                    text += advance();
                }
                if (closed) add(TokenType::COMMENT);
                else { error(start, "Unterminated block comment"); add(TokenType::INVALID); }
                continue;
            }

            // Identifiers and keywords
            if (isalpha(static_cast<unsigned char>(c)) || c == '_') {
                do { text += advance(); }
                while (isalnum(static_cast<unsigned char>(peek())) || peek() == '_');
                auto found = keywords.find(text);
                add(found == keywords.end() ? TokenType::IDENT : found->second);
                continue;
            }

            // Integer literals
            if (isdigit(static_cast<unsigned char>(c))) {
                do { text += advance(); }
                while (isdigit(static_cast<unsigned char>(peek())));
                add(TokenType::INT_LIT);
                continue;
            }

            // String literals, including escaped characters
            if (c == '"') {
                text += advance();
                bool closed = false;
                while (peek() && peek() != '\n') {
                    if (peek() == '\\') {
                        text += advance();
                        if (peek() && peek() != '\n') text += advance();
                    } else if (peek() == '"') {
                        text += advance(); closed = true; break;
                    } else text += advance();
                }
                if (closed) add(TokenType::STRING_LIT);
                else { error(start, "Unterminated string literal"); add(TokenType::INVALID); }
                continue;
            }

            // Character literals
            if (c == '\'') {
                text += advance();
                if (peek() == '\\') {
                    text += advance();
                    if (peek() && peek() != '\n') text += advance();
                } else if (peek() && peek() != '\n' && peek() != '\'') text += advance();
                if (peek() == '\'') { text += advance(); add(TokenType::CHAR_LIT); }
                else { error(start, "Unterminated or invalid character literal"); add(TokenType::INVALID); }
                continue;
            }

            // Longest match for two-character operators, then single characters
            string two{c, peek(1)};
            if (auto found = pairs.find(two); found != pairs.end()) {
                text += advance(); text += advance(); add(found->second);
                continue;
            }
            advance();
            text = string(1, c);
            if (auto found = singles.find(c); found != singles.end()) add(found->second);
            else { error(start, "Invalid character '" + text + "'"); add(TokenType::INVALID); }
        }

        tokens.push_back({TokenType::END_OF_FILE, "", location()});
        return tokens;
    }
};

// -----------------------------------------------------------------------------
// 3. One flexible AST node replaces the many repetitive node classes
// -----------------------------------------------------------------------------
enum class NodeKind {
    Program, Type, Identifier, Integer, Boolean, String, Character, Binary,
    Unary, Call, ArrayAccess, Block, Return, If, While, For, Print,
    Variable, Parameter, Function
};

string nodeKindName(NodeKind kind) {
    switch (kind) {
        case NodeKind::Program: return "Program";
        case NodeKind::Type: return "TypeNode";
        case NodeKind::Identifier: return "Identifier";
        case NodeKind::Integer: return "IntLiteral";
        case NodeKind::Boolean: return "BoolLiteral";
        case NodeKind::String: return "StringLiteral";
        case NodeKind::Character: return "CharLiteral";
        case NodeKind::Binary: return "BinaryExpr";
        case NodeKind::Unary: return "UnaryExpr";
        case NodeKind::Call: return "CallExpr";
        case NodeKind::ArrayAccess: return "ArrayAccess";
        case NodeKind::Block: return "BlockStmt";
        case NodeKind::Return: return "ReturnStmt";
        case NodeKind::If: return "IfStmt";
        case NodeKind::While: return "WhileStmt";
        case NodeKind::For: return "ForStmt";
        case NodeKind::Print: return "PrintStmt";
        case NodeKind::Variable: return "VarDecl";
        case NodeKind::Parameter: return "Param";
        case NodeKind::Function: return "FuncDecl";
    }
    return "Node";
}

struct ASTNode {
    NodeKind kind;
    SourceLocation loc, nameLoc;
    string value;
    string resolvedType; // filled later by semantic analysis
    vector<pair<string, unique_ptr<ASTNode>>> children;

    ASTNode(NodeKind k, SourceLocation l, string v = "")
        : kind(k), loc(move(l)), value(move(v)) {}

    ASTNode* add(string label, unique_ptr<ASTNode> child) {
        if (child) children.emplace_back(move(label), move(child));
        return this;
    }

    void print(int indent = 0) const {
        cout << string(indent * 2, ' ') << nodeKindName(kind);
        if (!value.empty()) cout << "(" << value << ')';
        if (kind != NodeKind::Program)
            cout << " [loc=" << loc.line << ':' << loc.col << ']';
        cout << '\n';
        for (const auto& [label, child] : children) {
            if (!label.empty()) cout << string((indent + 1) * 2, ' ') << label << ":\n";
            child->print(indent + (label.empty() ? 1 : 2));
        }
    }
};

using NodePtr = unique_ptr<ASTNode>;
NodePtr makeNode(NodeKind kind, const SourceLocation& loc, string value = "") {
    return make_unique<ASTNode>(kind, loc, move(value));
}

// -----------------------------------------------------------------------------
// 4. Recursive-descent parser with panic-mode recovery
// -----------------------------------------------------------------------------
class Parser {
    vector<Token> tokens;
    size_t current = 0;
    vector<string> errors;

    const Token& peek(int offset = 0) const {
        return tokens[min(current + static_cast<size_t>(offset), tokens.size() - 1)];
    }
    const Token& advance() {
        if (current < tokens.size() - 1) ++current;
        return tokens[current - 1];
    }
    bool check(TokenType type) const { return peek().type == type; }
    bool match(TokenType type) {
        if (!check(type)) return false;
        advance(); return true;
    }
    void error(const Token& token, const string& message) {
        ostringstream out;
        out << "[Syntax Error] " << token.loc.filename << ':' << token.loc.line
            << ':' << token.loc.col << ": " << message << " (got '"
            << token.lexeme << "')";
        errors.push_back(out.str());
    }

    void synchronize() {
        while (!check(TokenType::END_OF_FILE)) {
            if (match(TokenType::SEMI)) return;
            switch (peek().type) {
                case TokenType::RBRACE: case TokenType::KW_IF:
                case TokenType::KW_FOR: case TokenType::KW_WHILE:
                case TokenType::KW_RETURN: case TokenType::KW_PRINT:
                    return;
                default: advance();
            }
        }
    }

    NodePtr parseType() {
        Token token = peek();
        switch (token.type) {
            case TokenType::TYPE_INTEGER: case TokenType::TYPE_BOOLEAN:
            case TokenType::TYPE_CHAR: case TokenType::TYPE_STRING:
            case TokenType::KW_VOID: case TokenType::KW_AUTO:
                advance();
                return makeNode(NodeKind::Type, token.loc, token.lexeme);
            case TokenType::KW_ARRAY: {
                advance();
                auto type = makeNode(NodeKind::Type, token.loc, "array");
                if (!match(TokenType::LBRACKET)) error(peek(), "Expected '[' after 'array'");
                else {
                    if (!check(TokenType::RBRACKET)) type->add("size", parseExpr());
                    if (!match(TokenType::RBRACKET)) error(peek(), "Expected ']' after array size");
                }
                type->add("element type", parseType());
                return type;
            }
            default:
                error(token, "Expected type specifier");
                return nullptr;
        }
    }

    NodePtr parseDeclarationOrStatement() {
        // B-Minor declarations use: name: type ...
        if (check(TokenType::IDENT) && peek(1).type == TokenType::COLON) {
            Token name = advance(); advance(); // identifier and ':'

            if (match(TokenType::KW_FUNCTION)) {
                auto function = makeNode(NodeKind::Function, name.loc, name.lexeme);
                function->nameLoc = name.loc;
                function->add("return type", parseType());
                if (!match(TokenType::LPAREN)) error(peek(), "Expected '(' in function declaration");
                else {
                    if (!check(TokenType::RPAREN)) {
                        do {
                            if (!check(TokenType::IDENT)) {
                                error(peek(), "Expected parameter identifier");
                                break;
                            }
                            Token paramName = advance();
                            if (!match(TokenType::COLON)) error(peek(), "Expected ':' after parameter name");
                            auto param = makeNode(NodeKind::Parameter, paramName.loc, paramName.lexeme);
                            param->nameLoc = paramName.loc;
                            param->add("type", parseType());
                            function->add("parameter", move(param));
                        } while (match(TokenType::COMMA));
                    }
                    if (!match(TokenType::RPAREN)) error(peek(), "Expected ')' after parameter list");
                }
                if (match(TokenType::ASSIGN)) function->add("body", parseBlock());
                else if (!match(TokenType::SEMI)) error(peek(), "Expected '= {' or ';' after function header");
                return function;
            }

            auto variable = makeNode(NodeKind::Variable, name.loc, name.lexeme);
            variable->nameLoc = name.loc;
            variable->add("type", parseType());
            if (match(TokenType::ASSIGN)) {
                auto init = parseExpr();
                if (!init) { synchronize(); return nullptr; }
                variable->add("init", move(init));
            }
            if (!match(TokenType::SEMI)) {
                error(peek(), "Expected ';' after variable declaration");
                synchronize();
            }
            return variable;
        }
        return parseStatement();
    }

    NodePtr parseBlock() {
        Token start = peek();
        if (!match(TokenType::LBRACE)) {
            error(peek(), "Expected '{' to start block");
            synchronize();
            return nullptr;
        }
        auto block = makeNode(NodeKind::Block, start.loc);
        while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
            size_t before = current;
            auto statement = parseDeclarationOrStatement();
            block->add("", move(statement));
            if (current == before) advance(); // prevent endless recovery loops
        }
        if (!match(TokenType::RBRACE)) error(peek(), "Expected '}' at end of block");
        return block;
    }

    NodePtr parseStatement() {
        Token start = peek();
        if (check(TokenType::LBRACE)) return parseBlock();

        if (match(TokenType::KW_IF)) {
            if (!match(TokenType::LPAREN)) error(peek(), "Expected '(' after 'if'");
            auto node = makeNode(NodeKind::If, start.loc);
            node->add("condition", parseExpr());
            if (!match(TokenType::RPAREN)) error(peek(), "Missing ')' after if-condition");
            node->add("then", parseStatement());
            if (match(TokenType::KW_ELSE)) node->add("else", parseStatement());
            return node;
        }
        if (match(TokenType::KW_WHILE)) {
            if (!match(TokenType::LPAREN)) error(peek(), "Expected '(' after 'while'");
            auto node = makeNode(NodeKind::While, start.loc);
            node->add("condition", parseExpr());
            if (!match(TokenType::RPAREN)) error(peek(), "Missing ')' after while-condition");
            node->add("body", parseStatement());
            return node;
        }
        if (match(TokenType::KW_FOR)) {
            if (!match(TokenType::LPAREN)) error(peek(), "Expected '(' after 'for'");
            auto node = makeNode(NodeKind::For, start.loc);
            if (!check(TokenType::SEMI)) node->add("init", parseExpr());
            if (!match(TokenType::SEMI)) error(peek(), "Expected ';' after for-init");
            if (!check(TokenType::SEMI)) node->add("condition", parseExpr());
            if (!match(TokenType::SEMI)) error(peek(), "Expected ';' after for-condition");
            if (!check(TokenType::RPAREN)) node->add("update", parseExpr());
            if (!match(TokenType::RPAREN)) error(peek(), "Expected ')' after for-update");
            node->add("body", parseStatement());
            return node;
        }
        if (match(TokenType::KW_RETURN)) {
            auto node = makeNode(NodeKind::Return, start.loc);
            if (!check(TokenType::SEMI)) node->add("value", parseExpr());
            if (!match(TokenType::SEMI)) {
                error(peek(), "Expected ';' after return statement"); synchronize();
            }
            return node;
        }
        if (match(TokenType::KW_PRINT)) {
            auto node = makeNode(NodeKind::Print, start.loc);
            if (!check(TokenType::SEMI)) {
                do { node->add("argument", parseExpr()); }
                while (match(TokenType::COMMA));
            }
            if (!match(TokenType::SEMI)) {
                error(peek(), "Expected ';' after print statement"); synchronize();
            }
            return node;
        }
        if (match(TokenType::SEMI)) return nullptr; // empty statement

        auto expression = parseExpr();
        if (!expression) { synchronize(); return nullptr; }
        if (!match(TokenType::SEMI)) {
            error(peek(), "Expected ';' after expression"); synchronize();
        }
        return expression;
    }

    // Expression precedence, from lowest to highest.
    NodePtr binary(NodePtr left, const Token& op, NodePtr right) {
        auto node = makeNode(NodeKind::Binary, op.loc, op.lexeme);
        node->add("left", move(left));
        node->add("right", move(right));
        return node;
    }
    NodePtr parseExpr() { return parseAssignment(); }

    NodePtr parseAssignment() {
        auto left = parseLogicalOr();
        if (match(TokenType::ASSIGN)) {
            Token op = tokens[current - 1];
            return binary(move(left), op, parseAssignment()); // right associative
        }
        return left;
    }
    NodePtr parseLogicalOr() {
        auto left = parseLogicalAnd();
        while (match(TokenType::OR)) {
            Token op = tokens[current - 1];
            left = binary(move(left), op, parseLogicalAnd());
        }
        return left;
    }
    NodePtr parseLogicalAnd() {
        auto left = parseEquality();
        while (match(TokenType::AND)) {
            Token op = tokens[current - 1];
            left = binary(move(left), op, parseEquality());
        }
        return left;
    }
    NodePtr parseEquality() {
        auto left = parseRelational();
        while (check(TokenType::EQ) || check(TokenType::NEQ)) {
            Token op = advance();
            left = binary(move(left), op, parseRelational());
        }
        return left;
    }
    NodePtr parseRelational() {
        auto left = parseAdditive();
        while (check(TokenType::LT) || check(TokenType::LE) ||
               check(TokenType::GT) || check(TokenType::GE)) {
            Token op = advance();
            left = binary(move(left), op, parseAdditive());
        }
        return left;
    }
    NodePtr parseAdditive() {
        auto left = parseMultiplicative();
        while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
            Token op = advance();
            left = binary(move(left), op, parseMultiplicative());
        }
        return left;
    }
    NodePtr parseMultiplicative() {
        auto left = parseExponent();
        while (check(TokenType::STAR) || check(TokenType::SLASH) || check(TokenType::PERCENT)) {
            Token op = advance();
            left = binary(move(left), op, parseExponent());
        }
        return left;
    }
    NodePtr parseExponent() {
        auto left = parseUnary();
        if (match(TokenType::CARET)) {
            Token op = tokens[current - 1];
            return binary(move(left), op, parseExponent()); // right associative
        }
        return left;
    }
    NodePtr parseUnary() {
        if (check(TokenType::MINUS) || check(TokenType::NOT)) {
            Token op = advance();
            auto node = makeNode(NodeKind::Unary, op.loc, op.lexeme);
            node->add("operand", parseUnary());
            return node;
        }
        return parsePostfix();
    }
    NodePtr parsePostfix() {
        auto node = parsePrimary();
        if (!node) return nullptr;
        while (true) {
            if (match(TokenType::LPAREN)) {
                SourceLocation nameLoc = node->loc;
                string name = node->kind == NodeKind::Identifier ? node->value : "<anonymous>";
                auto call = makeNode(NodeKind::Call, nameLoc, name);
                call->nameLoc = nameLoc;
                if (!check(TokenType::RPAREN)) {
                    do { call->add("argument", parseExpr()); }
                    while (match(TokenType::COMMA));
                }
                if (!match(TokenType::RPAREN)) error(peek(), "Expected ')' after arguments in function call");
                node = move(call);
            } else if (match(TokenType::LBRACKET)) {
                Token bracket = tokens[current - 1];
                auto access = makeNode(NodeKind::ArrayAccess, bracket.loc);
                access->add("array", move(node));
                access->add("index", parseExpr());
                if (!match(TokenType::RBRACKET)) error(peek(), "Expected ']' after array index");
                node = move(access);
            } else if (check(TokenType::INC) || check(TokenType::DEC)) {
                Token op = advance();
                auto unary = makeNode(NodeKind::Unary, op.loc, op.lexeme);
                unary->add("operand", move(node));
                node = move(unary);
            } else break;
        }
        return node;
    }
    NodePtr parsePrimary() {
        Token token = peek();
        switch (token.type) {
            case TokenType::INT_LIT: advance(); return makeNode(NodeKind::Integer, token.loc, token.lexeme);
            case TokenType::BOOL_LIT: advance(); return makeNode(NodeKind::Boolean, token.loc, token.lexeme);
            case TokenType::STRING_LIT: advance(); return makeNode(NodeKind::String, token.loc, token.lexeme);
            case TokenType::CHAR_LIT: advance(); return makeNode(NodeKind::Character, token.loc, token.lexeme);
            case TokenType::IDENT: advance(); return makeNode(NodeKind::Identifier, token.loc, token.lexeme);
            case TokenType::LPAREN: {
                advance();
                auto expression = parseExpr();
                if (!match(TokenType::RPAREN)) error(peek(), "Expected ')' after grouped expression");
                return expression;
            }
            default:
                error(token, "Expected expression");
                return nullptr;
        }
    }

public:
    explicit Parser(const vector<Token>& allTokens) {
        for (const auto& token : allTokens)
            if (token.type != TokenType::COMMENT && token.type != TokenType::INVALID)
                tokens.push_back(token);
        if (tokens.empty() || tokens.back().type != TokenType::END_OF_FILE)
            tokens.push_back({TokenType::END_OF_FILE, "", {"", 1, 1, 0}});
    }

    const vector<string>& getErrors() const { return errors; }

    NodePtr parseProgram() {
        auto program = makeNode(NodeKind::Program, peek().loc);
        while (!check(TokenType::END_OF_FILE)) {
            size_t before = current;
            program->add("", parseDeclarationOrStatement());
            if (current == before) advance(); // keep recovery moving
        }
        return program;
    }
};

// -----------------------------------------------------------------------------
// 5. AST-based syntax highlighter (ANSI terminal and HTML/CSS)
// -----------------------------------------------------------------------------
enum class HighlightCategory {
    KEYWORD, TYPE_NAME, VARIABLE, FUNCTION, INTEGER, STRING_CHAR,
    BOOLEAN, OPERATOR, COMMENT, ERROR
};

class SyntaxHighlighter {
    unordered_set<int> functionOffsets;

    void collectFunctionNames(const ASTNode* node) {
        if (!node) return;
        if (node->kind == NodeKind::Function || node->kind == NodeKind::Call)
            functionOffsets.insert(node->nameLoc.offset);
        for (const auto& [label, child] : node->children)
            collectFunctionNames(child.get());
    }

    HighlightCategory classify(const Token& token) const {
        switch (token.type) {
            case TokenType::KW_ARRAY: case TokenType::KW_AUTO: case TokenType::KW_ELSE:
            case TokenType::KW_FOR: case TokenType::KW_FUNCTION: case TokenType::KW_IF:
            case TokenType::KW_PRINT: case TokenType::KW_RETURN: case TokenType::KW_VOID:
            case TokenType::KW_WHILE: return HighlightCategory::KEYWORD;
            case TokenType::TYPE_INTEGER: case TokenType::TYPE_BOOLEAN:
            case TokenType::TYPE_CHAR: case TokenType::TYPE_STRING:
                return HighlightCategory::TYPE_NAME;
            case TokenType::IDENT:
                return functionOffsets.count(token.loc.offset)
                    ? HighlightCategory::FUNCTION : HighlightCategory::VARIABLE;
            case TokenType::INT_LIT: return HighlightCategory::INTEGER;
            case TokenType::BOOL_LIT: return HighlightCategory::BOOLEAN;
            case TokenType::STRING_LIT: case TokenType::CHAR_LIT:
                return HighlightCategory::STRING_CHAR;
            case TokenType::COMMENT: return HighlightCategory::COMMENT;
            case TokenType::INVALID: return HighlightCategory::ERROR;
            default: return HighlightCategory::OPERATOR;
        }
    }

    static string ansiCode(HighlightCategory category) {
        switch (category) {
            case HighlightCategory::KEYWORD: return "\033[1;34m";
            case HighlightCategory::TYPE_NAME: return "\033[36m";
            case HighlightCategory::VARIABLE: return "\033[37m";
            case HighlightCategory::FUNCTION: return "\033[1;33m";
            case HighlightCategory::INTEGER: case HighlightCategory::BOOLEAN: return "\033[38;5;208m";
            case HighlightCategory::STRING_CHAR: return "\033[32m";
            case HighlightCategory::OPERATOR: return "\033[90m";
            case HighlightCategory::COMMENT: return "\033[2;3;37m";
            case HighlightCategory::ERROR: return "\033[4;31m";
        }
        return "\033[0m";
    }

    static string cssClass(HighlightCategory category) {
        switch (category) {
            case HighlightCategory::KEYWORD: return "kw";
            case HighlightCategory::TYPE_NAME: return "type";
            case HighlightCategory::VARIABLE: return "var";
            case HighlightCategory::FUNCTION: return "fn";
            case HighlightCategory::INTEGER: return "lit";
            case HighlightCategory::BOOLEAN: return "bool";
            case HighlightCategory::STRING_CHAR: return "str";
            case HighlightCategory::OPERATOR: return "op";
            case HighlightCategory::COMMENT: return "cmt";
            case HighlightCategory::ERROR: return "err";
        }
        return "var";
    }

    static string escapeHTML(const string& text) {
        string result;
        for (char c : text) {
            switch (c) {
                case '<': result += "&lt;"; break;
                case '>': result += "&gt;"; break;
                case '&': result += "&amp;"; break;
                case '"': result += "&quot;"; break;
                default: result += c;
            }
        }
        return result;
    }

public:
    explicit SyntaxHighlighter(const ASTNode* root) { collectFunctionNames(root); }

    string renderANSI(const string& source, const vector<Token>& tokens) const {
        ostringstream out;
        size_t cursor = 0;
        for (const auto& token : tokens) {
            if (token.type == TokenType::END_OF_FILE) break;
            size_t start = static_cast<size_t>(token.loc.offset);
            if (start > cursor) out << source.substr(cursor, start - cursor);
            out << ansiCode(classify(token)) << token.lexeme << "\033[0m";
            cursor = start + token.lexeme.size();
        }
        if (cursor < source.size()) out << source.substr(cursor);
        return out.str();
    }

    string renderHTML(const string& source, const vector<Token>& tokens) const {
        ostringstream out;
        out << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"UTF-8\">\n"
            << "<title>B-Minor AST Syntax Highlighting</title>\n<style>\n"
            << "body{background:#1e1e1e;color:#d4d4d4;font-family:monospace;padding:20px}"
            << "pre{font-size:14px;line-height:1.5}.kw{color:#569cd6;font-weight:bold}"
            << ".type{color:#4ec9b0}.var{color:#d4d4d4}.fn{color:#dcdcaa;font-weight:bold}"
            << ".lit,.bool{color:#ce9178}.str{color:#6a9955}.op{color:#a0a0a0}"
            << ".cmt{color:#608b4e;font-style:italic}.err{color:#f44747;text-decoration:underline wavy}</style>"
            << "\n</head>\n<body>\n<pre>";

        size_t cursor = 0;
        for (const auto& token : tokens) {
            if (token.type == TokenType::END_OF_FILE) break;
            size_t start = static_cast<size_t>(token.loc.offset);
            if (start > cursor) out << escapeHTML(source.substr(cursor, start - cursor));
            out << "<span class=\"" << cssClass(classify(token)) << "\">"
                << escapeHTML(token.lexeme) << "</span>";
            cursor = start + token.lexeme.size();
        }
        if (cursor < source.size()) out << escapeHTML(source.substr(cursor));
        out << "</pre>\n</body>\n</html>\n";
        return out.str();
    }
};

// -----------------------------------------------------------------------------
// 6. Driver and tests
// -----------------------------------------------------------------------------
void runPipeline(const string& filename, const string& code) {
    cout << "====================================================================\n"
         << "FILE: " << filename << "\n"
         << "====================================================================\n";

    Lexer lexer(filename, code);
    auto tokens = lexer.tokenizeAll();

    cout << "--- [1] TOKEN STREAM ---\n";
    for (const auto& token : tokens) {
        if (token.type == TokenType::COMMENT) continue;
        string type = tokenTypeToString(token.type);
        cout << type;
        if (!token.lexeme.empty() &&
            (token.type == TokenType::IDENT || token.type == TokenType::INT_LIT ||
             token.type == TokenType::BOOL_LIT || token.type == TokenType::STRING_LIT ||
             token.type == TokenType::CHAR_LIT || token.type == TokenType::INVALID || type == "OP"))
            cout << '(' << token.lexeme << ')';
        cout << ' ';
    }
    cout << "\n\n--- [2] ABSTRACT SYNTAX TREE (AST) ---\n";

    Parser parser(tokens);
    auto ast = parser.parseProgram();
    ast->print();

    if (!lexer.getErrors().empty() || !parser.getErrors().empty()) {
        cout << "\n--- [3] DIAGNOSTICS & ERROR RECOVERY ---\n";
        for (const auto& message : lexer.getErrors()) cout << message << '\n';
        for (const auto& message : parser.getErrors()) cout << message << '\n';
    }

    SyntaxHighlighter highlighter(ast.get());
    cout << "\n--- [4] ANSI TERMINAL HIGHLIGHTED OUTPUT ---\n"
         << highlighter.renderANSI(code, tokens)
         << "\n\n--- [5] STANDALONE HTML/CSS OUTPUT ---\n"
         << highlighter.renderHTML(code, tokens) << '\n';
}

int main() {
    string validProgram =
        "// Recursive factorial in B-Minor\n"
        "factorial: function integer ( n: integer ) = {\n"
        "    if (n <= 1) return 1;\n"
        "    return n * factorial(n - 1);\n"
        "}\n";
    runPipeline("factorial.bminor", validProgram);

    string recoveryTest =
        "x: integer = 5 @;\n"
        "bad: integer = ;\n"
        "y: integer = 42;\n"
        "if (y > 0 {\n"
        "    return y;\n"
        "}\n";
    runPipeline("recovery.bminor", recoveryTest);
    return 0;
}
