/************************************************************************
 * Generic OLC Library - Objects / genobj.c			v1.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-1999 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "boards.h"
#include "shop.h"
#include "genolc.h"
#include "genobj.h"
#include "genzon.h"

static int copy_object_main(struct obj_data *to, struct obj_data *from, int free_object);

extern struct obj_data *obj_proto;
extern struct obj_data *object_list;
extern struct index_data *obj_index;
extern struct zone_data *zone_table;
extern struct board_info_type board_info[];
extern struct shop_data *shop_index;
extern zone_rnum top_of_zone_table;
extern obj_rnum top_of_objt;
extern int top_shop;

obj_rnum add_object(struct obj_data *newobj, obj_vnum ovnum)
{
  int found = NOTHING;
  zone_rnum rznum = real_zone_by_thing(ovnum);

  /*
   * Write object to internal tables.
   */
  if ((newobj->item_number = real_object(ovnum)) != NOTHING) {
    copy_object(&obj_proto[newobj->item_number], newobj);
    update_objects(&obj_proto[newobj->item_number]);
    add_to_save_list(zone_table[rznum].number, SL_OBJ);
    return newobj->item_number;
  }

  found = insert_object(newobj, ovnum);
  adjust_objects(found);
  add_to_save_list(zone_table[rznum].number, SL_OBJ);
  return (found);
}

/* ------------------------------------------------------------------------------------------------------------------------------ */

/*
 * Fix all existing objects to have these values.
 * We need to run through each and every object currently in the
 * game to see which ones are pointing to this prototype.
 * if object is pointing to this prototype, then we need to replace it
 * with the new one.
 */
int update_objects(struct obj_data *refobj)
{
  struct obj_data *obj, swap;
  int count = 0;

  for (obj = object_list; obj; obj = obj->next) {
    if (obj->item_number != refobj->item_number)
      continue;

    count++;

    /* Update the existing object but save a copy for private information. */
    swap = *obj;
    *obj = *refobj;

    /* Copy game-time dependent variables over. */
    obj->in_room = swap.in_room;
    obj->carried_by = swap.carried_by;
    obj->worn_by = swap.worn_by;
    obj->worn_on = swap.worn_on;
    obj->in_obj = swap.in_obj;
    obj->contains = swap.contains;
    obj->next_content = swap.next_content;
    obj->next = swap.next;
  }

  return count;
}

/* ------------------------------------------------------------------------------------------------------------------------------ */

/*
 * Adjust the internal values of other objects as if something was inserted at the given array index.
 * Might also be useful to make 'holes' in the array for some reason.
 */
int adjust_objects(obj_rnum refpt)
{
  int shop, i, zone, cmd_no;
  struct obj_data *obj;

  if (refpt < 0 || refpt >= top_of_objt)
    return refpt;

  /*
   * Renumber live objects.
   */
  for (obj = object_list; obj; obj = obj->next)
    GET_OBJ_RNUM(obj) += (GET_OBJ_RNUM(obj) >= refpt);

  /*
   * Renumber zone table.
   */
  for (zone = 0; zone <= top_of_zone_table; zone++) {
    for (cmd_no = 0; ZCMD(zone, cmd_no).command != 'S'; cmd_no++) {
      switch (ZCMD(zone, cmd_no).command) {
      case 'P':
        ZCMD(zone, cmd_no).arg3 += (ZCMD(zone, cmd_no).arg3 >= refpt);
         /*
          * No break here - drop into next case.
          */
      case 'O':
      case 'G':
      case 'E':
        ZCMD(zone, cmd_no).arg1 += (ZCMD(zone, cmd_no).arg1 >= refpt);
        break;
      case 'R':
        ZCMD(zone, cmd_no).arg2 += (ZCMD(zone, cmd_no).arg2 >= refpt);
        break;
      }
    }
  }

  /*
   * Renumber notice boards.
   */
  for (i = 0; i < NUM_OF_BOARDS; i++)
    BOARD_RNUM(i) += (BOARD_RNUM(i) >= refpt);

  /*
   * Renumber shop produce.
   */
  for (shop = 0; shop <= top_shop - top_shop_offset; shop++)
    for (i = 0; SHOP_PRODUCT(shop, i) != NOTHING; i++)
      SHOP_PRODUCT(shop, i) += (SHOP_PRODUCT(shop, i) >= refpt);

  return refpt;
}

