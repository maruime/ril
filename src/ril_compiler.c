/*	see copyright notice in ril.h */

#include "ril_pcheader.h"
#include "ril_vm.h"
#include "ril_state.h"
#include "ril_compiler.h"
#include "ril_utils.h"
#include "ril_api.h"

typedef struct
{
  ril_cmdid_t cmdid;
  uint32_t line;
} _stack_pair_t;

static __inline uint32_t _labelnametoid(ril_compile_t *c_context, const char *name)
{
  int i;
  uint32_t namehash = ril_makecrc(name);
  ril_label_t *label;
  
  for (i = buffer_size(c_context->label_buffer) - 1; 0 <= i; --i)
  {
    label = buffer_index(c_context->label_buffer, i);
    if (namehash == label->namehash) return i;
  }
  
  i = buffer_size(c_context->label_buffer);
  label = buffer_malloc(c_context->label_buffer, 1);
  label->namehash = namehash;
  label->cmdid = LABEL_NULL;
  
  return i;
}

ril_cmd_t* rilc_newcmd(ril_compile_t *context, ril_signature_t signature)
{
  ril_cmd_t *cmd = buffer_malloc(context->cmd_buffer, 1);
  
  cmd->signature = signature;
  cmd->arg_offset = buffer_size(context->arg_buffer);
  cmd->pair_cmdid.id = buffer_size(context->cmd_buffer) - 1;
  
  context->cmd = cmd;
  context->cmdid.id = cmd->pair_cmdid.id;
  
  return cmd;
}

ril_cmd_t* rilc_addcmd(ril_compile_t *context, ril_signature_t signature)
{
  ril_cmd_t *cmd = rilc_newcmd(context, signature);
  
  // check pair stack
  if (RIL_FAILED(rilc_checkpair(context, cmd))) return NULL;
  
  // check child stack
  if (RIL_FAILED(rilc_checkchild(context, cmd))) return NULL;
  
  return cmd;
}

ril_arg_t* rilc_getarg(ril_compile_t *context, ril_cmd_t *cmd, uint32_t argid)
{
  ril_arg_t *arg_header = (ril_arg_t*)buffer_index(context->arg_buffer, cmd->arg_offset);
  
  return arg_header + argid;
}

const void* rilc_getargument(ril_compile_t *context, uint32_t argid)
{
  ril_arg_t *cmdarg = rilc_getarg(context, context->cmd, argid);
  void *ptr = (uint8_t*)buffer_front(context->data_buffer) + cmdarg->data_offset;

  return ptr;
}

const char* rilc_getstring(ril_compile_t *context, uint32_t argid)
{
  return calc_tostring(context->vm, rilc_getargument(context, argid));
}

void rilc_eraselastcmd(ril_compile_t *context)
{
  int i;
  uint32_t databegin = buffer_size(context->data_buffer);
  
  for (i = buffer_size(context->tag->param_buffer) - 1; 0 <= i; --i)
  {
    ril_arg_t *cmdarg = rilc_getarg(context, context->cmd, i);
    if (cmdarg->data_offset < databegin) databegin = cmdarg->data_offset;
  }
  buffer_erase(context->cmd_buffer, 1);
  buffer_resize(context->data_buffer, databegin);
  --context->cmd;
  if ((void*)context->cmd > buffer_front(context->cmd_buffer)) context->cmd = NULL;
}

void rilc_addarg(ril_compile_t *context)
{
  ril_arg_t *arg = (ril_arg_t*)buffer_malloc(context->arg_buffer, 1);
  arg->data_offset = buffer_size(context->data_buffer);
}

void rilc_addbytes(ril_compile_t *context, const void *value, int size)
{
  rilc_addarg(context);
  calc_writevalue2buffer(context->data_buffer, VARIANT_LITERAL | VARIANT_BYTES, NULL, sizeof(size) + size);
  buffer_write(context->data_buffer, &size, sizeof(size));
  buffer_write(context->data_buffer, value, size);
  calc_writeoperator(context->data_buffer, CALC_END);
}

