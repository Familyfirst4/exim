/*************************************************
*     Exim - an Internet mail transport agent    *
*************************************************/

/* Copyright (c) The Exim Maintainers 2020 - 2025 */
/* Copyright (c) University of Cambridge 1995 - 2018 */
/* See the file NOTICE for conditions of use and distribution. */
/* SPDX-License-Identifier: GPL-2.0-or-later */

/* All the global variables are defined together in this one module, so
that they are easy to find. */

#include "exim.h"


/* Generic options for auths, all of which live inside auth_instance
data blocks and hence have the opt_public flag set. */

optionlist optionlist_auths[] = {
  { "client_condition", opt_stringptr | opt_public,
                 OPT_OFF(auth_instance, client_condition) },
  { "client_set_id", opt_stringptr | opt_public,
                 OPT_OFF(auth_instance, set_client_id) },
  { "driver",        opt_stringptr | opt_public,
                 OPT_OFF(auth_instance, drinst.driver_name) },
  { "public_name",   opt_stringptr | opt_public,
                 OPT_OFF(auth_instance, public_name) },
  { "server_advertise_condition", opt_stringptr | opt_public,
                 OPT_OFF(auth_instance, advertise_condition)},
  { "server_condition", opt_stringptr | opt_public,
                 OPT_OFF(auth_instance, server_condition) },
  { "server_debug_print", opt_stringptr | opt_public,
                 OPT_OFF(auth_instance, server_debug_string) },
  { "server_mail_auth_condition", opt_stringptr | opt_public,
                 OPT_OFF(auth_instance, mail_auth_condition) },
  { "server_set_id", opt_stringptr | opt_public,
                 OPT_OFF(auth_instance, set_id) }
};

int     optionlist_auths_size = nelem(optionlist_auths);

/* An empty host aliases list. */

uschar *no_aliases             = NULL;


/* For comments on these variables, see globals.h. I'm too idle to
duplicate them here... */

#ifdef EXIM_PERL
uschar *opt_perl_startup       = NULL;
BOOL    opt_perl_at_start      = FALSE;
BOOL    opt_perl_started       = FALSE;
BOOL    opt_perl_taintmode     = FALSE;
#endif

#ifdef EXPAND_DLFUNC
tree_node *dlobj_anchor        = NULL;
#endif

#ifdef LOOKUP_IBASE
uschar *ibase_servers          = NULL;
#endif

#ifdef LOOKUP_LDAP
uschar *eldap_ca_cert_dir      = NULL;
uschar *eldap_ca_cert_file     = NULL;
uschar *eldap_cert_file        = NULL;
uschar *eldap_cert_key         = NULL;
uschar *eldap_cipher_suite     = NULL;
uschar *eldap_default_servers  = NULL;
uschar *eldap_require_cert     = NULL;
int     eldap_version          = -1;
BOOL    eldap_start_tls        = FALSE;
#endif

#ifdef LOOKUP_MYSQL
uschar *mysql_servers          = NULL;
#endif

#ifdef LOOKUP_ORACLE
uschar *oracle_servers         = NULL;
#endif

#ifdef LOOKUP_PGSQL
uschar *pgsql_servers          = NULL;
#endif

#ifdef LOOKUP_REDIS
uschar *redis_servers          = NULL;
#endif

#ifdef LOOKUP_SQLITE
uschar *sqlite_dbfile	       = NULL;
int     sqlite_lock_timeout    = 5;
#endif

#ifdef SUPPORT_MOVE_FROZEN_MESSAGES
BOOL    move_frozen_messages   = FALSE;
#endif

/* These variables are outside the TLS #ifdef because it keeps the code less
cluttered in several places (e.g. during logging) if we can always refer to
them. Also, the tls_ variables are now always visible.  Note that these are
only used for smtp connections, not for service-daemon access. */

#ifndef MACRO_PREDEF
tls_support tls_in = {
 .active =		{.sock = -1}
 /* all other elements zero */
};
tls_support tls_out = {
 .active =		{.sock = -1},
 /* all other elements zero */
};
#endif

uschar *dsn_envid              = NULL;
int     dsn_ret                = 0;
const pcre2_code  *regex_DSN         = NULL;
uschar *dsn_advertise_hosts    = NULL;

#ifndef DISABLE_TLS
BOOL    gnutls_compat_mode     = FALSE;
BOOL    gnutls_allow_auto_pkcs11 = FALSE;
uschar *hosts_require_alpn     = NULL;
uschar *openssl_options        = NULL;
const pcre2_code *regex_STARTTLS     = NULL;
uschar *tls_advertise_hosts    = US"*";
uschar *tls_alpn	       = US"smtp:esmtp";
uschar *tls_certificate        = NULL;
uschar *tls_crl                = NULL;
/* This default matches NSS DH_MAX_P_BITS value at current time (2012), because
that's the interop problem which has been observed: GnuTLS suggesting a higher
bit-count as "NORMAL" (2432) and Thunderbird dropping connection. */
int     tls_dh_max_bits        = 2236;
uschar *tls_dhparam            = NULL;
# ifdef EXPERIMENTAL_TLS_EARLY_BANNER
uschar *tls_early_banner_hosts = US"";
# endif
uschar *tls_eccurve            = US"auto";
# ifndef DISABLE_OCSP
uschar *tls_ocsp_file          = NULL;
# endif
uschar *tls_on_connect_ports   = NULL;
uschar *tls_privatekey         = NULL;
BOOL    tls_remember_esmtp     = FALSE;
uschar *tls_require_ciphers    = NULL;
# ifndef DISABLE_TLS_RESUME
uschar *tls_resumption_hosts   = NULL;
# endif
uschar *tls_try_verify_hosts   = NULL;
uschar *tls_verify_certificates= US"system";
uschar *tls_verify_hosts       = NULL;
int     tls_watch_fd	       = -1;
time_t  tls_watch_trigger_time = (time_t)0;
#else	/*DISABLE_TLS*/
uschar *tls_advertise_hosts    = NULL;
#endif

#ifndef DISABLE_PRDR
/* Per Recipient Data Response variables */
BOOL    prdr_enable            = FALSE;
BOOL    prdr_requested         = FALSE;
const pcre2_code *regex_PRDR         = NULL;
#endif

#ifdef SUPPORT_I18N
const pcre2_code *regex_UTF8         = NULL;
#endif

/* Input-reading functions for messages, so we can use special ones for
incoming TCP/IP. The defaults use stdin. We never need these for any
stand-alone tests. */

#if !defined(STAND_ALONE) && !defined(MACRO_PREDEF)
int	(*lwr_receive_getc)(unsigned)	= stdin_getc;
uschar * (*lwr_receive_getbuf)(unsigned *) = NULL;
int	(*lwr_receive_ungetc)(int)	= stdin_ungetc;
BOOL	(*lwr_receive_hasc)(void)	= stdin_hasc;

int	(*receive_getc)(unsigned) 	= stdin_getc;
uschar * (*receive_getbuf)(unsigned *) 	= NULL;		/* can return NULL */
void	(*receive_get_cache)(unsigned)	= NULL;
BOOL	(*receive_hasc)(void)		= stdin_hasc;
int	(*receive_ungetc)(int)    	= stdin_ungetc;
int	(*receive_feof)(void)     	= stdin_feof;
int	(*receive_ferror)(void)   	= stdin_ferror;
#endif


/* List of per-address expansion variables for clearing and saving/restoring
when verifying one address while routing/verifying another. We have to have
the size explicit, because it is referenced from more than one module. */

