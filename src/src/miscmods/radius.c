/*************************************************
*     Exim - an Internet mail transport agent    *
*************************************************/

/* Copyright (c) The Exim Maintainers 2020 - 2022 */
/* Copyright (c) University of Cambridge 1995 - 2016 */
/* See the file NOTICE for conditions of use and distribution. */
/* SPDX-License-Identifier: GPL-2.0-or-later */

/* This file was originally supplied by Ian Kirk. The libradius support came
from Alex Kiernan. */

#include "../exim.h"

/* This module contains functions that call the Radius authentication
mechanism.

We can't just compile this code and allow the library mechanism to omit the
functions if they are not wanted, because we need to have the Radius headers
available for compiling. Therefore, compile these functions only if
RADIUS_CONFIG_FILE is defined. However, some compilers don't like compiling
empty modules, so keep them happy with a dummy when skipping the rest. Make it
reference itself to stop picky compilers complaining that it is unused, and put
in a dummy argument to stop even pickier compilers complaining about infinite
loops. Then use a mutually-recursive pair as gcc is just getting stupid. */

#ifndef RADIUS_CONFIG_FILE
static void dummy(int x);
static void dummy2(int x) { dummy(x-1); }
static void dummy(int x) { dummy2(x-1); }
#else  /* RADIUS_CONFIG_FILE */


/* Two different Radius libraries are supported. The default is radiusclient,
using its original API. At release 0.4.0 the API changed. */

#ifdef RADIUS_LIB_RADLIB
# include <radlib.h>
#else
# if !defined(RADIUS_LIB_RADIUSCLIENT) && !defined(RADIUS_LIB_RADIUSCLIENTNEW)
#  define RADIUS_LIB_RADIUSCLIENT
# endif

# ifdef RADIUS_LIB_RADIUSCLIENTNEW
#  define ENV FREERADIUSCLIENT_ENV	/* Avoid clash with Berkeley DB */
#  include <freeradius-client.h>
# else
#  include <radiusclient.h>
# endif
#endif



/*************************************************
*              Perform RADIUS authentication     *
*************************************************/

/* This function calls the Radius authentication mechanism, passing over one or
more data strings.

Arguments:
  s        a colon-separated list of strings
  errptr   where to point an error message

Returns:   OK if authentication succeeded
           FAIL if authentication failed
           ERROR some other error condition
*/

static int
auth_call_radius(const uschar *s, uschar **errptr)
{
uschar *user;
const uschar *radius_args = s;
int result;
int sep = ':';

#ifdef RADIUS_LIB_RADLIB
  struct rad_handle *h;
#else
  #ifdef RADIUS_LIB_RADIUSCLIENTNEW
    rc_handle *h;
  #endif
  VALUE_PAIR *send = NULL;
  VALUE_PAIR *received;
  unsigned int service = PW_AUTHENTICATE_ONLY;
  char msg[4096];
#endif


if (!(user = string_nextinlist(&radius_args, &sep, NULL, 0))) user = US"";

DEBUG(D_auth) debug_printf("Running RADIUS authentication for user %q "
               "and %q\n", user, radius_args);

*errptr = NULL;


/* Authenticate using the radiusclient library */

#ifndef RADIUS_LIB_RADLIB

rc_openlog("exim");

#ifdef RADIUS_LIB_RADIUSCLIENT
if (rc_read_config(RADIUS_CONFIG_FILE) != 0)
  *errptr = string_sprintf("RADIUS: can't open %s", RADIUS_CONFIG_FILE);

else if (rc_read_dictionary(rc_conf_str("dictionary")) != 0)
  *errptr = US"RADIUS: can't read dictionary";

else if (!rc_avpair_add(&send, PW_USER_NAME, user, 0))
  *errptr = US"RADIUS: add user name failed";

else if (!rc_avpair_add(&send, PW_USER_PASSWORD, CS radius_args, 0))
  *errptr = US"RADIUS: add password failed");

else if (!rc_avpair_add(&send, PW_SERVICE_TYPE, &service, 0))
  *errptr = US"RADIUS: add service type failed";

