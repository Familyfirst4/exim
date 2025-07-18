/*************************************************
*     Exim - an Internet mail transport agent    *
*************************************************/

/* Copyright (c) The Exim Maintainers 2020 - 2024 */
/* Copyright (c) University of Cambridge 1995 - 2018 */
/* See the file NOTICE for conditions of use and distribution. */
/* SPDX-License-Identifier: GPL-2.0-or-later */


#include "../exim.h"

#ifdef TRANSPORT_AUTOREPLY	/* Remainder of file */
#include "autoreply.h"



/* Options specific to the autoreply transport. They must be in alphabetic
order (note that "_" comes before the lower case letters). Those starting
with "*" are not settable by the user but are used by the option-reading
software for alternative value types. Some options are publicly visible and so
are stored in the driver instance block. These are flagged with opt_public. */
#define LOFF(field) OPT_OFF(autoreply_transport_options_block, field)

optionlist autoreply_transport_options[] = {
  { "bcc",               opt_stringptr,	LOFF(bcc) },
  { "cc",                opt_stringptr,	LOFF(cc) },
  { "file",              opt_stringptr,	LOFF(file) },
  { "file_expand",     opt_bool,	LOFF(file_expand) },
  { "file_optional",     opt_bool,	LOFF(file_optional) },
  { "from",              opt_stringptr,	LOFF(from) },
  { "headers",           opt_stringptr,	LOFF(headers) },
  { "log",               opt_stringptr,	LOFF(logfile) },
  { "mode",              opt_octint,	LOFF(mode) },
  { "never_mail",        opt_stringptr,	LOFF(never_mail) },
  { "once",              opt_stringptr,	LOFF(oncelog) },
  { "once_file_size",    opt_int,	LOFF(once_file_size) },
  { "once_repeat",       opt_stringptr,	LOFF(once_repeat) },
  { "reply_to",          opt_stringptr,	LOFF(reply_to) },
  { "return_message",    opt_bool,	LOFF(return_message) },
  { "subject",           opt_stringptr,	LOFF(subject) },
  { "text",              opt_stringptr,	LOFF(text) },
  { "to",                opt_stringptr,	LOFF(to) },
};

/* Size of the options list. An extern variable has to be used so that its
address can appear in the tables drtables.c. */

int autoreply_transport_options_count =
  sizeof(autoreply_transport_options)/sizeof(optionlist);


#ifdef MACRO_PREDEF

/* Dummy values */
autoreply_transport_options_block autoreply_transport_option_defaults = {0};
void autoreply_transport_init(driver_instance *tblock) {}
BOOL autoreply_transport_entry(transport_instance *tblock, address_item *addr) {return FALSE;}

#else   /*!MACRO_PREDEF*/


/* Default private options block for the autoreply transport.
All non-mentioned lements zero/null/false. */

autoreply_transport_options_block autoreply_transport_option_defaults = {
  .mode = 0600,
};



/* Type of text for the checkexpand() function */

enum { cke_text, cke_hdr, cke_file };



/*************************************************
*          Initialization entry point            *
*************************************************/

/* Called for each instance, after its options have been read, to
enable consistency checks to be done, or anything else that needs
to be set up. */

void
autoreply_transport_init(driver_instance * t)
{
const transport_instance * tblock = (transport_instance *)t;
/*
autoreply_transport_options_block *ob =
  (autoreply_transport_options_block *)(tblock->options_block);
*/

/* If a fixed uid field is set, then a gid field must also be set. */

if (tblock->uid_set && !tblock->gid_set && tblock->expand_gid == NULL)
  log_write_die(0, LOG_CONFIG,
    "user set without group for the %s transport", t->name);
}




/*************************************************
*          Expand string and check               *
*************************************************/

/* If the expansion fails, the error is set up in the address. Expanded
strings must be checked to ensure they contain only printing characters
and white space. If not, the function fails.

Arguments:
   s         string to expand
   addr      address that is being worked on
   name      transport name, for error text
   type      type, for checking content:
               cke_text => no check
               cke_hdr  => header, allow \n + whitespace
               cke_file => file name, no non-printers allowed

Returns:     expanded string if expansion succeeds;
             NULL otherwise
*/

