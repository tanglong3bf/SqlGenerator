/**
 * @mainpage SqlGenerator Documentation
 *
 * @brief A SQL statement generator with support for parameter substitution
 * and sub-SQL inclusion.
 *
 * The SqlGenerator class provides a framework for generating SQL statements
 * dynamically. It supports the use of parameters and the inclusion of sub-SQL
 * statements, which can be defined in a JSON configuration file. This allows
 * for more flexible and modular SQL statement generation.
 *
 * The code is structured around three main classes: `Token`, `Lexer`, and
 * `Parser`.
 * - `Token` represents a single token in the SQL statement.
 * - `Lexer` is responsible for breaking down the SQL statement into tokens.
 * - `Parser` processes the tokens to generate the final SQL statement,
 * substituting parameters and including sub-SQL statements as necessary.
 *
 * @note This document provides detailed descriptions of each class and method
 * in the SqlGenerator library.
 */
/**
 * @file SqlGenerator.h
 * @brief Header file for the SqlGenerator class, which provides a framework
 * for generating SQL statements dynamically.
 *
 * @author tanglong3bf
 * @date 2025-02-13
 * @version 0.6.3
 *
 * This header file contains the declarations for the SqlGenerator library,
 * including the Token, Lexer, Parser, and SqlGenerator classes. The
 * SqlGenerator class allows for flexible and modular SQL statement generation
 * by supporting parameter substitution and sub-SQL inclusion.
 */
#pragma once

#include <drogon/plugins/Plugin.h>
#include <optional>
#include <variant>

/**
 * @namespace tl::sql
 * @brief Namespace containing all classes and functions related to the SQL
 * Generator.
 */
namespace tl::sql
{

/**
 * @enum TokenType
 * @brief Enumeration defining the types of tokens used in SQL generation.
 * @date 2025-02-05
 * @since 0.0.1
 */
enum TokenType
{
    NormalText,  ///< Regular text in the SQL statement.
    At,          ///< '@'
    Identifier,  ///< An identifier, such as a sub sql name or param name.
    LParen,      ///< '('
    Assign,      ///< '='
    String,      ///< A string parameter value.
    Integer,     ///< An integer parameter value.
    Comma,       ///< ','
    RParen,      ///< ')'
    Dollar,      ///< '$'
    LBrace,      ///< '{'
    RBrace,      ///< '}'
    Dot,         ///< '.'
    LBracket,    ///< '['
    RBracket,    ///< ']'
    If,          ///< 'if'
    And,         ///< 'and', '&&'
    Or,          ///< 'or', '||'
    Not,         ///< 'not', '!'
    EQ,          ///< '=='
    NEQ,         ///< '!=',
    Null,        ///< 'null'
    Else,        ///< 'else'
    ElIf,        ///< 'elif'
    EndIf,       ///< 'endif'
    For,         ///< 'for'
    Separator,   ///< 'separator'
    In,          ///< 'in'
    EndFor,      ///< 'endfor'
    Done,        ///< All token is processed.
    Unknown      ///< An unknown token type.
};

#define TOKEN_TYPE_CASE(type) \
    case type:                \
        return #type

/**
 * @brief Translates a TokenType to a string.
 * @param type The TokenType to be translated.
 * @return A string representation of the TokenType.
 * @date 2025-02-11
 * @since 0.5.1
 */
inline std::string to_string(TokenType type)
{
    switch (type)
    {
        TOKEN_TYPE_CASE(NormalText);
        TOKEN_TYPE_CASE(At);
        TOKEN_TYPE_CASE(Identifier);
        TOKEN_TYPE_CASE(LParen);
        TOKEN_TYPE_CASE(Assign);
        TOKEN_TYPE_CASE(String);
        TOKEN_TYPE_CASE(Integer);
        TOKEN_TYPE_CASE(Comma);
        TOKEN_TYPE_CASE(RParen);
        TOKEN_TYPE_CASE(Dollar);
        TOKEN_TYPE_CASE(LBrace);
        TOKEN_TYPE_CASE(RBrace);
        TOKEN_TYPE_CASE(Dot);
        TOKEN_TYPE_CASE(LBracket);
        TOKEN_TYPE_CASE(RBracket);
        TOKEN_TYPE_CASE(If);
        TOKEN_TYPE_CASE(And);
        TOKEN_TYPE_CASE(Or);
        TOKEN_TYPE_CASE(Not);
        TOKEN_TYPE_CASE(EQ);
        TOKEN_TYPE_CASE(NEQ);
        TOKEN_TYPE_CASE(Else);
        TOKEN_TYPE_CASE(ElIf);
        TOKEN_TYPE_CASE(EndIf);
        TOKEN_TYPE_CASE(For);
        TOKEN_TYPE_CASE(Separator);
        TOKEN_TYPE_CASE(In);
        TOKEN_TYPE_CASE(Null);
        TOKEN_TYPE_CASE(EndFor);
        TOKEN_TYPE_CASE(Done);
        default:
            return "Unknown";
    }
}

#undef TOKEN_TYPE_CASE

/**
 * @brief Outputs the name of a TokenType to an output stream.
 * @date 2025-02-05
 * @since 0.5.0
 */
inline std::ostream& operator<<(std::ostream& os, const TokenType& type)
{
    os << to_string(type);
    return os;
}

/**
 * @class Token
 * @brief Represents a single token in the SQL statement.
 *
 * @author tanglong3bf
 * @date 2025-01-17
 * @since 0.0.1
 */
class Token
{
  public:
    /**
     * @brief Default constructor for Token.
     * Initializes a Token with type Unknown and an empty value.
     */
    Token() : type_(Unknown), value_("")
    {
    }

