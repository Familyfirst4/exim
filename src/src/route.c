/*************************************************
*     Exim - an Internet mail transport agent    *
*************************************************/

/* Copyright (c) The Exim Maintainers 2020 - 2024 */
/* Copyright (c) University of Cambridge 1995 - 2018 */
/* See the file NOTICE for conditions of use and distribution. */
/* SPDX-License-Identifier: GPL-2.0-or-later */

/* Functions concerned with routing, and the list of generic router options. */


#include "exim.h"



/* Generic options for routers, all of which live inside router_instance
data blocks and which therefore have the opt_public flag set. */
#define LOFF(field) OPT_OFF(router_instance, field)

optionlist optionlist_routers[] = {
  { "*expand_group",      opt_stringptr | opt_hidden | opt_public,
                 LOFF(expand_gid) },
  { "*expand_more",       opt_stringptr | opt_hidden | opt_public,
                 LOFF(expand_more) },
  { "*expand_unseen",     opt_stringptr | opt_hidden | opt_public,
                 LOFF(expand_unseen) },
  { "*expand_user",       opt_stringptr | opt_hidden | opt_public,
                 LOFF(expand_uid) },
  { "*set_group",         opt_bool | opt_hidden | opt_public,
                 LOFF(gid_set) },
  { "*set_user",          opt_bool | opt_hidden | opt_public,
                 LOFF(uid_set) },
  { "address_data",       opt_stringptr|opt_public,
                 LOFF(address_data) },
  { "address_test",       opt_bool|opt_public,
                 LOFF(address_test) },
#ifdef EXPERIMENTAL_BRIGHTMAIL
  { "bmi_deliver_alternate",   opt_bool | opt_public,
                 LOFF(bmi_deliver_alternate) },
  { "bmi_deliver_default",   opt_bool | opt_public,
                 LOFF(bmi_deliver_default) },
  { "bmi_dont_deliver",   opt_bool | opt_public,
                 LOFF(bmi_dont_deliver) },
  { "bmi_rule",           opt_stringptr|opt_public,
                 LOFF(bmi_rule) },
#endif
  { "cannot_route_message", opt_stringptr | opt_public,
                 LOFF(cannot_route_message) },
  { "caseful_local_part", opt_bool | opt_public,
                 LOFF(caseful_local_part) },
  { "check_local_user",   opt_bool | opt_public,
                 LOFF(check_local_user) },
  { "condition",          opt_stringptr|opt_public|opt_rep_con,
                 LOFF(condition) },
  { "debug_print",        opt_stringptr | opt_public,
                 LOFF(debug_string) },
  { "disable_logging",    opt_bool | opt_public,
                 LOFF(disable_logging) },
  { "dnssec_request_domains",            opt_stringptr|opt_public,
                 LOFF(dnssec.request) },
  { "dnssec_require_domains",            opt_stringptr|opt_public,
                 LOFF(dnssec.require) },
  { "domains",            opt_stringptr|opt_public,
                 LOFF(domains) },
  { "driver",             opt_stringptr|opt_public,
                 LOFF(drinst.driver_name) },
  { "dsn_lasthop",        opt_bool|opt_public,
                 LOFF(dsn_lasthop) },
  { "errors_to",          opt_stringptr|opt_public,
                 LOFF(errors_to) },
  { "expn",               opt_bool|opt_public,
                 LOFF(expn) },
  { "fail_verify",        opt_bool_verify|opt_hidden|opt_public,
                 LOFF(fail_verify_sender) },
  { "fail_verify_recipient", opt_bool|opt_public,
                 LOFF(fail_verify_recipient) },
  { "fail_verify_sender", opt_bool|opt_public,
                 LOFF(fail_verify_sender) },
  { "fallback_hosts",     opt_stringptr|opt_public,
                 LOFF(fallback_hosts) },
  { "group",              opt_expand_gid | opt_public,
                 LOFF(gid) },
  { "headers_add",        opt_stringptr|opt_public|opt_rep_str,
                 LOFF(extra_headers) },
  { "headers_remove",     opt_stringptr|opt_public|opt_rep_str,
                 LOFF(remove_headers) },
  { "ignore_target_hosts",opt_stringptr|opt_public,
                 LOFF(ignore_target_hosts) },
  { "initgroups",         opt_bool | opt_public,
                 LOFF(initgroups) },
  { "local_part_prefix",  opt_stringptr|opt_public,
                 LOFF(prefix) },
  { "local_part_prefix_optional",opt_bool|opt_public,
                 LOFF(prefix_optional) },
  { "local_part_suffix",  opt_stringptr|opt_public,
                 LOFF(suffix) },
  { "local_part_suffix_optional",opt_bool|opt_public,
                 LOFF(suffix_optional) },
  { "local_parts",        opt_stringptr|opt_public,
                 LOFF(local_parts) },
  { "log_as_local",       opt_bool|opt_public,
                 LOFF(log_as_local) },
  { "more",               opt_expand_bool|opt_public,
                 LOFF(more) },
  { "pass_on_timeout",    opt_bool|opt_public,
                 LOFF(pass_on_timeout) },
  { "pass_router",       opt_stringptr|opt_public,
                 LOFF(pass_router_name) },
  { "redirect_router",    opt_stringptr|opt_public,
                 LOFF(redirect_router_name) },
  { "require_files",      opt_stringptr|opt_public,
                 LOFF(require_files) },
  { "retry_use_local_part", opt_bool|opt_public,
                 LOFF(retry_use_local_part) },
  { "router_home_directory", opt_stringptr|opt_public,
                 LOFF(router_home_directory) },
  { "self",               opt_stringptr|opt_public,
                 LOFF(self) },
  { "senders",            opt_stringptr|opt_public,
                 LOFF(senders) },
  { "set",                opt_stringptr|opt_public|opt_rep_str,
                 LOFF(set) },
  #ifdef SUPPORT_TRANSLATE_IP_ADDRESS
  { "translate_ip_address", opt_stringptr|opt_public,
                 LOFF(translate_ip_address) },
  #endif
  { "transport",          opt_stringptr|opt_public,
                 LOFF(transport_name) },
  { "transport_current_directory", opt_stringptr|opt_public,
                 LOFF(current_directory) },
  { "transport_home_directory", opt_stringptr|opt_public,
                 LOFF(home_directory) },
  { "unseen",             opt_expand_bool|opt_public,
                 LOFF(unseen) },
  { "user",               opt_expand_uid | opt_public,
                 LOFF(uid) },
  { "verify",             opt_bool_verify|opt_hidden|opt_public,
                 LOFF(verify_sender) },
  { "verify_only",        opt_bool|opt_public,
                 LOFF(verify_only) },
  { "verify_recipient",   opt_bool|opt_public,
                 LOFF(verify_recipient) },
  { "verify_sender",      opt_bool|opt_public,
                 LOFF(verify_sender) }
};

int optionlist_routers_size = nelem(optionlist_routers);


#ifdef MACRO_PREDEF

# include "macro_predef.h"

void
options_routers(void)
{
uschar buf[64];

options_from_list(optionlist_routers, nelem(optionlist_routers), US"ROUTERS", NULL);

for (driver_info * di = (driver_info *)routers_available; di; di = di->next)
  {
  spf(buf, sizeof(buf), US"_DRIVER_ROUTER_%T", di->driver_name);
  builtin_macro_create(buf);
  options_from_list(di->options, (unsigned)*di->options_count,
		    US"ROUTER", di->driver_name);
  }
}

#else	/*!MACRO_PREDEF*/

/*************************************************
*          Set router pointer from name          *
*************************************************/

/* This function is used for the redirect_router and pass_router options and
called from route_init() below.

Arguments:
  r           the current router
  name        new router name
  ptr         where to put the pointer
  after       TRUE if router must follow this one

Returns:      nothing.
*/

static void
set_router(router_instance *r, uschar *name, router_instance **ptr, BOOL after)
{
BOOL afterthis = FALSE;
router_instance *rr;

for (rr = routers; rr; rr = rr->drinst.next)
  {
  if (Ustrcmp(name, rr->drinst.name) == 0)
    {
    *ptr = rr;
    break;
    }
  if (rr == r) afterthis = TRUE;
  }

if (!rr)
  log_write_die(0, LOG_CONFIG,
    "new_router %q not found for %q router", name, r->drinst.name);

if (after && !afterthis)
  log_write_die(0, LOG_CONFIG,
    "new_router %q does not follow %q router", name, r->drinst.name);
}



/*************************************************
*             Initialize router list             *
*************************************************/

/* Read the routers section of the configuration file, and set up a chain of
router instances according to its contents. Each router has generic options and
may also have its own private options. This function is only ever called when
routers == NULL. We use generic code in readconf to do the work. It will set
values from the configuration file, and then call the driver's initialization
function. */

