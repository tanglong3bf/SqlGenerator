#include <json/value.h>
#include <json/reader.h>
#include <fstream>
#include <iostream>

#include "../src/SqlGenerator.h"

using namespace ::std;
using namespace ::tl::sql;

int main()
{
    Json::Value config;
    ifstream ifs;
    ifs.open("./config.json");

    if (!ifs.is_open())
    {
        cerr << "Failed to open config.json" << endl;
        return 1;
    }

    Json::CharReaderBuilder builder;
    JSONCPP_STRING errs;
    if (!Json::parseFromStream(builder, ifs, &config, &errs))
    {
        cerr << "Failed to parse config.json" << endl;
        cerr << errs << endl;
        return 1;
    }
    SqlGenerator sqlGenerator;
    sqlGenerator.initAndStart(config);

    auto printTokens = [&sqlGenerator](const std::string& name,
                                       const string& subSqlName = "main") {
        sqlGenerator.printTokens(name, subSqlName);
    };

    auto printAST = [&sqlGenerator](const std::string& name,
                                    const string& subSqlName = "main") {
        sqlGenerator.printAST(name, subSqlName);
    };

    auto getSqlAndPrint = [&sqlGenerator](const std::string& name,
                                          const ParamList& param = {}) {
        auto sql = sqlGenerator.getSql(name, param);
        std::cout << "SQL of " << name << ": " << std::endl;
        std::cout << "\033[92m" << sql << "\033[0m" << std::endl;
    };

    printTokens("count_user");
    printAST("count_user");
    getSqlAndPrint("count_user");

    printTokens("get_user_by_id");
    printAST("get_user_by_id");
    getSqlAndPrint("get_user_by_id", {{"user_id", 1}});

    printTokens("get_user_paginated");
    printAST("get_user_paginated");
    getSqlAndPrint("get_user_paginated", {{"limit", 10}, {"offset", 300}});

    printTokens("insert_user");
    printAST("insert_user");
    getSqlAndPrint("insert_user", {{"username", string("zhangsan")}});

    printTokens("get_height_more_than_avg");
    printAST("get_height_more_than_avg");
    printTokens("get_height_more_than_avg", "get_avg_height");
    printAST("get_height_more_than_avg", "get_avg_height");
    getSqlAndPrint("get_height_more_than_avg");

    printTokens("sub_sql_param");
    printAST("sub_sql_param");
    printTokens("sub_sql_param", "level1");
    printAST("sub_sql_param", "level1");
    getSqlAndPrint("sub_sql_param", {{"param", string("param")}});

    printTokens("deep_param");
    printAST("deep_param");
    printTokens("deep_param", "level1");
    printAST("deep_param", "level1");
    printTokens("deep_param", "level2");
    printAST("deep_param", "level2");
    getSqlAndPrint("deep_param", {{"param", string("param")}});

    printTokens("ignore_param");
    printAST("ignore_param");
    printTokens("ignore_param", "level1");
    printAST("ignore_param", "level1");
    printTokens("ignore_param", "level2");
    printAST("ignore_param", "level2");
    getSqlAndPrint("ignore_param", {{"param", string("ignore_param")}});

    printTokens("object_param");
    printAST("object_param");
    Json::Value param;
    param["province"] = "hlj";
    param["city"] = "sfh";
    getSqlAndPrint("object_param", {{"address", param}});

    printTokens("array_param");
    printAST("array_param");
    Json::Value param2;
    param2[0] = "hlj";
    param2[1] = "sfh";
    getSqlAndPrint("array_param", {{"address", param2}});

    printTokens("array_object_param");
    printAST("array_object_param");
    printTokens("array_object_param", "user_value");
    printAST("array_object_param", "user_value");
    Json::Value param3;
    param3[0]["name"] = "zhangsan";
    param3[0]["address"]["province"] = "hlj";
    param3[0]["address"]["city"] = "sfh";
    param3[1]["name"] = "lisi";
    param3[1]["address"]["province"] = "hlj";
    param3[1]["address"]["city"] = "mdj";
    getSqlAndPrint("array_object_param", {{"users", param3}});

    printTokens("array_object_param_with_array_param");
    printAST("array_object_param_with_array_param");
    printTokens("array_object_param_with_array_param", "user_value");
    printAST("array_object_param_with_array_param", "user_value");
    Json::Value param4;
    param4[0]["name"] = "张三";
    param4[0]["address"][0] = "黑龙江";
    param4[0]["address"][1] = "绥芬河";
    param4[1]["name"] = "李四";
    param4[1]["address"][0] = "黑龙江";
    param4[1]["address"][1] = "牡丹江";
    getSqlAndPrint("array_object_param_with_array_param", {{"users", param4}});

    printTokens("if_else_test");
    printAST("if_else_test");
    getSqlAndPrint("if_else_test");

    printTokens("for_test");
    printAST("for_test");
    getSqlAndPrint("for_test");

    printTokens("for_test2");
    printAST("for_test2");
    getSqlAndPrint("for_test2");

    printTokens("get_menu_with_submenu");
    printAST("get_menu_with_submenu");
    printTokens("get_menu_with_submenu", "recursive_query");
    printAST("get_menu_with_submenu", "recursive_query");
    printTokens("get_menu_with_submenu", "root_node");
    printAST("get_menu_with_submenu", "root_node");
    printTokens("get_menu_with_submenu", "child_nodes");
    printAST("get_menu_with_submenu", "child_nodes");
    getSqlAndPrint("get_menu_with_submenu", {{"menu_id", 1}});

    return 0;
}
