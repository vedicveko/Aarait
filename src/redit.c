/************************************************************************
 *  OasisOLC - Rooms / redit.c					v2.0	*
 *  Original author: Levork						*
 *  Copyright 1996 Harvey Gilpin					*
 *  Copyright 1997-1999 George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "boards.h"
#include "genolc.h"
#include "genwld.h"
#include "oasis.h"
#include "improved-edit.h"

/*------------------------------------------------------------------------*/

/*
 * External data structures.
 */
extern struct room_data *world;
extern struct obj_data *obj_proto;
extern struct char_data *mob_proto;
extern const char *room_bits[];
extern const char *sector_types[];
extern const char *exit_bits[];
extern struct zone_data *zone_table;
extern room_rnum r_mortal_start_room;
extern room_rnum r_immort_start_room;
extern room_rnum r_frozen_start_room;
extern room_vnum mortal_start_room;
extern room_vnum immort_start_room;
extern room_vnum frozen_start_room;
extern struct descriptor_data *descriptor_list;

/*------------------------------------------------------------------------*/


/*------------------------------------------------------------------------*\
  Utils and exported functions.
\*------------------------------------------------------------------------*/

void redit_setup_new(struct descriptor_data *d)
{
  CREATE(OLC_ROOM(d), struct room_data, 1);

  OLC_ROOM(d)->name = str_dup("An unfinished room");
  OLC_ROOM(d)->description = str_dup("You are in an unfinished room.\r\n");
  OLC_ROOM(d)->number = NOWHERE;
  redit_disp_menu(d);
  OLC_VAL(d) = 0;
}

/*------------------------------------------------------------------------*/

void redit_setup_existing(struct descriptor_data *d, int real_num)
{
  struct room_data *room;
  int counter;

  /*
   * Build a copy of the room for editing.
   */
  CREATE(room, struct room_data, 1);

  *room = world[real_num];
  /*
   * Allocate space for all strings.
   */
  room->name = str_udup(world[real_num].name);
  room->description = str_udup(world[real_num].description);
  room->description = str_udup(world[real_num].description);

  /*
   * Exits - We allocate only if necessary.
   */
  for (counter = 0; counter < NUM_OF_DIRS; counter++) {
    if (world[real_num].dir_option[counter]) {
      CREATE(room->dir_option[counter], struct room_direction_data, 1);

      /*
       * Copy the numbers over.
       */
      *room->dir_option[counter] = *world[real_num].dir_option[counter];
      /*
       * Allocate the strings.
       */
      if (world[real_num].dir_option[counter]->general_description)
        room->dir_option[counter]->general_description = str_dup(world[real_num].dir_option[counter]->general_description);
      if (world[real_num].dir_option[counter]->keyword)
        room->dir_option[counter]->keyword = str_dup(world[real_num].dir_option[counter]->keyword);
    }
  }

  /*
   * Extra descriptions, if necessary.
   */
  if (world[real_num].ex_description) {
    struct extra_descr_data *tdesc, *temp, *temp2;
    CREATE(temp, struct extra_descr_data, 1);

    room->ex_description = temp;
    for (tdesc = world[real_num].ex_description; tdesc; tdesc = tdesc->next) {
      temp->keyword = str_dup(tdesc->keyword);
      temp->description = str_dup(tdesc->description);
      if (tdesc->next) {
	CREATE(temp2, struct extra_descr_data, 1);
	temp->next = temp2;
	temp = temp2;
      } else
	temp->next = NULL;
    }
  }
  /*
   * Attach copy of room to player's descriptor.
   */
  OLC_ROOM(d) = room;
  OLC_VAL(d) = 0;
  redit_disp_menu(d);
}

/*------------------------------------------------------------------------*/

