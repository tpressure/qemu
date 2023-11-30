/*
 * CPU core abstract device
 *
 * Copyright (C) 2016 Bharata B Rao <bharata@linux.vnet.ibm.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */
#ifndef HW_CPU_CORE_H
#define HW_CPU_CORE_H

#include "hw/qdev-core.h"
#include "hw/core/cpu-topo.h"
#include "qom/object.h"

#define TYPE_CPU_CORE "cpu-core"

OBJECT_DECLARE_TYPE(CPUCore, CPUCoreClass, CPU_CORE)

struct CPUCoreClass {
    /*< private >*/
    CPUTopoClass parent_class;

    /*< public >*/
    DeviceRealize parent_realize;
};

struct CPUCore {
    /*< private >*/
    CPUTopoState parent_obj;

    /*< public >*/
    int core_id;

    /* Maximum number of threads contained in this core. */
    int nr_threads;

    /*
     * How many threads should be plugged in this core via
     * "-device"/"device_add"?
     */
    int plugged_threads;

    QTAILQ_ENTRY(CPUCore) node;
};

#endif
