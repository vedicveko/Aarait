/************************************************************************
 * OasisOLC - Mobiles / medit.c					v2.0	*
 * Copyright 1996 Harvey Gilpin						*
 * Copyright 1997-1999 George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "interpreter.h"
#include "comm.h"
#include "spells.h"
#include "utils.h"
#include "db.h"
#include "shop.h"
#include "genolc.h"
#include "genmob.h"
#include "genzon.h"
#include "genshp.h"
#include "oasis.h"
#include "handler.h"
#include "constants.h"
#include "improved-edit.h"
#include "coins.h"

/*-------------------------------------------------------------------*/

/*
 * External variable declarations.
 */
extern struct index_data *mob_index;
extern struct char_data *mob_proto;
extern struct char_data *character_list;
extern mob_rnum top_of_mobt;
extern struct zone_data *zone_table;
extern struct attack_hit_type attack_hit_text[];
extern struct shop_data *shop_index;
extern struct descriptor_data *descriptor_list;
#if CONFIG_OASIS_MPROG
extern const char *mobprog_types[];
#endif

/*-------------------------------------------------------------------*/

/*
 * Handy internal macros.
 */
#if CONFIG_OASIS_MPROG
#define GET_MPROG(mob)		(mob_index[(mob)->nr].mobprogs)
#define GET_MPROG_TYPE(mob)	(mob_index[(mob)->nr].progtypes)
#endif

/*-------------------------------------------------------------------*/

/*
 * Function prototypes.
 */
#if CONFIG_OASIS_MPROG
void medit_disp_mprog(struct descriptor_data *d);
void medit_change_mprog(struct descriptor_data *d);
const char *medit_get_mprog_type(struct mob_prog_data *mprog);
#endif

/*-------------------------------------------------------------------*\
  utility functions 
\*-------------------------------------------------------------------*/

/* Mob exp for autostatting */
const int mob_exp[] = {
   25,  100,  200,  350,  600,  900,  1500,  2250,  3750,  6000,
   9000,  11000,  13000,  16000,  18000,  21000,  24000,  28000,  30000,  35000,
   40000,  50000,   60000,   80000,   100000,  130000,  155000, 200000, 310000, 
   450000, 600000, 800000, 1000000, 2000000, 3000000
};

const int mob_hp[][3]={
// 0d0
// RACE_NPC_OTHER
{1, 10, 0},
// RACE_NPC_ANIMAL
{3, 8, 9},
// RACE_NPC_AVIAN
{1, 8, 0},
// RACE_NPC_MAMMAL
{3, 8, 9},
// RACE_NPC_ANPHIB
{1, 4, 0},
// RACE_NPC_REPTILE
{3, 8, 9},
// RACE_NPC_INSECT
{2, 8, 4},
// RACE_NPC_HIGHHUMAN
{13, 13, 0},
// RACE_NPC_SKELETON
{1, 12, 0},
// RACE_NPC_DRAGON
{24, 12, 120},
// RACE_NPC_GOBLIN
{1, 8, 0},
// RACE_NPC_PIG
{2, 13, 0},
// RACE_NPC_WOLF
{2, 12, 0},
// RACE_NPC_CRAB
{13, 13, 0},
// RACE_NPC_FISH
{13, 13, 0}
};

const int mob_stats[][6] = {
// INT, WIS, STR, DEX, CON, CHA
// RACE_NPC_OTHER 
{13, 13, 13, 13, 13, 13},
// RACE_NPC_ANIMAL
{2, 13, 15, 10, 17, 4},
// RACE_NPC_AVIAN 
{2, 14, 6, 17, 10, 4},
// RACE_NPC_MAMMAL
{2, 13, 15, 10, 17, 4},
// RACE_NPC_ANPHIB
{1, 14, 1, 12, 11, 4},
// RACE_NPC_REPTILE
{2, 12, 17, 15, 17, 2},
// RACE_NPC_INSECT
{1, 10, 9, 10, 14, 2},
// RACE_NPC_HIGHHUMAN
{11, 11, 11, 11, 11, 11},
// RACE_NPC_SKELETON
{1, 10, 10, 12, 1, 11},
// RACE_NPC_DRAGON
{18, 19, 31, 10, 23, 18},
// RACE_NPC_GOBLIN
{10, 11, 8, 13, 11, 8},
// RACE_NPC_PIG
{2, 13, 15, 10, 17, 4},
// RACE_NPC_WOLF
{2, 12, 13, 15, 15, 6},
// RACE_NPC_CRAB
{13, 13, 13, 13, 13, 13},
// RACE_NPC_FISH
{13, 13, 13, 13, 13, 13},
};


void medit_save_to_disk(zone_vnum foo)
{
  save_mobiles(real_zone(foo));
}

