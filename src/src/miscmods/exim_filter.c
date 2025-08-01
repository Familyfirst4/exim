/*************************************************
*     Exim - an Internet mail transport agent    *
*************************************************/

/* Copyright (c) The Exim Maintainers 2020 - 2024 */
/* Copyright (c) University of Cambridge 1995 - 2018 */
/* See the file NOTICE for conditions of use and distribution. */
/* SPDX-License-Identifier: GPL-2.0-or-later */


/* Code for mail filtering functions. */

#include "../exim.h"


/* Command arguments and left/right points in conditions can contain different
types of data, depending on the particular command or condition. Originally,
(void *) was used as "any old type", with casts, but this gives trouble and
warnings in some environments. So now it is done "properly", with a union. We
need to declare the structures first because some of them are recursive. */

struct filter_cmd;
struct condition_block;

union argtypes {
  struct string_item     *a;
  BOOL                    b;
  struct condition_block *c;
  struct filter_cmd      *f;
  int                     i;
  const uschar            *u;
};

/* Local structures used in this module */

typedef struct filter_cmd {
  struct filter_cmd *next;
  int command;
  BOOL seen;
  BOOL noerror;
  union argtypes args[1];
} filter_cmd;

typedef struct condition_block {
  struct condition_block *parent;
  int type;
  BOOL testfor;
  union argtypes left;
  union argtypes right;
} condition_block;

/* Miscellaneous other declarations */

static uschar **error_pointer;
static const uschar *log_filename;
static int  filter_options;
static int  line_number;
static int  expect_endif;
static int  had_else_endif;
static int  log_fd;
static int  log_mode;
static int  output_indent;
static BOOL filter_delivered;
static BOOL finish_obeyed;
static BOOL seen_force;
static BOOL seen_value;
static BOOL noerror_force;

enum { had_neither, had_else, had_elif, had_endif };

static BOOL read_command_list(const uschar **, filter_cmd ***, BOOL);


/* This defines the offsets for the arguments; first the string ones, and
then the non-string ones. The order must be as above. */

enum { mailarg_index_to,
       mailarg_index_cc,
       mailarg_index_bcc,
       mailarg_index_from,
       mailarg_index_reply_to,
       mailarg_index_subject,
       mailarg_index_headers,      /* misc headers must be last */
       mailarg_index_text,         /* text is first after headers */
       mailarg_index_file,         /* between text and expand are filenames */
       mailarg_index_log,
       mailarg_index_once,
       mailarg_index_once_repeat,  /* a time string */
       mailarg_index_expand,       /* first non-string argument */
       mailarg_index_return,
       mailargs_total              /* total number of arguments */
       };

/* The string arguments for the mail command. The header line ones (that are
permitted to include \n followed by white space) first, and then the body text
one (it can have \n anywhere). Then the file names and once_repeat, which may
not contain \n. */

static const char *mailargs[] = {  /* "to" must be first, and */
  [mailarg_index_to] = "to",       /* "cc" and "bcc" must follow */
  [mailarg_index_cc] = "cc",
  [mailarg_index_bcc] = "bcc",
  [mailarg_index_from] = "from",
  [mailarg_index_reply_to] = "reply_to",
  [mailarg_index_subject] = "subject",
  [mailarg_index_headers] = "extra_headers", /* misc added header lines */
  [mailarg_index_text] = "text",
  [mailarg_index_file] = "file",
  [mailarg_index_log] = "log",
  [mailarg_index_once] = "once",
  [mailarg_index_once_repeat] = "once_repeat"
};

/* The count of string arguments */

#define MAILARGS_STRING_COUNT (nelem(mailargs))

/* The count of string arguments that are actually passed over as strings
(once_repeat is converted to an int). */

#define mailargs_string_passed (MAILARGS_STRING_COUNT - 1)

/* Offsets in the data structure for the string arguments (note that
once_repeat isn't a string argument at this point.) */

static int reply_offsets[] = {
  [mailarg_index_to] = offsetof(reply_item, to),
  [mailarg_index_cc] = offsetof(reply_item, cc),
  [mailarg_index_bcc] = offsetof(reply_item, bcc),
  [mailarg_index_from] = offsetof(reply_item, from),
  [mailarg_index_reply_to] = offsetof(reply_item, reply_to),
  [mailarg_index_subject] = offsetof(reply_item, subject),
  [mailarg_index_headers] = offsetof(reply_item, headers),
  [mailarg_index_text] = offsetof(reply_item, text),
  [mailarg_index_file] = offsetof(reply_item, file),
  [mailarg_index_log] = offsetof(reply_item, logfile),
  [mailarg_index_once] = offsetof(reply_item, oncelog),
};

/* Condition identities and names, with negated versions for some
of them. */

enum { cond_and, cond_or, cond_personal, cond_begins, cond_BEGINS,
       cond_ends, cond_ENDS, cond_is, cond_IS, cond_matches,
       cond_MATCHES, cond_contains, cond_CONTAINS, cond_delivered,
       cond_above, cond_below, cond_errormsg, cond_firsttime,
       cond_manualthaw, cond_foranyaddress };

static const char *cond_names[] = {
  [cond_and] = "and",
  [cond_or] = "or",
  [cond_personal] = "personal",
  [cond_begins] = "begins",
  [cond_BEGINS] = "BEGINS",
  [cond_ends] = "ends",
  [cond_ENDS] = "ENDS",
  [cond_is] = "is",
  [cond_IS] = "IS",
  [cond_matches] = "matches",
  [cond_MATCHES] = "MATCHES",
  [cond_contains] = "contains",
  [cond_CONTAINS] = "CONTAINS",
  [cond_delivered] = "delivered",
  [cond_above] = "above",
  [cond_below] = "below",
  [cond_errormsg] = "error_message",
  [cond_firsttime] = "first_delivery",
  [cond_manualthaw] = "manually_thawed",
  [cond_foranyaddress] = "foranyaddress" };

static const char *cond_not_names[] = {
  [cond_and] = "",
  [cond_or] = "",
  [cond_personal] = "not personal",
  [cond_begins] = "does not begin",
  [cond_BEGINS] = "does not BEGIN",
  [cond_ends] = "does not end",
  [cond_ENDS] = "does not END",
  [cond_is] = "is not",
  [cond_IS] = "IS not",
  [cond_matches] = "does not match",
  [cond_MATCHES] = "does not MATCH",
  [cond_contains] = "does not contain",
  [cond_CONTAINS] = "does not CONTAIN",
  [cond_delivered] = "not delivered",
  [cond_above] = "not above",
  [cond_below] = "not below",
  [cond_errormsg] = "not error_message",
  [cond_firsttime] = "not first_delivery",
  [cond_manualthaw] = "not manually_thawed",
  [cond_foranyaddress] = "not foranyaddress" };

/* Tables of binary condition words and their corresponding types. Not easy
to amalgamate with the above because of the different variants. */

static const char *cond_words[] = {
   "BEGIN",
   "BEGINS",
   "CONTAIN",
   "CONTAINS",
   "END",
   "ENDS",
   "IS",
   "MATCH",
   "MATCHES",
   "above",
   "begin",
   "begins",
   "below",
   "contain",
   "contains",
   "end",
   "ends",
   "is",
   "match",
   "matches"};

static int cond_word_count = nelem(cond_words);

static int cond_types[] = { cond_BEGINS, cond_BEGINS, cond_CONTAINS,
  cond_CONTAINS, cond_ENDS, cond_ENDS, cond_IS, cond_MATCHES, cond_MATCHES,
  cond_above, cond_begins, cond_begins, cond_below, cond_contains,
  cond_contains, cond_ends, cond_ends, cond_is, cond_matches, cond_matches };

/* Command identities */

enum { ADD_COMMAND, DEFER_COMMAND, DELIVER_COMMAND, ELIF_COMMAND, ELSE_COMMAND,
       ENDIF_COMMAND, FINISH_COMMAND, FAIL_COMMAND, FREEZE_COMMAND,
       HEADERS_COMMAND, IF_COMMAND, LOGFILE_COMMAND, LOGWRITE_COMMAND,
       MAIL_COMMAND, NOERROR_COMMAND, PIPE_COMMAND, SAVE_COMMAND, SEEN_COMMAND,
       TESTPRINT_COMMAND, UNSEEN_COMMAND, VACATION_COMMAND };

static const char * command_list[] = {
  [ADD_COMMAND] =	"add",
  [DEFER_COMMAND] =	"defer",
  [DELIVER_COMMAND] =	"deliver",
  [ELIF_COMMAND] =	"elif",
  [ELSE_COMMAND] =	"else",
  [ENDIF_COMMAND] =	"endif",
  [FINISH_COMMAND] =	"finish",
  [FAIL_COMMAND] =	"fail",
  [FREEZE_COMMAND] =	"freeze",
  [HEADERS_COMMAND] =	"headers",
  [IF_COMMAND] =	"if",
  [LOGFILE_COMMAND] =	"logfile",
  [LOGWRITE_COMMAND] =	"logwrite",
  [MAIL_COMMAND] =	"mail",
  [NOERROR_COMMAND] =	"noerror",
  [PIPE_COMMAND] =	"pipe",
  [SAVE_COMMAND] =	"save",
  [SEEN_COMMAND] =	"seen",
  [TESTPRINT_COMMAND] =	"testprint",
  [UNSEEN_COMMAND] =	"unseen",
  [VACATION_COMMAND] =	"vacation"
};

static int command_list_count = nelem(command_list);

/* This table contains the number of expanded arguments in the bottom 4 bits.
If the top bit is set, it means that the default for the command is "seen". */

static uschar command_exparg_count[] = {
  [ADD_COMMAND] =	2,
  [DEFER_COMMAND] =	1,
  [DELIVER_COMMAND] =	128+2,
  [ELIF_COMMAND] =	0,
  [ELSE_COMMAND] =	0,
  [ENDIF_COMMAND] =	0,
  [FINISH_COMMAND] =	0,
  [FAIL_COMMAND] =	1,
  [FREEZE_COMMAND] =	1,
  [HEADERS_COMMAND] =	1,
  [IF_COMMAND] =	0,
  [LOGFILE_COMMAND] =	1,
  [LOGWRITE_COMMAND] =	1,
  [MAIL_COMMAND] =	MAILARGS_STRING_COUNT,
  [NOERROR_COMMAND] =	0,
  [PIPE_COMMAND] =	128+0,
  [SAVE_COMMAND] =	128+1,
  [SEEN_COMMAND] =	0,
  [TESTPRINT_COMMAND] =	1,
  [UNSEEN_COMMAND] =	0,
  [VACATION_COMMAND] =	MAILARGS_STRING_COUNT
};



