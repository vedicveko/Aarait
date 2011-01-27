/************************************************************************
 * Generic OLC Library - General / genolc.c			v1.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-1999 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "comm.h"
#include "shop.h"
#include "genolc.h"
#include "genwld.h"
#include "genmob.h"
#include "genshp.h"
#include "genzon.h"
#include "genobj.h"

extern struct zone_data *zone_table;
extern zone_rnum top_of_zone_table;
extern struct room_data *world;
extern struct char_data *mob_proto;
extern struct index_data *mob_index;

int top_shop_offset = 1;

/*
 * List of zones to be saved.
 */
struct save_list_data *save_list;

/*
 * Structure defining all known save types.
 */
struct {
  int save_type;
  int (*func)(sh_int rnum);
  const char *message;
} save_types[] = {
  { SL_MOB, save_mobiles , "mobile" },
  { SL_OBJ, save_objects, "object" },
  { SL_SHP, save_shops, "shop" },
  { SL_WLD, save_rooms, "room" },
  { SL_ZON, save_zone, "zone" },
  { -1, NULL, NULL },
};

/* -------------------------------------------------------------------------- */

int genolc_checkstring(struct descriptor_data *d, const char *arg)
{
  char gcbuf[128];
  if (strchr(arg, STRING_TERMINATOR)) {
    sprintf(gcbuf, "Sorry, you cannot use '%c' in your descriptions.\r\n", STRING_TERMINATOR);
    SEND_TO_Q(gcbuf, d);
    return FALSE;
  }
  return TRUE;
}

char *str_udup(const char *txt)
{
  return str_dup((txt && *txt) ? txt : "undefined");
}

/*
 * Original use: to be called at shutdown time.
 */
int save_all(void)
{
  while (save_list) {
    if (save_list->type < 0 || save_list->type > SL_MAX)
      log("SYSERR: GenOLC: Invalid save type %d in save list.\n", save_list->type);
    else if ((*save_types[save_list->type].func)(real_zone(save_list->zone)) < 0)
      save_list = save_list->next;	/* Fatal error, skip this one. */
  }

  return TRUE;
}

/* -------------------------------------------------------------------------- */

/*
 * NOTE: This changes the buffer passed in.
 */
void strip_cr(char *buffer)
{
  int rpos, wpos;

  if (buffer == NULL)
    return;

  for (rpos = 0, wpos = 0; buffer[rpos]; rpos++) {
    buffer[wpos] = buffer[rpos];
    wpos += (buffer[rpos] != '\r');
  }
  buffer[wpos] = '\0';
}

/* -------------------------------------------------------------------------- */

void copy_ex_descriptions(struct extra_descr_data **to, struct extra_descr_data *from)
{
  struct extra_descr_data *wpos;

  CREATE(*to, struct extra_descr_data, 1);
  wpos = *to;

  for (; from; from = from->next, wpos = wpos->next) {
    wpos->keyword = str_dup(from->keyword);
    wpos->description = str_dup(from->description);
    if (from->next)
      CREATE(wpos->next, struct extra_descr_data, 1);
  }
}

/* -------------------------------------------------------------------------- */

void free_ex_descriptions(struct extra_descr_data *head)
{
  struct extra_descr_data *thised, *next_one;

  if (!head) {
    log("free_ex_descriptions: NULL pointer or NULL data.");
    return;
  }

  for (thised = head; thised; thised = next_one) {
    next_one = thised->next;
    if (thised->keyword)
      free(thised->keyword);
    if (thised->description)
      free(thised->description);
    free(thised);
  }
}

/* -------------------------------------------------------------------------- */

int remove_from_save_list(zone_vnum zone, int type)
{
  struct save_list_data *ritem, *temp;

  for (ritem = save_list; ritem; ritem = ritem->next)
    if (ritem->zone == zone && ritem->type == type)
      break;

  if (ritem == NULL) {
    log("SYSERR: remove_from_save_list: Saved item not found. (%d/%d)", zone, type);
    return -1;
  }
  REMOVE_FROM_LIST(ritem, save_list, next);
  free(ritem);
  return TRUE;
}

/* -------------------------------------------------------------------------- */