const uschar **address_expansions[ADDRESS_EXPANSIONS_COUNT] = {
  CUSS &deliver_address_data,
  CUSS &deliver_domain,
  CUSS &deliver_domain_data,
  CUSS &deliver_domain_orig,
  CUSS &deliver_domain_parent,
  CUSS &deliver_localpart,
  CUSS &deliver_localpart_data,
  CUSS &deliver_localpart_orig,
  CUSS &deliver_localpart_parent,
  CUSS &deliver_localpart_prefix,
  CUSS &deliver_localpart_suffix,
  CUSS (uschar **)(&deliver_recipients),
  CUSS &deliver_host,
  CUSS &deliver_home,
  CUSS &address_file,
  CUSS &address_pipe,
  CUSS &self_hostname,
  NULL };

int address_expansions_count = nelem(address_expansions);

/******************************************************************************/
/* General global variables.  Boolean flags are done as a group
so that only one bit each is needed, packed, for all those we never
need to take a pointer - and only a char for the rest.
This means a struct, unfortunately since it clutters the sourcecode. */

struct global_flags f =
{
	.acl_temp_details       = FALSE,
	.active_local_from_check = FALSE,
	.active_local_sender_retain = FALSE,
	.address_test_mode      = FALSE,
	.admin_user             = FALSE,
	.allow_auth_unadvertised= FALSE,
	.allow_unqualified_recipient = TRUE,    /* For local messages */
	.allow_unqualified_sender = TRUE,       /* Reset for SMTP */
	.authentication_local   = FALSE,

	.background_daemon      = TRUE,
	.bdat_readers_wanted    = FALSE,

	.chunking_offered       = FALSE,
	.config_changed         = FALSE,
	.continue_more          = FALSE,

	.daemon_listen          = FALSE,
	.daemon_scion           = FALSE,
	.debug_daemon           = FALSE,
	.deliver_firsttime      = FALSE,
	.deliver_force          = FALSE,
	.deliver_freeze         = FALSE,
	.deliver_force_thaw     = FALSE,
	.deliver_manual_thaw    = FALSE,
	.deliver_selectstring_regex = FALSE,
	.deliver_selectstring_sender_regex = FALSE,
	.disable_callout_flush  = FALSE,
	.disable_delay_flush    = FALSE,
	.disable_logging        = FALSE,
#ifndef DISABLE_DKIM
	.dkim_disable_verify      = FALSE,
	.dkim_init_done           = FALSE,
#endif
#ifdef SUPPORT_DMARC
	.dmarc_has_been_checked  = FALSE,
	.dmarc_disable_verify    = FALSE,
	.dmarc_enable_forensic   = FALSE,
#endif
	.dont_deliver           = FALSE,
	.dot_ends               = TRUE,

	.enable_dollar_recipients = FALSE,
	.expand_string_forcedfail = FALSE,

	.filter_running         = FALSE,

	.header_rewritten       = FALSE,
	.helo_verified          = FALSE,
	.helo_verify_failed     = FALSE,
	.host_checking_callout  = FALSE,
	.host_find_failed_syntax= FALSE,

	.inetd_wait_mode        = FALSE,
	.is_inetd               = FALSE,

	.local_error_message    = FALSE,
	.log_testing_mode       = FALSE,

#ifdef WITH_CONTENT_SCAN
	.no_mbox_unspool        = FALSE,
#endif
	.no_multiline_responses = FALSE,
	.notifier_socket_en	= TRUE,

	.parse_allow_group      = FALSE,
	.parse_found_group      = FALSE,
	.pipelining_enable      = TRUE,
#if defined(SUPPORT_PROXY) || defined(SUPPORT_SOCKS)
	.proxy_session_failed   = FALSE,
#endif

	.queue_2stage           = FALSE,
	.queue_only_policy      = FALSE,
	.queue_run_local        = FALSE,
	.queue_running          = FALSE,
	.queue_smtp             = FALSE,

	.really_exim            = TRUE,
	.receive_call_bombout   = FALSE,
	.recipients_discarded   = FALSE,
	.running_in_test_harness = FALSE,

	.search_find_defer      = FALSE,
	.sender_address_forced  = FALSE,
	.sender_host_notsocket  = FALSE,
	.sender_host_unknown    = FALSE,
	.sender_local           = FALSE,
	.sender_name_forced     = FALSE,
	.sender_set_untrusted   = FALSE,
	.smtp_authenticated     = FALSE,
#ifndef DISABLE_PIPE_CONNECT
	.smtp_in_early_pipe_advertised = FALSE,
	.smtp_in_early_pipe_no_auth = FALSE,
	.smtp_in_early_pipe_used = FALSE,
#endif
	.smtp_in_pipelining_advertised = FALSE,
	.smtp_in_pipelining_used = FALSE,
	.smtp_in_quit		= FALSE,
	.spool_file_wireformat  = FALSE,
	.submission_mode        = FALSE,
	.suppress_local_fixups  = FALSE,
	.suppress_local_fixups_default = FALSE,
	.synchronous_delivery   = FALSE,
	.system_filtering       = FALSE,

	.taint_check_slow       = FALSE,
	.testsuite_delays	= TRUE,
	.tcp_fastopen_ok        = FALSE,
	.tcp_in_fastopen        = FALSE,
	.tcp_in_fastopen_data   = FALSE,
	.tcp_in_fastopen_logged = FALSE,
	.tcp_out_fastopen_logged= FALSE,
	.timestamps_utc         = FALSE,
	.transport_filter_timed_out = FALSE,
	.trusted_caller         = FALSE,
	.trusted_config         = TRUE,
};

/******************************************************************************/
/* These are the flags which are either variables or mainsection options,
so an address is needed for access, or are exported to local_scan. */

BOOL    accept_8bitmime        = TRUE; /* deliberately not RFC compliant */
BOOL    allow_domain_literals  = FALSE;
BOOL    allow_mx_to_ip         = FALSE;
BOOL    allow_utf8_domains     = FALSE;
BOOL    authentication_failed  = FALSE;

BOOL    bounce_return_body     = TRUE;
BOOL    bounce_return_message  = TRUE;
BOOL    check_rfc2047_length   = TRUE;
BOOL    commandline_checks_require_admin = FALSE;

#ifdef EXPERIMENTAL_DCC
BOOL    dcc_direct_add_header  = FALSE;
#endif
BOOL    debug_store            = FALSE;
BOOL    delivery_date_remove   = TRUE;
BOOL    deliver_drop_privilege = FALSE;
#ifdef ENABLE_DISABLE_FSYNC
BOOL    disable_fsync          = FALSE;
#endif
BOOL    disable_ipv6           = FALSE;
BOOL    dns_csa_use_reverse    = TRUE;
BOOL    drop_cr                = FALSE;         /* No longer used */

BOOL    envelope_to_remove     = TRUE;
BOOL    exim_gid_set           = TRUE;          /* This gid is always set */
BOOL    exim_uid_set           = TRUE;          /* This uid is always set */
BOOL    extract_addresses_remove_arguments = TRUE;

BOOL    host_checking          = FALSE;
BOOL    host_lookup_deferred   = FALSE;
BOOL    host_lookup_failed     = FALSE;
BOOL    ignore_fromline_local  = FALSE;

BOOL    local_from_check       = TRUE;
BOOL    local_sender_retain    = FALSE;
BOOL    log_timezone           = FALSE;
BOOL    message_body_newlines  = FALSE;
BOOL    message_logs           = TRUE;
#ifdef SUPPORT_I18N
BOOL    message_smtputf8       = FALSE;
#endif
BOOL    mua_wrapper            = FALSE;

BOOL    preserve_message_logs  = FALSE;
BOOL    print_topbitchars      = FALSE;
BOOL    prod_requires_admin    = TRUE;
#if defined(SUPPORT_PROXY) || defined(SUPPORT_SOCKS) || defined(EXPERIMENTAL_XCLIENT)
BOOL    proxy_session          = FALSE;
#endif

