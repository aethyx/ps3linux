/*
 * SPU core / file system interface and HW structures
 *
 * (C) Copyright IBM Deutschland Entwicklung GmbH 2005
 *
 * Author: Arnd Bergmann <arndb@de.ibm.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 *  PS3 Platform spu routines.
 *
 *  Copyright (C) 2012 glevand <geoffrey.levand@mail.ru>
 *  Copyright (C) 2006 Sony Computer Entertainment Inc.
 *  Copyright 2006 Sony Corp.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _SPU_H
#define _SPU_H

#define LS_SIZE				(256 * 1024)
#define LS_ADDR_MASK			(LS_SIZE - 1)

#define MFC_PUT_CMD			0x20
#define MFC_GET_CMD			0x40

#define MFC_TAGID_TO_TAGMASK(tag_id)	(1 << (tag_id & 0x1F))

union mfc_tag_size_class_cmd {
	struct {
		uint16_t mfc_size;
		uint16_t mfc_tag;
		uint8_t  pad;
		uint8_t  mfc_rclassid;
		uint16_t mfc_cmd;
	} u;
	struct {
		uint32_t mfc_size_tag32;
		uint32_t mfc_class_cmd32;
	} by32;
	uint64_t all64;
};

struct mfc_cq_sr {
	uint64_t mfc_cq_data0_RW;
	uint64_t mfc_cq_data1_RW;
	uint64_t mfc_cq_data2_RW;
	uint64_t mfc_cq_data3_RW;
};

struct spu_problem {
#define MS_SYNC_PENDING         1L
	uint64_t spc_mssync_RW;					/* 0x0000 */
	uint8_t  pad_0x0008_0x3000[0x3000 - 0x0008];

	/* DMA Area */
	uint8_t  pad_0x3000_0x3004[0x4];			/* 0x3000 */
	uint32_t mfc_lsa_W;					/* 0x3004 */
	uint64_t mfc_ea_W;					/* 0x3008 */
	union mfc_tag_size_class_cmd mfc_union_W;		/* 0x3010 */
	uint8_t  pad_0x3018_0x3104[0xec];			/* 0x3018 */
	uint32_t dma_qstatus_R;					/* 0x3104 */
	uint8_t  pad_0x3108_0x3204[0xfc];			/* 0x3108 */
	uint32_t dma_querytype_RW;				/* 0x3204 */
	uint8_t  pad_0x3208_0x321c[0x14];			/* 0x3208 */
	uint32_t dma_querymask_RW;				/* 0x321c */
	uint8_t  pad_0x3220_0x322c[0xc];			/* 0x3220 */
	uint32_t dma_tagstatus_R;				/* 0x322c */
#define DMA_TAGSTATUS_INTR_ANY	1u
#define DMA_TAGSTATUS_INTR_ALL	2u
	uint8_t  pad_0x3230_0x4000[0x4000 - 0x3230]; 		/* 0x3230 */

	/* SPU Control Area */
	uint8_t  pad_0x4000_0x4004[0x4];			/* 0x4000 */
	uint32_t pu_mb_R;					/* 0x4004 */
	uint8_t  pad_0x4008_0x400c[0x4];			/* 0x4008 */
	uint32_t spu_mb_W;					/* 0x400c */
	uint8_t  pad_0x4010_0x4014[0x4];			/* 0x4010 */
	uint32_t mb_stat_R;					/* 0x4014 */
	uint8_t  pad_0x4018_0x401c[0x4];			/* 0x4018 */
	uint32_t spu_runcntl_RW;				/* 0x401c */
#define SPU_RUNCNTL_STOP	0L
#define SPU_RUNCNTL_RUNNABLE	1L
#define SPU_RUNCNTL_ISOLATE	2L
	uint8_t  pad_0x4020_0x4024[0x4];			/* 0x4020 */
	uint32_t spu_status_R;					/* 0x4024 */
