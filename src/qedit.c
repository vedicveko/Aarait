/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*  _TwyliteMud_ by Rv.                          Based on CircleMud3.0bpl9 *
*    				                                          *
*  OasisOLC - qedit.c 		                                          *
*    				                                          *
*  Copyright 1997 Mike Steinmann                                          *
*  Used at Morgaelin (mud.dwango.com 3000)				  *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*. Original author: Levork .*/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "boards.h"
#include "olc.h"
#include "quest.h"

/*------------------------------------------------------------------------*/
/*. External data .*/

extern int top_of_aquestt;
extern struct aq_data *aquest_table;
extern struct zone_data *zone_table;
extern char *quest_types[];
extern char *aq_flags[];
extern void strip_string(char *buffer);

/*------------------------------------------------------------------------*/
/* function protos */
void qedit_setup_new(struct descriptor_data *d);
void qedit_setup_existing(struct descriptor_data *d, int real_num);
void qedit_save_internally(struct descriptor_data *d);
void qedit_save_to_disk(int znum);
void free_quest(struct aq_data *quest);
void qedit_disp_type_menu(struct descriptor_data *d);
void qedit_disp_menu(struct descriptor_data *d);
void qedit_parse(struct descriptor_data *d, char *arg);
void qedit_disp_flags_menu(struct descriptor_data *d);
void oedit_disp_val0_menu(struct descriptor_data *d);
void oedit_disp_val1_menu(struct descriptor_data *d);
void oedit_disp_val2_menu(struct descriptor_data *d);
void oedit_disp_val3_menu(struct descriptor_data *d);

/*------------------------------------------------------------------------*\
  Utils and exported functions.
\*------------------------------------------------------------------------*/

void smash_tilde(char *str)
{
    for (; *str != '\0'; str++) {
	if (*str == '~')
	    *str = '-';
	if (*str == 'M')
	    *str = ' ';
    }

    return;
}

void qedit_setup_new(struct descriptor_data *d)
{

    CREATE(OLC_QUEST(d), struct aq_data, 1);
    OLC_QUEST(d)->short_desc = str_dup("an unfinished quest");
    OLC_QUEST(d)->desc = str_dup("This is an unfinished quest.");
    OLC_QUEST(d)->info = str_dup("Information for unfinished quest.\r\n");
    OLC_QUEST(d)->ending = str_dup("Ending for unfinished quest.\r\n");
    OLC_QUEST(d)->next_quest = -1;
    qedit_disp_menu(d);
    OLC_VAL(d) = 0;
}

/*------------------------------------------------------------------------*/

void qedit_setup_existing(struct descriptor_data *d, int real_num)
{
    struct aq_data *quest;
    int i;

    /*. Build a copy of the quest . */
    CREATE(quest, struct aq_data, 1);
    *quest = aquest_table[real_num];
    /* allocate space for all strings  */
    if (aquest_table[real_num].short_desc)
	quest->short_desc = str_dup(aquest_table[real_num].short_desc);
    if (aquest_table[real_num].desc)
	quest->desc = str_dup(aquest_table[real_num].desc);
    if (aquest_table[real_num].info)
	quest->info = str_dup(aquest_table[real_num].info);
    if (aquest_table[real_num].ending)
	quest->ending = str_dup(aquest_table[real_num].ending);
    quest->type = aquest_table[real_num].type;
    quest->mob_vnum = aquest_table[real_num].mob_vnum;
    quest->flags = aquest_table[real_num].flags;
    quest->target = aquest_table[real_num].target;
    quest->exp = aquest_table[real_num].exp;
    quest->next_quest = aquest_table[real_num].next_quest;
    for (i = 0; i < 4; i++)
	quest->value[i] = aquest_table[real_num].value[i];
    /*. Attach room copy to players descriptor . */
    OLC_QUEST(d) = quest;
    OLC_VAL(d) = 0;
    qedit_disp_menu(d);
}

