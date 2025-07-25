/*************************************************
*     Exim - an Internet mail transport agent    *
*************************************************/

/* Copyright (c) The Exim Maintainers 2020 - 2024 */
/* Copyright (c) University of Cambridge 1995 - 2018 */
/* See the file NOTICE for conditions of use and distribution. */
/* SPDX-License-Identifier: GPL-2.0-or-later */

/* A number of functions for driving outgoing SMTP calls. */


#include "exim.h"
#include "transports/smtp.h"



/*************************************************
*           Find an outgoing interface           *
*************************************************/

/* This function is called from the smtp transport and also from the callout
code in verify.c. Its job is to expand a string to get a list of interfaces,
and choose a suitable one (IPv4 or IPv6) for the outgoing address.

Arguments:
  istring    string interface setting, may be NULL, meaning "any", in
               which case the function does nothing
  host_af    AF_INET or AF_INET6 for the outgoing IP address
  addr       the mail address being handled (for setting errors)
  interface  point this to the interface if there is one defined,
	     otherwise leave untouched
  msg        to add to any error message

Returns:     TRUE on success, FALSE on failure, with error message
               set in addr and transport_return set to PANIC
*/

BOOL
smtp_get_interface(const uschar * istring, int host_af, address_item * addr,
  const uschar ** interface, const uschar * msg)
{
const uschar * expint;
uschar * iface;
int sep = 0;

if (!istring) return TRUE;

if (!(expint = expand_string(istring)))
  {
  if (f.expand_string_forcedfail) return TRUE;
  addr->transport_return = PANIC;
  addr->message = string_sprintf("failed to expand \"interface\" "
      "option for %s: %s", msg, expand_string_message);
  return FALSE;
  }

if (is_tainted(expint))
  {
  log_write(0, LOG_MAIN|LOG_PANIC,
    "attempt to use tainted value '%s' from '%s' for interface",
    expint, istring);
  addr->transport_return = PANIC;
  addr->message = string_sprintf("failed to expand \"interface\" "
      "option for %s: configuration error", msg);
  return FALSE;
  }

Uskip_whitespace(&expint);
if (!*expint) return TRUE;

while ((iface = string_nextinlist(&expint, &sep, NULL, 0)))
  {
  int if_af = string_is_ip_address(iface, NULL);
  if (if_af == 0)
    {
    addr->transport_return = PANIC;
    addr->message = string_sprintf("%q is not a valid IP "
      "address for the \"interface\" option for %s",
      iface, msg);
    return FALSE;
    }

  if ((if_af == 4 ? AF_INET : AF_INET6) == host_af)
    break;
  }

*interface = iface;
return TRUE;
}



/*************************************************
*           Find an outgoing port                *
*************************************************/

/* This function is called from the smtp transport and also from the callout
code in verify.c. Its job is to find a port number. Note that getservbyname()
produces the number in network byte order.

Arguments:
  rstring     raw (unexpanded) string representation of the port
  addr        the mail address being handled (for setting errors)
  msg         for adding to error message

Returns:      port on success, -1 on failure, with error message set
                in addr, and transport_return set to PANIC
*/

int
smtp_get_port(const uschar * rstring, address_item * addr, const uschar * msg)
{
const uschar * pstring = expand_string(rstring);
int port;

if (!pstring)
  {
  addr->transport_return = PANIC;
  addr->message = string_sprintf("failed to expand %q (\"port\" option) "
    "for %s: %s", rstring, msg, expand_string_message);
  return FALSE;
  }

if (isdigit(*pstring))
  {
  uschar * end;
  port = Ustrtol(pstring, &end, 0);
  if (end != pstring + Ustrlen(pstring))
    {
    addr->transport_return = PANIC;
    addr->message = string_sprintf("invalid port number for %s: %s", msg,
      pstring);
    return -1;
    }
  }

else
  {
  struct servent *smtp_service = getservbyname(CCS pstring, "tcp");
  if (!smtp_service)
    {
    addr->transport_return = PANIC;
    addr->message = string_sprintf("TCP port %q is not defined for %s",
      pstring, msg);
    return -1;
    }
  port = ntohs(smtp_service->s_port);
  }

return port;
}




#ifdef TCP_FASTOPEN
/* Try to record if TFO was attmepted and if it was successfully used.  */