void medit_setup_new(struct descriptor_data *d)
{
  struct char_data *mob;

  /*
   * Allocate a scratch mobile structure.  
   */
  CREATE(mob, struct char_data, 1);

  init_mobile(mob);

  GET_MOB_RNUM(mob) = -1;
  /*
   * Set up some default strings.
   */
  GET_ALIAS(mob) = str_dup("mob unfinished");
  GET_SDESC(mob) = str_dup("the unfinished mob");
  GET_LDESC(mob) = str_dup("An unfinished mob stands here.\r\n");
  GET_DDESC(mob) = str_dup("It looks unfinished.\r\n");
#if CONFIG_OASIS_MPROG
  OLC_MPROGL(d) = NULL;
  OLC_MPROG(d) = NULL;
#endif

  OLC_MOB(d) = mob;
  /* Has changed flag. (It hasn't so far, we just made it.) */
  OLC_VAL(d) = FALSE;

  medit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

void medit_setup_existing(struct descriptor_data *d, int rmob_num)
{
  struct char_data *mob;

  /*
   * Allocate a scratch mobile structure. 
   */
  CREATE(mob, struct char_data, 1);

  copy_mobile(mob, mob_proto + rmob_num);

#if CONFIG_OASIS_MPROG
  {
    MPROG_DATA *temp;
    MPROG_DATA *head;

    if (GET_MPROG(mob))
      CREATE(OLC_MPROGL(d), MPROG_DATA, 1);
    head = OLC_MPROGL(d);
    for (temp = GET_MPROG(mob); temp; temp = temp->next) {
      OLC_MPROGL(d)->type = temp->type;
      OLC_MPROGL(d)->arglist = str_dup(temp->arglist);
      OLC_MPROGL(d)->comlist = str_dup(temp->comlist);
      if (temp->next) {
        CREATE(OLC_MPROGL(d)->next, MPROG_DATA, 1);
        OLC_MPROGL(d) = OLC_MPROGL(d)->next;
      }
    }
    OLC_MPROGL(d) = head;
    OLC_MPROG(d) = OLC_MPROGL(d);
  }
#endif

  OLC_MOB(d) = mob;
  medit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

/*
 * Ideally, this function should be in db.c, but I'll put it here for
 * portability.
 */
void init_mobile(struct char_data *mob)
{
  clear_char(mob);

  GET_HIT(mob) = GET_MANA(mob) = 1;
  GET_MAX_MANA(mob) = GET_MAX_MOVE(mob) = 100;
  GET_NDD(mob) = GET_SDD(mob) = 1;
  GET_WEIGHT(mob) = 200;
  GET_HEIGHT(mob) = 198;
  GET_MOB_WEIGHT(mob) = 1;

  mob->real_abils.str = mob->real_abils.intel = mob->real_abils.wis = 11;
  mob->real_abils.dex = mob->real_abils.con = mob->real_abils.cha = 11;
  mob->aff_abils = mob->real_abils;

  SET_BIT(MOB_FLAGS(mob), MOB_ISNPC);
  mob->player_specials = &dummy_mob;
}

/*-------------------------------------------------------------------*/

/*
 * Save new/edited mob to memory.
 */
void medit_save_internally(struct descriptor_data *d)
{
  int i;
  mob_rnum new_rnum;
  struct descriptor_data *dsc;

  i = (real_mobile(OLC_NUM(d)) == NOBODY);

  if ((new_rnum = add_mobile(OLC_MOB(d), OLC_NUM(d))) < 0) {
    log("medit_save_internally: add_object failed.");
    return;
  }

  if (!i)	/* Only renumber on new mobiles. */
    return;

  /*
   * Update keepers in shops being edited and other mobs being edited.
   */
  for (dsc = descriptor_list; dsc; dsc = dsc->next) {
    if (STATE(dsc) == CON_SEDIT)
      S_KEEPER(OLC_SHOP(dsc)) += (S_KEEPER(OLC_SHOP(dsc)) >= new_rnum);
    else if (STATE(dsc) == CON_MEDIT)
      GET_MOB_RNUM(OLC_MOB(dsc)) += (GET_MOB_RNUM(OLC_MOB(dsc)) >= new_rnum);
  }

  /*
   * Update other people in zedit too. From: C.Raehl 4/27/99
   */
  for (dsc = descriptor_list; dsc; dsc = dsc->next)
    if (STATE(dsc) == CON_ZEDIT)
      for (i = 0; OLC_ZONE(dsc)->cmd[i].command != 'S'; i++)
        if (OLC_ZONE(dsc)->cmd[i].command == 'M')
          if (OLC_ZONE(dsc)->cmd[i].arg1 >= new_rnum)
            OLC_ZONE(dsc)->cmd[i].arg1++;
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

/*
 * Display positions. (sitting, standing, etc)
 */
void medit_disp_positions(struct descriptor_data *d)
{
  int i;

  get_char_colors(d->character);
  clear_screen(d);

  for (i = 0; *position_types[i] != '\n'; i++) {
    sprintf(buf, "%s%2d%s) %s\r\n", grn, i, nrm, position_types[i]);
    SEND_TO_Q(buf, d);
  }
  SEND_TO_Q("Enter position number : ", d);
}

/*-------------------------------------------------------------------*/

#if CONFIG_OASIS_MPROG
/*
 * Get the type of MobProg.
 */
const char *medit_get_mprog_type(struct mob_prog_data *mprog)
{
  switch (mprog->type) {
  case IN_FILE_PROG:	return ">in_file_prog";
  case ACT_PROG:	return ">act_prog";
  case SPEECH_PROG:	return ">speech_prog";
  case RAND_PROG:	return ">rand_prog";
  case FIGHT_PROG:	return ">fight_prog";
  case HITPRCNT_PROG:	return ">hitprcnt_prog";
  case DEATH_PROG:	return ">death_prog";
  case ENTRY_PROG:	return ">entry_prog";
  case GREET_PROG:	return ">greet_prog";
  case ALL_GREET_PROG:	return ">all_greet_prog";
  case GIVE_PROG:	return ">give_prog";
  case BRIBE_PROG:	return ">bribe_prog";
  }
  return ">ERROR_PROG";
}

/*-------------------------------------------------------------------*/

/*
 * Display the MobProgs.
 */
void medit_disp_mprog(struct descriptor_data *d)
{
  struct mob_prog_data *mprog = OLC_MPROGL(d);

  OLC_MTOTAL(d) = 1;

  clear_screen(d);
  while (mprog) {
    sprintf(buf, "%d) %s %s\r\n", OLC_MTOTAL(d), medit_get_mprog_type(mprog),
		(mprog->arglist ? mprog->arglist : "NONE"));
    SEND_TO_Q(buf, d);
    OLC_MTOTAL(d)++;
    mprog = mprog->next;
  }
  sprintf(buf,  "%d) Create New Mob Prog\r\n"
		"%d) Purge Mob Prog\r\n"
		"Enter number to edit [0 to exit]:  ",
		OLC_MTOTAL(d), OLC_MTOTAL(d) + 1);
  SEND_TO_Q(buf, d);
  OLC_MODE(d) = MEDIT_MPROG;
}

/*-------------------------------------------------------------------*/

/*
 * Change the MobProgs.
 */
void medit_change_mprog(struct descriptor_data *d)
{
  clear_screen(d);
  sprintf(buf,  "1) Type: %s\r\n"
		"2) Args: %s\r\n"
		"3) Commands:\r\n%s\r\n\r\n"
		"Enter number to edit [0 to exit]: ",
	medit_get_mprog_type(OLC_MPROG(d)),
	(OLC_MPROG(d)->arglist ? OLC_MPROG(d)->arglist: "NONE"),
	(OLC_MPROG(d)->comlist ? OLC_MPROG(d)->comlist : "NONE"));

  SEND_TO_Q(buf, d);
  OLC_MODE(d) = MEDIT_CHANGE_MPROG;
}

/*-------------------------------------------------------------------*/

/*
 * Change the MobProg type.
 */
void medit_disp_mprog_types(struct descriptor_data *d)
{
  int i;

  get_char_colors(d->character);
  clear_screen(d);

  for (i = 0; i < NUM_PROGS-1; i++) {
    sprintf(buf, "%s%2d%s) %s\r\n", grn, i, nrm, mobprog_types[i]);
    SEND_TO_Q(buf, d);
  }
  SEND_TO_Q("Enter mob prog type : ", d);
  OLC_MODE(d) = MEDIT_MPROG_TYPE;
}
#endif

/*-------------------------------------------------------------------*/

/*
 * Display the gender of the mobile.
 */
void medit_disp_sex(struct descriptor_data *d)
{
  int i;

  get_char_colors(d->character);
  clear_screen(d);

  for (i = 0; i < NUM_GENDERS; i++) {
    sprintf(buf, "%s%2d%s) %s\r\n", grn, i, nrm, genders[i]);
    SEND_TO_Q(buf, d);
  }
  SEND_TO_Q("Enter gender number : ", d);
}

/*-------------------------------------------------------------------*/

/*
 * Display attack types menu.
 */
void medit_disp_attack_types(struct descriptor_data *d)
{
  int i;

  get_char_colors(d->character);
  clear_screen(d);

  for (i = 0; i < NUM_ATTACK_TYPES; i++) {
    sprintf(buf, "%s%2d%s) %s\r\n", grn, i, nrm, attack_hit_text[i].singular);
    SEND_TO_Q(buf, d);
  }
  SEND_TO_Q("Enter attack type : ", d);
}

/*-------------------------------------------------------------------*/

/*
 * Display mob-flags menu.
 */
void medit_disp_mob_flags(struct descriptor_data *d)
{
  int i, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);
  for (i = 0; i < NUM_MOB_FLAGS; i++) {
    sprintf(buf, "%s%2d%s) %-20.20s  %s", grn, i + 1, nrm, action_bits[i],
		!(++columns % 2) ? "\r\n" : "");
    SEND_TO_Q(buf, d);
  }
  sprintbit(MOB_FLAGS(OLC_MOB(d)), action_bits, buf1);
  sprintf(buf, "\r\nCurrent flags : %s%s%s\r\nEnter mob flags (0 to quit) : ",
		  cyn, buf1, nrm);
  SEND_TO_Q(buf, d);
}

/*-------------------------------------------------------------------*/

/*
 * Display affection flags menu.
 */
void medit_disp_aff_flags(struct descriptor_data *d)
{
  int i, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);
  for (i = 0; i < NUM_AFF_FLAGS; i++) {
    sprintf(buf, "%s%2d%s) %-20.20s  %s", grn, i + 1, nrm, affected_bits[i],
			!(++columns % 2) ? "\r\n" : "");
    SEND_TO_Q(buf, d);
  }
  sprintbit(AFF_FLAGS(OLC_MOB(d)), affected_bits, buf1);
  sprintf(buf, "\r\nCurrent flags   : %s%s%s\r\nEnter aff flags (0 to quit) : ",
			  cyn, buf1, nrm);
  SEND_TO_Q(buf, d);
}

/*-------------------------------------------------------------------*/

/*
 * Display Class menu.
 */
void medit_disp_class_flags(struct descriptor_data *d)
{
  int i, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);
  for (i = 0; i < NUM_NPC_CLASS; i++) {
    sprintf(buf, "%s%2d%s) %-20.20s  %s", grn, i, nrm, npc_class_types[i],
                        !(++columns % 3) ? "\r\n" : "");
    SEND_TO_Q(buf, d);
  }
  sprintf(buf, "\r\nEnter class type: "); 
  SEND_TO_Q(buf, d);
}