#ifndef DISABLE_QUEUE_RAMP
BOOL    queue_fast_ramp		= TRUE;
#endif
BOOL    queue_list_requires_admin = TRUE;
BOOL    queue_only             = FALSE;
BOOL    queue_only_load_latch  = TRUE;
BOOL    queue_only_override    = TRUE;
BOOL    queue_run_in_order     = FALSE;
BOOL    recipients_max_reject  = FALSE;
BOOL    return_path_remove     = TRUE;

BOOL    smtp_batched_input     = FALSE;
BOOL    sender_helo_dnssec     = FALSE;
BOOL    sender_host_dnssec     = FALSE;
BOOL    smtp_accept_keepalive  = TRUE;
BOOL    smtp_check_spool_space = TRUE;
BOOL    smtp_enforce_sync      = TRUE;
BOOL    smtp_etrn_serialize    = TRUE;
BOOL    smtp_input             = FALSE;
BOOL    smtp_return_error_details = FALSE;
BOOL    split_spool_directory  = FALSE;
BOOL    spool_wireformat       = FALSE;
BOOL    strict_acl_vars        = FALSE;
BOOL    strip_excess_angle_brackets = FALSE;
BOOL    strip_trailing_dot     = FALSE;
BOOL    syslog_duplication     = TRUE;
BOOL    syslog_pid             = TRUE;
BOOL    syslog_timestamp       = TRUE;
BOOL    system_filter_gid_set  = FALSE;
BOOL    system_filter_uid_set  = FALSE;

BOOL    tcp_nodelay            = TRUE;
BOOL    write_rejectlog        = TRUE;

/******************************************************************************/

header_line *acl_added_headers = NULL;
tree_node *acl_anchor          = NULL;
uschar *acl_arg[9]             = {NULL, NULL, NULL, NULL, NULL,
                                  NULL, NULL, NULL, NULL};
int     acl_narg               = 0;

int     acl_level	       = 0;

uschar *acl_not_smtp           = NULL;
#ifdef WITH_CONTENT_SCAN
uschar *acl_not_smtp_mime      = NULL;
#endif
uschar *acl_not_smtp_start     = NULL;
uschar *acl_removed_headers    = NULL;
uschar *acl_smtp_atrn          = NULL;
uschar *acl_smtp_auth          = NULL;
uschar *acl_smtp_connect       = NULL;
uschar *acl_smtp_data          = NULL;
#ifndef DISABLE_PRDR
uschar *acl_smtp_data_prdr     = US"accept";
#endif
#ifndef DISABLE_DKIM
uschar *acl_smtp_dkim          = NULL;
#endif
uschar *acl_smtp_etrn          = NULL;
uschar *acl_smtp_expn          = NULL;
uschar *acl_smtp_helo          = NULL;
uschar *acl_smtp_mail          = NULL;
uschar *acl_smtp_mailauth      = NULL;
#ifdef WITH_CONTENT_SCAN
uschar *acl_smtp_mime          = NULL;
#endif
uschar *acl_smtp_notquit       = NULL;
uschar *acl_smtp_predata       = NULL;
uschar *acl_smtp_quit          = NULL;
uschar *acl_smtp_rcpt          = NULL;
uschar *acl_smtp_starttls      = NULL;
uschar *acl_smtp_vrfy          = NULL;
#ifndef DISABLE_WELLKNOWN
uschar *acl_smtp_wellknown     = NULL;
#endif

tree_node *acl_var_c           = NULL;
tree_node *acl_var_m           = NULL;
uschar *acl_verify_message     = NULL;
string_item *acl_warn_logged   = NULL;
int acl_where		       = ACL_WHERE_UNKNOWN;

/* Names of SMTP places for use in ACL error messages, and corresponding SMTP
error codes (only those used) */

uschar *acl_wherenames[]       = { [ACL_WHERE_RCPT] =		US"RCPT",
                                   [ACL_WHERE_MAIL] =		US"MAIL",
                                   [ACL_WHERE_PREDATA] =	US"PREDATA",
                                   [ACL_WHERE_MIME] =		US"MIME",
                                   [ACL_WHERE_DKIM] =		US"DKIM",
                                   [ACL_WHERE_DATA] =		US"DATA",
#ifndef DISABLE_PRDR
                                   [ACL_WHERE_PRDR] =		US"PRDR",
#endif
                                   [ACL_WHERE_NOTSMTP] =	US"non-SMTP",
                                   [ACL_WHERE_ATRN] =		US"ATRN",
                                   [ACL_WHERE_AUTH] =		US"AUTH",
                                   [ACL_WHERE_CONNECT] =	US"connection",
                                   [ACL_WHERE_ETRN] =		US"ETRN",
                                   [ACL_WHERE_EXPN] =		US"EXPN",
                                   [ACL_WHERE_HELO] =		US"EHLO or HELO",
                                   [ACL_WHERE_MAILAUTH] =	US"MAILAUTH",
                                   [ACL_WHERE_NOTSMTP_START] =	US"non-SMTP-start",
                                   [ACL_WHERE_NOTQUIT] =	US"NOTQUIT",
                                   [ACL_WHERE_QUIT] =		US"QUIT",
                                   [ACL_WHERE_STARTTLS] =	US"STARTTLS",
                                   [ACL_WHERE_VRFY] =		US"VRFY",
#ifndef DISABLE_WELLKNOWN
				   [ACL_WHERE_WELLKNOWN] =	US"WELLKNOWN",
#endif
				   [ACL_WHERE_DELIVERY] =	US"delivery",
				   [ACL_WHERE_UNKNOWN] =	US"unknown"
                                 };

uschar *acl_wherecodes[]       = { [ACL_WHERE_RCPT] =	US"550",
                                   [ACL_WHERE_MAIL] =	US"550",
                                   [ACL_WHERE_PREDATA] = US"550",
                                   [ACL_WHERE_MIME] =	US"550",
                                   [ACL_WHERE_DKIM] =	US"550",
                                   [ACL_WHERE_DATA] =	US"550",
#ifndef DISABLE_PRDR
                                   [ACL_WHERE_PRDR] =	US"550",
#endif
                                   [ACL_WHERE_ATRN] =	US"450",
                                   [ACL_WHERE_AUTH] =	US"503",
                                   [ACL_WHERE_CONNECT] = US"550",
                                   [ACL_WHERE_ETRN] =	US"458",
                                   [ACL_WHERE_EXPN] =	US"550",
                                   [ACL_WHERE_HELO] =	US"550",
                                   [ACL_WHERE_STARTTLS] = US"550",
#ifndef DISABLE_WELLKNOWN
                                   [ACL_WHERE_WELLKNOWN] =US"550",
#endif
                                   [ACL_WHERE_VRFY] =	US"252",
                                 };

uschar *add_environment        = NULL;
address_item  *addr_duplicate  = NULL;

address_item address_defaults = {    /* All unmentioned elements 0/NULL/FALSE */
  .uid =		(uid_t)(-1),
  .gid =		(gid_t)(-1),
  .mode =		-1,
  .basic_errno =	ERRNO_UNKNOWNERROR,
  .return_file =	-1,
  .special_action =	SPECIAL_NONE,
  .transport_return =	DEFER,
};

const uschar *address_file           = NULL;
const uschar *address_pipe           = NULL;
tree_node *addresslist_anchor  = NULL;
int     addresslist_count      = 0;
gid_t  *admin_groups           = NULL;

const uschar *atrn_domains     = NULL;
const uschar *atrn_host        = NULL;
const uschar *atrn_mode        = NULL;

uschar *authenticated_fail_id  = NULL;
uschar *authenticated_id       = NULL;
uschar *authenticated_sender   = NULL;
auth_instance  *auths          = NULL;
uschar *auth_advertise_hosts   = US"*";
auth_instance auth_defaults    = {   /* All unmentioned elements 0/NULL/FALSE */
    0
};

