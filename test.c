#include "json.c"
#include "json.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
  // 初始化json树
  char a[0xFFFF];
  FILE *fp = fopen(".vscode/tasks.json", "r");
  if (fp == NULL)
    puts("hello");
  int temp = 1;
  size_t i;
  for (i = 0; temp != EOF; i++) {
    temp = fgetc(fp);
    a[i] = temp;
  }
  a[i - 1] = '\0';
  json *item = json_parse(a);
  char *e = item->value.Json->next->value.Jsons[0]
                ->next->next->next->value.Strings[1];

  enum json_value_type type;
  char *ee;
  // ee = json_read_str("version", item );
  ee = json_read_str("tasks:0:label", item);

  json_free(item);
  fclose(fp);
  return 0;
}