void
tfo_out_check(int sock)
{
static BOOL done_once = FALSE;

if (done_once) return;

# ifdef __FreeBSD__
  {
  struct tcp_info tinfo;
  socklen_t len = sizeof(tinfo);

  /* A getsockopt TCP_FASTOPEN unfortunately returns "was-used" for a TFO/R as
  well as a TFO/C.  Use what we can of the Linux hack below; reliability issues
  ditto. */
  switch (tcp_out_fastopen)
    {
    case TFO_ATTEMPTED_NODATA:
      if (  getsockopt(sock, IPPROTO_TCP, TCP_INFO, &tinfo, &len) == 0
	 && tinfo.tcpi_state == TCPS_SYN_SENT
	 && tinfo.__tcpi_unacked > 0
	 )
	{
	DEBUG(D_transport|D_v)
	 debug_printf("TCP_FASTOPEN tcpi_unacked %d\n", tinfo.__tcpi_unacked);
	tcp_out_fastopen = TFO_USED_NODATA;
	}
      break;
    /*
    case TFO_ATTEMPTED_DATA:
    case TFO_ATTEMPTED_DATA:
	 if (tinfo.tcpi_options & TCPI_OPT_SYN_DATA)   XXX no equvalent as of 12.2
    */
    }

  switch (tcp_out_fastopen)
    {
    case TFO_ATTEMPTED_DATA:	tcp_out_fastopen = TFO_USED_DATA; break;
    default: break; /* compiler quietening */
    }

  done_once = TRUE;
  }

# else	/* Linux & Apple */
#  if defined(TCP_INFO) && defined(EXIM_HAVE_TCPI_UNACKED)
  {
  struct tcp_info tinfo;
  socklen_t len = sizeof(tinfo);

  switch (tcp_out_fastopen)
    {
    /* This is a somewhat dubious detection method; totally undocumented so
    likely to fail in future kernels.  There seems to be no documented way.
    What we really want to know is if the server sent smtp-banner data before
    our ACK of his SYN,ACK hit him.  What this (possibly?) detects is whether we
    sent a TFO cookie with our SYN, as distinct from a TFO request.  This gets a
    false-positive when the server key is rotated; we send the old one (which
    this test sees) but the server returns the new one and does not send its
    SMTP banner before we ACK his SYN,ACK.  To force that rotation case:
     '# echo -n "00000000-00000000-00000000-0000000"
		  >/proc/sys/net/ipv4/tcp_fastopen_key'
    The kernel seems to be counting unack'd packets. */

    case TFO_ATTEMPTED_NODATA:
      if (  getsockopt(sock, IPPROTO_TCP, TCP_INFO, &tinfo, &len) == 0
	 && tinfo.tcpi_state == TCP_SYN_SENT
	 && tinfo.tcpi_unacked > 1
	 )
	{
	DEBUG(D_transport|D_v)
	  debug_printf("TCP_FASTOPEN tcpi_unacked %d\n", tinfo.tcpi_unacked);
	tcp_out_fastopen = TFO_USED_NODATA;
	}
      done_once = TRUE;
      break;

      /* When called after waiting for received data we should be able
      to tell if data we sent was accepted. Keep checking until we have hit
      ESTABLISHED state. */

    case TFO_ATTEMPTED_DATA:
      if (getsockopt(sock, IPPROTO_TCP, TCP_INFO, &tinfo, &len) != 0)
	done_once = TRUE;
      else if (  tinfo.tcpi_state == TCP_ESTABLISHED
	      || tinfo.tcpi_state == TCP_FIN_WAIT1
	      || tinfo.tcpi_state == TCP_FIN_WAIT2
	      )
	{
	if (tinfo.tcpi_options & TCPI_OPT_SYN_DATA)
	  {
	  DEBUG(D_transport|D_v) debug_printf("TFO: data was acked\n");
	  tcp_out_fastopen = TFO_USED_DATA;
	  }
	else
	  {
	  DEBUG(D_transport|D_v) debug_printf("TFO: had to retransmit\n");
	  tcp_out_fastopen = TFO_NOT_USED;
	  }
	done_once = TRUE;
	}
      break;

    default: break; /* compiler quietening */
    }
  }
#  endif
# endif	/* Linux & Apple */
}
#endif


/* Create and bind a socket, given the connect-args.
Update those with the state.  Return the fd, or -1 with errno set.
*/