void
route_init(void)
{
int old_pool = store_pool;
store_pool = POOL_PERM;
  {
  driver_info ** anchor = (driver_info **) &routers_available;

  /* Add the router drivers that are built for static linkage to the
  list of availables. */

#if defined(ROUTER_ACCEPT) && ROUTER_ACCEPT!=2
  extern router_info accept_router_info;
  add_driver_info(anchor, &accept_router_info.drinfo, sizeof(router_info));
#endif
#if defined(ROUTER_DNSLOOKUP) && ROUTER_DNSLOOKUP!=2
  extern router_info dnslookup_router_info;
  add_driver_info(anchor, &dnslookup_router_info.drinfo, sizeof(router_info));
#endif
# if defined(ROUTER_IPLITERAL) && ROUTER_IPLITERAL!=2
  extern router_info ipliteral_router_info;
  add_driver_info(anchor, &ipliteral_router_info.drinfo, sizeof(router_info));
#endif
#if defined(ROUTER_IPLOOKUP) && ROUTER_IPLOOKUP!=2
  extern router_info iplookup_router_info;
  add_driver_info(anchor, &iplookup_router_info.drinfo, sizeof(router_info));
#endif
#if defined(ROUTER_MANUALROUTE) && ROUTER_MANUALROUTE!=2
  extern router_info manualroute_router_info;
  add_driver_info(anchor, &manualroute_router_info.drinfo, sizeof(router_info));
#endif
#if defined(ROUTER_REDIRECT) && ROUTER_REDIRECT!=2
  extern router_info redirect_router_info;
  add_driver_info(anchor, &redirect_router_info.drinfo, sizeof(router_info));
#endif
#if defined(ROUTER_QUERYPROGRAM) && ROUTER_QUERYPROGRAM!=2
  extern router_info queryprogram_router_info;
  add_driver_info(anchor, &queryprogram_router_info.drinfo, sizeof(router_info));
#endif
  }
store_pool = old_pool;


/* Read the config file "routers" section, creating a routers instance list.
For any yet-undiscovered driver, check for a loadable module and add it to
those available. */

readconf_driver_init((driver_instance **)&routers,     /* chain anchor */
  (driver_info **)&routers_available, /* available drivers */
  sizeof(router_info),                /* size of info blocks */
  &router_defaults,                   /* default values for generic options */
  sizeof(router_instance),            /* size of instance block */
  optionlist_routers,                 /* generic options */
  optionlist_routers_size,
  US"router");

for (router_instance * r = routers; r; r = r->drinst.next)
  {
  uschar * s = r->self;
  const router_info * ri = r->drinst.info;

  /* If log_as_local is unset, its overall default is FALSE. (The accept
  router defaults it to TRUE.) */

  if (r->log_as_local == TRUE_UNSET) r->log_as_local = FALSE;

  /* Check for transport or no transport on certain routers */

  if (ri->ri_flags & ri_yestransport && !r->transport_name && !r->verify_only)
    log_write_die(0, LOG_CONFIG, "%s router:\n  "
      "a transport is required for this router", r->drinst.name);

  if (ri->ri_flags & ri_notransport && r->transport_name)
    log_write_die(0, LOG_CONFIG, "%s router:\n  "
      "a transport must not be defined for this router", r->drinst.name);

  /* The "self" option needs to be decoded into a code value and possibly a
  new domain string and a rewrite boolean. */

  if      (Ustrcmp(s, "freeze") == 0)    r->self_code = self_freeze;
  else if (Ustrcmp(s, "defer") == 0)     r->self_code = self_defer;
  else if (Ustrcmp(s, "send") == 0)      r->self_code = self_send;
  else if (Ustrcmp(s, "pass") == 0)      r->self_code = self_pass;
  else if (Ustrcmp(s, "fail") == 0)      r->self_code = self_fail;
  else if (Ustrncmp(s, "reroute:", 8) == 0)
    {
    s += 8;
    Uskip_whitespace(&s);
    if (Ustrncmp(s, "rewrite:", 8) == 0)
      {
      r->self_rewrite = TRUE;
      s += 8;
      Uskip_whitespace(&s);
      }
    r->self = s;
    r->self_code = self_reroute;
    }

  else log_write_die(0, LOG_CONFIG_FOR, "%s router:\n  "
      "%s is not valid for the self option", r->drinst.name, s);

  /* If any router has check_local_user set, default retry_use_local_part
  TRUE; otherwise its default is FALSE. */

  if (r->retry_use_local_part == TRUE_UNSET)
    r->retry_use_local_part =
      r->check_local_user || r->local_parts || r->condition || r->prefix || r->suffix || r->senders || r->require_files;

  /* Build a host list if fallback hosts is set and would not change under
  expansion. */

  if (r->fallback_hosts && Ustrchr(r->fallback_hosts, '$') == NULL)
    {
    int old_pool = store_pool;
    store_pool = POOL_PERM;
    host_build_hostlist(&r->fallback_hostlist, r->fallback_hosts, FALSE);
    store_pool = old_pool;
    }

  /* Check redirect_router and pass_router are valid */

  if (r->redirect_router_name)
    set_router(r, r->redirect_router_name, &(r->redirect_router), FALSE);

  if (r->pass_router_name)
    set_router(r, r->pass_router_name, &(r->pass_router), TRUE);

#ifdef notdef
  DEBUG(D_route) debug_printf_indent("DSN: %s %s\n", r->name,
	r->dsn_lasthop ? "lasthop set" : "propagating DSN");
#endif
  }
}



/*************************************************
*             Tidy up after routing              *
*************************************************/

/* Routers are entitled to keep hold of certain resources in their instance
blocks so as to save setting them up each time. An example is an open file.
Such routers must provide a tidyup entry point which is called when all routing
is finished, via this function. */

void
route_tidyup(void)
{
for (router_instance * r = routers; r; r = r->drinst.next)
  {
  router_info * ri = r->drinst.info;
  if (ri->tidyup) (ri->tidyup)(r);
  }
}



/*************************************************
*         Check local part for prefix            *
*************************************************/

/* This function is handed a local part and a list of possible prefixes; if any
one matches, return the prefix length. A prefix beginning with '*' is a
wildcard.

Arguments:
  local_part    the local part to check
  prefixes      the list of prefixes
  vp		if set, pointer to place for size of wildcard portion

Returns:        length of matching prefix or zero
*/

int
route_check_prefix(const uschar * local_part, const uschar * prefixes,
  unsigned * vp)
{
int sep = 0;
uschar *prefix;
const uschar *listptr = prefixes;

while ((prefix = string_nextinlist(&listptr, &sep, NULL, 0)))
  {
  int plen = Ustrlen(prefix);
  if (prefix[0] == '*')
    {
    prefix++;
    for (const uschar * p = local_part + Ustrlen(local_part) - (--plen);
         p >= local_part; p--)
      if (strncmpic(prefix, p, plen) == 0)
	{
	unsigned vlen = p - local_part;
	if (vp) *vp = vlen;
	return plen + vlen;
	}
    }
  else
    if (strncmpic(prefix, local_part, plen) == 0)
      {
      if (vp) *vp = 0;
      return plen;
      }
  }

return 0;
}



/*************************************************
*         Check local part for suffix            *
*************************************************/

/* This function is handed a local part and a list of possible suffixes;
if any one matches, return the suffix length. A suffix ending with '*'
is a wildcard.

Arguments:
  local_part    the local part to check
  suffixes      the list of suffixes
  vp		if set, pointer to place for size of wildcard portion

Returns:        length of matching suffix or zero
*/

int
route_check_suffix(const uschar * local_part, const uschar * suffixes,
  unsigned * vp)
{
int sep = 0, alen = Ustrlen(local_part);
const uschar * listptr = suffixes, * suffix;

while ((suffix = string_nextinlist(&listptr, &sep, NULL, 0)))
  {
  int slen = Ustrlen(suffix);
  if (suffix[slen-1] == '*')
    {
    const uschar * pend = local_part + alen - (--slen) + 1;
    for (const uschar * p = local_part; p < pend; p++)
      if (strncmpic(suffix, p, slen) == 0)
	{
	int tlen = alen - (p - local_part);
	if (vp) *vp = tlen - slen;
	return tlen;
	}
    }
  else
    if (alen > slen && strncmpic(suffix, local_part + alen - slen, slen) == 0)
      {
      if (vp) *vp = 0;
      return slen;
      }
  }

return 0;
}




/*************************************************
*     Check local part, domain, or sender        *
*************************************************/

