/**
 * @file SqlGenerator.cc
 * @brief Implementation file for the SqlGenerator class, which provides a
 * framework for generating SQL statements dynamically.
 *
 * @author tanglong3bf
 * @date 2025-01-26
 * @version 0.3.1
 *
 * This implementation file contains the definitions for the SqlGenerator
 * library, including the Token, Lexer, Parser, and SqlGenerator classes. The
 * SqlGenerator class allows for flexible and modular SQL statement generation
 * by supporting parameter substitution and sub-SQL inclusion.
 */
#include "SqlGenerator.h"
#include <drogon/utils/Utilities.h>

using namespace ::std;
using namespace ::tl::sql;
using namespace ::drogon;
using namespace ::drogon::utils;

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
        {'[', LBracket},
        {']', RBracket},
        {'.', Dot},
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
        case '.':
        case '[':
        case ']':
        label:
            return {tokenMap.at(sql_[pos_++])};
    }

    // Parameter value, string format
    if (c == '\'' || c == '"')
    {
        ++pos_;
        string str;
        while (!done() && sql_[pos_] != c)
        {
            str += sql_[pos_++];
        }
        if (done())
        {
            throw runtime_error("Invalid expression. Unclosed string.");
        }
        ++pos_;
        return {String, str};
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
    // Parameter value, integer format
    else if (isdigit(c) || c == '-' || c == '+')
    {
        string num;
        while (!done() && (isdigit(sql_[pos_])))
        {
            num += sql_[pos_++];
        }
        return {Integer, num};
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

// <sql> ::= [NormalText] {(<sub_sql>|<param>) [NormalText]}
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
        }
        else if (ahead_.type() == Dollar)
        {
            result += param();
        }
        else
        {
            return result;
        }
        if (ahead_.type() == NormalText)
        {
            result += match(NormalText);
        }
    }
}

// <param> ::= Dollar LBrace Identifier
//             {{LBracket Integer RBracket} {Dot Identifier}}
//             RBrace
string Parser::param()
{
    match(Dollar);
    match(LBrace);
    auto paramName = match(Identifier);
    auto param = getParamByName(paramName);
    string result;
    while (true)
    {
        if (ahead_.type() != LBracket && ahead_.type() != Dot)
        {
            if (holds_alternative<string>(param))
            {
                result = get<string>(param);
            }
            else if (holds_alternative<Json::Value>(param))
            {
                result = get<Json::Value>(param).asString();
            }
            break;
        }
        while (true)
        {
            if (ahead_.type() != LBracket)
            {
                break;
            }
            match(LBracket);
            auto index = utils::fromString<int>(match(Integer));
            match(RBracket);
            auto json = get<Json::Value>(param);
            if (json.isArray() && index < json.size())
            {
                param = json[index];
            }
        }
        while (true)
        {
            if (ahead_.type() != Dot)
            {
                break;
            }
            match(Dot);
            auto fieldName = match(Identifier);
            auto json = get<Json::Value>(param);
            if (json.isObject())
            {
                param = json[fieldName];
            }
        }
    }
    match(RBrace);
    return result;
}

// <sub_sql> ::= At Identifier LParen [<param_list>] RParen
string Parser::subSql()
{
    match(At);
    auto subSqlName = match(Identifier);
    match(LParen);
    ParamList params = {};
    if (ahead_.type() == Identifier)
    {
        params = paramList();
    }
    match(RParen);
    return subSqlGetter_(subSqlName, params);
}

// <param_list> ::= <param_item> { Comma <param_item> }
ParamList Parser::paramList()
{
    ParamList result;
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

// <param_item> ::= Identifier [ASSIGN <param_value>]
pair<string, string> Parser::paramItem()
{
    auto paramName = match(Identifier);
    string param;
    if (ahead_.type() == ASSIGN)
    {
        match(ASSIGN);
        param = paramValue();
    }
    else
    {
        auto result = getParamByName(paramName);
        if (holds_alternative<string>(result))
        {
            param = get<string>(result);
        }
    }
    return {paramName, param};
}

// <param_value> ::= String | Integer | <param> | <sub_sql>
string Parser::paramValue()
{
    if (ahead_.type() == String)
    {
        return match(String);
    }
    else if (ahead_.type() == Integer)
    {
        return match(Integer);
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

variant<string, Json::Value> Parser::getParamByName(
    const string &paramName) const
{
    auto it = params_.find(paramName);
    if (it == params_.end())
    {
        LOG_WARN << "Parameter " + paramName + " not found.";
        return string();
    }
    if (holds_alternative<string>(it->second))
    {
        return get<string>(it->second);
    }
    else if (holds_alternative<int64_t>(it->second))
    {
        return to_string(get<int64_t>(it->second));
    }
    else if (holds_alternative<trantor::Date>(it->second))
    {
        return "'" + get<trantor::Date>(it->second).toDbStringLocal() + "'";
    }
    else  // Json::Value
    {
        return get<Json::Value>(it->second);
    }
}

void SqlGenerator::initAndStart(const Json::Value &config)
{
    assert(config.isObject());
    assert(config.isMember("sqls"));
    sqls_ = config["sqls"];
}

string SqlGenerator::getSql(const string &name, const ParamList &params)
{
    assert(sqls_.isMember(name));
    const auto &item = sqls_[name];
    assert(item.isString() ||
           item.isMember("main") &&
               (item["main"].isString() || item["main"].isObject()));
    return getMainSql(name, params);
}

string SqlGenerator::getMainSql(const string &name, ParamList params)
{
    if (sqls_[name].isString())
    {
        return getSimpleSql(name, params);
    }
    return getSubSql(name, "main", params);
}

string SqlGenerator::getSubSql(const string &name,
                               const string &subSqlName,
                               ParamList params)
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
                if (params.find(paramName) == params.end())
                {
                    switch (paramsJson[paramName].type())
                    {
                        case Json::stringValue:
                            params.emplace(paramName,
                                           paramsJson[paramName].asString());
                            break;
                        case Json::intValue:
                            params.emplace(paramName,
                                           paramsJson[paramName].asInt());
                            break;
                        case Json::objectValue:
                            params.emplace(paramName, paramsJson[paramName]);
                            break;
                        default:
                            LOG_ERROR << "Invalid parameter type: "
                                      << paramsJson[paramName].type();
                            break;
                    }
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
            [this, &name](const string &subSqlName, const ParamList &params) {
                return getSubSql(name, subSqlName, params);
            });

    return parsers_[name].at(subSqlName).parse();
}

string SqlGenerator::getSimpleSql(const string &name, const ParamList &params)
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