/*************************************************
*          Find next significant uschar            *
*************************************************/

/* Function to skip over white space and, optionally, comments.

Arguments:
  ptr              pointer to next character
  comment_allowed  if TRUE, comments (# to \n) are skipped

Returns:           pointer to next non-whitespace character
*/

static const uschar *
nextsigchar(const uschar *ptr, BOOL comment_allowed)
{
for (;;)
  {
  while (isspace(*ptr))
    if (*ptr++ == '\n') line_number++;
  if (comment_allowed && *ptr == '#')
    while (*++ptr != '\n' && *ptr) ;
  else
    break;
  }
return ptr;
}



/*************************************************
*                Read one word                   *
*************************************************/

/* The terminator is white space unless bracket is TRUE, in which
case ( and ) terminate.

Arguments
  ptr       pointer to next character
  buffer    where to put the word
  size      size of buffer
  bracket   if TRUE, terminate on ( and ) as well as space

Returns:    pointer to the next significant character after the word
*/

static const uschar *
nextword(const uschar *ptr, uschar *buffer, int size, BOOL bracket)
{
uschar * bp = buffer;
while (*ptr && !isspace(*ptr) &&
       (!bracket || (*ptr != '(' && *ptr != ')')))
  if (bp - buffer < size - 1)
    *bp++ = *ptr++;
  else
    {
    *error_pointer = string_sprintf("word is too long in line %d of "
      "filter file (max = %d chars)", line_number, size);
    break;
    }

*bp = '\0';
return nextsigchar(ptr, TRUE);
}



/*************************************************
*                Read one item                   *
*************************************************/

/* Might be a word, or might be a quoted string; in the latter case
do the escape stuff.

Arguments:
  ptr        pointer to next character
  buffer     where to put the item
  size       size of buffer
  bracket    if TRUE, terminate non-quoted on ( and ) as well as space

Returns:     the next significant character after the item
*/

static const uschar *
nextitem(const uschar *ptr, uschar *buffer, int size, BOOL bracket)
{
uschar *bp = buffer;
if (*ptr != '\"') return nextword(ptr, buffer, size, bracket);

while (*++ptr && *ptr != '\"' && *ptr != '\n')
  {
  if (bp - buffer >= size - 1)
    {
    *error_pointer = string_sprintf("string is too long in line %d of "
      "filter file (max = %d chars)", line_number, size);
    break;
    }

  if (*ptr != '\\') *bp++ = *ptr; else
    {
    if (isspace(ptr[1]))    /* \<whitespace>NL<whitespace> ignored */
      {
      const uschar *p = ptr + 1;
      while (*p != '\n' && isspace(*p)) p++;
      if (*p == '\n')
        {
        line_number++;
        ptr = p;
        while (ptr[1] != '\n' && isspace(ptr[1])) ptr++;
        continue;
        }
      }

    *bp++ = string_interpret_escape(CUSS &ptr);
    }
  }

if (*ptr == '\"') ptr++;
  else if (!*error_pointer)
    *error_pointer = string_sprintf("quote missing at end of string "
      "in line %d", line_number);

*bp = 0;
return nextsigchar(ptr, TRUE);
}




/*************************************************
*          Convert a string + K|M to a number    *
*************************************************/

/*
Arguments:
  s        points to text string
  OK       set TRUE if a valid number was read

Returns:   the number, or 0 on error (with *OK FALSE)
*/

static int
get_number(const uschar *s, BOOL *ok)
{
int value, count;
*ok = FALSE;
if (sscanf(CS s, "%i%n", &value, &count) != 1) return 0;
if (tolower(s[count]) == 'k') { value *= 1024; count++; }
if (tolower(s[count]) == 'm') { value *= 1024*1024; count++; }
while (isspace(s[count])) count++;
if (s[count]) return 0;
*ok = TRUE;
return value;
}



/*************************************************
*            Read one condition                  *
*************************************************/

/* A complete condition must be terminated by "then"; bracketed internal
conditions must be terminated by a closing bracket. They are read by calling
this function recursively.

Arguments:
  ptr             points to start of condition
  condition_block where to hang the created condition block
  toplevel        TRUE when called at the top level

Returns:          points to next character after "then"
*/

static const uschar *
read_condition(const uschar *ptr, condition_block **cond, BOOL toplevel)
{
uschar buffer[1024];
BOOL testfor = TRUE;
condition_block *current_parent = NULL;
condition_block **current = cond;

*current = NULL;

/* Loop to read next condition */

for (;;)
  {
  condition_block *c;

  /* reaching the end of the input is an error. */

  if (!*ptr)
    {
    *error_pointer = US"\"then\" missing at end of filter file";
    break;
    }

  /* Opening bracket at the start of a condition introduces a nested
  condition, which must be terminated by a closing bracket. */

  if (*ptr == '(')
    {
    ptr = read_condition(nextsigchar(ptr+1, TRUE), &c, FALSE);
    if (*error_pointer != NULL) break;
    if (*ptr != ')')
      {
      *error_pointer = string_sprintf("expected \")\" in line %d of "
        "filter file", line_number);
      break;
      }
    if (!testfor)
      {
      c->testfor = !c->testfor;
      testfor = TRUE;
      }
    ptr = nextsigchar(ptr+1, TRUE);
    }


  /* Closing bracket at the start of a condition is an error. Give an
  explicit message, as otherwise "unknown condition" would be confusing. */

  else if (*ptr == ')')
    {
    *error_pointer = string_sprintf("unexpected \")\" in line %d of "
      "filter file", line_number);
    break;
    }

  /* Otherwise we expect a word or a string. */

  else
    {
    ptr = nextitem(ptr, buffer, sizeof(buffer), TRUE);
    if (*error_pointer) break;

    /* "Then" at the start of a condition is an error */

    if (Ustrcmp(buffer, "then") == 0)
      {
      *error_pointer = string_sprintf("unexpected \"then\" near line %d of "
        "filter file", line_number);
      break;
      }

    /* "Not" at the start of a condition negates the testing condition. */

    if (Ustrcmp(buffer, "not") == 0)
      {
      testfor = !testfor;
      continue;
      }

    /* Build a condition block from the specific word. */

    c = store_get(sizeof(condition_block), GET_UNTAINTED);
    c->left.u = c->right.u = NULL;
    c->testfor = testfor;
    testfor = TRUE;

    /* Check for conditions that start with a keyword */

    if (Ustrcmp(buffer, "delivered") == 0) c->type = cond_delivered;
    else if (Ustrcmp(buffer, "error_message") == 0) c->type = cond_errormsg;
    else if (Ustrcmp(buffer, "first_delivery") == 0) c->type = cond_firsttime;
    else if (Ustrcmp(buffer, "manually_thawed") == 0) c->type = cond_manualthaw;

    /* Personal can be followed by any number of aliases */

    else if (Ustrcmp(buffer, "personal") == 0)
      {
      c->type = cond_personal;
      for (;;)
        {
        string_item *aa;
        const uschar * saveptr = ptr;
        ptr = nextword(ptr, buffer, sizeof(buffer), TRUE);
        if (*error_pointer) break;
        if (Ustrcmp(buffer, "alias") != 0)
          {
          ptr = saveptr;
          break;
          }
        ptr = nextitem(ptr, buffer, sizeof(buffer), TRUE);
        if (*error_pointer) break;
        aa = store_get(sizeof(string_item), GET_UNTAINTED);
        aa->text = string_copy(buffer);
        aa->next = c->left.a;
        c->left.a = aa;
        }
      }

    /* Foranyaddress must be followed by a string and a condition enclosed
    in parentheses, which is handled as a subcondition. */

    else if (Ustrcmp(buffer, "foranyaddress") == 0)
      {
      ptr = nextitem(ptr, buffer, sizeof(buffer), TRUE);
      if (*error_pointer) break;
      if (*ptr != '(')
        {
        *error_pointer = string_sprintf("\"(\" expected after \"foranyaddress\" "
          "near line %d of filter file", line_number);
        break;
        }

      c->type = cond_foranyaddress;
      c->left.u = string_copy(buffer);

      ptr = read_condition(nextsigchar(ptr+1, TRUE), &(c->right.c), FALSE);
      if (*error_pointer) break;
      if (*ptr != ')')
        {
        *error_pointer = string_sprintf("expected \")\" in line %d of "
          "filter file", line_number);
        break;
        }
      ptr = nextsigchar(ptr+1, TRUE);
      }

    /* If it's not a word we recognize, then it must be the lefthand
    operand of one of the comparison words. */

    else
      {
      int i;
      const uschar *isptr = NULL;

      c->left.u = string_copy(buffer);
      ptr = nextword(ptr, buffer, sizeof(buffer), TRUE);
      if (*error_pointer) break;

      /* Handle "does|is [not]", preserving the pointer after "is" in
      case it isn't that, but the form "is <string>". */

      if (strcmpic(buffer, US"does") == 0 || strcmpic(buffer, US"is") == 0)
        {
        if (buffer[0] == 'i') { c->type = cond_is; isptr = ptr; }
        if (buffer[0] == 'I') { c->type = cond_IS; isptr = ptr; }

        ptr = nextword(ptr, buffer, sizeof(buffer), TRUE);
        if (*error_pointer) break;
        if (strcmpic(buffer, US"not") == 0)
          {
          c->testfor = !c->testfor;
          if (isptr) isptr = ptr;
          ptr = nextword(ptr, buffer, sizeof(buffer), TRUE);
          if (*error_pointer) break;
          }
        }

      for (i = 0; i < cond_word_count; i++)
        {
        if (Ustrcmp(buffer, cond_words[i]) == 0)
          {
          c->type = cond_types[i];
          break;
          }
        }

      /* If an unknown word follows "is" or "is not"
      it's actually the argument. Reset to read it. */

      if (i >= cond_word_count)
        {
        if (!isptr)
          {
          *error_pointer = string_sprintf("unrecognized condition word %q "
            "near line %d of filter file", buffer, line_number);
          break;
          }
        ptr = isptr;
        }

      /* Get the RH argument. */

      ptr = nextitem(ptr, buffer, sizeof(buffer), TRUE);
      if (*error_pointer) break;
      c->right.u = string_copy(buffer);
      }
    }

  /* We have read some new condition and set it up in the condition block
  c; point the current pointer at it, and then deal with what follows. */

  *current = c;

  /* Closing bracket terminates if this is a lower-level condition. Otherwise
  it is unexpected. */

  if (*ptr == ')')
    {
    if (toplevel)
      *error_pointer = string_sprintf("unexpected \")\" in line %d of "
        "filter file", line_number);
    break;
    }

  /* Opening bracket following a condition is an error; give an explicit
  message to make it clearer what is wrong. */

  else if (*ptr == '(')
    {
    *error_pointer = string_sprintf("unexpected \"(\" in line %d of "
      "filter file", line_number);
    break;
    }

  /* Otherwise the next thing must be one of the words "and", "or" or "then" */

  else
    {
//    const uschar *saveptr = ptr;
    ptr = nextword(ptr, buffer, sizeof(buffer), FALSE);
    if (*error_pointer) break;

    /* "Then" terminates a toplevel condition; otherwise a closing bracket
    has been omitted. Put a string terminator at the start of "then" so
    that reflecting the condition can be done when testing. */
    /*XXX This stops us doing a constification job in this file, unfortunately.
    Comment it out and see if anything breaks.
    With one addition down at DEFERFREEZEFAIL it passes the testsuite. */

    if (Ustrcmp(buffer, "then") == 0)
      {
//      if (toplevel) *saveptr = 0;
//      else
      if (!toplevel)
        *error_pointer = string_sprintf("missing \")\" at end of "
          "condition near line %d of filter file", line_number);
      break;
      }

    /* "And" causes a new condition block to replace the one we have
    just read, which becomes the left sub-condition. The current pointer
    is reset to the pointer for the right sub-condition. We have to keep
    track of the tree of sequential "ands", so as to traverse back up it
    if an "or" is met. */

    else if (Ustrcmp(buffer, "and") == 0)
      {
      condition_block * andc = store_get(sizeof(condition_block), GET_UNTAINTED);
      andc->parent = current_parent;
      andc->type = cond_and;
      andc->testfor = TRUE;
      andc->left.c = c;
      andc->right.u = NULL;    /* insurance */
      *current = andc;
      current = &(andc->right.c);
      current_parent = andc;
      }

    /* "Or" is similar, but has to be done a bit more carefully to
    ensure that "and" is more binding. If there's a parent set, we
    are following a sequence of "and"s and must track back to their
    start. */

    else if (Ustrcmp(buffer, "or") == 0)
      {
      condition_block * orc = store_get(sizeof(condition_block), GET_UNTAINTED);
      condition_block * or_parent = NULL;

      if (current_parent)
        {
        while (current_parent->parent &&
               current_parent->parent->type == cond_and)
          current_parent = current_parent->parent;

        /* If the parent has a parent, it must be an "or" parent. */

        if (current_parent->parent)
          or_parent = current_parent->parent;
        }

      orc->parent = or_parent;
      if (!or_parent) *cond = orc;
      else or_parent->right.c = orc;
      orc->type = cond_or;
      orc->testfor = TRUE;
      orc->left.c = (current_parent == NULL)? c : current_parent;
      orc->right.c = NULL;   /* insurance */
      current = &(orc->right.c);
      current_parent = orc;
      }

    /* Otherwise there is a disaster */

    else
      {
      *error_pointer = string_sprintf("\"and\" or \"or\" or %q "
        "expected near line %d of filter file, but found %q",
          toplevel? "then" : ")", line_number, buffer);
      break;
      }
    }
  }

return nextsigchar(ptr, TRUE);
}



