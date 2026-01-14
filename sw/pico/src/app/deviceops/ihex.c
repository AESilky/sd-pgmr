/**
 * Intel (Intellect) Hex Routines.
 *
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/

#include "ihex.h"

#include "board.h"
#include "dskops.h"
#include "msgpost.h"
#include "include/util.h"

#include "pico/types.h" // 'uint' and other standard types

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h> // For memcpy

// ====================================================================
// Data Section
// ====================================================================

static volatile bool _initialized;

static volatile bool _cancel;

#define RECSTART ':'
#define RECSIZEMIN 11 // The minimum number of text characters in a valid record
#define MAXRECSIZE 255

#define TBUFSIZE 512 // Keep this an 'even' binary size
static uint8_t _tbuf[TBUFSIZE];
static uint8_t _trec[MAXRECSIZE+1];

/**
 * @brief IntelHex record type indicators.
 * @ingroup ihex
 */
typedef enum ihex_rec_type_ {
    ihrt_invalid = -1,
    ihrt_data = 0,
    ihrt_eof = 1,
    ihrt_extsaddr = 2,
    ihrt_startsaddr = 3,
    ihrt_extladdr = 4,
    ihrt_startladdr = 5,
} ih_rec_type;


// ====================================================================
// Local/Private Method Declarations
// ====================================================================

static ihp_stat_t _getrecord(const char* inbuf, uint16_t chars, uint16_t* charsused, uint8_t* rec, uint8_t* recsize, uint16_t* addr, ih_rec_type* type);

static bool _valid_rec_type(int rt);


// ====================================================================
// Local/Private Methods
// ====================================================================

static ihp_stat_t _getrecord(const char* inbuf, uint16_t chars, uint16_t* charsused, uint8_t* rec, uint8_t* recsize, uint16_t* addr, ih_rec_type* type) {
    // First character must be RECSTART
    if (chars < RECSIZEMIN || *inbuf != RECSTART) {
        *charsused = 0;
        return IHP_INVALID;
    }
    // Okay, we know the first char is a RECSTART and we have enough characters for
    // a minimum sized record, so we can at least get the LENGTH, ADDRESS-OFFSET,
    // and RECORD-TYPE. Then we can check again.
    const char* bcpy = inbuf;
    char hb[5];
    // Make sure the checksum is correct
    int sum = 0;
    bool valid;
    bcpy++;
    bcpy += strcpynt(hb, bcpy, 2);
    uint8_t b = uint_from_hexstr(hb, &valid);
    if (!valid) {
        *charsused = (uint16_t)(bcpy - inbuf);
        return IHP_INVALID;
    }
    int len = b;
    sum += b;
    // Get the Address Offset and the Record Type
    bcpy += strcpynt(hb, bcpy, 4);
    uint16_t addroff = uint_from_hexstr(hb, &valid);
    if (!valid) {
        *charsused = (uint16_t)(bcpy - inbuf);
        return IHP_INVALID;
    }
    sum += highByte(addroff);
    sum += lowByte(addroff);
    *addr = addroff;
    bcpy += strcpynt(hb, bcpy, 2);
    b = uint_from_hexstr(hb, &valid);
    if (!valid || !_valid_rec_type(b)) {
        *charsused = (uint16_t)(bcpy - inbuf);
        return IHP_INVALID;
    }
    sum += b;
    *type = (ih_rec_type)b;
    // Process the record for the record len.
    //
    // Make sure that we have enough characters to process the rest of the record.
    int charsneeded = 2 * (len + 1);
    if (charsneeded > (chars - (bcpy - inbuf))) {
        *charsused = 0;
        return IHP_NECHRS;
    }
    // Enough chars, so finish processing the record.
    for (int i = 0; i < len; i++) {
        bcpy += strcpynt(hb, bcpy, 2);
        b = uint_from_hexstr(hb, &valid);
        if (!valid) {
            *charsused = (uint16_t)(bcpy - inbuf);
            return IHP_INVALID;
        }
        sum += b;
        rec[i] = b;
    }
    // Read the checksum and add it in
    bcpy += strcpynt(hb, bcpy, 2);
    b = uint_from_hexstr(hb, &valid);
    if (!valid) {
        *charsused = (uint16_t)(bcpy - inbuf);
        return IHP_INVALID;
    }
    sum += b;
    *charsused = (uint16_t)(bcpy - inbuf);
    if (lowByte(sum) != 0) {
        return IHP_BADSUM;
    }
    *recsize = (uint8_t)len;
    return (IHP_OK);
}

static bool _valid_rec_type(int rt) {
    bool retval = true;
    switch (rt) {
        case (int)ihrt_data:
        case (int)ihrt_eof:
        case (int)ihrt_extsaddr:
        case (int)ihrt_startsaddr:
        case (int)ihrt_extladdr:
        case (int)ihrt_startladdr:
            break;
        default:
            retval = false;
            break;
    }
    return (retval);
}

/**
 * @brief Write data from the data buffer to the binfile for recsize bytes at the location specified
 *      by `daddr`. One byte after the last location written is in `lastaddr`. The `lastaddr` is updated on return.
 *
 * @return true
 * @return false
 */
static bool _write_bin(FF_FILE* binfile, uint8_t* data, uint16_t recsize, uint32_t daddr, uint32_t* lastaddr) {
    if (*lastaddr != daddr) {
        if (ff_fseek(binfile, daddr, FF_SEEK_SET) != 0) {
            return false;
        }
    }
    int bc = ff_fwrite(data, sizeof(uint8_t), recsize, binfile);
    if (bc != recsize) {
        return false;
    }
    *lastaddr = daddr + recsize;
    return true;
}

