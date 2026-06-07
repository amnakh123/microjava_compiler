#pragma once

#include <vector>
#include <string>
#include "../lexer/Token.h"
#include "../semantic/SemanticAnalyzer.h"

struct Node {
    std::string name;
    std::vector<Node*> children;

    Node(std::string n) : name(n) {}
};

class Parser {
private:
    std::vector<Token> tokens;
    size_t current;

    SemanticAnalyzer semanticAnalyzer;

    std::vector<std::string> errors;   // ADD THIS

    // =====================
    // Helpers
    // =====================
    Token peek();
    Token advance();

    bool check(std::string lex);
    bool match(std::string lex);
    bool isType(std::string lex);

    void addError(std::string msg);

    // =====================
    // Grammar Productions
    // =====================
    Node* parseProgram();

    Node* parseConstDecl();
    Node* parseVarDecl();
    Node* parseClassDecl();

    Node* parseMethodDecl();
    Node* parseFormPars();

    Node* parseType();

    Node* parseBlock();
    Node* parseStatement();

    Node* parseActPars();

    Node* parseCondition();

    // Expressions
    Node* parseExpression();
    // Node* parseEquality();
    // Node* parseComparison();
    Node* parseTerm();
    Node* parseFactor();
    Node* parsePrimary();

    // Designators
    Node* parseDesignator();

    // Operators
    Node* parseRelop();
    Node* parseAddop();
    Node* parseMulop();

public:
    Parser(const std::vector<Token>& tokens);

    Node* parse();

   const std::vector<std::string>& getErrors() const
{
    return errors;
}
SemanticAnalyzer& getSemanticAnalyzer();
    }
;