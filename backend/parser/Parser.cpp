#include "Parser.h"
#include <iostream>



// =====================================
// CONSTRUCTOR
// =====================================

Parser::Parser(const std::vector<Token>& tokens)
    : tokens(tokens), current(0)
{
}

// =====================================
// HELPERS
// =====================================

Token Parser::peek()
{
    if (current < tokens.size())
        return tokens[current];

    return tokens.empty() ? Token() : tokens.back();
}

Token Parser::advance()
{
    if (current < tokens.size())
        current++;

    return tokens[current - 1];
}

bool Parser::check(std::string lex)
{
    return current < tokens.size() &&
           peek().lexeme == lex;
}

bool Parser::match(std::string lex)
{
    if (check(lex))
    {
        advance();
        return true;
    }

    return false;
}

void Parser::addError(std::string msg)
{
    Token t = peek();

    errors.push_back(
        "Line " +
        std::to_string(t.line) +
        ": " +
        msg +
        " near '" +
        t.lexeme +
        "'"
    );
}

bool Parser::isType(std::string lex)
{
    return lex == "int" ||
           lex == "char" ||
           lex == "bool";
}

// =====================================
// ENTRY POINT
// =====================================

Node* Parser::parse()
{
    return parseProgram();
}

// =====================================
// TYPE
//
// Type = ident ["[" "]"]
// =====================================

Node* Parser::parseType()
{
    Node* node = new Node("Type");

    if (peek().type != MyTokenType::IDENTIFIER &&
        !isType(peek().lexeme))
    {
        addError("Expected type");
        return nullptr;
    }

    node->children.push_back(
        new Node(advance().lexeme)
    );

    if (match("["))
    {
        if (!match("]"))
        {
            addError("Expected ']'");
            return nullptr;
        }

        node->children.push_back(
            new Node("Array")
        );
    }

    return node;
}

// =====================================
// CONST DECL
//
// final Type ident = value ;
// =====================================

Node* Parser::parseConstDecl()
{
    Node* node = new Node("ConstDecl");

    if (!match("final"))
    {
        addError("Expected 'final'");
        return nullptr;
    }

    Node* type = parseType();

    if (!type)
        return nullptr;

    node->children.push_back(type);

    if (peek().type != MyTokenType::IDENTIFIER)
    {
        addError("Expected constant name");
        return nullptr;
    }

    node->children.push_back(
        new Node(advance().lexeme)
    );

    if (!match("="))
    {
        addError("Expected '='");
        return nullptr;
    }

    if (peek().type == MyTokenType::NUMBER ||
        peek().type == MyTokenType::CHAR_CONST)
    {
        node->children.push_back(
            new Node(advance().lexeme)
        );
    }
    else
    {
        addError("Expected constant value");
        return nullptr;
    }

    if (!match(";"))
    {
        addError("Expected ';'");
        return nullptr;
    }

    return node;
}

// =====================================
// VAR DECL
//
// Type ident { , ident } ;
// =====================================

Node* Parser::parseVarDecl()

{
    std::cout << "parseVarDecl called" << std::endl;
    Node* node = new Node("VarDecl");

    Node* type = parseType();

    if (!type)
        return nullptr;

    node->children.push_back(type);

    if (peek().type != MyTokenType::IDENTIFIER)
    {
        addError("Expected variable name");
        return nullptr;
    }

    Token varToken = advance();

node->children.push_back(
    new Node(varToken.lexeme)
);

Symbol sym;

sym.name = varToken.lexeme;
sym.type = type->children[0]->name;
sym.kind = SymbolKind::VARIABLE;
sym.scopeLevel = 0;

if (!semanticAnalyzer.getTable().insert(sym))
{
    addError(
        "Semantic Error: '" +
        sym.name +
        "' already declared"
    );
}
else
{
    std::cout << "Inserted: "
              << sym.name
              << std::endl;
}

    while (match(","))
    {
        if (peek().type != MyTokenType::IDENTIFIER)
        {
            addError("Expected variable name");
            return nullptr;
        }

       Token varToken = advance();

node->children.push_back(
    new Node(varToken.lexeme)
);

Symbol sym;

sym.name = varToken.lexeme;
sym.type = type->children[0]->name;
sym.kind = SymbolKind::VARIABLE;
sym.scopeLevel = 0;

if (!semanticAnalyzer.getTable().insert(sym))
{
    addError(
        "Semantic Error: '" +
        sym.name +
        "' already declared"
    );
}
else
{
    std::cout << "Inserted: "
              << sym.name
              << std::endl;
}
    }

    if (!match(";"))
    {
        addError("Expected ';'");
        return nullptr;
    }

    return node;
}

