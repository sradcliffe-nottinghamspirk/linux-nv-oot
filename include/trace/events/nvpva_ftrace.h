// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023, NVIDIA Corporation.  All rights reserved.
 *
 * NVPVA event logging to ftrace
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM nvpva_ftrace

#if !defined(_TRACE_NVPVA_FTRACE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_NVPVA_FTRACE_H

#include <linux/tracepoint.h>
#include <linux/device.h>

TRACE_EVENT(job_submit,
	TP_PROTO(struct device *dev, u32 class_id, u32 job_id, u32 num_fences,
		  u64 prog_id, u64 stream_id, u64 hw_timestamp),
	TP_ARGS(dev, class_id, job_id, num_fences, prog_id, stream_id, hw_timestamp),
	TP_STRUCT__entry(
		__field(struct device *, dev)
		__field(u32, class_id)
		__field(u32, job_id)
		__field(u32, num_fences)
		__field(u64, prog_id)
		__field(u64, stream_id)
		__field(u64, hw_timestamp)
	),
	TP_fast_assign(
		__entry->dev = dev;
		__entry->class_id = class_id;
		__entry->job_id = job_id;
		__entry->num_fences = num_fences;
		__entry->prog_id = prog_id;
		__entry->stream_id = stream_id;
		__entry->hw_timestamp = hw_timestamp;
	),
	TP_printk("%s class=%02x id=%u fences=%u stream_id=%llu prog_id=%llu ts=%llu",
		dev_name(__entry->dev), __entry->class_id, __entry->job_id,
		__entry->num_fences, __entry->prog_id, __entry->stream_id,
		__entry->hw_timestamp
	)
);

TRACE_EVENT(pva_job_base_event,
	TP_PROTO(u32 job_id, u32 syncpt_id, u32 threshold, u32 vpu_id,
		 u32 queue_id, u64 vpu_begin_timestamp, u64 vpu_end_timestamp),
	TP_ARGS(job_id, syncpt_id, threshold, vpu_id, queue_id,
		vpu_begin_timestamp, vpu_end_timestamp),
	TP_STRUCT__entry(
		__field(u64, vpu_begin_timestamp)
		__field(u64, vpu_end_timestamp)
		__field(u32, job_id)
		__field(u32, syncpt_id)
		__field(u32, threshold)
		__field(u32, vpu_id)
		__field(u32, queue_id)
	),
	TP_fast_assign(
		__entry->job_id    = job_id;
		__entry->syncpt_id = syncpt_id;
		__entry->threshold = threshold;
		__entry->vpu_begin_timestamp = vpu_begin_timestamp;
		__entry->vpu_end_timestamp = vpu_end_timestamp;
		__entry->queue_id  = queue_id;
		__entry->vpu_id    = vpu_id;
	),
	TP_printk("job_id=%u syncpt_id=%u threshold=%u vpu_id=%u "
		  "queue_id=%u vpu_begin=%llu vpu_end=%llu ",
		__entry->job_id, __entry->syncpt_id, __entry->threshold,
		__entry->vpu_id, __entry->queue_id, __entry->vpu_begin_timestamp,
		__entry->vpu_end_timestamp
	)
);