void rilc_addstring(ril_compile_t *context, const char *value)
{
  rilc_addarg(context);
  calc_writevalue2buffer(context->data_buffer, VARIANT_LITERAL | VARIANT_STRING, value, strlen(value) + 1);
  calc_writeoperator(context->data_buffer, CALC_END);
}

static __inline RILRESULT _compilevar(ril_compile_t *c_context, calc_compile_t *e_context, bool isconst)
{
  char word[128];
  const char *name;
  ril_crc_t namehash;
  bool checksub = false;
  uint32_t *size;
  int dest_beginindex;
  void* calc_begin;
  
  for (; '\0' != *e_context->cur; e_context->cur += 2)
  {
    e_context->cur = ril_getword(word, e_context->cur, false);
    if ('\0' == *word) break;
    
    /* hash (hashkey + rawkey) */
    namehash = ril_makecrc(word);
    calc_writeoperator(e_context->dest_buffer, VAR_HASH);
    buffer_write(e_context->dest_buffer, &namehash, sizeof(namehash));
    buffer_write(e_context->dest_buffer, word, strlen(word) + 1);
    
    /* array */
    for (;;)
    {
      e_context->cur = ril_trimspace(e_context->cur);
      if ('[' != *e_context->cur) break;
      
      e_context->cur = ril_trimspace(e_context->cur + 1);
      if (']' == *e_context->cur)
      {
        calc_writeoperator(e_context->dest_buffer, VAR_ADD);
        ++e_context->cur;
        checksub = true;
        continue;
      }
      
      dest_beginindex = buffer_size(e_context->dest_buffer);
      calc_writeoperator(e_context->dest_buffer, VAR_CALC);
      buffer_malloc(e_context->dest_buffer, sizeof(uint32_t));
      if (RIL_FAILED(calc_compile(c_context, e_context->dest_buffer, e_context->cur)))
      {
        return RIL_ERROR;
      }
      e_context->cur = c_context->cur + 1;
      size = (uint32_t*)buffer_index(e_context->dest_buffer, dest_beginindex + sizeof(calc_opcode_t));
      *size = buffer_bytesize(e_context->dest_buffer) - (dest_beginindex + sizeof(calc_opcode_t) + sizeof(uint32_t));
      
      calc_begin = buffer_index(e_context->dest_buffer, dest_beginindex + sizeof(calc_opcode_t) + sizeof(uint32_t));
      if (1 != calc_countvalue(calc_begin))
      {
        continue;
      }
      if (calc_isvar(calc_begin)) continue;
      
      /* hash (hashkey + rawkey) */
      name = calc_tostring(c_context->vm, calc_begin);
      namehash = ril_makecrc(name);
      buffer_resize(e_context->dest_buffer, dest_beginindex);
      calc_writeoperator(e_context->dest_buffer, VAR_HASH);
      buffer_write(e_context->dest_buffer, &namehash, sizeof(namehash));
      buffer_write(e_context->dest_buffer, name, strlen(name) + 1);
    }
    
    /* not child */
    /* if ('-' != *e_context->cur || '>' != e_context->cur[1]) break; */
    break;
  }
  calc_writeoperator(e_context->dest_buffer, VAR_END);
  
  if (checksub)
  {
    e_context->cur = ril_trimspace(e_context->cur);
    if ('=' != e_context->cur[0] && '=' != e_context->cur[1])
    {
      return ril_error(c_context->vm, "Cannot use [] for reading");
    }
  }
  
  return RIL_OK;
}