void redit_save_internally(struct descriptor_data *d)
{
  int j, room_num, new_room = FALSE;
  struct descriptor_data *dsc;

  if (OLC_ROOM(d)->number == NOWHERE) {
    new_room = TRUE;
    OLC_ROOM(d)->number = OLC_NUM(d);
  }
  /* FIXME: Why is this not set elsewhere? */
  OLC_ROOM(d)->zone = OLC_ZNUM(d);

  if ((room_num = add_room(OLC_ROOM(d))) < 0) {
    SEND_TO_Q("Something went wrong...\r\n", d);
    log("SYSERR: redit_save_internally: Something failed! (%d)", room_num);
    return;
  }

  /* Don't adjust numbers on a room update. */
  if (!new_room)
    return;

  /* Idea contributed by C.Raehl 4/27/99 */
  for (dsc = descriptor_list; dsc; dsc = dsc->next) {
    if (dsc == d)
      continue;

    if (STATE(dsc) == CON_ZEDIT) {
      for (j = 0; OLC_ZONE(dsc)->cmd[j].command != 'S'; j++)
        switch (OLC_ZONE(dsc)->cmd[j].command) {
          case 'O':
          case 'M':
            OLC_ZONE(dsc)->cmd[j].arg3 += (OLC_ZONE(dsc)->cmd[j].arg3 >= room_num);
            break;
          case 'D':
            OLC_ZONE(dsc)->cmd[j].arg2 += (OLC_ZONE(dsc)->cmd[j].arg2 >= room_num);
            /* Fall through */
          case 'R':
            OLC_ZONE(dsc)->cmd[j].arg1 += (OLC_ZONE(dsc)->cmd[j].arg1 >= room_num);
            break;
          }
    } else if (STATE(dsc) == CON_REDIT) {
      for (j = 0; j < NUM_OF_DIRS; j++)
        if (OLC_ROOM(dsc)->dir_option[j])
          if (OLC_ROOM(dsc)->dir_option[j]->to_room >= room_num)
            OLC_ROOM(dsc)->dir_option[j]->to_room++;
    }
  }
}

/*------------------------------------------------------------------------*/

void redit_save_to_disk(zone_vnum zone_num)
{
  save_rooms(zone_num);		/* :) */
}

/*------------------------------------------------------------------------*/

void free_room(struct room_data *room)
{
  int i;
  struct extra_descr_data *tdesc, *next;

  if (room->name)
    free(room->name);
  if (room->description)
    free(room->description);

  /*
   * Free exits.
   */
  for (i = 0; i < NUM_OF_DIRS; i++) {
    if (room->dir_option[i]) {
      if (room->dir_option[i]->general_description)
	free(room->dir_option[i]->general_description);
      if (room->dir_option[i]->keyword)
	free(room->dir_option[i]->keyword);
      free(room->dir_option[i]);
    }
  }

  /*
   * Free extra descriptions.
   */
  for (tdesc = room->ex_description; tdesc; tdesc = next) {
    next = tdesc->next;
    if (tdesc->keyword)
      free(tdesc->keyword);
    if (tdesc->description)
      free(tdesc->description);
    free(tdesc);
  }
  free(room);	/* XXX ? */
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

/*
 * For extra descriptions.
 */
void redit_disp_extradesc_menu(struct descriptor_data *d)
{
  struct extra_descr_data *extra_desc = OLC_DESC(d);

  clear_screen(d);
  sprintf(buf,
	  "%s1%s) Keyword: %s%s\r\n"
	  "%s2%s) Description:\r\n%s%s\r\n"
	  "%s3%s) Goto next description: ",

	  grn, nrm, yel, extra_desc->keyword ? extra_desc->keyword : "<NONE>",
	  grn, nrm, yel, extra_desc->description ? extra_desc->description : "<NONE>",
	  grn, nrm
	  );

  strcat(buf, !extra_desc->next ? "<NOT SET>\r\n" : "Set.\r\n");
  strcat(buf, "Enter choice (0 to quit) : ");
  SEND_TO_Q(buf, d);
  OLC_MODE(d) = REDIT_EXTRADESC_MENU;
}

/*
 * For exits.
 */
void redit_disp_exit_menu(struct descriptor_data *d)
{
  /*
   * if exit doesn't exist, alloc/create it 
   */
  if (OLC_EXIT(d) == NULL)
    CREATE(OLC_EXIT(d), struct room_direction_data, 1);

  /*
   * Weird door handling! 
   */
  if (IS_SET(OLC_EXIT(d)->exit_info, EX_ISDOOR)) {
    if (IS_SET(OLC_EXIT(d)->exit_info, EX_PICKPROOF))
      strcpy(buf2, "Pickproof");
    else
      strcpy(buf2, "Is a door");
  } else
    strcpy(buf2, "No door");

  get_char_colors(d->character);
  clear_screen(d);
  sprintf(buf,
	  "%s1%s) Exit to     : %s%d\r\n"
	  "%s2%s) Description :-\r\n%s%s\r\n"
	  "%s3%s) Door name   : %s%s\r\n"
	  "%s4%s) Key         : %s%d\r\n"
	  "%s5%s) Door flags  : %s%s\r\n"
	  "%s6%s) Purge exit.\r\n"
	  "Enter choice, 0 to quit : ",

	  grn, nrm, cyn, OLC_EXIT(d)->to_room != -1 ? world[OLC_EXIT(d)->to_room].number : -1,
	  grn, nrm, yel, OLC_EXIT(d)->general_description ? OLC_EXIT(d)->general_description : "<NONE>",
	  grn, nrm, yel, OLC_EXIT(d)->keyword ? OLC_EXIT(d)->keyword : "<NONE>",
	  grn, nrm, cyn, OLC_EXIT(d)->key,
	  grn, nrm, cyn, buf2, grn, nrm
	  );

  SEND_TO_Q(buf, d);
  OLC_MODE(d) = REDIT_EXIT_MENU;
}

/*
 * For exit flags.
 */
void redit_disp_exit_flag_menu(struct descriptor_data *d)
{
  get_char_colors(d->character);
  sprintf(buf, "%s0%s) No door\r\n"
	  "%s1%s) Closeable door\r\n"
	  "%s2%s) Pickproof\r\n"
	  "Enter choice : ", grn, nrm, grn, nrm, grn, nrm);
  SEND_TO_Q(buf, d);
}

/*
 * For room flags.
 */
void redit_disp_flag_menu(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);
  for (counter = 0; counter < NUM_ROOM_FLAGS; counter++) {
    sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
		room_bits[counter], !(++columns % 2) ? "\r\n" : "");
    SEND_TO_Q(buf, d);
  }
  sprintbit(OLC_ROOM(d)->room_flags, room_bits, buf1);
  sprintf(buf, "\r\nRoom flags: %s%s%s\r\n"
	  "Enter room flags, 0 to quit : ", cyn, buf1, nrm);
  SEND_TO_Q(buf, d);
  OLC_MODE(d) = REDIT_FLAGS;
}

