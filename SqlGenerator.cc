/**
 * @file SqlGenerator.cc
 * @brief Implementation file for the SqlGenerator class, which provides a
 * framework for generating SQL statements dynamically.
 *
 * @author tanglong3bf
 * @date 2025-01-17
 * @version 0.0.1
 *
 * This implementation file contains the definitions for the SqlGenerator
 * library, including the Token, Lexer, Parser, and SqlGenerator classes. The
 * SqlGenerator class allows for flexible and modular SQL statement generation
 * by supporting parameter substitution and sub-SQL inclusion.
 */
#include "SqlGenerator.h"

using namespace ::std;
using namespace ::tl::sql;

Token Lexer::next()
{
    if (done())
    {
        return {Unknown};
    }
    static const unordered_map<char, TokenType> tokenMap = {
        {'(', LParen},
        {')', RParen},
        {',', Comma},
        {'@', At},
        {'=', ASSIGN},
        {'$', Dollar},
        {'{', LBrace},
        {'}', RBrace},
    };

    auto c = sql_[pos_];
    // Non-special syntax characters
    if (parenDepth_ == 0)
    {
        if (c == '@' || c == '$')
        {
            ++parenDepth_;
            ++pos_;
            return {tokenMap.at(c)};
        }
        string value;
        while (!done() && c != '@' && c != '$')
        {
            value += c;
            c = sql_[++pos_];
        }
        return {NormalText, value};
    }
    // Skip whitespace characters
    while (c == ' ' || c == '\t' || c == '\r' || c == '\n')
    {
        c = sql_[++pos_];
    }

    // Single character token
    switch (c)
    {
        case '@':
        case '$':
            ++parenDepth_;
            goto label;
        case ')':
        case '}':
            --parenDepth_;
        case '(':
        case ',':
        case '=':
        case '{':
        label:
            return {tokenMap.at(sql_[pos_++])};
    }

    // Parameter value, string format
    if (c == '\'' || c == '"')
    {
        string str;
        while (!done() && sql_[pos_] != c)
        {
            str += sql_[++pos_];
        }
        if (done())
        {
            throw runtime_error("Invalid expression. Unclosed string.");
        }
        ++pos_;
        return {ParamValue, str};
    }
    // Sub-SQL name or parameter name
    else if (isalpha(c) || c == '_' || c & 0x80)
    {
        string identifier;
        while (!done() &&
               (isalnum(sql_[pos_]) || sql_[pos_] == '_' || sql_[pos_] & 0x80))
        {
            identifier += sql_[pos_++];
        }
        return {Identifier, identifier};
    }
    throw runtime_error("Invalid expression(" + to_string(pos_) +
                        "): " + sql_.substr(pos_));
}

string Parser::parse()
{
    reset();
    auto result = sql();
    if (lexer_.done())
        return result;
    throw runtime_error("Invalid expression.");
}

string Parser::sql()
{
    string result;
    if (ahead_.type() == NormalText)
    {
        result += match(NormalText);
    }
    while (true)
    {
        if (ahead_.type() == At)
        {
            result += subSql();
            if (ahead_.type() == NormalText)
            {
                result += match(NormalText);
            }
        }
        else if (ahead_.type() == Dollar)
        {
            result += param();
            if (ahead_.type() == NormalText)
            {
                result += match(NormalText);
            }
        }
        else
        {
            return result;
        }
    }
}

string Parser::param()
{
    match(Dollar);
    match(LBrace);
    auto paramName = match(Identifier);
    match(RBrace);
    return params_[paramName];
}

string Parser::subSql()
{
    match(At);
    auto subSqlName = match(Identifier);
    match(LParen);
    unordered_map<string, string> params = {};
    if (ahead_.type() == Identifier)
    {
        params = paramList();
    }
    match(RParen);
    return subSqlGetter_(subSqlName, params);
}

unordered_map<string, string> Parser::paramList()
{
    unordered_map<string, string> result;
    result.emplace(paramItem());
    while (true)
    {
        if (ahead_.type() == Comma)
        {
            match(Comma);
            result.emplace(paramItem());
        }
        else
        {
            return result;
        }
    }
}