uschar *auth_defer_msg         = US"reason not recorded";
uschar *auth_defer_user_msg    = US"";
const uschar *auth_vars[AUTH_VARS];
uschar *authenticator_name     = NULL;
int     auto_thaw              = 0;
#ifdef WITH_CONTENT_SCAN
int     av_failed              = FALSE;	/* boolean but accessed as vtype_int*/
uschar *av_scanner             = US"sophie:/var/run/sophie";  /* AV scanner */
#endif

#if BASE_62 == 62
uschar *base62_chars=
    US"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
#else
uschar *base62_chars= US"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
#endif

uschar *bi_command             = NULL;
uschar *big_buffer             = NULL;
int     big_buffer_size        = BIG_BUFFER_SIZE;
#ifdef EXPERIMENTAL_BRIGHTMAIL
uschar *bmi_alt_location       = NULL;
uschar *bmi_base64_tracker_verdict = NULL;
uschar *bmi_base64_verdict     = NULL;
uschar *bmi_config_file        = US"/opt/brightmail/etc/brightmail.cfg";
int     bmi_deliver            = 1;
int     bmi_run                = 0;
uschar *bmi_verdicts           = NULL;
#endif
int     bsmtp_transaction_linecount = 0;
int     body_8bitmime          = 0;
int     body_linecount         = 0;
int     body_zerocount         = 0;
uschar *bounce_message_file    = NULL;
uschar *bounce_message_text    = NULL;
const uschar *bounce_recipient = NULL;
int     bounce_return_linesize_limit = 998;
int     bounce_return_size_limit = 100*1024;
uschar *bounce_sender_authentication = NULL;

uschar *callout_address        = NULL;
int     callout_cache_domain_positive_expire = 7*24*60*60;
int     callout_cache_domain_negative_expire = 3*60*60;
int     callout_cache_positive_expire = 24*60*60;
int     callout_cache_negative_expire = 2*60*60;
uschar *callout_random_local_part = US"$primary_hostname-$tod_epoch-testing";
uschar *check_dns_names_pattern= US"(?i)^(?>(?(1)\\.|())[^\\W](?>[a-z0-9/_-]*[^\\W])?)+(\\.?)$";
int     check_log_inodes       = 100;
int_eximarith_t check_log_space = 10*1024;	/* 10K Kbyte == 10MB */
int     check_spool_inodes     = 100;
int_eximarith_t check_spool_space = 10*1024;	/* 10K Kbyte == 10MB */

uschar *chunking_advertise_hosts = US"*";
unsigned chunking_datasize     = 0;
unsigned chunking_data_left    = 0;
chunking_state_t chunking_state= CHUNKING_NOT_OFFERED;
const pcre2_code *regex_CHUNKING     = NULL;

#ifndef DISABLE_ESMTP_LIMITS
const pcre2_code *regex_LIMITS        = NULL;
#endif

uschar *client_authenticator   = NULL;
uschar *client_authenticated_id = NULL;
uschar *client_authenticated_sender = NULL;
#ifndef DISABLE_CLIENT_CMD_LOG
gstring *client_cmd_log        = NULL;
#endif
int     clmacro_count          = 0;
uschar *clmacros[MAX_CLMACROS];
FILE   *config_file            = NULL;
const uschar *config_filename  = NULL;
unsigned config_lineno         = 0;
#ifdef CONFIGURE_GROUP
gid_t   config_gid             = CONFIGURE_GROUP;
#else
gid_t   config_gid             = 0;
#endif
const uschar * config_main_filelist = US CONFIGURE_FILE
                         "\0<-----------Space to patch configure_filename->";
uschar *config_main_filename   = NULL;
uschar *config_main_directory  = NULL;

#ifdef CONFIGURE_OWNER
uid_t   config_uid             = CONFIGURE_OWNER;
#else
uid_t   config_uid             = 0;
#endif

const uschar *connection_id    = NULL;
int     connection_max_messages= -1;
unsigned continue_flags	       = 0;
#ifndef DISABLE_ESMTP_LIMITS
unsigned continue_limit_mail   = 0;
unsigned continue_limit_rcpt   = 0;
unsigned continue_limit_rcptdom= 0;
int	continue_fd	       = -1;
uschar *continue_proxy_cipher  = NULL;
BOOL    continue_proxy_dane    = FALSE;
uschar *continue_proxy_sni     = NULL;
const uschar *continue_hostname      = NULL;
const uschar *continue_host_address  = NULL;
int     continue_host_port     = 0;
uschar  continue_next_id[MESSAGE_ID_LENGTH +1] = {[0]='\0'};
unsigned continue_sequence     = 1;
const uschar *continue_transport = NULL;
#ifndef COMPILE_UTILITY
open_db *continue_retry_db     = NULL;
open_db *continue_wait_db      = NULL;
#endif
#endif

uschar *csa_status             = NULL;
cut_t   cutthrough = {
  .callout_hold_only =	FALSE,				/* verify-only: normal delivery */
  .delivery =		FALSE,				/* when to attempt */
  .tpt_sender =		FALSE,				/* use tpt's sender */
  .defer_pass =		FALSE,				/* on defer: spool locally */
  .is_tls =		FALSE,				/* not a TLS conn yet */
  .cctx =		{.sock = -1},			/* open connection */
  .nrcpt =		0,				/* number of addresses */
};

int	daemon_notifier_fd     = -1;
uschar *daemon_smtp_port       = US"smtp";
int     daemon_startup_retries = 9;
int     daemon_startup_sleep   = 30;

#ifdef EXPERIMENTAL_DCC
uschar *dcc_header             = NULL;
uschar *dcc_result             = NULL;
uschar *dccifd_address         = US"/usr/local/dcc/var/dccifd";
uschar *dccifd_options         = US"header";
#endif

int     debug_fd               = -1;
FILE   *debug_file             = NULL;
int     debug_notall[]         = {
  Di_memory,
  Di_noutf8,
  -1
};
bit_table debug_options[]      = { /* must be in alphabetical order and use
				 only the enum values from macros.h */
  BIT_TABLE(D, acl),
  BIT_TABLE(D, all),
  BIT_TABLE(D, auth),
  BIT_TABLE(D, deliver),
  BIT_TABLE(D, dns),
  BIT_TABLE(D, dnsbl),
  BIT_TABLE(D, exec),
  BIT_TABLE(D, expand),
  BIT_TABLE(D, filter),
  BIT_TABLE(D, hints_lookup),
  BIT_TABLE(D, host_lookup),
  BIT_TABLE(D, ident),
  BIT_TABLE(D, interface),
  BIT_TABLE(D, lists),
  BIT_TABLE(D, load),
  BIT_TABLE(D, local_scan),
  BIT_TABLE(D, lookup),
  BIT_TABLE(D, memory),
  BIT_TABLE(D, noutf8),
  BIT_TABLE(D, pid),
  BIT_TABLE(D, process_info),
  BIT_TABLE(D, queue_run),
  BIT_TABLE(D, receive),
  BIT_TABLE(D, resolver),
  BIT_TABLE(D, retry),
  BIT_TABLE(D, rewrite),
  BIT_TABLE(D, route),
  BIT_TABLE(D, timestamp),
  BIT_TABLE(D, tls),
  BIT_TABLE(D, transport),
  BIT_TABLE(D, uid),
  BIT_TABLE(D, verify),
};
int      debug_options_count	= nelem(debug_options);
uschar   debuglog_name[LOG_NAME_SIZE] = {0};
unsigned debug_pretrigger_bsize	= 0;
uschar * debug_pretrigger_buf	= NULL;
unsigned int debug_selector	= 0;
BOOL	 debug_startup		= FALSE;

int     delay_warning[DELAY_WARNING_SIZE] = { DELAY_WARNING_SIZE, 1, 24*60*60 };
uschar *delay_warning_condition=
  US"${if or {"
            "{ !eq{$h_list-id:$h_list-post:$h_list-subscribe:}{} }"
            "{ match{$h_precedence:}{(?i)bulk|list|junk} }"
            "{ match{$h_auto-submitted:}{(?i)auto-generated|auto-replied} }"
            "} {no}{yes}}";
