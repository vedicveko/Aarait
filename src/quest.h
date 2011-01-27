/* *************************************************************************
*    File: quest.h					Part of CircleMUD  *
* Purpose: To provide special quest-related code.                          *
*                                                                          *
* Morgaelin - quest.h						           *
* Copyright (C) 1997 MS                					   *
************************************************************************* */

/* Aquest related defines **********************************************/

#define AQ_UNDEFINED	-1		/* (R) Quest unavailable	*/
#define AQ_OBJECT	0		/* Player must retreive object  */
#define AQ_ROOM		1		/* Player must reach room	*/
#define AQ_MOB_FIND	2		/* Player must find mob		*/
#define AQ_MOB_KILL	3		/* Player must kill mob		*/
#define AQ_MOB_SAVE	4		/* Player must save mob		*/
#define AQ_RETURN_OBJ	5		/* Player gives object to val0  */
#define AQ_FISH         6               // Player must catch job_spare

#define NUM_AQ_TYPES    7	

/* AQ Flags (much room for expansion) *********************************	*/
#define AQ_REPEATABLE	(1 << 0)	/* Quest can be repeated =(	*/

#define NUM_AQ_FLAGS	1

/* Main quest struct */
struct aq_data {
  int  virtual;			/* Virtual nr of the quest 		*/
  int  mob_vnum;		/* Vnum of the questmaster offering	*/
  char *short_desc;		/* For qlist and the sort		*/
  char *desc;			/* Description of the quest		*/
  char *info;			/* Long help for quest			*/
  char *ending;			/* Ending displayed			*/
  long flags;			/* Flags (repeatable, etc		*/
  int  type;			/* Quest type				*/
  int  target;			/* Target value				*/
  int  coins;			/* Amount of coins for the quest	*/
  int  value[4];		/* Expansion values			*/
  int  obj_reward;              // Obj Reward 
  sh_int next_quest;		/* Link to next quest, -1 is end	*/
}; 

void list_quests(struct char_data *ch, int questmaster);
void generic_complete_quest(struct char_data *ch);
void autoquest_trigger_check(struct char_data *ch, struct char_data *vict,
			struct obj_data *object, int type);
int real_quest(int vnum);                                          	
void add_completed_quest_u(struct char_file_u *player, int num);
int is_complete(struct char_data *ch, int vnum);
int find_quest_by_qmnum(int qm, int num);
void add_completed_quest(struct char_data *player, int num);
SPECIAL(questmaster);
void qedit_setup_new(struct descriptor_data *d);
void qedit_setup_existing(struct descriptor_data *d, int real_num);
void qedit_save_internally(struct descriptor_data *d);
void qedit_save_to_disk(int znum);
void free_quest(struct aq_data *quest);