#define SPU_STOP_STATUS_SHIFT           16
#define SPU_STATUS_STOPPED		0x0
#define SPU_STATUS_RUNNING		0x1
#define SPU_STATUS_STOPPED_BY_STOP	0x2
#define SPU_STATUS_STOPPED_BY_HALT	0x4
#define SPU_STATUS_WAITING_FOR_CHANNEL	0x8
#define SPU_STATUS_SINGLE_STEP		0x10
#define SPU_STATUS_INVALID_INSTR        0x20
#define SPU_STATUS_INVALID_CH           0x40
#define SPU_STATUS_ISOLATED_STATE       0x80
#define SPU_STATUS_ISOLATED_LOAD_STATUS 0x200
#define SPU_STATUS_ISOLATED_EXIT_STATUS 0x400
	uint8_t  pad_0x4028_0x402c[0x4];			/* 0x4028 */
	uint32_t spu_spe_R;					/* 0x402c */
	uint8_t  pad_0x4030_0x4034[0x4];			/* 0x4030 */
	uint32_t spu_npc_RW;					/* 0x4034 */
	uint8_t  pad_0x4038_0x14000[0x14000 - 0x4038];		/* 0x4038 */

	/* Signal Notification Area */
	uint8_t  pad_0x14000_0x1400c[0xc];			/* 0x14000 */
	uint32_t signal_notify1;				/* 0x1400c */
	uint8_t  pad_0x14010_0x1c00c[0x7ffc];			/* 0x14010 */
	uint32_t signal_notify2;				/* 0x1c00c */
} __attribute__ ((aligned(0x20000)));

/* SPU Privilege 2 State Area */
struct spu_priv2 {
	/* MFC Registers */
	uint8_t  pad_0x0000_0x1100[0x1100 - 0x0000]; 		/* 0x0000 */

	/* SLB Management Registers */
	uint8_t  pad_0x1100_0x1108[0x8];			/* 0x1100 */
	uint64_t slb_index_W;					/* 0x1108 */
#define SLB_INDEX_MASK				0x7L
	uint64_t slb_esid_RW;					/* 0x1110 */
	uint64_t slb_vsid_RW;					/* 0x1118 */
#define SLB_VSID_SUPERVISOR_STATE	(0x1ull << 11)
#define SLB_VSID_SUPERVISOR_STATE_MASK	(0x1ull << 11)
#define SLB_VSID_PROBLEM_STATE		(0x1ull << 10)
#define SLB_VSID_PROBLEM_STATE_MASK	(0x1ull << 10)
#define SLB_VSID_EXECUTE_SEGMENT	(0x1ull << 9)
#define SLB_VSID_NO_EXECUTE_SEGMENT	(0x1ull << 9)
#define SLB_VSID_EXECUTE_SEGMENT_MASK	(0x1ull << 9)
#define SLB_VSID_4K_PAGE		(0x0 << 8)
#define SLB_VSID_LARGE_PAGE		(0x1ull << 8)
#define SLB_VSID_PAGE_SIZE_MASK		(0x1ull << 8)
#define SLB_VSID_CLASS_MASK		(0x1ull << 7)
#define SLB_VSID_VIRTUAL_PAGE_SIZE_MASK	(0x1ull << 6)
	uint64_t slb_invalidate_entry_W;			/* 0x1120 */
	uint64_t slb_invalidate_all_W;				/* 0x1128 */
	uint8_t  pad_0x1130_0x2000[0x2000 - 0x1130]; 		/* 0x1130 */

	/* Context Save / Restore Area */
	struct mfc_cq_sr spuq[16];				/* 0x2000 */
	struct mfc_cq_sr puq[8];				/* 0x2200 */
	uint8_t  pad_0x2300_0x3000[0x3000 - 0x2300]; 		/* 0x2300 */