#else  /* RADIUS_LIB_RADIUSCLIENT unset => RADIUS_LIB_RADIUSCLIENT2 */

if (!(h = rc_read_config(RADIUS_CONFIG_FILE)))
  *errptr = string_sprintf("RADIUS: can't open %s", RADIUS_CONFIG_FILE);

else if (rc_read_dictionary(h, rc_conf_str(h, "dictionary")) != 0)
  *errptr = US"RADIUS: can't read dictionary";

else if (!rc_avpair_add(h, &send, PW_USER_NAME, user, Ustrlen(user), 0))
  *errptr = US"RADIUS: add user name failed";

else if (!rc_avpair_add(h, &send, PW_USER_PASSWORD, CS radius_args,
    Ustrlen(radius_args), 0))
  *errptr = US"RADIUS: add password failed";

else if (!rc_avpair_add(h, &send, PW_SERVICE_TYPE, &service, 0, 0))
  *errptr = US"RADIUS: add service type failed";

#endif  /* RADIUS_LIB_RADIUSCLIENT */

if (*errptr)
  {
  DEBUG(D_auth) debug_printf("%s\n", *errptr);
  return ERROR;
  }

#ifdef RADIUS_LIB_RADIUSCLIENT
result = rc_auth(0, send, &received, msg);
#else
result = rc_auth(h, 0, send, &received, msg);
#endif

DEBUG(D_auth) debug_printf("RADIUS code returned %d\n", result);

switch (result)
  {
  case OK_RC:
    return OK;

  case REJECT_RC:
  case ERROR_RC:
    return FAIL;

  case TIMEOUT_RC:
    *errptr = US"RADIUS: timed out";
    return ERROR;

  case BADRESP_RC:
  default:
    *errptr = string_sprintf("RADIUS: unexpected response (%d)", result);
    return ERROR;
  }

#else  /* RADIUS_LIB_RADLIB is set */

/* Authenticate using the libradius library */

if (!(h = rad_auth_open()))
  {
  *errptr = string_sprintf("RADIUS: can't initialise libradius");
  return ERROR;
  }
if (rad_config(h, RADIUS_CONFIG_FILE) != 0 ||
    rad_create_request(h, RAD_ACCESS_REQUEST) != 0 ||
    rad_put_string(h, RAD_USER_NAME, CS user) != 0 ||
    rad_put_string(h, RAD_USER_PASSWORD, CS radius_args) != 0 ||
    rad_put_int(h, RAD_SERVICE_TYPE, RAD_AUTHENTICATE_ONLY) != 0 ||
    rad_put_string(h, RAD_NAS_IDENTIFIER, CS primary_hostname) != 0)
  {
  *errptr = string_sprintf("RADIUS: %s", rad_strerror(h));
  result = ERROR;
  }
else
  switch (result = rad_send_request(h))
    {
    case RAD_ACCESS_ACCEPT:
      result = OK;
      break;

    case RAD_ACCESS_REJECT:
      result = FAIL;
      break;

    case -1:
      *errptr = string_sprintf("RADIUS: %s", rad_strerror(h));
      result = ERROR;
      break;

    default:
      *errptr = string_sprintf("RADIUS: unexpected response (%d)", result);
      result= ERROR;
      break;
    }

if (*errptr) DEBUG(D_auth) debug_printf("%s\n", *errptr);
rad_close(h);
return result;

#endif  /* RADIUS_LIB_RADLIB */
}



/******************************************************************************/
/* Module API */

static void * rad_functions[] = {
  [RADIUS_AUTH_CALL] =	(void *) auth_call_radius,
};

misc_module_info radius_module_info =
{
  .name =		US"radius",
# ifdef DYNLOOKUP
  .dyn_magic =		MISC_MODULE_MAGIC,
# endif

  .functions =		rad_functions,
  .functions_count =	nelem(rad_functions),
};

#endif  /* RADIUS_CONFIG_FILE */

/* End of call_radius.c */
