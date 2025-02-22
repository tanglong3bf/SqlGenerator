# SqlGenerator Plugin Documentation

English | [简体中文](./README.zh-CN.md)

## Overview

SqlGenerator is a dynamic SQL statement generator that supports parameter substitution and the inclusion of sub-SQL statements. By defining sub-SQL statements in a JSON configuration file, it allows for more flexible and modular SQL statement generation.

### Inspiration Source

A project by [liyupi](https://github.com/liyupi) in 2022 written in TypeScript and Vue.js: [sql-generator](https://github.com/liyupi/sql-generator).

## Main Classes and Features

SqlGenerator is primarily composed of four classes:

- `Token` : Represents a single token in the SQL statement.
- `Lexer` : Breaks down the SQL statement into tokens.
- `Parser` : Processes the tokens to generate the final SQL statement, including parameter substitution and sub-SQL inclusion.
- `SqlGenerator` : The main class for generating SQL statements, inheriting from `drogon::Plugin<SqlGenerator>`.

## Usage Guide

### Installation

Ensure you have the [drogon](https://github.com/drogonframework/drogon) framework installed and properly set up in your development environment.

To install the SqlGenerator plugin, you can use the following steps:

**Download code**

```
cd your_project/plugins
git clone https://github.com/tanglong3bf/SqlGenerator.git
```

**CMakeLists.txt**

```cmake
# ...
aux_source_directory(plugins/SqlGenerator/src SQL_SRC)
# ...
target_sources(${PROJECT_NAME}
               PRIVATE
               # ...
               ${SQL_SRC})
```

**compile_flags.txt(Optional)**

```
-I./plugins/SqlGenerator/src
```

### Configuration

Here is an example of how to configure this plugin in Yaml file:

```yaml
plugins:
  - name: tl::sql::SqlGenerator
    config:
      sqls:
        # Directly write simple SQL statements
        count_user: SELECT COUNT(*) FROM user
        # When called, parameters can be passed and will be directly substituted
        get_user_by_id: SELECT * FROM user WHERE id = ${user_id}
        # Can call sub-SQL and pass parameters
        get_user_by_name:
          main: SELECT * FROM user WHERE name LIKE @name_with_wildcard(raw_name=name)
          name_with_wildcard: "'%${raw_name}%'"
        # Param can have a default value
        get_height_difference:
          main: SELECT @height_difference() FROM (@student_table()) s1, (@student_table(id = 2)) s2
          height_difference: (s1.height - s2.height) as height_difference
          student_table:
            sql: SELECT * FROM student WHERE id = ${id}
            params:
              id: 1 # Default value for id parameter
        # More complex example: structure is clearer when writing complex SQL
        get_menu_with_submenu:
          main: WITH RECURSIVE menu_tree AS (@recursive_query(id=menu_id)) SELECT * FROM menu_tree
          # @root_node(id=id) can be simplified as @root_node(id)
          recursive_query:
            sql: "@root_node(id=id, column_names) UNION ALL @child_nodes(column_names)"
            params:
              column_names:
                - id
                - parent_id
          root_node: SELECT @for(column_name in column_names, separator=',') ${column_name} @endfor FROM menu WHERE id=${id}
          child_nodes: SELECT @for(column_name in column_names, separator=',') m.${column_name} @endfor FROM menu m INNER JOIN menu_tree mt ON m.parent_id = mt.id
```

### Generating SQL Statements

You can get SQL statements using the `getSql` method of the `SqlGenerator` class. This method accepts the name of the SQL statement and an optional map of parameters.

```cpp
#include <drogon/drogon.h>
#include <SqlGenerator.h>

using namespace drogon;
using namespace tl::sql;

int main()
{
    app().registerBeginningAdvice([] {
        auto sqlGenerator = app().getPlugin<SqlGenerator>();
        if (sqlGenerator)
        {
            auto sql = sqlGenerator->getSql("get_menu_with_submenu",
                                            {{"menu_id", 1}});
            LOG_INFO << sql;
        }
    });
    app().loadConfigFile("../config.yaml");
    app().run();
    return 0;
}
```

### Syntax

The SQL statements are defined using a specific syntax:

- Parameter Substitution: Use `$paramName` to substitute parameters.
- Sub-SQL Inclusion: Use `@subSqlName(param1=value1, param2=value2)` to include sub-SQL statements with parameters.
  - `subSqlName(param)` is a shorthand for `@subSqlName(param = param)`.
- Conditional Statement: use `@if(condition1) true_statement1 @elif(condition2) true_statement2 @else false_statement @endif` for conditional statements.
  - Conditional expressions can use `and`, `or`, `not`, `&&`, `||`, `!`, `(`, `)`, `==`, `!=` operators.
  - Conditional expressions can check for null values using `param == null`, which can be simplified to `param`.
- Loop statement: Use `@for((item, index) in list, separator = ',') statement @endfor` for looping.
  - Both `index` and `separator` are optional parameters.
  - When `list` is an object, `index` represents the property name; when `list` is an array, index represents the array index.

This document provides a basic overview of how to use the `SqlGenerator` plugin to dynamically generate SQL statements with parameter substitution and sub-SQL inclusion.