uschar *deliver_address_data   = NULL;
int     deliver_datafile       = -1;
const uschar *deliver_domain   = NULL;
uschar *deliver_domain_data    = NULL;
const uschar *deliver_domain_orig = NULL;
const uschar *deliver_domain_parent = NULL;
time_t  deliver_frozen_at      = 0;
uschar *deliver_home           = NULL;
const uschar *deliver_host     = NULL;
const uschar *deliver_host_address = NULL;
int     deliver_host_port      = 0;
uschar *deliver_in_buffer      = NULL;
ino_t   deliver_inode          = 0;
const uschar *deliver_localpart= NULL;
uschar *deliver_localpart_data = NULL;
const uschar *deliver_localpart_orig = NULL;
const uschar *deliver_localpart_parent = NULL;
const uschar *deliver_localpart_prefix = NULL;
const uschar *deliver_localpart_prefix_v = NULL;
const uschar *deliver_localpart_suffix = NULL;
const uschar *deliver_localpart_suffix_v = NULL;
uschar *deliver_out_buffer     = NULL;
int     deliver_queue_load_max = -1;
address_item  *deliver_recipients = NULL;
uschar *deliver_selectstring   = NULL;
uschar *deliver_selectstring_sender = NULL;

uschar *dns_again_means_nonexist = NULL;
int     dns_csa_search_limit   = 5;
int	dns_cname_loops	       = 1;
#ifdef SUPPORT_DANE
int     dns_dane_ok            = -1;
#endif
uschar *dns_ipv4_lookup        = NULL;
int     dns_retrans            = 0;
int     dns_retry              = 0;
int     dns_dnssec_ok          = -1; /* <0 = not coerced */
uschar *dns_trust_aa           = NULL;
int     dns_use_edns0          = -1; /* <0 = not coerced */
uschar *dnslist_domain         = NULL;
uschar *dnslist_matched        = NULL;
uschar *dnslist_text           = NULL;
uschar *dnslist_value          = NULL;
tree_node *domainlist_anchor   = NULL;
int     domainlist_count       = 0;
const uschar *driver_srcfile   = NULL;
int     driver_srcline	       = 0;
uschar *dsn_from               = US DEFAULT_DSN_FROM;
unsigned int dtrigger_selector = 0;

int     errno_quota            = ERRNO_QUOTA;
uschar *errors_copy            = NULL;
int     error_handling         = ERRORS_SENDER;
uschar *errors_reply_to        = NULL;
int     errors_sender_rc       = EXIT_FAILURE;
#ifndef DISABLE_EVENT
uschar *event_action           = NULL;	/* expansion for delivery events */
const uschar *event_data       = NULL;	/* auxiliary data variable for event */
int     event_defer_errno      = 0;
const uschar *event_name       = NULL;	/* event name variable */
#endif


gid_t   exim_gid               = EXIM_GID;
uschar *exim_path              = US BIN_DIRECTORY "/exim"
                        "\0<---------------Space to patch exim_path->";
uid_t   exim_uid               = EXIM_UID;
int     expand_level	       = 0;		/* Nesting depth, indent for debug */
int     expand_forbid          = 0;
int     expand_nlength[EXPAND_MAXN+1];
int     expand_nmax            = -1;
const uschar *expand_nstring[EXPAND_MAXN+1];
uschar *expand_string_message;
uschar *extra_local_interfaces = NULL;

int     fake_response          = OK;
uschar *fake_response_text     = US"Your message has been rejected but is "
                                   "being kept for evaluation.\nIf it was a "
                                   "legitimate message, it may still be "
                                   "delivered to the target recipient(s).";
int     filter_n[FILTER_VARIABLE_COUNT];
int     filter_sn[FILTER_VARIABLE_COUNT];
int     filter_test            = FTEST_NONE;
const uschar * filter_test_sfile = NULL;
const uschar * filter_test_ufile = NULL;
uschar *filter_thisaddress     = NULL;
int     finduser_retries       = 0;
uid_t   fixed_never_users[]    = { FIXED_NEVER_USERS };
uschar *freeze_tell            = NULL;
uschar *freeze_tell_config     = NULL;
uschar *fudged_queue_times     = US"";

uschar *gecos_name             = NULL;
uschar *gecos_pattern          = NULL;
rewrite_rule  *global_rewrite_rules = NULL;

volatile sig_atomic_t had_command_timeout = 0;
volatile sig_atomic_t had_command_sigterm = 0;
volatile sig_atomic_t had_data_timeout    = 0;
volatile sig_atomic_t had_data_sigint     = 0;
const uschar *headers_charset  = US HEADERS_CHARSET;
int     header_insert_maxlen   = 64 * 1024;
header_line  *header_last      = NULL;
header_line  *header_list      = NULL;
int     header_maxsize         = HEADER_MAXSIZE;
int     header_line_maxsize    = 0;

header_name header_names[] = {
  /* name		len	allow_resent	htype */
  { US"bcc",            3,	TRUE,		htype_bcc },
  { US"cc",             2,	TRUE,		htype_cc },
  { US"date",           4,	TRUE,		htype_date },
  { US"delivery-date", 13,	FALSE,		htype_delivery_date },
  { US"envelope-to",   11,	FALSE,		htype_envelope_to },
  { US"from",           4,	TRUE,		htype_from },
  { US"message-id",    10,	TRUE,		htype_id },
  { US"received",       8,	FALSE,		htype_received },
  { US"reply-to",       8,	FALSE,		htype_reply_to },
  { US"return-path",   11,	FALSE,		htype_return_path },
  { US"sender",         6,	TRUE,		htype_sender },
  { US"subject",        7,	FALSE,		htype_subject },
  { US"to",             2,	TRUE,		htype_to }
};

int header_names_size          = nelem(header_names);

uschar *helo_accept_junk_hosts = NULL;
uschar *helo_allow_chars       = US"";
uschar *helo_lookup_domains    = US"@ : @[]";
uschar *helo_try_verify_hosts  = NULL;
uschar *helo_verify_hosts      = NULL;
const uschar *hex_digits       = CUS"0123456789abcdef";
uschar *hold_domains           = NULL;
uschar *host_data              = NULL;
uschar *host_lookup            = NULL;
uschar *host_lookup_order      = US"bydns:byaddr";
uschar *host_lookup_msg        = US"";
int     host_number            = 0;
uschar *host_number_string     = NULL;
uschar *host_reject_connection = NULL;
uschar *hosts_connection_nolog = NULL;
#ifdef SUPPORT_PROXY
uschar *hosts_proxy            = NULL;
#endif
uschar *hosts_treat_as_local   = NULL;
uschar *hosts_require_helo     = US"*";
#ifdef EXPERIMENTAL_XCLIENT
uschar *hosts_xclient	       = NULL;
#endif
tree_node *hostlist_anchor     = NULL;
int     hostlist_count         = 0;


int     ignore_bounce_errors_after = 10*7*24*60*60;  /* 10 weeks */
uschar *ignore_fromline_hosts  = NULL;
int     inetd_wait_timeout     = -1;
uschar *initial_cwd            = NULL;
uschar *interface_address      = NULL;
int     interface_port         = -1;
uschar *iterate_item           = NULL;

int     journal_fd             = -1;

uschar *keep_environment       = NULL;

int     keep_malformed         = 4*24*60*60;    /* 4 days */

uschar *eldap_dn               = NULL;
const uschar *letter_digit_hyphen_dot =
    US"abcdefghijklmnopqrstuvwxyz"
      ".-0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
#ifndef DISABLE_ESMTP_LIMITS
uschar *limits_advertise_hosts = US"*";
#endif
int     load_average           = -2;
uschar *local_from_prefix      = NULL;
uschar *local_from_suffix      = NULL;

