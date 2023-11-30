/*
 * CPU cluster abstract device
 *
 * Copyright (c) 2018 GreenSocs SAS
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see
 * <http://www.gnu.org/licenses/gpl-2.0.html>
 */
#ifndef HW_CPU_CLUSTER_H
#define HW_CPU_CLUSTER_H

#include "hw/core/cpu-topo.h"
#include "hw/qdev-core.h"
#include "qom/object.h"

/*
 * # CPU Cluster
 *
 * A cluster is a group of CPUs, that is, a level above the CPU (or Core).
 *
 * - For accel case, it's a CPU topology level concept above cores, in which
 * the cores may share some resources (L2 cache or some others like L3
 * cache tags, depending on the Archs). It is used to emulate the physical
 * CPU cluster/module.
 *
 * - For TCG, cluster is used to organize CPUs directly without core. In one
 * cluster, CPUs are all identical and have the same view of the rest of the
 * system. It is mainly an internal QEMU representation and may not necessarily
 * match with the notion of clusters on the real hardware.
 *
 * If CPUs are not identical (for example, Cortex-A53 and Cortex-A57 CPUs in an
 * Arm big.LITTLE system) they should be in different clusters. If the CPUs do
 * not have the same view of memory (for example the main CPU and a management
 * controller processor) they should be in different clusters.
 *
 * # Use case for cluster in TCG
 *
 * A cluster is created by creating an object of TYPE_CPU_CLUSTER, and then
 * adding the CPUs to it as QOM child objects (e.g. using the
 * object_initialize_child() or object_property_add_child() functions).
 * The CPUs may be either direct children of the cluster object, or indirect
 * children (e.g. children of children of the cluster object).
 *
 * All CPUs must be added as children before the cluster is realized.
 * (Regrettably QOM provides no way to prevent adding children to a realized
 * object and no way for the parent to be notified when a new child is added
 * to it, so this restriction is not checked for, but the system will not
 * behave correctly if it is not adhered to. The cluster will assert that
 * it contains at least one CPU, which should catch most inadvertent
 * violations of this constraint.)
 *
 * A CPU which is not put into any cluster will be considered implicitly
 * to be in a cluster with all the other "loose" CPUs, so all CPUs that are
 * not assigned to clusters must be identical.
 */

#define TYPE_CPU_CLUSTER "cpu-cluster"
OBJECT_DECLARE_TYPE(CPUCluster, CPUClusterClass, CPU_CLUSTER)

/*
 * This limit is imposed by TCG, which puts the cluster ID into an
 * 8 bit field (and uses all-1s for the default "not in any cluster").
 */
#define MAX_TCG_CLUSTERS 255

struct TCGClusterOps {
    /**
     * @collect_cpus: Iterate children CPUs and set cluser_index.
     *
     * Called when the cluster is realized.
     */
    void (*collect_cpus)(CPUCluster *cluster, Error **errp);
};

struct CPUClusterClass {
    /*< private >*/
    CPUTopoClass parent_class;

    /*< public >*/
    /* when TCG is not available, this pointer is NULL */
    const struct TCGClusterOps *tcg_clu_ops;

    DeviceRealize parent_realize;
};

/**
 * CPUCluster:
 * @cluster_id: The cluster ID. This value is for internal use only and should
 *   not be exposed directly to the user or to the guest.
 *
 * State of a CPU cluster.
 */
struct CPUCluster {
    /*< private >*/
    CPUTopoState parent_obj;

    /*< public >*/
    uint32_t cluster_id;
};

#endif