// ====================================================================
// Public Methods
// ====================================================================

bool ih_is_hexfile(FF_FILE* fp) {
    // Scan the file to see if we find at least one valid record and only
    // valid 'fill' characters, which can include '\000', 'CR', 'LF', and comments.
    size_t br = ff_fread(&_tbuf, sizeof(uint8_t), TBUFSIZE, fp);
    // Rewind the file so it's ready for the actual processing.
    ff_fseek(fp, 0, FF_SEEK_SET);
    if (br < RECSIZEMIN) {
        return false; // Not enough data
    }
    for (int i = 0; i < br; i++) {
        // If we get control chars other than '\000', '\t', '\r' or '\n'
        // or if we don't get at least valid record in the entire
        // buffer, don't consider this a valid IntelHEX file.
        uint8_t v = _tbuf[i];
        uint16_t charsused;
        uint16_t addr;
        ih_rec_type type;
        uint8_t recsize;
        if (v == RECSTART) {
            if (_getrecord((const char*)(_tbuf + i), (br - i), &charsused, _trec, &recsize, &addr, &type) == IHP_OK) {
                // Valid record. We can stop.
                return true;
            }
            else {
                // Not a valid record
                // (actually, it could be that we just haven't read enough data)
                return false;
            }
        }
        else {
            if (v > 0x7F) {
                return false;
            }
            if (v < 0x20) {
                // Check for not in the list of control-characters we accept
                if (v != '\000' && v != '\t' && v != '\n' && v != '\r') {
                    return false;
                }
            }
        }
    }
    // We didn't find a single valid record.
    return false;
}

ihp_stat_t ih_to_bin(FF_FILE* hexfile, FF_FILE* binfile, const progstat_handler_fn progstatfn) {
    uint32_t hexrecno = 0;
    uint32_t addroff = 0;
    uint32_t lastaddr = 0;
    size_t wanted;
    char* bufp = (char*)_tbuf;
    char* bpc = bufp;
    char* dataend = bufp;
    bool some_processed = false;
    ihp_stat_t status = IHP_OK;
    _cancel = false;
    do {
        // Move existing data down to the beginning and adjust the end.
        if (bpc > bufp) {
            size_t bytes = dataend - bpc;
            memcpy(bufp, bpc, bytes);
            dataend = bufp + bytes;
            bpc = bufp;
        }
        // Try to fill the buffer from the hex file.
        wanted = (bufp + TBUFSIZE) - dataend;
        size_t br = ff_fread(dataend, sizeof(uint8_t), wanted, hexfile);
        dataend += br;
        if (dataend == bufp) {
            // We don't have any more data.
            // If we've processed something return the last status.
            status = (some_processed ? status : IHP_NONE_WRITTEN);
            goto _hex_proc_done;
        }
        while (bpc < dataend) {
            uint8_t v = *bpc;
            uint16_t charsused;
            uint16_t addr;
            ih_rec_type type;
            uint8_t recsize;
            if (v == RECSTART) {
                status = _getrecord(bpc, (dataend - bpc), &charsused, _trec, &recsize, &addr, &type);
                some_processed = true;
                bpc += charsused;
                if (status == IHP_BADSUM || status == IHP_INVALID) {
                    goto _hex_proc_done;
                }
                if (status == IHP_NECHRS) {
                    // Not enough characters in the buffer. Read more...
                    goto _need_more_data;
                }
                if (status == IHP_OK) {
                    // Valid record, process it.
                    hexrecno++;
                    if (progstatfn) {
                        if (progstatfn(PDS_PROC_HEX_REC, hexrecno, 0, 0) != 0) {
                            status = IHP_CANCELLED;
                            goto _hex_proc_done;
                        };
                    }
                    switch (type) {
                        case ihrt_extsaddr:
                            // Extended Segment Address - contained in data (not in the address)
                            if (recsize != 2) {
                                goto _hex_proc_done;
                            }
                            uint32_t saddr = wordFromBytes(*_trec, *(_trec+1));
                            addroff = saddr << 4;
                            break;
                        case ihrt_extladdr:
                            // Extended Linear Address - contained in data (not in the address)
                            if (recsize != 2) {
                                goto _hex_proc_done;
                            }
                            uint32_t laddr = wordFromBytes(*_trec, *(_trec + 1));
                            addroff = laddr << 16;
                            break;
                        case ihrt_eof:
                            // End of file marker
                            status = IHP_OK;
                            goto _hex_proc_done;
                        case ihrt_data:
                            uint32_t daddr = (addroff + addr);
                            _write_bin(binfile, _trec, recsize, daddr, &lastaddr);
                            break;
                        case ihrt_startsaddr:
                        case ihrt_startladdr:
                            // We don't do anything with program start addresses.
                            break;
                        case ihrt_invalid:
                            goto _hex_proc_done;
                        default:
                            goto _hex_proc_done;
                    }
                }
            }
            else {
                if (v > 0x7F) {
                    goto _hex_proc_done;
                }
                if (v < 0x20) {
                    // Check for not in the list of control-characters we accept
                    if (v != '\000' && v != '\t' && v != '\n' && v != '\r') {
                        status = IHP_INVALID;
                        goto _hex_proc_done;
                    }
                }
                bpc++;
            }
        }
        _need_more_data:
    } while(true);
_hex_proc_done:
    if (progstatfn) {
        progstatfn(PDS_PROC_HEX_CMPT, status, hexrecno, 0);
    }
    return status;
}

// ====================================================================
// Initialization/Start-Up Methods
// ====================================================================


void ih_modinit() {
    if (_initialized) {
        board_panic("!!! ihex_modinit: Called more than once !!!");
    }
    _initialized = true;
}