/*------------------------------------------------------------------------*/
void qedit_save_internally(struct descriptor_data *d)
{
    int i, quest_num, found = 0;
    struct aq_data *new_quest;

    quest_num = real_quest(OLC_NUM(d));
    if (quest_num >= 0) {
	free_quest(aquest_table + quest_num);
	aquest_table[quest_num] = *OLC_QUEST(d);
    } else {
	/*. Quest doesn't exist, hafta add it . */
	CREATE(new_quest, struct aq_data, top_of_aquestt + 2);

	/* count thru quest tables */
	for (i = 0; i <= top_of_aquestt; i++) {
	    if (!found) {
		/*. Is this the place? . */
		if (aquest_table[i].virtual > OLC_NUM(d)) {
		    found = 1;

		    new_quest[i] = *(OLC_QUEST(d));
		    new_quest[i].virtual = OLC_NUM(d);
		    quest_num = i;
		    new_quest[i + 1] = aquest_table[i];

		} else {
		    new_quest[i] = aquest_table[i];
		}
	    } else {		/*. Already been found  . */
		new_quest[i + 1] = aquest_table[i];
	    }
	}
	if (!found) {		/*. Still not found, insert at top of table . */
	    new_quest[i] = *(OLC_QUEST(d));
	    new_quest[i].virtual = OLC_NUM(d);
	    quest_num = i;
	}

	/* copy quest table over */
	free(aquest_table);
	aquest_table = new_quest;
	top_of_aquestt++;

    }
    olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_QUEST);
}


/*------------------------------------------------------------------------*/

void qedit_save_to_disk(int znum)
{
    int counter, realcounter;
    FILE *fp;
    struct aq_data *quest;
    char buf3[MAX_STRING_LENGTH];
    char buf4[MAX_STRING_LENGTH];

    sprintf(buf, "%s/%d.qst", QST_PREFIX, zone_table[znum].number);
    if (!(fp = fopen(buf, "w+"))) {
	mudlog("SYSERR: OLC: Cannot open quest file!", BRF, LVL_BUILDER,
	       TRUE);
	return;
    }

    for (counter = zone_table[znum].number * 100;
	 counter <= zone_table[znum].top; counter++) {
	realcounter = real_quest(counter);
	if (realcounter >= 0) {
	    quest = (aquest_table + realcounter);
	    strcpy(buf1,
		   quest->short_desc ? quest->
		   short_desc : "an unfinished quest");
	    smash_tilde(buf1);
	    strcpy(buf2,
		   quest->desc ? quest->
		   desc : "This is an unfinished quest.");
	    smash_tilde(buf2);
	    strcpy(buf3,
		   quest->info ? quest->
		   info : "Information for an unfinished quest.\r\n");
	    strip_string(buf3);
	    smash_tilde(buf3);
	    strcpy(buf4,
		   quest->ending ? quest->
		   ending : "There is no ending!\r\n");
	    strip_string(buf4);
	    smash_tilde(buf4);

	    /*. Build a buffer ready to save . */
	    sprintf(buf, "#%d\n%s~\n%s~\n%s~\n%s~\n%d %d %ld %d %d %d\n",
		    counter, buf1, buf2, buf3, buf4,
		    quest->type, quest->mob_vnum,
		    quest->flags, quest->target, quest->exp,
		    quest->next_quest);
	    /*. Save this section . */
	    fputs(buf, fp);

	    sprintf(buf, "%d %d %d %d\n", quest->value[0], quest->value[1],
		    quest->value[2], quest->value[3]);
	    fputs(buf, fp);

	    fprintf(fp, "S\n");
	}
    }
    /* write final line and close */
    fprintf(fp, "$~\n");
    fclose(fp);
    olc_remove_from_save_list(zone_table[znum].number, OLC_SAVE_QUEST);
}

/*------------------------------------------------------------------------*/

