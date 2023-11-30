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

#ifndef CPU_TOPO_H
#define CPU_TOPO_H

#include "hw/qdev-core.h"
#include "qemu/queue.h"

#define UNASSIGNED_TOPO_INDEX -1

typedef enum CPUTopoLevel {
    CPU_TOPO_UNKNOWN,
    CPU_TOPO_THREAD,
    CPU_TOPO_CORE,
    CPU_TOPO_CLUSTER,
    CPU_TOPO_DIE,
    CPU_TOPO_SOCKET,
    CPU_TOPO_BOOK,
    CPU_TOPO_DRAWER,
    CPU_TOPO_ROOT,
} CPUTopoLevel;

#define TYPE_CPU_TOPO "cpu-topo"
OBJECT_DECLARE_TYPE(CPUTopoState, CPUTopoClass, CPU_TOPO)

/**
 * CPUTopoClass:
 * @level: Topology level for this CPUTopoClass.
 * @update_topo_info: Method to update topology information statistics when
 *     new child (including direct child and non-direct child) is added.
 * @check_topo_child: Method to check the support for new child (including
 *     direct child and non-direct child) to be added.
 */
struct CPUTopoClass {
    /*< private >*/
    DeviceClass parent_class;

    /*< public >*/
    CPUTopoLevel level;
    void (*update_topo_info)(CPUTopoState *parent, CPUTopoState *child,
                             bool is_realize);
    void (*check_topo_child)(CPUTopoState *parent, CPUTopoState *child,
                             Error **errp);
};

/**
 * CPUTopoState:
 * @index: Topology index within parent's topology queue.
 * @free_child_index: Cached free index to be specified for next child.
 * @num_children: Number of topology children under this topology device.
 * @max_children: Maximum number of children allowed to be inserted under
 *     this topology device.
 * @child_level: Topology level for children.
 * @parent: Topology parent of this topology device.
 * @children: Queue of topology children.
 * @sibling: Queue node to be inserted in parent's topology queue.
 */
struct CPUTopoState {
    /*< private >*/
    DeviceState parent_obj;

    /*< public >*/
    int index;
    int free_child_index;
    int num_children;
    int max_children;
    CPUTopoLevel child_level;
    struct CPUTopoState *parent;
    QTAILQ_HEAD(, CPUTopoState) children;
    QTAILQ_ENTRY(CPUTopoState) sibling;
};

#define CPU_TOPO_LEVEL(topo)    (CPU_TOPO_GET_CLASS(topo)->level)

#endif /* CPU_TOPO_H */