/* The checks in check_router_conditions() require similar code, so we use
this function to save repetition.

Arguments:
  rname          router name for error messages
  type           type of check, for error message
  list           domains, local_parts, or senders list
  anchorptr      -> tree for possibly cached items (domains)
  cache_bits     cached bits pointer
  listtype       MCL_DOMAIN for domain check
                 MCL_LOCALPART for local part check
                 MCL_ADDRESS for sender check
  domloc         current domain, current local part, or NULL for sender check
  ldata          where to put lookup data
  caseless       passed on to match_isinlist()
  perror         where to put an error message

Returns:         OK     item is in list
                 SKIP   item is not in list, router is to be skipped
                 DEFER  lookup or other defer
*/

static int
route_check_dls(const uschar * rname, const uschar * type, const uschar * list,
  tree_node ** anchorptr, unsigned int * cache_bits, int listtype,
  const uschar * domloc, const uschar ** ldata, BOOL caseless, uschar ** perror)
{
if (!list) return OK;   /* Empty list always succeeds */

DEBUG(D_route) debug_printf_indent("checking %s\n", type);

/* The domain and local part use the same matching function, whereas sender
has its own code. */

switch(domloc
  ? match_isinlist(domloc, &list, 0, anchorptr, cache_bits, listtype,
    caseless, ldata)
  : match_address_list(sender_address ? sender_address : US"",
    TRUE, TRUE, &list, cache_bits, -1, 0, CUSS &sender_data)
      )
  {
  case OK:
    return OK;

  case FAIL:
    *perror = string_sprintf("%s router skipped: %s mismatch", rname, type);
    DEBUG(D_route) debug_printf_indent("%s\n", *perror);
    return SKIP;

  default:      /* Paranoia, and keeps compilers happy */
  case DEFER:
    *perror = string_sprintf("%s check lookup or other defer", type);
    DEBUG(D_route) debug_printf_indent("%s\n", *perror);
    return DEFER;
  }
}



/*************************************************
*        Check access by a given uid/gid         *
*************************************************/

/* This function checks whether a given uid/gid has access to a given file or
directory. It is called only from check_files() below. This is hopefully a
cheapish check that does the job most of the time. Exim does *not* rely on this
test when actually accessing any file. The test is used when routing to make it
possible to take actions such as "if user x can access file y then run this
router".

During routing, Exim is normally running as root, and so the test will work
except for NFS non-root mounts. When verifying during message reception, Exim
is running as "exim", so the test may not work. This is a limitation of the
Exim design.

Code in check_files() below detects the case when it cannot stat() the file (as
root), and in that situation it uses a setuid subprocess in which to run this
test.

Arguments:
  path          the path to check
  uid           the user
  gid           the group
  bits          the bits required in the final component

Returns:        TRUE
                FALSE errno=EACCES or ENOENT (or others from realpath or stat)
*/

static BOOL
route_check_access(const uschar * path, uid_t uid, gid_t gid, int bits)
{
struct stat statbuf;
uschar * rp = US realpath(CCS path, CS big_buffer);

DEBUG(D_route) debug_printf_indent("route_check_access(%s,%d,%d,%o)\n", path,
  (int)uid, (int)gid, bits);

if (!rp) return FALSE;

for (uschar * sp = rp + 1, * slash; slash = Ustrchr(sp, '/'); sp = slash + 1)
  {
  *slash = '\0';
  DEBUG(D_route) debug_printf_indent("stat %s\n", rp);
  if (Ustat(rp, &statbuf) < 0) return FALSE;
  if ((statbuf.st_mode &
       (statbuf.st_uid == uid ? 0100 : statbuf.st_gid == gid ? 0010 : 001)
      ) == 0)
    {
    errno = EACCES;
    return FALSE;
    }
  *slash = '/';
  }

/* Down to the final component */

DEBUG(D_route) debug_printf_indent("stat %s\n", rp);

if (Ustat(rp, &statbuf) < 0) return FALSE;

if (statbuf.st_uid == uid) bits = bits << 6;
else if (statbuf.st_gid == gid) bits = bits << 3;

if ((statbuf.st_mode & bits) != bits)
  {
  errno = EACCES;
  return FALSE;
  }

DEBUG(D_route) debug_printf_indent("route_check_access() succeeded\n");
return TRUE;
}



/*************************************************
*           Do file existence tests              *
*************************************************/

/* This function is given a colon-separated list of file tests, each of which
is expanded before use. A test consists of a file name, optionally preceded by
! (require non-existence) and/or + for handling permission denied (+ means
treat as non-existing).

An item that contains no slashes is interpreted as a username or id, with an
optional group id, for checking access to the file. This cannot be done
"perfectly", but it is good enough for a number of applications.

Arguments:
  s        a colon-separated list of file tests or NULL
  perror   a pointer to an anchor for an error text in the case of a DEFER

Returns:   OK if s == NULL or all tests are as required
           DEFER if the existence of at least one of the files is
             unclear (an error other than non-existence occurred);
           DEFER if an expansion failed
           DEFER if a name is not absolute
           DEFER if problems with user/group
           SKIP otherwise
*/

static int
check_files(const uschar *s, uschar **perror)
{
int sep = 0;              /* List has default separators */
uid_t uid = 0;            /* For picky compilers */
gid_t gid = 0;            /* For picky compilers */
BOOL ugid_set = FALSE;
const uschar *listptr;
uschar *check;

if (!s) return OK;

DEBUG(D_route|D_expand) debug_printf_indent("checking require_files\n");

listptr = s;
while ((check = string_nextinlist(&listptr, &sep, NULL, 0)))
  {
  int rc;
  int eacces_code = 0;
  BOOL invert = FALSE;
  struct stat statbuf;
  uschar *ss = expand_string(check);

  if (!ss)
    {
    if (f.expand_string_forcedfail) continue;
    *perror = string_sprintf("failed to expand %q for require_files: %s",
      check, expand_string_message);
    goto RETURN_DEFER;
    }

  /* Empty items are just skipped */

  if (!*ss) continue;

  /* If there are no slashes in the string, we have a user name or uid, with
  optional group/gid. */

  if (Ustrchr(ss, '/') == NULL)
    {
    BOOL ok;
    struct passwd *pw;
    uschar *comma = Ustrchr(ss, ',');

    /* If there's a comma, temporarily terminate the user name/number
    at that point. Then set the uid. */

    if (comma) *comma = 0;
    ok = route_finduser(ss, &pw, &uid);
    if (comma) *comma = ',';

    if (!ok)
      {
      *perror = string_sprintf("user %q for require_files not found", ss);
      goto RETURN_DEFER;
      }

    /* If there was no comma, the gid is that associated with the user. */

    if (!comma)
      if (pw)
	gid = pw->pw_gid;
      else
        {
        *perror = string_sprintf("group missing after numerical uid %d for "
          "require_files", (int)uid);
        goto RETURN_DEFER;
        }
    else
      if (!route_findgroup(comma + 1, &gid))
        {
        *perror = string_sprintf("group %q for require_files not found\n",
          comma + 1);
        goto RETURN_DEFER;
        }

    /* Note that we have values set, and proceed to next item */

    DEBUG(D_route)
      debug_printf_indent("check subsequent files for access by %s\n", ss);
    ugid_set = TRUE;
    continue;
    }

  /* Path, possibly preceded by + and ! */

  if (*ss == '+')
    {
    eacces_code = 1;
    while (isspace(*++ss));
    }

  if (*ss == '!')
    {
    invert = TRUE;
    while (isspace(*++ss));
    }

  if (*ss != '/')
    {
    *perror = string_sprintf("require_files: %q is not absolute", ss);
    goto RETURN_DEFER;
    }

  /* Stat the file, either as root (while routing) or as exim (while verifying
  during message reception). */

  rc = Ustat(ss, &statbuf);

  DEBUG(D_route)
    {
    debug_printf_indent("file check: %s\n", check);
    if (ss != check) debug_printf_indent("expanded file: %s\n", ss);
    debug_printf_indent("stat() yielded %d\n", rc);
    }

  /* If permission is denied, and we are running as root (i.e. routing for
  delivery rather than verifying), and the requirement is to test for access by
  a particular uid/gid, it must mean that the file is on a non-root-mounted NFS
  system. In this case, we have to use a subprocess that runs as the relevant
  uid in order to do the test. */

  if (rc != 0 && errno == EACCES && ugid_set && getuid() == root_uid)
    {
    int status;
    pid_t pid;
    void (*oldsignal)(int);

    DEBUG(D_route) debug_printf_indent("root is denied access: forking to check "
      "in subprocess\n");

    /* Before forking, ensure that SIGCHLD is set to SIG_DFL before forking, so
    that the child process can be waited for, just in case get here with it set
    otherwise. Save the old state for resetting on the wait. */

    oldsignal = signal(SIGCHLD, SIG_DFL);
    pid = exim_fork(US"require-files");

    /* If fork() fails, reinstate the original error and behave as if
    this block of code were not present. This is the same behaviour as happens
    when Exim is not running as root at this point. */

    if (pid < 0)
      {
      DEBUG(D_route)
       debug_printf_indent("require_files: fork failed: %s\n", strerror(errno));
      errno = EACCES;
      goto HANDLE_ERROR;
      }

    /* In the child process, change uid and gid, and then do the check using
    the route_check_access() function. This does more than just stat the file;
    it tests permissions as well. Return 0 for OK and 1 for failure. */

    if (pid == 0)
      {
      exim_setugid(uid, gid, TRUE,
        string_sprintf("require_files check, file=%s", ss));
      if (route_check_access(ss, uid, gid, 4))
	exim_underbar_exit(EXIT_SUCCESS);
      DEBUG(D_route) debug_printf_indent("route_check_access() failed\n");
      exim_underbar_exit(EXIT_FAILURE);
      }

    /* In the parent, wait for the child to finish */

    while (waitpid(pid, &status, 0) < 0)
     if (errno != EINTR)  /* unexpected error, interpret as failure */
       {
       status = 1;
       break;
       }

    signal(SIGCHLD, oldsignal);   /* restore */
    if ((status == 0) == invert) return SKIP;
    continue;   /* to test the next file */
    }

  /* Control reaches here if the initial stat() succeeds, or fails with an
  error other than EACCES, or no uid/gid is set, or we are not running as root.
  If we know the file exists and uid/gid are set, try to check read access for
  that uid/gid as best we can. */

  if (rc == 0 && ugid_set && !route_check_access(ss, uid, gid, 4))
    {
    DEBUG(D_route) debug_printf_indent("route_check_access() failed\n");
    rc = -1;
    }

  /* Handle error returns from stat() or route_check_access(). The EACCES error
  is handled specially. At present, we can force it to be treated as
  non-existence. Write the code so that it will be easy to add forcing for
  existence if required later. */

  HANDLE_ERROR:
  if (rc < 0)
    {
    DEBUG(D_route) debug_printf_indent("errno = %d\n", errno);
    if (errno == EACCES)
      {
      if (eacces_code == 1)
        {
        DEBUG(D_route) debug_printf_indent("EACCES => ENOENT\n");
        errno = ENOENT;   /* Treat as non-existent */
        }
      }
    if (errno != ENOENT)
      {
      *perror = string_sprintf("require_files: error for %s: %s", ss,
        strerror(errno));
      goto RETURN_DEFER;
      }
    }

  /* At this point, rc < 0 => non-existence; rc >= 0 => existence */

  if ((rc >= 0) == invert) return SKIP;
  }

return OK;

/* Come here on any of the errors that return DEFER. */

RETURN_DEFER:
DEBUG(D_route) debug_printf_indent("%s\n", *perror);
return DEFER;
}