    /**
     * @brief Constructor for Token.
     * Initializes a Token with the given type and value.
     * @param type The type of the token.
     * @param value The value of the token (default is an empty string).
     */
    Token(TokenType type, const std::string& value = "")
        : type_(type), value_(value)
    {
    }

  public:
    /**
     * @brief Returns the type of the token.
     * @return The type of the token.
     */
    TokenType type() const
    {
        return type_;
    }

    /**
     * @brief Returns the value of the token.
     * @return The value of the token.
     */
    std::string value() const
    {
        return value_;
    }

  private:
    TokenType type_;     ///< The type of the token.
    std::string value_;  ///< The value of the token.
};

/**
 * @class Lexer
 * @brief Breaks down the SQL statement into tokens.
 *
 * @author tanglong3bf
 * @date 2025-02-05
 * @since 0.0.1
 */
class Lexer
{
  public:
    /**
     * @brief Constructor for Lexer.
     * Initializes the Lexer with the given SQL statement.
     * @param sql The SQL statement to be tokenized.
     */
    Lexer(const std::string& sql) : sql_(sql), pos_(0), parenDepth_(0)
    {
    }

    /**
     * @brief Resets the Lexer to the beginning of the SQL statement.
     */
    void reset()
    {
        pos_ = 0;
        parenDepth_ = 0;
    }

    /**
     * @brief Returns the next token in the SQL statement.
     *
     * parenDepth_ represents the nesting depth of parentheses. If its value is
     * 0, it indicates that the parser is processing normal SQL text, and most
     * symbols are retained for the next token unless @ or $ is encountered.
     * The original logic incremented the parenthesis depth by one when @ or $
     * was encountered, and decremented it by one when ) or } was encountered,
     * which correctly parsed the sub-SQL name between @ and (.
     * However, with the increase in supported syntax, parentheses can also be
     * used to represent values and indices retrieved in for loops and nested
     * boolean expressions. Therefore, the logic has been modified.
     * When @ is encountered, the parenthesis depth is incremented by one, and
     * cancelOnceLParen_ is set to true.
     * When ( is encountered, if cancelOnceLParen_ is true, cancelOnceLParen_ is
     * set to false; otherwise, the parenthesis depth is incremented by one.
     * The handling of $ { } symbols remains the same as before.
     *
     * @return The next token.
     * @date 2025-02-13
     */
    Token next();

    /**
     * @brief Checks if the Lexer has reached the end of the SQL statement.
     * @return True if the end of the SQL statement has been reached, false
     * otherwise.
     */
    bool done()
    {
        return pos_ == sql_.size();
    }

  private:
    std::string sql_;       ///< The SQL statement being tokenized.
    size_t pos_{0};         ///< The current position in the SQL statement.
    size_t parenDepth_{0};  ///< The current depth of nested parentheses.
    bool cancelOnceLParen_{false};  ///< Whether to cancel the next LParen.
};

using ParamList =
    std::unordered_map<std::string,
                       std::variant<int32_t, std::string, Json::Value>>;
using ParamItem =
    std::optional<std::variant<int32_t, std::string, Json::Value>>;

/**
 * @brief Converts a ParamItem to a boolean value.
 *
 * This function takes a ParamItem and converts it to a boolean value.
 * An empty value, 0, or an empty string are considered false.
 * All other cases are considered true.
 *
 * @param value The ParamItem to be converted.
 * @return bool The converted boolean value.
 *
 * @date 2025-02-11
 * @since 0.5.2
 */
bool toBool(const ParamItem& value);

/**
 * @class ASTNode
 *
 * @brief Base class for all nodes in the Abstract Syntax Tree (AST).
 *
 * ASTNode serves as an abstract base class for all node types in the AST.
 * It provides common functionality such as setting a next sibling node and
 * generating SQL. The class also defines an interface for getting the value of
 * the node and printing it.
 *
 * @date 2025-02-11
 * @since 0.5.2
 */
class ASTNode
{
  public:
    ASTNode() = default;

