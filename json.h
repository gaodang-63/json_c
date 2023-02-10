#ifndef _JSON_H_
#define _JSON_H_

#define SPLIT ':'

#define JSON_NOT_FOUND_ERROR

/**
 * @brief json_value 类型
 *
 * Json 值为指向json结构体的指针，可理解为子对象
 *
 * Mix 为混合列表，值为json指针，但key无意义
 *
 * Strings 值为字符串指针数组，数组末尾为NULL
 * Jsons 值为json结构体指针的数组，数组末尾为NULL
 *
 * json_value_type 在Ints和Ints_end之间的为整数数组
 * json_value_type - Ints 代表数组长度
 * Bools和Floats类似
 */
enum json_value_type {
  json_Null,
  json_Int,
  json_Float,
  json_Bool,
  json_String,
  json_Json,
  json_Mix,
  json_Strings,
  json_Jsons,
  json_Ints,
  json_Ints_end = 0x10000000,
  json_Floats,
  json_Floats_end = 0x20000000,
  json_Bools,
  json_Bools_end = 0x30000000,
};

/**
 * @brief json 值的union
 *
 */
union json_value {
  long Int;
  double Float;
  bool Bool;
  char *String;
  struct json *Json;
  struct json *Mix;
  char **Strings;
  struct json **Jsons;
  long *Ints;
  double *Floats;
  bool *Bools;
};

/**
 * @brief json树的单元
 *
 */
struct json {
  struct json *next;
  enum json_value_type value_type;
  union json_value value;
  char *key;
};
typedef struct json json;

/**
 * @brief 从字符串中解析json
 *
 * @param s
 * @return json* 返回解析后的根节点
 * 若失败返回NULL
 */
json *json_parse(char *s);

/**
 * @brief 释放json树的内存
 *
 * @param root json树的根节点
 */
void json_free(json *root);

#endif