/*************************************************
*             Output the current indent          *
*************************************************/

static void
indent(void)
{
DEBUG(D_filter) for (int i = 0; i < output_indent; i++) debug_printf(" ");
}



/*************************************************
*          Condition printer: for debugging      *
*************************************************/

/*
Arguments:
  c           the block at the top of the tree
  toplevel    TRUE at toplevel - stops overall brackets

Returns:      nothing
*/

static void
print_condition(condition_block *c, BOOL toplevel)
{
const char *name = (c->testfor)? cond_names[c->type] : cond_not_names[c->type];
switch(c->type)
  {
  case cond_personal:
  case cond_delivered:
  case cond_errormsg:
  case cond_firsttime:
  case cond_manualthaw:
    debug_printf("%s", name);
    break;

  case cond_is:
  case cond_IS:
  case cond_matches:
  case cond_MATCHES:
  case cond_contains:
  case cond_CONTAINS:
  case cond_begins:
  case cond_BEGINS:
  case cond_ends:
  case cond_ENDS:
  case cond_above:
  case cond_below:
    debug_printf("%s %s %s", c->left.u, name, c->right.u);
    break;

  case cond_and:
    if (!c->testfor) debug_printf("not (");
    print_condition(c->left.c, FALSE);
    debug_printf(" %s ", cond_names[c->type]);
    print_condition(c->right.c, FALSE);
    if (!c->testfor) debug_printf(")");
    break;

  case cond_or:
    if (!c->testfor) debug_printf("not (");
    else if (!toplevel) debug_printf("(");
    print_condition(c->left.c, FALSE);
    debug_printf(" %s ", cond_names[c->type]);
    print_condition(c->right.c, FALSE);
    if (!toplevel || !c->testfor) debug_printf(")");
    break;

  case cond_foranyaddress:
    debug_printf("%s %s (", name, c->left.u);
    print_condition(c->right.c, FALSE);
    debug_printf(")");
    break;
  }
}




/*************************************************
*            Read one filtering command          *
*************************************************/

/*
Arguments:
   pptr        points to pointer to first character of command; the pointer
                 is updated to point after the last character read
   lastcmdptr  points to pointer to pointer to last command; used for hanging
                 on the newly read command

Returns:       TRUE if command successfully read, else FALSE
*/