/*************************************************
*             Check for router skipping          *
*************************************************/

/* This function performs various checks to see whether a router should be
skipped. The order in which they are performed is important.

Arguments:
  r            pointer to router instance block
  addr         address that is being handled
  verify       the verification type
  pw           ptr to ptr to passwd structure for local user
  perror       for lookup errors

Returns:       OK if all the tests succeed
               SKIP if router is to be skipped
               DEFER for a lookup defer
               FAIL for address to be failed
*/

static BOOL
check_router_conditions(router_instance * r, address_item * addr, int verify,
  struct passwd ** pw, uschar ** perror)
{
int rc;
uschar * check_local_part;
unsigned int * localpart_cache;
const uschar * rname = r->drinst.name;

/* Reset variables to hold a home directory and data from lookup of a domain or
local part, and ensure search_find_defer is unset, in case there aren't any
actual lookups. */

deliver_home = NULL;
deliver_domain_data = NULL;
deliver_localpart_data = NULL;
sender_data = NULL;
local_user_gid = (gid_t)(-1);
local_user_uid = (uid_t)(-1);
f.search_find_defer = FALSE;

/* Skip this router if not verifying and it has verify_only set */

if ((verify == v_none || verify == v_expn) && r->verify_only)
  {
  DEBUG(D_route) debug_printf_indent("%s router skipped: verify_only set\n", rname);
  return SKIP;
  }

/* Skip this router if testing an address (-bt) and address_test is not set */

if (f.address_test_mode && !r->address_test)
  {
  DEBUG(D_route) debug_printf_indent("%s router skipped: address_test is unset\n",
    rname);
  return SKIP;
  }

/* Skip this router if verifying and it hasn't got the appropriate verify flag
set. */

if ((verify == v_sender && !r->verify_sender) ||
    (verify == v_recipient && !r->verify_recipient))
  {
  DEBUG(D_route) debug_printf_indent("%s router skipped: verify %d %d %d\n",
    rname, verify, r->verify_sender, r->verify_recipient);
  return SKIP;
  }

/* Skip this router if processing EXPN and it doesn't have expn set */

if (verify == v_expn && !r->expn)
  {
  DEBUG(D_route) debug_printf_indent("%s router skipped: no_expn set\n", rname);
  return SKIP;
  }

/* Skip this router if there's a domain mismatch. */

if ((rc = route_check_dls(rname, US"domains", r->domains, &domainlist_anchor,
     addr->domain_cache, TRUE, addr->domain, CUSS &deliver_domain_data,
     MCL_DOMAIN, perror)) != OK)
  return rc;

/* Skip this router if there's a local part mismatch. We want to pass over the
caseful local part, so that +caseful can restore it, even if this router is
handling local parts caselessly. However, we can't just pass cc_local_part,
because that doesn't have the prefix or suffix stripped. A bit of massaging is
required. Also, we only use the match cache for local parts that have not had
a prefix or suffix stripped. */

check_local_part = string_copy(addr->cc_local_part);
if (!addr->prefix && !addr->suffix)
  localpart_cache = addr->localpart_cache;
else
  {
  localpart_cache = NULL;
  if (addr->prefix)
    check_local_part += Ustrlen(addr->prefix);
  if (addr->suffix)
    check_local_part[Ustrlen(check_local_part) - Ustrlen(addr->suffix)] = 0;
  }

if ((rc = route_check_dls(rname, US"local_parts", r->local_parts,
       &localpartlist_anchor, localpart_cache, MCL_LOCALPART,
       check_local_part, CUSS &deliver_localpart_data,
       !r->caseful_local_part, perror)) != OK)
  return rc;

/* If the check_local_user option is set, check that the local_part is the
login of a local user. Note: the third argument to route_finduser() must be
NULL here, to prevent a numeric string being taken as a numeric uid. If the
user is found, set deliver_home to the home directory, and also set
local_user_{uid,gid} and local_part_data.  */

if (r->check_local_user)
  {
  DEBUG(D_route) debug_printf_indent("checking for local user\n");
  if (!route_finduser(addr->local_part, pw, NULL))
    {
    DEBUG(D_route) debug_printf_indent("%s router skipped: %s is not a local user\n",
      rname, addr->local_part);
    return SKIP;
    }
  addr->prop.localpart_data =
    deliver_localpart_data = string_copy(US (*pw)->pw_name);
  deliver_home = string_copy(US (*pw)->pw_dir);
  local_user_gid = (*pw)->pw_gid;
  local_user_uid = (*pw)->pw_uid;
  }

/* Set (or override in the case of check_local_user) the home directory if
router_home_directory is set. This is done here so that it overrides $home from
check_local_user before any subsequent expansions are done. Otherwise, $home
could mean different things for different options, which would be extremely
confusing. */

GET_OPTION("router_home_directory");
if (r->router_home_directory)
  {
  uschar * router_home = expand_string(r->router_home_directory);
  if (router_home)
    {
    setflag(addr, af_home_expanded); /* Note set from router_home_directory */
    deliver_home = router_home;
    }
  else if (!f.expand_string_forcedfail)
    {
    *perror = string_sprintf("failed to expand %q for "
      "router_home_directory: %s", r->router_home_directory,
      expand_string_message);
    return DEFER;
    }
  }

/* Skip if the sender condition is not met. We leave this one till after the
local user check so that $home is set - enabling the possibility of letting
individual recipients specify lists of acceptable/unacceptable senders. */

if ((rc = route_check_dls(rname, US"senders", r->senders, NULL,
     sender_address_cache, MCL_ADDRESS, NULL, NULL, FALSE, perror)) != OK)
  return rc;

/* This is the point at which we print out the router's debugging string if it
is set. We wait till here so as to have $home available for local users (and
anyway, we don't want too much stuff for skipped routers). */

debug_print_string(r->debug_string);

/* Perform file existence tests. */

if ((rc = check_files(r->require_files, perror)) != OK)
  {
  DEBUG(D_route) debug_printf_indent("%s router %s: file check\n", rname,
    (rc == SKIP)? "skipped" : "deferred");
  return rc;
  }

/* Now the general condition test. */

if (r->condition)
  {
  DEBUG(D_route|D_expand)
    debug_printf_indent("checking \"condition\" \"%.80s\"...\n", r->condition);
  if (!expand_check_condition(r->condition, rname, US"router"))
    {
    if (f.search_find_defer)
      {
      *perror = US"condition check lookup defer";
      DEBUG(D_route) debug_printf_indent("%s\n", *perror);
      return DEFER;
      }
    DEBUG(D_route)
      debug_printf_indent("%s router skipped: condition failure\n", rname);
    return SKIP;
    }
  }

#ifdef EXPERIMENTAL_BRIGHTMAIL
/* check if a specific Brightmail AntiSpam rule fired on the message */
if (r->bmi_rule)
  {
  DEBUG(D_route) debug_printf_indent("checking bmi_rule\n");
  if (bmi_check_rule(bmi_base64_verdict, r->bmi_rule) == 0)
    {    /* none of the rules fired */
    DEBUG(D_route)
      debug_printf_indent("%s router skipped: none of bmi_rule rules fired\n", rname);
    return SKIP;
    }
  }

/* check if message should not be delivered */
if (r->bmi_dont_deliver && bmi_deliver == 1)
  {
  DEBUG(D_route)
    debug_printf_indent("%s router skipped: bmi_dont_deliver is FALSE\n", rname);
  return SKIP;
  }

/* check if message should go to an alternate location */
if (  r->bmi_deliver_alternate
   && (bmi_deliver == 0 || !bmi_alt_location)
   )
  {
  DEBUG(D_route)
    debug_printf_indent("%s router skipped: bmi_deliver_alternate is FALSE\n", rname);
  return SKIP;
  }

/* check if message should go to default location */
if (  r->bmi_deliver_default
   && (bmi_deliver == 0 || bmi_alt_location)
   )
  {
  DEBUG(D_route)
    debug_printf_indent("%s router skipped: bmi_deliver_default is FALSE\n", rname);
  return SKIP;
  }
#endif

/* All the checks passed. */

return OK;
}