/*
 * Display Race menu.
 */
void medit_disp_race_flags(struct descriptor_data *d)
{
  int i, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);
  for (i = 0; i < NUM_NPC_RACE; i++) {
    sprintf(buf, "%s%2d%s) %-20.20s  %s", grn, i, nrm, npc_race_types[i],
                        !(++columns % 3) ? "\r\n" : "");
    SEND_TO_Q(buf, d);
  }
  sprintf(buf, "\r\nEnter race type: "); 
  SEND_TO_Q(buf, d);
}

void medit_disp_size_flags(struct descriptor_data *d)
{

  get_char_colors(d->character);
  clear_screen(d);
  sprintf(buf, "8 ) FINE	4 ) DIMINUTIVE		2 ) TINY\r\n"
               "1 ) SMALL	0 ) MEDIUM		-1) LARGE\r\n"
               "-2) HUGE	-4) GARGANTUAN		-8) COLOSSAL\r\n");
  SEND_TO_Q(buf, d);
  sprintf(buf, "\r\nEnter Size: ");
  SEND_TO_Q(buf, d);
}

/*-------------------------------------------------------------------*/

/*
 * Display main menu.
 */
void medit_disp_menu(struct descriptor_data *d)
{
  struct char_data *mob;

  mob = OLC_MOB(d);
  get_char_colors(d->character);
  clear_screen(d);

  sprintf(buf,
	  "-- Mob Number:  [%s%d%s]\r\n"
	  "%s1%s) Sex: %s%-7.7s%s	         %s2%s) Alias: %s%s\r\n"
	  "%s3%s) S-Desc: %s%s\r\n"
	  "%s4%s) L-Desc:-\r\n%s%s"
	  "%s5%s) D-Desc:-\r\n%s%s"
     "%s6%s) Level:       [%s%4d%s],  %s7%s) Alignment:    [%s%4d%s]\r\n"
     "%s8%s) HR//SKY:      [%s%4d%s],  %s9%s) DR//FD:        [%s%4d%s]\r\n"
     "%sA%s) NumDamDice:  [%s%4d%s],  %sB%s) SizeDamDice:  [%s%4d%s]\r\n"
	  "%sC%s) Num HP Dice: [%s%4d%s],  %sD%s) Size HP Dice: [%s%4d%s],  %sE%s) HP Bonus: [%s%5d%s]\r\n"
	  "%sF%s) Armor Class: [%s%4d%s],  %sG%s) Exp:     [%s%9d%s],  %sH%s) Gold:  [%s%8d%s]\r\n",

	  cyn, OLC_NUM(d), nrm,
	  grn, nrm, yel, genders[(int)GET_SEX(mob)], nrm,
	  grn, nrm, yel, GET_ALIAS(mob),
	  grn, nrm, yel, GET_SDESC(mob),
	  grn, nrm, yel, GET_LDESC(mob),
	  grn, nrm, yel, GET_DDESC(mob),
	  grn, nrm, cyn, GET_LEVEL(mob), nrm,
	  grn, nrm, cyn, GET_ALIGNMENT(mob), nrm,
	  grn, nrm, cyn, GET_HITROLL(mob), nrm,
	  grn, nrm, cyn, GET_DAMROLL(mob), nrm,
	  grn, nrm, cyn, GET_NDD(mob), nrm,
	  grn, nrm, cyn, GET_SDD(mob), nrm,
	  grn, nrm, cyn, GET_HIT(mob), nrm,
	  grn, nrm, cyn, GET_MANA(mob), nrm,
	  grn, nrm, cyn, GET_MOVE(mob), nrm,
	  grn, nrm, cyn, GET_AC(mob), nrm,
	  grn, nrm, cyn, GET_EXP(mob), nrm,
	  grn, nrm, cyn, convert_all_to_copper(mob), nrm
	  );
  SEND_TO_Q(buf, d);

  sprintbit(MOB_FLAGS(mob), action_bits, buf1);
  sprintbit(AFF_FLAGS(mob), affected_bits, buf2);
  sprintf(buf,
	  "%sI%s) Position  : %s%s\r\n"
	  "%sJ%s) Default   : %s%s\r\n"
	  "%sK%s) Attack    : %s%s\r\n"
          "%sN%s) Class     : %s%s\r\n"
          "%sR%s) Race      : %s%s\r\n"
          "%sS%s) Size      : %s%d\r\n"
          "%sW%s) Weight    : %s%d\r\n"
	  "%sL%s) NPC Flags : %s%s\r\n"
	  "%sM%s) AFF Flags : %s%s\r\n"
#if CONFIG_OASIS_MPROG
	  "%sP%s) Mob Progs : %s%s\r\n"
#endif
	  "%sQ%s) Quit\r\n"
	  "Enter choice : ",

	  grn, nrm, yel, position_types[(int)GET_POS(mob)],
	  grn, nrm, yel, position_types[(int)GET_DEFAULT_POS(mob)],
	  grn, nrm, yel, attack_hit_text[GET_ATTACK(mob)].singular,
          grn, nrm, cyn, npc_class_types[(int)GET_CLASS(mob)],
          grn, nrm, cyn, npc_race_types[(int)GET_RACE(mob)],
          grn, nrm, cyn, GET_MOB_SIZE(mob),
          grn, nrm, cyn, GET_MOB_WEIGHT(mob), 
	  grn, nrm, cyn, buf1,
	  grn, nrm, cyn, buf2,
#if CONFIG_OASIS_MPROG
	  grn, nrm, cyn, (OLC_MPROGL(d) ? "Set." : "Not Set."),
#endif
	  grn, nrm
	  );
  SEND_TO_Q(buf, d);

  OLC_MODE(d) = MEDIT_MAIN_MENU;
}