#if HAVE_IPV6
uschar *local_interfaces       = US"<; ::0 ; 0.0.0.0";
#else
uschar *local_interfaces       = US"0.0.0.0";
#endif

#ifdef HAVE_LOCAL_SCAN
uschar *local_scan_data        = NULL;
int     local_scan_timeout     = 5*60;
#endif
gid_t   local_user_gid         = (gid_t)(-1);
uid_t   local_user_uid         = (uid_t)(-1);

tree_node *localpartlist_anchor= NULL;
int     localpartlist_count    = 0;
uschar *log_buffer             = NULL;

int     log_default[]          = { /* for initializing log_selector */
  Li_acl_warn_skipped,
  Li_connection_reject,
  Li_delay_delivery,
  Li_dkim,
  Li_dnslist_defer,
  Li_etrn,
  Li_host_lookup_failed,
  Li_lost_incoming_connection,
  Li_outgoing_interface, /* see d_log_interface in deliver.c */
  Li_msg_id,
  Li_queue_run,
  Li_queue_time_exclusive,
  Li_rejected_header,
  Li_retry_defer,
  Li_sender_verify_fail,
  Li_size_reject,
  Li_skip_delivery,
  Li_smtp_confirmation,
  Li_tls_certificate_verified,
  Li_tls_cipher,
  -1
};

uschar *log_file_path          = US LOG_FILE_PATH
                           "\0<--------------Space to patch log_file_path->";

int     log_notall[]           = {
  -1
};
bit_table log_options[]        = { /* must be in alphabetical order,
				with definitions from enum logbit. */
  BIT_TABLE(L, 8bitmime),
  BIT_TABLE(L, acl_warn_skipped),
  BIT_TABLE(L, address_rewrite),
  BIT_TABLE(L, all),
  BIT_TABLE(L, all_parents),
  BIT_TABLE(L, arguments),
  BIT_TABLE(L, connection_id),
  BIT_TABLE(L, connection_reject),
  BIT_TABLE(L, delay_delivery),
  BIT_TABLE(L, deliver_time),
  BIT_TABLE(L, delivery_size),
#ifndef DISABLE_DKIM
  BIT_TABLE(L, dkim),
  BIT_TABLE(L, dkim_verbose),
#endif
  BIT_TABLE(L, dnslist_defer),
  BIT_TABLE(L, dnssec),
  BIT_TABLE(L, etrn),
  BIT_TABLE(L, host_lookup_failed),
  BIT_TABLE(L, ident_timeout),
  BIT_TABLE(L, incoming_interface),
  BIT_TABLE(L, incoming_port),
  BIT_TABLE(L, lost_incoming_connection),
  BIT_TABLE(L, millisec),
  BIT_TABLE(L, msg_id),
  BIT_TABLE(L, msg_id_created),
  BIT_TABLE(L, outgoing_interface),
  BIT_TABLE(L, outgoing_port),
  BIT_TABLE(L, pid),
  BIT_TABLE(L, pipelining),
  BIT_TABLE(L, protocol_detail),
#if defined(SUPPORT_PROXY) || defined(SUPPORT_SOCKS)
  BIT_TABLE(L, proxy),
#endif
  BIT_TABLE(L, queue_run),
  BIT_TABLE(L, queue_time),
  BIT_TABLE(L, queue_time_exclusive),
  BIT_TABLE(L, queue_time_overall),
  BIT_TABLE(L, receive_time),
  BIT_TABLE(L, received_recipients),
  BIT_TABLE(L, received_sender),
  BIT_TABLE(L, rejected_header),
  { US"rejected_headers", Li_rejected_header },
  BIT_TABLE(L, retry_defer),
  BIT_TABLE(L, return_path_on_delivery),
  BIT_TABLE(L, sender_on_delivery),
  BIT_TABLE(L, sender_verify_fail),
  BIT_TABLE(L, size_reject),
  BIT_TABLE(L, skip_delivery),
  BIT_TABLE(L, smtp_confirmation),
  BIT_TABLE(L, smtp_connection),
  BIT_TABLE(L, smtp_incomplete_transaction),
  BIT_TABLE(L, smtp_mailauth),
  BIT_TABLE(L, smtp_no_mail),
  BIT_TABLE(L, smtp_protocol_error),
  BIT_TABLE(L, smtp_syntax_error),
  BIT_TABLE(L, subject),
  BIT_TABLE(L, tls_certificate_verified),
  BIT_TABLE(L, tls_cipher),
  BIT_TABLE(L, tls_on_connect),
  BIT_TABLE(L, tls_peerdn),
  BIT_TABLE(L, tls_resumption),
  BIT_TABLE(L, tls_sni),
  BIT_TABLE(L, unknown_in_list),
};
int     log_options_count      = nelem(log_options);

const uschar *log_ports	       = NULL;
int     log_reject_target      = 0;
unsigned int log_selector[log_selector_size]; /* initialized in main() */
uschar *log_selector_string    = NULL;
FILE   *log_stderr             = NULL;
uschar *login_sender_address   = NULL;
uschar *lookup_dnssec_authenticated = NULL;
int     lookup_open_max        = 25;
uschar *lookup_value           = NULL;

macro_item *macros_user        = NULL;
uschar *mailstore_basename     = NULL;
#ifdef WITH_CONTENT_SCAN
uschar *malware_name           = NULL;  /* Virus Name */
#endif
int     max_received_linelength= 0;
int     max_username_length    = 0;
int     message_age            = 0;
uschar *message_body           = NULL;
uschar *message_body_end       = NULL;
int     message_body_size      = 0;
int     message_body_visible   = 500;
int     message_ended          = END_NOTSTARTED;
uschar *message_headers        = NULL;
uschar *message_id;
uschar *message_id_domain      = NULL;
uschar *message_id_text        = NULL;
uschar  message_id_option[MESSAGE_ID_LENGTH + 3];
uschar *message_id_external;
int     message_linecount      = 0;
int     message_size           = 0;
uschar *message_size_limit     = US"50M";
#ifdef SUPPORT_I18N
int     message_utf8_downconvert = 0;	/* -1 ifneeded; 0 never; 1 always */
#endif
uschar  message_subdir[2]      = { 0, 0 };
const uschar *message_reference= NULL;

/* MIME ACL expandables */
#ifdef WITH_CONTENT_SCAN
int     mime_anomaly_level     = 0;
const uschar *mime_anomaly_text      = NULL;
uschar *mime_boundary          = NULL;
uschar *mime_charset           = NULL;
uschar *mime_content_description = NULL;
uschar *mime_content_disposition = NULL;
uschar *mime_content_id        = NULL;
unsigned int mime_content_size = 0;
uschar *mime_content_transfer_encoding = NULL;
uschar *mime_content_type      = NULL;
uschar *mime_decoded_filename  = NULL;
uschar *mime_filename          = NULL;
int     mime_is_multipart      = 0;
int     mime_is_coverletter    = 0;
int     mime_is_rfc822         = 0;
int     mime_part_count        = -1;
#endif

uid_t  *never_users            = NULL;
uschar *notifier_socket        = US"$spool_directory/" NOTIFIER_SOCKET_NAME ;

const int on                   = 1;	/* for setsockopt */
const int off                  = 0;

uid_t   original_euid;
gid_t   originator_gid;
uschar *originator_login       = NULL;
uschar *originator_name        = NULL;
uid_t   originator_uid;
uschar *override_local_interfaces = NULL;
const uschar *override_pid_file_path = NULL;

BOOL    panic_coredump	       = FALSE;
pcre2_general_context * pcre_gen_ctx = NULL;
pcre2_compile_context * pcre_gen_cmp_ctx = NULL;
pcre2_match_context * pcre_gen_mtc_ctx = NULL;
pcre2_general_context * pcre_mlc_ctx = NULL;
pcre2_compile_context * pcre_mlc_cmp_ctx = NULL;

