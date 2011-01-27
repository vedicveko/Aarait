/*
 * Originally written by: Michael Scott -- Manx.
 * Last known e-mail address: scottm@workcomm.net
 *
 * XXX: This needs Oasis-ifying.
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "comm.h"
#include "db.h"
#include "genolc.h"
#include "oasis.h"
#include "improved-edit.h"
#include "tedit.h"

extern const char *credits;
extern const char *news;
extern const char *motd;
extern const char *imotd;
extern const char *help;
extern const char *info;
extern const char *background;
extern const char *handbook;
extern const char *policies;

void tedit_string_cleanup(struct descriptor_data *d, int terminator)
{
  FILE *fl;
  char *storage = (char *)d->olc;

  if (!storage)
    terminator = STRINGADD_ABORT;

  switch (terminator) {
  case STRINGADD_SAVE:
    if (!(fl = fopen(storage, "w"))) {
      sprintf(buf, "SYSERR: Can't write file '%s'.", storage);
      mudlog(buf, CMP, LVL_IMPL, TRUE);
    } else {
      if (*d->str) {
        strip_cr(*d->str);
        fputs(*d->str, fl);
      }
      fclose(fl);
      sprintf(buf, "OLC: %s saves '%s'.", GET_NAME(d->character), storage);
      mudlog(buf, CMP, LVL_GOD, TRUE);
      SEND_TO_Q("Saved.\r\n", d);
    }
    break;
  case STRINGADD_ABORT:
    SEND_TO_Q("Edit aborted.\r\n", d);
    act("$n stops editing some scrolls.", TRUE, d->character, 0, 0, TO_ROOM);
    break;
  default:
    log("SYSERR: tedit_string_cleanup: Unknown terminator status.");
    break;
  }

  /* Common cleanup code. */
  if (d->olc) {
    free(d->olc);
    d->olc = NULL;
  }
  STATE(d) = CON_PLAYING;
}

ACMD(do_tedit)
{
  int l, i;
  char field[MAX_INPUT_LENGTH];
  char *backstr = NULL;
   
  struct {
    char *cmd;
    char level;
    const char **buffer;
    int  size;
    char *filename;
  } fields[] = {
	/* edit the lvls to your own needs */
	{ "credits",	LVL_IMPL,	&credits,	2400,	CREDITS_FILE},
	{ "news",	LVL_GRGOD,	&news,		8192,	NEWS_FILE},
	{ "motd",	LVL_GRGOD,	&motd,		2400,	MOTD_FILE},
	{ "imotd",	LVL_IMPL,	&imotd,		2400,	IMOTD_FILE},
	{ "help",       LVL_GRGOD,	&help,		2400,	HELP_PAGE_FILE},
	{ "info",	LVL_GRGOD,	&info,		8192,	INFO_FILE},
	{ "background",	LVL_IMPL,	&background,	8192,	BACKGROUND_FILE},
	{ "handbook",   LVL_IMPL,	&handbook,	8192,   HANDBOOK_FILE},
	{ "policies",	LVL_IMPL,	&policies,	8192,	POLICIES_FILE},
	{ "\n",		0,		NULL,		0,	NULL }
  };

  if (ch->desc == NULL)
    return;
   
  half_chop(argument, field, buf);

  if (!*field) {
    strcpy(buf, "Files available to be edited:\r\n");
    i = 1;
    for (l = 0; *fields[l].cmd != '\n'; l++) {
      if (GET_LEVEL(ch) >= fields[l].level) {
	sprintf(buf, "%s%-11.11s", buf, fields[l].cmd);
	if (!(i % 7))
	  strcat(buf, "\r\n");
	i++;
      }
    }
    if (--i % 7)
      strcat(buf, "\r\n");
    if (i == 0)
      strcat(buf, "None.\r\n");
    send_to_char(buf, ch);
    return;
  }
  for (l = 0; *(fields[l].cmd) != '\n'; l++)
    if (!strncmp(field, fields[l].cmd, strlen(field)))
      break;
   
  if (*fields[l].cmd == '\n') {
    send_to_char("Invalid text editor option.\r\n", ch);
    return;
  }
   
  if (GET_LEVEL(ch) < fields[l].level) {
    send_to_char("You are not godly enough for that!\r\n", ch);
    return;
  }

  /* set up editor stats */
  clear_screen(ch->desc);
  send_editor_help(ch->desc);
  send_to_char("Edit file below:\r\n\r\n", ch);

  if (*fields[l].buffer) {
    send_to_char(*fields[l].buffer, ch);
    backstr = str_dup(*fields[l].buffer);
  }

  ch->desc->olc = str_dup(fields[l].filename);
  string_write(ch->desc, (char **)fields[l].buffer, fields[l].size, 0, backstr);

  act("$n begins editing a scroll.", TRUE, ch, 0, 0, TO_ROOM);
  STATE(ch->desc) = CON_TEDIT;
}
