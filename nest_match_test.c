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

int main(void) {
  char *s = "{"
            "emmm asdfghjkl;"
            "[aemm]"
            "}"
            "hello";
  char *b = nest_match(s);
  return 0;
}