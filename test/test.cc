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

    auto sql = sqlGenerator.getSql("count_user");
    cout << "count_user: " << sql << endl;

    sql = sqlGenerator.getSql("get_user_by_id", {{"user_id", 1}});
    cout << "get_user_by_id: " << sql << endl;

    sql = sqlGenerator.getSql("get_user_paginated",
                              {{"limit", 10}, {"offset", 300}});
    cout << "get_user_paginated: " << sql << endl;

    sql =
        sqlGenerator.getSql("insert_user", {{"username", string("zhangsan")}});
    cout << "insert_user: " << sql << endl;

    sql = sqlGenerator.getSql("get_height_more_than_avg");
    cout << "get_height_more_than_avg: " << sql << endl;

    sql = sqlGenerator.getSql("sub_sql_param", {{"param", string("param")}});
    cout << "sub_sql_param: " << sql << endl;

    sql = sqlGenerator.getSql("deep_param", {{"param", string("param")}});
    cout << "deep_param: " << sql << endl;

    sql = sqlGenerator.getSql("ignore_param",
                              {{"param", string("ignore_param")}});
    cout << "ignore_param: " << sql << endl;

    Json::Value param;
    param["province"] = "hlj";
    param["city"] = "sfh";
    sql = sqlGenerator.getSql("object_param", {{"address", param}});
    cout << "object_param: " << sql << endl;

    Json::Value param2;
    param2[0] = "hlj";
    param2[1] = "sfh";
    sql = sqlGenerator.getSql("array_param", {{"address", param2}});
    cout << "array_param: " << sql << endl;

    Json::Value param3;
    param3[0]["name"] = "zhangsan";
    param3[0]["address"]["province"] = "hlj";
    param3[0]["address"]["city"] = "sfh";
    param3[1]["name"] = "lisi";
    param3[1]["address"]["province"] = "hlj";
    param3[1]["address"]["city"] = "mdj";
    sql = sqlGenerator.getSql("array_object_param", {{"users", param3}});
    cout << "array_object_param: " << sql << endl;

    Json::Value param4;
    param4[0]["name"] = "张三";
    param4[0]["address"][0] = "黑龙江";
    param4[0]["address"][1] = "绥芬河";
    param4[1]["name"] = "李四";
    param4[1]["address"][0] = "黑龙江";
    param4[1]["address"][1] = "八面通";
    sql = sqlGenerator.getSql("array_object_param_with_array_param",
                              {{"users", param4}});
    cout << "array_object_param_with_array_param: " << sql << endl;

    sqlGenerator.printTokens("if_else_test");
    sql = sqlGenerator.getSql("if_else_test");
    cout << "if_else_test: " << sql << endl;

    return 0;
}