// =====================================
// PROGRAM
//
// program ident
// {ConstDecl|VarDecl|ClassDecl}
// {
//     {MethodDecl}
// }
// =====================================

Node* Parser::parseProgram()
{
    Node* root = new Node("Program");

    // program
    if (!match("program"))
    {
        addError("Expected 'program'");
        return nullptr;
    }

    // program name
    if (peek().type != MyTokenType::IDENTIFIER)
    {
        addError("Expected program name");
        return nullptr;
    }

    root->children.push_back(
        new Node(advance().lexeme)
    );

    // =====================================
    // Global declarations
    // =====================================

    while (current < tokens.size())
    {
        // final Type id = value;
        if (check("final"))
        {
            Node* c = parseConstDecl();

            if (!c)
                return nullptr;

            root->children.push_back(c);
        }

        // class ...
        else if (check("class"))
        {
            Node* cls = parseClassDecl();

            if (!cls)
                return nullptr;

            root->children.push_back(cls);
        }

        // variable declaration
        else if (isType(peek().lexeme))
        {
            // look ahead:
            // int x;
            // int func(
            if (current + 2 < tokens.size() &&
                tokens[current + 1].type == MyTokenType::IDENTIFIER &&
                tokens[current + 2].lexeme == "(")
            {
                break;
            }

            Node* v = parseVarDecl();

            if (!v)
                return nullptr;

            root->children.push_back(v);
        }

        else
        {
            break;
        }
    }

    // =====================================
    // Opening {
    // =====================================

    if (!match("{"))
    {
        addError("Expected '{'");
        return nullptr;
    }

    // =====================================
    // Methods
    // =====================================

    while (current < tokens.size() &&
           !check("}"))
    {
        if (check("void") ||
            isType(peek().lexeme))
        {
            Node* method = parseMethodDecl();

            if (!method)
                return nullptr;

            root->children.push_back(method);
        }
        else
        {
            addError(
                "Expected method declaration"
            );
            return nullptr;
        }
    }

    // =====================================
    // Closing }
    // =====================================

    if (!match("}"))
    {
        addError("Expected '}'");
        return nullptr;
    }

    return root;
}
// =====================================
// CLASS DECL
//
// class ident
// {
//      {VarDecl}
// }
// =====================================

Node* Parser::parseClassDecl()
{
    Node* node = new Node("ClassDecl");

    if (!match("class"))
    {
        addError("Expected 'class'");
        return nullptr;
    }

    if (peek().type != MyTokenType::IDENTIFIER)
    {
        addError("Expected class name");
        return nullptr;
    }

    node->children.push_back(
        new Node(advance().lexeme)
    );

    if (!match("{"))
    {
        addError("Expected '{'");
        return nullptr;
    }

    while (current < tokens.size() &&
           !check("}"))
    {
        Node* var = parseVarDecl();

        if (!var)
            return nullptr;

        node->children.push_back(var);
    }

    if (!match("}"))
    {
        addError("Expected '}'");
        return nullptr;
    }

    return node;
}
// =====================================
// FORM PARS
//
// Type ident
// { , Type ident }
// =====================================

Node* Parser::parseFormPars()
{
    Node* node = new Node("FormPars");

    Node* type = parseType();

    if (!type)
        return nullptr;

    node->children.push_back(type);

    if (peek().type != MyTokenType::IDENTIFIER)
    {
        addError("Expected parameter name");
        return nullptr;
    }

    node->children.push_back(
        new Node(advance().lexeme)
    );

    while (match(","))
    {
        Node* nextType = parseType();

        if (!nextType)
            return nullptr;

        node->children.push_back(nextType);

        if (peek().type != MyTokenType::IDENTIFIER)
        {
            addError("Expected parameter name");
            return nullptr;
        }

        node->children.push_back(
            new Node(advance().lexeme)
        );
    }

    return node;
}
// =====================================
// METHOD DECL
//
// (Type|void)
// ident
// ( [FormPars] )
// {VarDecl}
// Block
// =====================================