uschar *percent_hack_domains   = NULL;
const uschar *pid_file_path    = US PID_FILE_PATH
                           "\0<--------------Space to patch pid_file_path->";
#ifndef DISABLE_PIPE_CONNECT
uschar *pipe_connect_advertise_hosts = US"*";
#endif
uschar *pipelining_advertise_hosts = US"*";
uschar *primary_hostname       = NULL;
uschar *process_info;
int     process_info_len       = 0;
uschar *process_log_path       = NULL;
const uschar *process_purpose  = US"fresh-exec";

#if defined(SUPPORT_PROXY) || defined(SUPPORT_SOCKS) || defined(EXPERIMENTAL_XCLIENT)
uschar *proxy_external_address = NULL;
unsigned proxy_external_port   = 0;
uschar *proxy_local_address    = NULL;
unsigned proxy_local_port      = 0;
int     proxy_protocol_timeout = 3;
#endif

uschar *prvscheck_address      = NULL;
uschar *prvscheck_keynum       = NULL;
uschar *prvscheck_result       = NULL;


qrunner *qrunners	       = NULL;

const uschar *qualify_domain_recipient = NULL;
uschar *qualify_domain_sender  = NULL;
uschar *queue_domains          = NULL;
int     queue_interval         = -1;
uschar *queue_name             = US"";
uschar *queue_name_dest        = NULL;
uschar *queue_only_file        = NULL;
int     queue_only_load        = -1;
uschar *queue_run_max          = US"5";
pid_t   queue_run_pid          = (pid_t)0;
int     queue_run_pipe         = -1;
unsigned queue_size            = 0;
time_t  queue_size_next        = 0;
uschar *queue_smtp_domains     = NULL;

uint32_t random_seed	       = 0;
tree_node *ratelimiters_cmd    = NULL;
tree_node *ratelimiters_conn   = NULL;
tree_node *ratelimiters_mail   = NULL;
uschar *raw_active_hostname    = NULL;
const uschar *raw_sender       = NULL;
const uschar **raw_recipients  = NULL;
int     raw_recipients_count   = 0;

int     rcpt_count             = 0;
int     rcpt_fail_count        = 0;
int     rcpt_defer_count       = 0;
gid_t   real_gid;
uid_t   real_uid;
int     receive_linecount      = 0;
int     receive_messagecount   = 0;
int     receive_timeout        = 0;
int     received_count         = 0;
const uschar *received_for     = NULL;

/*  This is the default text for Received headers generated by Exim. The
date  will be automatically added on the end. */

uschar *received_header_text   = US
     "Received: "
     "${if def:sender_rcvhost {from $sender_rcvhost\n\t}"
       "{${if def:sender_ident {from ${quote_local_part:$sender_ident} }}"
         "${if def:sender_helo_name {(helo=$sender_helo_name)\n\t}}}}"
     "by $primary_hostname "
     "${if def:received_protocol {with $received_protocol }}"
#ifndef DISABLE_TLS
     "${if def:tls_in_ver        { ($tls_in_ver)}}"
     "${if def:tls_in_cipher_std { tls $tls_in_cipher_std\n\t}}"
#endif
     "(Exim $version_number)\n\t"
     "${if def:sender_address {(envelope-from <$sender_address>)\n\t}}"
     "id $message_exim_id"
     "${if def:received_for {\n\tfor $received_for}}"
     "\0<---------------Space to patch received_header_text->";

int     received_headers_max   = 30;
uschar *received_protocol      = NULL;
struct timeval received_time   = { 0, 0 };
struct timeval received_time_complete = { 0, 0 };
uschar *recipient_data         = NULL;
uschar *recipient_unqualified_hosts = NULL;
uschar *recipient_verify_failure = NULL;
int     recipients_count       = 0;
recipient_item  *recipients_list = NULL;
int     recipients_list_max    = 0;
uschar *recipients_max         = US"50000";
int     recipients_max_expanded= 0;
const pcre2_code *regex_AUTH         = NULL;
const pcre2_code *regex_check_dns_names = NULL;
const pcre2_code *regex_From         = NULL;
const pcre2_code *regex_IGNOREQUOTA  = NULL;
const pcre2_code *regex_PIPELINING   = NULL;
const pcre2_code *regex_SIZE         = NULL;
#ifndef DISABLE_PIPE_CONNECT
const pcre2_code *regex_EARLY_PIPE   = NULL;
#endif
int    regex_cachesize		     = 0;
const pcre2_code *regex_ismsgid      = NULL;
const pcre2_code *regex_smtp_code    = NULL;
const uschar *regex_vars[REGEX_VARS] = { NULL };
#ifdef WHITELIST_D_MACROS
const pcre2_code *regex_whitelisted_macro = NULL;
#endif
#ifdef WITH_CONTENT_SCAN
uschar *regex_match_string     = NULL;
#endif
int     remote_delivery_count  = 0;
int     remote_max_parallel    = 4;
uschar *remote_sort_domains    = NULL;
int     retry_data_expire      = 7*24*60*60;
int     retry_interval_max     = 24*60*60;
int     retry_maximum_timeout  = 0;        /* set from retry config */
retry_config  *retries         = NULL;
const uschar *return_path            = NULL;
int     rewrite_existflags     = 0;
uschar *rfc1413_hosts          = US"@[]";
int     rfc1413_query_timeout  = 0;
uid_t   root_gid               = ROOT_GID;
uid_t   root_uid               = ROOT_UID;

router_instance  *routers  = NULL;

/* All elements not mentioned will be 0/NULL/FALSE */
router_instance  router_defaults = {
    .self =			US"freeze",

    .address_test =		TRUE,
    .expn =			TRUE,
    .log_as_local =		TRUE_UNSET,
    .more =			TRUE,
    .repeat_use =		TRUE,
    .retry_use_local_part =	TRUE_UNSET,
    .verify_recipient =		TRUE,
    .verify_sender =		TRUE,

    .self_code =		self_freeze,
    .uid =			(uid_t)(-1),
    .gid =			(gid_t)(-1),

    .dnssec =                   { .request= US"*", .require=NULL },
};

const uschar *router_name      = NULL;
tree_node *router_var	       = NULL;

ip_address_item *running_interfaces = NULL;

/* This is a weird one. The following string gets patched in the binary by the
script that sets up a copy of Exim for running in the test harness. It seems
that compilers are now clever, and share constant strings if they can.
Elsewhere in Exim the string "<" is used. The compiler optimization seems to
make use of the end of this string in order to save space. So the patching then
wrecks this. We defeat this optimization by adding some additional characters
onto the end of the string. */

uschar *running_status         = US">>>running<<<" "\0EXTRA";

int     runrc                  = 0;

