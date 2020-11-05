#include <stdio.h>
#include <locale.h>
#include "ril.h"

RIL_FUNC(custom_gotofile, vm)
{
  int result = RIL_CALLFUNC(gotofile)(vm);
  ril_tag_t *tag = ril_getregisteredtag(vm, "init", NULL);

  if (NULL != tag)
  {
    ril_state_t *state = ril_newstate(vm);
    ril_setstate(state);
    ril_callmacro(vm, tag);
    ril_deletestate(state);
  }

  return result;
}

int main(int argc, char *argv[])
{
  RILVM vm;
  ril_tag_t *tag;
#ifdef _WIN32
  char c;
#endif

  setlocale(LC_CTYPE, "");

  if (argc < 2)
  {
    printf("ex. %s test.ril\n", argv[0]);
  }
  else
  {
    vm = ril_open();
    // set custom execute handlers
    tag = ril_getregisteredtag(vm, "goto", "file");
    ril_setexecutehandler(tag, RIL_CALLFUNC(custom_gotofile));
    tag = ril_getregisteredtag(vm, "goto", "label, file");
    ril_setexecutehandler(tag, RIL_CALLFUNC(custom_gotofile));

    if (RIL_FAILED(ril_loadfile(vm, argv[1]))) return -1;
    ril_execute(vm);
    ril_close(vm);
  }

#ifdef _WIN32
  printf("\nTo exit, please press the ENTER key.\n");
  for(c = 0; (int)c != 10 && c != EOF; c = getchar());
#endif

  return 0;
}