int
smtp_boundsock(smtp_connect_args * sc)
{
transport_instance * tb = sc->tblock;
smtp_transport_options_block * ob = tb->drinst.options_block;
const uschar * dscp = ob->dscp;
int sock, dscp_value, dscp_level, dscp_option;

if ((sock = ip_socket(SOCK_STREAM, sc->host_af)) < 0)
  return -1;

/* Set TCP_NODELAY; Exim does its own buffering. */

if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, US &on, sizeof(on)))
  HDEBUG(D_transport|D_acl|D_v)
    debug_printf_indent("failed to set NODELAY: %s ", strerror(errno));

/* Set DSCP value, if we can. For now, if we fail to set the value, we don't
bomb out, just log it and continue in default traffic class. */

GET_OPTION("dscp");
if (dscp && dscp_lookup(dscp, sc->host_af, &dscp_level, &dscp_option, &dscp_value))
  {
  HDEBUG(D_transport|D_acl|D_v)
    debug_printf_indent("DSCP %q=%x ", dscp, dscp_value);
  if (setsockopt(sock, dscp_level, dscp_option, &dscp_value, sizeof(dscp_value)) < 0)
    HDEBUG(D_transport|D_acl|D_v)
      debug_printf_indent("failed to set DSCP: %s ", strerror(errno));
  /* If the kernel supports IPv4 and IPv6 on an IPv6 socket, we need to set the
  option for both; ignore failures here */
  if (sc->host_af == AF_INET6 &&
      dscp_lookup(dscp, AF_INET, &dscp_level, &dscp_option, &dscp_value))
    (void) setsockopt(sock, dscp_level, dscp_option, &dscp_value, sizeof(dscp_value));
  }

/* Bind to a specific interface if requested. Caller must ensure the interface
is the same type (IPv4 or IPv6) as the outgoing address. */

if (sc->interface)
  {
  union sockaddr_46 interface_sock;
  EXIM_SOCKLEN_T size = sizeof(interface_sock);

  if (  ip_bind(sock, sc->host_af, sc->interface, 0) < 0
     || getsockname(sock, (struct sockaddr *) &interface_sock, &size) < 0
     )
    {
    HDEBUG(D_transport|D_acl|D_v)
      debug_printf_indent("unable to bind outgoing SMTP call to %s: %s\n", sc->interface,
	strerror(errno));
    close(sock);
    return -1;
    }
  sending_ip_address = host_ntoa(-1, &interface_sock, NULL, &sending_port);
  }

sc->sock = sock;
return sock;
}


/* Arguments:
  sc		details for making connection: host, af, interface, transport
  timeout	timeout value or 0
  early_data	if non-NULL, idempotent data to be sent -
		preferably in the TCP SYN segment
	      Special case: non-NULL but with NULL blob.data - caller is
	      client-data-first (eg. TLS-on-connect) and a lazy-TCP-connect is
	      acceptable.

Returns:      connected socket number, or -1 with errno set
*/

