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
aux_source_directory(plugins/SqlGenerator SQL_SRC)
# ...
target_sources(${PROJECT_NAME}
               PRIVATE
               # ...
               ${SQL_SRC})
```

**compile_flags.txt(Optional)**

```
-I./plugins/SqlGenerator
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
          main: SELECT * FROM user WHERE name LIKE @name_with_wildcard(raw_name=${name})
          name_with_wildcard: "'%${raw_name}%'"
        # Param can have a default value
        get_height_difference:
          main: SELECT @height_difference() FROM (@student_table()) s1, (@student_table(id = "2")) s2 # param value must be string
          height_difference: (s1.height - s2.height) as height_difference
          student_table:
            sql: SELECT * FROM student WHERE id = ${id}
            params:
              id: "1" # Default value for id parameter
        # More complex example: structure is clearer when writing complex SQL
        get_menu_with_submenu:
          main: WITH RECURSIVE menu_tree AS (@recursive_query(id=${menu_id})) SELECT * FROM menu_tree
          recursive_query: "@root_node(id=${id}) UNION ALL @child_nodes()"
          root_node: SELECT @root_node_columns() FROM menu WHERE id=${id}
          root_node_columns: id, parent_id, ...
          child_nodes: SELECT @child_node_columns() FROM menu m INNER JOIN menu_tree mt ON m.parent_id = mt.id
          child_node_columns: m.id, m.parent_id, ...
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
                                            {{"menu_id", "1"}});
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
- Sub-SQL Inclusion: Use `@subSqlName(param1=value1, param2=value2)` to include sub-SQL statements with parameters. Parameters' type only can be string.

This document provides a basic overview of how to use the `SqlGenerator` plugin to dynamically generate SQL statements with parameter substitution and sub-SQL inclusion.