// expansion of the calclation
// ex. *label ]
//     x+y ]
RILRESULT calc_cb_compile(calc_compile_t *context, ril_compile_t *c_context)
{
  char word[128], *cur;
  int dest_begin, temp;
  bool isconst = false;
  ril_label_t label;
  
  if (!context->prev_is_operator)
  {
    if (ril_isrightdelimiter(c_context->vm, context->cur))
    {
      c_context->cur = context->cur;
      return CALC_END;
    }
    ril_getword(word, context->cur, false);
    if ('\0' != word[0])
    {
      c_context->cur = context->cur;
      return CALC_END;
    }
    return RIL_OK;
  }
  
  switch (*context->cur)
  {
  case 'n':
    cur = ril_getword(word, context->cur, false);
    ril_str2lower(word, word);
    if (!strcmp("null", word))
    {
      calc_writevalue(context, VARIANT_NULL, NULL, 0);
      context->cur = cur;
      break;
    }
  default: // const
    ril_getword(word, context->cur, false);
    if ('\0' == word[0]) break;
    isconst = true;
    --context->cur;
  case '$': // var
    ++context->cur;
    ril_getword(word, context->cur, false);
    if ('\0' == word[0])
    {
      return ril_error(c_context->vm, "unnecessary '%c'", context->cur[-1]);
    }
    calc_writevalue(context, context->isref ? VARIANT_REFVAR : VARIANT_VAR, NULL, 0);
    dest_begin = buffer_bytesize(context->dest_buffer);
    if (RIL_FAILED(_compilevar(c_context, context, isconst)))
    {
      return RIL_ERROR;
    }
    calc_lastvalue(context)->size = buffer_bytesize(context->dest_buffer) - dest_begin;
    if (context->hasminus)
    {
      temp = -1;
      calc_writevalue(context, VARIANT_INTEGER, &temp, sizeof(int));
      calc_writeoperator(context->dest_buffer, CALC_MULTI);
    }
    return RIL_OK;
  case '*': // label
    ++context->cur;
    context->cur = ril_getword(word, context->cur, false);
    if ('\0' == word[0])
    {
      --context->cur;
      break;
    }
    label.id = _labelnametoid(c_context, word);
    label.namehash = ((ril_label_t*)buffer_index(c_context->label_buffer, label.id))->namehash;
    calc_writevalue(context, VARIANT_LABEL, &label, sizeof(label));
    return RIL_OK;
  }
  
  return RIL_OK;
}

RILRESULT rilc_checkchild(ril_compile_t *context, ril_cmd_t *cmd)
{
  int i, j;
  ril_tag_t *tag = ril_getregisteredtag2(context->vm, cmd->signature);
  ril_tag_t *parenttag;
  ril_cmd_t *parentcmd;
  _stack_pair_t *stack_pair;
  
  if (!tag->hasparent) return RIL_OK;
  
  for (i = -1; ; --i)
  {
    stack_pair = (_stack_pair_t*)stack_index(context->pair_stack, i, NULL);
    if (NULL == stack_pair) break;
    
    parentcmd = (ril_cmd_t*)buffer_index(context->cmd_buffer, stack_pair->cmdid.id);
    parenttag = ril_getregisteredtag2(context->vm, parentcmd->signature);
    
    for (j = buffer_size(parenttag->child_buffer) - 1; 0 <= j; --j)
    {
      ril_childtag_t *childtag = (ril_childtag_t*)buffer_index(parenttag->child_buffer, j);
      if (childtag->tag != tag) continue;
      cmd->parent_cmdid = stack_pair->cmdid;
      return RIL_OK;
    }
  }
  
  return RIL_COMPILE_ERROR(context, "Fatal error: '%s' is not available at this location", context->tagname);
}