int
smtp_sock_connect(smtp_connect_args * sc, int timeout, const blob * early_data)
{
const smtp_transport_options_block * ob = sc->tblock->drinst.options_block;
int sock;
int save_errno = 0;
const blob * fastopen_blob = NULL;

#ifndef DISABLE_EVENT
deliver_host_address = sc->host->address;
deliver_host_port = sc->host->port;
if (event_raise(sc->tblock->event_action, US"tcp:connect", NULL, &errno)) return -1;
#endif

if (  (sock = sc->sock) < 0
   && (sock = smtp_boundsock(sc)) < 0)
  save_errno = errno;
sc->sock = -1;

/* Connect to the remote host, and add keepalive to the socket before returning
it, if requested.  If the build supports TFO, request it - and if the caller
requested some early-data then include that in the TFO request.  If there is
early-data but no TFO support, send it after connecting. */

if (!save_errno)
  {
#ifdef TCP_FASTOPEN
  /* See if TCP Fast Open usable.  Default is a traditional 3WHS connect */
  expand_level++;
  if (verify_check_given_host(CUSS &ob->hosts_try_fastopen, sc->host) == OK)
    {
    if (!early_data)
      fastopen_blob = &tcp_fastopen_nodata;	/* TFO, with no data */
    else if (early_data->data)
      fastopen_blob = early_data;		/* TFO, with data */
# ifdef TCP_FASTOPEN_CONNECT
    else
      {						/* expecting client data */
      DEBUG(D_transport|D_acl|D_v) debug_printf(" set up lazy-connect\n");
      setsockopt(sock, IPPROTO_TCP, TCP_FASTOPEN_CONNECT, US &on, sizeof(on));
      /* fastopen_blob = NULL;		 lazy TFO, triggered by data write */
      tcp_out_fastopen = TFO_ATTEMPTED_DATA;
      }
# endif
    }
  expand_level--;
#endif

  if (ip_connect(sock, sc->host_af, sc->host->address, sc->host->port, timeout, fastopen_blob) < 0)
    save_errno = errno;
  else if (early_data && !fastopen_blob && early_data->data && early_data->len)
    {
    /* We had some early-data to send, but couldn't do TFO */
    HDEBUG(D_transport|D_acl|D_v)
      debug_printf("sending %ld nonTFO early-data\n", (long)early_data->len);

#ifdef TCP_QUICKACK_notdef
    (void) setsockopt(sock, IPPROTO_TCP, TCP_QUICKACK, US &off, sizeof(off));
#endif
    if (send(sock, early_data->data, early_data->len, 0) < 0)
      save_errno = errno;
    }
#ifdef TCP_QUICKACK_notdef
  /* Under TFO (with openssl & pipe-conn; testcase 4069, as of
  5.10.8-100.fc32.x86_64) this seems to be inop.
  Perhaps overwritten when we (client) go -> ESTABLISHED on seeing the 3rd-ACK?
  For that case, added at smtp_reap_banner(). */
  (void) setsockopt(sock, IPPROTO_TCP, TCP_QUICKACK, US &off, sizeof(off));
#endif
  }

if (!save_errno)
  {
  union sockaddr_46 interface_sock;
  EXIM_SOCKLEN_T size = sizeof(interface_sock);

  /* Both bind() and connect() succeeded, and any early-data */

  HDEBUG(D_transport|D_acl|D_v) debug_printf_indent("connected\n");
  if (getsockname(sock, (struct sockaddr *)(&interface_sock), &size) == 0)
    sending_ip_address = host_ntoa(-1, &interface_sock, NULL, &sending_port);
  else
    {
    log_write(0, LOG_MAIN | ((errno == ECONNRESET)? 0 : LOG_PANIC),
      "getsockname() failed: %s", strerror(errno));
    close(sock);
    return -1;
    }

  if (ob->keepalive) ip_keepalive(sock, sc->host->address, TRUE);
#ifdef TCP_FASTOPEN
  tfo_out_check(sock);
#endif
  return sock;
  }

/* Either bind() or connect() failed */

HDEBUG(D_transport|D_acl|D_v)
  {
  debug_printf_indent(" sock_connect failed: %s", CUstrerror(save_errno));
  if (save_errno == ETIMEDOUT)
    debug_printf(" (timeout=%s)", readconf_printtime(timeout));
  debug_printf("\n");
  }
(void)close(sock);
errno = save_errno;
return -1;
}





void
smtp_port_for_connect(host_item * host, int tpt_port)
{
if (host->port == PORT_NONE)
  host->port = tpt_port;    /* Set the port actually used */

else HDEBUG(D_transport|D_acl|D_v) if (tpt_port != host->port)
  debug_printf_indent("Transport port=%d replaced by host-specific port=%d\n",
		      tpt_port, host->port);
}


/*************************************************
*           Connect to remote host               *
*************************************************/

/* Create a socket, and connect it to a remote host. IPv6 addresses are
detected by checking for a colon in the address. AF_INET6 is defined even on
non-IPv6 systems, to enable the code to be less messy. However, on such systems
host->address will always be an IPv4 address.

Arguments:
  sc	      details for making connection: host, af, interface, transport
  early_data  if non-NULL, data to be sent - preferably in the TCP SYN segment
	      Special case: non-NULL but with NULL blob.data - caller is
	      client-data-first (eg. TLS-on-connect) and a lazy-TCP-connect is
	      acceptable.

Returns:      connected socket number, or -1 with errno set
*/