static BOOL
read_command(const uschar **pptr, filter_cmd ***lastcmdptr)
{
int command, i, cmd_bit;
filter_cmd *new, **newlastcmdptr;
BOOL yield = TRUE;
BOOL was_seen_or_unseen = FALSE;
BOOL was_noerror = FALSE;
uschar buffer[1024];
const uschar *ptr = *pptr;
const uschar *saveptr;
uschar *fmsg = NULL;

/* Read the next word and find which command it is. Command words are normally
terminated by white space, but there are two exceptions, which are the "if" and
"elif" commands. We must allow for them to be terminated by an opening bracket,
as brackets are allowed in conditions and users will expect not to require
white space here. */

*buffer = '\0';	/* compiler quietening */

if (Ustrncmp(ptr, "if(", 3) == 0)
  {
  Ustrcpy(buffer, US"if");
  ptr += 2;
  }
else if (Ustrncmp(ptr, "elif(", 5) == 0)
  {
  Ustrcpy(buffer, US"elif");
  ptr += 4;
  }
else
  {
  ptr = nextword(ptr, buffer, sizeof(buffer), FALSE);
  if (*error_pointer) return FALSE;
  }

for (command = 0; command < command_list_count; command++)
  if (Ustrcmp(buffer, command_list[command]) == 0) break;

/* Handle the individual commands */

switch (command)
  {
  /* Add takes two arguments, separated by the word "to". Headers has two
  arguments, but the first must be "add", "remove", or "charset", and it gets
  stored in the second argument slot. Neither may be preceded by seen, unseen
  or noerror. */

  case ADD_COMMAND:
  case HEADERS_COMMAND:
    if (seen_force || noerror_force)
      {
      *error_pointer = string_sprintf("\"seen\", \"unseen\", or \"noerror\" "
	"found before an %q command near line %d",
	  command_list[command], line_number);
      yield = FALSE;
      }
  /* Fall through */

  /* Logwrite, logfile, pipe, and testprint all take a single argument, save
  and logfile can have an option second argument for the mode, and deliver can
  have "errors_to <address>" in a system filter, or in a user filter if the
  address is the current one. */

  case DELIVER_COMMAND:
  case LOGFILE_COMMAND:
  case LOGWRITE_COMMAND:
  case PIPE_COMMAND:
  case SAVE_COMMAND:
  case TESTPRINT_COMMAND:

    ptr = nextitem(ptr, buffer, sizeof(buffer), FALSE);
    if (!*buffer)
      *error_pointer = string_sprintf("%q requires an argument "
	"near line %d of filter file", command_list[command], line_number);

    if (*error_pointer) yield = FALSE; else
      {
      union argtypes argument, second_argument;

      argument.u = second_argument.u = NULL;

      if (command == ADD_COMMAND)
	{
	argument.u = string_copy(buffer);
	ptr = nextitem(ptr, buffer, sizeof(buffer), FALSE);
	if (!*buffer || Ustrcmp(buffer, "to") != 0)
	  *error_pointer = string_sprintf("\"to\" expected in \"add\" command "
	    "near line %d of filter file", line_number);
	else
	  {
	  ptr = nextitem(ptr, buffer, sizeof(buffer), FALSE);
	  if (!*buffer)
	    *error_pointer = string_sprintf("value missing after \"to\" "
	      "near line %d of filter file", line_number);
	  else second_argument.u = string_copy(buffer);
	  }
	}

      else if (command == HEADERS_COMMAND)
	{
	if (Ustrcmp(buffer, "add") == 0)
	  second_argument.b = TRUE;
	else
	  if (Ustrcmp(buffer, "remove") == 0) second_argument.b = FALSE;
	else
	  if (Ustrcmp(buffer, "charset") == 0)
	    second_argument.b = TRUE_UNSET;
	else
	  {
	  *error_pointer = string_sprintf("\"add\", \"remove\", or \"charset\" "
	    "expected after \"headers\" near line %d of filter file",
	      line_number);
	  yield = FALSE;
	  }

	if (!f.system_filtering && second_argument.b != TRUE_UNSET)
	  {
	  *error_pointer = string_sprintf("header addition and removal is "
	    "available only in system filters: near line %d of filter file",
	    line_number);
	  yield = FALSE;
	  break;
	  }

	if (yield)
	  {
	  ptr = nextitem(ptr, buffer, sizeof(buffer), FALSE);
	  if (!*buffer)
	    *error_pointer = string_sprintf("value missing after \"add\", "
	      "\"remove\", or \"charset\" near line %d of filter file",
		line_number);
	  else argument.u = string_copy(buffer);
	  }
	}

      /* The argument for the logwrite command must end in a newline, and the save
      and logfile commands can have an optional mode argument. The deliver
      command can have an optional "errors_to <address>" for a system filter,
      or for a user filter if the address is the user's address. Accept the
      syntax here - the check is later. */

      else
	{
	if (command == LOGWRITE_COMMAND)
	  {
	  int len = Ustrlen(buffer);
	  if (len == 0 || buffer[len-1] != '\n') Ustrcat(buffer, US"\n");
	  }

	argument.u = string_copy(buffer);

	if (command == SAVE_COMMAND || command == LOGFILE_COMMAND)
	  {
	  if (isdigit(*ptr))
	    {
	    ptr = nextword(ptr, buffer, sizeof(buffer), FALSE);
	    second_argument.i = (int)Ustrtol(buffer, NULL, 8);
	    }
	  else second_argument.i = -1;
	  }

	else if (command == DELIVER_COMMAND)
	  {
	  const uschar *save_ptr = ptr;
	  ptr = nextword(ptr, buffer, sizeof(buffer), FALSE);
	  if (Ustrcmp(buffer, "errors_to") == 0)
	    {
	    ptr = nextword(ptr, buffer, sizeof(buffer), FALSE);
	    second_argument.u = string_copy(buffer);
	    }
	  else ptr = save_ptr;
	  }
	}

      /* Set up the command block. Seen defaults TRUE for delivery commands,
      FALSE for logging commands, and it doesn't matter for testprint, as
      that doesn't change the "delivered" status. */

      if (*error_pointer) yield = FALSE;
      else
	{
	new = store_get(sizeof(filter_cmd) + sizeof(union argtypes), GET_UNTAINTED);
	new->next = NULL;
	**lastcmdptr = new;
	*lastcmdptr = &(new->next);
	new->command = command;
	new->seen = seen_force? seen_value : command_exparg_count[command] >= 128;
	new->noerror = noerror_force;
	new->args[0] = argument;
	new->args[1] = second_argument;
	}
      }
    break;


  /* Elif, else and endif just set a flag if expected. */

  case ELIF_COMMAND:
  case ELSE_COMMAND:
  case ENDIF_COMMAND:
    if (seen_force || noerror_force)
      {
      *error_pointer = string_sprintf("\"seen\", \"unseen\", or \"noerror\" "
	"near line %d is not followed by a command", line_number);
      yield = FALSE;
      }

    if (expect_endif > 0)
      had_else_endif = (command == ELIF_COMMAND)? had_elif :
		       (command == ELSE_COMMAND)? had_else : had_endif;
    else
      {
      *error_pointer = string_sprintf("unexpected %q command near "
	"line %d of filter file", buffer, line_number);
      yield = FALSE;
      }
    break;


  /* Defer, freeze, and fail are available only if permitted. */

  case DEFER_COMMAND:
    cmd_bit = RDO_DEFER;
    goto DEFER_FREEZE_FAIL;

  case FAIL_COMMAND:
    cmd_bit = RDO_FAIL;
    goto DEFER_FREEZE_FAIL;

  case FREEZE_COMMAND:
    cmd_bit = RDO_FREEZE;

  DEFER_FREEZE_FAIL:
    if (!(filter_options & cmd_bit))
      {
      *error_pointer = string_sprintf("filtering command %q is disabled: "
	"near line %d of filter file", buffer, line_number);
      yield = FALSE;
      break;
      }

    /* A text message can be provided after the "text" keyword, or
    as a string in quotes. */

    saveptr = ptr;
    ptr = nextitem(ptr, buffer, sizeof(buffer), FALSE);
    if (*saveptr != '\"' && (!*buffer || Ustrcmp(buffer, "text") != 0))
      {
      ptr = saveptr;
      fmsg = US"";
      }
    else
      {
      if (*saveptr != '\"')
	ptr = nextitem(ptr, buffer, sizeof(buffer), FALSE);
      fmsg = string_copy(buffer);
      }

    /* Drop through and treat as "finish", but never set "seen". */

    seen_value = FALSE;

    /* Finish has no arguments; fmsg defaults to NULL */

    case FINISH_COMMAND:
    new = store_get(sizeof(filter_cmd), GET_UNTAINTED);
    new->next = NULL;
    **lastcmdptr = new;
    *lastcmdptr = &(new->next);
    new->command = command;
    new->seen = seen_force ? seen_value : FALSE;
    new->args[0].u = fmsg;
    break;


  /* Seen, unseen, and noerror are not allowed before if, which takes a
  condition argument and then and else sub-commands. */

  case IF_COMMAND:
    if (seen_force || noerror_force)
      {
      *error_pointer = string_sprintf("\"seen\", \"unseen\", or \"noerror\" "
	"found before an \"if\" command near line %d",
	  line_number);
      yield = FALSE;
      }

    /* Set up the command block for if */

    new = store_get(sizeof(filter_cmd) + 4 * sizeof(union argtypes), GET_UNTAINTED);
    new->next = NULL;
    **lastcmdptr = new;
    *lastcmdptr = &new->next;
    new->command = command;
    new->seen = FALSE;
    new->args[0].u = NULL;
    new->args[1].u = new->args[2].u = NULL;
    new->args[3].u = ptr;

    /* Read the condition */

    ptr = read_condition(ptr, &new->args[0].c, TRUE);
    if (*error_pointer) { yield = FALSE; break; }

    /* Read the commands to be obeyed if the condition is true */

    newlastcmdptr = &(new->args[1].f);
    if (!read_command_list(&ptr, &newlastcmdptr, TRUE)) yield = FALSE;

    /* If commands were successfully read, handle the various possible
    terminators. There may be a number of successive "elif" sections. */

    else
      {
      while (had_else_endif == had_elif)
	{
	filter_cmd *newnew =
	  store_get(sizeof(filter_cmd) + 4 * sizeof(union argtypes), GET_UNTAINTED);
	new->args[2].f = newnew;
	new = newnew;
	new->next = NULL;
	new->command = command;
	new->seen = FALSE;
	new->args[0].u = NULL;
	new->args[1].u = new->args[2].u = NULL;
	new->args[3].u = ptr;

	ptr = read_condition(ptr, &new->args[0].c, TRUE);
	if (*error_pointer) { yield = FALSE; break; }
	newlastcmdptr = &(new->args[1].f);
	if (!read_command_list(&ptr, &newlastcmdptr, TRUE))
	  yield = FALSE;
	}

      if (yield == FALSE) break;

      /* Handle termination by "else", possibly following one or more
      "elsif" sections. */

      if (had_else_endif == had_else)
	{
	newlastcmdptr = &(new->args[2].f);
	if (!read_command_list(&ptr, &newlastcmdptr, TRUE))
	  yield = FALSE;
	else if (had_else_endif != had_endif)
	  {
	  *error_pointer = string_sprintf("\"endif\" missing near line %d of "
	    "filter file", line_number);
	  yield = FALSE;
	  }
	}

      /* Otherwise the terminator was "endif" - this is checked by
      read_command_list(). The pointer is already set to NULL. */
      }

    /* Reset the terminator flag. */

    had_else_endif = had_neither;
    break;


  /* The mail & vacation commands have a whole slew of keyworded arguments.
  The final argument values are the file expand and return message booleans,
  whose offsets are defined in mailarg_index_{expand,return}. Although they
  are logically booleans, because they are stored in a uschar * value, we use
  NULL and not FALSE, to keep 64-bit compilers happy. */

  case MAIL_COMMAND:
  case VACATION_COMMAND:
    new = store_get(sizeof(filter_cmd) + mailargs_total * sizeof(union argtypes), GET_UNTAINTED);
    new->next = NULL;
    new->command = command;
    new->seen = seen_force ? seen_value : FALSE;
    new->noerror = noerror_force;
    for (i = 0; i < mailargs_total; i++) new->args[i].u = NULL;

    /* Read keyword/value pairs until we hit one that isn't. The data
    must contain only printing chars plus tab, though the "text" value
    can also contain newlines. The "file" keyword can be preceded by the
    word "expand", and "return message" has no data. */

    for (;;)
      {
      const uschar * saveptr = ptr;
      ptr = nextword(ptr, buffer, sizeof(buffer), FALSE);
      if (*error_pointer)
	{ yield = FALSE; break; }

      /* Ensure "return" is followed by "message"; that's a complete option */

      if (Ustrcmp(buffer, "return") == 0)
	{
	new->args[mailarg_index_return].u = US"";  /* not NULL => TRUE */
	ptr = nextword(ptr, buffer, sizeof(buffer), FALSE);
	if (Ustrcmp(buffer, "message") != 0)
	  {
	  *error_pointer = string_sprintf("\"return\" not followed by \"message\" "
	    " near line %d of filter file", line_number);
	  yield = FALSE;
	  break;
	  }
	continue;
	}

      /* Ensure "expand" is followed by "file", then fall through to process the
      file keyword. */

      if (Ustrcmp(buffer, "expand") == 0)
	{
	new->args[mailarg_index_expand].u = US"";  /* not NULL => TRUE */
	ptr = nextword(ptr, buffer, sizeof(buffer), FALSE);
	if (Ustrcmp(buffer, "file") != 0)
	  {
	  *error_pointer = string_sprintf("\"expand\" not followed by \"file\" "
	    " near line %d of filter file", line_number);
	  yield = FALSE;
	  break;
	  }
	}

      /* Scan for the keyword */

      for (i = 0; i < MAILARGS_STRING_COUNT; i++)
	if (Ustrcmp(buffer, mailargs[i]) == 0) break;

      /* Not found keyword; assume end of this command */

      if (i >= MAILARGS_STRING_COUNT)
	{
	ptr = saveptr;
	break;
	}

      /* Found keyword, read the data item */

      ptr = nextitem(ptr, buffer, sizeof(buffer), FALSE);
      if (*error_pointer)
	{ yield = FALSE; break; }
      else new->args[i].u = string_copy(buffer);
      }

    /* If this is the vacation command, apply some default settings to
    some of the arguments. */

    if (command == VACATION_COMMAND)
      {
      if (!new->args[mailarg_index_file].u)
	{
	new->args[mailarg_index_file].u = string_copy(US".vacation.msg");
	new->args[mailarg_index_expand].u = US"";   /* not NULL => TRUE */
	}
      if (!new->args[mailarg_index_log].u)
	new->args[mailarg_index_log].u = string_copy(US".vacation.log");
      if (!new->args[mailarg_index_once].u)
	new->args[mailarg_index_once].u = string_copy(US".vacation");
      if (!new->args[mailarg_index_once_repeat].u)
	new->args[mailarg_index_once_repeat].u = string_copy(US"7d");
      if (!new->args[mailarg_index_subject].u)
	new->args[mailarg_index_subject].u = string_copy(US"On vacation");
      }

    /* Join the address on to the chain of generated addresses */

    **lastcmdptr = new;
    *lastcmdptr = &(new->next);
    break;


  /* Seen and unseen just set flags */

  case SEEN_COMMAND:
  case UNSEEN_COMMAND:
    if (!*ptr)
      {
      *error_pointer = string_sprintf("\"seen\" or \"unseen\" "
	"near line %d is not followed by a command", line_number);
      yield = FALSE;
      }
    if (seen_force)
      {
      *error_pointer = string_sprintf("\"seen\" or \"unseen\" repeated "
	"near line %d", line_number);
      yield = FALSE;
      }
    seen_value = (command == SEEN_COMMAND);
    seen_force = TRUE;
    was_seen_or_unseen = TRUE;
    break;


  /* So does noerror */

  case NOERROR_COMMAND:
    if (!*ptr)
      {
      *error_pointer = string_sprintf("\"noerror\" "
	"near line %d is not followed by a command", line_number);
      yield = FALSE;
      }
    noerror_force = TRUE;
    was_noerror = TRUE;
    break;


  /* Oops */

  default:
    *error_pointer = string_sprintf("unknown filtering command %q "
      "near line %d of filter file", buffer, line_number);
    yield = FALSE;
    break;
  }

if (!was_seen_or_unseen && !was_noerror)
  {
  seen_force = FALSE;
  noerror_force = FALSE;
  }

*pptr = ptr;
return yield;
}



