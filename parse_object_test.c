#include "json.c"
#include "json.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief 解析json对象
 *
 * @param s 从*s字符串中解析，确保**s为`{`
 * 并修改*s指向json对象结束的下一个字符
 * @return json* 返回该json对象的首个item的指针
 */
static json *parse_object(char **s) {
  char *str = *s;
  if (*str != '{')
    return NULL;
  str++;
  json *ret = NULL;
  json *head;
  do {
    // 忽略 `,`
    while (*str == ','){
      str++;
      str = skip(str);
    }

    // 解析 key
    if (*str == '}')
      break;
    if (*str != '"')
      return NULL;
    char *key = parse_str(&str);
    if (!str)
      return NULL;

    // 检测语法 `:`
    str = skip(str);
    if (*str != ':') {
      free(key);
      return NULL;
    }
    str++;

    // 创建json节点
    if (ret)
      head = head->next = json_create();
    else
      ret = head = json_create();
    head->key = key;

    // 解析 value
    str = skip(str);

    if (*str == '"') {
      // value 为字符串
      head->value_type = json_String;
      head->value.String = parse_str(&str);

    } else if ((*str >= '0' && *str <= '9') || *str == '-') {
      // value 为数字类型

      // 审查数字是否为浮点类型，并将str推向数字后
      char *temp = str;
      bool isfloat = false;
      while (str++) {
        if (*str >= '0' && *str <= '9' || *str == '-')
          continue;
        else if (*str == '.' || *str == 'e' || *str == 'E')
          isfloat = true;
        else
          break;
      }

      // 将数字转化为json的值
      if (isfloat) {
        head->value_type = json_Float;
        head->value.Float = atof(temp);
      } else {
        head->value_type = json_Int;
        head->value.Int = atol(temp);
      }

    } else if (*str == '{') {
      // value 为 object
      head->value_type = json_Json;
      head->value.Json = parse_object(&str);

    } else if (*str == '[') {
      // value 为 array
      parse_array(&str, head);

    } else {
      // value 为 null 或 bool 类型
      if (!strncmp("null", str, 4)) {
        head->value_type = json_Null;
        str += 4;
      } else if (!strncmp("true", str, 4)) {
        head->value_type = json_Bool;
        head->value.Bool = true;
        str += 4;
      } else if (!strncmp("false", str, 5)) {
        head->value_type = json_Bool;
        head->value.Bool = false;
        str += 5;
      }
    }

    str = skip(str);
  } while (*str == ',');
  head->next = NULL;

  if (*str == '}') {
    *s = str + 1;
    return ret;
  }
  *s = str;
  return ret;
}

int main(void) {
  char *s = "{\"nice\":{"
            "\"hello\":\"world\","
            "\"nice\":true,"
            "\t"
            "     "
            "\"emmm\":null,"
            "\"number\":3346,"
            "\"pi\":0.31415926E1,"
            "}";
  json *root = parse_object(&s);
  return 0;
}