    virtual ~ASTNode() = default;

  public:
    /**
     * @brief Sets the next sibling node.
     *
     * This method allows setting the next sibling node in the AST, enabling
     * traversal of siblings.
     *
     * @param nextSibling A shared pointer to the next sibling ASTNode.
     */
    void setNextSibling(std::shared_ptr<ASTNode> nextSibling)
    {
        nextSibling_ = nextSibling;
    }

    /**
     * @brief Generates SQL based on the node and its parameters.
     *
     * This method generates an SQL string based on the node's type and the
     * provided parameters.
     *
     * @param params A list of parameters to be used in SQL generation.
     * @return std::string The generated SQL string.
     */
    std::string generateSql(const ParamList& params = {}) const;

    /**
     * @brief Gets the value of the node.
     *
     * This method returns the value of the node.
     *
     * @param params A list of parameters that may affect the node's value.
     * @return ParamItem The value of the node.
     */
    virtual ParamItem getValue(const ParamList& params = {}) const = 0;

    /**
     * TODO:
     */
    void print(const std::string& indent = "") const;

  protected:
    std::shared_ptr<ASTNode>
        nextSibling_;  ///< Pointer to the next sibling node in the AST.
};

using ASTNodePtr = std::shared_ptr<ASTNode>;

/**
 * @class NormalTextNode
 *
 * @brief Represents a normal text node in the AST.
 *
 * NormalTextNode is a concrete class representing a node that contains plain
 * text. It overrides the getValue method to return the text as a ParamItem.
 *
 * @date 2025-02-11
 * @since 0.5.2
 */
class NormalTextNode : public ASTNode
{
  public:
    NormalTextNode(const std::string& text) : ASTNode(), text_(text)
    {
    }

    virtual ~NormalTextNode() = default;

  public:
    /**
     * @brief Gets the value of the node.
     *
     * @return ParamItem The text content of the node.
     */
    virtual ParamItem getValue(const ParamList& = {}) const override
    {
        return text_;
    }

  private:
    std::string text_;  ///< The text content of the node.
};

/**
 * @class NumberNode
 *
 * @brief Represents a number node in the AST.
 *
 * NumberNode is a concrete class representing a node that contains an integer
 * value. It overrides the getValue method to return the number as a ParamItem.
 *
 * @date 2025-02-11
 * @since 0.5.2
 */
class NumberNode : public ASTNode
{
  public:
    NumberNode(int32_t value) : ASTNode(), value_(value)
    {
    }

    virtual ~NumberNode() = default;

  public:
    /**
     * @brief Gets the value of the node.
     *
     * @return ParamItem The integer value of the node.
     */
    virtual ParamItem getValue(const ParamList& = {}) const override
    {
        return value_;
    }

  private:
    int32_t value_;  ///< The integer value of the node.
};

/**
 * @class StringNode
 *
 * @brief Represents a string node in the AST.
 *
 * StringNode is a concrete class representing a node that contains a string
 * value. It overrides the getValue method to return the string as a ParamItem.
 *
 * @date 2025-02-11
 * @since 0.5.2
 */
class StringNode : public ASTNode
{
  public:
    StringNode(const std::string& value) : ASTNode(), value_(value)
    {
    }

    virtual ~StringNode() = default;

  public:
    /**
     * @brief Gets the value of the node.
     *
     * @return ParamItem The string value of the node.
     */
    virtual ParamItem getValue(const ParamList& = {}) const override
    {
        return value_;
    }

  private:
    std::string value_;  ///< The string value of the node.
};

/**
 * @class NullNode
 *
 * @brief Represents a null node in the AST.
 *
 * NullNode is a concrete class representing a node that contains a null value.
 * It overrides the getValue method to return std::nullopt, indicating no value.
 *
 * @date 2025-02-11
 * @since 0.5.2
 */
class NullNode : public ASTNode
{
  public:
    NullNode() : ASTNode()
    {
    }

    virtual ~NullNode() = default;

  public:
    /**
     * @brief Gets the value of the node.
     *
     * @return ParamItem std::nullopt, indicating no value.
     */
    virtual ParamItem getValue(const ParamList& = {}) const override
    {
        return std::nullopt;
    }
};

/**
 * @class VariableNode
 *
 * @brief Represents a variable node in the AST.
 *
 * VariableNode is a concrete class representing a node that contains a variable
 * name. It overrides the getValue method to retrieve the variable's value from
 * the provided parameter list.
 *
 * @date 2025-02-11
 * @since 0.5.2
 */
class VariableNode : public ASTNode
{
  public:
    VariableNode(const std::string& name) : ASTNode(), name_(name)
    {
    }