int
smtp_connect(smtp_connect_args * sc, const blob * early_data)
{
smtp_transport_options_block * ob = sc->ob;

callout_address = string_sprintf("[%s]:%d", sc->host->address, sc->host->port);

HDEBUG(D_transport|D_acl|D_v)
  {
  gstring * g = sc->interface
    ? string_fmt_append(NULL, " from %s", sc->interface)
    : string_get(20);
#ifdef SUPPORT_SOCKS
  if (ob->socks_proxy) g = string_cat(g, US" (proxy option set)");
#endif
  debug_printf_indent("Connecting to %s %s%Y ...\n",
		      sc->host->name, callout_address, g);
  }

/* Create and connect the socket */

#ifdef SUPPORT_SOCKS
GET_OPTION("socks_proxy");
if (ob->socks_proxy)
  {
  if (!(ob->socks_proxy = expand_string(ob->socks_proxy)))
    {
    log_write(0, LOG_MAIN|LOG_PANIC, "Bad expansion for socks_proxy in %s",
      sc->tblock->drinst.name);
    return -1;
    }
  if (*ob->socks_proxy)
    return socks_sock_connect(sc, early_data);
  }
#endif

return smtp_sock_connect(sc, ob->connect_timeout, early_data);
}


/*************************************************
*        Flush outgoing command buffer           *
*************************************************/

/* This function is called only from smtp_write_command() below. It flushes
the buffer of outgoing commands. There is more than one in the buffer only when
pipelining.

Argument:
  outblock   the SMTP output block
  mode	     further data expected, or plain

Returns:     TRUE if OK, FALSE on error, with errno set
*/

static BOOL
flush_buffer(smtp_outblock * outblock, int mode)
{
int n = outblock->ptr - outblock->buffer, rc;
BOOL more = mode == SCMD_MORE;
client_conn_ctx * cctx;
const uschar * where;

HDEBUG(D_transport|D_acl) debug_printf_indent("cmd buf flush %d bytes%s\n", n,
  more ? " (more expected)" : "");

if (!(cctx = outblock->cctx))
  {
  log_write(0, LOG_MAIN|LOG_PANIC, "null conn-context pointer");
  errno = 0;
  return FALSE;
  }

#ifndef DISABLE_TLS
where = US"tls_write";
if (cctx->tls_ctx)		/*XXX have seen a null cctx here, rvfy sending QUIT, hence check above */
  rc = tls_write(cctx->tls_ctx, outblock->buffer, n, more);
else
#endif

  {
  if (outblock->conn_args)
    {
    blob early_data = { .data = outblock->buffer, .len = n };

    /* We ignore the more-flag if we're doing a connect with early-data, which
    means we won't get BDAT+data. A pity, but wise due to the idempotency
    requirement: TFO with data can, in rare cases, replay the data to the
    receiver. */

    where = US"smtp_connect";
    if (  (cctx->sock = smtp_connect(outblock->conn_args, &early_data))
       < 0)
      return FALSE;
    outblock->conn_args = NULL;
    rc = n;
    }
  else
    {
    where = US"send";
    rc = send(cctx->sock, outblock->buffer, n,
#ifdef MSG_MORE
	      more ? MSG_MORE : 0
#else
	      0
#endif
	     );

#if defined(__linux__)
    /* This is a workaround for a current linux kernel bug: as of
    5.6.8-200.fc31.x86_64  small (<MSS) writes get delayed by about 200ms,
    This is despite NODELAY being active.
    https://bugzilla.redhat.com/show_bug.cgi?id=1803806 */

    where = US"cork";
    if (!more)
      setsockopt(cctx->sock, IPPROTO_TCP, TCP_CORK, &off, sizeof(off));
#endif
    }
  }

if (rc <= 0)
  {
  HDEBUG(D_transport|D_acl) debug_printf_indent("%s (fd %d) failed: %s\n",
    where, cctx->sock, strerror(errno));
  return FALSE;
  }

outblock->ptr = outblock->buffer;
outblock->cmd_count = 0;
return TRUE;
}



/*************************************************
*             Write SMTP command                 *
*************************************************/

/* The formatted command is left in big_buffer so that it can be reflected in
any error message.

Arguments:
  sx	     SMTP connection, contains buffer for pipelining, and socket
  mode       buffer, write-with-more-likely, write
  format     a format, starting with one of
             of HELO, MAIL FROM, RCPT TO, DATA, BDAT, ".", or QUIT.
	     If NULL, flush pipeline buffer only.
  ...        data for the format

Returns:     0 if command added to pipelining buffer, with nothing transmitted
            +n if n commands transmitted (may still have buffered the new one)
            -1 on error, with errno set
*/

