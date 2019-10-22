// qring.h

#ifndef _QRING_H_
#define _QRING_H_

#include <stdint.h>

typedef struct _QRING_HDR
{
	// queue info, size, item bytes.
	uint32_t uSize;
	uint32_t uSlotBytes;

	// ring read or write index.
	volatile uint32_t uIndex_read;
	volatile uint32_t uIndex_write;

	// slot save value
	uint8_t* pSlots;

	// slot, read or write by test state.
	uint8_t* pSlotState_read;
	uint8_t* pSlotState_write;

	// queue, push or pop by test length
	uint32_t uLen;

} QRING_HDR, *PQRING_HDR;
#define QRING_HDR_SIZE (sizeof(QRING_HDR))

class QRingMgr
{
public:
	QRingMgr();
	~QRingMgr();

	int Create(uint32_t uQSize, uint32_t uItemBytes);
	int Destory();
	int Push(uint8_t* pVal, bool bSched_yield);
	int Pop(uint8_t* pVal, bool bSched_yield);

	bool Empty();
	uint32_t Len();
	uint32_t Size();

private:
	inline uint32_t Length();

private:
	QRING_HDR m_qringHdr;
};


#endif //_QRING_H_
