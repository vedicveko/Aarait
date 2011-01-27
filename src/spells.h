/* ************************************************************************
*   File: spells.h                                      Part of CircleMUD *
*  Usage: header file: constants and fn prototypes for spell system       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#define TYPE_UNDEFINED               -1
#define SPELL_RESERVED_DBC            0  /* SKILL NUMBER ZERO -- RESERVED */

#define MAX_SPELLS		    130

/* PLAYER SKILLS - Numbered from MAX_SPELLS+1 to MAX_SKILLS */
#define SKILL_BACKSTAB              131 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_BASH                  132 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_HIDE                  133 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_KICK                  134 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_PICK_LOCK             135 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_UNUSED1               136 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_RESCUE                137 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_SNEAK                 138 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_STEAL                 139 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_TRACK		    140 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_SCAN                  141
#define SKILL_PIG_FARMING           142
#define SKILL_GOBLIN_SLAYING        143
#define SKILL_FISHING               144
#define SKILL_THROW                 145
#define SKILL_BLAH                  146
#define SKILL_D                     147
#define SKILL_AMBUSH                148
#define SKILL_UNUSED                149
#define SKILL_HIT                   150 
#define SKILL_STING                 151 
#define SKILL_WHIP                  152 
#define SKILL_SLASH                 153 
#define SKILL_BITE                  154 
#define SKILL_BLUDGEON              155 
#define SKILL_CRUSH                 156 
#define SKILL_POUND                 157 
#define SKILL_CLAW                  158 
#define SKILL_MAUL                  159 
#define SKILL_THRASH                160 
#define SKILL_PIERCE                161 
#define SKILL_BLAST                 162 
#define SKILL_PUNCH                 163 
#define SKILL_STAB                  164 
#define SKILL_CLEAVE                165
#define SKILL_BLOCK                 166
#define SKILL_PARRY                 167
#define SKILL_ASSEMBLE              168
#define SKILL_BAKE                  169 
#define SKILL_BREW                  170 
#define SKILL_CRAFT                 171
#define SKILL_FLETCH                172
#define SKILL_KNIT                  173 
#define SKILL_MAKE                  174 
#define SKILL_MIX                   175 
#define SKILL_THATCH                176 
#define SKILL_BARG                  177
#define SKILL_WEAVE                 178 
#define SKILL_BOW                   179
#define SKILL_CROSSBOW              180
#define SKILL_REPAIR                181
#define SKILL_TAME                  182

/* New skills may be added here up to MAX_SKILLS (200) */

#define TOP_SPELL_DEFINE	     299
/* NEW NPC/OBJECT SPELLS can be inserted here up to 299 */


/* WEAPON ATTACK TYPES */

#define TYPE_HIT                     300
#define TYPE_STING                   301
#define TYPE_WHIP                    302
#define TYPE_SLASH                   303
#define TYPE_BITE                    304
#define TYPE_BLUDGEON                305
#define TYPE_CRUSH                   306
#define TYPE_POUND                   307
#define TYPE_CLAW                    308
#define TYPE_MAUL                    309
#define TYPE_THRASH                  310
#define TYPE_PIERCE                  311
#define TYPE_BLAST		     312
#define TYPE_PUNCH		     313
#define TYPE_STAB		     314
#define TYPE_CLEAVE                  315

/* new attack types can be added here - up to TYPE_SUFFERING */
#define TYPE_SUFFERING		     399



#define SAVING_PARA   0
#define SAVING_ROD    1
#define SAVING_PETRI  2
#define SAVING_BREATH 3
#define SAVING_SPELL  4


#define TAR_IGNORE      (1 << 0)
#define TAR_CHAR_ROOM   (1 << 1)
#define TAR_CHAR_WORLD  (1 << 2)
#define TAR_FIGHT_SELF  (1 << 3)
#define TAR_FIGHT_VICT  (1 << 4)
#define TAR_SELF_ONLY   (1 << 5) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_NOT_SELF   	(1 << 6) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_OBJ_INV     (1 << 7)
#define TAR_OBJ_ROOM    (1 << 8)
#define TAR_OBJ_WORLD   (1 << 9)
#define TAR_OBJ_EQUIP	(1 << 10)


/* Possible Targets:

   bit 0 : IGNORE TARGET
   bit 1 : PC/NPC in room
   bit 2 : PC/NPC in world
   bit 3 : Object held
   bit 4 : Object in inventory
   bit 5 : Object in room
   bit 6 : Object in world
   bit 7 : If fighting, and no argument, select tar_char as self
   bit 8 : If fighting, and no argument, select tar_char as victim (fighting)
   bit 9 : If no argument, select self, if argument check that it IS self.

*/

/* Attacktypes with grammar */

struct attack_hit_type {
   const char	*singular;
   const char	*plural;
};

