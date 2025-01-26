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
 * @date 2025-01-25
 * @version 0.3.0
 *
 * This header file contains the declarations for the SqlGenerator library,
 * including the Token, Lexer, Parser, and SqlGenerator classes. The
 * SqlGenerator class allows for flexible and modular SQL statement generation
 * by supporting parameter substitution and sub-SQL inclusion.
 */
#pragma once

#include <drogon/plugins/Plugin.h>
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
 */
enum TokenType
{
    NormalText,  ///< Regular text in the SQL statement.
    At,          ///< '@'
    Identifier,  ///< An identifier, such as a sub sql name or param name.
    LParen,      ///< '('
    ASSIGN,      ///< '='
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
    Unknown      ///< An unknown token type.
};

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
 * @date 2025-01-17
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
};

using ParamList = std::unordered_map<
    std::string,
    std::variant<int64_t, trantor::Date, std::string, Json::Value>>;

/**
 * @class Parser
 * @brief Processes tokens to generate the final SQL statement.
 *
 * @author tanglong3bf
 * @date 2025-01-17
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
     * @brief Resets the Parser to the beginning of the SQL statement.
     */
    void reset()
    {
        lexer_.reset();
        ahead_ = lexer_.next();
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
     * The rules are as follows:
     * @code
<sql> ::= [NormalText] {(<sub_sql>|<param>) [NormalText]}
<param> ::= Dollar LBrace Identifier {{LBracket Integer RBracket} {Dot Identifier}} RBrace
<sub_sql> ::= At Identifier LParen [<param_list>] RParen
<param_list> ::= <param_item> { Comma <param_item> }
<param_item> ::= Identifier [ASSIGN <param_value>]
<param_value> ::= ParamValue | <param> | <sub_sql>
     * @endcode
     * @return The final SQL statement with parameters substituted and sub-SQL
     * statements included.
     */
    // clang-format on
    std::string parse();

  private:
    std::string sql();

    std::string param();

    std::string subSql();

    ParamList paramList();

    std::pair<std::string, std::string> paramItem();

    std::string paramValue();

    std::string match(TokenType);

    /**
     * @brief Retrieves the value of a parameter by name. If the parameter is
     * not found, an empty string is returned.
     * @param paramName The name of the parameter to retrieve.
     * @return The value of the parameter, or an empty string.
     * @note int64_t, trantor::Date will be converted to string.
     */
    std::variant<std::string, Json::Value> getParamByName(
        const std::string& paramName) const;

  private:
    ParamList params_;  ///< Map of parameter names and their values.
    std::function<std::string(const std::string&,
                              const ParamList&)>
        subSqlGetter_;  ///< Function to retrieve sub-SQL statements.
    Lexer lexer_;       ///< Lexer used to tokenize the SQL statement.
    Token ahead_;       ///< The next token to be processed.
};

/**
 * @class SqlGenerator
 * @brief The main class for generating SQL statements.
 * Inherits from `drogon::Plugin<SqlGenerator>`.
 *
 * @author tanglong3bf
 * @date 2025-01-17
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
