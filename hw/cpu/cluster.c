/*
 * QEMU CPU cluster
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

#include "qemu/osdep.h"

#include "hw/core/cpu.h"
#include "hw/cpu/cluster.h"
#include "hw/qdev-properties.h"
#include "qapi/error.h"

static Property cpu_cluster_properties[] = {
#ifdef CONFIG_TCG
    DEFINE_PROP_UINT32("cluster-id", CPUCluster, cluster_id, 0),
#endif
    DEFINE_PROP_END_OF_LIST()
};

typedef struct CallbackData {
    CPUCluster *cluster;
    int cpu_count;
} CallbackData;

static int add_cpu_to_cluster(Object *obj, void *opaque)
{
    CallbackData *cbdata = opaque;
    CPUState *cpu = (CPUState *)object_dynamic_cast(obj, TYPE_CPU);

    if (cpu) {
        cpu->cluster_index = cbdata->cluster->cluster_id;
        cbdata->cpu_count++;
    }
    return 0;
}

static void cpu_cluster_common_collect_cpus(CPUCluster *cluster, Error **errp)
{
    /* Iterate through all our CPU children and set their cluster_index */
    Object *cluster_obj = OBJECT(cluster);
    CallbackData cbdata = {
        .cluster = cluster,
        .cpu_count = 0,
    };

    if (cluster->cluster_id >= MAX_TCG_CLUSTERS) {
        error_setg(errp, "cluster-id must be less than %d", MAX_TCG_CLUSTERS);
        return;
    }

    object_child_foreach_recursive(cluster_obj, add_cpu_to_cluster, &cbdata);

    /*
     * A cluster with no CPUs is a bug in the board/SoC code that created it;
     * if you hit this during development of new code, check that you have
     * created the CPUs and parented them into the cluster object before
     * realizing the cluster object.
     */
    assert(cbdata.cpu_count > 0);
}

static const struct TCGClusterOps common_cluster_tcg_ops = {
    .collect_cpus = cpu_cluster_common_collect_cpus,
};

static void cpu_cluster_realize(DeviceState *dev, Error **errp)
{
    CPUCluster *cluster = CPU_CLUSTER(dev);
    CPUClusterClass *cc = CPU_CLUSTER_GET_CLASS(dev);

    if (cc->tcg_clu_ops->collect_cpus) {
        cc->tcg_clu_ops->collect_cpus(cluster, errp);
    }
}

static void cpu_cluster_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    CPUClusterClass *cc = CPU_CLUSTER_CLASS(klass);

    device_class_set_props(dc, cpu_cluster_properties);
    dc->realize = cpu_cluster_realize;

    /* This is not directly for users, CPU children must be attached by code */
    dc->user_creatable = false;

#ifdef CONFIG_TCG
    cc->tcg_clu_ops = &common_cluster_tcg_ops;
#endif
}

static const TypeInfo cpu_cluster_type_info = {
    .name = TYPE_CPU_CLUSTER,
    .parent = TYPE_DEVICE,
    .instance_size = sizeof(CPUCluster),
    .class_init = cpu_cluster_class_init,
};

static void cpu_cluster_register_types(void)
{
    type_register_static(&cpu_cluster_type_info);
}

type_init(cpu_cluster_register_types)