Node* Parser::parseMethodDecl()
{
    Node* node = new Node("MethodDecl");

    // return type

    if (check("void"))
    {
        node->children.push_back(
            new Node("void")
        );

        advance();
    }
    else
    {
        Node* type = parseType();

        if (!type)
            return nullptr;

        node->children.push_back(type);
    }

    // method name

    if (peek().type != MyTokenType::IDENTIFIER)
    {
        addError("Expected method name");
        return nullptr;
    }

    node->children.push_back(
        new Node(advance().lexeme)
    );

    // (

    if (!match("("))
    {
        addError("Expected '('");
        return nullptr;
    }

    // optional parameters

    if (!check(")"))
    {
        Node* pars = parseFormPars();

        if (!pars)
            return nullptr;

        node->children.push_back(pars);
    }

    // )

    if (!match(")"))
    {
        addError("Expected ')'");
        return nullptr;
    }

    // local vars

    while (isType(peek().lexeme))
    {
        Node* var = parseVarDecl();

        if (!var)
            return nullptr;

        node->children.push_back(var);
    }

    // block

    Node* body = parseBlock();

    if (!body)
        return nullptr;

    node->children.push_back(body);

    return node;
}
// =====================================
// BLOCK
//
// {
//      {Statement}
// }
// =====================================

Node* Parser::parseBlock()
{
    Node* block = new Node("Block");

    if (!match("{"))
    {
        addError("Expected '{'");
        return nullptr;
    }

    while (!check("}") &&
           current < tokens.size())
    {
        Node* stmt = parseStatement();

        if (!stmt)
            return nullptr;

        block->children.push_back(stmt);
    }

    if (!match("}"))
    {
        addError("Expected '}'");
        return nullptr;
    }

    return block;
}
// =====================================
// CONDITION
//
// Expr Relop Expr
// =====================================

