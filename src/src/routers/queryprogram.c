/*************************************************
*     Exim - an Internet mail transport agent    *
*************************************************/

/* Copyright (c) The Exim Maintainers 2020 - 2024 */
/* Copyright (c) University of Cambridge 1995 - 2018 */
/* See the file NOTICE for conditions of use and distribution. */
/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "../exim.h"

#ifdef ROUTER_QUERYPROGRAM	/* Remainder of file */
#include "rf_functions.h"
#include "queryprogram.h"



/* Options specific to the queryprogram router. */

optionlist queryprogram_router_options[] = {
  { "*expand_command_group", opt_bool | opt_hidden,
      OPT_OFF(queryprogram_router_options_block, expand_cmd_gid) },
  { "*expand_command_user", opt_bool | opt_hidden,
      OPT_OFF(queryprogram_router_options_block, expand_cmd_uid) },
  { "*set_command_group",   opt_bool | opt_hidden,
      OPT_OFF(queryprogram_router_options_block, cmd_gid_set) },
  { "*set_command_user",    opt_bool | opt_hidden,
      OPT_OFF(queryprogram_router_options_block, cmd_uid_set) },
  { "command",      opt_stringptr,
      OPT_OFF(queryprogram_router_options_block, command) },
  { "command_group",opt_expand_gid,
      OPT_OFF(queryprogram_router_options_block, cmd_gid) },
  { "command_user", opt_expand_uid,
      OPT_OFF(queryprogram_router_options_block, cmd_uid) },
  { "current_directory", opt_stringptr,
      OPT_OFF(queryprogram_router_options_block, current_directory) },
  { "timeout",      opt_time,
      OPT_OFF(queryprogram_router_options_block, timeout) }
};

/* Size of the options list. An extern variable has to be used so that its
address can appear in the tables drtables.c. */

int queryprogram_router_options_count =
  sizeof(queryprogram_router_options)/sizeof(optionlist);


#ifdef MACRO_PREDEF

/* Dummy entries */
queryprogram_router_options_block queryprogram_router_option_defaults = {0};
void queryprogram_router_init(driver_instance *rblock) {}
int queryprogram_router_entry(router_instance *rblock, address_item *addr,
  struct passwd *pw, int verify, address_item **addr_local,
  address_item **addr_remote, address_item **addr_new,
  address_item **addr_succeed) {return 0;}

#else   /*!MACRO_PREDEF*/


/* Default private options block for the queryprogram router. */

queryprogram_router_options_block queryprogram_router_option_defaults = {
  NULL,         /* command */
  60*60,        /* timeout */
  (uid_t)(-1),  /* cmd_uid */
  (gid_t)(-1),  /* cmd_gid */
  FALSE,        /* cmd_uid_set */
  FALSE,        /* cmd_gid_set */
  US"/",        /* current_directory */
  NULL,         /* expand_cmd_gid */
  NULL          /* expand_cmd_uid */
};



/*************************************************
*          Initialization entry point            *
*************************************************/

/* Called for each instance, after its options have been read, to enable
consistency checks to be done, or anything else that needs to be set up. */

void
queryprogram_router_init(driver_instance * rblock)
{
queryprogram_router_options_block *ob =
  (queryprogram_router_options_block *)(rblock->options_block);

/* A command must be given */

if (!ob->command)
  log_write_die(0, LOG_CONFIG_FOR, "%s router:\n  "
    "a command specification is required", rblock->name);

/* A uid/gid must be supplied */

if (!ob->cmd_uid_set && !ob->expand_cmd_uid)
  log_write_die(0, LOG_CONFIG_FOR, "%s router:\n  "
    "command_user must be specified", rblock->name);
}



/*************************************************
*    Process a set of generated new addresses    *
*************************************************/

/* This function sets up a set of newly generated child addresses and puts them
on the new address chain.

Arguments:
  rblock                  router block
  addr_new                new address chain
  addr                    original address
  generated               list of generated addresses
  addr_prop               the propagated data block, containing errors_to,
                            header change stuff, and address_data

Returns:         nothing
*/

static void
add_generated(router_instance * rblock, address_item ** addr_new,
  address_item * addr, address_item * generated,
  const address_item_propagated * addr_prop)
{
while (generated != NULL)
  {
  BOOL ignore_error = addr->prop.ignore_error;
  address_item *next = generated;

  generated = next->next;

  next->parent = addr;
  next->prop = *addr_prop;
  next->prop.ignore_error = next->prop.ignore_error || ignore_error;
  next->start_router = rblock->redirect_router;

  next->next = *addr_new;
  *addr_new = next;

  if (addr->child_count == USHRT_MAX)
    log_write_die(0, LOG_MAIN, "%s router generated more than %d "
      "child addresses for <%s>", rblock->drinst.name, USHRT_MAX, addr->address);
  addr->child_count++;

  DEBUG(D_route)
    debug_printf("%s router generated %s\n", rblock->drinst.name, next->address);
  }
}