	/* MFC Control */
	uint64_t mfc_control_RW;				/* 0x3000 */
#define MFC_CNTL_RESUME_DMA_QUEUE		(0ull << 0)
#define MFC_CNTL_SUSPEND_DMA_QUEUE		(1ull << 0)
#define MFC_CNTL_SUSPEND_DMA_QUEUE_MASK		(1ull << 0)
#define MFC_CNTL_SUSPEND_MASK			(1ull << 4)
#define MFC_CNTL_NORMAL_DMA_QUEUE_OPERATION	(0ull << 8)
#define MFC_CNTL_SUSPEND_IN_PROGRESS		(1ull << 8)
#define MFC_CNTL_SUSPEND_COMPLETE		(3ull << 8)
#define MFC_CNTL_SUSPEND_DMA_STATUS_MASK	(3ull << 8)
#define MFC_CNTL_DMA_QUEUES_EMPTY		(1ull << 14)
#define MFC_CNTL_DMA_QUEUES_EMPTY_MASK		(1ull << 14)
#define MFC_CNTL_PURGE_DMA_REQUEST		(1ull << 15)
#define MFC_CNTL_PURGE_DMA_IN_PROGRESS		(1ull << 24)
#define MFC_CNTL_PURGE_DMA_COMPLETE		(3ull << 24)
#define MFC_CNTL_PURGE_DMA_STATUS_MASK		(3ull << 24)
#define MFC_CNTL_RESTART_DMA_COMMAND		(1ull << 32)
#define MFC_CNTL_DMA_COMMAND_REISSUE_PENDING	(1ull << 32)
#define MFC_CNTL_DMA_COMMAND_REISSUE_STATUS_MASK (1ull << 32)
#define MFC_CNTL_MFC_PRIVILEGE_STATE		(2ull << 33)
#define MFC_CNTL_MFC_PROBLEM_STATE		(3ull << 33)
#define MFC_CNTL_MFC_KEY_PROTECTION_STATE_MASK	(3ull << 33)
#define MFC_CNTL_DECREMENTER_HALTED		(1ull << 35)
#define MFC_CNTL_DECREMENTER_RUNNING		(1ull << 40)
#define MFC_CNTL_DECREMENTER_STATUS_MASK	(1ull << 40)
	uint8_t  pad_0x3008_0x4000[0x4000 - 0x3008]; 		/* 0x3008 */

	/* Interrupt Mailbox */
	uint64_t puint_mb_R;					/* 0x4000 */
	uint8_t  pad_0x4008_0x4040[0x4040 - 0x4008]; 		/* 0x4008 */

	/* SPU Control */
	uint64_t spu_privcntl_RW;				/* 0x4040 */
#define SPU_PRIVCNTL_MODE_NORMAL		(0x0ull << 0)
#define SPU_PRIVCNTL_MODE_SINGLE_STEP		(0x1ull << 0)
#define SPU_PRIVCNTL_MODE_MASK			(0x1ull << 0)
#define SPU_PRIVCNTL_NO_ATTENTION_EVENT		(0x0ull << 1)
#define SPU_PRIVCNTL_ATTENTION_EVENT		(0x1ull << 1)
#define SPU_PRIVCNTL_ATTENTION_EVENT_MASK	(0x1ull << 1)
#define SPU_PRIVCNT_LOAD_REQUEST_NORMAL		(0x0ull << 2)
#define SPU_PRIVCNT_LOAD_REQUEST_ENABLE_MASK	(0x1ull << 2)
	uint8_t  pad_0x4048_0x4058[0x10];			/* 0x4048 */
	uint64_t spu_lslr_RW;					/* 0x4058 */
	uint64_t spu_chnlcntptr_RW;				/* 0x4060 */
	uint64_t spu_chnlcnt_RW;				/* 0x4068 */
	uint64_t spu_chnldata_RW;				/* 0x4070 */
	uint64_t spu_cfg_RW;					/* 0x4078 */
	uint8_t  pad_0x4080_0x5000[0x5000 - 0x4080]; 		/* 0x4080 */

	/* PV2_ImplRegs: Implementation-specific privileged-state 2 regs */
	uint64_t spu_pm_trace_tag_status_RW;			/* 0x5000 */
	uint64_t spu_tag_status_query_RW;			/* 0x5008 */
#define TAG_STATUS_QUERY_CONDITION_BITS (0x3ull << 32)
#define TAG_STATUS_QUERY_MASK_BITS (0xffffffffull)
	uint64_t spu_cmd_buf1_RW;				/* 0x5010 */
#define SPU_COMMAND_BUFFER_1_LSA_BITS (0x7ffffull << 32)
#define SPU_COMMAND_BUFFER_1_EAH_BITS (0xffffffffull)
	uint64_t spu_cmd_buf2_RW;				/* 0x5018 */
#define SPU_COMMAND_BUFFER_2_EAL_BITS ((0xffffffffull) << 32)
#define SPU_COMMAND_BUFFER_2_TS_BITS (0xffffull << 16)
#define SPU_COMMAND_BUFFER_2_TAG_BITS (0x3full)
	uint64_t spu_atomic_status_RW;				/* 0x5020 */
} __attribute__ ((aligned(0x20000)));