    virtual ~VariableNode() = default;

  public:
    /**
     * @brief Gets the value of the node.
     *
     * This method retrieves the value of the variable from the provided
     * parameter list. If the variable is not found, it returns std::nullopt.
     *
     * @param params A list of parameters containing variable names and their
     * values.
     * @return ParamItem The value of the variable, or std::nullopt if not
     * found.
     */
    virtual ParamItem getValue(const ParamList& params = {}) const override;

  private:
    std::string name_;  ///< The name of the variable.
};

/**
 * @class BinaryOpNode
 *
 * @brief Represents a binary operation node in the AST.
 *
 * BinaryOpNode is an abstract class representing a node that contains two
 * operands. It provides common functionality for binary operations, but the
 * specific operation is determined by derived classes.
 *
 * @date 2025-02-11
 * @since 0.5.2
 */
class BinaryOpNode : public ASTNode
{
  public:
    BinaryOpNode(const ASTNodePtr& left, const ASTNodePtr& right)
        : ASTNode(), left_(left), right_(right)
    {
    }

    virtual ~BinaryOpNode() = default;

  protected:
    ASTNodePtr left_;   ///< The left operand node.
    ASTNodePtr right_;  ///< The right operand node.
};

/**
 * @class LogicalOpCode
 *
 * @brief Represents a logical operation node in the AST.
 *
 * LogicalOpCode is an abstract class representing a node that contains a
 * logical operation. It inherits from BinaryOpNode and provides common
 * functionality for logical operations.
 *
 * @date 2025-02-11
 * @since 0.5.2
 */
class LogicalOpCode : public BinaryOpNode
{
  public:
    LogicalOpCode(const ASTNodePtr& left, const ASTNodePtr& right)
        : BinaryOpNode(left, right)
    {
    }

    virtual ~LogicalOpCode() = default;
};

/**
 * @class MemberNode
 *
 * @brief Represents a member access node in the AST.
 *
 * MemberNode is a concrete class representing a node that accesses a member of
 * an object. It overrides the getValue method to retrieve the member's value.
 *
 * @date 2025-02-11
 * @since 0.5.2
 */
class MemberNode : public BinaryOpNode
{
  public:
    MemberNode(const ASTNodePtr& object, const ASTNodePtr& member)
        : BinaryOpNode(object, member)
    {
    }

    virtual ~MemberNode() = default;

  public:
    /**
     * @brief Gets the value of the node.
     *
     * This method retrieves the value of the member from the object.
     *
     * @param params A list of parameters (used to resolve variable values in
     * the object or member).
     * @return ParamItem The value of the member.
     */
    virtual ParamItem getValue(const ParamList& params = {}) const override;
};

/**
 * @class ArrayNode
 *
 * @brief Represents an array access node in the AST.
 *
 * ArrayNode is a concrete class representing a node that accesses an element of
 * an array. It overrides the getValue method to retrieve the element's value.
 *
 * @date 2025-02-11
 * @since 0.5.2
 */
class ArrayNode : public BinaryOpNode
{
  public:
    ArrayNode(const ASTNodePtr& array, const ASTNodePtr& index)
        : BinaryOpNode(array, index)
    {
    }

    virtual ~ArrayNode() = default;

  public:
    /**
     * @brief Gets the value of the node.
     *
     * This method retrieves the value of the element at the specified index in
     * the array.
     *
     * @param params A list of parameters (used to resolve variable values in
     * the array or index).
     * @return ParamItem The value of the array element.
     */
    virtual ParamItem getValue(const ParamList& params = {}) const override;
};

/**
 * @class SubSqlNode
 *
 * @brief Represents a sub-SQL node in the AST.
 *
 * SubSqlNode is a concrete class representing a node that contains a sub-SQL
 * query. It overrides the getValue method to retrieve the sub-SQL query as a
 * string.
 *
 * @date 2025-02-11
 * @since 0.5.2
 */
class SubSqlNode : public ASTNode
{
  public:
    SubSqlNode(const std::string& name,
               const std::function<std::string(const std::string&,
                                               const ParamList&)>& subSqlGetter,
               const std::unordered_map<std::string, ASTNodePtr>& params = {})
        : ASTNode(), name_(name), subSqlGetter_(subSqlGetter), params_(params)
    {
    }

    virtual ~SubSqlNode() = default;

  public:
    /**
     * @brief Gets the value of the node.
     *
     * This method retrieves the sub-SQL query as a string using the provided
     * sub-SQL getter function and parameters.
     *
     * @param params A list of parameters (used to resolve variable values in
     * the sub-SQL query).
     * @return ParamItem The sub-SQL query as a string.
     */
    virtual ParamItem getValue(const ParamList& params = {}) const override;

  private:
    std::string name_;  ///< The name of the sub-SQL query.