/*************************************************
*              Read a list of commands           *
*************************************************/

/* If conditional is TRUE, the list must be terminated
by the words "else" or "endif".

Arguments:
  pptr        points to pointer to next character; the pointer is updated
  lastcmdptr  points to pointer to pointer to previously-read command; used
                for hanging on the new command
  conditional TRUE if this command is the subject of a condition

Returns:      TRUE on success
*/

static BOOL
read_command_list(const uschar **pptr, filter_cmd ***lastcmdptr, BOOL conditional)
{
if (conditional) expect_endif++;
had_else_endif = had_neither;
while (**pptr && had_else_endif == had_neither)
  {
  if (!read_command(pptr, lastcmdptr)) return FALSE;
  *pptr = nextsigchar(*pptr, TRUE);
  }
if (conditional)
  {
  expect_endif--;
  if (had_else_endif == had_neither)
    {
    *error_pointer = US"\"endif\" missing at end of filter file";
    return FALSE;
    }
  }
return TRUE;
}




/*************************************************
*        Test for a personal message             *
*************************************************/

/* Module API: This function is also called from the code that
implements Sieve filters.

Arguments:
  aliases    a chain of aliases
  scan_cc    TRUE if Cc: and Bcc: are to be scanned (Exim filters do not)

Returns:     TRUE if the message is deemed to be personal
*/

static BOOL
filter_personal(string_item *aliases, BOOL scan_cc)
{
const uschar *self, *self_from, *self_to;
uschar *psself = NULL;
const uschar *psself_from = NULL, *psself_to = NULL;
rmark reset_point = store_mark();
BOOL yield;
header_line *h;
int to_count = 2;
int from_count = 9;

/* If any header line in the message is a defined "List-" header field, it is
not a personal message. We used to check for any header line that started with
"List-", but this was tightened up for release 4.54. The check is now for
"List-Id", defined in RFC 2929, or "List-Help", "List-Subscribe", "List-
Unsubscribe", "List-Post", "List-Owner" or "List-Archive", all of which are
defined in RFC 2369. We also scan for "Auto-Submitted"; if it is found to
contain any value other than "no", the message is not personal (RFC 3834).
Previously the test was for "auto-". */

for (h = header_list; h; h = h->next)
  {
  if (h->type == htype_old) continue;

  if (strncmpic(h->text, US"List-", 5) == 0)
    {
    const uschar * s = h->text + 5;
    if (strncmpic(s, US"Id:", 3) == 0 ||
        strncmpic(s, US"Help:", 5) == 0 ||
        strncmpic(s, US"Subscribe:", 10) == 0 ||
        strncmpic(s, US"Unsubscribe:", 12) == 0 ||
        strncmpic(s, US"Post:", 5) == 0 ||
        strncmpic(s, US"Owner:", 6) == 0 ||
        strncmpic(s, US"Archive:", 8) == 0)
      return FALSE;
    }

  else if (strncmpic(h->text, US"Auto-submitted:", 15) == 0)
    {
    uschar * s = h->text + 15;
    Uskip_whitespace(&s);
    if (strncmpic(s, US"no", 2) != 0) return FALSE;
    s += 2;
    Uskip_whitespace(&s);
    if (*s) return FALSE;
    }
  }

/* Set up "my" address */

self = string_sprintf("%s@%s", deliver_localpart, deliver_domain);
self_from = rewrite_one(self, rewrite_from, NULL, FALSE, US"",
  global_rewrite_rules);
self_to   = rewrite_one(self, rewrite_to, NULL, FALSE, US"",
  global_rewrite_rules);


if (!self_from) self_from = self;
if (self_to) self_to = self;

/* If there's a prefix or suffix set, we must include the prefixed/
suffixed version of the local part in the tests. */

if (deliver_localpart_prefix || deliver_localpart_suffix)
  {
  psself = string_sprintf("%s%s%s@%s",
    deliver_localpart_prefix ? deliver_localpart_prefix : US"",
    deliver_localpart,
    deliver_localpart_suffix ? deliver_localpart_suffix : US"",
    deliver_domain);
  psself_from = rewrite_one(psself, rewrite_from, NULL, FALSE, US"",
    global_rewrite_rules);
  psself_to   = rewrite_one(psself, rewrite_to, NULL, FALSE, US"",
    global_rewrite_rules);
  if (psself_from == NULL) psself_from = psself;
  if (psself_to == NULL) psself_to = psself;
  to_count += 2;
  from_count += 2;
  }

/* Do all the necessary tests; the counts are adjusted for {pre,suf}fix */

yield =
  (
  header_match(US"to:", TRUE, TRUE, aliases, to_count, self, self_to, psself,
               psself_to) ||
    (scan_cc &&
       (
       header_match(US"cc:", TRUE, TRUE, aliases, to_count, self, self_to,
                             psself, psself_to)
       ||
       header_match(US"bcc:", TRUE, TRUE, aliases, to_count, self, self_to,
                              psself, psself_to)
       )
    )
  ) &&

  header_match(US"from:", TRUE, FALSE, aliases, from_count, "^server@",
    "^daemon@", "^root@", "^listserv@", "^majordomo@", "^.*?-request@",
    "^owner-[^@]+@", self, self_from, psself, psself_from) &&

  header_match(US"precedence:", FALSE, FALSE, NULL, 3, "bulk","list","junk") &&

  (sender_address == NULL || sender_address[0] != 0);

store_reset(reset_point);
return yield;
}



/*************************************************
*             Test a condition                   *
*************************************************/

/*
Arguments:
  c              points to the condition block; c->testfor indicated whether
                   it's a positive or negative condition
  toplevel       TRUE if called from "if" directly; FALSE otherwise

Returns:         TRUE if the condition is met
*/