void free_quest(struct aq_data *quest)
{
    if (quest->desc)
	free(quest->desc);
    if (quest->short_desc)
	free(quest->short_desc);
    if (quest->info)
	free(quest->info);
    if (quest->ending)
	free(quest->ending);
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

void qedit_disp_flags_menu(struct descriptor_data *d)
{
    int counter, columns = 0;

    get_char_cols(d->character);
    send_to_char("[H[J", d->character);
    for (counter = 0; counter < NUM_AQ_FLAGS; counter++) {
	sprintf(buf, "%s%2d%s) %-20.20s ",
		grn, counter + 1, nrm, aq_flags[counter]);
	if (!(++columns % 2))
	    strcat(buf, "\r\n");
	send_to_char(buf, d->character);
    }
    sprintbit(OLC_QUEST(d)->flags, aq_flags, buf1);
    sprintf(buf,
	    "\r\nQuest flags: %s%s%s\r\n"
	    "Enter quest flags, 0 to quit : ", cyn, buf1, nrm);
    send_to_char(buf, d->character);
    OLC_MODE(d) = QEDIT_FLAGS;
}


void qedit_disp_type_menu(struct descriptor_data *d)
{
    int counter, columns = 0;

    send_to_char("[H[J", d->character);
    for (counter = 0; counter < NUM_AQ_TYPES; counter++) {
	sprintf(buf, "%s%2d%s) %-20.20s ",
		cyn, counter, nrm, quest_types[counter]);
	if (!(++columns % 2))
	    strcat(buf, "\r\n");
	send_to_char(buf, d->character);
    }
    send_to_char("\r\nEnter quest type : ", d->character);
    OLC_MODE(d) = QEDIT_TYPE;
}

void qedit_disp_val0_menu(struct descriptor_data *d)
{
    OLC_MODE(d) = QEDIT_VALUE_0;
    switch (OLC_QUEST(d)->type) {
    case AQ_RETURN_OBJ:
	send_to_char("Enter vnum of mob to receive object: ",
		     d->character);
	break;
    default:
	qedit_disp_menu(d);
    }
}

void qedit_disp_val1_menu(struct descriptor_data *d)
{
    OLC_MODE(d) = QEDIT_VALUE_1;
    switch (OLC_QUEST(d)->type) {
    default:
	qedit_disp_menu(d);
    }
}

void qedit_disp_val2_menu(struct descriptor_data *d)
{
    OLC_MODE(d) = QEDIT_VALUE_2;
    switch (OLC_QUEST(d)->type) {
    default:
	qedit_disp_menu(d);
    }
}

void qedit_disp_val3_menu(struct descriptor_data *d)
{
    OLC_MODE(d) = QEDIT_VALUE_3;
    switch (OLC_QUEST(d)->type) {
    default:
	qedit_disp_menu(d);
    }
}

/* the main menu */
void qedit_disp_menu(struct descriptor_data *d)
{
    struct aq_data *quest;
    extern struct char_data *mob_proto;

    get_char_cols(d->character);
    quest = OLC_QUEST(d);

    sprintbit(quest->flags, aq_flags, buf1);
    sprintf(buf2, "%d %d %d %d", quest->value[0], quest->value[1],
	    quest->value[2], quest->value[3]);
    sprintf(buf,
	    "[H[J"
	    "-- Quest number : [%s%d%s]  	Quest zone: [%s%d%s]\r\n"
	    "%s1%s) Name        : %s%s%s\r\n"
	    "%s2%s) Description :\r\n%s%s%s\r\n"
	    "%s3%s) Information :\r\n%s%s%s"
	    "%s4%s) Ending      :\r\n%s%s%s"
	    "%s5%s) Questmaster : %s%d%s -- %s%s%s\r\n"
	    "%s6%s) Type        : %s%s%s\r\n"
	    "%s7%s) Flags       : %s%s%s\r\n"
	    "%s8%s) Target vnum : %s%d%s\r\n"
	    "%s9%s) Experience  : %s%d%s\r\n"
	    "%sA%s) Next quest  : %s%d%s\r\n"
	    "%sB%s) Values      : %s%s%s\r\n" "%sQ%s) Quit\r\n"
	    "Enter choice : ", cyn, OLC_NUM(d), nrm, cyn,
	    zone_table[OLC_ZNUM(d)].number, nrm, cyn, nrm, grn,
	    quest->short_desc, nrm, cyn, nrm, grn, quest->desc, nrm, cyn,
	    nrm, grn, quest->info, nrm, cyn, nrm, grn, quest->ending, nrm,
	    cyn, nrm, grn, quest->mob_vnum, nrm, grn,
	    real_mobile(quest->mob_vnum) >
	    -1 ? mob_proto[real_mobile(quest->mob_vnum)].player.
	    short_descr : "None", nrm, cyn, nrm, grn,
	    quest_types[quest->type], nrm, cyn, nrm, grn, buf1, nrm, cyn,
	    nrm, grn, quest->target, nrm, cyn, nrm, grn, quest->exp, nrm,
	    cyn, nrm, grn, quest->next_quest, nrm, cyn, nrm, grn, buf2,
	    nrm, cyn, nrm);
    send_to_char(buf, d->character);

    OLC_MODE(d) = QEDIT_MAIN_MENU;
}



/**************************************************************************
  The main loop
 **************************************************************************/

void qedit_parse(struct descriptor_data *d, char *arg)
{
    extern struct aq_data *aquest_table;
    int number = 0;

    switch (OLC_MODE(d)) {
    case QEDIT_CONFIRM_SAVESTRING:
	switch (*arg) {
	case 'y':
	case 'Y':
	    qedit_save_internally(d);
	    sprintf(buf, "OLC: %s edits quest %d", GET_NAME(d->character),
		    OLC_NUM(d));
	    mudlog(buf, CMP, LVL_BUILDER, TRUE);
	    /*. Do NOT free strings! just the room structure . */
	    cleanup_olc(d, CLEANUP_STRUCTS);
	    send_to_char("Quest saved to memory.\r\n", d->character);
	    break;
	case 'n':
	case 'N':
	    /* free everything up, including strings etc */
	    cleanup_olc(d, CLEANUP_ALL);
	    break;
	default:
	    send_to_char("Invalid choice!\r\n", d->character);
	    send_to_char("Do you wish to save this quest internally? : ",
			 d->character);
	    break;
	}
	return;

    case QEDIT_MAIN_MENU:
	switch (*arg) {
	case 'q':
	case 'Q':
	    if (OLC_VAL(d)) {	/*. Something has been modified . */
		send_to_char
		    ("Do you wish to save this quest internally? : ",
		     d->character);
		OLC_MODE(d) = QEDIT_CONFIRM_SAVESTRING;
	    } else
		cleanup_olc(d, CLEANUP_ALL);
	    return;
	case '1':
	    send_to_char("Enter quest name:-\r\n| ", d->character);
	    OLC_MODE(d) = QEDIT_NAME;
	    break;
	case '2':
	    send_to_char("Enter quest description:-\r\n| ", d->character);
	    OLC_MODE(d) = QEDIT_DESC;
	    break;
	case '3':
	    OLC_MODE(d) = QEDIT_INFO;
#if defined(CLEAR_SCREEN)
	    SEND_TO_Q("\x1B[H\x1B[J", d);
#endif
	    SEND_TO_Q
		("Enter quest information: (/s saves /h for help)\r\n\r\n",
		 d);
	    d->backstr = NULL;
	    if (OLC_QUEST(d)->info) {
		SEND_TO_Q(OLC_QUEST(d)->info, d);
		d->backstr = str_dup(OLC_QUEST(d)->info);
	    }
	    d->str = &OLC_QUEST(d)->info;
	    d->max_str = MAX_QUEST_INFO;
	    d->mail_to = 0;
	    OLC_VAL(d) = 1;
	    break;
	case '4':
	    OLC_MODE(d) = QEDIT_ENDING;
#if defined(CLEAR_SCREEN)
	    SEND_TO_Q("\x1B[H\x1B[J", d);
#endif
	    SEND_TO_Q("Enter quest ending: (/s saves /h for help)\r\n\r\n",
		      d);
	    d->backstr = NULL;
	    if (OLC_QUEST(d)->ending) {
		SEND_TO_Q(OLC_QUEST(d)->ending, d);
		d->backstr = str_dup(OLC_QUEST(d)->ending);
	    }
	    d->str = &OLC_QUEST(d)->ending;
	    d->max_str = MAX_QUEST_ENDING;
	    d->mail_to = 0;
	    OLC_VAL(d) = 1;
	    break;
	case '5':
	    send_to_char("Enter questmaster vnum: ", d->character);
	    OLC_MODE(d) = QEDIT_QUESTMASTER;
	    break;
	case '6':
	    qedit_disp_type_menu(d);
	    break;
	case '7':
	    qedit_disp_flags_menu(d);
	    break;
	case '8':
	    send_to_char("Enter target vnum: ", d->character);
	    OLC_MODE(d) = QEDIT_TARGET;
	    break;
	case '9':
	    send_to_char("Enter experience: ", d->character);
	    OLC_MODE(d) = QEDIT_EXP;
	    break;
	case 'a':
	case 'A':
	    send_to_char("Next quest (-1 to end): ", d->character);
	    OLC_MODE(d) = QEDIT_NEXT;
	    break;
	case 'b':
	case 'B':
	    OLC_QUEST(d)->value[0] = 0;
	    OLC_QUEST(d)->value[1] = 0;
	    OLC_QUEST(d)->value[2] = 0;
	    OLC_QUEST(d)->value[3] = 0;
	    qedit_disp_val0_menu(d);
	    break;
	default:
	    send_to_char("Invalid choice!", d->character);
	    qedit_disp_menu(d);
	    break;
	}
	return;

    case QEDIT_NAME:
	if (OLC_QUEST(d)->short_desc)
	    free(OLC_QUEST(d)->short_desc);
	if (strlen(arg) > MAX_QUEST_NAME)
	    arg[MAX_QUEST_NAME - 1] = 0;
	OLC_QUEST(d)->short_desc = str_dup(arg);
	break;

    case QEDIT_DESC:
	if (OLC_QUEST(d)->desc)
	    free(OLC_QUEST(d)->desc);
	if (strlen(arg) > 80)
	    arg[79] = 0;
	OLC_QUEST(d)->desc = str_dup(arg);
	break;

    case QEDIT_INFO:
	/*
	 * We will NEVER get here, we hope.
	 */
	mudlog("SYSERR: Reached QEDIT_INFO case in parse_qedit", BRF,
	       LVL_BUILDER, TRUE);
	break;

    case QEDIT_ENDING:
	/*
	 * We will NEVER get here, we hope.
	 */
	mudlog("SYSERR: Reached QEDIT_ENDING case in parse_qedit", BRF,
	       LVL_BUILDER, TRUE);
	break;

    case QEDIT_QUESTMASTER:
	number = atoi(arg);
	if (real_mobile(number) >= 0)
	    OLC_QUEST(d)->mob_vnum = number;
	else
	    OLC_QUEST(d)->mob_vnum = -1;
	break;

    case QEDIT_TYPE:
	number = atoi(arg);
	if (number < 0 || number >= NUM_AQ_TYPES) {
	    send_to_char("Invalid choice!", d->character);
	    qedit_disp_type_menu(d);
	    return;
	} else
	    OLC_QUEST(d)->type = number;
	break;

    case QEDIT_FLAGS:
	number = atoi(arg);
	if ((number < 0) || (number > NUM_AQ_FLAGS)) {
	    send_to_char("That's not a valid choice!\r\n", d->character);
	    qedit_disp_flags_menu(d);
	} else {
	    if (number == 0)
		break;
	    else {
		/* toggle bits */
		if (IS_SET(OLC_QUEST(d)->flags, 1 << (number - 1)))
		    REMOVE_BIT(OLC_QUEST(d)->flags, 1 << (number - 1));
		else
		    SET_BIT(OLC_QUEST(d)->flags, 1 << (number - 1));
		qedit_disp_flags_menu(d);
	    }
	}
	return;

    case QEDIT_TARGET:
	number = atoi(arg);
	OLC_QUEST(d)->target = MAX(0, number);
	break;

    case QEDIT_EXP:
	number = atoi(arg);
	OLC_QUEST(d)->exp = MAX(0, MIN(number, 100000));
	break;

    case QEDIT_NEXT:
	number = atoi(arg);
	if (real_quest(number) >= 0)
	    OLC_QUEST(d)->next_quest = number;
	else
	    OLC_QUEST(d)->next_quest = -1;
	break;

    case QEDIT_VALUE_0:
	number = atoi(arg);
	switch (OLC_QUEST(d)->type) {
	case AQ_RETURN_OBJ:
	    if (real_mobile(number) < 0)
		number = 0;
	    break;
	}
	OLC_QUEST(d)->value[0] = number;
	qedit_disp_val1_menu(d);
	return;

    case QEDIT_VALUE_1:
	OLC_QUEST(d)->value[1] = atoi(arg);
	qedit_disp_val2_menu(d);
	return;

    case QEDIT_VALUE_2:
	OLC_QUEST(d)->value[2] = atoi(arg);
	qedit_disp_val3_menu(d);
	return;

    case QEDIT_VALUE_3:
	OLC_QUEST(d)->value[3] = atoi(arg);
	break;

    default:
	/* we should never get here */
	mudlog("SYSERR: Reached default case in parse_qedit", BRF,
	       LVL_BUILDER, TRUE);
	break;
    }
    /*. If we get this far, something has be changed . */
    OLC_VAL(d) = 1;
    qedit_disp_menu(d);
}
