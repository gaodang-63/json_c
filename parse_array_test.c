#include "json.c"
#include "json.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief 解析元素均为字符串的数组
 *
 * @param s 从s开始解析，应保证*s==[
 * @param nums 元素个数
 * @return union 返回Strings
 */
static union json_value parse_array_strings(char *s, size_t nums) {
  // 初始化Strings
  union json_value ret;
  ret.Strings = malloc(sizeof(char *) * (nums + 1));
  ret.Strings[nums] = NULL;

  // 解析Strings
  char *str = s;
  str++;
  str = skip(str);
  for (size_t i = 0; i < nums; i++) {
    // 忽略`,`
    while (*str == ',') {
      str++;
      str = skip(str);
    }
    ret.Strings[i] = parse_str(&str);
    str = skip(str);
  }
  return ret;
}

/**
 * @brief 解析元素均为json的数组
 *
 * @param s 从s开始解析，应保证*s==[
 * @param nums 元素个数
 * @return union 返回Jsons
 */
static union json_value parse_array_jsons(char *s, size_t nums) {
  // 初始化Jsons
  union json_value ret;
  ret.Strings = malloc(sizeof(json *) * (nums + 1));
  ret.Strings[nums] = NULL;

  // 解析Jsons
  char *str = s;
  str++;
  str = skip(str);
  for (size_t i = 0; i < nums; i++) {
    // 忽略`,`
    while (*str == ',') {
      str++;
      str = skip(str);
    }
    ret.Jsons[i] = parse_object(&str);
    str = skip(str);
  }
  return ret;
}

/**
 * @brief 解析元素均为整数的数组
 *
 * @param s 从s开始解析，应保证*s==[
 * @param nums 元素个数
 * @return union 返回Ints
 */
static union json_value parse_array_ints(char *s, size_t nums) {
  // 初始化Ints
  union json_value ret;
  ret.Ints = malloc(sizeof(long) * nums);

  //解析Ints
  char *str = s;
  str++;
  str = skip(str);
  for (size_t i = 0; i < nums; i++) {
    // 忽略`,`
    while (*str == ',') {
      str++;
      str = skip(str);
    }
    ret.Ints[i] = atol(str);
    while (str++) {
      if (*str >= '0' && *str <= '9' || *str == '-')
        continue;
      else
        break;
    }
    str = skip(str);
  }
  return ret;
}

/**
 * @brief 解析元素均为浮点数的数组
 *
 * @param s 从s开始解析，应保证*s==[
 * @param nums 元素个数
 * @return union 返回Floats
 */
static union json_value parse_array_floats(char *s, size_t nums) {
  // 初始化Floats
  union json_value ret;
  ret.Floats = malloc(sizeof(double) * nums);

  //解析Floats
  char *str = s;
  str++;
  str = skip(str);
  for (size_t i = 0; i < nums; i++) {
    // 忽略`,`
    while (*str == ',') {
      str++;
      str = skip(str);
    }
    ret.Floats[i] = atof(str);
    while (str++) {
      if (*str >= '0' && *str <= '9' || *str == '-' || *str == '.' ||
          *str == 'e' || *str == 'E')
        continue;
      else
        break;
    }
    str = skip(str);
  }
  return ret;
}

/**
 * @brief 解析元素均为布尔的数组
 *
 * @param s 从s开始解析，应保证*s==[
 * @param nums 元素个数
 * @return union 返回Bools
 */
static union json_value parse_array_bools(char *s, size_t nums) {
  // 初始化Bools
  union json_value ret;
  ret.Bools = malloc(sizeof(bool) * nums);

  //解析Bools
  char *str = s;
  str++;
  str = skip(str);
  for (size_t i = 0; i < nums; i++) {
    // 忽略`,`
    while (*str == ',') {
      str++;
      str = skip(str);
    }
    if (!strncmp("true", str, 4)) {
      str += 4;
      ret.Bools[i] = true;
    } else if (!strncmp("flase", str, 5)) {
      str += 5;
      ret.Bools[i] = false;
    }
    str = skip(str);
  }
  return ret;
}

/**
 * @brief 解析混合元素的数组
 * 
 * @param s 从s开始解析，应保证*s==[
 * @param nums 元素个数
 * @return union 返回Mix
 */
static union json_value parse_array_Mix(char *s, size_t nums) {
  char *str = s;
  str++;
  str = skip(str);
  union json_value ret;
  ret.Mix = NULL;
  json *head = NULL;
  for (size_t i = 0; i < nums; i++) {
    // 忽略`,`
    while (*str == ',') {
      str++;
      str = skip(str);
    }
    // 创建json节点
    if (ret.Mix)
      head = head->next = json_create();
    else
      ret.Mix = head = json_create();
    head->key = NULL;

    // 判断元素并解析到json节点
    if (*str == '"') {
      // 数组元素为字符串
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
  }
  head->next = NULL;
  return ret;
}

/**
 * @brief 解析array
 *
 * @param s 从*s字符串中解析，确保**s为`[`
 * 并修改*s指向array对象结束的下一个字符
 * @param item 父json节点指针，将修改value, value_type
 */
static void parse_array(char **s, json *item) {
  char *str = *s;
  if (*str != '[')
    return;
  str++;
  str = skip(str);
  if (*str == ']') {
    item->value_type = json_Mix;
    item->value.Mix = NULL;
    *s = str + 1;
    return;
  }

  // 类型判断
  enum json_value_type type = json_Null;
  size_t nums = 0;
  do {
    // 忽略`,`
    while (*str == ',') {
      str++;
      str = skip(str);
    }

    //判断是否结束
    if (*str == ']')
      break;

    if (*str == '"') {
      // 数组元素为字符串
      str = nest_match_str(str);

      if (type == json_Null || type == json_Strings)
        type = json_Strings;
      else
        type = json_Mix;

    } else if (*str >= '0' && *str <= '9') {
      // 数组元素为数字
      bool isfloat = false;
      while (str++) {
        if (*str >= '0' && *str <= '9' || *str == '-')
          continue;
        else if (*str == '.' || *str == 'e' || *str == 'E')
          isfloat = true;
        else
          break;
      }

      if (isfloat) {
        if (type == json_Null)
          type = json_Floats + 1;
        else if (type >= json_Floats && type <= json_Floats_end)
          type = type + 1;
        else
          type = json_Mix;
      } else {
        if (type == json_Null)
          type = json_Ints + 1;
        else if (type >= json_Ints && type <= json_Ints_end)
          type = type + 1;
        else
          type = json_Mix;
      }

    } else if (*str == '{') {
      // 数组元素为 object
      str = nest_match(str);

      if (type == json_Null || type == json_Jsons)
        type = json_Jsons;
      else
        type = json_Mix;
    } else if (*str == '[') {
      // 数组元素为 array
      str = nest_match(str);
      type = json_Mix;
    } else {
      // 数组元素为 null 或 bool
      if (!strncmp("null", str, 4)) {
        str += 4;
        type = json_Mix;
      } else if (!strncmp("true", str, 4)) {
        str += 4;
        if (type == json_Null)
          type = json_Bools + 1;
        else if (type >= json_Bools && type < json_Bools_end)
          type = type + 1;
        else
          type = json_Mix;
      } else if (!strncmp("false", str, 5)) {
        str += 5;
        if (type == json_Null)
          type = json_Bools + 1;
        else if (type >= json_Bools && type < json_Bools_end)
          type = type + 1;
        else
          type = json_Mix;
      }
    }

    str = skip(str);
    nums++;
  } while (*str == ',');
  if (type == json_Strings)
    item->value = parse_array_strings(*s, nums);
  else if (type == json_Jsons)
    item->value = parse_array_jsons(*s, nums);
  else if (type >= json_Ints && type <= json_Ints_end)
    item->value = parse_array_ints(*s, nums);
  else if (type >= json_Floats && type <= json_Floats_end)
    item->value = parse_array_floats(*s, nums);
  else if (type >= json_Bools && type <= json_Bools_end)
    item->value = parse_array_bools(*s, nums);
  else if (type == json_Mix)
    item->value = parse_array_Mix(*s, nums);

  item->value_type = type;
  if (*str == ']')
    *s = str + 1;
  else
    *s = str;
  return;
}

int main(void) {
  char *s = "[\"args\", \"asdf\"]";
  s = "[3.14, 2.13, 32.1]";
  s = "[1,2,3,4,5,6,7,8]";
  s = "[3,32,3.14]";
  s = "[{\"hello\":\"world\",\"emmm\":\"hello\"},{\"yyds\":3346}]";
  s = "[true,false,false,true]";
  json item;
  parse_array(&s, &item);
  // printf("%s\t%s\t%p", item.value.Strings[0], item.value.Strings[1],
  //  item.value.Strings[2]);

  return 0;
}