pair<string, string> Parser::paramItem()
{
    auto paramName = match(Identifier);
    std::string param;
    if (ahead_.type() == ASSIGN)
    {
        match(ASSIGN);
        param = paramValue();
    }
    else
    {
        param = params_[paramName];
    }
    return {paramName, param};
}

string Parser::paramValue()
{
    if (ahead_.type() == ParamValue)
    {
        return match(ParamValue);
    }
    else if (ahead_.type() == Dollar)
    {
        return param();
    }
    else if (ahead_.type() == At)
    {
        return subSql();
    }
    throw runtime_error("Invalid expression. Empty parameter value.");
}

string Parser::match(TokenType type)
{
    assert(ahead_.type() == type);
    auto value = ahead_.value();
    ahead_ = lexer_.next();
    return value;
}

void SqlGenerator::initAndStart(const Json::Value &config)
{
    assert(config.isObject());
    assert(config.isMember("sqls"));
    sqls_ = config["sqls"];
}

string SqlGenerator::getSql(const string &name,
                            const unordered_map<string, string> &params)
{
    assert(sqls_.isMember(name));
    const auto &item = sqls_[name];
    assert(item.isString() ||
           item.isMember("main") &&
               (item["main"].isString() || item["main"].isObject()));
    return getMainSql(name, params);
}

string SqlGenerator::getMainSql(const string &name,
                                unordered_map<string, string> params)
{
    if (sqls_[name].isString())
    {
        return getSimpleSql(name, params);
    }
    return getSubSql(name, "main", params);
}

string SqlGenerator::getSubSql(const string &name,
                               const string &subSqlName,
                               unordered_map<string, string> params)
{
    // Set default parameters if not provided by the user
    auto setDefaultParam = [&]() {
        auto &subSqlJson = sqls_[name][subSqlName];
        if (subSqlJson.isObject() && subSqlJson.isMember("params") &&
            subSqlJson["params"].isObject())
        {
            auto &paramsJson = subSqlJson["params"];
            for (const auto &paramName : paramsJson.getMemberNames())
            {
                if (paramsJson[paramName].isString() &&
                    params.find(paramName) == params.end())
                {
                    params.emplace(paramName, paramsJson[paramName].asString());
                }
            }
        }
    };

    if (parsers_.find(name) == parsers_.end())
    {
        parsers_.emplace(name, unordered_map<string, Parser>());
    }

    // If the parser has been set, use it directly
    if (parsers_[name].find(subSqlName) != parsers_[name].end())
    {
        setDefaultParam();
        parsers_[name].at(subSqlName).setParams(params);
        return parsers_[name].at(subSqlName).parse();
    }

    // Prepare parameters
    string sql;
    auto &subSqlJson = sqls_[name][subSqlName];
    if (subSqlJson.isString())
    {
        sql = subSqlJson.asString();
    }
    else if (subSqlJson.isObject())
    {
        if (subSqlJson.isMember("sql") && subSqlJson["sql"].isString())
        {
            sql = subSqlJson["sql"].asString();
        }
        setDefaultParam();
    }

    // Set the parser for the corresponding SQL statement
    parsers_[name].emplace(subSqlName, Parser(sql));
    parsers_[name].at(subSqlName).setParams(params);
    parsers_[name]
        .at(subSqlName)
        .setSubSqlGetter(
            [this, &name](const string &subSqlName,
                          const unordered_map<string, string> &params) {
                return getSubSql(name, subSqlName, params);
            });

    return parsers_[name].at(subSqlName).parse();
}

string SqlGenerator::getSimpleSql(const string &name,
                                  const unordered_map<string, string> &params)

{
    if (parsers_.find(name) == parsers_.end())
    {
        parsers_.emplace(name, unordered_map<string, Parser>());
    }
    if (parsers_[name].find("main") == parsers_[name].end())
    {
        parsers_[name].emplace("main", Parser(sqls_[name].asString()));
    }
    parsers_[name].at("main").setParams(params);
    return parsers_[name].at("main").parse();
}
