#ifndef IHEX_H_
#define IHEX_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "prog_device.h"
#include "dskops.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum ih_proc_rec_status_ {
    IHP_OK = 0,
    IHP_CANCELLED,      // Operation was cancelled (by request)
    IHP_NECHRS,         // Not enough data
    IHP_INVALID,        // Invalid record
    IHP_BADSUM,         // Invalid record - bad checksum
    IHP_NONE_WRITTEN,   // Do data was written to the output
} ihp_stat_t;


extern bool ih_is_hexfile(FF_FILE* fp);

/**
 * @brief Read an IntelHex file, process the records, and write a binary file.
 * @ingroup device
 *
 * @param hexfile Valid IntelHex file open for reading.
 * @param binfile Valid file open for writing and positioned the point to
 *      start writing (assumed to be the 0 point for the output).
 * @return true
 * @return false
 */
extern ihp_stat_t ih_to_bin(FF_FILE* hexfile, FF_FILE* binfile, const progstat_handler_fn progstatfn);

/**
 * @brief Initialize the module. Must be called once/only-once before module use.
 * @ingroup device
 *
 */
extern void ih_modinit();

#ifdef __cplusplus
}
#endif
#endif // IHEX_H_
