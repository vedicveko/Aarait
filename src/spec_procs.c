/* ************************************************************************
*   File: spec_procs.c                                  Part of CircleMUD *
*  Usage: implementation of special procedures for mobiles/objects/rooms  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"
#include "coins.h"

/*   external vars  */
extern struct room_data *world;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct time_info_data time_info;
void add_to_webpage(struct char_data *ch, int mode);

/* extern functions */
void add_follower(struct char_data * ch, struct char_data * leader);
ACMD(do_drop);
ACMD(do_gen_door);
ACMD(do_say);

/* local functions */
SPECIAL(dump);
SPECIAL(mayor);
void npc_steal(struct char_data * ch, struct char_data * victim);
SPECIAL(snake);
SPECIAL(thief);
SPECIAL(magic_user);
SPECIAL(puff);
SPECIAL(fido);
SPECIAL(janitor);
SPECIAL(cityguard);
SPECIAL(pet_shops);


void mobaction(struct char_data *ch, char *string, char *action)
{
  sprintf(buf, "%s %s", action, string);
  command_interpreter(ch, buf);
}

void mobsay(struct char_data *ch, const char *msg)
{
  char Gbuf[MAX_INPUT_LENGTH];

  if (!msg || !*msg) {
    log("SYSERR: No text in mobsay()");
    return;
  }
  if (strlen(msg) > (MAX_INPUT_LENGTH - 1)) {
    log("SYSERR: text too long in mobsay()");
    return;
  }
  strcpy(Gbuf, msg);
  do_say(ch, Gbuf, 0, 0);
}



/* ********************************************************************
*  Special procedures for mobiles                                     *
******************************************************************** */
SPECIAL(dump)
{
  struct obj_data *k;
  int value = 0;

  for (k = world[ch->in_room].contents; k; k = world[ch->in_room].contents) {
    act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
    extract_obj(k);
  }

  if (!CMD_IS("drop"))
    return (0);

  do_drop(ch, argument, cmd, 0);

  for (k = world[ch->in_room].contents; k; k = world[ch->in_room].contents) {
    act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
    value += MAX(1, MIN(50, GET_OBJ_COST(k) / 10));
    extract_obj(k);
  }

  if (value) {
    send_to_char("You are awarded for outstanding performance.\r\n", ch);
    act("$n has been awarded for being a good citizen.", TRUE, ch, 0, 0, TO_ROOM);

    if (GET_LEVEL(ch) < 3)
      gain_exp(ch, value);
    else
      GET_GOLD(ch) += value;
  }
  return (1);
}