/*************************************************
*           Find a local user                    *
*************************************************/

/* Try several times (if configured) to find a local user, in case delays in
NIS or NFS whatever cause an incorrect refusal. It's a pity that getpwnam()
doesn't have some kind of indication as to why it has failed. If the string
given consists entirely of digits, and the third argument is not NULL, assume
the string is the numerical value of the uid. Otherwise it is looked up using
getpwnam(). The uid is passed back via return_uid, if not NULL, and the
pointer to a passwd structure, if found, is passed back via pw, if not NULL.

Because this may be called several times in succession for the same user for
different routers, cache the result of the previous getpwnam call so that it
can be re-used. Note that we can't just copy the structure, as the store it
points to can get trashed.

Arguments:
  s           the login name or textual form of the numerical uid of the user
  pw          if not NULL, return the result of getpwnam here, or set NULL
                if no call to getpwnam is made (s numeric, return_uid != NULL)
  return_uid  if not NULL, return the uid via this address

Returns:      TRUE if s is numerical or was looked up successfully

*/

static struct passwd pwcache;
static struct passwd *lastpw = NULL;
static uschar lastname[48] = { 0 };
static uschar lastdir[128];
static uschar lastgecos[128];
static uschar lastshell[128];

BOOL
route_finduser(const uschar *s, struct passwd **pw, uid_t *return_uid)
{
BOOL cache_set = (Ustrcmp(lastname, s) == 0);

DEBUG(D_uid) debug_printf_indent("seeking password data for user %q: %s\n", s,
  cache_set ? "using cached result" : "cache not available");

if (!cache_set)
  {
  if (return_uid && (isdigit(*s) || *s == '-') &&
       s[Ustrspn(s+1, "0123456789")+1] == 0)
    {
    *return_uid = (uid_t)Uatoi(s);
    if (pw) *pw = NULL;
    return TRUE;
    }

  string_format_nt(lastname, sizeof(lastname), "%s", s);

  /* Force failure if string length is greater than given maximum */

  if (max_username_length > 0 && Ustrlen(lastname) > max_username_length)
    {
    DEBUG(D_uid) debug_printf_indent("forced failure of finduser(): string "
      "length of %s is greater than %d\n", lastname, max_username_length);
    lastpw = NULL;
    }

  /* Try a few times if so configured; this handles delays in NIS etc. */

  else for (int i = 0;;)
    {
    errno = 0;
    if ((lastpw = getpwnam(CS s))) break;
    if (++i > finduser_retries) break;
    sleep(1);
    }

  if (lastpw)
    {
    pwcache.pw_uid = lastpw->pw_uid;
    pwcache.pw_gid = lastpw->pw_gid;
    (void)string_format(lastdir, sizeof(lastdir), "%s", lastpw->pw_dir);
    (void)string_format(lastgecos, sizeof(lastgecos), "%s", lastpw->pw_gecos);
    (void)string_format(lastshell, sizeof(lastshell), "%s", lastpw->pw_shell);
    pwcache.pw_name = CS lastname;
    pwcache.pw_dir = CS lastdir;
    pwcache.pw_gecos = CS lastgecos;
    pwcache.pw_shell = CS lastshell;
    lastpw = &pwcache;
    }

  else DEBUG(D_uid) if (errno != 0)
    debug_printf_indent("getpwnam(%s) failed: %s\n", s, strerror(errno));
  }

if (!lastpw)
  {
  DEBUG(D_uid) debug_printf_indent("getpwnam() returned NULL (user not found)\n");
  return FALSE;
  }

DEBUG(D_uid) debug_printf_indent("getpwnam() succeeded uid=%d gid=%d\n",
    lastpw->pw_uid, lastpw->pw_gid);

if (return_uid) *return_uid = lastpw->pw_uid;
if (pw) *pw = lastpw;

return TRUE;
}




/*************************************************
*           Find a local group                   *
*************************************************/

/* Try several times (if configured) to find a local group, in case delays in
NIS or NFS whatever cause an incorrect refusal. It's a pity that getgrnam()
doesn't have some kind of indication as to why it has failed.

Arguments:
  s           the group name or textual form of the numerical gid
  return_gid  return the gid via this address

Returns:      TRUE if the group was found; FALSE otherwise

*/

BOOL
route_findgroup(uschar *s, gid_t *return_gid)
{
const struct group * gr;

if ((isdigit(*s) || *s == '-') && s[Ustrspn(s+1, "0123456789")+1] == 0)
  {
  *return_gid = (gid_t)Uatoi(s);
  return TRUE;
  }

for (int i = 0;;)
  {
  if ((gr = getgrnam(CS s)))
    {
    *return_gid = gr->gr_gid;
    return TRUE;
    }
  if (++i > finduser_retries) break;
  sleep(1);
  }

return FALSE;
}




/*************************************************
*          Find user by expanding string         *
*************************************************/

/* Expands a string, and then looks up the result in the passwd file.

Arguments:
  string       the string to be expanded, yielding a login name or a numerical
                 uid value (to be passed to route_finduser())
  driver_name  caller name for panic error message (only)
  driver_type  caller type for panic error message (only)
  pw           return passwd entry via this pointer
  uid          return uid via this pointer
  errmsg       where to point a message on failure

Returns:       TRUE if user found, FALSE otherwise
*/

BOOL
route_find_expanded_user(uschar *string, uschar *driver_name,
  uschar *driver_type, struct passwd **pw, uid_t *uid, uschar **errmsg)
{
uschar *user = expand_string(string);

if (!user)
  {
  *errmsg = string_sprintf("Failed to expand user string %q for the "
    "%s %s: %s", string, driver_name, driver_type, expand_string_message);
  log_write(0, LOG_MAIN|LOG_PANIC, "%s", *errmsg);
  return FALSE;
  }

if (route_finduser(user, pw, uid)) return TRUE;

*errmsg = string_sprintf("Failed to find user %q from expanded string "
  "%q for the %s %s", user, string, driver_name, driver_type);
log_write(0, LOG_MAIN|LOG_PANIC, "%s", *errmsg);
return FALSE;
}



/*************************************************
*          Find group by expanding string        *
*************************************************/

/* Expands a string and then looks up the result in the group file.

Arguments:
  string       the string to be expanded, yielding a group name or a numerical
                 gid value (to be passed to route_findgroup())
  driver_name  caller name for panic error message (only)
  driver_type  caller type for panic error message (only)
  gid          return gid via this pointer
  errmsg       return error message via this pointer

Returns:       TRUE if found group, FALSE otherwise
*/