/*
 * For sector type.
 */
void redit_disp_sector_menu(struct descriptor_data *d)
{
  int counter, columns = 0;

  clear_screen(d);
  for (counter = 0; counter < NUM_ROOM_SECTORS; counter++) {
    sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
		sector_types[counter], !(++columns % 2) ? "\r\n" : "");
    SEND_TO_Q(buf, d);
  }
  SEND_TO_Q("\r\nEnter sector type : ", d);
  OLC_MODE(d) = REDIT_SECTOR;
}

/*
 * The main menu.
 */
void redit_disp_menu(struct descriptor_data *d)
{
  struct room_data *room;

  get_char_colors(d->character);
  clear_screen(d);
  room = OLC_ROOM(d);

  sprintbit((long)room->room_flags, room_bits, buf1);
  sprinttype(room->sector_type, sector_types, buf2);
  sprintf(buf,
	  "-- Room number : [%s%d%s]  	Room zone: [%s%d%s]\r\n"
	  "%s1%s) Name        : %s%s\r\n"
	  "%s2%s) Description :\r\n%s%s"
	  "%s3%s) Room flags  : %s%s\r\n"
	  "%s4%s) Sector type : %s%s\r\n"
	  "%s5%s) Exit north  : %s%d\r\n"
	  "%s6%s) Exit east   : %s%d\r\n"
	  "%s7%s) Exit south  : %s%d\r\n"
	  "%s8%s) Exit west   : %s%d\r\n"
	  "%s9%s) Exit up     : %s%d\r\n"
	  "%sA%s) Exit down   : %s%d\r\n"
	  "%sB%s) Extra descriptions menu\r\n"
	  "%sQ%s) Quit\r\n"
	  "Enter choice : ",

	  cyn, OLC_NUM(d), nrm,
	  cyn, zone_table[OLC_ZNUM(d)].number, nrm,
	  grn, nrm, yel, room->name,
	  grn, nrm, yel, room->description,
	  grn, nrm, cyn, buf1,
	  grn, nrm, cyn, buf2,
	  grn, nrm, cyn,
	  room->dir_option[NORTH] && room->dir_option[NORTH]->to_room != -1 ?
	  world[room->dir_option[NORTH]->to_room].number : -1,
	  grn, nrm, cyn,
	  room->dir_option[EAST] && room->dir_option[EAST]->to_room != -1 ?
	  world[room->dir_option[EAST]->to_room].number : -1,
	  grn, nrm, cyn,
	  room->dir_option[SOUTH] && room->dir_option[SOUTH]->to_room != -1 ?
	  world[room->dir_option[SOUTH]->to_room].number : -1,
	  grn, nrm, cyn,
	  room->dir_option[WEST] && room->dir_option[WEST]->to_room != -1 ?
	  world[room->dir_option[WEST]->to_room].number : -1,
	  grn, nrm, cyn,
	  room->dir_option[UP] && room->dir_option[UP]->to_room != -1 ? 
	  world[room->dir_option[UP]->to_room].number : -1,
	  grn, nrm, cyn,
	  room->dir_option[DOWN] && room->dir_option[DOWN]->to_room != -1 ?
	  world[room->dir_option[DOWN]->to_room].number : -1,
	  grn, nrm, grn, nrm
	  );
  SEND_TO_Q(buf, d);

  OLC_MODE(d) = REDIT_MAIN_MENU;
}