int
smtp_write_command(void * sx, int mode, const char * format, ...)
{
smtp_outblock * outblock = &((smtp_context *)sx)->outblock;
int rc = 0;

if (format)
  {
  gstring gs = { .size = big_buffer_size, .ptr = 0, .s = big_buffer };
  va_list ap;

  /* Use taint-unchecked routines for writing into big_buffer, trusting that
  we'll never expand the results.  Actually, the error-message use - leaving
  the results in big_buffer for potential later use - is uncomfortably distant.
  XXX Would be better to assume all smtp commands are short, use normal pool
  alloc rather than big_buffer, and another global for the data-for-error. */

  va_start(ap, format);
  if (!string_vformat(&gs, SVFMT_TAINT_NOCHK, CS format, ap))
    log_write_die(0, LOG_MAIN, "overlong write_command in outgoing "
      "SMTP");
  va_end(ap);

  if (gs.ptr > outblock->buffersize)
    log_write_die(0, LOG_MAIN, "overlong write_command in outgoing "
      "SMTP");

  if (gs.ptr > outblock->buffersize - (outblock->ptr - outblock->buffer))
    {
    rc = outblock->cmd_count;                 /* flush resets */
    if (!flush_buffer(outblock, SCMD_FLUSH)) return -1;
    }

  Ustrncpy(outblock->ptr, gs.s, gs.ptr);
  outblock->ptr += gs.ptr;
  outblock->cmd_count++;
  gs.ptr -= 2; string_from_gstring(&gs); /* remove \r\n for error message */

  /* We want to hide the actual data sent in AUTH transactions from reflections
  and logs. While authenticating, a flag is set in the outblock to enable this.
  The AUTH command itself gets any data flattened. Other lines are flattened
  completely. */

  if (outblock->authenticating)
    {
    uschar * p = big_buffer;
    if (Ustrncmp(big_buffer, "AUTH ", 5) == 0)
      {
      p += 5;
      Uskip_whitespace(&p);
      Uskip_nonwhite(&p);
      Uskip_whitespace(&p);
      }
    while (*p) *p++ = '*';
    }

  smtp_debug_cmd(big_buffer, mode);
  }

if (mode != SCMD_BUFFER)
  {
  rc += outblock->cmd_count;                /* flush resets */
  if (!flush_buffer(outblock, mode)) return -1;
  }

return rc;
}



/*************************************************
*          Read one line of SMTP response        *
*************************************************/

/* This function reads one line of SMTP response from the server host. This may
not be a complete response - it could be just part of a multiline response. We
have to use a buffer for incoming packets, because when pipelining or using
LMTP, there may well be more than one response in a single packet. This
function is called only from the one that follows.

Arguments:
  inblock   the SMTP input block (contains holding buffer, socket, etc.)
  buffer    where to put the line
  size      space available for the line
  timelimit deadline for reading the lime, seconds past epoch

Returns:    length of a line that has been put in the buffer
            -1 otherwise, with errno set, and inblock->ptr adjusted
*/

static int
read_response_line(smtp_inblock * inblock, uschar * buffer, int size,
  time_t timelimit)
{
uschar * p = buffer, * ptr = inblock->ptr;
const uschar * ptrend = inblock->ptrend;
client_conn_ctx * cctx = inblock->cctx;

/* Loop for reading multiple packets or reading another packet after emptying
a previously-read one. */

for (;;)
  {
  int rc;

  /* If there is data in the input buffer left over from last time, copy
  characters from it until the end of a line, at which point we can return,
  having removed any whitespace (which will include CR) at the end of the line.
  The rules for SMTP say that lines end in CRLF, but there are have been cases
  of hosts using just LF, and other MTAs are reported to handle this, so we
  just look for LF. If we run out of characters before the end of a line,
  carry on to read the next incoming packet. */

  while (ptr < ptrend)
    {
    int c = *ptr++;
    if (c == '\n')
      {
      while (p > buffer && isspace(p[-1])) p--;
      *p = 0;
      inblock->ptr = ptr;
      return p - buffer;
      }
    *p++ = c;
    if (--size < 4)
      {
      *p = 0;                     /* Leave malformed line for error message */
      errno = ERRNO_SMTPFORMAT;
      inblock->ptr = ptr;
      return -1;
      }
    }

  /* Need to read a new input packet. */

  if((rc = ip_recv(cctx, inblock->buffer, inblock->buffersize, timelimit)) <= 0)
    {
    DEBUG(D_deliver|D_transport|D_acl|D_v)
      debug_printf_indent(errno ? "  SMTP(%s)<<\n" : "  SMTP(closed)<<\n",
	strerror(errno));
    break;
    }

  /* Another block of data has been successfully read. Set up the pointers
  and let the loop continue. */

  ptrend = inblock->ptrend = inblock->buffer + rc;
  ptr = inblock->buffer;
  DEBUG(D_transport|D_acl) debug_printf_indent("read response data: size=%d\n", rc);
  }

/* Get here if there has been some kind of recv() error; errno is set, but we
ensure that the result buffer is empty before returning. */

inblock->ptr = inblock->ptrend = inblock->buffer;
*buffer = 0;
return -1;
}





