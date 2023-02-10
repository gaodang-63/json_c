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
 * @brief 测试skip
 *
 * 测试结果正常
 *
 * @return int
 */
int main(void) {
  char *str = " \n"
              "// hello world\n"
              "/*xiaogou*/"
              "/*/"
              "hello world";
  char *a = skip(str);
  return 0;
}