RILRESULT rilc_checkpair(ril_compile_t *context, ril_cmd_t *cmd)
{
  ril_tag_t *tag = ril_getregisteredtag2(context->vm, cmd->signature);
  ril_cmdid_t cmdid = cmd->pair_cmdid;
  int i;
  _stack_pair_t *stack_pair;
  bool pair_equired = false;
  
  if ((stack_pair = (_stack_pair_t*)stack_back(context->pair_stack, NULL)))
  {
    ril_cmd_t *paircmd = (ril_cmd_t*)buffer_index(context->cmd_buffer, stack_pair->cmdid.id);
    ril_tag_t *pairtag = ril_getregisteredtag2(context->vm, paircmd->signature);
    bool ispair = false;
    
    for (i = buffer_size(pairtag->pair_buffer) - 1; 0 <= i; --i)
    {
      ril_pairtag_t *pair = (ril_pairtag_t*)buffer_index(pairtag->pair_buffer, i);
      if (pair->tag != tag) continue;
      cmd->pair_cmdid = paircmd->pair_cmdid;
      paircmd->pair_cmdid = cmdid;
      stack_pop(context->pair_stack, NULL);
      ispair = true;
      break;
    }
    if (!ispair && tag->refcount) pair_equired = true;
  }
  else
  {
    if (tag->refcount) pair_equired = true;
  }
  
  if (pair_equired)
  {
    return RIL_COMPILE_ERROR(context, "Parse error: pair tag before '%s' is required", context->tagname);
  }
  
  // add to stack
  if (0 < buffer_size(tag->pair_buffer))
  {
    stack_pair = stack_push(context->pair_stack, NULL);
    stack_pair->cmdid = cmdid;
    stack_pair->line = context->line;
  }
  
  return RIL_OK;
}

static __inline RILRESULT _findtag(ril_compile_t *context, ril_cmd_t *cmd, const char *name, int argc, bool hasnonamearg, const char *args)
{
  ril_tag_t *tag = ril_getregisteredtag3(context->vm, name, args, hasnonamearg, false);
  
  if (NULL == tag) return RIL_ERROR;
  
  cmd->signature = ril_signature(tag);
  context->tag = tag;
  
  return RIL_OK;
}

static __inline RILRESULT _checkargs(ril_compile_t *context, ril_cmd_t *cmd)
{
  int i;
  ril_parameter_t *param;
  ril_arg_t *arg;
  void *src;
  calc_value_t *value;

  for (i = buffer_size(context->tag->param_buffer) - 1; 0 <= i; --i)
  {
    param = (ril_parameter_t*)buffer_index(context->tag->param_buffer, i);
    arg = (ril_arg_t*)buffer_index(context->arg_buffer, cmd->arg_offset + i);
    /* require variable */
    if (param->isrefvar)
    {
      src = buffer_index(context->data_buffer, arg->data_offset);
      if (!calc_isvar(src))
      {
        return RIL_COMPILE_ERROR(context, "Parse error: syntax error, require variable in '%s' argument of '%s' tag", param->name, context->tagname);
      }
      value = (calc_value_t*)((uint8_t*)src + sizeof(calc_opcode_t));
      value->type = VARIANT_REFVAR;
    }
  }

  return RIL_OK;
}

