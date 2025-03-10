/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef TARGET__RISCV__SCANS_H
#define TARGET__RISCV__SCANS_H

#include "target/target.h"
#include "jtag/jtag.h"
#include "riscv.h"

enum riscv_scan_type {
	RISCV_SCAN_TYPE_INVALID,
	RISCV_SCAN_TYPE_NOP,
	RISCV_SCAN_TYPE_READ,
	RISCV_SCAN_TYPE_WRITE,
};

/* A batch of multiple JTAG scans, which are grouped together to avoid the
 * overhead of some JTAG adapters when sending single commands.  This is
 * designed to support block copies, as that's what we actually need to go
 * fast. */
struct riscv_batch {
	struct target *target;

	size_t allocated_scans;
	size_t used_scans;

	size_t idle_count;

	uint8_t *data_out;
	uint8_t *data_in;
	struct scan_field *fields;

	/* If in BSCAN mode, this field will be allocated (one per scan),
	   and utilized to tunnel all the scans in the batch.  If not in
	   BSCAN mode, this field is unallocated and stays NULL */
	riscv_bscan_tunneled_scan_context_t *bscan_ctxt;

	/* In JTAG we scan out the previous value's output when performing a
	 * scan.  This is a pain for users, so we just provide them the
	 * illusion of not having to do this by eliding all but the last NOP.
	 * */
	enum riscv_scan_type last_scan;

	/* The read keys. */
	size_t *read_keys;
	size_t read_keys_used;
};

/* Allocates (or frees) a new scan set.  "scans" is the maximum number of JTAG
 * scans that can be issued to this object, and idle is the number of JTAG idle
 * cycles between every real scan. */
struct riscv_batch *riscv_batch_alloc(struct target *target, size_t scans, size_t idle);
void riscv_batch_free(struct riscv_batch *batch);

/* Checks to see if this batch is full. */
bool riscv_batch_full(struct riscv_batch *batch);

/* Executes this batch of JTAG DTM DMI scans.
 *
 * If resets_delays is true, the algorithm will stop inserting idle cycles
 * (JTAG Run-Test Idle) after "reset_delays_after" number of scans is
 * performed.  This is useful for stress-testing of RISC-V algorithms in
 * OpenOCD that are based on batches.
 */
int riscv_batch_run(struct riscv_batch *batch, bool resets_delays,
		size_t reset_delays_after);

/* Adds a DM register write to this batch. */
void riscv_batch_add_dm_write(struct riscv_batch *batch, uint64_t address, uint32_t data,
	bool read_back);

/* DM register reads must be handled in two parts: the first one schedules a read and
 * provides a key, the second one actually obtains the result of the read -
 * status (op) and the actual data. */
size_t riscv_batch_add_dm_read(struct riscv_batch *batch, uint64_t address);
unsigned int riscv_batch_get_dmi_read_op(const struct riscv_batch *batch, size_t key);
uint32_t riscv_batch_get_dmi_read_data(const struct riscv_batch *batch, size_t key);

/* Scans in a NOP. */
void riscv_batch_add_nop(struct riscv_batch *batch);

/* Returns the number of available scans. */
size_t riscv_batch_available_scans(struct riscv_batch *batch);

/* Return true iff the last scan in the batch returned DMI_OP_BUSY. */
bool riscv_batch_dmi_busy_encountered(const struct riscv_batch *batch);

/* TODO: The function is defined in `riscv-013.c`. This is done to reduce the
 * diff of the commit. The intention is to move the function definition to
 * a separate module (e.g. `riscv013-jtag-dtm.c/h`) in another commit. */
void riscv_decode_dmi_scan(const struct target *target, int idle, const struct scan_field *field,
		bool discard_in);

#endif
