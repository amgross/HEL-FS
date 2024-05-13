
/*
 * This file is for defines that user can control.
 */

/*
 * As much as the base type is bigger, the more overhead needed in each block metadata but the memory size may be larger.
 *
 * options:
 *      16: max memory size - ((1 << 14) - 1), max sectors num - ((1 << 7) - 1)
 *      32: max memory size - ((1 << 30) - 1), max sectors num - ((1 << 15) - 1)
 *      64: max memory size - ((1 << 62) - 1), max sectors num - ((1 << 31) - 1)
 */
// #define HEL_BASE_TYPE_BITS 32
