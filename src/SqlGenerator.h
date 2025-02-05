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
 * @date 2025-02-05
 * @version 0.5.1
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
 * @date 2025-02-05
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
        TOKEN_TYPE_CASE(In);
        TOKEN_TYPE_CASE(Null);
        TOKEN_TYPE_CASE(EndFor);
        TOKEN_TYPE_CASE(Done);
        TOKEN_TYPE_CASE(Unknown);
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
     * @return The next token.
     * @date 2025-02-05
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
    std::stack<size_t> rollbackPos_;  ///< The number of tokens to backtrack.
};

using ParamList =
    std::unordered_map<std::string,
                       std::variant<int32_t, std::string, Json::Value>>;
using ParamItem =
    std::optional<std::variant<int32_t, std::string, Json::Value>>;

/**
 * @class Parser
 * @brief Processes tokens to generate the final SQL statement.
 *
 * @author tanglong3bf
 * @date 2025-02-05
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
     * @date 2025-02-05
     * @since 0.5.0
     */
    void printTokens()
    {
        auto printToken = [](const Token& token) {
            if (token.type() != Done)
            {
                std::cout << "[" << token.type() << "]";
                if (token.value() != "")
                {
                    std::cout << "<" << token.value() << ">";
                }
                std::cout << std::endl;
            }
        };
        for (; !lexer_.done(); nextToken())
        {
            printToken(ahead_.front());
        }
        printToken(ahead_.front());
        printToken(ahead_.back());
    }

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
     * sql ::= [NormalText] {(sub_sql|print_expr|if_stmt) [NormalText]}
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
     * factor ::= ["!"|"not"] ("(" bool_expr ")" | expr)
     * comp_expr ::= expr [("=="|"!=") expr]

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
    std::string sql();

    std::string printExpr();

    ParamItem expr();

    void paramSuffix(ParamItem& param);

    std::string subSql();

    ParamList paramList();

    ParamList::value_type paramItem();

    ParamItem paramValue();

    std::string ifStmt();

    bool boolExpr();

    bool term();

    bool factor();

    bool compExpr();

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
};

/**
 * @class SqlGenerator
 * @brief The main class for generating SQL statements.
 * Inherits from `drogon::Plugin<SqlGenerator>`.
 *
 * @author tanglong3bf
 * @date 2025-02-03
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
     * @param name The name of the SQL statement to print.
     * @param subSqlName The name of the sub-SQL statement to print (default is
     * "main").
     * @date 2025-02-03
     * @since 0.5.0
     */
    void printTokens(const std::string& name,
                     const std::string& subSqlName = "main")
    {
        std::cout << "Tokens for " << name << ":\n";
        if (parsers_.find(name) == parsers_.end())
        {
            parsers_.emplace(name, std::unordered_map<std::string, Parser>());
        }

        if (parsers_[name].find(subSqlName) == parsers_[name].end())
        {
            if (sqls_[name][subSqlName].isString())
            {
                parsers_[name].emplace(subSqlName,
                                       Parser(
                                           sqls_[name][subSqlName].asString()));
            }
            else if (sqls_[name][subSqlName].isObject() &&
                     sqls_[name][subSqlName].isMember("sql") &&
                     sqls_[name][subSqlName]["sql"].isString())
            {
                parsers_[name].emplace(
                    subSqlName,
                    Parser(sqls_[name][subSqlName]["sql"].asString()));
            }
            else
            {
                std::cout << "Invalid SQL statement for " << name << ":"
                          << subSqlName << std::endl;
                return;
            }
        }
        parsers_[name].at(subSqlName).printTokens();
    }

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

  private:
    Json::Value sqls_;  ///< The JSON object containing SQL statements.
    std::unordered_map<std::string, std::unordered_map<std::string, Parser>>
        parsers_;  ///< Map of parsers for each SQL statement and sub-SQL
                   ///< statement.
};
};  // namespace tl::sql
