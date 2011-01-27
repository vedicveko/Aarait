/************************************************************************
 * OasisOLC - Zones / zedit.c					v2.0	*
 * Copyright 1996 Harvey Gilpin						*
 * Copyright 1997-1999 George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "interpreter.h"
#include "utils.h"
#include "db.h"
#include "constants.h"
#include "genolc.h"
#include "genzon.h"
#include "oasis.h"

/*-------------------------------------------------------------------*/

extern struct zone_data *zone_table;
extern zone_rnum top_of_zone_table;
extern struct index_data *mob_index;
extern struct char_data *mob_proto;
extern struct index_data *obj_index;
extern struct obj_data *obj_proto;
extern struct descriptor_data *descriptor_list;

/*-------------------------------------------------------------------*/

/*
 * Nasty internal macros to clean up the code.
 */
#define MYCMD		(OLC_ZONE(d)->cmd[subcmd])
#define OLC_CMD(d)	(OLC_ZONE(d)->cmd[OLC_VAL(d)])

/* Prototypes. */
int start_change_command(struct descriptor_data *d, int pos);

/*-------------------------------------------------------------------*/

void zedit_setup(struct descriptor_data *d, int room_num)
{
  struct zone_data *zone;
  int subcmd = 0, count = 0, cmd_room = -1;

  /*
   * Allocate one scratch zone structure.  
   */
  CREATE(zone, struct zone_data, 1);

  /*
   * Copy all the zone header information over.
   */
  zone->name = str_dup(zone_table[OLC_ZNUM(d)].name);
  zone->lifespan = zone_table[OLC_ZNUM(d)].lifespan;
  zone->top = zone_table[OLC_ZNUM(d)].top;
  zone->reset_mode = zone_table[OLC_ZNUM(d)].reset_mode;
  /*
   * The remaining fields are used as a 'has been modified' flag  
   */
  zone->number = 0;	/* Header information has changed.	*/
  zone->age = 0;	/* The commands have changed.		*/

  /*
   * Start the reset command list with a terminator.
   */
  CREATE(zone->cmd, struct reset_com, 1);
  zone->cmd[0].command = 'S';

  /*
   * Add all entries in zone_table that relate to this room.
   */
  while (ZCMD(OLC_ZNUM(d), subcmd).command != 'S') {
    switch (ZCMD(OLC_ZNUM(d), subcmd).command) {
    case 'M':
    case 'O':
      cmd_room = ZCMD(OLC_ZNUM(d), subcmd).arg3;
      break;
    case 'D':
    case 'R':
      cmd_room = ZCMD(OLC_ZNUM(d), subcmd).arg1;
      break;
    default:
      break;
    }
    if (cmd_room == room_num) {
      add_cmd_to_list(&(zone->cmd), &ZCMD(OLC_ZNUM(d), subcmd), count);
      count++;
    }
    subcmd++;
  }

  OLC_ZONE(d) = zone;
  /*
   * Display main menu.
   */
  zedit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

/*
 * Create a new zone.
 */

void zedit_new_zone(struct char_data *ch, int vzone_num)
{
  int result;
  const char *error;
  struct descriptor_data *dsc;

  if ((result = create_new_zone(vzone_num, &error)) < 0) {
    SEND_TO_Q(error, ch->desc);
    return;
  }

  for (dsc = descriptor_list; dsc; dsc = dsc->next) {
    switch (STATE(dsc)) {
      case CON_REDIT:
        OLC_ROOM(dsc)->zone += (OLC_ZNUM(dsc) >= result);
        /* Fall through. */
      case CON_ZEDIT:
      case CON_MEDIT:
      case CON_SEDIT:
      case CON_OEDIT:
        OLC_ZNUM(dsc) += (OLC_ZNUM(dsc) >= result);
        break;
      default:
        break;
    }
  }

  sprintf(buf, "OLC: %s creates new zone #%d", GET_NAME(ch), vzone_num);
  mudlog(buf, BRF, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE);
  SEND_TO_Q("Zone create successfully\r\n", ch->desc);
}

/*-------------------------------------------------------------------*/

/*
 * Save all the information in the player's temporary buffer back into
 * the current zone table.
 */
void zedit_save_internally(struct descriptor_data *d)
{
  int	mobloaded = FALSE,
	objloaded = FALSE,
	subcmd = 0,
	room_num = real_room(OLC_NUM(d));

  remove_room_zone_commands(OLC_ZNUM(d), room_num);

  /*
   * Now add all the entries in the players descriptor list  
   */
  for (subcmd = 0; MYCMD.command != 'S'; subcmd++) {
    add_cmd_to_list(&(zone_table[OLC_ZNUM(d)].cmd), &MYCMD, subcmd);

    /*
     * Since Circle does not keep track of what rooms the 'G', 'E', and
     * 'P' commands are exitted in, but OasisOLC groups zone commands
     * by rooms, this creates interesting problems when builders use these
     * commands without loading a mob or object first.  This fix prevents such
     * commands from being saved and 'wandering' through the zone command
     * list looking for mobs/objects to latch onto.
     * C.Raehl 4/27/99
     */
    switch (MYCMD.command) {
      case 'M':
        mobloaded = TRUE;
        break;
      case 'G':
      case 'E':
        if (mobloaded)
          break;
        SEND_TO_Q("Equip/Give command not saved since no mob was loaded first.\r\n", d);
        remove_cmd_from_list(&(OLC_ZONE(d)->cmd), subcmd);
        break;
      case 'O':
        objloaded = TRUE;
        break;
      case 'P':
        if (objloaded)
          break;
        SEND_TO_Q("Put command not saved since another object was not loaded first.\r\n", d);
        remove_cmd_from_list(&(OLC_ZONE(d)->cmd), subcmd);
        break;
      default:
        mobloaded = objloaded = FALSE;
        break;
    }
  }

  /*
   * Finally, if zone headers have been changed, copy over  
   */
  if (OLC_ZONE(d)->number) {
    free(zone_table[OLC_ZNUM(d)].name);
    zone_table[OLC_ZNUM(d)].name = str_dup(OLC_ZONE(d)->name);
    zone_table[OLC_ZNUM(d)].top = OLC_ZONE(d)->top;
    zone_table[OLC_ZNUM(d)].reset_mode = OLC_ZONE(d)->reset_mode;
    zone_table[OLC_ZNUM(d)].lifespan = OLC_ZONE(d)->lifespan;
  }
  add_to_save_list(zone_table[OLC_ZNUM(d)].number, SL_ZON);
}

/*-------------------------------------------------------------------*/

void zedit_save_to_disk(int zone)
{
  save_zone(zone);
}

/*-------------------------------------------------------------------*/

/*
 * Error check user input and then setup change  
 */
int start_change_command(struct descriptor_data *d, int pos)
{
  int subcmd = 0;

  /*
   * Error check to ensure users hasn't given too large an index  
   */
  while (MYCMD.command != 'S')
    subcmd++;

  if (pos < 0 || pos >= subcmd)
    return 0;

  /*
   * Ok, let's get editing.
   */
  OLC_VAL(d) = pos;
  return 1;
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

/*
 * the main menu 
 */
void zedit_disp_menu(struct descriptor_data *d)
{
  int subcmd = 0, room, counter = 0;

  get_char_colors(d->character);
  clear_screen(d);
  room = real_room(OLC_NUM(d));

  /*
   * Menu header  
   */
  sprintf(buf,
	  "Room number: %s%d%s		Room zone: %s%d\r\n"
	  "%sZ%s) Zone name   : %s%s\r\n"
	  "%sL%s) Lifespan    : %s%d minutes\r\n"
	  "%sT%s) Top of zone : %s%d\r\n"
	  "%sR%s) Reset Mode  : %s%s%s\r\n"
	  "[Command list]\r\n",

	  cyn, OLC_NUM(d), nrm,
	  cyn, zone_table[OLC_ZNUM(d)].number,
	  grn, nrm, yel, OLC_ZONE(d)->name ? OLC_ZONE(d)->name : "<NONE!>",
	  grn, nrm, yel, OLC_ZONE(d)->lifespan,
	  grn, nrm, yel, OLC_ZONE(d)->top,
	  grn, nrm,
          yel,
          OLC_ZONE(d)->reset_mode ? ((OLC_ZONE(d)->reset_mode == 1) ? "Reset when no players are in zone." : "Normal reset.") : "Never reset",
          nrm
	  );

  /*
   * Print the commands for this room into display buffer.
   */
  while (MYCMD.command != 'S') {
    /*
     * Translate what the command means.
     */
    switch (MYCMD.command) {
    case 'M':
      sprintf(buf2, "%sLoad %s [%s%d%s], Max : %d, Percent: %d",
              MYCMD.if_flag ? " then " : "",
              mob_proto[MYCMD.arg1].player.short_descr, cyn,
              mob_index[MYCMD.arg1].vnum, yel, MYCMD.arg2, MYCMD.arg4
              );
      break;
    case 'G':
      sprintf(buf2, "%sGive it %s [%s%d%s], Max : %d, Percent: %d",
	      MYCMD.if_flag ? " then " : "",
	      obj_proto[MYCMD.arg1].short_description,
	      cyn, obj_index[MYCMD.arg1].vnum, yel,
	      MYCMD.arg2, MYCMD.arg4
	      );
      break;
    case 'O':
      sprintf(buf2, "%sLoad %s [%s%d%s], Max : %d, Percent: %d",
	      MYCMD.if_flag ? " then " : "",
	      obj_proto[MYCMD.arg1].short_description,
	      cyn, obj_index[MYCMD.arg1].vnum, yel,
	      MYCMD.arg2, MYCMD.arg4
	      );
      break;
    case 'E':
      sprintf(buf2, "%sEquip with %s [%s%d%s], %s, Max : %d, Percent: %d",
	      MYCMD.if_flag ? " then " : "",
	      obj_proto[MYCMD.arg1].short_description,
	      cyn, obj_index[MYCMD.arg1].vnum, yel,
	      equipment_types[MYCMD.arg3],
	      MYCMD.arg2, MYCMD.arg4
	      );
      break;
    case 'P':
      sprintf(buf2, "%sPut %s [%s%d%s] in %s [%s%d%s], Max : %d",
	      MYCMD.if_flag ? " then " : "",
	      obj_proto[MYCMD.arg1].short_description,
	      cyn, obj_index[MYCMD.arg1].vnum, yel,
	      obj_proto[MYCMD.arg3].short_description,
	      cyn, obj_index[MYCMD.arg3].vnum, yel,
	      MYCMD.arg2
	      );
      break;
    case 'R':
      sprintf(buf2, "%sRemove %s [%s%d%s] from room.",
	      MYCMD.if_flag ? " then " : "",
	      obj_proto[MYCMD.arg2].short_description,
	      cyn, obj_index[MYCMD.arg2].vnum, yel
	      );
      break;
    case 'D':
      sprintf(buf2, "%sSet door %s as %s.",
	      MYCMD.if_flag ? " then " : "",
	      dirs[MYCMD.arg2],
	      MYCMD.arg3 ? ((MYCMD.arg3 == 1) ? "closed" : "locked") : "open"
	      );
      break;
    default:
      strcpy(buf2, "<Unknown Command>");
      break;
    }
    /*
     * Build the display buffer for this command  
     */
    sprintf(buf1, "%s%d - %s%s\r\n", nrm, counter++, yel, buf2);
    strcat(buf, buf1);
    subcmd++;
  }
  /*
   * Finish off menu  
   */
  sprintf(buf1,
	  "%s%d - <END OF LIST>\r\n"
	  "%sN%s) New command.\r\n"
	  "%sE%s) Edit a command.\r\n"
	  "%sD%s) Delete a command.\r\n"
	  "%sQ%s) Quit\r\nEnter your choice : ",
	  nrm, counter, grn, nrm, grn, nrm, grn, nrm, grn, nrm
	  );

  strcat(buf, buf1);
  SEND_TO_Q(buf, d);

  OLC_MODE(d) = ZEDIT_MAIN_MENU;
}

/*-------------------------------------------------------------------*/

/*
 * Print the command type menu and setup response catch. 
 */
void zedit_disp_comtype(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);
  sprintf(buf,
	"%sM%s) Load Mobile to room             %sO%s) Load Object to room\r\n"
	"%sE%s) Equip mobile with object        %sG%s) Give an object to a mobile\r\n"
	"%sP%s) Put object in another object    %sD%s) Open/Close/Lock a Door\r\n"
	"%sR%s) Remove an object from the room\r\n"
	"What sort of command will this be? : ",
	grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm,
	grn, nrm, grn, nrm
	);
  SEND_TO_Q(buf, d);
  OLC_MODE(d) = ZEDIT_COMMAND_TYPE;
}

/*-------------------------------------------------------------------*/

/*
 * Print the appropriate message for the command type for arg1 and set
 * up the input catch clause  
 */
void zedit_disp_arg1(struct descriptor_data *d)
{
  switch (OLC_CMD(d).command) {
  case 'M':
    SEND_TO_Q("Input mob's vnum : ", d);
    OLC_MODE(d) = ZEDIT_ARG1;
    break;
  case 'O':
  case 'E':
  case 'P':
  case 'G':
    SEND_TO_Q("Input object vnum : ", d);
    OLC_MODE(d) = ZEDIT_ARG1;
    break;
  case 'D':
  case 'R':
    /*
     * Arg1 for these is the room number, skip to arg2  
     */
    OLC_CMD(d).arg1 = real_room(OLC_NUM(d));
    zedit_disp_arg2(d);
    break;
  default:
    /*
     * We should never get here.
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog("SYSERR: OLC: zedit_disp_arg1(): Help!", BRF, LVL_BUILDER, TRUE);
    SEND_TO_Q("Oops...\r\n", d);
    return;
  }
}

/*-------------------------------------------------------------------*/

/*
 * Print the appropriate message for the command type for arg2 and set
 * up the input catch clause.
 */
void zedit_disp_arg2(struct descriptor_data *d)
{
  int i;

  switch (OLC_CMD(d).command) {
  case 'M':
  case 'O':
  case 'E':
  case 'P':
  case 'G':
    SEND_TO_Q("Input the maximum number that can exist on the mud : ", d);
    break;
  case 'D':
    for (i = 0; *dirs[i] != '\n'; i++) {
      sprintf(buf, "%d) Exit %s.\r\n", i, dirs[i]);
      SEND_TO_Q(buf, d);
    }
    SEND_TO_Q("Enter exit number for door : ", d);
    break;
  case 'R':
    SEND_TO_Q("Input object's vnum : ", d);
    break;
  default:
    /*
     * We should never get here, but just in case...
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog("SYSERR: OLC: zedit_disp_arg2(): Help!", BRF, LVL_BUILDER, TRUE);
    SEND_TO_Q("Oops...\r\n", d);
    return;
  }
  OLC_MODE(d) = ZEDIT_ARG2;
}

void zedit_disp_arg4(struct descriptor_data *d)
{

  switch (OLC_CMD(d).command) {
  case 'M':
  case 'O':
  case 'E':
  case 'G':
    SEND_TO_Q("Enter The Load % : ", d);
    break;
  case 'P':
  case 'D':
  case 'R':
    break;
  default:
    /*
     * We should never get here, but just in case...
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog("SYSERR: OLC: zedit_disp_arg4(): Help!", BRF, LVL_BUILDER, TRUE);
    SEND_TO_Q("Oops...\r\n", d);
    return;
  }
  OLC_MODE(d) = ZEDIT_ARG4;
}


/*-------------------------------------------------------------------*/

/*
 * Print the appropriate message for the command type for arg3 and set
 * up the input catch clause.
 */
void zedit_disp_arg3(struct descriptor_data *d)
{
  int i = 0;

  switch (OLC_CMD(d).command) {
  case 'E':
    while (*equipment_types[i] != '\n') {
      sprintf(buf, "%2d) %26.26s %2d) %26.26s\r\n", i,
	   equipment_types[i], i + 1, (*equipment_types[i + 1] != '\n') ?
	      equipment_types[i + 1] : "");
      SEND_TO_Q(buf, d);
      if (*equipment_types[i + 1] != '\n')
	i += 2;
      else
	break;
    }
    SEND_TO_Q("Location to equip : ", d);
    break;
  case 'P':
    SEND_TO_Q("Virtual number of the container : ", d);
    break;
  case 'D':
    SEND_TO_Q(	"0)  Door open\r\n"
		"1)  Door closed\r\n"
		"2)  Door locked\r\n"
		"Enter state of the door : ", d);
    break;
  case 'M':
  case 'O':
  case 'R':
  case 'G':
  default:
    /*
     * We should never get here, just in case.
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog("SYSERR: OLC: zedit_disp_arg3(): Help!", BRF, LVL_BUILDER, TRUE);
    SEND_TO_Q("Oops...\r\n", d);
    return;
  }
  OLC_MODE(d) = ZEDIT_ARG3;
}

/**************************************************************************
  The GARGANTAUN event handler
 **************************************************************************/

void zedit_parse(struct descriptor_data *d, char *arg)
{
  int pos, i = 0;

  switch (OLC_MODE(d)) {
/*-------------------------------------------------------------------*/
  case ZEDIT_CONFIRM_SAVESTRING:
    switch (*arg) {
    case 'y':
    case 'Y':
      /*
       * Save the zone in memory, hiding invisible people.
       */
      SEND_TO_Q("Saving zone info in memory.\r\n", d);
      zedit_save_internally(d);
      sprintf(buf, "OLC: %s edits zone info for room %d.", GET_NAME(d->character), OLC_NUM(d));
      mudlog(buf, CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE);
      /* FALL THROUGH */
    case 'n':
    case 'N':
      cleanup_olc(d, CLEANUP_ALL);
      break;
    default:
      SEND_TO_Q("Invalid choice!\r\n", d);
      SEND_TO_Q("Do you wish to save the zone info? : ", d);
      break;
    }
    break;
   /* End of ZEDIT_CONFIRM_SAVESTRING */

/*-------------------------------------------------------------------*/
  case ZEDIT_MAIN_MENU:
    switch (*arg) {
    case 'q':
    case 'Q':
      if (OLC_ZONE(d)->age || OLC_ZONE(d)->number) {
	SEND_TO_Q("Do you wish to save the changes to the zone info? (y//n) : ", d);
	OLC_MODE(d) = ZEDIT_CONFIRM_SAVESTRING;
      } else {
	SEND_TO_Q("No changes made.\r\n", d);
	cleanup_olc(d, CLEANUP_ALL);
      }
      break;
    case 'n':
    case 'N':
      /*
       * New entry.
       */
      SEND_TO_Q("What number in the list should the new command be? : ", d);
      OLC_MODE(d) = ZEDIT_NEW_ENTRY;
      break;
    case 'e':
    case 'E':
      /*
       * Change an entry.
       */
      SEND_TO_Q("Which command do you wish to change? : ", d);
      OLC_MODE(d) = ZEDIT_CHANGE_ENTRY;
      break;
    case 'd':
    case 'D':
      /*
       * Delete an entry.
       */
      SEND_TO_Q("Which command do you wish to delete? : ", d);
      OLC_MODE(d) = ZEDIT_DELETE_ENTRY;
      break;
    case 'z':
    case 'Z':
      /*
       * Edit zone name.
       */
      SEND_TO_Q("Enter new zone name : ", d);
      OLC_MODE(d) = ZEDIT_ZONE_NAME;
      break;
    case 't':
    case 'T':
      /*
       * Edit top of zone.
       */
      if (GET_LEVEL(d->character) < LVL_IMPL)
	zedit_disp_menu(d);
      else {
	SEND_TO_Q("Enter new top of zone : ", d);
	OLC_MODE(d) = ZEDIT_ZONE_TOP;
      }
      break;
    case 'l':
    case 'L':
      /*
       * Edit zone lifespan.
       */
      SEND_TO_Q("Enter new zone lifespan : ", d);
      OLC_MODE(d) = ZEDIT_ZONE_LIFE;
      break;
    case 'r':
    case 'R':
      /*
       * Edit zone reset mode.
       */
      SEND_TO_Q("\r\n"
		"0) Never reset\r\n"
		"1) Reset only when no players in zone\r\n"
		"2) Normal reset\r\n"
		"Enter new zone reset type : ", d);
      OLC_MODE(d) = ZEDIT_ZONE_RESET;
      break;
    default:
      zedit_disp_menu(d);
      break;
    }
    break;
    /* End of ZEDIT_MAIN_MENU */

/*-------------------------------------------------------------------*/
  case ZEDIT_NEW_ENTRY:
    /*
     * Get the line number and insert the new line.
     */
    pos = atoi(arg);
    if (isdigit(*arg) && new_command(OLC_ZONE(d), pos)) {
      if (start_change_command(d, pos)) {
	zedit_disp_comtype(d);
	OLC_ZONE(d)->age = 1;
      }
    } else
      zedit_disp_menu(d);
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_DELETE_ENTRY:
    /*
     * Get the line number and delete the line.
     */
    pos = atoi(arg);
    if (isdigit(*arg)) {
      delete_command(OLC_ZONE(d), pos);
      OLC_ZONE(d)->age = 1;
    }
    zedit_disp_menu(d);
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_CHANGE_ENTRY:
    /*
     * Parse the input for which line to edit, and goto next quiz.
     */
    pos = atoi(arg);
    if (isdigit(*arg) && start_change_command(d, pos)) {
      zedit_disp_comtype(d);
      OLC_ZONE(d)->age = 1;
    } else
      zedit_disp_menu(d);
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_COMMAND_TYPE:
    /*
     * Parse the input for which type of command this is, and goto next
     * quiz.
     */
    OLC_CMD(d).command = toupper(*arg);
    if (!OLC_CMD(d).command || (strchr("MOPEDGR", OLC_CMD(d).command) == NULL)) {
      SEND_TO_Q("Invalid choice, try again : ", d);
    } else {
      if (OLC_VAL(d)) {	/* If there was a previous command. */
	SEND_TO_Q("Is this command dependent on the success of the previous one? (y//n)\r\n", d);
	OLC_MODE(d) = ZEDIT_IF_FLAG;
      } else {	/* 'if-flag' not appropriate. */
	OLC_CMD(d).if_flag = 0;
	zedit_disp_arg1(d);
      }
    }
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_IF_FLAG:
    /*
     * Parse the input for the if flag, and goto next quiz.
     */
    switch (*arg) {
    case 'y':
    case 'Y':
      OLC_CMD(d).if_flag = 1;
      break;
    case 'n':
    case 'N':
      OLC_CMD(d).if_flag = 0;
      break;
    default:
      SEND_TO_Q("Try again : ", d);
      return;
    }
    zedit_disp_arg1(d);
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_ARG1:
    /*
     * Parse the input for arg1, and goto next quiz.
     */
    if (!isdigit(*arg)) {
      SEND_TO_Q("Must be a numeric value, try again : ", d);
      return;
    }
    switch (OLC_CMD(d).command) {
    case 'M':
      if ((pos = real_mobile(atoi(arg))) >= 0) {
	OLC_CMD(d).arg1 = pos;
	zedit_disp_arg2(d);
      } else
	SEND_TO_Q("That mobile does not exist, try again : ", d);
      break;
    case 'O':
    case 'P':
    case 'E':
    case 'G':
      if ((pos = real_object(atoi(arg))) >= 0) {
	OLC_CMD(d).arg1 = pos;
	zedit_disp_arg2(d);
      } else
	SEND_TO_Q("That object does not exist, try again : ", d);
      break;
    case 'D':
    case 'R':
    default:
      /*
       * We should never get here.
       */
      cleanup_olc(d, CLEANUP_ALL);
      mudlog("SYSERR: OLC: zedit_parse(): case ARG1: Ack!", BRF, LVL_BUILDER, TRUE);
      SEND_TO_Q("Oops...\r\n", d);
      break;
    }
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_ARG2:
    /*
     * Parse the input for arg2, and goto next quiz.
     */
    if (!isdigit(*arg)) {
      SEND_TO_Q("Must be a numeric value, try again : ", d);
      return;
    }
    switch (OLC_CMD(d).command) {
    case 'M':
    case 'O':
      OLC_CMD(d).arg2 = atoi(arg);
      OLC_CMD(d).arg3 = real_room(OLC_NUM(d));
      zedit_disp_arg4(d);
      break;
    case 'G':
      OLC_CMD(d).arg2 = atoi(arg);
      zedit_disp_arg4(d);
      break;
    case 'P':
    case 'E':
      OLC_CMD(d).arg2 = atoi(arg);
      zedit_disp_arg3(d);
      break;
    case 'D':
      pos = atoi(arg);
      /*
       * Count directions.
       */
      if (pos < 0 || pos > NUM_OF_DIRS)
	SEND_TO_Q("Try again : ", d);
      else {
	OLC_CMD(d).arg2 = pos;
	zedit_disp_arg3(d);
      }
      break;
    case 'R':
      if ((pos = real_object(atoi(arg))) >= 0) {
	OLC_CMD(d).arg2 = pos;
	zedit_disp_menu(d);
      } else
	SEND_TO_Q("That object does not exist, try again : ", d);
      break;
    default:
      /*
       * We should never get here, but just in case...
       */
      cleanup_olc(d, CLEANUP_ALL);
      mudlog("SYSERR: OLC: zedit_parse(): case ARG2: Ack!", BRF, LVL_BUILDER, TRUE);
      SEND_TO_Q("Oops...\r\n", d);
      break;
    }
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_ARG3:
    /*
     * Parse the input for arg3, and goto arg4's menu.
     */
    if (!isdigit(*arg)) {
      SEND_TO_Q("Must be a numeric value, try again : ", d);
      return;
    }
    switch (OLC_CMD(d).command) {
    case 'E':
      pos = atoi(arg);
      /*
       * Count number of wear positions.  We could use NUM_WEARS, this is
       * more reliable.
       */
      while (*equipment_types[i] != '\n')
	i++;
      if (pos < 0 || pos > i)
	SEND_TO_Q("Try again : ", d);
      else {
	OLC_CMD(d).arg3 = pos;
        zedit_disp_arg4(d);
      }
      break;
    case 'P':
      if ((pos = real_object(atoi(arg))) >= 0) {
	OLC_CMD(d).arg3 = pos;
        zedit_disp_arg4(d);
      } else
	SEND_TO_Q("That object does not exist, try again : ", d);
      break;
    case 'D':
      pos = atoi(arg);
      if (pos < 0 || pos > 2)
	SEND_TO_Q("Try again : ", d);
      else {
	OLC_CMD(d).arg3 = pos;
        zedit_disp_arg4(d);
      }
      break;
    case 'M':
    case 'O':
    case 'G':
    case 'R':
    default:
      /*
       * We should never get here, but just in case...
       */
      cleanup_olc(d, CLEANUP_ALL);
      mudlog("SYSERR: OLC: zedit_parse(): case ARG3: Ack!", BRF, LVL_BUILDER, TRUE);
      SEND_TO_Q("Oops...\r\n", d);
      break;
    }
    break;

  case ZEDIT_ARG4:
    /*
     * Parse the input for arg4, and go back to main menu.
     */
    if (!isdigit(*arg)) {
      SEND_TO_Q("Must be a numeric value, try again : ", d);
      return;
    }
    switch (OLC_CMD(d).command) {
    case 'M':
    case 'O':
    case 'G':
    case 'E':
      pos = atoi(arg);
      if (pos < 0 || pos > 100)
        SEND_TO_Q("Try again : ", d);
      else {
        OLC_CMD(d).arg4 = pos;
        zedit_disp_menu(d);
      }
      break;

    case 'P':
    case 'D':
        OLC_CMD(d).arg3 = 100;
      break;

    default:
      /*

       * We should never get here, but just in case...
       */
      cleanup_olc(d, CLEANUP_ALL);
      mudlog("SYSERR: OLC: zedit_parse(): case ARG3: Ack!", BRF, LVL_BUILDER, TRUE);
      SEND_TO_Q("Oops...\r\n", d);
      break;
    }
    break;


/*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_NAME:
    /*
     * Add new name and return to main menu.
     */
    if (genolc_checkstring(d, arg)) {
      if (OLC_ZONE(d)->name)
        free(OLC_ZONE(d)->name);
      else
        log("SYSERR: OLC: ZEDIT_ZONE_NAME: no name to free!");
      OLC_ZONE(d)->name = str_dup(arg);
      OLC_ZONE(d)->number = 1;
    }
    zedit_disp_menu(d);
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_RESET:
    /*
     * Parse and add new reset_mode and return to main menu.
     */
    pos = atoi(arg);
    if (!isdigit(*arg) || pos < 0 || pos > 2)
      SEND_TO_Q("Try again (0-2) : ", d);
    else {
      OLC_ZONE(d)->reset_mode = pos;
      OLC_ZONE(d)->number = 1;
      zedit_disp_menu(d);
    }
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_LIFE:
    /*
     * Parse and add new lifespan and return to main menu.
     */
    pos = atoi(arg);
    if (!isdigit(*arg) || pos < 0 || pos > 240)
      SEND_TO_Q("Try again (0-240) : ", d);
    else {
      OLC_ZONE(d)->lifespan = pos;
      OLC_ZONE(d)->number = 1;
      zedit_disp_menu(d);
    }
    break;

/*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_TOP:
    /*
     * Parse and add new top room in zone and return to main menu.
     */
    if (OLC_ZNUM(d) == top_of_zone_table)
      OLC_ZONE(d)->top = LIMIT(atoi(arg), OLC_ZNUM(d) * 100, 32000);
    else
      OLC_ZONE(d)->top = LIMIT(atoi(arg), OLC_ZNUM(d) * 100, zone_table[OLC_ZNUM(d) + 1].number * 100);
    zedit_disp_menu(d);
    break;

/*-------------------------------------------------------------------*/
  default:
    /*
     * We should never get here, but just in case...
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog("SYSERR: OLC: zedit_parse(): Reached default case!", BRF, LVL_BUILDER, TRUE);
    SEND_TO_Q("Oops...\r\n", d);
    break;
  }
}

/*
 * End of parse_zedit()  
 */