BOOL
route_find_expanded_group(uschar *string, uschar *driver_name, uschar *driver_type,
  gid_t *gid, uschar **errmsg)
{
BOOL yield = TRUE;
uschar *group = expand_string(string);

if (!group)
  {
  *errmsg = string_sprintf("Failed to expand group string %q for the "
    "%s %s: %s", string, driver_name, driver_type, expand_string_message);
  log_write(0, LOG_MAIN|LOG_PANIC, "%s", *errmsg);
  return FALSE;
  }

if (!route_findgroup(group, gid))
  {
  *errmsg = string_sprintf("Failed to find group %q from expanded string "
    "%q for the %s %s", group, string, driver_name, driver_type);
  log_write(0, LOG_MAIN|LOG_PANIC, "%s", *errmsg);
  yield = FALSE;
  }

return yield;
}



/*************************************************
*            Handle an unseen routing            *
*************************************************/

/* This function is called when an address is routed by a router with "unseen"
set. It must make a clone of the address, for handling by subsequent drivers.
The clone is set to start routing at the next router.

The original address must be replaced by an invented "parent" which has the
routed address plus the clone as its children. This is necessary in case the
address is at the top level - we don't want to mark it complete until both
deliveries have been done.

A new unique field must be made, so that the record of the delivery isn't a
record of the original address, and checking for already delivered has
therefore to be done here. If the delivery has happened, then take the base
address off whichever delivery queue it is on - it will always be the top item.

Arguments:
  name          router name
  addr          address that was routed
  paddr_local   chain of local-delivery addresses
  paddr_remote  chain of remote-delivery addresses
  addr_new      chain for newly created addresses

Returns:        nothing
*/

static void
route_unseen(const uschar * name, address_item * addr,
  address_item **paddr_local, address_item ** paddr_remote,
  address_item ** addr_new)
{
address_item *parent = deliver_make_addr(addr->address, TRUE);
address_item *new = deliver_make_addr(addr->address, TRUE);

/* The invented parent is a copy that replaces the original; note that
this copies its parent pointer. It has two children, and its errors_address is
from the original address' parent, if present, otherwise unset. */

*parent = *addr;
parent->child_count = 2;
parent->prop.errors_address =
  addr->parent ? addr->parent->prop.errors_address : NULL;

/* The routed address gets a new parent. */

addr->parent = parent;

/* The clone has this parent too. Set its errors address from the parent. This
was set from the original parent (or to NULL) - see above. We do NOT want to
take the errors address from the unseen router. */

new->parent = parent;
new->prop.errors_address = parent->prop.errors_address;

/* Copy the propagated flags and address_data from the original. */

new->prop.ignore_error = addr->prop.ignore_error;
new->prop.address_data = addr->prop.address_data;
new->prop.variables = NULL;
tree_dup((tree_node **)&new->prop.variables, addr->prop.variables);
new->dsn_flags = addr->dsn_flags;
new->dsn_orcpt = addr->dsn_orcpt;


/* As it has turned out, we haven't set headers_add or headers_remove for the
clone. Thinking about it, it isn't entirely clear whether they should be
copied from the original parent, like errors_address, or taken from the
unseen router, like address_data and the flags. Until somebody brings this
up, I propose to leave the code as it is.  */


/* Set the cloned address to start at the next router, and put it onto the
chain of new addresses. */

new->start_router = addr->router->drinst.next;
new->next = *addr_new;
*addr_new = new;

DEBUG(D_route) debug_printf_indent("\"unseen\" set: replicated %s\n", addr->address);

/* Make a new unique field, to distinguish from the normal one. */

addr->unique = string_sprintf("%s/%s", addr->unique, name);

/* If the address has been routed to a transport, see if it was previously
delivered. If so, we take it off the relevant queue so that it isn't delivered
again. Otherwise, it was an alias or something, and the addresses it generated
are handled in the normal way. */

if (addr->transport && tree_search(tree_nonrecipients, addr->unique))
  {
  DEBUG(D_route)
    debug_printf_indent("\"unseen\" delivery previously done - discarded\n");
  parent->child_count--;
  if (*paddr_remote == addr) *paddr_remote = addr->next;
  if (*paddr_local == addr) *paddr_local = addr->next;
  }
}



/************************************************/
/* Add router-assigned variables
Return OK/DEFER/FAIL/PASS */

static int
set_router_vars(address_item * addr, const router_instance * r)
{
const uschar * varlist = r->set;
tree_node ** root = (tree_node **) &addr->prop.variables;
int sep = ';';
const uschar * drname = r->drinst.name;

GET_OPTION("set");
if (!varlist) return OK;

/* Walk the varlist, creating variables */

for (uschar * ele; (ele = string_nextinlist(&varlist, &sep, NULL, 0)); )
  {
  const uschar * assignment = ele;
  int esep = '=';
  uschar * name = string_nextinlist(&assignment, &esep, NULL, 0);
  uschar * val;
  tree_node * node;

  /* Variable name must exist and start "r_". */

  if (!name || name[0] != 'r' || name[1] != '_' || !name[2])
    {
    log_write(0, LOG_MAIN|LOG_PANIC,
	"bad router variable name '%s' in router '%s'\n", name, drname);
    return FAIL;
    }
  name += 2;

  Uskip_whitespace(&assignment);

  if (!(val = expand_string(US assignment)))
    if (f.expand_string_forcedfail)
      {
      int yield;
      BOOL more;
      DEBUG(D_route) debug_printf_indent("forced failure in expansion of %q "
	  "(router variable): decline action taken\n", ele);

      /* Expand "more" if necessary; DEFER => an expansion failed */

      if ((yield = exp_bool(addr, US"router", drname, D_route,
		US"more", r->more, r->expand_more, &more)) != OK) return yield;

      if (more) return PASS;

      DEBUG(D_route)
	debug_printf_indent("\"more\"=false: skipping remaining routers\n");
      router_name = NULL;
      r = NULL;
      return FAIL;
      }
    else
      {
      addr->message = string_sprintf("expansion of %q failed "
	"in %s router: %s", ele, drname, expand_string_message);
      /* Caller will replace that for logging, if a DB lookup, to avoid exposing
      passwords */
      DEBUG(D_route) debug_printf_indent("%s\n", addr->message);
      return f.search_find_defer ? DEFER : FAIL;
      }

  if (!(node = tree_search(*root, name)))
    {				/* name should never be tainted */
    node = store_get(sizeof(tree_node) + Ustrlen(name), GET_UNTAINTED);
    Ustrcpy(node->name, name);
    (void)tree_insertnode(root, node);
    }
  node->data.ptr = US val;
  DEBUG(D_route) debug_printf_indent("set r_%s%s = '%s'%s\n",
		    name, is_tainted(name)?" (tainted)":"",
		    val, is_tainted(val)?" (tainted)":"");

  /* All expansions after this point need visibility of that variable */
  router_var = *root;
  }
return OK;
}


/*************************************************
*                 Route one address              *
*************************************************/

/* This function is passed in one address item, for processing by the routers.
The verify flag is set if this is being called for verification rather than
delivery. If the router doesn't have its "verify" flag set, it is skipped.

Arguments:
  addr           address to route
  paddr_local    chain of local-delivery addresses
  paddr_remote   chain of remote-delivery addresses
  addr_new       chain for newly created addresses
  addr_succeed   chain for completed addresses
  verify         v_none if not verifying
                 v_sender if verifying a sender address
                 v_recipient if verifying a recipient address
                 v_expn if processing an EXPN address

Returns:         OK      => address successfully routed
                 DISCARD => address was discarded
                 FAIL    => address could not be routed
                 DEFER   => some temporary problem
                 ERROR   => some major internal or configuration failure
*/

