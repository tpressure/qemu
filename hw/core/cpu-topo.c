/*
 * General CPU topology device abstraction
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

#include "hw/core/cpu-topo.h"
#include "hw/qdev-properties.h"
#include "qapi/error.h"

static const char *cpu_topo_level_to_string(CPUTopoLevel level)
{
    switch (level) {
    case CPU_TOPO_UNKNOWN:
        return "unknown";
    case CPU_TOPO_THREAD:
        return "thread";
    case CPU_TOPO_CORE:
        return "core";
    case CPU_TOPO_CLUSTER:
        return "cluster";
    case CPU_TOPO_DIE:
        return "die";
    case CPU_TOPO_SOCKET:
        return "socket";
    case CPU_TOPO_BOOK:
        return "book";
    case CPU_TOPO_DRAWER:
        return "drawer";
    case CPU_TOPO_ROOT:
        return "root";
    }

    return NULL;
}

static void cpu_topo_refresh_free_child_index(CPUTopoState *topo)
{
    CPUTopoState *child;

    /*
     * Fast way: Assume that the index grows sequentially and that there
     * are no "index hole" in the previous children.
     *
     * The previous check on num_children ensures that free_child_index + 1
     * does not hit the max_children limit.
     */
    if (topo->free_child_index + 1 == topo->num_children) {
        topo->free_child_index++;
        return;
    }

    /* Slow way: Search the "index hole". The index hole must be found. */
    for (int index = 0; index < topo->num_children; index++) {
        bool existed = false;

        QTAILQ_FOREACH(child, &topo->children, sibling) {
            if (child->index == index) {
                existed = true;
                break;
            }
        }

        if (!existed) {
            topo->free_child_index = index;
            return;
        }
    }
}

static void cpu_topo_validate_index(CPUTopoState *topo, Error **errp)
{
    CPUTopoState *parent = topo->parent, *child;

    if (topo->index < 0) {
        error_setg(errp, "Invalid topology index (%d).",
                   topo->index);
        return;
    }

    if (parent->max_children && topo->index >= parent->max_children) {
        error_setg(errp, "Invalid topology index (%d): "
                   "The maximum index is %d.",
                   topo->index, parent->max_children);
        return;
    }

    QTAILQ_FOREACH(child, &topo->children, sibling) {
        if (child->index == topo->index) {
            error_setg(errp, "Duplicate topology index (%d)",
                       topo->index);
            return;
        }
    }
}

static void cpu_topo_build_hierarchy(CPUTopoState *topo, Error **errp)
{
    CPUTopoState *parent = topo->parent;
    CPUTopoLevel level = CPU_TOPO_LEVEL(topo);
    g_autofree char *name = NULL;

    if (!parent) {
        return;
    }

    if (parent->child_level == CPU_TOPO_UNKNOWN) {
        parent->child_level = level;
    } else if (parent->child_level != level) {
        error_setg(errp, "cpu topo: the parent level %s asks for the "
                   "%s child, but current level is %s",
                   cpu_topo_level_to_string(CPU_TOPO_LEVEL(parent)),
                   cpu_topo_level_to_string(parent->child_level),
                   cpu_topo_level_to_string(level));
        return;
    }

    if (parent->max_children && parent->max_children <= parent->num_children) {
        error_setg(errp, "cpu topo: the parent limit the (%d) children, "
                   "currently it has %d children",
                   parent->max_children,
                   parent->num_children);
        return;
    }

    parent->num_children++;
    if (topo->index == UNASSIGNED_TOPO_INDEX) {
        topo->index = parent->free_child_index;
    } else if (topo->index != parent->free_child_index) {
        /* The index has been set, then we need to validate it. */
        cpu_topo_validate_index(topo, errp);
        if (*errp) {
            return;
        }
    }

    QTAILQ_INSERT_TAIL(&parent->children, topo, sibling);
    cpu_topo_refresh_free_child_index(parent);
}