/* ------------------------------------------------------------------------------------------------------------------------------ */

/*
 * Function handle the insertion of an object within the prototype framework.  Note that this does not adjust internal values
 * of other objects, use add_object() for that.
 */
int insert_object(struct obj_data *obj, obj_vnum ovnum)
{
  int i;

  top_of_objt++;

  RECREATE(obj_index, struct index_data, top_of_objt + 1);
  RECREATE(obj_proto, struct obj_data, top_of_objt + 1);

  /*
   * Start counting through both tables.
   */
  for (i = top_of_objt; i > 0; i--) {
    /*
     * Check if current virtual is bigger than our virtual number.
     */
    if (ovnum > obj_index[i - 1].vnum)
      return index_object(obj, ovnum, i);

    /* Copy over the object that should be here. */
    obj_index[i] = obj_index[i - 1];
    obj_proto[i] = obj_proto[i - 1];
    obj_proto[i].item_number = i;
  }

  /* Not found, place at 0. */
  return index_object(obj, ovnum, 0);
}

/* ------------------------------------------------------------------------------------------------------------------------------ */

int index_object(struct obj_data *obj, obj_vnum ovnum, obj_rnum ornum)
{
  if (obj == NULL || ovnum < 0 || ornum < 0 || ornum > top_of_objt)
    return NOWHERE;

  obj->item_number = ornum;
  obj_index[ornum].vnum = ovnum;
  obj_index[ornum].number = 0;
  obj_index[ornum].func = NULL;

  copy_object_preserve(&obj_proto[ornum], obj);
  obj_proto[ornum].in_room = NOWHERE;

  return ornum;
}

/* ------------------------------------------------------------------------------------------------------------------------------ */

int save_objects(zone_rnum zone_num)
{
  int counter, counter2, realcounter;
  FILE *fp;
  struct obj_data *obj;
  struct extra_descr_data *ex_desc;

  if (zone_num < 0 || zone_num > top_of_zone_table) {
    log("SYSERR: GenOLC: save_objects: Invalid real zone number %d. (0-%d)", zone_num, top_of_zone_table);
    return FALSE;
  }

  sprintf(buf, "%s/%d.new", OBJ_PREFIX, zone_table[zone_num].number);
  if (!(fp = fopen(buf, "w+"))) {
    mudlog("SYSERR: OLC: Cannot open objects file!", BRF, LVL_IMMORT, TRUE);
    return FALSE;
  }
  /*
   * Start running through all objects in this zone.
   */
  for (counter = zone_table[zone_num].number * 100; counter <= zone_table[zone_num].top; counter++) {
    if ((realcounter = real_object(counter)) >= 0) {
      if ((obj = &obj_proto[realcounter])->action_description) {
        buf1[MAX_STRING_LENGTH - 1] = '\0';
	strncpy(buf1, obj->action_description, MAX_STRING_LENGTH - 1);
	strip_cr(buf1);
      } else
	*buf1 = '\0';

      fprintf(fp,
	      "#%d\n"
	      "%s~\n"
	      "%s~\n"
	      "%s~\n"
	      "%s~\n"
	      "%d %d %d %ld\n"
	      "%d %d %d %d %d %d\n"
	      "%d %d %d %d\n",

	      GET_OBJ_VNUM(obj),
	      (obj->name && *obj->name) ? obj->name : "undefined",
	      (obj->short_description && *obj->short_description) ? obj->short_description : "undefined",
	      (obj->description && *obj->description) ?	obj->description : "undefined",
	      buf1,
	      GET_OBJ_TYPE(obj), GET_OBJ_EXTRA(obj), GET_OBJ_WEAR(obj), GET_OBJ_PERM(obj),
	      GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 3), GET_OBJ_CSLOTS(obj), GET_OBJ_TSLOTS(obj),
	      GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj), GET_OBJ_RENT(obj), GET_OBJ_LEVEL(obj)
      );

      /*
       * Do we have extra descriptions? 
       */
      if (obj->ex_description) {	/* Yes, save them too. */
	for (ex_desc = obj->ex_description; ex_desc; ex_desc = ex_desc->next) {
	  /*
	   * Sanity check to prevent nasty protection faults.
	   */
	  if (!ex_desc->keyword || !ex_desc->description || !*ex_desc->keyword || !*ex_desc->description) {
	    mudlog("SYSERR: OLC: oedit_save_to_disk: Corrupt ex_desc!", BRF, LVL_IMMORT, TRUE);
	    continue;
	  }
          buf1[MAX_STRING_LENGTH - 1] = '\0';
	  strncpy(buf1, ex_desc->description, MAX_STRING_LENGTH - 1);
	  strip_cr(buf1);
	  fprintf(fp, "E\n"
		  "%s~\n"
		  "%s~\n", ex_desc->keyword, buf1);
	}
      }
      /*
       * Do we have affects? 
       */
      for (counter2 = 0; counter2 < MAX_OBJ_AFFECT; counter2++)
	if (obj->affected[counter2].modifier)
	  fprintf(fp, "A\n"
		  "%d %d\n", obj->affected[counter2].location,
		  obj->affected[counter2].modifier);
    }
  }

  /*
   * Write the final line, close the file.
   */
  fprintf(fp, "$~\n");
  fclose(fp);
  sprintf(buf2, "%s/%d.obj", OBJ_PREFIX, zone_table[zone_num].number);
  remove(buf2);
  rename(buf, buf2);

  remove_from_save_list(zone_table[zone_num].number, SL_OBJ);
  return TRUE;
}

