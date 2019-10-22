// common.h

#ifndef _COMMON_H_
#define _COMMON_H_

#define ERR_NONE                0
#define ERR_FAILED             -1
#define ERR_INVALID_PARAM      -2
#define ERR_MALLOC             -3
#define ERR_SIZE               -4
#define ERR_EMPTY              -5
#define ERR_FULLY              -6

// return true if the comparison is successful and newval was written
#define atom_bool_compare_and_swap(ptr, oldval, newval) \
    __sync_bool_compare_and_swap((ptr), (oldval), (newval))

// return old value
#define atom_fetch_compare_and_swap(ptr, oldval, newval) \
	__sync_val_compare_and_swap( (ptr), (oldval), (newval) )

#define atom_fetch_and_add(addr, value) \
    __sync_fetch_and_add((addr), (value))

#define atom_fetch_and_sub(addr, value) \
    __sync_fetch_and_sub((addr), (value))

#define atom_fetch_and_and(addr, value) \
    __sync_fetch_and_and((addr), (value))

#define atom_fetch_and_nand(addr, value) \
    __sync_fetch_and_nand((addr), (value))

#define atom_fetch_and_xor(addr, value) \
    __sync_fetch_and_xor((addr), (value))

#define atom_fetch_and_or(addr, value) \
    __sync_fetch_and_or((addr), (value))

// return new value
#define atom_add_and_fetch(addr, value) \
    __sync_add_and_fetch((addr), (value))

#define atom_sub_and_fetch(addr, value) \
    __sync_sub_and_fetch((addr), (value))

#define atom_and_and_fetch(addr, value) \
	__sync_and_and_fetch((addr), (value))

#define atom_nand_and_fetch(addr, value) \
	__sync_nand_and_fetch((addr), (value))

#define atom_xor_and_fetch(addr, value) \
	__sync_xor_and_fetch((addr), (value))

#define atom_or_and_fetch(addr, value) \
	__sync_or_and_fetch((addr), (value))



#endif // _COMMON_H_