static BOOL
test_condition(condition_block * c, BOOL toplevel)
{
BOOL yield = FALSE, textonly_re;
const uschar * exp[2], * p, * pp;
int val[2];

if (!c) return TRUE;  /* does this ever occur? */

switch (c->type)
  {
  case cond_and:
    yield = test_condition(c->left.c, FALSE)
	    && !*error_pointer
	    && test_condition(c->right.c, FALSE);
    break;

  case cond_or:
    yield = test_condition(c->left.c, FALSE)
	    || (!*error_pointer && test_condition(c->right.c, FALSE));
    break;

    /* The personal test is meaningless in a system filter. The tests are now in
    a separate function (so Sieve can use them). However, an Exim filter does not
    scan Cc: (hence the FALSE argument). */

  case cond_personal:
    yield = f.system_filtering? FALSE : filter_personal(c->left.a, FALSE);
    break;

  case cond_delivered:
    yield = filter_delivered;
    break;

    /* Only TRUE if a message is actually being processed; FALSE for address
    testing and verification. */

  case cond_errormsg:
    yield = message_id[0] && (!sender_address || !*sender_address);
    break;

    /* Only FALSE if a message is actually being processed; TRUE for address
    and filter testing and verification. */

  case cond_firsttime:
    yield = filter_test != FTEST_NONE || !message_id[0] || f.deliver_firsttime;
    break;

    /* Only TRUE if a message is actually being processed; FALSE for address
    testing and verification. */

  case cond_manualthaw:
    yield = message_id[0] && f.deliver_manual_thaw;
    break;

    /* The foranyaddress condition loops through a list of addresses */

  case cond_foranyaddress:
    p = c->left.u;
    if (!(pp = expand_string(p)))
      {
      *error_pointer = string_sprintf("failed to expand %q in "
	"filter file: %s", p, expand_string_message);
      return FALSE;
      }

    yield = FALSE;
    f.parse_allow_group = TRUE;     /* Allow group syntax */

    while (*pp)
      {
      uschar * error;
      int start, end, domain;
      const uschar * s;

      p = parse_find_address_end(pp, FALSE);
      s = string_copyn(pp, p - pp);

      filter_thisaddress =
	parse_extract_address(s, &error, &start, &end, &domain, FALSE);

      if (filter_thisaddress)
	{
	if ((filter_test != FTEST_NONE && debug_selector != 0) ||
	    (debug_selector & D_filter) != 0)
	  {
	  indent();
	  debug_printf_indent("Extracted address %s\n", filter_thisaddress);
	  }
	yield = test_condition(c->right.c, FALSE);
	}

      if (yield) break;
      if (!*p) break;
      pp = p + 1;
      }

    f.parse_allow_group = FALSE;      /* Reset group syntax flags */
    f.parse_found_group = FALSE;
    break;

    /* All other conditions have left and right values that need expanding;
    on error, it doesn't matter what value is returned. */

    default:
    p = c->left.u;
    for (int i = 0; i < 2; i++)
      {
      if (!(exp[i] = expand_string_2(p, &textonly_re)))
	{
	*error_pointer = string_sprintf("failed to expand %q in "
	  "filter file: %s", p, expand_string_message);
	return FALSE;
	}
      p = c->right.u;
      }

    /* Inner switch for the different cases */

    switch(c->type)
      {
      case cond_is:
	yield = strcmpic(exp[0], exp[1]) == 0;
	break;

      case cond_IS:
	yield = Ustrcmp(exp[0], exp[1]) == 0;
	break;

      case cond_contains:
	yield = strstric_c(exp[0], exp[1], FALSE) != NULL;
	break;

      case cond_CONTAINS:
	yield = Ustrstr(exp[0], exp[1]) != NULL;
	break;

      case cond_begins:
	yield = strncmpic(exp[0], exp[1], Ustrlen(exp[1])) == 0;
	break;

      case cond_BEGINS:
	yield = Ustrncmp(exp[0], exp[1], Ustrlen(exp[1])) == 0;
	break;

      case cond_ends:
      case cond_ENDS:
	{
	int len = Ustrlen(exp[1]);
	const uschar *s = exp[0] + Ustrlen(exp[0]) - len;
	yield = s < exp[0]
	  ? FALSE
	  : (c->type == cond_ends ? strcmpic(s, exp[1]) : Ustrcmp(s, exp[1])) == 0;
	break;
	}

      case cond_matches:
      case cond_MATCHES:
	{
	const pcre2_code * re;
	mcs_flags flags = textonly_re ? MCS_CACHEABLE : MCS_NOFLAGS;

	if ((filter_test != FTEST_NONE && debug_selector != 0) ||
	    (debug_selector & D_filter) != 0)
	  {
	  debug_printf_indent("Match expanded arguments:\n");
	  debug_printf_indent("  Subject = %s\n", exp[0]);
	  debug_printf_indent("  Pattern = %s\n", exp[1]);
	  }

	if (c->type == cond_matches) flags |= MCS_CASELESS;
	if (!(re = regex_compile(exp[1], flags, error_pointer, pcre_gen_cmp_ctx)))
	  return FALSE;

	yield = regex_match_and_setup(re, exp[0], PCRE_EOPT, -1);
	break;
	}

      /* For above and below, convert the strings to numbers */

      case cond_above:
      case cond_below:
	for (int i = 0; i < 2; i++)
	  {
	  val[i] = get_number(exp[i], &yield);
	  if (!yield)
	    {
	    *error_pointer = string_sprintf("malformed numerical string %q",
	      exp[i]);
	    return FALSE;
	    }
	  }
	yield = c->type == cond_above ? (val[0] > val[1]) : (val[0] < val[1]);
	break;
      }
    break;
  }

if ((filter_test != FTEST_NONE && debug_selector != 0) ||
    (debug_selector & D_filter) != 0)
  {
  indent();
  debug_printf_indent("%sondition is %s: ",
    toplevel ? "C" : "Sub-c",
    yield == c->testfor ? "true" : "false");
  print_condition(c, TRUE);
  debug_printf_indent("\n");
  }

return yield == c->testfor;
}



/*************************************************
*          Interpret chain of commands           *
*************************************************/

/* In testing state, just say what would be done rather than doing it. The
testprint command just expands and outputs its argument in testing state, and
does nothing otherwise.

Arguments:
  commands    points to chain of commands to interpret
  generated   where to hang newly-generated addresses

Returns:      FF_DELIVERED     success, a significant action was taken
              FF_NOTDELIVERED  success, no significant action
              FF_DEFER         defer requested
              FF_FAIL          fail requested
              FF_FREEZE        freeze requested
              FF_ERROR         there was a problem
*/