/*************************************************
*              Read SMTP response                *
*************************************************/

/* This function reads an SMTP response with a timeout, and returns the
response in the given buffer, as a string. A multiline response will contain
newline characters between the lines. The function also analyzes the first
digit of the reply code and returns FALSE if it is not acceptable. FALSE is
also returned after a reading error. In this case buffer[0] will be zero, and
the error code will be in errno.

Arguments:
  sx        the SMTP connection (contains input block with holding buffer,
		socket, etc.)
  buffer    where to put the response
  size      the size of the buffer
  okdigit   the expected first digit of the response
  timeout   the timeout to use, in seconds

Returns:    TRUE if a valid, non-error response was received; else FALSE
*/
/*XXX could move to smtp transport; no other users */

BOOL
smtp_read_response(void * sx0, uschar * buffer, int size, int okdigit,
   int timeout)
{
smtp_context * sx = sx0;
uschar * ptr = buffer;
int count = 0;
time_t timelimit = time(NULL) + timeout;
BOOL yield = FALSE;

errno = 0;  /* Ensure errno starts out zero */
buffer[0] = '\0';

#ifndef DISABLE_PIPE_CONNECT
if (sx->pending_BANNER || sx->pending_EHLO)
  {
  int rc;
  if ((rc = smtp_reap_early_pipe(sx, &count)) != OK)
    {
    DEBUG(D_transport) debug_printf("failed reaping pipelined cmd responsess\n");
    if (rc == DEFER) errno = ERRNO_TLSFAILURE;
    goto out;
    }
  }
#endif

/* This is a loop to read and concatenate the lines that make up a multi-line
response. */

for (;;)
  {
  if ((count = read_response_line(&sx->inblock, ptr, size, timelimit)) < 0)
    return FALSE;

  HDEBUG(D_transport|D_acl|D_v)
    debug_printf_indent("  %s %s\n", ptr == buffer ? "SMTP<<" : "      ", ptr);

  /* Check the format of the response: it must start with three digits; if
  these are followed by a space or end of line, the response is complete. If
  they are followed by '-' this is a multi-line response and we must look for
  another line until the final line is reached. The only use made of multi-line
  responses is to pass them back as error messages. We therefore just
  concatenate them all within the buffer, which should be large enough to
  accept any reasonable number of lines. */

  if (count < 3 ||
     !isdigit(ptr[0]) ||
     !isdigit(ptr[1]) ||
     !isdigit(ptr[2]) ||
     (ptr[3] != '-' && ptr[3] != ' ' && ptr[3] != 0))
    {
    errno = ERRNO_SMTPFORMAT;    /* format error */
    goto out;
    }

  /* If the line we have just read is a terminal line, line, we are done.
  Otherwise more data has to be read. */

  if (ptr[3] != '-') break;

  /* Move the reading pointer upwards in the buffer and insert \n between the
  components of a multiline response. Space is left for this by read_response_
  line(). */

  ptr += count;
  *ptr++ = '\n';
  size -= count + 1;
  }

#ifdef TCP_FASTOPEN
tfo_out_check(sx->cctx.sock);
#endif

/* Return a value that depends on the SMTP return code. On some systems a
non-zero value of errno has been seen at this point, so ensure it is zero,
because the caller of this function looks at errno when FALSE is returned, to
distinguish between an unexpected return code and other errors such as
timeouts, lost connections, etc. */

errno = 0;
yield = buffer[0] == okdigit;

out:
  smtp_debug_resp(buffer);
  return yield;
}

/* End of smtp_out.c */
/* vi: aw ai sw=2
*/