/*
 * Free all, unconditionally.
 */
void free_object_strings(struct obj_data *obj)
{
#if 0 /* Debugging, do not enable. */
  extern struct obj_data *object_list;
  struct obj_data *t;
  int i = 0;

  for (t = object_list; t; t = t->next) {
    if (t == obj) {
      i++;
      continue;
    }
    assert(obj->name != t->name);
    assert(obj->description != t->description);
    assert(obj->short_description != t->short_description);
    assert(obj->action_description != t->action_description);
    assert(obj->ex_description != t->ex_description);
  }
  assert(i <= 1);
#endif

  if (obj->name)
    free(obj->name);
  if (obj->description)
    free(obj->description);
  if (obj->short_description)
    free(obj->short_description);
  if (obj->action_description)
    free(obj->action_description);
  if (obj->ex_description)
    free_ex_descriptions(obj->ex_description);
}

/*
 * For object instances that are not the prototype.
 */
void free_object_strings_proto(struct obj_data *obj)
{
  int robj_num = GET_OBJ_RNUM(obj);

  if (obj->name && obj->name != obj_proto[robj_num].name)
    free(obj->name);
  if (obj->description && obj->description != obj_proto[robj_num].description)
    free(obj->description);
  if (obj->short_description && obj->short_description != obj_proto[robj_num].short_description)
    free(obj->short_description);
  if (obj->action_description && obj->action_description != obj_proto[robj_num].action_description)
    free(obj->action_description);
  if (obj->ex_description) {
    struct extra_descr_data *thised, *plist, *next_one; /* O(horrible) */
    int ok_key, ok_desc, ok_item;
    for (thised = obj->ex_description; thised; thised = next_one) {
      next_one = thised->next;
      for (ok_item = ok_key = ok_desc = 1, plist = obj_proto[robj_num].ex_description; plist; plist = plist->next) {
        if (plist->keyword == thised->keyword)
          ok_key = 0;
        if (plist->description == thised->description)
          ok_desc = 0;
        if (plist == thised)
          ok_item = 0;
      }
      if (thised->keyword && ok_key)
        free(thised->keyword);
      if (thised->description && ok_desc)
        free(thised->description);
      if (ok_item)
        free(thised);
    }
  }
}

void copy_object_strings(struct obj_data *to, struct obj_data *from)
{
  to->name = from->name ? str_dup(from->name) : NULL;
  to->description = from->description ? str_dup(from->description) : NULL;
  to->short_description = from->short_description ? str_dup(from->short_description) : NULL;
  to->action_description = from->action_description ? str_dup(from->action_description) : NULL;

  if (from->ex_description)
    copy_ex_descriptions(&to->ex_description, from->ex_description);
  else
    to->ex_description = NULL;
}

int copy_object(struct obj_data *to, struct obj_data *from)
{
  free_object_strings(to);
  return copy_object_main(to, from, TRUE);
}

int copy_object_preserve(struct obj_data *to, struct obj_data *from)
{
  return copy_object_main(to, from, FALSE);
}

static int copy_object_main(struct obj_data *to, struct obj_data *from, int free_object)
{
  *to = *from;
  copy_object_strings(to, from);
  return TRUE;
}