static void cpu_topo_update_info(CPUTopoState *topo, bool is_realize)
{
    CPUTopoState *parent = topo->parent;
    CPUTopoClass *tc;

    while (parent) {
        tc = CPU_TOPO_GET_CLASS(parent);
        if (tc->update_topo_info) {
            tc->update_topo_info(parent, topo, is_realize);
        }
        parent = parent->parent;
    }
}

static void cpu_topo_set_parent(CPUTopoState *topo, Error **errp)
{
    Object *obj = OBJECT(topo);
    CPUTopoLevel level = CPU_TOPO_LEVEL(topo);

    if (!obj->parent) {
        return;
    }

    if (object_dynamic_cast(obj->parent, TYPE_CPU_TOPO)) {
        CPUTopoState *parent = CPU_TOPO(obj->parent);

        if (level >= CPU_TOPO_LEVEL(parent)) {
            error_setg(errp, "cpu topo: current level (%s) should be "
                       "lower than parent (%s) level",
                       object_get_typename(obj),
                       object_get_typename(OBJECT(parent)));
            return;
        }
        topo->parent = parent;
    }

    if (topo->parent) {
        cpu_topo_build_hierarchy(topo, errp);
        if (*errp) {
            return;
        }

        cpu_topo_update_info(topo, true);
    }
}

static void cpu_topo_realize(DeviceState *dev, Error **errp)
{
    CPUTopoState *topo = CPU_TOPO(dev);
    CPUTopoClass *tc = CPU_TOPO_GET_CLASS(topo);

    if (tc->level == CPU_TOPO_UNKNOWN) {
        error_setg(errp, "cpu topo: no level specified"
                   " type: %s", object_get_typename(OBJECT(dev)));
        return;
    }

    cpu_topo_set_parent(topo, errp);
}

static void cpu_topo_destroy_hierarchy(CPUTopoState *topo)
{
    CPUTopoState *parent = topo->parent;

    if (!parent) {
        return;
    }

    cpu_topo_update_info(topo, false);
    QTAILQ_REMOVE(&parent->children, topo, sibling);
    parent->num_children--;

    if (topo->index < parent->free_child_index) {
        parent->free_child_index = topo->index;
    }

    if (!parent->num_children) {
        parent->child_level = CPU_TOPO_UNKNOWN;
    }
}

static void cpu_topo_unrealize(DeviceState *dev)
{
    CPUTopoState *topo = CPU_TOPO(dev);

    /*
     * The specific unrealize method must consider the bottom-up,
     * layer-by-layer unrealization implementation.
     */
    g_assert(!topo->num_children);

    if (topo->parent) {
        cpu_topo_destroy_hierarchy(topo);
    }
}

static void cpu_topo_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    CPUTopoClass *tc = CPU_TOPO_CLASS(oc);

    /* All topology devices belong to CPU property. */
    set_bit(DEVICE_CATEGORY_CPU, dc->categories);
    dc->realize = cpu_topo_realize;
    dc->unrealize = cpu_topo_unrealize;

    /*
     * The general topo device is not hotpluggable by default.
     * If any topo device needs hotplug support, this flag must be
     * overridden under arch-specific topo device code.
     */
    dc->hotpluggable = false;

    tc->level = CPU_TOPO_UNKNOWN;
}

static void cpu_topo_instance_init(Object *obj)
{
    CPUTopoState *topo = CPU_TOPO(obj);
    QTAILQ_INIT(&topo->children);

    topo->index = UNASSIGNED_TOPO_INDEX;
    topo->free_child_index = 0;
    topo->child_level = CPU_TOPO_UNKNOWN;
}

static const TypeInfo cpu_topo_type_info = {
    .name = TYPE_CPU_TOPO,
    .parent = TYPE_DEVICE,
    .abstract = true,
    .class_size = sizeof(CPUTopoClass),
    .class_init = cpu_topo_class_init,
    .instance_size = sizeof(CPUTopoState),
    .instance_init = cpu_topo_instance_init,
};

static void cpu_topo_register_types(void)
{
    type_register_static(&cpu_topo_type_info);
}

type_init(cpu_topo_register_types)