/**************************************************************************
  The main loop
 **************************************************************************/

void redit_parse(struct descriptor_data *d, char *arg)
{
  int number;
  char *oldtext = NULL;

  switch (OLC_MODE(d)) {
  case REDIT_CONFIRM_SAVESTRING:
    switch (*arg) {
    case 'y':
    case 'Y':
      redit_save_internally(d);
      sprintf(buf, "OLC: %s edits room %d.", GET_NAME(d->character), OLC_NUM(d));
      mudlog(buf, CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE);
      /*
       * Do NOT free strings! Just the room structure. 
       */
      cleanup_olc(d, CLEANUP_STRUCTS);
      SEND_TO_Q("Room saved to memory.\r\n", d);
      break;
    case 'n':
    case 'N':
      /*
       * Free everything up, including strings, etc.
       */
      cleanup_olc(d, CLEANUP_ALL);
      break;
    default:
      SEND_TO_Q("Invalid choice!\r\nDo you wish to save this room internally? : ", d);
      break;
    }
    return;

  case REDIT_MAIN_MENU:
    switch (*arg) {
    case 'q':
    case 'Q':
      if (OLC_VAL(d)) { /* Something has been modified. */
	SEND_TO_Q("Do you wish to save this room internally? : ", d);
	OLC_MODE(d) = REDIT_CONFIRM_SAVESTRING;
      } else
	cleanup_olc(d, CLEANUP_ALL);
      return;
    case '1':
      SEND_TO_Q("Enter room name:-\r\n] ", d);
      OLC_MODE(d) = REDIT_NAME;
      break;
    case '2':
      OLC_MODE(d) = REDIT_DESC;
      clear_screen(d);
      send_editor_help(d);
      SEND_TO_Q("Enter room description:\r\n\r\n", d);

      if (OLC_ROOM(d)->description) {
	SEND_TO_Q(OLC_ROOM(d)->description, d);
	oldtext = str_dup(OLC_ROOM(d)->description);
      }
      string_write(d, &OLC_ROOM(d)->description, MAX_ROOM_DESC, 0, oldtext);
      OLC_VAL(d) = 1;
      break;
    case '3':
      redit_disp_flag_menu(d);
      break;
    case '4':
      redit_disp_sector_menu(d);
      break;
    case '5':
      OLC_VAL(d) = NORTH;
      redit_disp_exit_menu(d);
      break;
    case '6':
      OLC_VAL(d) = EAST;
      redit_disp_exit_menu(d);
      break;
    case '7':
      OLC_VAL(d) = SOUTH;
      redit_disp_exit_menu(d);
      break;
    case '8':
      OLC_VAL(d) = WEST;
      redit_disp_exit_menu(d);
      break;
    case '9':
      OLC_VAL(d) = UP;
      redit_disp_exit_menu(d);
      break;
    case 'a':
    case 'A':
      OLC_VAL(d) = DOWN;
      redit_disp_exit_menu(d);
      break;
    case 'b':
    case 'B':
      /*
       * If the extra description doesn't exist.
       */
      if (!OLC_ROOM(d)->ex_description)
	CREATE(OLC_ROOM(d)->ex_description, struct extra_descr_data, 1);
      OLC_DESC(d) = OLC_ROOM(d)->ex_description;
      redit_disp_extradesc_menu(d);
      break;
    default:
      SEND_TO_Q("Invalid choice!", d);
      redit_disp_menu(d);
      break;
    }
    return;

  case REDIT_NAME:
    if (!genolc_checkstring(d, arg))
      break;
    if (OLC_ROOM(d)->name)
      free(OLC_ROOM(d)->name);
    arg[MAX_ROOM_NAME - 1] = '\0';
    OLC_ROOM(d)->name = str_udup(arg);
    break;

  case REDIT_DESC:
    /*
     * We will NEVER get here, we hope.
     */
    mudlog("SYSERR: Reached REDIT_DESC case in parse_redit().", BRF, LVL_BUILDER, TRUE);
    SEND_TO_Q("Oops, in REDIT_DESC.\r\n", d);
    break;

  case REDIT_FLAGS:
    number = atoi(arg);
    if (number < 0 || number > NUM_ROOM_FLAGS) {
      SEND_TO_Q("That is not a valid choice!\r\n", d);
      redit_disp_flag_menu(d);
    } else if (number == 0)
      break;
    else {
      /*
       * Toggle the bit.
       */
      TOGGLE_BIT(OLC_ROOM(d)->room_flags, 1 << (number - 1));
      redit_disp_flag_menu(d);
    }
    return;

  case REDIT_SECTOR:
    number = atoi(arg);
    if (number < 0 || number >= NUM_ROOM_SECTORS) {
      SEND_TO_Q("Invalid choice!", d);
      redit_disp_sector_menu(d);
      return;
    }
    OLC_ROOM(d)->sector_type = number;
    break;

  case REDIT_EXIT_MENU:
    switch (*arg) {
    case '0':
      break;
    case '1':
      OLC_MODE(d) = REDIT_EXIT_NUMBER;
      SEND_TO_Q("Exit to room number : ", d);
      return;
    case '2':
      OLC_MODE(d) = REDIT_EXIT_DESCRIPTION;
      send_editor_help(d);
      SEND_TO_Q("Enter exit description:\r\n\r\n", d);
      if (OLC_EXIT(d)->general_description) {
	SEND_TO_Q(OLC_EXIT(d)->general_description, d);
	oldtext = str_dup(OLC_EXIT(d)->general_description);
      }
      string_write(d, &OLC_EXIT(d)->general_description, MAX_EXIT_DESC, 0, oldtext);
      return;
    case '3':
      OLC_MODE(d) = REDIT_EXIT_KEYWORD;
      SEND_TO_Q("Enter keywords : ", d);
      return;
    case '4':
      OLC_MODE(d) = REDIT_EXIT_KEY;
      SEND_TO_Q("Enter key number : ", d);
      return;
    case '5':
      OLC_MODE(d) = REDIT_EXIT_DOORFLAGS;
      redit_disp_exit_flag_menu(d);
      return;
    case '6':
      /*
       * Delete an exit.
       */
      if (OLC_EXIT(d)->keyword)
	free(OLC_EXIT(d)->keyword);
      if (OLC_EXIT(d)->general_description)
	free(OLC_EXIT(d)->general_description);
      if (OLC_EXIT(d))
	free(OLC_EXIT(d));
      OLC_EXIT(d) = NULL;
      break;
    default:
      SEND_TO_Q("Try again : ", d);
      return;
    }
    break;

  case REDIT_EXIT_NUMBER:
    if ((number = atoi(arg)) != -1)
      if ((number = real_room(number)) < 0) {
	SEND_TO_Q("That room does not exist, try again : ", d);
	return;
      }
    OLC_EXIT(d)->to_room = number;
    redit_disp_exit_menu(d);
    return;

  case REDIT_EXIT_DESCRIPTION:
    /*
     * We should NEVER get here, hopefully.
     */
    mudlog("SYSERR: Reached REDIT_EXIT_DESC case in parse_redit", BRF, LVL_BUILDER, TRUE);
    SEND_TO_Q("Oops, in REDIT_EXIT_DESCRIPTION.\r\n", d);
    break;

  case REDIT_EXIT_KEYWORD:
    if (OLC_EXIT(d)->keyword)
      free(OLC_EXIT(d)->keyword);
    OLC_EXIT(d)->keyword = str_udup(arg);
    redit_disp_exit_menu(d);
    return;

  case REDIT_EXIT_KEY:
    OLC_EXIT(d)->key = atoi(arg);
    redit_disp_exit_menu(d);
    return;

  case REDIT_EXIT_DOORFLAGS:
    number = atoi(arg);
    if (number < 0 || number > 2) {
      SEND_TO_Q("That's not a valid choice!\r\n", d);
      redit_disp_exit_flag_menu(d);
    } else {
      /*
       * Doors are a bit idiotic, don't you think? :) -- I agree. -gg
       */
      OLC_EXIT(d)->exit_info = (number == 0 ? 0 :
				(number == 1 ? EX_ISDOOR :
				(number == 2 ? EX_ISDOOR | EX_PICKPROOF : 0)));
      /*
       * Jump back to the menu system.
       */
      redit_disp_exit_menu(d);
    }
    return;

  case REDIT_EXTRADESC_KEY:
    if (genolc_checkstring(d, arg))
      OLC_DESC(d)->keyword = str_dup(arg);
    redit_disp_extradesc_menu(d);
    return;

  case REDIT_EXTRADESC_MENU:
    switch ((number = atoi(arg))) {
    case 0:
      /*
       * If something got left out, delete the extra description
       * when backing out to the menu.
       */
      if (OLC_DESC(d)->keyword == NULL || OLC_DESC(d)->description == NULL) {
	struct extra_descr_data **tmp_desc;
	if (OLC_DESC(d)->keyword)
	  free(OLC_DESC(d)->keyword);
	if (OLC_DESC(d)->description)
	  free(OLC_DESC(d)->description);

	/*
	 * Clean up pointers.
	 */
	for (tmp_desc = &(OLC_ROOM(d)->ex_description); *tmp_desc; tmp_desc = &((*tmp_desc)->next))
	  if (*tmp_desc == OLC_DESC(d)) {
	    *tmp_desc = NULL;
	    break;
	  }
	free(OLC_DESC(d));
      }
      break;
    case 1:
      OLC_MODE(d) = REDIT_EXTRADESC_KEY;
      SEND_TO_Q("Enter keywords, separated by spaces : ", d);
      return;
    case 2:
      OLC_MODE(d) = REDIT_EXTRADESC_DESCRIPTION;
      send_editor_help(d);
      SEND_TO_Q("Enter extra description:\r\n\r\n", d);
      if (OLC_DESC(d)->description) {
	SEND_TO_Q(OLC_DESC(d)->description, d);
	oldtext = str_dup(OLC_DESC(d)->description);
      }
      string_write(d, &OLC_DESC(d)->description, MAX_MESSAGE_LENGTH, 0, oldtext);
      return;
    case 3:
      if (OLC_DESC(d)->keyword == NULL || OLC_DESC(d)->description == NULL) {
	SEND_TO_Q("You can't edit the next extra description without completing this one.\r\n", d);
	redit_disp_extradesc_menu(d);
      } else {
	struct extra_descr_data *new_extra;

	if (OLC_DESC(d)->next)
	  OLC_DESC(d) = OLC_DESC(d)->next;
	else {
	  /*
	   * Make new extra description and attach at end.
	   */
	  CREATE(new_extra, struct extra_descr_data, 1);
	  OLC_DESC(d)->next = new_extra;
	  OLC_DESC(d) = new_extra;
	}
	redit_disp_extradesc_menu(d);
      }
      return;
    }
    break;

  default:
    /*
     * We should never get here.
     */
    mudlog("SYSERR: Reached default case in parse_redit", BRF, LVL_BUILDER, TRUE);
    break;
  }
  /*
   * If we get this far, something has been changed.
   */
  OLC_VAL(d) = 1;
  redit_disp_menu(d);
}

void redit_string_cleanup(struct descriptor_data *d, int terminator)
{
  switch (OLC_MODE(d)) {
  case REDIT_DESC:
    redit_disp_menu(d);
    break;
  case REDIT_EXIT_DESCRIPTION:
    redit_disp_exit_menu(d);
    break;
  case REDIT_EXTRADESC_DESCRIPTION:
    redit_disp_extradesc_menu(d);
    break;
  }
}