TRACE_EVENT(pva_job_ext_event,
	TP_PROTO(u32 job_id, u32 syncpt_id, u32 threshold, u32 vpu_id, u32 queue_id,
		 u64 queue_begin_timestamp, u64 queue_end_timestamp,
		 u64 prepare_begin_timestamp, u64 prepare_end_timestamp,
		 u64 vpu_begin_timestamp, u64 vpu_end_timestamp,
		 u64 post_begin_timestamp, u64 post_end_timestamp),
	TP_ARGS(job_id, syncpt_id, threshold, vpu_id, queue_id,
		queue_begin_timestamp, queue_end_timestamp,
		prepare_begin_timestamp, prepare_end_timestamp,
		vpu_begin_timestamp, vpu_end_timestamp,
		post_begin_timestamp, post_end_timestamp),
	TP_STRUCT__entry(
		__field(u64, queue_begin_timestamp)
		__field(u64, queue_end_timestamp)
		__field(u64, prepare_begin_timestamp)
		__field(u64, prepare_end_timestamp)
		__field(u64, vpu_begin_timestamp)
		__field(u64, vpu_end_timestamp)
		__field(u64, post_begin_timestamp)
		__field(u64, post_end_timestamp)
		__field(u32, job_id)
		__field(u32, syncpt_id)
		__field(u32, threshold)
		__field(u32, vpu_id)
		__field(u32, queue_id)
	),
	TP_fast_assign(
		__entry->job_id    = job_id;
		__entry->syncpt_id = syncpt_id;
		__entry->threshold = threshold;
		__entry->queue_begin_timestamp = queue_begin_timestamp;
		__entry->queue_end_timestamp = queue_end_timestamp;
		__entry->prepare_begin_timestamp = prepare_begin_timestamp;
		__entry->prepare_end_timestamp = prepare_end_timestamp;
		__entry->vpu_begin_timestamp = vpu_begin_timestamp;
		__entry->vpu_end_timestamp = vpu_end_timestamp;
		__entry->post_begin_timestamp = post_begin_timestamp;
		__entry->post_end_timestamp = post_end_timestamp;
		__entry->queue_id  = queue_id;
		__entry->vpu_id    = vpu_id;
	),
	TP_printk("job_id=%u syncpt_id=%u threshold=%u vpu_id=%u queue_id=%u "
		  "queue_begin=%llu queue_end=%llu "
		  "prepare_begin=%llu prepare_end=%llu "
		  "vpu_begin=%llu vpu_end=%llu "
		  "post_begin=%llu post_end=%llu",
		  __entry->job_id, __entry->syncpt_id, __entry->threshold,
		  __entry->vpu_id, __entry->queue_id,
		  __entry->queue_begin_timestamp, __entry->queue_end_timestamp,
		  __entry->prepare_begin_timestamp, __entry->prepare_end_timestamp,
		  __entry->vpu_begin_timestamp, __entry->vpu_end_timestamp,
		  __entry->post_begin_timestamp, __entry->post_end_timestamp
	)
);

DECLARE_EVENT_CLASS(job_fence,
	TP_PROTO(u32 job_id, u32 syncpt_id, u32 threshold),
	TP_ARGS(job_id, syncpt_id, threshold),
	TP_STRUCT__entry(
		__field(u32, job_id)
		__field(u32, syncpt_id)
		__field(u32, threshold)
	),
	TP_fast_assign(
		__entry->job_id = job_id;
		__entry->syncpt_id = syncpt_id;
		__entry->threshold = threshold;
	),
	TP_printk("job_id=%u syncpt_id=%u threshold=%u",
		__entry->job_id, __entry->syncpt_id, __entry->threshold
	)
);

DEFINE_EVENT(job_fence, job_prefence,
	TP_PROTO(u32 job_id, u32 syncpt_id, u32 threshold),
	TP_ARGS(job_id, syncpt_id, threshold));

DEFINE_EVENT(job_fence, job_postfence,
	TP_PROTO(u32 job_id, u32 syncpt_id, u32 threshold),
	TP_ARGS(job_id, syncpt_id, threshold));

TRACE_EVENT(job_timestamps,
	TP_PROTO(u32 job_id, u64 begin, u64 end),
	TP_ARGS(job_id, begin, end),
	TP_STRUCT__entry(
		__field(u32, job_id)
		__field(u64, begin)
		__field(u64, end)
	),
	TP_fast_assign(
		__entry->job_id = job_id;
		__entry->begin = begin;
		__entry->end = end;
	),
	TP_printk("job_id=%u begin=%llu end=%llu",
		__entry->job_id, __entry->begin, __entry->end
	)
);

#endif /* End of _TRACE_NVPVA_FTRACE_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
