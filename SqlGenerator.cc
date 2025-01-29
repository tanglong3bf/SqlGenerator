/**
 * @file SqlGenerator.cc
 * @brief Implementation file for the SqlGenerator class, which provides a
 * framework for generating SQL statements dynamically.
 *
 * @author tanglong3bf
 * @date 2025-01-29
 * @version 0.4.0
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
    else if (isdigit(c))
    {
        string num;
        while (!done() && (isdigit(sql_[pos_])))
        {
            num += sql_[pos_++];
        }
        if (num.size() == 1)
        {
            return {Integer, num};
        }
        else if (num.size() > 1 && num[0] == '0')
        {
            int i = 1;
            for (; i < num.size() && num[i] == '0'; ++i)
                ;
            return {Integer, i == num.size() ? "0" : num.substr(i)};
        }
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

// sql ::= [NormalText] {(sub_sql|print_expr) [NormalText]}
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
            result += printExpr();
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

// print_expr ::= "$" "{" expr "}"
string Parser::printExpr()
{
    match(Dollar);
    match(LBrace);
    auto result = expr();
    match(RBrace);
    if (holds_alternative<int32_t>(result))
    {
        return to_string(get<int32_t>(result));
    }
    else  // string
    {
        return get<string>(result);
    }
}

// expr ::= Integer | String | Identifier {param_suffix}
ParamItem Parser::expr()
{
    if (ahead_.type() == Integer)
    {
        return fromString<int32_t>(match(Integer));
    }
    else if (ahead_.type() == String)
    {
        return match(String);
    }
    else if (ahead_.type() == Identifier)
    {
        auto paramName = match(Identifier);
        auto paramValue = getParamByName(paramName);
        while (ahead_.type() == Dot || ahead_.type() == LBracket)
        {
            paramSuffix(paramValue);
        }
        if (holds_alternative<string>(paramValue))
        {
            return get<string>(paramValue);
        }
        else if (holds_alternative<int32_t>(paramValue))
        {
            return get<int32_t>(paramValue);
        }
        else  // Json::Value
        {
            auto json = get<Json::Value>(paramValue);
            if (json.isInt())
            {
                return json.asInt();
            }
            else if (json.isString())
            {
                return json.asString();
            }
            return json;
        }
    }
    throw runtime_error("Invalid expression. Empty expression.");
}

// param_suffix ::= "[" expr "]" | "." Identifier
void Parser::paramSuffix(ParamItem &param)
{
    if (ahead_.type() == LBracket)
    {
        match(LBracket);
        auto index = expr();
        match(RBracket);
        if (holds_alternative<string>(param))
        {
            throw runtime_error("Invalid expression. Index for string.");
        }
        auto json = get<Json::Value>(param);
        if (json.isArray() && holds_alternative<int32_t>(index))
        {
            auto indexInt = get<int32_t>(index);
            if (indexInt < 0 || indexInt >= json.size())
            {
                throw runtime_error(
                    "Invalid expression. Array index out of range.");
            }
            param = json[indexInt];
        }
        else if (json.isObject() && holds_alternative<string>(index))
        {
            auto indexStr = get<string>(index);
            if (!json.isMember(indexStr))
            {
                throw runtime_error(
                    "Invalid expression. Object key not found.");
            }
            param = json[indexStr];
        }
    }
    else if (ahead_.type() == Dot)
    {
        match(Dot);
        auto memberName = match(Identifier);
        if (holds_alternative<string>(param))
        {
            throw runtime_error(
                "Invalid expression. Member access for string.");
        }
        auto json = get<Json::Value>(param);
        if (!json.isObject() || !json.isMember(memberName))
        {
            throw runtime_error(
                "Invalid expression. Member not found in object.");
        }
        param = json[memberName];
    }
}

// sub_sql ::= "@" Identifier "(" [param_list] ")"
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

// param_list ::= param_item { "," param_item }
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

// param_item ::= Identifier ["=" param_value]
ParamList::value_type Parser::paramItem()
{
    auto paramName = match(Identifier);
    ParamItem param;
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

// param_value ::= expr | sub_sql
ParamItem Parser::paramValue()
{
    if (ahead_.type() == At)
    {
        return subSql();
    }
    else
    {
        auto result = expr();
        if (holds_alternative<int32_t>(result))
        {
            return to_string(get<int32_t>(result));
        }
        else if (holds_alternative<string>(result))
        {
            return get<string>(result);
        }
        else  // Json::Value
        {
            return get<Json::Value>(result);
        }
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

ParamItem Parser::getParamByName(const string &paramName) const
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
    else if (holds_alternative<int32_t>(it->second))
    {
        return get<int32_t>(it->second);
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
                        default:
                            params.emplace(paramName, paramsJson[paramName]);
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