/************************************************************************
 *			The GARGANTAUN event handler			*
 ************************************************************************/

void medit_parse(struct descriptor_data *d, char *arg)
{
  int i = -1;
  char *oldtext = NULL;

  if (OLC_MODE(d) > MEDIT_NUMERICAL_RESPONSE) {
    i = atoi(arg);
    if (!*arg || (!isdigit(arg[0]) && ((*arg == '-') && !isdigit(arg[1])))) {
      SEND_TO_Q("Field must be numerical, try again : ", d);
      return;
    }
  } else {	/* String response. */
    if (!genolc_checkstring(d, arg))
      return;
  }
  switch (OLC_MODE(d)) {
/*-------------------------------------------------------------------*/
  case MEDIT_CONFIRM_SAVESTRING:
    /*
     * Ensure mob has MOB_ISNPC set or things will go pear shaped.
     */
    SET_BIT(MOB_FLAGS(OLC_MOB(d)), MOB_ISNPC);
    switch (*arg) {
    case 'y':
    case 'Y':
      /*
       * Save the mob in memory and to disk.
       */
      SEND_TO_Q("Saving mobile to memory.\r\n", d);
      medit_save_internally(d);
      sprintf(buf, "OLC: %s edits mob %d", GET_NAME(d->character), OLC_NUM(d));
      mudlog(buf, CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE);
      /* FALL THROUGH */
    case 'n':
    case 'N':
      cleanup_olc(d, CLEANUP_ALL);
      return;
    default:
      SEND_TO_Q("Invalid choice!\r\n", d);
      SEND_TO_Q("Do you wish to save the mobile? : ", d);
      return;
    }
    break;

/*-------------------------------------------------------------------*/
  case MEDIT_MAIN_MENU:
    i = 0;
    switch (*arg) {
    case 'q':
    case 'Q':
      if (OLC_VAL(d)) {	/* Anything been changed? */
	SEND_TO_Q("Do you wish to save the changes to the mobile? (y//n) : ", d);
	OLC_MODE(d) = MEDIT_CONFIRM_SAVESTRING;
      } else
	cleanup_olc(d, CLEANUP_ALL);
      return;
    case '1':
      OLC_MODE(d) = MEDIT_SEX;
      medit_disp_sex(d);
      return;
    case '2':
      OLC_MODE(d) = MEDIT_ALIAS;
      i--;
      break;
    case '3':
      OLC_MODE(d) = MEDIT_S_DESC;
      i--;
      break;
    case '4':
      OLC_MODE(d) = MEDIT_L_DESC;
      i--;
      break;
    case '5':
      OLC_MODE(d) = MEDIT_D_DESC;
      send_editor_help(d);
      SEND_TO_Q("Enter mob description:\r\n\r\n", d);
      if (OLC_MOB(d)->player.description) {
	SEND_TO_Q(OLC_MOB(d)->player.description, d);
	oldtext = str_dup(OLC_MOB(d)->player.description);
      }
      string_write(d, &OLC_MOB(d)->player.description, MAX_MOB_DESC, 0, oldtext);
      OLC_VAL(d) = 1;
      return;
    case '6':
      OLC_MODE(d) = MEDIT_LEVEL;
      i++;
      break;
    case '7':
      OLC_MODE(d) = MEDIT_ALIGNMENT;
      i++;
      break;
    case '8':
      OLC_MODE(d) = MEDIT_HITROLL;
      i++;
      break;
    case '9':
      OLC_MODE(d) = MEDIT_DAMROLL;
      i++;
      break;
    case 'a':
    case 'A':
      OLC_MODE(d) = MEDIT_NDD;
      i++;
      break;
    case 'b':
    case 'B':
      OLC_MODE(d) = MEDIT_SDD;
      i++;
      break;
    case 'c':
    case 'C':
      OLC_MODE(d) = MEDIT_NUM_HP_DICE;
      i++;
      break;
    case 'd':
    case 'D':
      OLC_MODE(d) = MEDIT_SIZE_HP_DICE;
      i++;
      break;
    case 'e':
    case 'E':
      OLC_MODE(d) = MEDIT_ADD_HP;
      i++;
      break;
    case 'f':
    case 'F':
      OLC_MODE(d) = MEDIT_AC;
      i++;
      break;
    case 'g':
    case 'G':
      OLC_MODE(d) = MEDIT_EXP;
      i++;
      break;
    case 'h':
    case 'H':
      OLC_MODE(d) = MEDIT_GOLD;
      i++;
      break;
    case 'i':
    case 'I':
      OLC_MODE(d) = MEDIT_POS;
      medit_disp_positions(d);
      return;
    case 'j':
    case 'J':
      OLC_MODE(d) = MEDIT_DEFAULT_POS;
      medit_disp_positions(d);
      return;
    case 'k':
    case 'K':
      OLC_MODE(d) = MEDIT_ATTACK;
      medit_disp_attack_types(d);
      return;
    case 'l':
    case 'L':
      OLC_MODE(d) = MEDIT_NPC_FLAGS;
      medit_disp_mob_flags(d);
      return;
    case 'm':
    case 'M':
      OLC_MODE(d) = MEDIT_AFF_FLAGS;
      medit_disp_aff_flags(d);
      return;
    case 'n':
    case 'N':
     OLC_MODE(d) = MEDIT_CLASS;
     medit_disp_class_flags(d);
     return;
    case 'r':
    case 'R':
     OLC_MODE(d) = MEDIT_RACE;
     medit_disp_race_flags(d);
     return;
    case 's':
    case 'S':
     OLC_MODE(d) = MEDIT_MSIZE;
     medit_disp_size_flags(d);
     return;
    case 'w':
    case 'W':
     OLC_MODE(d) = MEDIT_WEIGHT;
      send_to_char("Please enter the mobiles weight in pounds: ", d->character); 
     return;

#if CONFIG_OASIS_MPROG
    case 'p':
    case 'P':
      OLC_MODE(d) = MEDIT_MPROG;
      medit_disp_mprog(d);
      return;
#endif
    default:
      medit_disp_menu(d);
      return;
    }
    if (i == 0)
      break;
    else if (i == 1)
      SEND_TO_Q("\r\nEnter new value : ", d);
    else if (i == -1)
      SEND_TO_Q("\r\nEnter new text :\r\n] ", d);
    else
      SEND_TO_Q("Oops...\r\n", d);
    return;
/*-------------------------------------------------------------------*/
  case MEDIT_ALIAS:
    if (GET_ALIAS(OLC_MOB(d)))
      free(GET_ALIAS(OLC_MOB(d)));
    GET_ALIAS(OLC_MOB(d)) = str_udup(arg);
    break;
/*-------------------------------------------------------------------*/
  case MEDIT_S_DESC:
    if (GET_SDESC(OLC_MOB(d)))
      free(GET_SDESC(OLC_MOB(d)));
    GET_SDESC(OLC_MOB(d)) = str_udup(arg);
    break;
/*-------------------------------------------------------------------*/
  case MEDIT_L_DESC:
    if (GET_LDESC(OLC_MOB(d)))
      free(GET_LDESC(OLC_MOB(d)));
    if (arg && *arg) {
      strcpy(buf, arg);
      strcat(buf, "\r\n");
      GET_LDESC(OLC_MOB(d)) = str_dup(buf);
    } else
      GET_LDESC(OLC_MOB(d)) = str_dup("undefined");

    break;
/*-------------------------------------------------------------------*/
  case MEDIT_D_DESC:
    /*
     * We should never get here.
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog("SYSERR: OLC: medit_parse(): Reached D_DESC case!", BRF, LVL_BUILDER, TRUE);
    SEND_TO_Q("Oops...\r\n", d);
    break;
/*-------------------------------------------------------------------*/
#if CONFIG_OASIS_MPROG
  case MEDIT_MPROG_COMLIST:
    /*
     * We should never get here, but if we do, bail out.
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog("SYSERR: OLC: medit_parse(): Reached MPROG_COMLIST case!", BRF, LVL_BUILDER, TRUE);
    break;
#endif
/*-------------------------------------------------------------------*/
  case MEDIT_NPC_FLAGS:
    if ((i = atoi(arg)) <= 0)
      break;
    else if (i <= NUM_MOB_FLAGS)
      TOGGLE_BIT(MOB_FLAGS(OLC_MOB(d)), 1 << (i - 1));
    medit_disp_mob_flags(d);
    return;
/*-------------------------------------------------------------------*/
  case MEDIT_AFF_FLAGS:
    if ((i = atoi(arg)) <= 0)
      break;
    else if (i <= NUM_AFF_FLAGS)
      TOGGLE_BIT(AFF_FLAGS(OLC_MOB(d)), 1 << (i - 1));
    medit_disp_aff_flags(d);
    return;
/*-------------------------------------------------------------------*/
#if CONFIG_OASIS_MPROG
  case MEDIT_MPROG:
    if ((i = atoi(arg)) == 0)
      medit_disp_menu(d);
    else if (i == OLC_MTOTAL(d)) {
      struct mob_prog_data *temp;
      CREATE(temp, struct mob_prog_data, 1);
      temp->next = OLC_MPROGL(d);
      temp->type = -1;
      temp->arglist = NULL;
      temp->comlist = NULL;
      OLC_MPROG(d) = temp;
      OLC_MPROGL(d) = temp;
      OLC_MODE(d) = MEDIT_CHANGE_MPROG;
      medit_change_mprog (d);
    } else if (i < OLC_MTOTAL(d)) {
      struct mob_prog_data *temp;
      int x = 1;
      for (temp = OLC_MPROGL(d); temp && x < i; temp = temp->next)
        x++;
      OLC_MPROG(d) = temp;
      OLC_MODE(d) = MEDIT_CHANGE_MPROG;
      medit_change_mprog (d);
    } else if (i == (OLC_MTOTAL(d) + 1)) {
      SEND_TO_Q("Which mob prog do you want to purge? ", d);
      OLC_MODE(d) = MEDIT_PURGE_MPROG;
    } else
      medit_disp_menu(d);
    return;

  case MEDIT_PURGE_MPROG:
    if ((i = atoi(arg)) > 0 && i < OLC_MTOTAL(d)) {
      struct mob_prog_data *temp;
      int x = 1;

      for (temp = OLC_MPROGL(d); temp && x < i; temp = temp->next)
	x++;
      OLC_MPROG(d) = temp;
      REMOVE_FROM_LIST(OLC_MPROG(d), OLC_MPROGL(d), next);
      free(OLC_MPROG(d)->arglist);
      free(OLC_MPROG(d)->comlist);
      free(OLC_MPROG(d));
      OLC_MPROG(d) = NULL;
      OLC_VAL(d) = 1;
    }
    medit_disp_mprog(d);
    return;

  case MEDIT_CHANGE_MPROG:
    if ((i = atoi(arg)) == 1)
      medit_disp_mprog_types(d);
    else if (i == 2) {
      SEND_TO_Q("Enter new arg list: ", d);
      OLC_MODE(d) = MEDIT_MPROG_ARGS;
    } else if (i == 3) {
      SEND_TO_Q("Enter new mob prog commands:\r\n", d);
      /*
       * Pass control to modify.c for typing.
       */
      OLC_MODE(d) = MEDIT_MPROG_COMLIST;
      if (OLC_MPROG(d)->comlist) {
        SEND_TO_Q(OLC_MPROG(d)->comlist, d);
        oldtext = str_dup(OLC_MPROG(d)->comlist);
      }
      string_write(d, &OLC_MPROG(d)->comlist, MAX_STRING_LENGTH, 0, oldtext);
      OLC_VAL(d) = 1;
    } else
      medit_disp_mprog(d);
    return;
#endif

/*-------------------------------------------------------------------*/

/*
 * Numerical responses.
 */

#if CONFIG_OASIS_MPROG
  case MEDIT_MPROG_TYPE:
    /*
     * This calculation may be off by one too many powers of 2?
     * Someone who actually uses MobProgs will have to check.
     */
    OLC_MPROG(d)->type = (1 << LIMIT(atoi(arg), 0, NUM_PROGS - 1));
    OLC_VAL(d) = 1;
    medit_change_mprog(d);
    return;

  case MEDIT_MPROG_ARGS:
    OLC_MPROG(d)->arglist = str_dup(arg);
    OLC_VAL(d) = 1;
    medit_change_mprog(d);
    return;
#endif

  case MEDIT_SEX:
    GET_SEX(OLC_MOB(d)) = LIMIT(i, 0, NUM_GENDERS - 1);
    break;

  case MEDIT_HITROLL:
    GET_HITROLL(OLC_MOB(d)) = LIMIT(i, 0, 50);
    break;

  case MEDIT_DAMROLL:
    GET_DAMROLL(OLC_MOB(d)) = LIMIT(i, 0, 50);
    break;

  case MEDIT_NDD:
    GET_NDD(OLC_MOB(d)) = LIMIT(i, 0, 30);
    break;

  case MEDIT_SDD:
    GET_SDD(OLC_MOB(d)) = LIMIT(i, 0, 127);
    break;

  case MEDIT_NUM_HP_DICE:
    GET_HIT(OLC_MOB(d)) = LIMIT(i, 0, 30);
    break;

  case MEDIT_SIZE_HP_DICE:
    GET_MANA(OLC_MOB(d)) = LIMIT(i, 0, 1000);
    break;

  case MEDIT_ADD_HP:
    GET_MOVE(OLC_MOB(d)) = LIMIT(i, 0, 30000);
    break;

  case MEDIT_AC:
    GET_AC(OLC_MOB(d)) = LIMIT(i, 10, 200);
    break;

  case MEDIT_EXP:
    GET_EXP(OLC_MOB(d)) = MAX(i, 0);
    break;

  case MEDIT_GOLD:
    add_money_to_char(OLC_MOB(d), MAX(i, 0), COPPER_COINS);
    break;

  case MEDIT_POS:
    GET_POS(OLC_MOB(d)) = LIMIT(i, 0, NUM_POSITIONS - 1);
    break;

  case MEDIT_DEFAULT_POS:
    GET_DEFAULT_POS(OLC_MOB(d)) = LIMIT(i, 0, NUM_POSITIONS - 1);
    break;

  case MEDIT_ATTACK:
    GET_ATTACK(OLC_MOB(d)) = LIMIT(i, 0, NUM_ATTACK_TYPES - 1);
    break;

  case MEDIT_LEVEL:
    GET_LEVEL(OLC_MOB(d)) = i;
    break;

  case MEDIT_ALIGNMENT:
    GET_ALIGNMENT(OLC_MOB(d)) = LIMIT(i, -1000, 1000);
    break;

  case MEDIT_CLASS:
    GET_CLASS(OLC_MOB(d)) = MAX(0, MIN(NUM_NPC_CLASS, atoi(arg)));
    break;
  case MEDIT_RACE:
    GET_RACE(OLC_MOB(d)) = MAX(0, MIN(NUM_NPC_RACE, atoi(arg)));
    // INT, WIS, STR, DEX, CON, CHA
    GET_INT(OLC_MOB(d)) = mob_stats[(int)GET_RACE(OLC_MOB(d))][0];
    GET_WIS(OLC_MOB(d)) = mob_stats[(int)GET_RACE(OLC_MOB(d))][1];
    GET_STR(OLC_MOB(d)) = mob_stats[(int)GET_RACE(OLC_MOB(d))][2];
    GET_DEX(OLC_MOB(d)) = mob_stats[(int)GET_RACE(OLC_MOB(d))][3];
    GET_CON(OLC_MOB(d)) = mob_stats[(int)GET_RACE(OLC_MOB(d))][4];
    GET_CHA(OLC_MOB(d)) = mob_stats[(int)GET_RACE(OLC_MOB(d))][5];
    GET_HIT(OLC_MOB(d)) = mob_hp[i][0];
    GET_MANA(OLC_MOB(d)) = mob_hp[i][2];
    GET_MOVE(OLC_MOB(d)) = mob_hp[i][3];
    break;

  case MEDIT_WEIGHT:
    GET_MOB_WEIGHT(OLC_MOB(d)) = LIMIT(i, 1, 1000);
    break;

  case MEDIT_MSIZE:
    GET_MOB_SIZE(OLC_MOB(d)) = atoi(arg);
    break;

/*-------------------------------------------------------------------*/
  default:
    /*
     * We should never get here.
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog("SYSERR: OLC: medit_parse(): Reached default case!", BRF, LVL_BUILDER, TRUE);
    SEND_TO_Q("Oops...\r\n", d);
    break;
  }
/*-------------------------------------------------------------------*/

/*
 * END OF CASE 
 * If we get here, we have probably changed something, and now want to
 * return to main menu.  Use OLC_VAL as a 'has changed' flag  
 */

  OLC_VAL(d) = TRUE;
  medit_disp_menu(d);
}

void medit_string_cleanup(struct descriptor_data *d, int terminator)
{
  switch (OLC_MODE(d)) {

#if CONFIG_OASIS_MPROG
  case MEDIT_MPROG_COMLIST:
    medit_change_mprog(d);
    break;
#endif

  case MEDIT_D_DESC:
  default:
     medit_disp_menu(d);
     break;
  }
}

