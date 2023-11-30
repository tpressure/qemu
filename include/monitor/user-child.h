/*
 * Child configurable interface header.
 *
 * Copyright (c) 2023 Intel Corporation
 * Author: Zhao Liu <zhao1.liu@intel.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef USER_CHILD_H
#define USER_CHILD_H

#include "qom/object.h"

#define TYPE_USER_CHILD "user-child"

typedef struct UserChildClass UserChildClass;
DECLARE_CLASS_CHECKERS(UserChildClass, USER_CHILD, TYPE_USER_CHILD)
#define USER_CHILD(obj) INTERFACE_CHECK(UserChild, (obj), TYPE_USER_CHILD)

typedef struct UserChild UserChild;

/**
 * UserChildClass:
 * @get_parent: Method to get the default parent if user doesn't specify
 *     the parent in cli.
 * @get_child_name: Method to get the default device id for this device
 *     if user doesn't specify id in cli.
 * @check_parent: Method to check if the parent specified by user in cli
 *     is valid.
 */
struct UserChildClass {
    /* <private> */
    InterfaceClass parent_class;

    /* <public> */
    Object *(*get_parent)(UserChild *uc, Error **errp);
    char *(*get_child_name)(UserChild *uc, Object *parent);
    bool (*check_parent)(UserChild *uc, Object *parent);
};

Object *uc_provide_default_parent(Object *obj, Error **errp);
char *uc_name_future_child(Object *obj, Object *parent);
bool uc_check_user_parent(Object *obj, Object *parent);

#endif /* USER_CHILD_H */
