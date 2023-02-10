#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "json.h"

/**
 * @brief 跳过空白和注释
 *
 * @param str 从str指向的位置开始
 * @return char* 距离str最近的下一个非空白字符的指针
 */
static char *skip(char *str) {
continueskip:

  // 跳过除'\0'外所有控制字符和空白
  while (*str <= ' ' && *str)
    str++;

  // 检查是否为注释
  if (*str == '/') {

    // 跳过注释 `//`
    if (*(str + 1) == '/') {
      while (*str != '\n' && *str)
        str++;
      goto continueskip;
    }

    // 跳过注释 `/* */`
    if (*(str + 1) == '*') {
      str += 2;
      while (!(*str == '*' && *(str + 1) == '/') && *str)
        str++;
      if (*str == '*')
        str += 2;
      goto continueskip;
    }
  }
  return str;
}

/**
 * @brief 4位hex转为utf8编码字符串
 *
 * @param write 写入对象，并修改指向最后一个写入字符的下一字符
 * @param from 读取对象，从u开始，修改为指向最后一位hex
 */
static void hex4ToUtf8(char **write, char **from) {

  // 将4个hex字符转为数字hex
  uint32_t hex = 0;
  for (int i = 0; i < 4; i++) {
    char ch = *(++(*from));
    if (ch >= '0' && ch <= '9') {
      hex = (hex << 4) + ch - '0';
    } else if (ch >= 'a' && ch <= 'f') {
      hex = (hex << 4) + ch - 'a' + 10;
    } else if (ch >= 'A' && ch <= 'F') {
      hex = (hex << 4) + ch - 'A' + 10;
    }
  }

  // 将数字hex转化为utf8编码的字符串
  if (hex < 0x00000080) {
    *((*write)++) = hex;
  } else if (hex < 0x00000800) {
    *((*write)++) = hex >> 6 | 0xC0;   // 0xC0 1100 0000
    *((*write)++) = hex & 0x3F | 0x80; // 0x3f 0011 1111
  } else if (hex < 0x00010000) {
    *((*write)++) = hex >> 12 | 0xE0;       // 0xE0 1110 0000
    *((*write)++) = hex >> 6 & 0x3F | 0x80; // 0x80 1000 0000
    *((*write)++) = hex & 0x3F | 0x80;
  }
}

/**
 * @brief 解析字符串并储存
 *
 * @param s 从*s开始读取，确保**s为`"`，并修改*s为这个字符串末尾`"`后
 * @return char* 返回为 malloc，解析后的字符串
 */
static char *parse_str(char **s) {
  char *str = *s;
  if (*str != '"')
    return NULL;
  // 将str修改为字符串开始`"`之后
  str++;
  // 将*s修改为字符串结尾`"`之后
  while (*(++(*s)) != '"' || *((*s) - 1) == '\\')
    if (!**s)
      return NULL;
  (*s)++;

  // 根据估算的字符串最大长度申请内存
  char *ret = malloc(*s - str);

  // 从str解析字符串到ret
  char *write = ret;
  while (*str != '"') {

    if (*str == '\\') {
      str++;
      // 转义字符解码
      switch (*str) {
      case '"':
        *(write++) = '\"';
        break;
      case '\\':
        *(write++) = '\\';
        break;
      case '/':
        *(write++) = '/';
        break;
      case 'b':
        *(write++) = '\b';
        break;
      case 'f':
        *(write++) = '\f';
        break;
      case 'n':
        *(write++) = '\n';
        break;
      case 'r':
        *(write++) = '\r';
        break;
      case 't':
        *(write++) = '\t';
        break;
      case 'u':
        hex4ToUtf8(&write, &str);
      }
      str++;
      continue;
    }
    *(write++) = *(str++);
  }
  *(write++) = '\0';

  // 重新分配大小，释放多余内存
  ret = realloc(ret, write - ret);
  return ret;
}

/**
 * @brief 申请一块内存储存json节点
 *
 * @return json* 如果失败返回NULL
 */
static json *json_create() { return malloc(sizeof(json)); }

/**
 * @brief 从字符串开头的“跳转到结尾”
 *
 * @param str 第一个字符为`"`
 * @return char* 返回指向`"`的下一个字符
 */
static char *nest_match_str(char *str) {
  str++;
  while (*(str++) != '"' || *(str - 1) == '\\')
    continue;
  return str;
}