    std::function<std::string(const std::string&, const ParamList&)>
        subSqlGetter_;  ///< This function takes the name of the sub-SQL query
                        ///< and a list of parameters, and returns the sub-SQL
                        ///< query as a string.

    std::unordered_map<std::string, ASTNodePtr>
        params_;  ///< Map of parameter names and their corresponding ASTNodePtr
                  ///< values.
};

/**
 * @brief Represents a NOT logical operation node in the AST.
 *
 * NotNode is a concrete class representing a node that performs a NOT operation
 * on a single operand. It overrides the getValue method to return the boolean
 * negation of the operand's value.
 *
 * @date 2025-02-11
 * @since 0.5.2
 */
class NotNode : public LogicalOpCode
{
  public:
    NotNode(const ASTNodePtr& node) : LogicalOpCode(node, nullptr)
    {
    }

    virtual ~NotNode() = default;

  public:
    /**
     * @brief Gets the value of the node.
     *
     * This method returns the boolean negation of the operand's value.
     *
     * @param params A list of parameters (used to resolve variable values in
     * the operand).
     * @return ParamItem 1 if the operand's value is false, 0 if true.
     *
     * @see toBool
     */
    virtual ParamItem getValue(const ParamList& params = {}) const override
    {
        return toBool(left_->getValue(params)) ? 0 : 1;
    }
};

/**
 * @brief Represents an AND logical operation node in the AST.
 *
 * AndNode is a concrete class representing a node that performs an AND
 * operation on two operands. It overrides the getValue method to return the
 * logical AND of the operands' values.
 *
 * @date 2025-02-11
 * @since 0.5.2
 */
class AndNode : public LogicalOpCode
{
  public:
    AndNode(const ASTNodePtr& left, const ASTNodePtr& right)
        : LogicalOpCode(left, right)
    {
    }

    virtual ~AndNode() = default;

  public:
    /**
     * @brief Gets the value of the node.
     *
     * This method returns the logical AND of the operands' values.
     * An empty value, 0, or an empty string are considered false.
     * All other cases are considered true.
     *
     * @param params A list of parameters (used to resolve variable values in
     * the operands).
     * @return ParamItem 1 if both operands' values are true, 0 otherwise.
     */
    virtual ParamItem getValue(const ParamList& params = {}) const override;
};

/**
 * @brief Represents an OR logical operation node in the AST.
 *
 * OrNode is a concrete class representing a node that performs an OR operation
 * on two operands. It overrides the getValue method to return the logical OR of
 * the operands' values.
 *
 * @date 2025-02-11
 * @since 0.5.2
 */
class OrNode : public LogicalOpCode
{
  public:
    OrNode(const ASTNodePtr& left, const ASTNodePtr& right)
        : LogicalOpCode(left, right)
    {
    }

    virtual ~OrNode() = default;

  public:
    /**
     * @brief Gets the value of the node.
     *
     * This method returns the logical OR of the operands' values.
     * An empty value, 0, or an empty string are considered false.
     * All other cases are considered true.
     *
     * @param params A list of parameters (used to resolve variable values in
     * the operands).
     * @return ParamItem 1 if at least one operand's value is true, 0 otherwise.
     */
    virtual ParamItem getValue(const ParamList& params = {}) const override;
};

/**
 * @brief Represents an equality comparison node in the AST.
 *
 * EQNode is a concrete class representing a node that performs an equality
 * comparison between two operands. It overrides the getValue method to return
 * the result of the comparison.
 *
 * @date 2025-02-11
 * @since 0.5.2
 */
class EQNode : public BinaryOpNode
{
  public:
    EQNode(const ASTNodePtr& left, const ASTNodePtr& right)
        : BinaryOpNode(left, right)
    {
    }

    virtual ~EQNode() = default;

  public:
    /**
     * @brief Gets the value of the node.
     *
     * This method returns the result of the equality comparison between the
     * operands' values.
     *
     * @param params A list of parameters (used to resolve variable values in
     * the operands).
     * @return ParamItem 1 if the operands are equal, 0 otherwise.
     */
    virtual ParamItem getValue(const ParamList& params = {}) const override;
};

/**
 * @brief Represents a not-equality comparison node in the AST.
 *
 * NEQNode is a concrete class representing a node that performs a not-equality
 * comparison between two operands. It overrides the getValue method to return
 * the result of the comparison.
 *
 * @date 2025-02-11
 * @since 0.5.2
 */
class NEQNode : public BinaryOpNode
{
  public:
    NEQNode(const ASTNodePtr& left, const ASTNodePtr& right)
        : BinaryOpNode(left, right)
    {
    }

    virtual ~NEQNode() = default;

