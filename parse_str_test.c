#include <stdint.h>
#include <stdlib.h>

/**
 * @brief 4位hex转为utf8编码字符串
 *
 * @param write 写入对象，并修改指向最后一个写入字符的下一字符
 * @param from 读取对象，从u开始，修改为指向最后一位hex
 */
void hex4ToUtf8(char **write, char **from) {

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
char *parse_str(char **s) {
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
 * @brief 测试parse_str 与 hex4ToUtf8
 *
 * 测试结果正常
 *
 * @return int
 */
int main(void) {
  char *s = "\""
            "\\n\\hello world"
            "\u0d12"
            "\\\\"
            "succeess"
            "\""
            "\u0d12";
  char *ret = parse_str(&s);
  return 0;
}