/**
 * @brief 符号匹配
 *
 * 向后匹配同一层级的符号
 *
 * @param str 第一个字符为匹配对象，向后匹配
 * @return char* 返回匹配到的符号的下一字符的指针
 */
static char *nest_match(char *str) {
  char l = *str;
  char r;
  switch (l) {
  case '[':
    r = ']';
    break;
  case '{':
    r = '}';
    break;
  case '(':
    r = ')';
    break;
  default:
    return str;
  }

  unsigned int level = 0;
  do {
    if (*str == l)
      level++;
    if (*str == r)
      level--;
    if (*str == '"')
      str = nest_match_str(str);
    else
      str++;
  } while (level);
  return str;
}

// 因为parse_object 要调用此函数，提前声明一下
static void parse_array(char **s, json *item);

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
  str = skip(str);
  json *ret = NULL;
  json *head;
  do {
    // 忽略 `,`
    while (*str == ',') {
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

/**
 * @brief 从字符串中解析json
 *
 * @param s
 * @return json* 返回解析后的根节点
 * 若失败返回NULL
 */
json *json_parse(char *s) {
  char *str = s;
  str = skip(str);
  if (*str != '{')
    return NULL;
  json *ret = json_create();
  if (!ret)
    return NULL;
  ret->key = NULL;
  ret->next = NULL;
  ret->value_type = json_Json;
  ret->value.Json = parse_object(&str);
  return ret;
}

/**
 * @brief 释放json树的内存
 *
 * @param root json树的根节点
 */
void json_free(json *root) {
  json *next = root;
  while (next) {
    // 释放key
    free(next->key);

    // 释放value
    if (next->value_type == json_String) {
      free(next->value.String);
    } else if (next->value_type == json_Json) {
      json_free(next->value.Json);
    } else if (next->value_type >= json_Ints &&
               next->value_type <= json_Ints_end) {
      free(next->value.Ints);
    } else if (next->value_type >= json_Floats &&
               next->value_type <= json_Floats_end) {
      free(next->value.Floats);
    } else if (next->value_type >= json_Bools &&
               next->value_type <= json_Bools_end) {
      free(next->value.Bools);
    } else if (next->value_type == json_Strings) {
      for (size_t i = 0; next->value.Strings[i]; i++)
        free(next->value.Strings[i]);
      free(next->value.Strings);
    } else if (next->value_type == json_Jsons) {
      for (size_t i = 0; next->value.Jsons[i]; i++)
        json_free(next->value.Jsons[i]);
      free(next->value.Jsons);
    } else if (next->value_type == json_Mix) {
      json_free(next->value.Mix);
    }

    // 并释放本节点
    json *temp = next;
    next = next->next;
    free(temp);
  }
}

/**
 * @brief 根据key描述该返回值，并通过type告知该值的类型
 *
 * @param key 跳转描述，从key指向字符开始到SPLIT或'\0'结束
 * 描述规则：
 *   - 当key第一个字符为SPLIT时
 *     - 如果为 null，bool，number，string，返回该值。
 *     - 如果为 json 返回第一个值
 *     - 如果为 array 返回第一个元素
 *   - 当base值类型为 json 时，按key到SPLIT或`\0`中的字符串查找值并返回
 *   - 当base值类型为 array 时，
 * 按key到SPLIT或`\0`的字符串atoi(十进制)为数字n并返回第n+1个元素
 * @param base 跳转的基点，从value开始搜索
 * @param type 修改为该返回值的类型
 * @return union
 */
static union json_value jump_value(char *key, json *base,
                                   enum json_value_type *type) {
  char *str = key;
  union json_value ret;
  if (*str == SPLIT) {
    if (base->value_type == json_Null || base->value_type == json_Int ||
        base->value_type == json_Float || base->value_type == json_Bool ||
        base->value_type == json_String || base->value_type == json_Json) {
      *type = base->value_type;
      return base->value;
    } else if (base->value_type == json_Mix) {
      *type = base->value.Mix->value_type;
      return base->value.Mix->value;
    } else if (base->value_type == json_Strings) {
      *type = json_String;
      ret.String = base->value.Strings[0];
      return ret;
    } else if (base->value_type == json_Jsons) {
      *type = json_Json;
      ret.Json = base->value.Jsons[0];
      return ret;
    } else if (base->value_type >= json_Ints &&
               base->value_type <= json_Ints_end) {
      *type = json_Int;
      ret.Int = base->value.Ints[0];
      return ret;
    } else if (base->value_type >= json_Floats &&
               base->value_type <= json_Floats_end) {
      *type = json_Float;
      ret.Float = base->value.Floats[0];
      return ret;
    } else if (base->value_type >= json_Bools &&
               base->value_type <= json_Bools_end) {
      *type = json_Bool;
      ret.Bool = base->value.Bools[0];
      return ret;
    }
  }
  if (base->value_type == json_Json) {
    json *next = base->value.Json;
    size_t strn;
    for (strn = 0; str[strn] != SPLIT && str[strn]; strn++)
      continue;
  find:
    while (strncmp(str, next->key, strn) && next != NULL)
      next = next->next;
    if (!next) {
      JSON_NOT_FOUND_ERROR;
    }
    if (next->key[strn] != '\0') {
      next = next->next;
      goto find;
    }
    *type = next->value_type;
    return next->value;
  }
  if (base->value_type == json_Mix) {
    int n = atoi(str);
    if (n < 0) {
      JSON_NOT_FOUND_ERROR;
    }
    json *next = base->value.Mix;
    for (int i = n; i && next; i--)
      next = next->next;
    if (!next) {
      JSON_NOT_FOUND_ERROR;
    }
    *type = next->value_type;
    return next->value;
  }
  if (base->value_type == json_Strings) {
    int n = atoi(str);
    if (n < 0) {
      JSON_NOT_FOUND_ERROR;
    }
    char **strs = base->value.Strings;
    size_t i = 0;
    while (strs[i])
      i++;
    if (n > i) {
      JSON_NOT_FOUND_ERROR;
    }
    *type = json_String;
    ret.String = strs[n];
    return ret;
  }
  if (base->value_type == json_Jsons) {
    int n = atoi(str);
    if (n < 0) {
      JSON_NOT_FOUND_ERROR;
    }
    json **jsons = base->value.Jsons;
    size_t i = 0;
    while (jsons[i]) {
      i++;
    }
    if (n > i) {
      JSON_NOT_FOUND_ERROR;
    }
    *type = json_Json;
    ret.Json = jsons[n];
    return ret;
  }
  *type = json_Null;
  return ret;
}

/**
 * @brief 根据key的描述返回该节点
 * 操作对象为 json
 *
 * @param key 跳转描述，从key指向字符开始到SPLIT或'\0'结束
 * - 当base值类型为 json 时，按key到SPLIT或`\0`中的字符串查找值并返回
 * - 若str指向 SPLIT 则返回第一个json节点
 * @param base 跳转基点，从value开始搜索
 * @return json* key匹配的json节点
 */
static json *jump(char *key, json *base) {
  char *str = key;
  if (*str == SPLIT || *str == '\0') {
    if (base->value_type == json_Json) {
      return base->value.Json;
    }
  }
  if (base->value_type == json_Json) {
    json *next = base->value.Json;
    size_t strn;
    for (strn = 0; str[strn] != SPLIT && str[strn]; strn++)
      continue;
  find:
    while (strncmp(str, next->key, strn) && next != NULL)
      next = next->next;
    if (!next) {
      JSON_NOT_FOUND_ERROR;
    }
    if (next->key[strn] != '\0') {
      next = next->next;
      goto find;
    }
    return next;
  }
  JSON_NOT_FOUND_ERROR;
  return NULL;
}

/**
 * @brief 根据key返回对应的字符串
 *
 * @param key 键
 * @param base 待操作json树
 * @return char* 返回key对应的字符串
 */
char *json_read_str(char *key, json *base) {
  char *str = key;
  json *item = NULL;
  size_t strn;
  for (strn = 0; str[strn] != SPLIT && str[strn]; strn++)
    continue;
  if (base->value_type == json_Json) {
    item = jump(key, base);
  } else if (base->value_type == json_Jsons) {
    int n = atoi(str);
    if (n < 0) {
      JSON_NOT_FOUND_ERROR;
    }
    json **items = base->value.Jsons;
    size_t i = 0;
    while (items[i])
      i++;
    if (n >= i) {
      JSON_NOT_FOUND_ERROR;
    }
    item = base->value.Jsons[n];
    str = str + strn + 1;
    return json_read_str(str, item);
  } else {
    return NULL;
  }
  if (str[strn])
    return json_read_str(str + strn + 1, item);
  else
    return item->value.String;
}