static const uschar *
checkexpand(const uschar * s, address_item * addr, const uschar * name,
  int type)
{
const uschar * ss = expand_string(s);

if (!ss)
  {
  addr->transport_return = FAIL;
  addr->message = string_sprintf("Expansion of %q failed in %s transport: "
    "%s", s, name, expand_string_message);
  return NULL;
  }

if (type != cke_text) for (const uschar * t = ss; *t; t++)
  {
  int c = *t;
  const uschar * sp;
  if (mac_isprint(c)) continue;
  if (type == cke_hdr && c == '\n' && (t[1] == ' ' || t[1] == '\t')) continue;
  sp = string_printing(s);
  addr->transport_return = FAIL;
  addr->message = string_sprintf("Expansion of %q in %s transport "
    "contains non-printing character %d", sp, name, c);
  return NULL;
  }

return ss;
}




/*************************************************
*          Check a header line for never_mail    *
*************************************************/

/* This is called to check to, cc, and bcc for addresses in the never_mail
list. Any that are found are removed.

Arguments:
  list        list of addresses to be checked
  never_mail  an address list, already expanded

Returns:      edited replacement address list, or NULL, or original
*/

static const uschar *
check_never_mail(const uschar * list, const uschar * never_mail)
{
rmark reset_point = store_mark();
uschar * newlist = string_copy(list);
uschar * s = newlist;
BOOL hit = FALSE;

while (*s)
  {
  uschar *error, *next;
  uschar *e = parse_find_address_end(s, FALSE);
  int terminator = *e;
  int start, end, domain, rc;

  /* Temporarily terminate the string at the address end while extracting
  the operative address within. */

  *e = 0;
  next = parse_extract_address(s, &error, &start, &end, &domain, FALSE);
  *e = terminator;

  /* If there is some kind of syntax error, just give up on this header
  line. */

  if (!next) break;

  /* See if the address is on the never_mail list */

  rc = match_address_list(next,         /* address to check */
                          TRUE,         /* start caseless */
                          FALSE,        /* don't expand the list */
                          &never_mail,  /* the list */
                          NULL,         /* no caching */
                          -1,           /* no expand setup */
                          0,            /* separator from list */
                          NULL);        /* no lookup value return */

  if (rc == OK)                         /* Remove this address */
    {
    DEBUG(D_transport)
      debug_printf("discarding recipient %s (matched never_mail)\n", next);
    hit = TRUE;
    if (terminator == ',') e++;
    memmove(s, e, Ustrlen(e) + 1);
    }
  else                                  /* Skip over this address */
    {
    s = e;
    if (terminator == ',') s++;
    }
  }

/* If no addresses were removed, retrieve the memory used and return
the original. */

if (!hit)
  {
  store_reset(reset_point);
  return list;
  }

/* Check to see if we removed the last address, leaving a terminating comma
that needs to be removed */

s = newlist + Ustrlen(newlist);
while (s > newlist && (isspace(s[-1]) || s[-1] == ',')) s--;
*s = '\0';

/* Check to see if there any addresses left; if not, return NULL */

s = newlist;
if (Uskip_whitespace(&s))
  return newlist;

store_reset(reset_point);
return NULL;
}



/*************************************************
*              Main entry point                  *
*************************************************/

/* See local README for interface details. This transport always returns
FALSE, indicating that the top address has the status for all - though in fact
this transport can handle only one address at at time anyway. */