static __inline RILRESULT _compiletag(ril_compile_t *context)
{
  char word[128], args[512];
  const char *start;
  int argc = 0;
  bool hasnonamearg = false;
  ril_parameter_t *param;
  int i, j, argsize;
  ril_crc_t hash, arghashlist[RIL_ARGUMENT_SIZE] = {0};
  ril_arg_t *cmdarg;
  
  args[0] = '\0';
  
  // get name
  context->cur = ril_getword(context->tagname, context->cur, true);
  
  // add tag
  if (NULL == rilc_newcmd(context, 0)) return RIL_ERROR;
  
  /* get args */
  for (;; ++argc)
  {
    /* name */
    start = context->cur;
    context->cur = ril_getword(word, context->cur, true);
    
    /* param name */
    if ('\0' == *word)
    {
      if (ril_isrightdelimiter(context->vm, context->cur)) break;
      hasnonamearg = true;
    }
    else
    {
      context->cur = ril_trimspace(context->cur);
      if (':' != *context->cur)
      {
        context->cur = start;
        hasnonamearg = true;
      }
      else
      {
        hash = ril_makecrc(word);
        for (j = argc - 1; 0 <= j; --j)
        {
          if (arghashlist[j] == hash)
          {
            return RIL_COMPILE_ERROR(context, "Fatal error: '%s' is already specified arguments in '%s' tag", word, context->tagname);
          }
        }
        arghashlist[argc] = hash;
        if ('\0' != args[0]) strcat(args, ",");
        strcat(args, word);
        context->cur = ril_trimspace(context->cur + 1);
      }
    }
    
    /* param */
    rilc_addarg(context);
    
    if (RIL_FAILED(calc_compile(context, context->data_buffer, context->cur)))
    {
      return RIL_COMPILE_ERROR(context,
                               "Parse error: syntax error, %s in '%s' argument of '%s' tag",
                               context->calc_error, word, context->tagname);
    }
  }
  
  context->cur += context->vm->delimiter.right.length;
  
  if (RIL_FAILED(_findtag(context, context->cmd, context->tagname, argc, hasnonamearg, args)))
  {
    return RIL_COMPILE_ERROR(context, "Fatal error: Call to undefined tag '%s'", context->tagname);
  }
  
  argsize = buffer_size(context->tag->param_buffer);
  buffer_malloc(context->arg_buffer, argsize - argc);
  for (i = hasnonamearg; i < argsize; ++i)
  {
    param = (ril_parameter_t*)buffer_index(context->tag->param_buffer, i);
    cmdarg = rilc_getarg(context, context->cmd, i);
    if (arghashlist[i] == param->namehash) continue;
    for (j = argc - 1; 0 <= j; --j)
    {
      if (arghashlist[j] == param->namehash) break;
    }
    if (0 <= j)
    {
      // sort argument
      ril_arg_t temp;
      if (j == i) continue;
      if (i < argc) temp = *cmdarg;
      *cmdarg = *rilc_getarg(context, context->cmd, j);
      if (i < argc)
      {
        arghashlist[j] = arghashlist[i];
        *rilc_getarg(context, context->cmd, j) = temp;
      }
      else
      {
        arghashlist[j] = 0;
      }
      continue;
    }
    // set default value
    if (buffer_empty(param->valuebuffer)) return RIL_ERROR;
    
    if (i < argc)
    {
      *rilc_getarg(context, context->cmd, argc) = *cmdarg;
      arghashlist[argc] = arghashlist[i];
      arghashlist[i] = 0;
      ++argc;
    }
    cmdarg->data_offset = buffer_size(context->data_buffer);
    if (RIL_FAILED(calc_compile(context, context->data_buffer, (char*)buffer_front(param->valuebuffer))))
    {
      return ril_error(context->vm, "Parse error: syntax error, %s in '%s' argument of '%s' tag", context->calc_error, param->name, context->tagname);
    }
  }

  // check args
  if (RIL_FAILED(_checkargs(context, context->cmd))) return RIL_ERROR;
  
  // check pair stack
  if (RIL_FAILED(rilc_checkpair(context, context->cmd))) return RIL_ERROR;
  
  // check child stack
  if (RIL_FAILED(rilc_checkchild(context, context->cmd))) return RIL_ERROR;
  
  // call compile handler
  if (NULL != context->tag->compile_handler)
  {
    if (RIL_FAILED(context->tag->compile_handler(context)))
    {
      return RIL_ERROR;
    }
  }
  
  return RIL_OK;
}

RILRESULT ril_compilefile(RILVM vm, const char *file, buffer_t *dest)
{
  RILRESULT result;
  char *buf;
  const char *path = ril_getpath(vm, file);
  
  buf = ril_readfile(path);
  if (NULL == buf)
  {
    return ril_error(vm, "Fatal error: Cannot open %s", path);
  }
  
  result = ril_compile(vm, buf, dest);
  
  free(buf);
  
  return result;
}