int add_to_save_list(zone_vnum zone, int type)
{
  struct save_list_data *nitem;
  zone_rnum rznum = real_zone(zone);

  if (rznum < 0 || rznum > top_of_zone_table) {
    log("SYSERR: add_to_save_list: Invalid zone number passed. (%d => %d, 0-%d)", zone, rznum, top_of_zone_table);
    return -1;
  }

  for (nitem = save_list; nitem; nitem = nitem->next)
    if (nitem->zone == zone && nitem->type == type)
      return FALSE;

  CREATE(nitem, struct save_list_data, 1);
  nitem->zone = zone;
  nitem->type = type;
  nitem->next = save_list;
  save_list = nitem;
  return TRUE;
}

/* -------------------------------------------------------------------------- */

/*
 * Used from do_show(), ideally.
 */
void do_show_save_list(struct char_data *ch)
{
  if (save_list == NULL)
    send_to_char("All world files are up to date.\r\n", ch);
  else {
    struct save_list_data *item;

    send_to_char("The following files need saving:\r\n", ch);
    for (item = save_list; item; item = item->next) {
      char svbuf[128];
      sprintf(svbuf, " - %s data for zone %d.\r\n",
		save_types[item->type].message, item->zone);
      send_to_char(svbuf, ch);
    }
  }
}

/* -------------------------------------------------------------------------- */

/*
 * TEST FUNCTION! Not meant to be pretty!
 *
 * edit q	- Query unsaved files.
 * edit a	- Save all.
 * edit r1204 c	- Copies current room to 1204.
 * edit r1204 d	- Deletes room 1204.
 * edit r12 s	- Saves rooms in zone 12.
 * edit m3000 c3001	- Copies mob 3000 to 3001.
 * edit m3000 d	- Deletes mob 3000.
 * edit m30 s	- Saves mobiles in zone 30.
 * edit o186 s	- Saves objects in zone 186.
 * edit s25 s	- Saves shops in zone 25.
 * edit z31 s	- Saves zone 31.
 */
#include "interpreter.h"
ACMD(do_edit);
ACMD(do_edit)
{
  int idx, num, mun;
  char a[MAX_INPUT_LENGTH], b[MAX_INPUT_LENGTH];

  two_arguments(argument, a, b);
  num = atoi(a + 1);
  mun = atoi(b + 1);
  switch (a[0]) {
  case 'a':
    save_all();
    break;
  case 'm':
    switch (b[0]) {
    case 'd':
      if ((idx = real_mobile(num)) != NOBODY) {
	delete_mobile(idx);	/* Delete -> Real */
	send_to_char(OK, ch);
      } else
	send_to_char("What mobile?\r\n", ch);
      break;
    case 's':
      save_mobiles(num);
      break;
    case 'c':
      if ((num = real_mobile(num)) == NOBODY)
	send_to_char("What mobile?\r\n", ch);
      else if ((mun = real_mobile(mun)) == NOBODY)
	send_to_char("Can only copy over an existing mob.\r\n", ch);
      else {
        /* Otherwise the old ones have dangling string pointers. */
        extract_mobile_all(mob_index[mun].vnum);
        /* To <- From */
	copy_mobile(mob_proto + mun, mob_proto + num);
	send_to_char(OK, ch);
      }
      break;
    }
    break;
  case 's':
    switch (b[0]) {
    case 's':
      save_objects(real_zone(num));
      break;
    }
    break;
  case 'z':
    switch (b[0]) {
    case 's':
      save_zone(real_zone(num));
      break;
    }
    break;
  case 'o':
    switch (b[0]) {
    case 's':
      save_objects(real_zone(num));
      break;
    }
    break;
  case 'r':
    switch (b[0]) {
    case 'd':
      if ((idx = real_room(num)) != NOWHERE) {
	delete_room(idx);
	send_to_char(OK, ch);
      } else
	send_to_char("What room?\r\n", ch);
      break;
    case 's':
      save_rooms(real_zone(num));
      break;
    case 'c':
      duplicate_room(num, ch->in_room);	/* To -> Virtual, From -> Real */
      break;
    }
  case 'q':
    do_show_save_list(ch);
    break;
  default:
    send_to_char("What?\r\n", ch);
    break;
  }
}