  public:
    /**
     * @brief Gets the value of the node.
     *
     * This method returns the result of the not-equality comparison between the
     * operands' values.
     *
     * @param params A list of parameters (used to resolve variable values in
     * the operands).
     * @return ParamItem 1 if the operands are not equal, 0 otherwise.
     */
    virtual ParamItem getValue(const ParamList& params = {}) const override;
};

/**
 * @brief Represents an if-statement node in the AST.
 *
 * IfStmtNode is a concrete class representing a node that contains an
 * if-statement. It overrides the getValue method to return the value of the
 * if-statement based on the condition.
 *
 * @date 2025-02-11
 * @since 0.5.2
 */
class IfStmtNode : public ASTNode
{
  public:
    IfStmtNode(const ASTNodePtr& condition,
               const ASTNodePtr& ifStmt,
               const ASTNodePtr& elseStmt = nullptr)
        : ASTNode(), condition_(condition), ifStmt_(ifStmt), elseStmt_(elseStmt)
    {
    }

    virtual ~IfStmtNode() = default;

  public:
    /**
     * @brief Gets the value of the node.
     *
     * This method returns the value of the if-statement based on the condition.
     * If the condition is true, it returns the value of the if-statement.
     * If the condition is false and an else-statement is provided, it returns
     * the value of the else-statement. Otherwise, it returns std::nullopt.
     *
     * @param params A list of parameters (used to resolve variable values in
     * the condition, if-statement, or else-statement).
     * @return ParamItem The value of the if-statement or else-statement based
     * on the condition.
     */
    virtual ParamItem getValue(const ParamList& params = {}) const override;

    /**
     * @brief Adds an else-if statement to the node.
     *
     * This method allows adding an else-if statement to the if-statement node.
     * The else-if statement consists of a boolean expression and the
     * corresponding if-statement.
     *
     * @param boolExpr A shared pointer to the boolean expression node.
     * @param ifStmt A shared pointer to the if-statement node.
     */
    void addElIfStmt(const ASTNodePtr& boolExpr, const ASTNodePtr& ifStmt)
    {
        elIfStmts_.emplace_back(boolExpr, ifStmt);
    }

  private:
    ASTNodePtr condition_;  ///< The condition node of the if-statement.
    ASTNodePtr ifStmt_;     ///< The if-statement node.
    std::vector<std::pair<ASTNodePtr, ASTNodePtr>>
        elIfStmts_;  ///< Vector of else-if statements, each consisting of a
                     ///< boolean expression and an if-statement.
    ASTNodePtr elseStmt_;  ///< The else-statement node (optional).
};

/**
 * @class ForLoopNode
 * @brief Represents a for loop node in the abstract syntax tree (AST) of an SQL
 * statement.
 *
 * This class is a specific type of AST node that handles the for loop construct
 * within SQL statements.
 * It iterates over a collection of values, optionally using an index, and
 * applies a loop body for each iteration.
 * The class supports collections represented as JSON arrays or objects.
 *
 * @date 2025-02-12
 * @since 0.6.0
 */
class ForLoopNode : public ASTNode
{
  public:
    ForLoopNode(const std::string& valueName,
                const std::string& indexName,
                const ASTNodePtr& collection,
                const ASTNodePtr& separator,
                const ASTNodePtr& block)
        : ASTNode(),
          valueName_(valueName),
          indexName_(indexName),
          collection_(collection),
          separator_(separator),
          loopBody_(block)
    {
    }

    virtual ~ForLoopNode() = default;

  public:
    /**
     * @brief Retrieves the value represented by this for loop node.
     *
     * This method evaluates the for loop by iterating over the collection,
     * optionally using an index, and generating SQL for each iteration. The
     * generated SQL is concatenated with the separator string.
     *
     * @param params A list of parameters to be used in evaluating the loop.
     * @return A ParamItem containing the generated SQL string for the loop.
     */
    virtual ParamItem getValue(const ParamList& params = {}) const override
    {
        auto newParams = params;
        auto collection = collection_->getValue(params);
        auto collectionJson =
            collection ? std::get<Json::Value>(*collection) : Json::Value{};
        auto separator =
            separator_ ? separator_->getValue(params) : std::nullopt;
        auto separatorStr = separator ? std::get<std::string>(*separator) : "";

        auto setParamAndIndex = [this](ParamList& newParams,
                                       const Json::Value& collectionJson,
                                       const auto& index) {
            const auto& valueJson = collectionJson[index];
            if (valueJson.isInt())
            {
                newParams.erase(valueName_);
                newParams.emplace(valueName_, valueJson.asInt());
            }
            else if (valueJson.isString())
            {
                newParams.erase(valueName_);
                newParams.emplace(valueName_, valueJson.asString());
            }
            else
            {
                newParams.erase(valueName_);
                newParams.emplace(valueName_, valueJson);
            }
            if (!indexName_.empty())
            {
                newParams.erase(indexName_);
                newParams.emplace(indexName_, index);
            }
        };
        std::string result;
        auto appendResult =
            [this, &result, &separatorStr](const ParamList& params,
                                           const Json::Value& collectionJson,
                                           const int i) {
                result += loopBody_->generateSql(params);
                if (static_cast<size_t>(i) + 1 != collectionJson.size())
                {
                    result += separatorStr;
                }
            };
        if (collectionJson.isArray())
        {
            for (int i = 0; static_cast<size_t>(i) < collectionJson.size(); ++i)
            {
                setParamAndIndex(newParams, collectionJson, i);
                appendResult(newParams, collectionJson, i);
            }
        }
        else if (collectionJson.isObject())
        {
            auto memberNames = collectionJson.getMemberNames();
            for (int i = 0; static_cast<size_t>(i) < memberNames.size(); ++i)
            {
                setParamAndIndex(newParams, collectionJson, memberNames[i]);
                appendResult(newParams, collectionJson, i);
            }
        }
        return result;
    }

