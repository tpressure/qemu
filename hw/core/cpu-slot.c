/*
 * CPU slot device abstraction
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

#include "hw/core/cpu-slot.h"

static inline
CPUTopoStatEntry *get_topo_stat_entry(CPUTopoStat *stat,
                                      CPUTopoLevel level)
{
    assert(level != CPU_TOPO_UNKNOWN);

    return &stat->entries[TOPO_STAT_ENTRY_IDX(level)];
}

static void cpu_slot_add_topo_info(CPUTopoState *root, CPUTopoState *child)
{
    CPUSlot *slot = CPU_SLOT(root);
    CPUTopoLevel level = CPU_TOPO_LEVEL(child);
    CPUTopoStatEntry *entry;

    if (level == CPU_TOPO_CORE) {
        CPUCore *core = CPU_CORE(child);
        CPUTopoStatEntry *thread_entry;

        QTAILQ_INSERT_TAIL(&slot->cores, core, node);

        /* Max CPUs per core is pre-configured by "nr-threads". */
        slot->stat.max_cpus += core->nr_threads;
        slot->stat.pre_plugged_cpus += core->plugged_threads;

        thread_entry = get_topo_stat_entry(&slot->stat, CPU_TOPO_THREAD);
        if (child->max_children > thread_entry->max_units) {
            thread_entry->max_units = child->max_children;
        }
    }

    entry = get_topo_stat_entry(&slot->stat, level);
    entry->total_units++;
    if (child->parent->num_children > entry->max_units) {
        entry->max_units = child->parent->num_children;
    }

    set_bit(level, slot->stat.curr_levels);
    return;
}

static void cpu_slot_del_topo_info(CPUTopoState *root, CPUTopoState *child)
{
    CPUSlot *slot = CPU_SLOT(root);
    CPUTopoLevel level = CPU_TOPO_LEVEL(child);
    CPUTopoStatEntry *entry;

    assert(level != CPU_TOPO_UNKNOWN);

    if (level == CPU_TOPO_CORE) {
        QTAILQ_REMOVE(&slot->cores, CPU_CORE(child), node);
    }

    entry = get_topo_stat_entry(&slot->stat, level);
    entry->total_units--;

    /* No need to update entries[*].max_units and curr_levels. */
    return;
}

static void cpu_slot_update_topo_info(CPUTopoState *root, CPUTopoState *child,
                                      bool is_realize)
{
    g_assert(child->parent);

    if (is_realize) {
        cpu_slot_add_topo_info(root, child);
    } else {
        cpu_slot_del_topo_info(root, child);
    }
}

static void cpu_slot_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    CPUTopoClass *tc = CPU_TOPO_CLASS(oc);

    set_bit(DEVICE_CATEGORY_CPU_DEF, dc->categories);
    dc->user_creatable = false;

    tc->level = CPU_TOPO_ROOT;
    tc->update_topo_info = cpu_slot_update_topo_info;
}

static void cpu_slot_instance_init(Object *obj)
{
    CPUSlot *slot = CPU_SLOT(obj);

    QTAILQ_INIT(&slot->cores);
    set_bit(CPU_TOPO_ROOT, slot->stat.curr_levels);
}

static const TypeInfo cpu_slot_type_info = {
    .name = TYPE_CPU_SLOT,
    .parent = TYPE_CPU_TOPO,
    .class_init = cpu_slot_class_init,
    .instance_init = cpu_slot_instance_init,
    .instance_size = sizeof(CPUSlot),
};

static void cpu_slot_register_types(void)
{
    type_register_static(&cpu_slot_type_info);
}

type_init(cpu_slot_register_types)
