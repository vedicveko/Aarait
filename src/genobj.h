/************************************************************************
 * Generic OLC Library - Objects / genobj.h			v1.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-1999 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

void copy_object_strings(struct obj_data *to, struct obj_data *from);
void free_object_strings(struct obj_data *obj);
void free_object_strings_proto(struct obj_data *obj);
int copy_object(struct obj_data *to, struct obj_data *from);
int copy_object_preserve(struct obj_data *to, struct obj_data *from);
int save_objects(zone_rnum vznum);
int insert_object(struct obj_data *obj, obj_vnum ovnum);
int adjust_objects(obj_rnum refpt);
int index_object(struct obj_data *obj, obj_vnum ovnum, obj_rnum ornum);
int update_objects(struct obj_data *refobj);
obj_rnum add_object(struct obj_data *, obj_vnum ovnum);

