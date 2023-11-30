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

#ifndef CPU_SLOT_H
#define CPU_SLOT_H

#include "hw/core/cpu-topo.h"
#include "hw/cpu/core.h"
#include "hw/qdev-core.h"

/**
 * @USER_AVAIL_LEVEL_NUM: the number of total topology levels in topology
 *                        bitmap, which includes CPU_TOPO_UNKNOWN.
 */
#define USER_AVAIL_LEVEL_NUM (CPU_TOPO_ROOT + 1)

/**
 * @VALID_LEVEL_NUM: the number of valid topology levels, which excludes
 *                   CPU_TOPO_UNKNOWN and CPU_TOPO_ROOT.
 */
#define VALID_LEVEL_NUM (CPU_TOPO_ROOT - 1)

#define TOPO_STAT_ENTRY_IDX(level) ((level) - 1)

/**
 * CPUTopoStatEntry:
 * @total: Total number of topological units at the same level that are
 *         currently inserted in CPU slot
 * @max: Maximum number of topological units at the same level under the
 *       parent topolofical container
 */
typedef struct CPUTopoStatEntry {
    unsigned int total_units;
    unsigned int max_units;
} CPUTopoStatEntry;

/**
 * CPUTopoStat:
 * @max_cpus: Maximum number of CPUs in CPU slot.
 * @pre_plugged_cpus: Number of pre-plugged CPUs in CPU slot.
 * @entries: Detail count information for valid topology levels under
 *           CPU slot
 * @curr_levels: Current CPU topology levels inserted in CPU slot
 */
typedef struct CPUTopoStat {
    unsigned int max_cpus;
    unsigned int pre_plugged_cpus;
    CPUTopoStatEntry entries[VALID_LEVEL_NUM];
    DECLARE_BITMAP(curr_levels, USER_AVAIL_LEVEL_NUM);
} CPUTopoStat;

#define TYPE_CPU_SLOT "cpu-slot"

OBJECT_DECLARE_SIMPLE_TYPE(CPUSlot, CPU_SLOT)

/**
 * CPUSlot:
 * @cores: Queue consisting of all the cores in the topology tree
 *     where the cpu-slot is the root. cpu-slot can maintain similar
 *     queues for other topology levels to facilitate traversal
 *     when necessary.
 * @stat: Statistical topology information for topology tree.
 * @supported_levels: Supported topology levels for topology tree.
 * @ms: Machine in which this cpu-slot is plugged.
 * @smp_parsed: Flag indicates if topology tree is derived from "-smp".
 *      If not, MachineState.smp needs to be initialized based on
 *      topology tree.
 */
struct CPUSlot {
    /*< private >*/
    CPUTopoState parent_obj;

    /*< public >*/
    QTAILQ_HEAD(, CPUCore) cores;
    CPUTopoStat stat;
    DECLARE_BITMAP(supported_levels, USER_AVAIL_LEVEL_NUM);
    MachineState *ms;
    bool smp_parsed;
};

#define MACHINE_CORE_FOREACH(ms, core) \
    QTAILQ_FOREACH((core), &(ms)->topo->cores, node)

void machine_plug_cpu_slot(MachineState *ms);
void machine_create_smp_topo_tree(MachineState *ms, Error **errp);
bool machine_validate_cpu_topology(MachineState *ms, Error **errp);

#endif /* CPU_SLOT_H */
