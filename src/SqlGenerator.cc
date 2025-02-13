/**
 * @file SqlGenerator.cc
 * @brief Implementation file for the SqlGenerator class, which provides a
 * framework for generating SQL statements dynamically.
 *
 * @author tanglong3bf
 * @date 2025-02-13
 * @version 0.6.2
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
        return {Done};
    }
    static const unordered_map<char, TokenType> tokenMap = {
        {'(', LParen},
        {')', RParen},
        {',', Comma},
        {'@', At},
        {'$', Dollar},
        {'{', LBrace},
        {'}', RBrace},
        {'[', LBracket},
        {']', RBracket},
        {'.', Dot},
    };
    static const unordered_map<string, TokenType> keywordMap = {
        {"and", And},
        {"or", Or},
        {"not", Not},
        {"if", If},
        {"else", Else},
        {"elif", ElIf},
        {"endif", EndIf},
        {"for", For},
        {"separator", Separator},
        {"in", In},
        {"null", Null},
        {"endfor", EndFor},
    };
    auto c = sql_[pos_];
    // Non-special syntax characters
    if (parenDepth_ == 0)
    {
        if (c == '@')
        {
            cancelOnceLParen_ = true;
        }
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
        case '(':
            if (!cancelOnceLParen_)
            {
                case '@':
                    cancelOnceLParen_ = true;
                case '$':
                    ++parenDepth_;
            }
            cancelOnceLParen_ = false;
            goto label;
        case ')':
        case '}':
            --parenDepth_;
            [[fallthrough]];
        case ',':
        case '{':
        case '.':
        case '[':
        case ']':
        label:
            return {tokenMap.at(sql_[pos_++])};
    }

    // ! or !=
    if (c == '!')
    {
        if (pos_ + 1 < sql_.size() && sql_[pos_ + 1] == '=')
        {
            pos_ += 2;
            return {NEQ};
        }
        else
        {
            ++pos_;
            return {Not};
        }
    }
    // = or ==
    else if (c == '=')
    {
        if (pos_ + 1 < sql_.size() && sql_[pos_ + 1] == '=')
        {
            pos_ += 2;
            return {EQ};
        }
        else
        {
            ++pos_;
            return {Assign};
        }
    }
    // &&
    else if (c == '&' && pos_ + 1 < sql_.size() && sql_[pos_ + 1] == '&')
    {
        pos_ += 2;
        return {And};
    }
    // ||
    else if (c == '|' && pos_ + 1 < sql_.size() && sql_[pos_ + 1] == '|')
    {
        pos_ += 2;
        return {Or};
    }
    // Parameter value, string format
    else if (c == '\'' || c == '"')
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
    // Sub-SQL name or parameter name or keyword
    else if (isalpha(c) || c == '_' || c & 0x80)
    {
        string identifier;
        while (!done() &&
               (isalnum(sql_[pos_]) || sql_[pos_] == '_' || sql_[pos_] & 0x80))
        {
            identifier += sql_[pos_++];
        }
        if (keywordMap.count(identifier))
        {
            if (identifier == "else" || identifier == "endif" ||
                identifier == "endfor")
            {
                --parenDepth_;
            }
            return {keywordMap.at(identifier)};
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
            size_t i = 1;
            for (; i < num.size() && num[i] == '0'; ++i)
                ;
            return {Integer, i == num.size() ? "0" : num.substr(i)};
        }
    }

    throw runtime_error("Invalid expression(" + std::to_string(pos_) +
                        "): " + sql_.substr(pos_));
}

bool tl::sql::toBool(const ParamItem &value)
{
    if (!value)
    {
        return false;
    }
    if (holds_alternative<int32_t>(*value))
    {
        return get<int32_t>(*value) != 0;
    }
    else if (holds_alternative<string>(*value))
    {
        return get<string>(*value) != "";
    }
    // Json::Value
    return true;
}

string ASTNode::generateSql(const ParamList &params) const
{
    auto value = getValue(params);
    string result;
    if (value)
    {
        if (holds_alternative<string>(*value))
        {
            result = get<string>(*value);
        }
        else if (holds_alternative<int32_t>(*value))
        {
            result = std::to_string(get<int32_t>(*value));
        }
        // JSon::Value (objectValue or arrayValue) is not supported to be
        // converted to string
    }
    if (nextSibling_)
    {
        result += nextSibling_->generateSql(params);
    }
    return result;
}

void ASTNode::print(const string &indent) const
{
    cout << indent << "\033[31m"
         << "[Not implemented]"
         << "\033[0m" << endl;
}

ParamItem VariableNode::getValue(const ParamList &params) const
{
    if (params.count(name_) == 0)
    {
        return nullopt;
    }
    auto value = params.at(name_);
    if (holds_alternative<int32_t>(value))
    {
        return get<int32_t>(value);
    }
    else if (holds_alternative<string>(value))
    {
        return get<string>(value);
    }
    else  // Json::Value
    {
        return get<Json::Value>(value);
    }
}

ParamItem MemberNode::getValue(const ParamList &params) const
{
    auto value = left_->getValue(params);
    if (value && holds_alternative<Json::Value>(*value))
    {
        auto json = get<Json::Value>(*value);
        auto memberName = get<string>(*right_->getValue());
        if (json.isObject() && json.isMember(memberName))
        {
            auto result = json[memberName];
            if (result.isInt())
            {
                return result.asInt();
            }
            else if (result.isString())
            {
                return result.asString();
            }
            else  // object or array
            {
                return result;
            }
        }
    }
    return nullopt;
}

ParamItem ArrayNode::getValue(const ParamList &params) const
{
    auto value = left_->getValue(params);
    if (value && holds_alternative<Json::Value>(*value))
    {
        auto json = get<Json::Value>(*value);
        Json::Value result;
        auto rightValue = right_->getValue(params);
        if (holds_alternative<int32_t>(*rightValue))
        {
            auto index = get<int32_t>(*rightValue);
            if (json.isArray() && index >= 0 &&
                index < static_cast<int>(json.size()))
            {
                result = json[index];
            }
        }
        else if (holds_alternative<string>(*rightValue))
        {
            auto memberName = get<string>(*rightValue);
            if (json.isObject() && json.isMember(memberName))
            {
                result = json[memberName];
            }
        }
        if (result.isInt())
        {
            return result.asInt();
        }
        else if (result.isString())
        {
            return result.asString();
        }
        else  // object or array
        {
            return result;
        }
    }
    return nullopt;
}

ParamItem SubSqlNode::getValue(const ParamList &params) const
{
    ParamList subParams;
    for (const auto &param : params_)
    {
        auto paramValue = param.second->getValue(params);
        if (paramValue)
        {
            subParams.emplace(param.first, *paramValue);
        }
        else
        {
            LOG_ERROR << "Parameter " << param.first << " not found";
        }
    }
    return subSqlGetter_(name_, subParams);
}

ParamItem AndNode::getValue(const ParamList &params) const
{
    auto leftValue = left_->getValue(params);
    auto rightValue = right_->getValue(params);
    if (!toBool(leftValue))
    {
        return 0;  // false
    }
    return toBool(rightValue);
}

ParamItem OrNode::getValue(const ParamList &params) const
{
    auto leftValue = left_->getValue(params);
    auto rightValue = right_->getValue(params);
    if (toBool(leftValue))
    {
        return 1;  // true
    }
    return toBool(rightValue);
}

ParamItem EQNode::getValue(const ParamList &params) const
{
    auto leftValue = left_->getValue(params);
    auto rightValue = right_->getValue(params);
    if (!leftValue && !rightValue)
    {
        return 1;  // true
    }
    if (!leftValue || !rightValue)
    {
        return 0;  // false
    }
    if (holds_alternative<int32_t>(*leftValue) &&
        holds_alternative<int32_t>(*rightValue))
    {
        return get<int32_t>(*leftValue) == get<int32_t>(*rightValue);
    }
    else if (holds_alternative<string>(*leftValue) &&
             holds_alternative<string>(*rightValue))
    {
        return get<string>(*leftValue) == get<string>(*rightValue);
    }
    else if (holds_alternative<Json::Value>(*leftValue) &&
             holds_alternative<Json::Value>(*rightValue))
    {
        return get<Json::Value>(*leftValue) == get<Json::Value>(*rightValue);
    }
    return 0;  // false
}

ParamItem NEQNode::getValue(const ParamList &params) const
{
    auto leftValue = left_->getValue(params);
    auto rightValue = right_->getValue(params);
    if (!leftValue && !rightValue)
    {
        return 0;  // false
    }
    if (!leftValue || !rightValue)
    {
        return 1;  // true
    }
    if (holds_alternative<int32_t>(*leftValue) &&
        holds_alternative<int32_t>(*rightValue))
    {
        return get<int32_t>(*leftValue) != get<int32_t>(*rightValue);
    }
    else if (holds_alternative<string>(*leftValue) &&
             holds_alternative<string>(*rightValue))
    {
        return get<string>(*leftValue) != get<string>(*rightValue);
    }
    else if (holds_alternative<Json::Value>(*leftValue) &&
             holds_alternative<Json::Value>(*rightValue))
    {
        return get<Json::Value>(*leftValue) != get<Json::Value>(*rightValue);
    }
    return 1;  // true
}

ParamItem IfStmtNode::getValue(const ParamList &params) const
{
    auto condition = condition_->getValue(params);
    if (toBool(condition))
    {
        return ifStmt_->generateSql(params);
    }
    for (const auto &elseIfStmt : elIfStmts_)
    {
        auto elseIfCondition = elseIfStmt.first->getValue(params);
        if (toBool(elseIfCondition))
        {
            return elseIfStmt.second->generateSql(params);
        }
    }
    if (elseStmt_)
    {
        return elseStmt_->generateSql(params);
    }
    return nullopt;
}

void Parser::printTokens()
{
    reset();
    size_t parenDepth = 0;
    auto printToken = [&parenDepth](const Token &token) {
        std::array<string, 31> colors{
            "\033[38;5;46m",   // NormalText
            "\033[38;5;208m",  // At
            "\033[38;5;105m",  // Identifier
            "\033[38;5;105m",  // LParen
            "\033[38;5;226m",  // Assign
            "\033[38;5;46m",   // String
            "\033[38;5;202m",  // Integer
            "\033[38;5;105m",  // Comma
            "\033[38;5;105m",  // RParen
            "\033[38;5;208m",  // Dollar
            "\033[38;5;105m",  // LBrace
            "\033[38;5;105m",  // RBrace
            "\033[38;5;105m",  // Dot
            "\033[38;5;105m",  // LBracket
            "\033[38;5;105m",  // RBracket
            "\033[38;5;201m",  // If
            "\033[38;5;208m",  // And
            "\033[38;5;196m",  // Or
            "\033[38;5;196m",  // Not
            "\033[38;5;226m",  // EQ
            "\033[38;5;226m",  // NEQ
            "\033[38;5;244m",  // Null
            "\033[38;5;201m",  // Else
            "\033[38;5;201m",  // ElIf
            "\033[38;5;201m",  // EndIf
            "\033[38;5;201m",  // For
            "\033[38;5;201m",  // Separator
            "\033[38;5;201m",  // In
            "\033[38;5;201m",  // EndFor
            "\033[38;5;255m",  // Done
            "\033[38;5;196m",  // Unknown
        };
        std::array<string, 3> parenColors{
            "\033[38;5;105m",
            "\033[38;5;214m",
            "\033[38;5;76m",
        };
        if (token.type() != Done)
        {
            parenDepth += (token.type() == LParen || token.type() == LBrace ||
                           token.type() == LBracket);
            auto color = colors[token.type()];
            if (token.type() == LParen || token.type() == RParen ||
                token.type() == LBrace || token.type() == RBrace ||
                token.type() == LBracket || token.type() == RBracket ||
                token.type() == Dot || token.type() == Comma)
            {
                color = parenColors[parenDepth % parenColors.size()];
            }
            cout << color << "[" << token.type() << "]"
                 << "\033[0m";
            if (token.value() != "")
            {
                cout << "<" << color << token.value()
                     << "\033[0m"
                        ">";
            }
            cout << endl;
            parenDepth -= (token.type() == RParen || token.type() == RBrace ||
                           token.type() == RBracket);
        }
    };
    for (; !lexer_.done(); nextToken())
    {
        printToken(ahead_.front());
    }
    printToken(ahead_.front());
    printToken(ahead_.back());
}

void Parser::printAST()
{
    reset();
    if (!root_)
    {
        root_ = sql();
        if (!lexer_.done())
        {
            throw runtime_error("Invalid expression.");
        }
    }
    cout << "\033[37m"
         << "[root]"
         << "\033[0m" << endl;
    root_->print("└── ");
}

string Parser::parse()
{
    reset();
    if (!root_)
    {
        root_ = sql();
        if (!lexer_.done())
        {
            throw runtime_error("Invalid expression.");
        }
    }
    return root_->generateSql(params_);
}

// sql ::= [NormalText] {(sub_sql|print_expr|if_stmt|for_loop) [NormalText]}
ASTNodePtr Parser::sql()
{
    ASTNodePtr head;
    ASTNodePtr tail;
    auto addNode = [&](const ASTNodePtr &node) {
        if (head)
        {
            tail->setNextSibling(node);
            tail = node;
        }
        else
        {
            head = tail = node;
        }
    };
    if (ahead_[0].type() == NormalText)
    {
        auto normalText = match(NormalText);
        auto normalTextNode = make_shared<NormalTextNode>(normalText);
        addNode(normalTextNode);
    }
    while (true)
    {
        if (ahead_[0].type() == At)
        {
            if (ahead_[1].type() == Identifier)
            {
                addNode(subSql());
            }
            else if (ahead_[1].type() == If)
            {
                addNode(ifStmt());
            }
            else if (ahead_[1].type() == For)
            {
                addNode(forLoop());
            }
            else
            {
                return head;
            }
        }
        else if (ahead_[0].type() == Dollar)
        {
            addNode(printExpr());
        }
        else
        {
            return head;
        }
        if (ahead_[0].type() == NormalText)
        {
            auto normalText = match(NormalText);
            auto normalTextNode = make_shared<NormalTextNode>(normalText);
            addNode(normalTextNode);
        }
    }
}

// print_expr ::= "$" "{" expr "}"
ASTNodePtr Parser::printExpr()
{
    match(Dollar);
    match(LBrace);
    auto result = expr();
    match(RBrace);
    return result;
}

// expr ::= 'null' | Integer | String | Identifier {param_suffix}
ASTNodePtr Parser::expr()
{
    if (ahead_[0].type() == Null)
    {
        match(Null);
        return make_shared<NullNode>();
    }
    else if (ahead_[0].type() == Integer)
    {
        auto integer = fromString<int32_t>(match(Integer));
        return make_shared<NumberNode>(integer);
    }
    else if (ahead_[0].type() == String)
    {
        return make_shared<StringNode>(match(String));
    }
    else if (ahead_[0].type() == Identifier)
    {
        auto paramName = match(Identifier);
        ASTNodePtr variableNode = make_shared<VariableNode>(paramName);
        while (ahead_[0].type() == Dot || ahead_[0].type() == LBracket)
        {
            paramSuffix(variableNode);
        }
        return variableNode;
    }
    throw runtime_error("Invalid expression. Unexpected token: " +
                        to_string(ahead_[0].type()));
}

// param_suffix ::= "[" expr "]" | "." Identifier
void Parser::paramSuffix(ASTNodePtr &param)
{
    if (ahead_[0].type() == LBracket)
    {
        match(LBracket);
        auto indexNode = expr();
        match(RBracket);
        param = make_shared<ArrayNode>(param, indexNode);
    }
    else if (ahead_[0].type() == Dot)
    {
        match(Dot);
        auto memberName = match(Identifier);
        param =
            make_shared<MemberNode>(param, make_shared<StringNode>(memberName));
    }
}

// sub_sql ::= "@" Identifier "(" [param_list] ")"
ASTNodePtr Parser::subSql()
{
    match(At);
    auto subSqlName = match(Identifier);
    match(LParen);
    unordered_map<string, ASTNodePtr> params = {};
    if (ahead_[0].type() == Identifier)
    {
        params = paramList();
    }
    match(RParen);
    return make_shared<SubSqlNode>(subSqlName, subSqlGetter_, params);
}

// param_list ::= param_item { "," param_item }
unordered_map<string, ASTNodePtr> Parser::paramList()
{
    unordered_map<string, ASTNodePtr> result;
    result.emplace(paramItem());
    while (true)
    {
        if (ahead_[0].type() == Comma)
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
pair<string, ASTNodePtr> Parser::paramItem()
{
    auto paramName = match(Identifier);
    ASTNodePtr param;
    if (ahead_[0].type() == Assign)
    {
        match(Assign);
        param = paramValue();
    }
    else
    {
        param = make_shared<VariableNode>(paramName);
    }
    return {paramName, param};
}

// param_value ::= expr | sub_sql
ASTNodePtr Parser::paramValue()
{
    if (ahead_[0].type() == At)
    {
        return subSql();
    }
    else
    {
        return expr();
    }
}

// if_stmt ::= "@" "if" "(" bool_expr ")" sql
//            {"@" "elif" "(" bool_expr ")" sql}
//            ["@" "else" sql]
//             "@" "endif"
ASTNodePtr Parser::ifStmt()
{
    match(At);
    match(If);
    match(LParen);
    auto condition = boolExpr();
    match(RParen);
    auto ifStmt = sql();
    vector<pair<ASTNodePtr, ASTNodePtr>> elIfStmts;
    while (ahead_[0].type() == At && ahead_[1].type() == ElIf)
    {
        match(At);
        match(ElIf);
        match(LParen);
        auto elIfCondition = boolExpr();
        match(RParen);
        auto elIfStmt = sql();
        elIfStmts.emplace_back(elIfCondition, elIfStmt);
    }
    ASTNodePtr elseStmt;
    if (ahead_[0].type() == At && ahead_[1].type() == Else)
    {
        match(At);
        match(Else);
        elseStmt = sql();
    }
    match(At);
    match(EndIf);
    auto ifNode = make_shared<IfStmtNode>(condition, ifStmt, elseStmt);
    for (const auto &elIfStmt : elIfStmts)
    {
        ifNode->addElIfStmt(elIfStmt.first, elIfStmt.second);
    }
    return ifNode;
}

// bool_expr ::= term {("or"|"||") term}
ASTNodePtr Parser::boolExpr()
{
    auto root = term();
    while (true)
    {
        if (ahead_[0].type() == Or)
        {
            match(Or);
            root = make_shared<OrNode>(root, term());
        }
        else
        {
            return root;
        }
    }
}

// term ::= factor {("and"|"&&") factor}
ASTNodePtr Parser::term()
{
    auto root = factor();
    while (true)
    {
        if (ahead_[0].type() == And)
        {
            match(And);
            root = make_shared<AndNode>(root, factor());
        }
        else
        {
            return root;
        }
    }
}

// factor ::= ["!"|"not"] ("(" bool_expr ")" | comp_expr)
ASTNodePtr Parser::factor()
{
    auto isNegated = false;
    if (ahead_[0].type() == Not)
    {
        match(Not);
        isNegated = true;
    }
    if (ahead_[0].type() == LParen)
    {
        match(LParen);
        auto boolNode = boolExpr();
        match(RParen);
        if (isNegated)
        {
            return make_shared<NotNode>(boolNode);
        }
        return boolNode;
    }
    auto compNode = compExpr();
    if (isNegated)
    {
        return make_shared<NotNode>(compNode);
    }
    return compNode;
}

// comp_expr ::= expr [("=="|"!=") expr]
ASTNodePtr Parser::compExpr()
{
    auto result1 = expr();
    ASTNodePtr result2;
    if (ahead_[0].type() == EQ)
    {
        match(EQ);
        result2 = expr();
        return make_shared<EQNode>(result1, result2);
    }
    else if (ahead_[0].type() == NEQ)
    {
        match(NEQ);
        result2 = expr();
        return make_shared<NEQNode>(result1, result2);
    }
    // `param` means `param != null`
    return result1;
}

// for_loop ::= "@" "for" "("
//              (Identifier|"(" Identifier "," Identifier ")") "in" expr
//              ["," "separator" "=" String]
//              ")" sql "@" "endif"
ASTNodePtr Parser::forLoop()
{
    match(At);
    match(For);
    match(LParen);
    string varName;
    string indexName;
    if (ahead_[0].type() == LParen)
    {
        match(LParen);
        varName = match(Identifier);
        match(Comma);
        indexName = match(Identifier);
        match(RParen);
    }
    else if (ahead_[0].type() == Identifier)
    {
        varName = match(Identifier);
    }
    match(In);
    auto collection = this->expr();
    ASTNodePtr separator{nullptr};
    if (ahead_[0].type() == Comma)
    {
        match(Comma);
        match(Separator);
        match(Assign);
        separator = make_shared<StringNode>(match(String));
    }
    match(RParen);
    auto loopBody = sql();
    match(At);
    match(EndFor);
    return make_shared<ForLoopNode>(
        varName, indexName, collection, separator, loopBody);
}

string Parser::match(TokenType type)
{
    assert(ahead_[0].type() == type);
    auto value = ahead_[0].value();
    nextToken();
    return value;
}

void Parser::nextToken()
{
    ahead_.pop_front();
    ahead_.emplace_back(lexer_.next());
}

ParamItem Parser::getParamByName(const string &paramName) const
{
    auto it = params_.find(paramName);
    if (it == params_.end())
    {
        LOG_ERROR << "parameter \"" << paramName << "\" not found";
        return nullopt;
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

void SqlGenerator::printTokens(const string &name, const string &subSqlName)
{
    prepareParser(name, subSqlName);
    cout << "Tokens for " << name << "." << subSqlName << ":\n";
    parsers_[name].at(subSqlName).printTokens();
}

void SqlGenerator::printAST(const string &name, const string &subSqlName)
{
    prepareParser(name, subSqlName);
    cout << "AST for " << name << "." << subSqlName << ":\n";
    parsers_[name].at(subSqlName).printAST();
}

string SqlGenerator::getSql(const string &name, const ParamList &params)
{
    assert(sqls_.isMember(name));
    const auto &item = sqls_[name];
    assert(item.isString() ||
           (item.isMember("main") &&
            (item["main"].isString() || item["main"].isObject())));
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
    prepareParser(name, subSqlName);

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

    parsers_[name].at(subSqlName).setParams(params);
    return parsers_[name].at(subSqlName).parse();
}

string SqlGenerator::getSimpleSql(const string &name, const ParamList &params)
{
    prepareParser(name, "main");
    parsers_[name].at("main").setParams(params);
    return parsers_[name].at("main").parse();
}

void SqlGenerator::prepareParser(const string &name, const string &subSqlName)
{
    if (parsers_.find(name) == parsers_.end())
    {
        parsers_.emplace(name, unordered_map<string, Parser>());
    }

    // If the parser has been set, use it directly
    if (parsers_[name].find(subSqlName) != parsers_[name].end())
    {
        return;
    }

    // If the parser has not been set, create a new one
    if (sqls_[name].isString())
    {
        if (subSqlName == "main")
        {
            parsers_[name].emplace(subSqlName, Parser(sqls_[name].asString()));
        }
    }
    else if (sqls_[name].isObject() && sqls_[name].isMember(subSqlName))
    {
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
        }
        parsers_[name].emplace(subSqlName, Parser(sql));
        parsers_[name]
            .at(subSqlName)
            .setSubSqlGetter(
                [this, name = std::move(name)](const string &subSqlName,
                                               const ParamList &params) {
                    return getSubSql(name, subSqlName, params);
                });
    }
}