static __inline ril_compile_t* _open(RILVM vm)
{
  ril_compile_t *context = malloc(sizeof(ril_compile_t));
  
  context->line = 1;
  context->cmd = NULL;
  context->tag = NULL;
  
  context->pair_stack = stack_open(sizeof(_stack_pair_t));
  stack_resize(context->pair_stack, 100);
  
  context->label_buffer = buffer_open(sizeof(ril_label_t), LABEL_BUFF_SIZE);
  context->cmd_buffer = buffer_open(sizeof(ril_cmd_t), TAG_BUFF_SIZE);
  context->arg_buffer = buffer_open(sizeof(ril_arg_t), ARG_BUFF_SIZE);
  context->data_buffer = buffer_open(1, DATA_BUFF_SIZE);
  
  context->vm = vm;
  
  return context;
}

static void _close(ril_compile_t *context)
{
  ril_deletemacros(context->vm);
  stack_close(context->pair_stack);
  buffer_close(context->label_buffer);
  buffer_close(context->cmd_buffer);
  buffer_close(context->arg_buffer);
  buffer_close(context->data_buffer);
  
  free(context);
}

static __inline void _output(buffer_t *buffer, ril_compile_t *context)
{
  ril_common_header_t common_header;
  uint32_t common_header_size = sizeof(common_header);
  uint32_t label_size = buffer_bytesize(context->label_buffer);
  uint32_t cmd_size = buffer_bytesize(context->cmd_buffer);
  uint32_t arg_size = buffer_bytesize(context->arg_buffer);
  uint32_t data_size = buffer_bytesize(context->data_buffer);
  void *dest, *dest_cur;
  int size;
  
  size = common_header_size + label_size + cmd_size + arg_size + data_size;
  
  common_header.endian = ril_endian();
  common_header.cmd_size   = buffer_size(context->cmd_buffer);
  common_header.arg_size   = buffer_size(context->arg_buffer);
  common_header.label_size = buffer_size(context->label_buffer);
  common_header.label_offset = common_header_size;
  common_header.cmd_offset   = common_header.label_offset + label_size;
  common_header.arg_offset   = common_header.cmd_offset + cmd_size;
  common_header.data_offset  = common_header.arg_offset + arg_size;
  
  dest = buffer_malloc(buffer, size);
  dest_cur = ril_write(dest, &common_header, common_header_size);
  dest_cur = ril_write(dest_cur, buffer_front(context->label_buffer), label_size);
  dest_cur = ril_write(dest_cur, buffer_front(context->cmd_buffer), cmd_size);
  dest_cur = ril_write(dest_cur, buffer_front(context->arg_buffer), arg_size);
  dest_cur = ril_write(dest_cur, buffer_front(context->data_buffer), data_size);
}

RILRESULT ril_compile(RILVM vm, const char *src, buffer_t *dest)
{
  ril_compile_t *context = _open(vm);
  
  if (RIL_FAILED(rilc_compile(src, context)))
  {
    _close(context);
    return RIL_ERROR;
  }

  // exit tag
  if (NULL == rilc_addcmd(context, RIL_TAG_RETURN)) return RIL_ERROR;
  rilc_addarg(context);
  calc_writevalue2buffer(context->data_buffer, VARIANT_NULL, NULL, 0);
  *(calc_opcode_t*)buffer_malloc(context->data_buffer, sizeof(calc_opcode_t)) = CALC_END;
  
  _output(dest, context);
  _close(context);
  
  return RIL_OK;
}