int
route_address(address_item *addr, address_item **paddr_local,
  address_item **paddr_remote, address_item **addr_new,
  address_item **addr_succeed, int verify)
{
int yield = OK;
BOOL unseen;
router_instance * r, * nextr;
const uschar * old_domain = addr->domain;
const uschar * rname_l;

HDEBUG(D_route)
  {
  debug_printf_indent(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
  debug_printf_indent("routing %s\n", addr->address);
  }

/* Loop through all router instances until a router succeeds, fails, defers, or
encounters an error. If the address has start_router set, we begin from there
instead of at the first router. */

expand_level++;
for (r = addr->start_router ? addr->start_router : routers; r; r = nextr)
  {
  uschar * error;
  struct passwd * pw = NULL, pwcopy;
  BOOL loop_detected = FALSE, more;
  int loopcount = 0, rc;

  rname_l = r->drinst.name;
  DEBUG(D_route)
    {
    expand_level--;
    debug_printf_indent("--------> %s router <--------\n", rname_l);
    expand_level++;
    }

  /* Reset any search error message from the previous router. */

  search_error_message = NULL;

  /* There are some weird cases where logging is disabled */

  f.disable_logging = r->disable_logging;

  /* Record the last router to handle the address, and set the default
  next router. */

  addr->router = r;
  nextr = r->drinst.next;

  /* Loop protection: If this address has an ancestor with the same address,
  and that ancestor was routed by this router, we skip this router. This
  prevents a variety of looping states when a new address is created by
  redirection or by the use of "unseen" on a router.

  If no_repeat_use is set on the router, we skip if _any_ ancestor was routed
  by  this router, even if it was different to the current address.

  Just in case someone does put it into a loop (possible with redirection
  continually adding to an address, for example), put a long stop counter on
  the number of parents. */

  for (address_item * parent = addr->parent; parent; parent = parent->parent)
    {
    if (parent->router == r)
      {
      BOOL break_loop = !r->repeat_use;

      /* When repeat_use is set, first check the active addresses caselessly.
      If they match, we have to do a further caseful check of the local parts
      when caseful_local_part is set. This is assumed to be rare, which is why
      the code is written this way. */

      if (!break_loop)
        {
        break_loop = strcmpic(parent->address, addr->address) == 0;
        if (break_loop && r->caseful_local_part)
          break_loop = Ustrncmp(parent->address, addr->address,
             Ustrrchr(addr->address, '@') - addr->address) == 0;
        }

      if (break_loop)
        {
        DEBUG(D_route) debug_printf_indent("%s router skipped: previously routed %s\n",
          rname_l, parent->address);
        loop_detected = TRUE;
        break;
        }
      }

    /* Continue with parents, limiting the size of the dynasty. */

    if (loopcount++ > 100)
      {
      log_write(0, LOG_MAIN|LOG_PANIC, "routing loop for %s", addr->address);
      yield = DEFER;
      goto ROUTERS_LOOP_EXIT;
      }
    }

  if (loop_detected) continue;

  /* Default no affixes and select whether to use a caseful or caseless local
  part in this router. */

  addr->prefix = addr->prefix_v = addr->suffix = addr->suffix_v = NULL;
  addr->local_part = r->caseful_local_part
    ? addr->cc_local_part : addr->lc_local_part;

  DEBUG(D_route) debug_printf_indent("local_part=%s domain=%s\n", addr->local_part,
    addr->domain);

  /* Handle any configured prefix by replacing the local_part address,
  and setting the prefix. Skip the router if the prefix doesn't match,
  unless the prefix is optional. */

  if (r->prefix)
    {
    unsigned vlen;
    int plen = route_check_prefix(addr->local_part, r->prefix, &vlen);
    if (plen > 0)
      {
      /* If the variable-part is zero-length then the prefix was not
      wildcarded and we can detaint-copy it since it matches the
      (non-expandable) router option.  Otherwise copy the (likely) tainted match
      and the variable-part of the match from the local_part. */

      if (vlen)
	{
	addr->prefix = string_copyn(addr->local_part, plen);
	addr->prefix_v = string_copyn(addr->local_part, vlen);
	}
      else
	addr->prefix = string_copyn_taint(addr->local_part, plen, GET_UNTAINTED);
      addr->local_part += plen;
      DEBUG(D_route) debug_printf_indent("stripped prefix %s\n", addr->prefix);
      }
    else if (!r->prefix_optional)
      {
      DEBUG(D_route)
	debug_printf_indent("%s router skipped: prefix mismatch\n", rname_l);
      continue;
      }
    }

  /* Handle any configured suffix likewise. */

  if (r->suffix)
    {
    unsigned vlen;
    int slen = route_check_suffix(addr->local_part, r->suffix, &vlen);
    if (slen > 0)
      {
      int lplen = Ustrlen(addr->local_part) - slen;
      addr->suffix = vlen
	? addr->local_part + lplen
	: string_copy_taint(addr->local_part + lplen, GET_UNTAINTED);
      addr->suffix_v = addr->suffix + Ustrlen(addr->suffix) - vlen;
      addr->local_part = string_copyn(addr->local_part, lplen);
      DEBUG(D_route) debug_printf_indent("stripped suffix %s\n", addr->suffix);
      }
    else if (!r->suffix_optional)
      {
      DEBUG(D_route)
	debug_printf_indent("%s router skipped: suffix mismatch\n", rname_l);
      continue;
      }
    }

  /* Set the expansion variables now that we have the affixes and the case of
  the local part sorted. */

  router_name = rname_l;
  driver_srcfile = r->drinst.srcfile;
  driver_srcline = r->drinst.srcline;
  deliver_set_expansions(addr);

  /* For convenience, the pre-router checks are in a separate function, which
  returns OK, SKIP, FAIL, or DEFER. */

  if ((rc = check_router_conditions(r, addr, verify, &pw, &error)) != OK)
    {
    driver_srcfile = router_name = NULL; driver_srcline = 0;
    if (rc == SKIP) continue;
    addr->message = error;
    yield = rc;
    goto ROUTERS_LOOP_EXIT;
    }

  /* All pre-conditions have been met. Reset any search error message from
  pre-condition tests. These can arise in negated tests where the failure of
  the lookup leads to a TRUE pre-condition. */

  search_error_message = NULL;

  /* Add any variable-settings that are on the router, to the set on the
  addr. Expansion is done here and not later when the addr is used.  There may
  be multiple settings, gathered during readconf; this code gathers them during
  router traversal.  On the addr string they are held as a variable tree, so
  as to maintain the post-expansion taints separate. */

  switch (rc = set_router_vars(addr, r))
    {
    case OK:	break;
    case PASS:	continue;		/* with next router */
    default: 	yield = rc; goto ROUTERS_LOOP_EXIT;
    }

  /* Finally, expand the address_data field in the router. Forced failure
  behaves as if the router declined. Any other failure is more serious. On
  success, the string is attached to the address for all subsequent processing.
  */

  if (r->address_data)
    {
    DEBUG(D_route|D_expand) debug_printf_indent("processing address_data\n");
    if (!(deliver_address_data = expand_string(r->address_data)))
      {
      if (f.expand_string_forcedfail)
        {
        DEBUG(D_route) debug_printf_indent("forced failure in expansion of %q "
            "(address_data): decline action taken\n", r->address_data);

        /* Expand "more" if necessary; DEFER => an expansion failed */

        yield = exp_bool(addr, US"router", rname_l, D_route,
			US"more", r->more, r->expand_more, &more);
        if (yield != OK) goto ROUTERS_LOOP_EXIT;

        if (!more)
          {
          DEBUG(D_route)
            debug_printf_indent("\"more\"=false: skipping remaining routers\n");
	  driver_srcfile = router_name = NULL; driver_srcline = 0;
          r = NULL;
          break;
          }
        else continue;    /* With next router */
        }

      else
        {
        addr->message = string_sprintf("expansion of %q failed "
          "in %s router: %s", r->address_data, rname_l, expand_string_message);
        yield = DEFER;
        goto ROUTERS_LOOP_EXIT;
        }
      }
    addr->prop.address_data = deliver_address_data;
    }

  /* We are finally cleared for take-off with this router. Clear the the flag
  that records that a local host was removed from a routed host list. Make a
  copy of relevant fields in the password information from check_local_user,
  because it will be overwritten if check_local_user is invoked again while
  verifying an errors_address setting. */

  clearflag(addr, af_local_host_removed);

  if (pw)
    {
    pwcopy.pw_name = CS string_copy(US pw->pw_name);
    pwcopy.pw_uid = pw->pw_uid;
    pwcopy.pw_gid = pw->pw_gid;
    pwcopy.pw_gecos = CS string_copy(US pw->pw_gecos);
    pwcopy.pw_dir = CS string_copy(US pw->pw_dir);
    pwcopy.pw_shell = CS string_copy(US pw->pw_shell);
    pw = &pwcopy;
    }

  /* If this should be the last hop for DSN flag the addr. */

  if (r->dsn_lasthop && !(addr->dsn_flags & rf_dsnlasthop))
    {
    addr->dsn_flags |= rf_dsnlasthop;
    HDEBUG(D_route) debug_printf_indent("DSN: last hop for %s\n", addr->address);
    }

  /* Run the router, and handle the consequences. */

  HDEBUG(D_route) debug_printf_indent("calling %s router\n", rname_l);

    {
    router_info * ri = r->drinst.info;
    yield = (ri->code)(r, addr, pw, verify, paddr_local, paddr_remote,
      addr_new, addr_succeed);
    }

  driver_srcfile = router_name = NULL; driver_srcline = 0;

  if (yield == FAIL)
    {
    HDEBUG(D_route) debug_printf_indent("%s router forced address failure\n", rname_l);
    goto ROUTERS_LOOP_EXIT;
    }

  /* If succeeded while verifying but fail_verify is set, convert into
  a failure, and take it off the local or remote delivery list. */

  if (  (  verify == v_sender && r->fail_verify_sender
	|| verify == v_recipient && r->fail_verify_recipient
	)
     && (yield == OK || yield == PASS))
    {
    addr->message = string_sprintf("%s router forced verify failure", rname_l);
    if (*paddr_remote == addr) *paddr_remote = addr->next;
    if (*paddr_local == addr) *paddr_local = addr->next;
    yield = FAIL;
    goto ROUTERS_LOOP_EXIT;
    }

  /* PASS and DECLINE are the only two cases where the loop continues. For all
  other returns, we break the loop and handle the result below. */

  if (yield != PASS && yield != DECLINE) break;

  HDEBUG(D_route)
    {
    debug_printf_indent("%s router %s for %s\n", rname_l,
      yield == PASS ? "passed" : "declined", addr->address);
    if (Ustrcmp(old_domain, addr->domain) != 0)
      debug_printf_indent("domain %s rewritten\n", old_domain);
    }

  /* PASS always continues to another router; DECLINE does so if "more"
  is true. Initialization insists that pass_router is always a following
  router. Otherwise, break the loop as if at the end of the routers. */

  if (yield == PASS)
    {
    if (r->pass_router) nextr = r->pass_router;
    }
  else
    {
    /* Expand "more" if necessary */

    yield = exp_bool(addr, US"router", rname_l, D_route,
		       	US"more", r->more, r->expand_more, &more);
    if (yield != OK) goto ROUTERS_LOOP_EXIT;

    if (!more)
      {
      HDEBUG(D_route)
        debug_printf_indent("\"more\" is false: skipping remaining routers\n");
      r = NULL;
      break;
      }
    }
  }                                      /* Loop for all routers */
expand_level--;

/* On exit from the routers loop, if r == NULL we have run out of routers,
either genuinely, or as a result of no_more. Otherwise, the loop ended
prematurely, either because a router succeeded, or because of some special
router response. Note that FAIL errors and errors detected before actually
running a router go direct to ROUTE_EXIT from code above. */

if (!r)
  {
  HDEBUG(D_route) debug_printf_indent("no more routers\n");
  if (!addr->message)
    {
    uschar * message = US"Unrouteable address";
    if (addr->router)
      {
      uschar * s = addr->router->cannot_route_message;
      GET_OPTION("cannot_route_message");
      if (s)
	{
	if ((s = expand_string(s)))
	  message = s;
	else
	  if (!f.expand_string_forcedfail)
	    log_write(0, LOG_MAIN|LOG_PANIC, "failed to expand "
	      "cannot_route_message in %s router: %s",
	      addr->router->drinst.name,
	      expand_string_message);
	}
      }
    addr->user_message = addr->message = message;
    }
  addr->router = NULL;         /* For logging */
  yield = FAIL;
  goto ROUTE_EXIT;
  }

if (yield == DEFER)
  {
  HDEBUG(D_route) debug_printf_indent("%s router: defer for %s\n  message: %s\n",
      rname_l, addr->address, addr->message ? addr->message : US"<none>");
  goto ROUTE_EXIT;
  }

if (yield == DISCARD) goto ROUTE_EXIT;

/* The yield must be either OK or REROUTED. */

if (yield != OK && yield != REROUTED)
  log_write_die(0, LOG_MAIN, "%s router returned unknown value %d",
    rname_l, yield);

/* If the yield was REROUTED, the router put a child address on the new chain
as a result of a domain change of some sort (widening, typically). */

if (yield == REROUTED)
  {
  HDEBUG(D_route) debug_printf_indent("re-routed to %s\n", addr->address);
  yield = OK;
  goto ROUTE_EXIT;
  }

/* The only remaining possibility is that the router succeeded. If the
translate_ip_address options is set and host addresses were associated with the
address, run them through the translation. This feature is for weird and
wonderful situations (the amateur packet radio people need it) or very broken
networking, so it is included in the binary only if requested. */

#ifdef SUPPORT_TRANSLATE_IP_ADDRESS

GET_OPTION("translate_ip_address");
if (r->translate_ip_address)
  {
  int rc;
  int old_pool = store_pool;
  for (host_item * h = addr->host_list; h; h = h->next)
    {
    uschar *newaddress;
    uschar *oldaddress, *oldname;

    if (!h->address) continue;

    deliver_host_address = h->address;
    newaddress = expand_string(r->translate_ip_address);
    deliver_host_address = NULL;

    if (!newaddress)
      {
      if (f.expand_string_forcedfail) continue;
      addr->basic_errno = ERRNO_EXPANDFAIL;
      addr->message = string_sprintf("translate_ip_address expansion "
        "failed: %s", expand_string_message);
      yield = DEFER;
      goto ROUTE_EXIT;
      }

    DEBUG(D_route) debug_printf_indent("%s [%s] translated to %s\n",
      h->name, h->address, newaddress);
    if (string_is_ip_address(newaddress, NULL) != 0)
      {
      h->address = newaddress;
      continue;
      }

    oldname = h->name;
    oldaddress = h->address;
    h->name = newaddress;
    h->address = NULL;
    h->mx = MX_NONE;

    store_pool = POOL_PERM;
    rc = host_find_byname(h, NULL, HOST_FIND_QUALIFY_SINGLE, NULL, TRUE);
    store_pool = old_pool;

    if (rc == HOST_FIND_FAILED || rc == HOST_FIND_AGAIN)
      {
      addr->basic_errno = ERRNO_UNKNOWNHOST;
      addr->message = string_sprintf("host %s not found when "
        "translating %s [%s]", h->name, oldname, oldaddress);
      yield = DEFER;
      goto ROUTE_EXIT;
      }
    }
  }
#endif  /* SUPPORT_TRANSLATE_IP_ADDRESS */

/* See if this is an unseen routing; first expand the option if necessary.
DEFER can be given if the expansion fails */

if ((yield = exp_bool(addr, US"router", rname_l, D_route,
      US"unseen", r->unseen, r->expand_unseen, &unseen)) != OK) goto ROUTE_EXIT;

/* Debugging output recording a successful routing */

HDEBUG(D_route) debug_printf_indent("routed by %s router%s\n", rname_l,
    unseen ? " (unseen)" : "");

DEBUG(D_route)
  {
  debug_printf_indent("  envelope to:\t%s\n", addr->address);
  debug_printf_indent("  transport:\t%s\n", addr->transport
    ? addr->transport->drinst.name : US"<none>");

  if (addr->prop.errors_address)
    debug_printf_indent("  errors to:\t%s\n", addr->prop.errors_address);

  for (host_item * h = addr->host_list; h; h = h->next)
    {
    debug_printf_indent("  host %s", h->name);
    if (h->address) debug_printf_indent(" [%s]", h->address);
    if (h->mx >= 0) debug_printf_indent(" MX=%d", h->mx);
    else if (h->mx != MX_NONE) debug_printf_indent(" rgroup=%d", h->mx);

    if (h->port != PORT_NONE) debug_printf_indent(" port=%d", h->port);
    if (h->dnssec_used != DS_UNK)
      debug_printf_indent(" dnssec=%s", h->dnssec_used==DS_YES ? "yes" : "no");
    debug_printf_indent("\n");
    }
  }

/* Clear any temporary error message set by a router that declined, and handle
the "unseen" option (ignore if there are no further routers). */

addr->message = NULL;
if (unseen && r->drinst.next)
  route_unseen(rname_l, addr, paddr_local, paddr_remote, addr_new);

/* Unset the address expansions, and return the final result. */

ROUTE_EXIT:
  if (yield == DEFER && addr->message)
    addr->message = expand_hide_passwords(addr->message);

  deliver_set_expansions(NULL);
  driver_srcfile = router_name = NULL; driver_srcline = 0;
  f.disable_logging = FALSE;
  return yield;

ROUTERS_LOOP_EXIT:
  expand_level--;
  goto ROUTE_EXIT;
}



/* For error messages, a string describing the config location associated
with current processing.  NULL if we are not in a router. */
/* Name only, for now */

uschar *
router_current_name(void)
{
if (!router_name) return NULL;
return string_sprintf(" (router %s, %s %d)", router_name, driver_srcfile, driver_srcline);
}

#endif	/*!MACRO_PREDEF*/
/* End of route.c */