  private:
    std::string valueName_;  ///< The name of the value variable in the loop.
    std::string indexName_;  ///< The name of the index or key variable in the
                             ///< loop.
    ASTNodePtr collection_;  ///< The collection to iterate over.
    ASTNodePtr separator_;   ///< The separator to use between generated SQL
                             ///< statements.
    ASTNodePtr loopBody_;    ///< The block of SQL statements to execute in each
                             ///< iteration.;
};

/**
 * @class Parser
 * @brief Processes tokens to generate the final SQL statement.
 *
 * @author tanglong3bf
 * @date 2025-02-11
 * @since 0.0.1
 */
class Parser
{
  public:
    /**
     * @brief Constructor for Parser.
     * Initializes the Parser with the given SQL statement.
     * @param sql The SQL statement to be parsed.
     */
    Parser(const std::string& sql) : lexer_(sql)
    {
        reset();
    }

    /**
     * @brief This function is used to print the tokens of the SQL statement.
     *
     * @date 2025-02-13
     * @since 0.5.0
     */
    void printTokens();

    /**
     * @brief Prints the Abstract Syntax Tree (AST) to the standard output.
     * @date 2025-02-11
     * @since 0.5.2
     */
    void printAST();

    /**
     * @brief Resets the Parser to the beginning of the SQL statement.
     * @date 2025-02-05
     */
    void reset()
    {
        lexer_.reset();
        ahead_.clear();
        ahead_.emplace_back(lexer_.next());
        ahead_.emplace_back(lexer_.next());
    }

    /**
     * @brief Sets the parameters for the SQL statement.
     * @param params A map of parameter names and their values.
     */
    void setParams(const ParamList& params)
    {
        this->params_ = params;
    }

    /**
     * @brief Sets the function to retrieve sub-SQL statements.
     * @param subSqlGetter A function that takes the name of a sub-SQL statement
     * and a map of parameters, and returns the sub-SQL statement.
     */
    void setSubSqlGetter(
        const std::function<std::string(const std::string&, const ParamList&)>&
            subSqlGetter)
    {
        this->subSqlGetter_ = subSqlGetter;
    }

    // clang-format off
    /**
     * @brief Parses the SQL statement.
     * This function parses the input SQL statement according to the given SQL
     * syntax rules, replaces the parameters, and processes nested sub SQL
     * statements.
     *
     * The rules are as follows:
     * @code{.ebnf}
     * sql ::= [NormalText] {(sub_sql|print_expr|if_stmt|for_loop) [NormalText]}
     * print_expr ::= "$" "{" expr "}"
     * expr ::= 'null' | Integer | String | Identifier {param_suffix}
     * param_suffix ::= "[" expr "]" | "." Identifier
     * sub_sql ::= "@" Identifier "(" [param_list] ")"
     * param_list ::= param_item { "," param_item }
     * param_item ::= Identifier ["=" param_value]
     * param_value ::= expr | sub_sql
     * if_stmt ::= "@" "if" "(" bool_expr ")" sql
     *            {"@" "elif" "(" bool_expr ")" sql}
     *            ["@" "else" sql]
     *             "@" "endif"
     * bool_expr ::= term {("or"|"||") term}
     * term ::= factor {("and"|"&&") factor}
     * factor ::= ["!"|"not"] ("(" bool_expr ")" | comp_expr)
     * comp_expr ::= expr [("=="|"!=") expr]
	 * for_loop ::= "@" "for" "("
	 *              (Identifier|"(" Identifier "," Identifier ")") "in" expr
	 *              ["," "separator" "=" String]
	 *              ")" sql "@" "endif"
     *
     * // Regular expressions to express basic tokens.
     * NormalText ::= [^@$]*
     * Identifier ::= [a-zA-Z0x80-0xff_][a-zA-Z0-90x80-0xff_]*
     * Integer ::= [1-9]\d*|0     // remove prefix '0'
     * String ::= "[^"]*"|'[^']*'
     * @endcode
     *
     * @date 2025-02-05
     * @since 0.0.1
     * @return The final SQL statement with parameters substituted and sub-SQL
     * statements included.
     */
    // clang-format on
    std::string parse();