static int
interpret_commands(filter_cmd *commands, address_item **generated)
{
const uschar *s;
int mode;
address_item *addr;
BOOL condition_value;

while (commands)
  {
  int ff_ret;
  uschar *fmsg, *ff_name;
  const uschar *expargs[MAILARGS_STRING_COUNT];

  int i, n[2];

  /* Expand the relevant number of arguments for the command that are
  not NULL. */

  for (i = 0; i < (command_exparg_count[commands->command] & 15); i++)
    {
    const uschar *ss = commands->args[i].u;
    if (!ss)
      expargs[i] = NULL;
    else if (!(expargs[i] = expand_string(ss)))
      {
      *error_pointer = string_sprintf("failed to expand %q in "
	"%s command: %s", ss, command_list[commands->command],
	expand_string_message);
      return FF_ERROR;
      }
    }

  /* Now switch for each command, setting the "delivered" flag if any of them
  have "seen" set. */

  if (commands->seen) filter_delivered = TRUE;

  switch(commands->command)
    {
    case ADD_COMMAND:
      for (i = 0; i < 2; i++)
	{
	const uschar *ss = expargs[i];
	uschar *end;

	if (i == 1 && (*ss++ != 'n' || ss[1] != 0))
	  {
	  *error_pointer = string_sprintf("unknown variable %q in \"add\" "
	    "command", expargs[i]);
	  return FF_ERROR;
	  }

	/* Allow for "--" at the start of the value (from -$n0) for example */
	if (i == 0) while (ss[0] == '-' && ss[1] == '-') ss += 2;

	n[i] = (int)Ustrtol(ss, &end, 0);
	if (*end != 0)
	  {
	  *error_pointer = string_sprintf("malformed number %q in \"add\" "
	    "command", ss);
	  return FF_ERROR;
	  }
	}

      filter_n[n[1]] += n[0];
      if (filter_test != FTEST_NONE) printf("Add %d to n%d\n", n[0], n[1]);
      break;

      /* A deliver command's argument must be a valid address. Its optional
      second argument (system filter only) must also be a valid address. */

    case DELIVER_COMMAND:
      for (i = 0; i < 2; i++)
	{
	s = expargs[i];
	if (s != NULL)
	  {
	  int start, end, domain;
	  uschar * error;
	  const uschar * ss = parse_extract_address(s, &error,
						  &start, &end, &domain, FALSE);
	  if (ss)
	    expargs[i] = filter_options & RDO_REWRITE
	      ? rewrite_address(ss, TRUE, FALSE, global_rewrite_rules,
				rewrite_existflags)
	      : rewrite_address_qualify(ss, TRUE);
	  else
	    {
	    *error_pointer = string_sprintf("malformed address %q in "
	      "filter file: %s", s, error);
	    return FF_ERROR;
	    }
	  }
	}

      /* Stick the errors address into a simple variable, as it will
      be referenced a few times. Check that the caller is permitted to
      specify it. */

      s = expargs[1];

      if (s && !f.system_filtering)
	{
	const uschar * ownaddress = expand_string(US"$local_part@$domain");
	if (strcmpic(ownaddress, s) != 0)
	  {
	  *error_pointer = US"errors_to must point to the caller's address";
	  return FF_ERROR;
	  }
	}

      /* Test case: report what would happen */

      if (filter_test != FTEST_NONE)
	{
	indent();
	printf("%seliver message to: %s%s%s%s\n",
	  commands->seen ? "D" : "Unseen d",
	  expargs[0],
	  commands->noerror? " (noerror)" : "",
	  s ? " errors_to " : "",
	  s ? s : US"");
	}

      /* Real case. */

      else
	{
	DEBUG(D_filter) debug_printf_indent("Filter: %sdeliver message to: %s%s%s%s\n",
	  commands->seen ? "" : "unseen ",
	  expargs[0],
	  commands->noerror ? " (noerror)" : "",
	  s ? " errors_to " : "",
	  s ? s : US"");

	/* Create the new address and add it to the chain, setting the
	af_ignore_error flag if necessary, and the errors address, which can be
	set in a system filter and to the local address in user filters. */

	addr = deliver_make_addr(US expargs[0], TRUE);  /* TRUE => copy s, so deconst ok */
	addr->prop.errors_address = !s ? NULL : string_copy(s); /* Default is NULL */
	if (commands->noerror) addr->prop.ignore_error = TRUE;
	addr->next = *generated;
	*generated = addr;
	}
      break;

    case SAVE_COMMAND:
      s = expargs[0];
      mode = commands->args[1].i;

      /* Test case: report what would happen */

      if (filter_test != FTEST_NONE)
	{
	indent();
	if (mode < 0)
	  printf("%save message to: %s%s\n",
	    commands->seen ? "S" : "Unseen s",
	    s, commands->noerror ? " (noerror)" : "");
	else
	  printf("%save message to: %s %04o%s\n",
	  commands->seen ?  "S" : "Unseen s",
	  s, mode, commands->noerror ? " (noerror)" : "");
	}

      /* Real case: Ensure save argument starts with / if there is a home
      directory to prepend. */

      else
	{
	if (s[0] != '/' && filter_options & RDO_PREPEND_HOME &&
	    deliver_home && *deliver_home)
	  s = string_sprintf("%s/%s", deliver_home, s);
	DEBUG(D_filter) debug_printf_indent("Filter: %ssave message to: %s%s\n",
	  commands->seen ? "" : "unseen ",
	  s, commands->noerror ? " (noerror)" : "");

	/* Create the new address and add it to the chain, setting the
	af_pfr and af_file flags, the af_ignore_error flag if necessary, and the
	mode value. */

	addr = deliver_make_addr(US s, TRUE);  /* TRUE => copy s, so deconst ok */
	setflag(addr, af_pfr);
	setflag(addr, af_file);
	if (commands->noerror) addr->prop.ignore_error = TRUE;
	addr->mode = mode;
	addr->next = *generated;
	*generated = addr;
	}
      break;

    case PIPE_COMMAND:
      s = string_copy(commands->args[0].u);
      if (filter_test != FTEST_NONE)
	{
	indent();
	printf("%sipe message to: %s%s\n",
	  commands->seen ? "P" : "Unseen p",
	  s, commands->noerror? " (noerror)" : "");
	}
      else /* Ensure pipe command starts with | */
	{
	DEBUG(D_filter) debug_printf_indent("Filter: %spipe message to: %s%s\n",
	  commands->seen ? "" : "unseen ", s,
	  commands->noerror ? " (noerror)" : "");
	if (s[0] != '|') s = string_sprintf("|%s", s);

	/* Create the new address and add it to the chain, setting the
	af_ignore_error flag if necessary. Set the af_expand_pipe flag so that
	each command argument is expanded in the transport after the command
	has been split up into separate arguments. */

	addr = deliver_make_addr(US s, TRUE);  /* TRUE => copy s, so deconst ok */
	setflag(addr, af_pfr);
	setflag(addr, af_expand_pipe);
	if (commands->noerror) addr->prop.ignore_error = TRUE;
	addr->next = *generated;
	*generated = addr;

	/* If there are any numeric variables in existence (e.g. after a regex
	condition), or if $thisaddress is set, take a copy for use in the
	expansion. Note that we can't pass NULL for filter_thisaddress, because
	NULL terminates the list. */

	if (expand_nmax >= 0 || filter_thisaddress != NULL)
	  {
	  int ecount = expand_nmax >= 0 ? expand_nmax : -1;
	  uschar ** ss = store_get(sizeof(uschar *) * (ecount + 3), GET_UNTAINTED);

	  addr->pipe_expandn = ss;
	  if (!filter_thisaddress) filter_thisaddress = US"";
	  *ss++ = string_copy(filter_thisaddress);
	  for (int j = 0; j <= expand_nmax; j++)
	    *ss++ = string_copyn(expand_nstring[j], expand_nlength[j]);
	  *ss = NULL;
	  }
	}
      break;

      /* Set up the file name and mode, and close any previously open
      file. */

    case LOGFILE_COMMAND:
      log_mode = commands->args[1].i;
      if (log_mode == -1) log_mode = 0600;
      if (log_fd >= 0)
	{
	(void)close(log_fd);
	log_fd = -1;
	}
      log_filename = expargs[0];
      if (filter_test != FTEST_NONE)
	{
	indent();
	printf("%sogfile %s\n", commands->seen ? "Seen l" : "L", log_filename);
	}
      break;

    case LOGWRITE_COMMAND:
      s = expargs[0];

      if (filter_test != FTEST_NONE)
	{
	indent();
	printf("%sogwrite \"%s\"\n", commands->seen ? "Seen l" : "L",
	  string_printing(s));
	}

      /* Attempt to write to a log file only if configured as permissible.
      Logging may be forcibly skipped for verifying or testing. */

      else if (filter_options & RDO_LOG)   /* Locked out */
	{
	DEBUG(D_filter)
	  debug_printf_indent("filter log command aborted: euid=%ld\n",
	  (long int)geteuid());
	*error_pointer = US"logwrite command forbidden";
	return FF_ERROR;
	}
      else if (filter_options & RDO_REALLOG)
	{
	int len;
	DEBUG(D_filter) debug_printf_indent("writing filter log as euid %ld\n",
	  (long int)geteuid());
	if (log_fd < 0)
	  {
	  if (!log_filename)
	    {
	    *error_pointer = US"attempt to obey \"logwrite\" command "
	      "without a previous \"logfile\"";
	    return FF_ERROR;
	    }
	  log_fd = Uopen(log_filename, O_CREAT|O_APPEND|O_WRONLY, log_mode);
	  if (log_fd < 0)
	    {
	    *error_pointer = string_open_failed("filter log file %q",
	      log_filename);
	    return FF_ERROR;
	    }
	  }
	len = Ustrlen(s);
	if (write(log_fd, s, len) != len)
	  {
	  *error_pointer = string_sprintf("write error on file %q: %s",
	    log_filename, strerror(errno));
	  return FF_ERROR;
	  }
	}
      else
	DEBUG(D_filter)
	  debug_printf_indent("skipping logwrite (verifying or testing)\n");
      break;

      /* Header addition and removal is available only in the system filter. The
      command is rejected at parse time otherwise. However "headers charset" is
      always permitted. */

    case HEADERS_COMMAND:
	{
	int subtype = commands->args[1].i;
	s = expargs[0];

	if (filter_test != FTEST_NONE)
	  printf("Headers %s \"%s\"\n",
	    subtype == TRUE ? "add"
	    : subtype == FALSE ? "remove"
	    : "charset",
	    string_printing(s));

	if (subtype == TRUE)
	  {
	  if (Uskip_whitespace(&s))
	    {
	    header_add(htype_other, "%s%s", s,
	      s[Ustrlen(s)-1] == '\n' ? "" : "\n");
	    header_last->type = header_checkname(header_last, FALSE);
	    if (header_last->type >= 'a') header_last->type = htype_other;
	    }
	  }

	else if (subtype == FALSE)
	  {
	  int sep = 0;
	  const uschar * list = s;

	  for (uschar * ss; ss = string_nextinlist(&list, &sep, NULL, 0); )
	    header_remove(0, ss);
	  }

	/* This setting lasts only while the filter is running; on exit, the
	variable is reset to the previous value. */

	else headers_charset = s;
	}
      break;

      /* Defer, freeze, and fail are available only when explicitly permitted.
      These commands are rejected at parse time otherwise. The message can get
      very long by the inclusion of message headers; truncate if it is, and also
      ensure printing characters so as not to mess up log files. */

    case DEFER_COMMAND:
      ff_name = US"defer";
      ff_ret = FF_DEFER;
      goto DEFERFREEZEFAIL;

    case FAIL_COMMAND:
      ff_name = US"fail";
      ff_ret = FF_FAIL;
      goto DEFERFREEZEFAIL;

    case FREEZE_COMMAND:
      ff_name = US"freeze";
      ff_ret = FF_FREEZE;

    DEFERFREEZEFAIL:
      *error_pointer = fmsg = US string_printing(Ustrlen(expargs[0]) > 1024
	? string_sprintf("%.1000s ... (truncated)", expargs[0])
	: string_copy(expargs[0]));
      for(uschar * t = fmsg; *t; t++)
	if (!t[1] && *t == '\n') { *t = '\0'; break; }	/* drop trailing newline */

      if (filter_test != FTEST_NONE)
	{
	indent();
	printf("%c%s text \"%s\"\n", toupper(ff_name[0]), ff_name+1, fmsg);
	}
      else
        DEBUG(D_filter) debug_printf_indent("Filter: %s %q\n", ff_name, fmsg);
      return ff_ret;

    case FINISH_COMMAND:
      if (filter_test != FTEST_NONE)
	{
	indent();
	printf("%sinish\n", commands->seen ? "Seen f" : "F");
	}
      else
	DEBUG(D_filter) debug_printf_indent("Filter: %sfinish\n",
	  commands->seen ? " Seen " : "");
      finish_obeyed = TRUE;
      return filter_delivered ? FF_DELIVERED : FF_NOTDELIVERED;

    case IF_COMMAND:
	{
	uschar *save_address = filter_thisaddress;
	int ok = FF_DELIVERED;
	condition_value = test_condition(commands->args[0].c, TRUE);
	if (*error_pointer)
	  ok = FF_ERROR;
	else
	  {
	  output_indent += 2;
	  ok = interpret_commands(commands->args[condition_value ? 1:2].f,
	    generated);
	  output_indent -= 2;
	  }
	filter_thisaddress = save_address;
	if (finish_obeyed  ||  ok != FF_DELIVERED && ok != FF_NOTDELIVERED)
	  return ok;
	}
      break;


      /* To try to catch runaway loops, do not generate mail if the
      return path is unset or if a non-trusted user supplied -f <>
      as the return path. */

    case MAIL_COMMAND:
    case VACATION_COMMAND:
	if (!return_path || !*return_path)
	  {
	  if (filter_test != FTEST_NONE)
	    printf("%s command ignored because return_path is empty\n",
	      command_list[commands->command]);
	  else DEBUG(D_filter)
	    debug_printf_indent("%s command ignored because return_path "
	    "is empty\n", command_list[commands->command]);
	  break;
	  }

	/* Check the contents of the strings. The type of string can be deduced
	from the value of i.

	. If i is equal to mailarg_index_text it's a text string for the body,
	  where anything goes.

	. If i is > mailarg_index_text, we are dealing with a file name, which
	  cannot contain non-printing characters.

	. If i is less than mailarg_index_headers we are dealing with something
	  that will go in a single message header line, where newlines must be
	  followed by white space.

	. If i is equal to mailarg_index_headers, we have a string that contains
	  one or more headers. Newlines that are not followed by white space must
	  be followed by a header name.
	*/

	for (i = 0; i < MAILARGS_STRING_COUNT; i++)
	  {
	  const uschar * t = expargs[i];

	  if (!t) continue;

	  if (i != mailarg_index_text) for (const uschar * p = t; *p; p++)
	    {
	    int c = *p;
	    if (i > mailarg_index_text)
	      {
	      if (!mac_isprint(c))
		{
		*error_pointer = string_sprintf("non-printing character in %q "
		  "in %s command", string_printing(t),
		  command_list[commands->command]);
		return FF_ERROR;
		}
	      }

	    /* i < mailarg_index_text */

	    else if (c == '\n' && !isspace(p[1]))
	      {
	      if (i < mailarg_index_headers)
		{
		*error_pointer = string_sprintf("\\n not followed by space in "
		  "\"%.1024s\" in %s command", string_printing(t),
		  command_list[commands->command]);
		return FF_ERROR;
		}

	      /* Check for the start of a new header line within the string */

	      else
		{
		const uschar *pp;
		for (pp = p + 1;; pp++)
		  {
		  c = *pp;
		  if (c == ':' && pp != p + 1) break;
		  if (!c || c == ':' || isspace(c))
		    {
		    *error_pointer = string_sprintf("\\n not followed by space or "
		      "valid header name in \"%.1024s\" in %s command",
		      string_printing(t), command_list[commands->command]);
		    return FF_ERROR;
		    }
		  }
		p = pp;
		}
	      }
	    }       /* Loop to scan the string */

	  /* The string is OK */

	  commands->args[i].u = t;
	  }

	/* Proceed with mail or vacation command */

	if (filter_test != FTEST_NONE)
	  {
	  const uschar *to = commands->args[mailarg_index_to].u;
	  indent();
	  printf("%sail to: %s%s%s\n", (commands->seen)? "Seen m" : "M",
	    to ? to : US"<default>",
	    commands->command == VACATION_COMMAND ? " (vacation)" : "",
	    commands->noerror ? " (noerror)" : "");
	  for (i = 1; i < MAILARGS_STRING_COUNT; i++)
	    {
	    const uschar * arg = commands->args[i].u;
	    if (arg)
	      {
	      int len = Ustrlen(mailargs[i]);
	      int indent = debug_selector != 0 ? output_indent : 0;
	      while (len++ < 7 + indent) printf(" ");
	      printf("%s: %s%s\n", mailargs[i], string_printing(arg),
		(  commands->args[mailarg_index_expand].u
		&& Ustrcmp(mailargs[i], "file") == 0) ? " (expanded)" : "");
	      }
	    }
	  if (commands->args[mailarg_index_return].u)
	    printf("Return original message\n");
	  }
	else
	  {
	  const uschar * tt;
	  const uschar * to = commands->args[mailarg_index_to].u;
	  gstring * log_addr = NULL;

	  if (!to) to = expand_string(US"$reply_address");
	  Uskip_whitespace(&to);

	  for (tt = to; *tt; tt++)     /* Get rid of newlines */
	    if (*tt == '\n')
	      {
	      uschar * p = string_copy(to);
	      for (uschar * ss = p; *ss; ss++)
		if (*ss == '\n') *ss = ' ';
	      to = p;
	      break;
	      }

	  DEBUG(D_filter)
	    {
	    debug_printf_indent("Filter: %smail to: %s%s%s\n",
	      commands->seen ? "seen " : "",
	      to,
	      commands->command == VACATION_COMMAND ? " (vacation)" : "",
	      commands->noerror ? " (noerror)" : "");
	    for (i = 1; i < MAILARGS_STRING_COUNT; i++)
	      {
	      const uschar *arg = commands->args[i].u;
	      if (arg)
		{
		int len = Ustrlen(mailargs[i]);
		while (len++ < 15) debug_printf_indent(" ");
		debug_printf_indent("%s: %s%s\n", mailargs[i], string_printing(arg),
		  (commands->args[mailarg_index_expand].u != NULL &&
		    Ustrcmp(mailargs[i], "file") == 0)? " (expanded)" : "");
		}
	      }
	    }

	  /* Create the "address" for the autoreply. This is used only for logging,
	  as the actual recipients are extracted from the To: line by -t. We use the
	  same logic here to extract the working addresses (there may be more than
	  one). Just in case there are a vast number of addresses, stop when the
	  string gets too long. */

	  tt = to;
	  while (*tt)
	    {
	    uschar * ss = parse_find_address_end(tt, FALSE), * errmess;
	    const uschar * recipient;
	    int start, end, domain;
	    int temp = *ss;

	    *ss = 0;
	    recipient = parse_extract_address(tt, &errmess,
					      &start, &end, &domain, FALSE);
	    *ss = temp;

	    /* Ignore empty addresses and errors; an error will occur later if
	    there's something really bad. */

	    if (recipient)
	      {
	      log_addr = string_catn(log_addr, log_addr ? US"," : US">", 1);
	      log_addr = string_cat (log_addr, recipient);
	      }

	    /* Check size */

	    if (log_addr && log_addr->ptr > 256)
	      {
	      log_addr = string_catn(log_addr, US", ...", 5);
	      break;
	      }

	    /* Move on past this address */

	    tt = ss + (*ss ? 1 : 0);
	    Uskip_whitespace(&tt);
	    }

	  if (log_addr)
	    addr = deliver_make_addr(string_from_gstring(log_addr), FALSE);
	  else
	    {
	    addr = deliver_make_addr(US ">**bad-reply**", FALSE);
	    setflag(addr, af_bad_reply);
	    }

	  setflag(addr, af_pfr);
	  if (commands->noerror) addr->prop.ignore_error = TRUE;
	  addr->next = *generated;
	  *generated = addr;

	  addr->reply = store_get(sizeof(reply_item), GET_UNTAINTED);
	  addr->reply->from = NULL;
	  addr->reply->to = string_copy(to);
	  addr->reply->file_expand =
	    commands->args[mailarg_index_expand].u != NULL;
	  addr->reply->expand_forbid = expand_forbid;
	  addr->reply->return_message =
	    commands->args[mailarg_index_return].u != NULL;
	  addr->reply->once_repeat = 0;

	  if (commands->args[mailarg_index_once_repeat].u != NULL)
	    {
	    addr->reply->once_repeat =
	      readconf_readtime(commands->args[mailarg_index_once_repeat].u, 0,
		FALSE);
	    if (addr->reply->once_repeat < 0)
	      {
	      *error_pointer = string_sprintf("Bad time value for \"once_repeat\" "
		"in mail or vacation command: %s",
		commands->args[mailarg_index_once_repeat].u);
	      return FF_ERROR;
	      }
	    }

	  /* Set up all the remaining string arguments (those other than "to") */

	  for (i = 1; i < mailargs_string_passed; i++)
	    {
	    const uschar *ss = commands->args[i].u;
	    *(USS((US addr->reply) + reply_offsets[i])) =
	      ss ? string_copy(ss) : NULL;
	    }
	  }
	break;

    case TESTPRINT_COMMAND:
	if (filter_test != FTEST_NONE || (debug_selector & D_filter) != 0)
	  {
	  const uschar * t = string_printing(expargs[0]);
	  if (filter_test == FTEST_NONE)
	    debug_printf_indent("Filter: testprint: %s\n", t);
	  else
	    printf("Testprint: %s\n", t);
	  }
    }

  commands = commands->next;
  }

return filter_delivered ? FF_DELIVERED : FF_NOTDELIVERED;
}