Node* Parser::parseCondition()
{
    Node* node = new Node("Condition");

    Node* left = parseExpression();

    if (!left)
        return nullptr;

    node->children.push_back(left);

    Node* rel = parseRelop();

    if (!rel)
        return nullptr;

    node->children.push_back(rel);

    Node* right = parseExpression();

    if (!right)
        return nullptr;

    node->children.push_back(right);

    return node;
}
Node* Parser::parseStatement()
{
    // =========================
    // EMPTY ;
    // =========================

    if (match(";"))
    {
        return new Node("Empty");
    }

    // =========================
    // BLOCK
    // =========================

    if (check("{"))
    {
        return parseBlock();
    }

    // =========================
    // IF
    // =========================

    if (match("if"))
    {
        Node* node = new Node("If");

        if (!match("("))
        {
            addError("Expected '('");
            return nullptr;
        }

        Node* cond = parseCondition();

        if (!cond)
            return nullptr;

        node->children.push_back(cond);

        if (!match(")"))
        {
            addError("Expected ')'");
            return nullptr;
        }

        Node* thenStmt = parseStatement();

        if (!thenStmt)
            return nullptr;

        node->children.push_back(thenStmt);

        if (match("else"))
        {
            Node* elseStmt = parseStatement();

            if (!elseStmt)
                return nullptr;

            node->children.push_back(elseStmt);
        }

        return node;
    }

    // =========================
    // WHILE
    // =========================

    if (match("while"))
    {
        Node* node = new Node("While");

        if (!match("("))
        {
            addError("Expected '('");
            return nullptr;
        }

        Node* cond = parseCondition();

        if (!cond)
            return nullptr;

        node->children.push_back(cond);

        if (!match(")"))
        {
            addError("Expected ')'");
            return nullptr;
        }

        Node* body = parseStatement();

        if (!body)
            return nullptr;

        node->children.push_back(body);

        return node;
    }

    // =========================
    // RETURN
    // =========================

    if (match("return"))
    {
        Node* node = new Node("Return");

        if (!check(";"))
        {
            Node* expr = parseExpression();

            if (!expr)
                return nullptr;

            node->children.push_back(expr);
        }

        if (!match(";"))
        {
            addError("Expected ';'");
            return nullptr;
        }

        return node;
    }

    // =========================
    // READ
    // =========================

    if (match("read"))
    {
        Node* node = new Node("Read");

        if (!match("("))
        {
            addError("Expected '('");
            return nullptr;
        }

        Node* des = parseDesignator();

        if (!des)
            return nullptr;

        node->children.push_back(des);

        if (!match(")"))
        {
            addError("Expected ')'");
            return nullptr;
        }

        if (!match(";"))
        {
            addError("Expected ';'");
            return nullptr;
        }

        return node;
    }

    // =========================
    // PRINT
    // =========================

    if (match("print"))
    {
        Node* node = new Node("Print");

        if (!match("("))
        {
            addError("Expected '('");
            return nullptr;
        }

        Node* expr = parseExpression();

        if (!expr)
            return nullptr;

        node->children.push_back(expr);

        if (match(","))
        {
            if (peek().type != MyTokenType::NUMBER)
            {
                addError("Expected number");
                return nullptr;
            }

            node->children.push_back(
                new Node(advance().lexeme)
            );
        }

        if (!match(")"))
        {
            addError("Expected ')'");
            return nullptr;
        }

        if (!match(";"))
        {
            addError("Expected ';'");
            return nullptr;
        }

        return node;
    }

    // =========================
    // ASSIGNMENT / METHOD CALL
    // =========================

    if (peek().type == MyTokenType::IDENTIFIER)
    {
        Node* des = parseDesignator();

        if (!des)
            return nullptr;

        // assignment

        if (match("="))
        {
            Node* node = new Node("Assign");

            node->children.push_back(des);

            Node* expr = parseExpression();

            if (!expr)
                return nullptr;

            node->children.push_back(expr);

            if (!match(";"))
            {
                addError("Expected ';'");
                return nullptr;
            }

            return node;
        }

        // method call

        if (check("("))
        {
            Node* node = new Node("Call");

            node->children.push_back(des);

            Node* pars = parseActPars();

            if (!pars)
                return nullptr;

            node->children.push_back(pars);

            if (!match(";"))
            {
                addError("Expected ';'");
                return nullptr;
            }

            return node;
        }

        addError("Expected '=' or method call");
        return nullptr;
    }

    addError("Invalid statement");
    return nullptr;
}
Node* Parser::parseExpression()
{
    Node* node = parseTerm();

    while (check("+") || check("-"))
    {
        std::string op = advance().lexeme;

        Node* parent = new Node(op);

        parent->children.push_back(node);

        Node* right = parseTerm();

        if (!right)
            return nullptr;

        parent->children.push_back(right);

        node = parent;
    }

    return node;
}
// Node* Parser::parseEquality()
// {
//     Node* node = parseComparison();

//     while (check("==") || check("!="))
//     {
//         std::string op = advance().lexeme;

//         Node* parent = new Node(op);

//         parent->children.push_back(node);

//         Node* right = parseComparison();

//         if (!right)
//             return nullptr;

//         parent->children.push_back(right);

//         node = parent;
//     }

//     return node;
// }
// Node* Parser::parseComparison()
// {
//     Node* node = parseTerm();

//     while (check(">") ||
//            check(">=") ||
//            check("<") ||
//            check("<="))
//     {
//         std::string op = advance().lexeme;

//         Node* parent = new Node(op);

//         parent->children.push_back(node);

//         Node* right = parseTerm();

//         if (!right)
//             return nullptr;

//         parent->children.push_back(right);

//         node = parent;
//     }