SPECIAL(mayor)
{
  const char open_path[] =
	"W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";
  const char close_path[] =
	"W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

  static const char *path = NULL;
  static int index;
  static bool move = FALSE;

  if (!move) {
    if (time_info.hours == 6) {
      move = TRUE;
      path = open_path;
      index = 0;
    } else if (time_info.hours == 20) {
      move = TRUE;
      path = close_path;
      index = 0;
    }
  }
  if (cmd || !move || (GET_POS(ch) < POS_SLEEPING) ||
      (GET_POS(ch) == POS_FIGHTING))
    return (FALSE);

  switch (path[index]) {
  case '0':
  case '1':
  case '2':
  case '3':
    perform_move(ch, path[index] - '0', 1);
    break;

  case 'W':
    GET_POS(ch) = POS_STANDING;
    act("$n awakens and groans loudly.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'S':
    GET_POS(ch) = POS_SLEEPING;
    act("$n lies down and instantly falls asleep.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'a':
    act("$n says 'Hello Honey!'", FALSE, ch, 0, 0, TO_ROOM);
    act("$n smirks.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'b':
    act("$n says 'What a view!  I must get something done about that dump!'",
	FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'c':
    act("$n says 'Vandals!  Youngsters nowadays have no respect for anything!'",
	FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'd':
    act("$n says 'Good day, citizens!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'e':
    act("$n says 'I hereby declare the bazaar open!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'E':
    act("$n says 'I hereby declare Midgaard closed!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'O':
    do_gen_door(ch, "gate", 0, SCMD_UNLOCK);
    do_gen_door(ch, "gate", 0, SCMD_OPEN);
    break;

  case 'C':
    do_gen_door(ch, "gate", 0, SCMD_CLOSE);
    do_gen_door(ch, "gate", 0, SCMD_LOCK);
    break;

  case '.':
    move = FALSE;
    break;

  }

  index++;
  return (FALSE);
}


/* ********************************************************************
*  General special procedures for mobiles                             *
******************************************************************** */


void npc_steal(struct char_data * ch, struct char_data * victim)
{
  int gold;

  if (IS_NPC(victim))
    return;
  if (GET_LEVEL(victim) >= LVL_IMMORT)
    return;

  if (AWAKE(victim) && (number(0, GET_LEVEL(ch)) == 0)) {
    act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, victim, TO_VICT);
    act("$n tries to steal gold from $N.", TRUE, ch, 0, victim, TO_NOTVICT);
  } else {
    /* Steal some gold coins */
    gold = (int) ((GET_GOLD(victim) * number(1, 10)) / 100);
    if (gold > 0) {
      GET_GOLD(ch) += gold;
      remove_money_from_char(ch, gold, GOLD_COINS);
    }
  }
}


SPECIAL(snake)
{
  if (cmd)
    return (FALSE);

  if (GET_POS(ch) != POS_FIGHTING)
    return (FALSE);

  if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room) &&
      (number(0, 42 - GET_LEVEL(ch)) == 0)) {
    act("$n bites $N!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
    act("$n bites you!", 1, ch, 0, FIGHTING(ch), TO_VICT);
// POISON!
    return (TRUE);
  }
  return (FALSE);
}


SPECIAL(thief)
{
  struct char_data *cons;

  if (cmd)
    return (FALSE);

  if (GET_POS(ch) != POS_STANDING)
    return (FALSE);

  for (cons = world[ch->in_room].people; cons; cons = cons->next_in_room)
    if (!IS_NPC(cons) && (GET_LEVEL(cons) < LVL_IMMORT) && (!number(0, 4))) {
      npc_steal(ch, cons);
      return (TRUE);
    }
  return (FALSE);
}




/* ********************************************************************
*  Special procedures for mobiles                                      *
******************************************************************** */


SPECIAL(puff)
{
  if (cmd)
    return (0);

  switch (number(0, 60)) {
  case 0:
    do_say(ch, "My god!  It's full of stars!", 0, 0);
    return (1);
  case 1:
    do_say(ch, "How'd all those fish get up here?", 0, 0);
    return (1);
  case 2:
    do_say(ch, "I'm a very female dragon.", 0, 0);
    return (1);
  case 3:
    do_say(ch, "I've got a peaceful, easy feeling.", 0, 0);
    return (1);
  default:
    return (0);
  }
}



SPECIAL(fido)
{

  struct obj_data *i, *temp, *next_obj;

  if (cmd || !AWAKE(ch))
    return (FALSE);

  for (i = world[ch->in_room].contents; i; i = i->next_content) {
    if (IS_CORPSE(i)) {
      act("$n savagely devours a corpse.", FALSE, ch, 0, 0, TO_ROOM);
      for (temp = i->contains; temp; temp = next_obj) {
	next_obj = temp->next_content;
	obj_from_obj(temp);
	obj_to_room(temp, ch->in_room);
      }
      extract_obj(i);
      return (TRUE);
    }
  }
  return (FALSE);
}



SPECIAL(janitor)
{
  struct obj_data *i;

  if (cmd || !AWAKE(ch))
    return (FALSE);

  for (i = world[ch->in_room].contents; i; i = i->next_content) {
    if (!CAN_WEAR(i, ITEM_WEAR_TAKE))
      continue;
    if (GET_OBJ_TYPE(i) != ITEM_DRINKCON && GET_OBJ_COST(i) >= 15)
      continue;
if(GET_OBJ_TYPE(i) == ITEM_CONTAINER && GET_OBJ_VAL(i, 3) == 1) {
    return TRUE;
}
    act("$n sweeps up some trash.", FALSE, ch, 0, 0, TO_ROOM);
    obj_from_room(i);
    obj_to_char(i, ch);
    return (TRUE);
  }

  return (FALSE);
}


SPECIAL(cityguard)
{
  struct char_data *tch;

  if (cmd || !AWAKE(ch) || FIGHTING(ch))
    return (FALSE);

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
    if(IS_NPC(tch) || !CAN_SEE(ch, tch) || GET_LEVEL(tch) > LVL_GOD) { continue; }
    if (PLR_FLAGGED(tch, PLR_KILLER)) {
      act("$n screams 'HEY!!!  You're one of those PLAYER KILLERS!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
      hit(ch, tch, TYPE_UNDEFINED);
      return (TRUE);
    }

    if (PLR_FLAGGED(tch, PLR_THIEF)){
      act("$n screams 'HEY!!!  You're one of those PLAYER THIEVES!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
      hit(ch, tch, TYPE_UNDEFINED);
      return (TRUE);
    }
    if(GET_EQ(tch, WEAR_WIELD) && !tch->char_specials.weapwarned && GET_OBJ_TYPE(GET_EQ(tch, WEAR_WIELD)) != ITEM_POLE) {
      sprintf(buf, "Remove your weapon, %s. I will not warn you again.", GET_NAME(tch));
      mobsay(ch, buf);
      tch->char_specials.weapwarned = 1;
      return (TRUE);
    }
    if(GET_EQ(tch, WEAR_WIELD) && tch->char_specials.weapwarned && GET_OBJ_TYPE(GET_EQ(tch, WEAR_WIELD)) != ITEM_POLE) {
      obj_to_char(unequip_char(tch, WEAR_WIELD), tch);
      tch->char_specials.weapwarned = 0;
      act("$n hits you over the head with the butt of $s weapon, knocking you to the ground.", FALSE, ch, 0, tch, TO_VICT); 
      act("$n hits $N over the head with the butt of $s weapon, knocking $N to the ground.", FALSE, ch, 0, tch, TO_NOTVICT);
      act("$n deflty disarms you.", FALSE, ch, 0, tch, TO_VICT);
      act("$n deftly disarms $N.", FALSE, ch, 0, tch, TO_NOTVICT);
      GET_POS(tch) = POS_SITTING;
      if(GET_HIT(tch) > 0) {
        GET_HIT(tch) = GET_HIT(tch) - number(1, 3);
        update_pos(tch);
      }
      return (TRUE);
    }
  }
  return (FALSE);
}

// New specs for AARAIT

/* 
This is the guy you report to when completing a job. He should add you to the webpage and
set your class to jobless.
*/
SPECIAL(employment_master)
{
  struct char_data *vict = (struct char_data *) me;
  bool fail = 0;
  bool complete = 0;

 if(CMD_IS("finished")) {
   act("$n tells $N $e is finished with $s occupation.", FALSE, ch, 0, vict, TO_ROOM);
   switch(GET_CLASS(ch)) {

     case CLASS_GOBLIN_SLAYER:
        if(convert_all_to_copper(ch) > job_needs[(int)GET_CLASS(ch)][0] && GET_POINTS(ch) > job_needs[(int)GET_CLASS(ch)][1]) {
          complete = 1;
          add_to_webpage(ch, MODE_JOB_DONE);
          SET_BIT(PLR_FLAGS(ch), PLR_GOB_DONE);
        }
        else {
          fail = 1;
        }
     break;

     case CLASS_PIG_FARMER:
        if(convert_all_to_copper(ch) > job_needs[(int)GET_CLASS(ch)][0] && GET_POINTS(ch) > job_needs[(int)GET_CLASS(ch)][1]) {
          complete = 1;
          add_to_webpage(ch, MODE_JOB_DONE);
          SET_BIT(PLR_FLAGS(ch), PLR_PIG_DONE);
        }
        else {
          fail = 1;
        }


     case CLASS_FISHERMAN:
       if(convert_all_to_copper(ch) > job_needs[(int)GET_CLASS(ch)][0] && GET_POINTS(ch) > job_needs[(int)GET_CLASS(ch)][1]) {
         complete = 1;
         add_to_webpage(ch, MODE_JOB_DONE);
         SET_BIT(PLR_FLAGS(ch), PLR_FISH_DONE);
       }
       else {
         fail = 1;
       }
       
     break;

     default:
       mobsay(vict, "Go get an occupation ya loser.");
       return TRUE;

   }
   if(complete) {
     make_change(ch, job_needs[(int)GET_CLASS(ch)][0], BUYING);
     GET_CLASS(ch) = CLASS_JOBLESS;
     sprintf(buf, "Thank you for your contributions %s! You make %s proud!", GET_NAME(ch), noble_houses[(int)GET_HOUSE(ch)]);
     mobsay(vict, buf);

     mobsay(vict, "If you want something else to do, I would suggest finding another occupation.");
     mobsay(vict, "The employment office is always open in Allyton.");

     sprintf(buf, "%s has completed their occupation.", GET_NAME(ch));
     log(buf);
     mudlog(buf, BRF, 31, TRUE);

     GET_POINTS(ch) = 0;
    
     if(IS_SET(PLR_FLAGS(ch), PLR_PIG_DONE) && IS_SET(PLR_FLAGS(ch), PLR_GOB_DONE) && IS_SET(PLR_FLAGS(ch), PLR_FISH_DONE)) {
      if(GET_RANK(ch) < RANK_EMPEROR) { 
        GET_RANK(ch) += 1;
        sprintf(buf, "%s has achieved the rank of %s!\r\n", GET_NAME(ch), rank[(int)GET_RANK(ch)][(int)GET_SEX(ch)]);
        send_to_all(buf);
        
      }
     }
     return TRUE;
    }
    if(fail) {
     sprintf(buf, "%s, you need %d coins and %d points in order to complete your occupation.", GET_NAME(ch), job_needs[(int)GET_CLASS(ch)][0], job_needs[(int)GET_CLASS(ch)][1]);
     mobsay(vict, buf);
     return TRUE;
    }
   }

 complete = 0;

 if(CMD_IS("request")) {
   act("$n asks about an occupation.", FALSE, ch, 0, 0, TO_ROOM);
   one_argument(argument, arg);

   if(!IS_JOBLESS(ch)) {
      sprintf(buf, "You already have a occupation %s", GET_NAME(ch));     
      mobsay(vict, buf);
      return TRUE;
   }

   if(convert_all_to_copper(ch) < 1000) {
      sprintf(buf, "Changing occupations costs 1000 coins, which you can't afford %s", GET_NAME(ch));
      mobsay(vict, buf);
      return TRUE;
   }

   if (is_abbrev(arg, "goblin") || is_abbrev(arg, "slayer")) {

     GET_CLASS(ch) = CLASS_GOBLIN_SLAYER;
     complete = 1;
   }

   else if (is_abbrev(arg, "pig") || is_abbrev(arg, "farmer")) {

     GET_CLASS(ch) = CLASS_PIG_FARMER; 
     complete = 1;
   }

   else if (is_abbrev(arg, "fisherman")) {

     GET_CLASS(ch) = CLASS_PIG_FARMER;
     complete = 1;
   }
   else {
     sprintf(buf, "I don't understand what you mean %s", GET_NAME(ch));
     mobsay(vict, buf);
     mobsay(vict, "If you would like to REQUEST a occupation, type; REQUEST GOBLIN, FARMER, or FISHERMAN.");
   }
 
   if(complete) {
     make_change(ch, 1000, BUYING);
     sprintf(buf, "%s changed class to %s", GET_NAME(ch), pc_class_types[(int)GET_CLASS(ch)]);
     log(buf);
     mudlog(buf, BRF, 31, TRUE);

     sprintf(buf, "Welcome to your new occupation%s!", GET_NAME(ch));
     mobsay(vict, buf);
     mobsay(vict, "As your first step, you should read HELP JOB");
   
     // mudlog
   }
   return TRUE; 
 }

 return FALSE;
}  

SPECIAL(bennyguard)
{

  struct char_data *vict = (struct char_data *) me;

  if (cmd == SCMD_UP) {
    if (GET_MAX_HIT(ch) < 12 && !IS_NPC(ch)) {
      act("The Guard steps in front of $n.\r\n", FALSE, ch, 0, 0, TO_ROOM);
      act("The Guard steps in front of you.\r\n", FALSE, ch, 0, 0, TO_CHAR);
      sprintf(buf, "%s, You cannot enter Benny Mountain without first being an adept warrior. Try killing some rabbits.", GET_NAME(ch));
      mobsay(vict, buf);
      return TRUE;
    } else {
        act("The Guard nods as you pass.", FALSE, ch, 0, 0, TO_CHAR);
        act("The Guard nods as $n passes.", FALSE, ch, 0, 0, TO_ROOM); 
      }
  }
  return FALSE;
}

SPECIAL(blacksmith)
{
 struct obj_data *repair;
 struct char_data *vict = (struct char_data *) me;
 int cost = 0;

if(CMD_IS("repair")) {
 one_argument(argument, arg);

 if (!*arg) {
  send_to_char("Repair what?\r\n", ch);
  return TRUE;
 }

 if (!(repair = get_obj_in_list_vis(ch, arg, ch->carrying))) {
    sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
    send_to_char(buf, ch);
  return TRUE;
 }

 cost = ((GET_OBJ_COST(repair) + 2) / 2);

// BEEP
 if(convert_all_to_copper(ch) < cost) {
   sprintf(buf, "Repairing %s will cost %d coins, which you do not have on your person.\r\n", repair->short_description, cost);
   send_to_char(buf, ch); 
 }

 if ((GET_OBJ_CSLOTS(repair) == 0) && (GET_OBJ_TSLOTS(repair) == 0)) {
  act("$p seems to be indestructable!", FALSE, ch, repair, 0,
  TO_CHAR); 
  return TRUE;
 }

 if (GET_OBJ_CSLOTS(repair) == GET_OBJ_TSLOTS(repair)) {
  act("$n says, '$p seems to be in perfect condition!'", FALSE, ch, repair, 0, TO_ROOM); 
  return TRUE;
 }

 if (GET_OBJ_CSLOTS(repair) < 0) {
  act("$n tries to repair $N's $p, but it crumbles
  away!", TRUE, vict, repair, ch, TO_NOTVICT); 
  act("$n tries to repair your $p, but it crumbles away!", FALSE, vict, repair, ch, TO_VICT);
  extract_obj(repair); 
  return TRUE;
 }

  act("$n repairs $N's $p, making it as good as new again!", TRUE, vict, repair, ch, TO_NOTVICT);
  act("$n repairs your $p, making it as good as new!", FALSE, vict, repair, ch, TO_VICT);
  if(GET_OBJ_TSLOTS(repair) > 5) {
    GET_OBJ_TSLOTS(repair) -= 5;
  }
  else {
    GET_OBJ_TSLOTS(repair) = 1;
  }
  GET_OBJ_CSLOTS(repair) = GET_OBJ_TSLOTS(repair);
  remove_money_from_char(ch, cost, GOLD_COINS);
  return TRUE;
 }
 return FALSE;
}


SPECIAL(bank)
{
  int amount;
  char money[255];
  char type[255];
  argument = one_argument(argument, money);
  argument = one_argument(argument, type);


  if (CMD_IS("balance")) {
    if (GET_BANK_GOLD(ch) > 0)
      sprintf(buf, "Your current balance is %d coins.\r\n",
	      GET_BANK_GOLD(ch));
    else
      sprintf(buf, "You currently have no money deposited.\r\n");
    send_to_char(buf, ch);
    return (1);
  } else if (CMD_IS("deposit")) {
    if ((amount = atoi(money)) <= 0) {
      send_to_char("How much do you want to deposit?\r\n", ch);
      return (1);
    }

    if (is_abbrev(type, "platinum")) {


      if (GET_MONEY(ch, PLAT_COINS) < amount) {
        send_to_char("You don't have that many coins!\r\n", ch);
        return (1);
      }

      remove_money_from_char(ch, amount, PLAT_COINS);
      GET_BANK_GOLD(ch) += (amount * 1000);

      sprintf(buf, "You deposit %d platinum.\r\n", amount);
      send_to_char(buf, ch);

    }
    else if (is_abbrev(type, "gold")) {


      if (GET_MONEY(ch, GOLD_COINS) < amount) {
        send_to_char("You don't have that many coins!\r\n", ch);
        return (1);
      }

      remove_money_from_char(ch, amount, GOLD_COINS);
      GET_BANK_GOLD(ch) += (amount * 100);

      sprintf(buf, "You deposit %d gold.\r\n", amount);
      send_to_char(buf, ch);

    }
    else if (is_abbrev(type, "silver")) {


      if (GET_MONEY(ch, SILVER_COINS) < amount) {
        send_to_char("You don't have that many coins!\r\n", ch);
        return (1);
      }

      remove_money_from_char(ch, amount, SILVER_COINS);
      GET_BANK_GOLD(ch) += (amount * 10);

      sprintf(buf, "You deposit %d silver.\r\n", amount);
      send_to_char(buf, ch);

    }
    else if (is_abbrev(type, "copper")) {


      if (GET_MONEY(ch, COPPER_COINS) < amount) {
        send_to_char("You don't have that many coins!\r\n", ch);
        return (1);
      }

      remove_money_from_char(ch, amount, COPPER_COINS);
      GET_BANK_GOLD(ch) += amount;

      sprintf(buf, "You deposit %d copper.\r\n", amount);
      send_to_char(buf, ch);

    }
    else {
      send_to_char("Deposit what kind of coin?\r\n", ch);
      return (1);
    }
    act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
    return (1);
  } else if (CMD_IS("withdraw")) {
    if ((amount = atoi(money)) <= 0) {
      send_to_char("How much do you want to withdraw?\r\n", ch);
      return (1);
    }

    if (is_abbrev(type, "platinum")) {


      if ((GET_BANK_GOLD(ch) / 1000) < amount) {
        send_to_char("You don't have that many coins!\r\n", ch);
        return (1);
      }

      add_money_to_char(ch, amount, PLAT_COINS);
      GET_BANK_GOLD(ch) -= (amount * 1000);

      sprintf(buf, "You withdraw %d platinum.\r\n", amount);
      send_to_char(buf, ch);

    }
    else if (is_abbrev(type, "gold")) {


      if ((GET_BANK_GOLD(ch) / 100) < amount) {
        send_to_char("You don't have that many coins!\r\n", ch);
        return (1);
      }

      add_money_to_char(ch, amount, GOLD_COINS);
      GET_BANK_GOLD(ch) -= (amount * 100);

      sprintf(buf, "You withdraw %d gold.\r\n", amount);
      send_to_char(buf, ch);

    }
    else if (is_abbrev(type, "silver")) {


      if ((GET_BANK_GOLD(ch) / 10) < amount) {
        send_to_char("You don't have that many coins!\r\n", ch);
        return (1);
      }

      add_money_to_char(ch, amount, SILVER_COINS);
      GET_BANK_GOLD(ch) -= (amount * 10);

      sprintf(buf, "You withdraw %d silver.\r\n", amount);
      send_to_char(buf, ch);

    }
    else if (is_abbrev(type, "copper")) {


      if (GET_BANK_GOLD(ch) < amount) {
        send_to_char("You don't have that many coins!\r\n", ch);
        return (1);
      }

      add_money_to_char(ch, amount, COPPER_COINS);
      GET_BANK_GOLD(ch) -= amount;

      sprintf(buf, "You withdraw %d copper.\r\n", amount);
      send_to_char(buf, ch);

    }
    else {
      send_to_char("Withdraw what kind of coin?\r\n", ch);
      return (1);
    }

    act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
    return (1);
  } else
    return (0);
}

SPECIAL(dynamite) {
  struct char_data *fish = NULL;
  struct obj_data *fish_obj = NULL;
  struct obj_data *tobj = me;  

  if(CMD_IS("light")) {

    act("$n lights a stick of dynamite", FALSE, ch, 0, 0, TO_ROOM);
    send_to_char("You light a stick of dynamite.\r\n", ch);
    send_to_room("A stick of dynamite EXPLODES!!!!!!!!!!!!\r\n", ch->in_room); 


    for (fish = world[ch->in_room].people; fish; fish = fish->next_in_room) {
 
     if (IS_NPC_FISH(fish)) {
 
      sprintf(buf, "%s floats to the surface, stunned.\r\n", GET_NAME(fish));
      CAP(buf);
      send_to_room(buf, ch->in_room);
      fish_obj = read_object(GET_MOB_VNUM(fish), VIRTUAL);

      if(!fish_obj) {
        send_to_char("Bug, please report.\r\n", ch);
        log("No fish_obj");
        return TRUE;
      }

      GET_OBJ_WEIGHT(fish_obj) = GET_MOB_WEIGHT(fish);
      GET_OBJ_COST(fish_obj) = GET_MOB_WEIGHT(fish) * 10;
      GET_OBJ_RENT(fish_obj) = GET_MOB_WEIGHT(fish) * 10;

      if(GET_MOB_WEIGHT(fish)) {
        GET_OBJ_COST(fish_obj) = GET_MOB_WEIGHT(fish) * 10;
        GET_OBJ_RENT(fish_obj) = GET_MOB_WEIGHT(fish) * 10;
        sprintf(buf, "%s fish", fish->player.name);
      }
      else {
        GET_OBJ_COST(fish_obj) = number(1, 7);
        GET_OBJ_RENT(fish_obj) = 1;
        GET_OBJ_VAL(fish_obj, 0) = number(15, 18); 
        sprintf(buf, "%s fish bait", fish->player.name);
      }

      fish_obj->name = str_dup(buf);

      sprintf(buf, "%s is lying here.", GET_NAME(fish));
      CAP(buf);
      fish_obj->description = str_dup(buf);

      fish_obj->short_description = str_dup(GET_NAME(fish));

      obj_to_room(fish_obj, ch->in_room);
     }  
    } 
    extract_obj(tobj);
    return TRUE;
  }

  return FALSE;
}

