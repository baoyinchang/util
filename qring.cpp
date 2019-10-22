// qring.cpp
// by leoyin

#include "qring.h"
#include "common.h"
#include <stdlib.h>
#include <memory.h>
#include <sched.h>
#include <unistd.h>

QRingMgr::QRingMgr()
{
	memset(&m_qringHdr, 0, QRING_HDR_SIZE);
}
QRingMgr::~QRingMgr()
{

}

int QRingMgr::Create(uint32_t uQSize, uint32_t uItemBytes)
{
	if(!uQSize || !uItemBytes)
		return ERR_INVALID_PARAM;

	int nRet = ERR_NONE;
	memset(&m_qringHdr, 0, QRING_HDR_SIZE);

	m_qringHdr.pSlots = (uint8_t*)malloc(uQSize * uItemBytes);
	m_qringHdr.pSlotState_read = (uint8_t*)malloc(uQSize * sizeof(uint8_t));
	m_qringHdr.pSlotState_write = (uint8_t*)malloc(uQSize * sizeof(uint8_t));
	if(!m_qringHdr.pSlots || !m_qringHdr.pSlotState_read || !m_qringHdr.pSlotState_write)
	{
		nRet = ERR_MALLOC;
		goto _ERR;
	}

	memset(m_qringHdr.pSlotState_read, 0, uQSize * sizeof(uint8_t));
	memset(m_qringHdr.pSlotState_write, 0, uQSize * sizeof(uint8_t));

	m_qringHdr.uSize = uQSize;
	m_qringHdr.uSlotBytes = uItemBytes;
	
	nRet = ERR_NONE;

_ERR:
	if(ERR_NONE != nRet)
		Destory();
	
	return nRet;
}

int QRingMgr::Destory()
{
	atom_fetch_and_and( &(m_qringHdr.uSize), 0 );
	atom_fetch_and_and( &(m_qringHdr.uLen), 0 );

	if(m_qringHdr.pSlots)
		free(m_qringHdr.pSlots);

	if(m_qringHdr.pSlotState_read)
		free(m_qringHdr.pSlotState_read);

	if(m_qringHdr.pSlotState_write)
		free(m_qringHdr.pSlotState_write);

	memset(&m_qringHdr, 0, QRING_HDR_SIZE);

	return ERR_NONE;
}

// 0, 1, 2, 3, 4, 5
int QRingMgr::Push(uint8_t* pVal, bool bSched_yield)
{
	int nRet = ERR_FULLY;
	uint32_t uIdx = 0;

	if(!pVal)
		return ERR_INVALID_PARAM;

	// 只会有一个线程 compare 会成功，然后 q-length+1， 再将idx_write + 1，
	// 如果在 length和 idx_write 加一 之前有线程调用了Length() 则只会在原idx的位置空转。
	// 所以 这两个 加一 顺序很重要。
	
	// 最后 在该位置 置 state_read 为 1， 保证pop可以正确读取该值。
	// 如果在 state_read 置1 之前有pop读到该位置， idx_read并不会 减一， 所以会在该位置产生空转。 

	// length 用来lock测试该ring是否可push or pop
	// state 用来lock测试该位置是否可 read or write

	// like spin lock,
	// len < size, there is empty slot, and not long distance.
	for( ; Length() < m_qringHdr.uSize; )
	{
		// 1: if compare OK, set write state 1.
		//    ONLY one can write current slot, the others get next slot by for(). 
		if(atom_bool_compare_and_swap(&(m_qringHdr.pSlotState_write[m_qringHdr.uIndex_write % m_qringHdr.uSize]), 0, 1))
		{
			// 2: add q length
			atom_fetch_and_add( &(m_qringHdr.uLen), 1 );

			// 3: get old idx, and add idx 1.
			uIdx = atom_fetch_and_add(&(m_qringHdr.uIndex_write), 1);

			// 3: get ring idx.
			uIdx %= m_qringHdr.uSize;

			// 4: set value
			memcpy(m_qringHdr.pSlots+(uIdx * m_qringHdr.uSlotBytes), pVal, m_qringHdr.uSlotBytes);

			// 5: set readable. add 1
			atom_fetch_and_add( &(m_qringHdr.pSlotState_read[uIdx % m_qringHdr.uSize]), 1 );

			// 6: set FOUND, break
			nRet = ERR_NONE;
			break;
		}

		if(bSched_yield)
			usleep(0);
	}

	return nRet;
}

int QRingMgr::Pop(uint8_t* pVal, bool bSched_yield)
{
	int nRet = ERR_EMPTY;
	uint32_t uIdx = 0;

	if(!pVal)
		return ERR_INVALID_PARAM;

	for(; Length() > 0; )
	{
		// 1: if compare OK, set read state 0.
		//    ONLY one can read current slot, the others get next slot by for(). 
		if(atom_bool_compare_and_swap(&(m_qringHdr.pSlotState_read[m_qringHdr.uIndex_read % m_qringHdr.uSize]), 1, 0))
		{
			// 2: sub q length
			atom_fetch_and_sub( &(m_qringHdr.uLen), 1 );

			// 3: get old idx, and add idx 1.
			uIdx = atom_fetch_and_add(&(m_qringHdr.uIndex_read), 1);

			// 3: get ring idx.
			uIdx %= m_qringHdr.uSize;

			// 4: set value
			memcpy(pVal, m_qringHdr.pSlots + (uIdx * m_qringHdr.uSlotBytes), m_qringHdr.uSlotBytes);

			// 5: set writeable. sub 1.
			atom_fetch_and_sub( &(m_qringHdr.pSlotState_write[uIdx % m_qringHdr.uSize]), 1 );

			// 6: set FOUND, break
			nRet = ERR_NONE;
			break;
		}

		if(bSched_yield)
			usleep(0);

	}

	return nRet;
}

bool QRingMgr::Empty()
{
	if(Length())
		return false;

	return true;
}

uint32_t QRingMgr::Len()
{
	return Length();
}

uint32_t QRingMgr::Size()
{
	return m_qringHdr.uSize;
}

inline uint32_t QRingMgr::Length()
{
	return atom_fetch_and_add( &(m_qringHdr.uLen), 0 );
}