/**
 * struct spe_shadow - logical spe shadow register area.
 *
 * Read-only shadow of spe registers.
 */

struct spe_shadow {
	uint8_t padding_0140[0x0140];
	uint64_t int_status_class0_RW;       			/* 0x0140 */
	uint64_t int_status_class1_RW;       			/* 0x0148 */
	uint64_t int_status_class2_RW;       			/* 0x0150 */
	uint8_t padding_0158[0x0610-0x0158];
	uint64_t mfc_dsisr_RW;               			/* 0x0610 */
	uint8_t padding_0618[0x0620-0x0618];
	uint64_t mfc_dar_RW;                 			/* 0x0620 */
	uint8_t padding_0628[0x0800-0x0628];
	uint64_t mfc_dsipr_R;                			/* 0x0800 */
	uint8_t padding_0808[0x0810-0x0808];
	uint64_t mfc_lscrr_R;                			/* 0x0810 */
	uint8_t padding_0818[0x0c00-0x0818];
	uint64_t mfc_cer_R;                  			/* 0x0c00 */
	uint8_t padding_0c08[0x0f00-0x0c08];
	uint64_t spe_execution_status;       			/* 0x0f00 */
	uint8_t padding_0f08[0x1000-0x0f08];
};

/*
 * in_be8
 */
static inline uint8_t in_be8(const volatile uint8_t *addr)
{
	uint8_t ret;

	__asm__ __volatile__ ("sync" : : : "memory");
	ret = *addr;
	__asm__ __volatile__ ("isync" : : : "memory");

	return (ret);
}

/*
 * in_be16
 */
static inline uint16_t in_be16(const volatile uint16_t *addr)
{
	uint16_t ret;

	__asm__ __volatile__ ("sync" : : : "memory");
	ret = *addr;
	__asm__ __volatile__ ("isync" : : : "memory");

	return (ret);
}

/*
 * in_be32
 */
static inline uint32_t in_be32(const volatile uint32_t *addr)
{
	uint32_t ret;

	__asm__ __volatile__ ("sync" : : : "memory");
	ret = *addr;
	__asm__ __volatile__ ("isync" : : : "memory");

	return (ret);
}

/*
 * in_be64
 */
static inline uint64_t in_be64(const volatile uint64_t *addr)
{
	uint64_t ret;

	__asm__ __volatile__ ("sync" : : : "memory");
	ret = *addr;
	__asm__ __volatile__ ("isync" : : : "memory");

	return (ret);
}

/*
 * out_be8
 */
static inline void out_be8(volatile uint8_t *addr, uint8_t val)
{
	__asm__ __volatile__ ("sync" : : : "memory");
	*addr = val;
}

/*
 * out_be16
 */
static inline void out_be16(volatile uint16_t *addr, uint16_t val)
{
	__asm__ __volatile__ ("sync" : : : "memory");
	*addr = val;
}

/*
 * out_be32
 */
static inline void out_be32(volatile uint32_t *addr, uint32_t val)
{
	__asm__ __volatile__ ("sync" : : : "memory");
	*addr = val;
}

/*
 * out_be64
 */
static inline void out_be64(volatile uint64_t *addr, uint64_t val)
{
	__asm__ __volatile__ ("sync" : : : "memory");
	*addr = val;
}

int spu_mfc_dma_xfer(struct spu_problem *problem, uint32_t lsa, uint64_t ea,
	uint16_t size, uint16_t tag, uint16_t class, uint16_t cmd);

void spu_mfc_dma_wait(struct spu_problem *problem, uint16_t tag);

#endif