RILRESULT rilc_compile(const char *src, ril_compile_t *context)
{
  const char *beforecur = context->cur;
  char word[128];
  uint8_t isschar = false;
  uint32_t id;
  int valueindex, readbyte;
  calc_value_t *value;
  ril_cmd_t *textcmd = NULL;
  ril_label_t *label;
  
  context->cur = src;
  
  while ('\0' != *context->cur)
  {
    // left delimiter
    if (ril_isleftdelimiter(context->vm, context->cur) && !isschar)
    {
      context->cur += context->vm->delimiter.left.length;
      if (RIL_FAILED(_compiletag(context))) return RIL_ERROR;
      continue;
    }
    // right delimiter
    if (ril_isrightdelimiter(context->vm, context->cur) && !isschar)
    {
      return RIL_COMPILE_ERROR(context, "Parse error: syntax error, unexpected '%s'", context->vm->delimiter.right.string);
    }

    switch (*context->cur)
    {
    case '\\': // special char
      if (isschar) break;
      isschar = true;
      ++context->cur;
      continue;
    case ' ': // space
      if (NULL != context->cmd && RIL_TAG_CH == context->cmd->signature) break;
      ++context->cur;
      continue;
    case '\n': // line feed
      ++context->line;
      ++context->cur;
      continue;
    case '\r':
      if ('\n' != context->cur[1]) ++context->line;
      ++context->cur;
      continue;
    case '*': // label
      if (isschar) break;
      else
      {
        context->cur = ril_getword(word, context->cur + 1, true);
        if ('\0' == word[0])
        {
          return RIL_COMPILE_ERROR(context, "Parse error: syntax error, unexpected '*'");
        }
        id = _labelnametoid(context, word);
        label = buffer_index(context->label_buffer, id);
        if (LABEL_NULL != label->cmdid)
        {
          return RIL_COMPILE_ERROR(context, "Fatal error: label '%s' is overloaded", word);
        }
        
        label->cmdid = buffer_size(context->cmd_buffer);
        if (NULL == rilc_addcmd(context, RIL_TAG_LABEL)) return RIL_ERROR;
        rilc_addarg(context);
        rilc_addbytes(context, &label->namehash, sizeof(label->namehash));
      }
      continue;
    case '/': { // comment out
      bool warning = true;
      if ('*' != context->cur[1])
      {
        ++context->cur;
        continue;
      }
      context->cur += 2;
      while ('\0' != *context->cur)
      {
        if ('*' == *context->cur && '/' == context->cur[1])
        {
          context->cur += 2;
          warning = false;
          break;
        }
        ++context->cur;
      }
      if (warning)
      {
        RIL_COMPILE_ERROR(context, "Warning: Unterminated comment starting");
      }
      continue; }
    case ';': // comment out
      if (isschar) break;
      else context->cur = ril_nextline(context->cur);
      continue;
    default:
      break;
    }
    
    // text
    isschar = false;
    if (NULL == context->cmd || textcmd != context->cmd)
    {
      textcmd = rilc_addcmd(context, RIL_TAG_CH);
      if (NULL == textcmd) return RIL_ERROR;
      rilc_addarg(context);
      calc_writeoperator(context->data_buffer, CALC_PUSH);
      valueindex = buffer_size(context->data_buffer);
      value = buffer_malloc(context->data_buffer, sizeof(calc_value_t));
      value->type = VARIANT_LITERAL | VARIANT_STRING;
      value->size = 1;
    }
    else buffer_erase(context->data_buffer, 1 + sizeof(calc_opcode_t));

    readbyte = mblen(context->cur, MB_CUR_MAX);
    buffer_write(context->data_buffer, context->cur, readbyte);
    *(char*)buffer_malloc(context->data_buffer, 1) = '\0';
    calc_writeoperator(context->data_buffer, CALC_END);
    
    value = (calc_value_t*)buffer_index(context->data_buffer, valueindex);
    value->size += readbyte;
    context->cur += readbyte;
  }
  
  if (NULL != stack_back(context->pair_stack, NULL))
  {
    context->line = ((_stack_pair_t*)stack_back(context->pair_stack, NULL))->line;
    return RIL_COMPILE_ERROR(context, "Parse error: syntax error, tag pair does not exist");
  }
  
  context->cur = beforecur;
  
  return RIL_OK;
}