/*************************************************
*            Interpret a mail filter file        *
*************************************************/

/* Module API:
Arguments:
  filter      points to the entire file, read into store as a single string
  options     controls whether various special things are allowed, and requests
              special actions
  generated   where to hang newly-generated addresses
  error       where to pass back an error text

Returns:      FF_DELIVERED     success, a significant action was taken
              FF_NOTDELIVERED  success, no significant action
              FF_DEFER         defer requested
              FF_FAIL          fail requested
              FF_FREEZE        freeze requested
              FF_ERROR         there was a problem
*/

static int
filter_interpret(const uschar *filter, int options, address_item **generated,
  uschar **error)
{
int i;
int yield = FF_ERROR;
const uschar *ptr = filter;
const uschar *save_headers_charset = headers_charset;
filter_cmd *commands = NULL;
filter_cmd **lastcmdptr = &commands;

DEBUG(D_route) debug_printf("Filter: start of processing\n");
acl_level++;

/* Initialize "not in an if command", set the global flag that is always TRUE
while filtering, and zero the variables. */

expect_endif = 0;
output_indent = 0;
f.filter_running = TRUE;
for (i = 0; i < FILTER_VARIABLE_COUNT; i++) filter_n[i] = 0;

/* To save having to pass certain values about all the time, make them static.
Also initialize the line number, for error messages, and the log file
variables. */

filter_options = options;
filter_delivered = FALSE;
finish_obeyed = FALSE;
error_pointer = error;
*error_pointer = NULL;
line_number = 1;
log_fd = -1;
log_mode = 0600;
log_filename = NULL;

/* Scan filter file for syntax and build up an interpretation thereof, and
interpret the compiled commands, and if testing, say whether we ended up
delivered or not, unless something went wrong. */

seen_force = FALSE;
ptr = nextsigchar(ptr, TRUE);

if (read_command_list(&ptr, &lastcmdptr, FALSE))
  yield = interpret_commands(commands, generated);

if (filter_test != FTEST_NONE || (debug_selector & D_filter) != 0)
  {
  uschar *s = US"";
  switch(yield)
    {
    case FF_DEFER:
      s = US"Filtering ended by \"defer\".";
      break;

    case FF_FREEZE:
      s = US"Filtering ended by \"freeze\".";
      break;

    case FF_FAIL:
      s = US"Filtering ended by \"fail\".";
      break;

    case FF_DELIVERED:
      s = US"Filtering set up at least one significant delivery "
	     "or other action.\n"
	     "No other deliveries will occur.";
      break;

    case FF_NOTDELIVERED:
      s = US"Filtering did not set up a significant delivery.\n"
	     "Normal delivery will occur.";
      break;

    case FF_ERROR:
      s = string_sprintf("Filter error: %s", *error);
      break;
    }

  if (filter_test != FTEST_NONE) printf("%s\n", CS s);
    else debug_printf_indent("%s\n", s);
  }

/* Close the log file if it was opened, and kill off any numerical variables
before returning. Reset the header decoding charset. */

if (log_fd >= 0) (void)close(log_fd);
expand_nmax = -1;
f.filter_running = FALSE;
headers_charset = save_headers_charset;

acl_level--;
DEBUG(D_route) debug_printf("Filter: end of processing\n");
return yield;
}




/******************************************************************************/
/* Module API */

static void * exim_functions[] = {
  [EXIM_INTERPRET] =		(void *) filter_interpret,
  [EXIM_FILTER_PERSONAL] =	(void *) filter_personal,
};

misc_module_info exim_filter_module_info =
{
  .name =		US"exim_filter",
# ifdef DYNLOOKUP
  .dyn_magic =		MISC_MODULE_MAGIC,
# endif

  .functions =		exim_functions,
  .functions_count =	nelem(exim_functions),
};

/* End of filter.c */
/* vi: aw ai sw=2
*/