BOOL
autoreply_transport_entry(
  transport_instance *tblock,      /* data for this instantiation */
  address_item *addr)              /* address we are working on */
{
autoreply_transport_options_block * ob = tblock->drinst.options_block;
const uschar * trname = tblock->drinst.name;
int fd, pid, rc;
int cache_fd = -1;
int cache_size = 0;
int add_size = 0;
EXIM_DB * dbm_file = NULL;
BOOL file_expand, return_message;
const uschar * from, * reply_to, * to, * cc, * bcc, * subject, * headers;
const uschar * text, * file, * logfile, * oncelog;
uschar * cache_buff = NULL, * cache_time = NULL, * message_id = NULL;
header_line * h;
time_t now = time(NULL), once_repeat_sec = 0;
FILE * ff = NULL, * fp;

DEBUG(D_transport) debug_printf("%s transport entered\n", trname);

/* Set up for the good case */

addr->transport_return = OK;
addr->basic_errno = 0;

/* If the address is pointing to a reply block, then take all the data
from that block. It has typically been set up by a mail filter processing
router. Otherwise, the data must be supplied by this transport, and
it has to be expanded here. */

if (addr->reply)
  {
  DEBUG(D_transport) debug_printf("taking data from address\n");
  from = addr->reply->from;
  reply_to = addr->reply->reply_to;
  to = addr->reply->to;
  cc = addr->reply->cc;
  bcc = addr->reply->bcc;
  subject = addr->reply->subject;
  headers = addr->reply->headers;
  text = addr->reply->text;
  file = addr->reply->file;
  logfile = addr->reply->logfile;
  oncelog = addr->reply->oncelog;
  once_repeat_sec = addr->reply->once_repeat;
  file_expand = addr->reply->file_expand;
  expand_forbid = addr->reply->expand_forbid;
  return_message = addr->reply->return_message;
  }
else
  {
  const uschar * oncerepeat;

  DEBUG(D_transport) debug_printf("taking data from transport\n");
  GET_OPTION("once_repeat");	oncerepeat = ob->once_repeat;
  GET_OPTION("from"); 		from = ob->from;
  GET_OPTION("reply_to");	reply_to = ob->reply_to;
  GET_OPTION("to");		to = ob->to;
  GET_OPTION("cc");		cc = ob->cc;
  GET_OPTION("bcc");		bcc = ob->bcc;
  GET_OPTION("subject");	subject = ob->subject;
  GET_OPTION("headers"); 	headers = ob->headers;
  GET_OPTION("text");		text = ob->text;
  GET_OPTION("file");		file = ob->file;
  GET_OPTION("log");		logfile = ob->logfile;
  GET_OPTION("once");		oncelog = ob->oncelog;
  file_expand = ob->file_expand;
  return_message = ob->return_message;

  if (  from && !(from = checkexpand(from, addr, trname, cke_hdr))
     || reply_to && !(reply_to = checkexpand(reply_to, addr, trname, cke_hdr))
     || to && !(to = checkexpand(to, addr, trname, cke_hdr))
     || cc && !(cc = checkexpand(cc, addr, trname, cke_hdr))
     || bcc && !(bcc = checkexpand(bcc, addr, trname, cke_hdr))
     || subject && !(subject = checkexpand(subject, addr, trname, cke_hdr))
     || headers && !(headers = checkexpand(headers, addr, trname, cke_text))
     || text && !(text = checkexpand(text, addr, trname, cke_text))
     || file && !(file = checkexpand(file, addr, trname, cke_file))
     || logfile && !(logfile = checkexpand(logfile, addr, trname, cke_file))
     || oncelog && !(oncelog = checkexpand(oncelog, addr, trname, cke_file))
     || oncerepeat && !(oncerepeat = checkexpand(oncerepeat, addr, trname, cke_file))
     )
    return FALSE;

  if (oncerepeat)
    if ((once_repeat_sec = readconf_readtime(oncerepeat, 0, FALSE)) < 0)
      {
      addr->transport_return = FAIL;
      addr->message = string_sprintf("Invalid time value %q for "
        "\"once_repeat\" in %s transport", oncerepeat, trname);
      return FALSE;
      }
  }

/* If the never_mail option is set, we have to scan all the recipients and
remove those that match. */

if (ob->never_mail)
  {
  const uschar * never_mail = expand_string(ob->never_mail);

  if (!never_mail)
    {
    addr->transport_return = FAIL;
    addr->message = string_sprintf("Failed to expand %q for "
      "\"never_mail\" in %s transport", ob->never_mail, trname);
    return FALSE;
    }

  if (to) to = check_never_mail(to, never_mail);
  if (cc) cc = check_never_mail(cc, never_mail);
  if (bcc) bcc = check_never_mail(bcc, never_mail);

  if (!to && !cc && !bcc)
    {
    DEBUG(D_transport)
      debug_printf("*** all recipients removed by never_mail\n");
    return OK;
    }
  }

/* If the -N option is set, can't do any more. */

if (f.dont_deliver)
  {
  DEBUG(D_transport)
    debug_printf("*** delivery by %s transport bypassed by -N option\n",
      trname);
  return FALSE;
  }


/* If the oncelog field is set, we send want to send only one message to the
given recipient(s). This works only on the "To" field. If there is no "To"
field, the message is always sent. If the To: field contains more than one
recipient, the effect might not be quite as envisaged. If once_file_size is
set, instead of a dbm file, we use a regular file containing a circular buffer
recipient cache. */

if (oncelog && *oncelog && to)
  {
  time_t then = 0;

  if (is_tainted(oncelog))
    {
    addr->transport_return = DEFER;
    addr->basic_errno = EACCES;
    addr->message = string_sprintf("Tainted '%s' (once file for %s transport)"
      " not permitted", oncelog, trname);
    goto END_OFF;
    }

  /* Handle fixed-size cache file. */

  if (ob->once_file_size > 0)
    {
    uschar * nextp;
    struct stat statbuf;

    cache_fd = Uopen(oncelog, O_CREAT|O_RDWR, ob->mode);
    if (cache_fd < 0 || fstat(cache_fd, &statbuf) != 0)
      {
      addr->transport_return = DEFER;
      addr->basic_errno = errno;
      addr->message = string_sprintf("Failed to %s \"once\" file %s when "
        "sending message from %s transport: %s",
        cache_fd < 0 ? "open" : "stat", oncelog, trname, strerror(errno));
      goto END_OFF;
      }

    /* Get store in the temporary pool and read the entire file into it. We get
    an amount of store that is big enough to add the new entry on the end if we
    need to do that. */

    cache_size = statbuf.st_size;
    add_size = sizeof(time_t) + Ustrlen(to) + 1;
    cache_buff = store_get(cache_size + add_size, oncelog);

    if (read(cache_fd, cache_buff, cache_size) != cache_size)
      {
      addr->transport_return = DEFER;
      addr->basic_errno = errno;
      addr->message = US"error while reading \"once\" file";
      goto END_OFF;
      }

    DEBUG(D_transport) debug_printf("%d bytes read from %s\n", cache_size, oncelog);

    /* Scan the data for this recipient. Each entry in the file starts with
    a time_t sized time value, followed by the address, followed by a binary
    zero. If we find a match, put the time into "then", and the place where it
    was found into "cache_time". Otherwise, "then" is left at zero. */

    for (uschar * p = cache_buff; p < cache_buff + cache_size; p = nextp)
      {
      uschar *s = p + sizeof(time_t);
      nextp = s + Ustrlen(s) + 1;
      if (Ustrcmp(to, s) == 0)
        {
        memcpy(&then, p, sizeof(time_t));
        cache_time = p;
        break;
        }
      }
    }

  /* Use a DBM file for the list of previous recipients. */

  else
    {
    EXIM_DATUM key_datum, result_datum;
    const uschar * s = Ustrrchr(oncelog, '/');
    const uschar * dirname = s ? string_copyn(oncelog, s - oncelog) : NULL;

    if (!(dbm_file = exim_dbopen(oncelog, dirname, O_RDWR|O_CREAT, ob->mode)))
      {
      addr->transport_return = DEFER;
      addr->basic_errno = errno;
      addr->message = string_sprintf("Failed to open %s file %s when sending "
        "message from %s transport: %s", EXIM_DBTYPE, oncelog, trname,
        strerror(errno));
      goto END_OFF;
      }

    exim_datum_init(&key_datum);        /* Some DBM libraries need datums */
    exim_datum_init(&result_datum);     /* to be cleared */
    exim_datum_data_set(&key_datum, (void *) to);
    exim_datum_size_set(&key_datum, Ustrlen(to) + 1);

    if (exim_dbget(dbm_file, &key_datum, &result_datum))
      memcpy(&then, exim_datum_data_get(&result_datum), sizeof(time_t));
    }

  /* Either "then" is set zero, if no message has yet been sent, or it
  is set to the time of the last sending. */

  if (then != 0 && (once_repeat_sec <= 0 || now - then < once_repeat_sec))
    {
    int log_fd;
    if (is_tainted(logfile))
      {
      addr->transport_return = DEFER;
      addr->basic_errno = EACCES;
      addr->message = string_sprintf("Tainted '%s' (logfile for %s transport)"
	" not permitted", logfile, trname);
      goto END_OFF;
      }

    DEBUG(D_transport) debug_printf("message previously sent to %s%s\n", to,
      (once_repeat_sec > 0)? " and repeat time not reached" : "");
    log_fd = logfile ? Uopen(logfile, O_WRONLY|O_APPEND|O_CREAT, ob->mode) : -1;
    if (log_fd >= 0)
      {
      uschar *ptr = log_buffer;
      sprintf(CS ptr, "%s\n  previously sent to %.200s\n", tod_stamp(tod_log), to);
      while(*ptr) ptr++;
      if(write(log_fd, log_buffer, ptr - log_buffer) != ptr-log_buffer
        || close(log_fd))
        DEBUG(D_transport) debug_printf("Problem writing log file %s for %s "
          "transport\n", logfile, trname);
      }
    goto END_OFF;
    }

  DEBUG(D_transport) debug_printf("%s %s\n", (then <= 0)?
    "no previous message sent to" : "repeat time reached for", to);
  }

/* We are going to send a message. Ensure any requested file is available. */
if (file)
  {
  if (is_tainted(file))
    {
    addr->transport_return = DEFER;
    addr->basic_errno = EACCES;
    addr->message = string_sprintf("Tainted '%s' (file for %s transport)"
      " not permitted", file, trname);
    return FALSE;
    }
  if (!(ff = Ufopen(file, "rb")) && !ob->file_optional)
    {
    addr->transport_return = DEFER;
    addr->basic_errno = errno;
    addr->message = string_sprintf("Failed to open file %s when sending "
      "message from %s transport: %s", file, trname, strerror(errno));
    return FALSE;
    }
  }

/* Make a subprocess to send the message */

if ((pid = child_open_exim(&fd, US"autoreply")) < 0)
  {
  /* Creation of child failed; defer this delivery. */

  addr->transport_return = DEFER;
  addr->basic_errno = errno;
  addr->message = string_sprintf("Failed to create child process to send "
    "message from %s transport: %s", trname, strerror(errno));
  DEBUG(D_transport) debug_printf("%s\n", addr->message);
  if (dbm_file) exim_dbclose(dbm_file);
  return FALSE;
  }

/* Create the message to be sent - recipients are taken from the headers,
as the -t option is used. The "headers" stuff *must* be last in case there
are newlines in it which might, if placed earlier, screw up other headers. */

fp = fdopen(fd, "wb");

if (from) fprintf(fp, "From: %s\n", from);
if (reply_to) fprintf(fp, "Reply-To: %s\n", reply_to);
if (to) fprintf(fp, "To: %s\n", to);
if (cc) fprintf(fp, "Cc: %s\n", cc);
if (bcc) fprintf(fp, "Bcc: %s\n", bcc);
if (subject) fprintf(fp, "Subject: %s\n", subject);

/* Generate In-Reply-To from the message_id header; there should
always be one, but code defensively. */

for (h = header_list; h; h = h->next)
  if (h->type == htype_id) break;

if (h)
  {
  message_id = Ustrchr(h->text, ':') + 1;
  Uskip_whitespace(&message_id);
  fprintf(fp, "In-Reply-To: %s", message_id);
  }

moan_write_references(fp, message_id);

/* Add an Auto-Submitted: header */

fprintf(fp, "Auto-Submitted: auto-replied\n");

/* Add any specially requested headers */

if (headers) fprintf(fp, "%s\n", headers);
fprintf(fp, "\n");

if (text)
  {
  fprintf(fp, "%s", CS text);
  if (text[Ustrlen(text)-1] != '\n') fprintf(fp, "\n");
  }

if (ff)
  {
  while (Ufgets(big_buffer, big_buffer_size, ff) != NULL)
    if (file_expand)
      {
      const uschar * s = expand_string(big_buffer);
      if (!s) DEBUG(D_transport)
	debug_printf("error while expanding line from file:\n  %s\n  %s\n",
	  big_buffer, expand_string_message);
      fprintf(fp, "%s", s ? CS s : CS big_buffer);
      }
    else
      fprintf(fp, "%s", CS big_buffer);

  (void) fclose(ff);
  }

/* Copy the original message if required, observing the return size
limit if we are returning the body. */

if (return_message)
  {
  const uschar * rubric = tblock->headers_only
    ? US"------ This is a copy of the message's header lines.\n"
    : tblock->body_only
    ? US"------ This is a copy of the body of the message, without the headers.\n"
    : US"------ This is a copy of the message, including all the headers.\n";
  transport_ctx tctx = {
    .u = {.fd = fileno(fp)},
    .tblock = tblock,
    .addr = addr,
    .check_string = NULL,
    .escape_string =  NULL,
    .options = (tblock->body_only ? topt_no_headers : 0)
    	| (tblock->headers_only ? topt_no_body : 0)
    	| (tblock->return_path_add ? topt_add_return_path : 0)
    	| (tblock->delivery_date_add ? topt_add_delivery_date : 0)
    	| (tblock->envelope_to_add ? topt_add_envelope_to : 0)
	| topt_not_socket
  };

  if (bounce_return_size_limit > 0 && !tblock->headers_only)
    {
    struct stat statbuf;
    int max = (bounce_return_size_limit/DELIVER_IN_BUFFER_SIZE + 1) *
      DELIVER_IN_BUFFER_SIZE;
    if (fstat(deliver_datafile, &statbuf) == 0 && statbuf.st_size > max)
      {
      fprintf(fp, "\n%s"
"------ The body of the message is " OFF_T_FMT " characters long; only the first\n"
"------ %d or so are included here.\n\n", rubric, statbuf.st_size,
        (max/1000)*1000);
      }
    else fprintf(fp, "\n%s\n", rubric);
    }
  else fprintf(fp, "\n%s\n", rubric);

  fflush(fp);
  transport_count = 0;
  transport_write_message(&tctx, bounce_return_size_limit);
  }

/* End the message and wait for the child process to end; no timeout. */

(void)fclose(fp);
rc = child_close(pid, 0);

/* Update the "sent to" log whatever the yield. This errs on the side of
missing out a message rather than risking sending more than one. We either have
cache_fd set to a fixed size, circular buffer file, or dbm_file set to an open
DBM file (or neither, if "once" is not set). */

/* Update fixed-size cache file. If cache_time is set, we found a previous
entry; that is the spot into which to put the current time. Otherwise we have
to add a new record; remove the first one in the file if the file is too big.
We always rewrite the entire file in a single write operation. This is
(hopefully) going to be the safest thing because there is no interlocking
between multiple simultaneous deliveries. */

if (cache_fd >= 0)
  {
  if (lseek(cache_fd, 0, SEEK_SET) == 0)
    {
    uschar * from = cache_buff;
    int size = cache_size;

    if (!cache_time)
      {
      cache_time = from + size;
      memcpy(cache_time + sizeof(time_t), to, add_size - sizeof(time_t));
      size += add_size;

      if (cache_size > 0 && size > ob->once_file_size)
	{
	from += sizeof(time_t) + Ustrlen(from + sizeof(time_t)) + 1;
	size -= (from - cache_buff);
	}
      }

    memcpy(cache_time, &now, sizeof(time_t));
    if(write(cache_fd, from, size) != size)
      DEBUG(D_transport) debug_printf("Problem writing cache file %s for %s "
	"transport\n", oncelog, trname);
    }
  }

/* Update DBM file */

else if (dbm_file)
  {
  EXIM_DATUM key_datum, value_datum;
  exim_datum_init(&key_datum);          /* Some DBM libraries need to have */
  exim_datum_init(&value_datum);        /* cleared datums. */
  exim_datum_data_set(&key_datum, US to);   /*XXX rely on dbput not modifying */
  exim_datum_size_set(&key_datum, Ustrlen(to) + 1);

  /* Many OS define the datum value, sensibly, as a void *. However, there
  are some which still have char *. By casting this address to a char * we
  can avoid warning messages from the char * systems. */

  exim_datum_data_set(&value_datum, &now);
  exim_datum_size_set(&value_datum, sizeof(time_t));
  exim_dbput(dbm_file, &key_datum, &value_datum);
  }

/* If sending failed, defer to try again - but if once is set the next
try will skip, of course. However, if there were no recipients in the
message, we do not fail. */

if (rc != 0)
  if (rc == EXIT_NORECIPIENTS)
    {
    DEBUG(D_any) debug_printf("%s transport: message contained no recipients\n",
      trname);
    }
  else
    {
    addr->transport_return = DEFER;
    addr->message = string_sprintf("Failed to send message from %s "
      "transport (%d)", trname, rc);
    goto END_OFF;
    }

/* Log the sending of the message if successful and required. If the file
fails to open, it's hard to know what to do. We cannot write to the Exim
log from here, since we may be running under an unprivileged uid. We don't
want to fail the delivery, since the message has been successfully sent. For
the moment, ignore open failures. Write the log entry as a single write() to a
file opened for appending, in order to avoid interleaving of output from
different processes. The log_buffer can be used exactly as for main log
writing. */

if (logfile)
  {
  int log_fd = Uopen(logfile, O_WRONLY|O_APPEND|O_CREAT, ob->mode);
  if (log_fd >= 0)
    {
    gstring gs = { .size = LOG_BUFFER_SIZE, .ptr = 0, .s = log_buffer }, *g = &gs;

    /* Use taint-unchecked routines for writing into log_buffer, trusting
    that we'll never expand it. */

    DEBUG(D_transport) debug_printf("logging message details\n");
    g = string_fmt_append_f(g, SVFMT_TAINT_NOCHK, "%s\n", tod_stamp(tod_log));
    if (from)
      g = string_fmt_append_f(g, SVFMT_TAINT_NOCHK, "  From: %s\n", from);
    if (to)
      g = string_fmt_append_f(g, SVFMT_TAINT_NOCHK, "  To: %s\n", to);
    if (cc)
      g = string_fmt_append_f(g, SVFMT_TAINT_NOCHK, "  Cc: %s\n", cc);
    if (bcc)
      g = string_fmt_append_f(g, SVFMT_TAINT_NOCHK, "  Bcc: %s\n", bcc);
    if (subject)
      g = string_fmt_append_f(g, SVFMT_TAINT_NOCHK, "  Subject: %s\n", subject);
    if (headers)
      g = string_fmt_append_f(g, SVFMT_TAINT_NOCHK, "  %s\n", headers);
    if(write(log_fd, g->s, g->ptr) != g->ptr || close(log_fd))
      DEBUG(D_transport) debug_printf("Problem writing log file %s for %s "
        "transport\n", logfile, trname);
    }
  else DEBUG(D_transport) debug_printf("Failed to open log file %s for %s "
    "transport: %s\n", logfile, trname, strerror(errno));
  }

END_OFF:
if (dbm_file) exim_dbclose(dbm_file);
if (cache_fd > 0) (void)close(cache_fd);

DEBUG(D_transport) debug_printf("%s transport succeeded\n", trname);

return FALSE;
}




# ifdef DYNLOOKUP
#  define autoreply_transport_info _transport_info
# endif

transport_info autoreply_transport_info = {
.drinfo = {
  .driver_name =	US"autoreply",
  .options =		autoreply_transport_options,
  .options_count =	&autoreply_transport_options_count,
  .options_block =	&autoreply_transport_option_defaults,
  .options_len =	sizeof(autoreply_transport_options_block),
  .init =		autoreply_transport_init,
# ifdef DYNLOOKUP
  .dyn_magic =		TRANSPORT_MAGIC,
# endif
  },
.code =		autoreply_transport_entry,
.tidyup =	NULL,
.closedown =	NULL,
.local =	TRUE
};

#endif	/*!MACRO_PREDEF*/
#endif	/*TRANSPORT_AUTOREPOL*/
/* End of transport/autoreply.c */
/* vi: aw ai sw=2
*/