//     return node;
// }
Node* Parser::parseTerm()
{
    Node* node = parseFactor();

    while (check("+") || check("-"))
    {
        std::string op = advance().lexeme;

        Node* parent = new Node(op);

        parent->children.push_back(node);

        Node* right = parseFactor();

        if (!right)
            return nullptr;

        parent->children.push_back(right);

        node = parent;
    }

    return node;
}
Node* Parser::parseFactor()
{
    Node* node = parsePrimary();

    while (check("*") ||
           check("/") ||
           check("%"))
    {
        std::string op = advance().lexeme;

        Node* parent = new Node(op);

        parent->children.push_back(node);

        Node* right = parsePrimary();

        if (!right)
            return nullptr;

        parent->children.push_back(right);

        node = parent;
    }

    return node;
}
Node* Parser::parsePrimary()
{
    // unary minus
    if (match("-"))
    {
        Node* node = new Node("NEG");

        Node* expr = parsePrimary();

        if (!expr)
            return nullptr;

        node->children.push_back(expr);

        return node;
    }

    // ( Expr )
    if (match("("))
    {
        Node* expr = parseExpression();

        if (!match(")"))
        {
            addError("Expected ')'");
            return nullptr;
        }

        return expr;
    }

    // number
    if (peek().type == MyTokenType::NUMBER)
    {
        return new Node(advance().lexeme);
    }

    // char const
    if (peek().type == MyTokenType::CHAR_CONST)
    {
        return new Node(advance().lexeme);
    }

    // new ident [Expr]
    if (match("new"))
    {
        Node* node = new Node("New");

        if (peek().type != MyTokenType::IDENTIFIER)
        {
            addError("Expected type name");
            return nullptr;
        }

        node->children.push_back(
            new Node(advance().lexeme)
        );

        if (match("["))
        {
            Node* expr = parseExpression();

            if (!expr)
                return nullptr;

            node->children.push_back(expr);

            if (!match("]"))
            {
                addError("Expected ']'");
                return nullptr;
            }
        }

        return node;
    }

    // Designator [ActPars]
    if (peek().type == MyTokenType::IDENTIFIER)
    {
        Node* designator = parseDesignator();

        if (!designator)
            return nullptr;

        if (check("("))
        {
            Node* call = new Node("FunctionCall");

            call->children.push_back(designator);

            Node* pars = parseActPars();

            if (!pars)
                return nullptr;

            call->children.push_back(pars);

            return call;
        }

        return designator;
    }

    addError("Invalid expression");
    return nullptr;
}
Node* Parser::parseDesignator()
{
    Node* node = new Node("Designator");

    if (peek().type != MyTokenType::IDENTIFIER)
    {
        addError("Expected identifier");
        return nullptr;
    }

    node->children.push_back(
        new Node(advance().lexeme)
    );

    while (true)
    {
        if (match("."))
        {
            if (peek().type != MyTokenType::IDENTIFIER)
            {
                addError("Expected identifier");
                return nullptr;
            }

            node->children.push_back(
                new Node(".")
            );

            node->children.push_back(
                new Node(advance().lexeme)
            );
        }
        else if (match("["))
        {
            Node* expr = parseExpression();

            if (!expr)
                return nullptr;

            node->children.push_back(expr);

            if (!match("]"))
            {
                addError("Expected ']'");
                return nullptr;
            }
        }
        else
        {
            break;
        }
    }

    return node;
}
// =====================================
// ACT PARS
//
// ( [ Expr { , Expr } ] )
// =====================================

Node* Parser::parseActPars()
{
    Node* node = new Node("ActPars");

    if (!match("("))
    {
        addError("Expected '('");
        return nullptr;
    }

    // optional expressions

    if (!check(")"))
    {
        Node* expr = parseExpression();

        if (!expr)
            return nullptr;

        node->children.push_back(expr);

        while (match(","))
        {
            Node* nextExpr = parseExpression();

            if (!nextExpr)
                return nullptr;

            node->children.push_back(nextExpr);
        }
    }

    if (!match(")"))
    {
        addError("Expected ')'");
        return nullptr;
    }

    return node;
}
Node* Parser::parseRelop()
{
    if (check("==") ||
        check("!=") ||
        check(">")  ||
        check(">=") ||
        check("<")  ||
        check("<="))
    {
        return new Node(
            advance().lexeme
        );
    }

    addError("Expected relational operator");
    return nullptr;


}
SemanticAnalyzer&
Parser::getSemanticAnalyzer()
{
    return semanticAnalyzer;
}
