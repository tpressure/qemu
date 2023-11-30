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
    QTAILQ_INSERT_TAIL(&parent->children, topo, sibling);
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

    QTAILQ_REMOVE(&parent->children, topo, sibling);
    parent->num_children--;

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
