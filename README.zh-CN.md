# SqlGenerator 插件文档

[English](./README.md) | 简体中文

## 概述

SqlGenerator 是一个动态 SQL 语句生成器，支持参数替换和包含子 SQL 语句。通过在 Json 配置文件中定义 SQL 语句，它允许更灵活和模块化的编写 SQL 语句。

### 灵感来源

[鱼皮](https://github.com/liyupi) 在 2022 年使用 TypeScript 和 Vue.js 编写的一个项目：[sql-generator](https://github.com/liyupi/sql-generator)。

## 主要类和功能

SqlGenerator 主要由四个类组成：

- `Token` ：表示 SQL 语句中的一个标记。
- `Lexer` ：将 SQL 语句分割成 Token 序列。
- `Parser` ：处理 Token 序列以生成最终的 SQL 语句，包括参数替换和包含子 SQL 语句。
- `SqlGenerator` ：插件本体，继承自 `drogon::Plugin<SqlGenerator>` 。

## 使用指南

### 安装

确保您已经在开发环境中安装并正确配置了 [drogon](https://github.com/drogonframework/drogon) 框架。

要安装 SqlGenerator 插件，可以按照以下步骤执行：

**下载代码**

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

**compile_flags.txt（可选）**

```
-I./plugins/SqlGenerator/src
```

### 配置

以下是一个在Yaml文件配置此插件的示例：

```yaml
plugins:
  - name: tl::sql::SqlGenerator
    config:
      sqls:
        # 简单的SQL语句可以直接编写
        count_user: SELECT COUNT(*) FROM user
        # 调用时可以传递参数，并将直接替换
        get_user_by_id: SELECT * FROM user WHERE id = ${user_id}
        # 可以调用子 SQL 并传递参数
        get_user_by_name:
          main: SELECT * FROM user WHERE name LIKE @name_with_wildcard(raw_name=name)
          name_with_wildcard: "'%${raw_name}%'"
        # 参数可以有默认值
        get_height_difference:
          main: SELECT @height_difference() FROM (@student_table()) s1, (@student_table(id = 2)) s2
          height_difference: (s1.height - s2.height) as height_difference
          student_table:
            sql: SELECT * FROM student WHERE id = ${id}
            params:
              id: 1 # id 参数的默认值
        # 更复杂的例子：结构更清晰，便于编写复杂的 SQL
        get_menu_with_submenu:
          main: WITH RECURSIVE menu_tree AS (@recursive_query(id=menu_id)) SELECT * FROM menu_tree
          # @root_node(id=id) 可以简写为 @root_node(id)
          recursive_query:
            sql: "@root_node(id=id, column_names) UNION ALL @child_nodes(column_names)"
            params:
              column_names:
                - id
                - parent_id
          root_node: SELECT @for(column_name in column_names, separator=',') ${column_name} @endfor FROM menu WHERE id=${id}
          child_nodes: SELECT @for(column_name in column_names, separator=',') m.${column_name} @endfor FROM menu m INNER JOIN menu_tree mt ON m.parent_id = mt.id
```

### 生成 SQL 语句

你可以使用 `SqlGenerator` 类的 `getSql` 方法获取 SQL 语句。该方法接受 SQL 语句的名称和一个可选的参数列表。

```cpp
#include <drogon/drogon.h>
#include <SqlGenerator.h>

using namespace ::drogon;
using namespace ::tl::sql;

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

### 语法

定义 SQL 语句时，可以使用以下语法：

- 参数替换：使用 `${paramName}` 进行参数替换。
- 包含子 SQL：使用 `@subSqlName(param1=value1, param2=value2)` 包含子 SQL 语句，可以传参。
  - `subSqlName(param = param)` 可以简写为 `subSqlName(param)` 。
- 条件判断：使用 `@if(condition1) true_statement1 @elif(condition2) true_statement2 @else false_statement @endif` 进行条件判断。
  - 条件表达式可以使用 `and` 、 `or` 、 `not` 、 `&&` 、 `||` 、 `!` 、 `(` 、 `)` 、 `==` 、 `!=` 运算符。
  - 条件表达式可以使用 `param == null` 进行空值判断，可以简写为 `param` 。
- 循环语句：使用 `@for((item, index) in list, separator = ',') statement @endfor` 进行循环。
  - 其中 `index` `separator` 都是可选参数。
  - 当 `list` 为对象时， `index` 为属性名，当 `list` 为数组时，` index` 为数组下标。

本文档提供了使用 `SqlGenerator` 插件动态生成 SQL 语句（支持参数替换和子SQL包含）的基本概述。