  private:
    ASTNodePtr sql();

    ASTNodePtr printExpr();

    ASTNodePtr expr();

    void paramSuffix(ASTNodePtr& param);

    ASTNodePtr subSql();

    std::unordered_map<std::string, ASTNodePtr> paramList();

    std::pair<std::string, ASTNodePtr> paramItem();

    ASTNodePtr paramValue();

    ASTNodePtr ifStmt();

    ASTNodePtr boolExpr();

    ASTNodePtr term();

    ASTNodePtr factor();

    ASTNodePtr compExpr();

    ASTNodePtr forLoop();

    std::string match(TokenType);

    /**
     * @date 2025-02-05
     * @since 0.5.1
     */
    void nextToken();

    /**
     * @brief Retrieves the value of a parameter by name. If the parameter is
     * not found, an std::nullopt is returned.
     * @param paramName The name of the parameter to retrieve.
     * @return The value of the parameter, or an std::nullopt.
     */
    ParamItem getParamByName(const std::string& paramName) const;

  private:
    ParamList params_;  ///< Map of parameter names and their values.
    std::function<std::string(const std::string&,
                              const ParamList&)>
        subSqlGetter_;         ///< Function to retrieve sub-SQL statements.
    Lexer lexer_;              ///< Lexer used to tokenize the SQL statement.
    std::deque<Token> ahead_;  ///< The next token to be processed.
    ASTNodePtr root_;          ///< The root node of the AST.
};

/**
 * @class SqlGenerator
 * @brief The main class for generating SQL statements.
 * Inherits from `drogon::Plugin<SqlGenerator>`.
 *
 * @author tanglong3bf
 * @date 2025-02-11
 * @since 0.0.1
 */
class SqlGenerator : public drogon::Plugin<SqlGenerator>
{
  public:
    /// This method must be called by drogon to initialize and start the
    /// plugin.. It must be implemented by the user..
    void initAndStart(const Json::Value& config);

    /// This method must be called by drogon to shutdown the plugin..
    /// It must be implemented by the user..
    void shutdown()
    {
    }

    /**
     * @brief Only print the tokens of a SQL statement.
     *
     * @date 2025-02-03
     * @since 0.5.0
     */
    void printTokens(const std::string& name,
                     const std::string& subSqlName = "main");

    /**
     * @brief Print the Abstract Syntax Tree (AST) of a SQL statement.
     *
     * @date 2025-02-11
     * @since 0.5.2
     */
    void printAST(const std::string& name,
                  const std::string& subSqlName = "main");

    /**
     * @brief Retrieves a SQL statement by name, with optional parameters.
     * @param name The name of the SQL statement to retrieve.
     * @param params A map of parameter names and their values (default is an
     * empty map).
     * @return The SQL statement with parameters substituted.
     */
    std::string getSql(const std::string& name, const ParamList& params = {});

  private:
    /**
     * @brief Retrieves the main SQL statement by name.
     * @param name The name of the main SQL statement.
     * @param params A map of parameter names and their values.
     * @return The main SQL statement with parameters substituted.
     */
    std::string getMainSql(const std::string& name, ParamList params);

    /**
     * @brief Retrieves a sub-SQL statement by name.
     * @param name The name of the SQL statement containing the sub-SQL
     * statement.
     * @param subSqlName The name of the sub-SQL statement.
     * @param params A map of parameter names and their values.
     * @return The sub-SQL statement with parameters substituted.
     */
    std::string getSubSql(const std::string& name,
                          const std::string& subSqlName,
                          ParamList params);

    /**
     * @brief Retrieves a simple SQL statement by name.
     * @param name The name of the simple SQL statement.
     * @param params A map of parameter names and their values.
     * @return The simple SQL statement with parameters substituted.
     */
    std::string getSimpleSql(const std::string& name, const ParamList& params);

    /**
     * @brief Prepares the parser before printing tokens, printing AST, or
     * obtaining the SQL statement.
     *
     * @date 2025-02-11
     * @since 0.5.2
     */
    void prepareParser(const std::string& name, const std::string& subSqlName);

  private:
    Json::Value sqls_;  ///< The JSON object containing SQL statements.
    std::unordered_map<std::string, std::unordered_map<std::string, Parser>>
        parsers_;  ///< Map of parsers for each SQL statement and sub-SQL
                   ///< statement.
};
};  // namespace tl::sql
