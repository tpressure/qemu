/*
 * Child configurable interface.
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

#include "qemu/osdep.h"

#include "monitor/user-child.h"

Object *uc_provide_default_parent(Object *obj, Error **errp)
{
    UserChild *uc = USER_CHILD(obj);
    UserChildClass *ucc = USER_CHILD_GET_CLASS(uc);

    if (ucc->get_parent) {
        return ucc->get_parent(uc, errp);
    }

    return NULL;
}

char *uc_name_future_child(Object *obj, Object *parent)
{
    UserChild *uc = USER_CHILD(obj);
    UserChildClass *ucc = USER_CHILD_GET_CLASS(uc);

    if (ucc->get_child_name) {
        return ucc->get_child_name(uc, parent);
    }

    return NULL;
}

bool uc_check_user_parent(Object *obj, Object *parent)
{
    UserChild *uc = USER_CHILD(obj);
    UserChildClass *ucc = USER_CHILD_GET_CLASS(uc);

    if (ucc->check_parent) {
        ucc->check_parent(uc, parent);
    }

    return true;
}

static const TypeInfo user_child_interface_info = {
    .name          = TYPE_USER_CHILD,
    .parent        = TYPE_INTERFACE,
    .class_size = sizeof(UserChildClass),
};

static void user_child_register_types(void)
{
    type_register_static(&user_child_interface_info);
}

type_init(user_child_register_types)
