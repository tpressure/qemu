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

#include "hw/boards.h"
#include "hw/core/cpu-slot.h"
#include "hw/cpu/book.h"
#include "hw/cpu/cluster.h"
#include "hw/cpu/die.h"
#include "hw/cpu/drawer.h"
#include "hw/cpu/socket.h"
#include "qapi/error.h"

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

static void cpu_slot_check_topo_support(CPUTopoState *root, CPUTopoState *child,
                                        Error **errp)
{
    CPUSlot *slot = CPU_SLOT(root);
    CPUTopoLevel child_level = CPU_TOPO_LEVEL(child);

    if (!test_bit(child_level, slot->supported_levels)) {
        error_setg(errp, "cpu topo: the level %s is not supported",
                   cpu_topo_level_to_string(child_level));
        return;
    }

    /*
     * Currently we doesn't support hybrid topology. For SMP topology,
     * each child under the same parent are same type.
     */
    if (child->parent->num_children) {
        CPUTopoState *sibling = QTAILQ_FIRST(&child->parent->children);
        const char *sibling_type = object_get_typename(OBJECT(sibling));
        const char *child_type = object_get_typename(OBJECT(child));

        if (strcmp(sibling_type, child_type)) {
            error_setg(errp, "Invalid smp topology: different CPU "
                       "topology types (%s child vs %s sibling) "
                       "under the same parent (%s).",
                       child_type, sibling_type,
                       object_get_typename(OBJECT(child->parent)));
        }
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
    tc->check_topo_child = cpu_slot_check_topo_support;
}

static void cpu_slot_instance_init(Object *obj)
{
    CPUSlot *slot = CPU_SLOT(obj);

    QTAILQ_INIT(&slot->cores);
    set_bit(CPU_TOPO_ROOT, slot->stat.curr_levels);

    /* Set all levels by default. */
    bitmap_fill(slot->supported_levels, USER_AVAIL_LEVEL_NUM);
    clear_bit(CPU_TOPO_UNKNOWN, slot->supported_levels);
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

void machine_plug_cpu_slot(MachineState *ms)
{
    MachineClass *mc = MACHINE_GET_CLASS(ms);

    ms->topo = CPU_SLOT(qdev_new(TYPE_CPU_SLOT));

    object_property_add_child(container_get(OBJECT(ms), "/peripheral"),
                              "cpu-slot", OBJECT(ms->topo));
    DEVICE(ms->topo)->id = g_strdup_printf("%s", "cpu-slot");

    qdev_realize_and_unref(DEVICE(ms->topo), NULL, &error_abort);
    ms->topo->ms = ms;

    if (!mc->smp_props.clusters_supported) {
        clear_bit(CPU_TOPO_CLUSTER, ms->topo->supported_levels);
    }

    if (!mc->smp_props.dies_supported) {
        clear_bit(CPU_TOPO_DIE, ms->topo->supported_levels);
    }

    if (!mc->smp_props.books_supported) {
        clear_bit(CPU_TOPO_BOOK, ms->topo->supported_levels);
    }

    if (!mc->smp_props.drawers_supported) {
        clear_bit(CPU_TOPO_DRAWER, ms->topo->supported_levels);
    }
}

static unsigned int *get_smp_info_by_level(CpuTopology *smp_info,
                                           CPUTopoLevel child_level)
{
    switch (child_level) {
    case CPU_TOPO_THREAD:
        return &smp_info->threads;
    case CPU_TOPO_CORE:
        return &smp_info->cores;
    case CPU_TOPO_CLUSTER:
        return &smp_info->clusters;
    case CPU_TOPO_DIE:
        return &smp_info->dies;
    case CPU_TOPO_SOCKET:
        return &smp_info->sockets;
    case CPU_TOPO_BOOK:
        return &smp_info->books;
    case CPU_TOPO_DRAWER:
        return &smp_info->drawers;
    default:
        /* No need to consider CPU_TOPO_UNKNOWN, and CPU_TOPO_ROOT. */
        g_assert_not_reached();
    }

    return NULL;
}

static const char *get_topo_typename_by_level(CPUTopoLevel level)
{
    switch (level) {
    case CPU_TOPO_CORE:
        return TYPE_CPU_CORE;
    case CPU_TOPO_CLUSTER:
        return TYPE_CPU_CLUSTER;
    case CPU_TOPO_DIE:
        return TYPE_CPU_DIE;
    case CPU_TOPO_SOCKET:
        return TYPE_CPU_SOCKET;
    case CPU_TOPO_BOOK:
        return TYPE_CPU_BOOK;
    case CPU_TOPO_DRAWER:
        return TYPE_CPU_DRAWER;
    default:
        /*
         * No need to consider CPU_TOPO_UNKNOWN, CPU_TOPO_THREAD
         * and CPU_TOPO_ROOT.
         */
        g_assert_not_reached();
    }

    return NULL;
}

static char *get_topo_global_name(CPUTopoStat *stat,
                                  CPUTopoLevel level)
{
    const char *type = NULL;
    CPUTopoStatEntry *entry;

    type = cpu_topo_level_to_string(level);
    entry = get_topo_stat_entry(stat, level);
    return g_strdup_printf("%s[%d]", type, entry->total_units);
}

typedef struct SMPBuildCbData {
    unsigned long *supported_levels;
    unsigned int plugged_cpus;
    CpuTopology *smp_info;
    CPUTopoStat *stat;
    Error **errp;
} SMPBuildCbData;

static int smp_core_set_threads(Object *core, unsigned int max_threads,
                                unsigned int *plugged_cpus, Error **errp)
{
    if (!object_property_set_int(core, "nr-threads", max_threads, errp)) {
        object_unref(core);
        return TOPO_FOREACH_ERR;
    }

    if (*plugged_cpus > max_threads) {
        if (!object_property_set_int(core, "plugged-threads",
                                     max_threads, errp)) {
            object_unref(core);
            return TOPO_FOREACH_ERR;
        }
        *plugged_cpus -= max_threads;
    } else{
        if (!object_property_set_int(core, "plugged-threads",
                                     *plugged_cpus, errp)) {
            object_unref(core);
            return TOPO_FOREACH_ERR;
        }
        *plugged_cpus = 0;
    }

    return TOPO_FOREACH_CONTINUE;
}

static int add_smp_topo_child(CPUTopoState *topo, void *opaque)
{
    CPUTopoLevel level, child_level;
    Object *parent = OBJECT(topo);
    SMPBuildCbData *cb = opaque;
    unsigned int *nr_children;
    Error **errp = cb->errp;

    level = CPU_TOPO_LEVEL(topo);
    child_level = find_last_bit(cb->supported_levels, level);

    /*
     * child_level equals to level, that means no child needs to create.
     * This shouldn't happen.
     */
    g_assert(child_level != level);

    nr_children = get_smp_info_by_level(cb->smp_info, child_level);
    topo->max_children = *nr_children;

    for (int i = 0; i < topo->max_children; i++) {
        ObjectProperty *prop;
        Object *child;
        gchar *name;

        child = object_new(get_topo_typename_by_level(child_level));
        name = get_topo_global_name(cb->stat, child_level);

        prop = object_property_try_add_child(parent, name, child, errp);
        g_free(name);
        if (!prop) {
            return TOPO_FOREACH_ERR;
        }

        if (child_level == CPU_TOPO_CORE) {
            int ret = smp_core_set_threads(child, cb->smp_info->threads,
                                           &cb->plugged_cpus, errp);

            if (ret == TOPO_FOREACH_ERR) {
                return ret;
            }
        }

        qdev_realize(DEVICE(child), NULL, errp);
        if (*errp) {
            return TOPO_FOREACH_ERR;
        }
    }

    return TOPO_FOREACH_CONTINUE;
}

void machine_create_smp_topo_tree(MachineState *ms, Error **errp)
{
    DECLARE_BITMAP(foreach_bitmap, USER_AVAIL_LEVEL_NUM);
    MachineClass *mc = MACHINE_GET_CLASS(ms);
    CPUSlot *slot = ms->topo;
    SMPBuildCbData cb;

    if (!slot) {
        error_setg(errp, "Invalid machine: "
                   "the cpu-slot of machine is not initialized.");
        return;
    }

    if (mc->smp_props.possible_cpus_qom_granu != CPU_TOPO_CORE &&
        mc->smp_props.possible_cpus_qom_granu != CPU_TOPO_THREAD) {
        error_setg(errp, "Invalid machine: Only support building "
                   "qom smp topology with core/thread granularity. "
                   "Not support %s granularity.",
                   cpu_topo_level_to_string(
                       mc->smp_props.possible_cpus_qom_granu));
        return;
    }

    cb.supported_levels = slot->supported_levels;
    cb.plugged_cpus = ms->smp.cpus;
    cb.smp_info = &ms->smp;
    cb.stat = &slot->stat;
    cb.errp = errp;

    if (add_smp_topo_child(CPU_TOPO(slot), &cb) < 0) {
        return;
    }

    bitmap_copy(foreach_bitmap, slot->supported_levels, USER_AVAIL_LEVEL_NUM);

    /*
     * Don't create threads from -smp, and just record threads
     * number in core.
     */
    clear_bit(CPU_TOPO_CORE, foreach_bitmap);
    clear_bit(CPU_TOPO_THREAD, foreach_bitmap);

    /*
     * If the core level is inserted by hotplug way, don't create cores
     * from -smp ethier.
     */
    if (mc->smp_props.possible_cpus_qom_granu == CPU_TOPO_CORE) {
        CPUTopoLevel next_level;

        next_level = find_next_bit(foreach_bitmap, USER_AVAIL_LEVEL_NUM,
                                   CPU_TOPO_CORE + 1);
        clear_bit(next_level, foreach_bitmap);
    }

    cpu_topo_child_foreach_recursive(CPU_TOPO(slot), foreach_bitmap,
                                     add_smp_topo_child, &cb);
    if (*errp) {
        return;
    }
    slot->smp_parsed = true;
}

static void set_smp_child_topo_info(CpuTopology *smp_info,
                                    CPUTopoStat *stat,
                                    CPUTopoLevel child_level)
{
    unsigned int *smp_count;
    CPUTopoStatEntry *entry;

    smp_count = get_smp_info_by_level(smp_info, child_level);
    entry = get_topo_stat_entry(stat, child_level);
    *smp_count = entry->max_units ? entry->max_units : 1;

    return;
}

typedef struct ValidateCbData {
    CPUTopoStat *stat;
    CpuTopology *smp_info;
    Error **errp;
} ValidateCbData;

static int validate_topo_children(CPUTopoState *topo, void *opaque)
{
    CPUTopoLevel level = CPU_TOPO_LEVEL(topo), next_level;
    ValidateCbData *cb = opaque;
    unsigned int max_children;
    CPUTopoStatEntry *entry;
    Error **errp = cb->errp;

    if (level != CPU_TOPO_THREAD && !topo->num_children &&
        !topo->max_children) {
        error_setg(errp, "Invalid topology: the CPU topology "
                   "(level: %s, index: %d) isn't completed.",
                   cpu_topo_level_to_string(level), topo->index);
        return TOPO_FOREACH_ERR;
    }

    if (level == CPU_TOPO_UNKNOWN) {
        error_setg(errp, "Invalid CPU topology: unknown topology level.");
        return TOPO_FOREACH_ERR;
    }

    /*
     * Only CPU_TOPO_THREAD level's child_level could be CPU_TOPO_UNKNOWN,
     * but machine_validate_cpu_topology() is before CPU creation.
     */
    if (topo->child_level == CPU_TOPO_UNKNOWN) {
        error_setg(errp, "Invalid CPU topology: incomplete topology "
                   "(level: %s, index: %d), no child?.",
                   cpu_topo_level_to_string(level), topo->index);
        return TOPO_FOREACH_ERR;
    }

    /*
     * Currently hybrid topology isn't supported, so only SMP topology
     * is allowed.
     */

    entry = get_topo_stat_entry(cb->stat, topo->child_level);

    /* Max threads per core is pre-configured by "nr-threads". */
    max_children = topo->child_level != CPU_TOPO_THREAD ?
                   topo->num_children : topo->max_children;

    if (entry->max_units != max_children) {
        error_setg(errp, "Invalid SMP topology: "
                   "The %s topology is asymmetric.",
                   cpu_topo_level_to_string(level));
        return TOPO_FOREACH_ERR;
    }

    next_level = find_next_bit(cb->stat->curr_levels, USER_AVAIL_LEVEL_NUM,
                               topo->child_level + 1);

    if (next_level != level) {
        error_setg(errp, "Invalid smp topology: "
                   "asymmetric CPU topology depth.");
        return TOPO_FOREACH_ERR;
    }

    set_smp_child_topo_info(cb->smp_info, cb->stat, topo->child_level);

    return TOPO_FOREACH_CONTINUE;
}

/*
 * Only check the case user configures CPU topology via -device
 * without -smp. In this case, MachineState.smp also needs to be
 * initialized based on topology tree.
 */
bool machine_validate_cpu_topology(MachineState *ms, Error **errp)
{
    MachineClass *mc = MACHINE_GET_CLASS(ms);
    CPUTopoState *slot_topo = CPU_TOPO(ms->topo);
    CPUTopoStat *stat = &ms->topo->stat;
    CpuTopology *smp_info = &ms->smp;
    unsigned int total_cpus;
    ValidateCbData cb;

    if (ms->topo->smp_parsed) {
        return true;
    } else if (!slot_topo->num_children) {
        /*
         * If there's no -smp nor -device to add topology children,
         * then create the default topology.
         */
        machine_create_smp_topo_tree(ms, errp);
        if (*errp) {
            return false;
        }
        return true;
    }

    if (test_bit(CPU_TOPO_CLUSTER, stat->curr_levels)) {
        mc->smp_props.has_clusters = true;
    }

    /*
     * The next cpu_topo_child_foreach_recursive() doesn't cover the
     * parent topology unit. Update information for root here.
     */
    set_smp_child_topo_info(smp_info, stat, slot_topo->child_level);

    cb.stat = stat;
    cb.smp_info = smp_info;
    cb.errp = errp;

    cpu_topo_child_foreach_recursive(slot_topo, NULL,
                                     validate_topo_children, &cb);
    if (*errp) {
        return false;
    }

    ms->smp.cpus = stat->pre_plugged_cpus ?
                   stat->pre_plugged_cpus : 1;
    ms->smp.max_cpus = stat->max_cpus ?
                       stat->max_cpus : 1;

    total_cpus = ms->smp.drawers * ms->smp.books *
                 ms->smp.sockets * ms->smp.dies *
                 ms->smp.clusters * ms->smp.cores *
                 ms->smp.threads;
    g_assert(total_cpus == ms->smp.max_cpus);

    return true;
}