/*************************************************
*              Main entry point                  *
*************************************************/

/* See local README for interface details. This router returns:

DECLINE
  . DECLINE returned
  . self = DECLINE

PASS
  . PASS returned
  . timeout of host lookup and pass_on_timeout set
  . self = PASS

DEFER
  . verifying the errors address caused a deferment or a big disaster such
      as an expansion failure (rf_get_errors_address)
  . expanding a headers_{add,remove} string caused a deferment or another
      expansion error (rf_get_munge_headers)
  . a problem in rf_get_transport: no transport when one is needed;
      failed to expand dynamic transport; failed to find dynamic transport
  . bad lookup type
  . problem looking up host (rf_lookup_hostlist)
  . self = DEFER or FREEZE
  . failure to set up uid/gid for running the command
  . failure of transport_set_up_command: too many arguments, expansion fail
  . failure to create child process
  . child process crashed or timed out or didn't return data
  . :defer: in data
  . DEFER or FREEZE returned
  . problem in redirection data
  . unknown transport name or trouble expanding router transport

FAIL
  . :fail: in data
  . FAIL returned
  . self = FAIL

OK
  . address added to addr_local or addr_remote for delivery
  . new addresses added to addr_new
*/

int
queryprogram_router_entry(
  router_instance *rblock,        /* data for this instantiation */
  address_item *addr,             /* address we are working on */
  struct passwd *pw,              /* passwd entry after check_local_user */
  int verify,                     /* v_none/v_recipient/v_sender/v_expn */
  address_item **addr_local,      /* add it to this if it's local */
  address_item **addr_remote,     /* add it to this if it's remote */
  address_item **addr_new,        /* put new addresses on here */
  address_item **addr_succeed)    /* put old address here on success */
{
int fd_in, fd_out, len, rc;
pid_t pid;
struct passwd *upw = NULL;
uschar buffer[1024];
const uschar **argvptr;
uschar *rword, *rdata, *s;
address_item_propagated addr_prop;
queryprogram_router_options_block * ob =
  (queryprogram_router_options_block *)(rblock->drinst.options_block);
uschar *current_directory = ob->current_directory;
ugid_block ugid;
uid_t curr_uid = getuid();
gid_t curr_gid = getgid();
uid_t uid = ob->cmd_uid;
gid_t gid = ob->cmd_gid;
uid_t *puid = &uid;
gid_t *pgid = &gid;

DEBUG(D_route) debug_printf_indent("%s router called for %s: domain = %s\n",
  rblock->drinst.name, addr->address, addr->domain);

ugid.uid_set = ugid.gid_set = FALSE;

/* Set up the propagated data block with the current address_data and the
errors address and extra header stuff. */

bzero(&addr_prop, sizeof(addr_prop));
addr_prop.address_data = deliver_address_data;
tree_dup((tree_node **)&addr_prop.variables, addr->prop.variables);

rc = rf_get_errors_address(addr, rblock, verify, &addr_prop.errors_address);
if (rc != OK) return rc;

rc = rf_get_munge_headers(addr, rblock, &addr_prop.extra_headers,
  &addr_prop.remove_headers);
if (rc != OK) return rc;

/* Get the fixed or expanded uid under which the command is to run
(initialization ensures that one or the other is set). */

if (  !ob->cmd_uid_set
   && !route_find_expanded_user(ob->expand_cmd_uid, rblock->drinst.name,
	US"router", &upw, &uid, &(addr->message)))
    return DEFER;

/* Get the fixed or expanded gid, or take the gid from the passwd entry. */

if (!ob->cmd_gid_set)
  if (ob->expand_cmd_gid)
    {
    if (route_find_expanded_group(ob->expand_cmd_gid, rblock->drinst.name,
        US"router", &gid, &(addr->message)))
      return DEFER;
    }
  else if (upw)
    gid = upw->pw_gid;
  else
    {
    addr->message = string_sprintf("command_user set without command_group "
      "for %s router", rblock->drinst.name);
    return DEFER;
    }

DEBUG(D_route) debug_printf("requires uid=%ld gid=%ld current_directory=%s\n",
  (long int)uid, (long int)gid, current_directory);

/* If we are not running as root, we will not be able to change uid/gid. */

if (curr_uid != root_uid && (uid != curr_uid || gid != curr_gid))
  {
  DEBUG(D_route)
    {
    debug_printf("not running as root: cannot change uid/gid\n");
    debug_printf("subprocess will run with uid=%ld gid=%ld\n",
      (long int)curr_uid, (long int)curr_gid);
    }
  puid = pgid = NULL;
  }

/* Set up the command to run */

GET_OPTION("command");
if (!transport_set_up_command(&argvptr, /* anchor for arg list */
    ob->command,                        /* raw command */
    TSUC_EXPAND_ARGS,                   /* arguments expanded but must not be tainted */
    0,                                  /* not relevant when... */
    NULL,                               /* no transporting address */
    US"queryprogram router",            /* for error messages */
    &addr->message))                    /* where to put error message */
  return DEFER;

/* Create the child process, making it a group leader. */

if ((pid = child_open_uid(argvptr, NULL, 0077, puid, pgid, &fd_in, &fd_out,
			  current_directory, TRUE, US"queryprogram-cmd")) < 0)
  {
  addr->message = string_sprintf("%s router couldn't create child process: %s",
    rblock->drinst.name, strerror(errno));
  return DEFER;
  }

/* Nothing is written to the standard input. */

(void)close(fd_in);

/* Wait for the process to finish, applying the timeout, and inspect its return
code. */

if ((rc = child_close(pid, ob->timeout)) != 0)
  {
  if (rc > 0)
    addr->message = string_sprintf("%s router: command returned non-zero "
      "code %d", rblock->drinst.name, rc);

  else if (rc == -256)
    {
    addr->message = string_sprintf("%s router: command timed out",
      rblock->drinst.name);
    killpg(pid, SIGKILL);       /* Kill the whole process group */
    }

  else if (rc == -257)
    addr->message = string_sprintf("%s router: wait() failed: %s",
      rblock->drinst.name, strerror(errno));

  else
    addr->message = string_sprintf("%s router: command killed by signal %d",
      rblock->drinst.name, -rc);

  return DEFER;
  }

/* Read the pipe to get the command's output, and then close it. */

len = read(fd_out, buffer, sizeof(buffer) - 1);
(void)close(fd_out);

/* Failure to return any data is an error. */

if (len <= 0)
  {
  addr->message = string_sprintf("%s router: command failed to return data",
    rblock->drinst.name);
  return DEFER;
  }

/* Get rid of leading and trailing white space, and pick off the first word of
the result. */

while (len > 0 && isspace(buffer[len-1])) len--;
buffer[len] = 0;

DEBUG(D_route) debug_printf("command wrote: %s\n", buffer);

rword = buffer;
Uskip_whitespace(&rword);
rdata = rword;
Uskip_nonwhite(&rdata);
if (*rdata) *rdata++ = '\0';

/* The word must be a known yield name. If it is "REDIRECT", the rest of the
line is redirection data, as for a .forward file. It may not contain filter
data, and it may not contain anything other than addresses (no files, no pipes,
no specials). */

if (strcmpic(rword, US"REDIRECT") == 0)
  {
  int filtertype;
  redirect_block redirect;
  address_item *generated = NULL;

  redirect.string = rdata;
  redirect.isfile = FALSE;

  rc = rda_interpret(&redirect,  /* redirection data */
    RDO_BLACKHOLE |              /* forbid :blackhole: */
      RDO_FAIL    |              /* forbid :fail: */
      RDO_INCLUDE |              /* forbid :include: */
      RDO_REWRITE,               /* rewrite generated addresses */
    NULL,                        /* :include: directory not relevant */
    NULL,                        /* sieve info not relevant */
    &ugid,                       /* uid/gid (but not set) */
    &generated,                  /* where to hang the results */
    &addr->message,              /* where to put messages */
    NULL,                        /* don't skip syntax errors */
    &filtertype,                 /* not used; will always be FILTER_FORWARD */
    string_sprintf("%s router", rblock->drinst.name));

  switch (rc)
    {
    /* FF_DEFER and FF_FAIL can arise only as a result of explicit commands.
    If a configured message was supplied, allow it to be  included in an SMTP
    response after verifying. */

    case FF_DEFER:
      if (!addr->message) addr->message = US"forced defer";
      else addr->user_message = addr->message;
      return DEFER;

    case FF_FAIL:
      add_generated(rblock, addr_new, addr, generated, &addr_prop);
      if (!addr->message) addr->message = US"forced rejection";
      else addr->user_message = addr->message;
      return FAIL;

    case FF_DELIVERED:
      break;

    case FF_NOTDELIVERED:    /* an empty redirection list is bad */
      addr->message = US"no addresses supplied";
    /* Fall through */

    case FF_ERROR:
    default:
      addr->basic_errno = ERRNO_BADREDIRECT;
      addr->message = string_sprintf("error in redirect data: %s", addr->message);
      return DEFER;
    }

  /* Handle the generated addresses, if any. */

  add_generated(rblock, addr_new, addr, generated, &addr_prop);

  /* Put the original address onto the succeed queue so that any retry items
  that get attached to it get processed. */

  addr->next = *addr_succeed;
  *addr_succeed = addr;

  return OK;
  }

/* Handle other returns that are not ACCEPT */

if (strcmpic(rword, US"accept") != 0)
  {
  if (strcmpic(rword, US"decline") == 0) return DECLINE;
  if (strcmpic(rword, US"pass") == 0) return PASS;
  addr->message = string_copy(rdata);                /* data is a message */
  if (strcmpic(rword, US"fail") == 0)
    {
    setflag(addr, af_pass_message);
    return FAIL;
    }
  if (strcmpic(rword, US"freeze") == 0) addr->special_action = SPECIAL_FREEZE;
  else if (strcmpic(rword, US"defer") != 0)
    {
    addr->message = string_sprintf("bad command yield: %s %s", rword, rdata);
    log_write(0, LOG_PANIC, "%s router: %s", rblock->drinst.name, addr->message);
    }
  return DEFER;
  }

/* The command yielded "ACCEPT". The rest of the string is a number of keyed
fields from which we can fish out values using the equivalent of the "extract"
expansion function. */

if ((s = expand_getkeyed(US"data", rdata)) && *s)
  addr_prop.address_data = string_copy(s);

/* If we found a transport name, find the actual transport */

if ((s = expand_getkeyed(US"transport", rdata)) && *s)
  {
  transport_instance * transport;
  for (transport = transports; transport; transport = transport->drinst.next)
    if (Ustrcmp(transport->drinst.name, s) == 0) break;
  if (!transport)
    {
    addr->message = string_sprintf("unknown transport name %s yielded by "
      "command", s);
    log_write(0, LOG_PANIC, "%s router: %s", rblock->drinst.name, addr->message);
    return DEFER;
    }
  addr->transport = transport;
  }

/* No transport given; get the transport from the router configuration. It may
be fixed or expanded, but there will be an error if it is unset, requested by
the last argument not being NULL. */

else
  {
  if (!rf_get_transport(rblock->transport_name, &rblock->transport, addr,
       rblock->drinst.name, US"transport"))
    return DEFER;
  addr->transport = rblock->transport;
  }

/* See if a host list is given, and if so, look up the addresses. */

if ((s = expand_getkeyed(US"hosts", rdata)) && *s)
  {
  int lookup_type = LK_DEFAULT;
  uschar * ss = expand_getkeyed(US"lookup", rdata);

  if (ss && *ss)
    {
    if (Ustrcmp(ss, "byname") == 0) lookup_type = LK_BYNAME;
    else if (Ustrcmp(ss, "bydns") == 0) lookup_type = LK_BYDNS;
    else
      {
      addr->message = string_sprintf("bad lookup type %q yielded by command", ss);
      log_write(0, LOG_PANIC, "%s router: %s", rblock->drinst.name, addr->message);
      return DEFER;
      }
    }

  host_build_hostlist(&(addr->host_list), s, FALSE);  /* pro tem no randomize */

  rc = rf_lookup_hostlist(rblock, addr, rblock->ignore_target_hosts,
    lookup_type, hff_defer, addr_new);
  if (rc != OK) return rc;
  }
lookup_value = NULL;

/* Put the errors address, extra headers, and address_data into this address */

addr->prop = addr_prop;

/* Queue the address for local or remote delivery. */

return rf_queue_add(addr, addr_local, addr_remote, rblock, pw) ? OK : DEFER;
}



# ifdef DYNLOOKUP
#  define queryprogram_router_info _router_info
# endif

router_info queryprogram_router_info =
{
.drinfo = {
  .driver_name =      US"queryprogram",
  .options =          queryprogram_router_options,
  .options_count =    &queryprogram_router_options_count,
  .options_block =    &queryprogram_router_option_defaults,
  .options_len =      sizeof(queryprogram_router_options_block),
  .init =             queryprogram_router_init,
# ifdef DYNLOOKUP
  .dyn_magic =	      ROUTER_MAGIC,
# endif
  },
.code =               queryprogram_router_entry,
.tidyup =             NULL,     /* no tidyup entry */
.ri_flags =           0
};

#endif	/*!MACRO_PREDEF*/
#endif	/*ROUTER_QUERYPROGRAM*/
/* End of routers/queryprogram.c */