uschar *search_error_message   = NULL;
uschar *self_hostname          = NULL;
const uschar *sender_address   = NULL;
unsigned int sender_address_cache[(MAX_NAMED_LIST * 2)/32];
uschar *sender_address_data    = NULL;
const uschar *sender_address_unrewritten = NULL;
uschar *sender_data            = NULL;
unsigned int sender_domain_cache[(MAX_NAMED_LIST * 2)/32];
uschar *sender_fullhost        = NULL;
uschar *sender_helo_name       = NULL;
uschar **sender_host_aliases   = &no_aliases;
uschar *sender_host_address    = NULL;
uschar *sender_host_authenticated = NULL;
uschar *sender_host_auth_pubname  = NULL;
unsigned int sender_host_cache[(MAX_NAMED_LIST * 2)/32];
uschar *sender_host_name       = NULL;
int     sender_host_port       = 0;
uschar *sender_ident           = NULL;
uschar *sender_rate            = NULL;
uschar *sender_rate_limit      = NULL;
uschar *sender_rate_period     = NULL;
uschar *sender_rcvhost         = NULL;
uschar *sender_unqualified_hosts = NULL;
uschar *sender_verify_failure = NULL;
address_item *sender_verified_list  = NULL;
address_item *sender_verified_failed = NULL;
int     sender_verified_rc     = -1;
uschar *sending_ip_address     = NULL;
int     sending_port           = -1;
SIGNAL_BOOL sigalrm_seen       = FALSE;
const uschar *sigalarm_setter  = NULL;
const uschar **sighup_argv     = NULL;
int     slow_lookup_log        = 0;	/* millisecs, zero disables */
int     smtp_accept_count      = 0;
int     smtp_accept_max        = 20;
int     smtp_accept_max_nonmail= 10;
uschar *smtp_accept_max_nonmail_hosts = US"*";
uschar *smtp_accept_max_per_connection = US"1000";
uschar *smtp_accept_max_per_host = NULL;
int     smtp_accept_queue      = 0;
int     smtp_accept_queue_per_connection = 10;
int     smtp_accept_reserve    = 0;
uschar *smtp_active_hostname   = NULL;
int	smtp_backlog_monitor   = 0;
uschar *smtp_banner            = US"$smtp_active_hostname ESMTP "
                             "Exim $version_number $tod_full"
                             "\0<---------------Space to patch smtp_banner->";
int     smtp_ch_index          = 0;
uschar *smtp_cmd_argument      = NULL;
uschar *smtp_cmd_buffer        = NULL;
struct timeval smtp_connection_start  = {0,0};
uschar  smtp_connection_had[SMTP_HBUFF_SIZE];
int     smtp_connect_backlog   = 20;
double  smtp_delay_mail        = 0.0;
double  smtp_delay_rcpt        = 0.0;
int	smtp_in_fd	       = -1;
int     smtp_listen_backlog    = 0;
int     smtp_load_reserve      = -1;
int     smtp_mailcmd_count     = 0;
int     smtp_mailcmd_max       = -1;
int	smtp_out_fd	       = -1;
uschar *smtp_etrn_command      = NULL;
int     smtp_max_synprot_errors= 3;
int     smtp_max_unknown_commands = 3;
const uschar *smtp_notquit_reason = NULL;
unsigned smtp_peer_options     = 0;
unsigned smtp_peer_options_wrap= 0;
uschar *smtp_ratelimit_hosts   = NULL;
uschar *smtp_ratelimit_mail    = NULL;
uschar *smtp_ratelimit_rcpt    = NULL;
int     smtp_receive_timeout   = 5*60;
uschar *smtp_receive_timeout_s = NULL;
uschar *smtp_reserve_hosts     = NULL;
int     smtp_rlm_base          = 0;
double  smtp_rlm_factor        = 0.0;
int     smtp_rlm_limit         = 0;
int     smtp_rlm_threshold     = INT_MAX;
int     smtp_rlr_base          = 0;
double  smtp_rlr_factor        = 0.0;
int     smtp_rlr_limit         = 0;
int     smtp_rlr_threshold     = INT_MAX;
#ifdef SUPPORT_I18N
uschar *smtputf8_advertise_hosts = US"*";	/* overridden under test-harness */
#endif

#ifdef WITH_CONTENT_SCAN
uschar *spamd_address          = US"127.0.0.1 783";
uschar *spam_bar               = NULL;
uschar *spam_report            = NULL;
uschar *spam_action            = NULL;
uschar *spam_score             = NULL;
uschar *spam_score_int         = NULL;
#endif

FILE   *spool_data_file	       = NULL;
uschar *spool_directory        = US SPOOL_DIRECTORY
                           "\0<--------------Space to patch spool_directory->";
#ifdef SUPPORT_SRS
uschar *srs_recipient          = NULL;
#endif
int     string_datestamp_offset= -1;
int     string_datestamp_length= 0;
int     string_datestamp_type  = -1;
const uschar *submission_domain = NULL;
const uschar *submission_name  = NULL;
int     syslog_facility        = LOG_MAIL;
const uschar *syslog_processname= US"exim";
uschar *system_filter          = NULL;

uschar *system_filter_directory_transport = NULL;
uschar *system_filter_file_transport = NULL;
uschar *system_filter_pipe_transport = NULL;
uschar *system_filter_reply_transport = NULL;

gid_t   system_filter_gid      = 0;
uid_t   system_filter_uid      = (uid_t)-1;

blob	tcp_fastopen_nodata    = { .data = NULL, .len = 0 };
tfo_state_t tcp_out_fastopen   = TFO_NOT_USED;
int	test_harness_identd_port  = IDENT_PORT;
int     test_harness_load_avg  = 0;
int     thismessage_size_limit = 0;
int     timeout_frozen_after   = 0;
#ifdef MEASURE_TIMING
struct timeval timestamp_startup;
#endif

transport_instance  *transports = NULL;

transport_instance  transport_defaults = {
    /* All non-mentioned elements zero/NULL/FALSE */
    .batch_max =		1,
    .multi_domain =		TRUE,
    .max_addresses =		US"100",
    .connection_max_messages =	500,
    .uid =			(uid_t)(-1),
    .gid =			(gid_t)(-1),
    .filter_timeout =		300,
    .retry_use_local_part =	TRUE_UNSET,	/* retry_use_local_part: BOOL, but set neither
						 1 nor 0 so can detect unset */
};

int     transport_count;
const uschar *transport_name          = NULL;
int     transport_newlines;
const uschar **transport_filter_argv  = NULL;
int     transport_filter_timeout;
int     transport_write_timeout= 0;

tree_node  *tree_dns_fails     = NULL;
tree_node  *tree_duplicates    = NULL;
tree_node  *tree_nonrecipients = NULL;
tree_node  *tree_unusable      = NULL;

gid_t  *trusted_groups         = NULL;
uid_t  *trusted_users          = NULL;
uschar *timezone_string        = US TIMEZONE_DEFAULT;

uschar *unknown_login          = NULL;
uschar *unknown_username       = NULL;
uschar *untrusted_set_sender   = NULL;

/*  A regex for matching a "From_" line in an incoming message, in the form

    From ph10 Fri Jan  5 12:35 GMT 1996

which  the "mail" commands send to the MTA (undocumented, of course), or in
the  form

    From ph10 Fri, 7 Jan 97 14:00:00 GMT

which  is apparently used by some UUCPs, despite it not being in RFC 976.
Because  of variations in time formats, just match up to the minutes. That
should  be sufficient. Examples have been seen of time fields like 12:1:03,
so  just require one digit for hours and minutes. The weekday is also absent
in  some forms. */

uschar *uucp_from_pattern      = US
   "^From\\s+(\\S+)\\s+(?:[a-zA-Z]{3},?\\s+)?"    /* Common start */
   "(?:"                                          /* Non-extracting bracket */
   "[a-zA-Z]{3}\\s+\\d?\\d|"                      /* First form */
   "\\d?\\d\\s+[a-zA-Z]{3}\\s+\\d\\d(?:\\d\\d)?"  /* Second form */
   ")"                                            /* End alternation */
   "\\s+\\d\\d?:\\d\\d?";                         /* Start of time */

uschar *uucp_from_sender       = US"$1";

uschar *verify_mode	       = NULL;
uschar *version_copyright      =
 US"Copyright (c) University of Cambridge, 1995 - 2018\n"
   "(c) The Exim Maintainers and contributors in ACKNOWLEDGMENTS file, 2007 - 2024";
uschar *version_date           = US"?";
uschar *version_cnumber        = US"????";
uschar *version_string         = US"?";

uschar *warn_message_file      = NULL;
int     warning_count          = 0;
const uschar *warnmsg_delay    = NULL;
const uschar *warnmsg_recipients = NULL;

#ifndef DISABLE_WELLKNOWN
uschar *wellknown_advertise_hosts = NULL;
uschar *wellknown_response     = NULL;
#endif

/*  End of globals.c */
/* vi: aw ai sw=2
*/
