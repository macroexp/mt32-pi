//
// test_ringbuffer.cpp
//
// mt32-pi - A baremetal MIDI synthesizer for Raspberry Pi
// Copyright (C) 2026 pim-32 contributors
//
// This file is part of mt32-pi.
//
// mt32-pi is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// mt32-pi is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// mt32-pi. If not, see <http://www.gnu.org/licenses/>.
//

// CRingBuffer: wrap-around, full and empty behaviour, and one producer and
// one consumer on different threads (as the IRQ handler and the main loop)

#include "check.h"
#include "ringbuffer.h"

#include <atomic>
#include <thread>
#include <vector>

TEST(EmptyAndFull)
{
	CRingBuffer<u8, 8> Buffer;
	u8 nByte = 0;
	CHECK(!Buffer.Dequeue(nByte));

	// One slot stays free to tell full from empty: capacity is N - 1
	for (u8 i = 0; i < 7; ++i)
		CHECK(Buffer.Enqueue(i));
	CHECK(!Buffer.Enqueue(99));

	for (u8 i = 0; i < 7; ++i)
	{
		CHECK(Buffer.Dequeue(nByte));
		CHECK_EQ(nByte, i);
	}
	CHECK(!Buffer.Dequeue(nByte));
}

TEST(BulkWrapAround)
{
	CRingBuffer<u8, 16> Buffer;
	u8 nNext = 0, nExpected = 0;
	for (unsigned nRound = 0; nRound < 100; ++nRound)
	{
		u8 In[5];
		for (u8& n : In)
			n = nNext++;
		CHECK_EQ(Buffer.Enqueue(In, 5), 5u);

		u8 Out[16];
		const size_t nOut = Buffer.Dequeue(Out, 3 + nRound % 4);
		for (size_t i = 0; i < nOut; ++i)
			CHECK_EQ(Out[i], nExpected++);

		// Drain when it gets close to full
		if (nRound % 3 == 2)
		{
			const size_t nRest = Buffer.Dequeue(Out, sizeof(Out));
			for (size_t i = 0; i < nRest; ++i)
				CHECK_EQ(Out[i], nExpected++);
		}
	}
}

TEST(PartialEnqueueWhenFull)
{
	CRingBuffer<u8, 8> Buffer;
	const u8 In[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	CHECK_EQ(Buffer.Enqueue(In, 10), 7u);
	u8 Out[10];
	CHECK_EQ(Buffer.Dequeue(Out, 10), 7u);
	CHECK_EQ(Out[6], 6);
}

TEST(ProducerConsumerThreads)
{
	CRingBuffer<u32, 64> Buffer;
	constexpr u32 nCount = 200000;
	std::atomic<bool> bOrderOK{true};

	std::thread Consumer([&]
	{
		u32 nExpected = 0;
		while (nExpected < nCount)
		{
			u32 Out[16];
			const size_t n = Buffer.Dequeue(Out, 16);
			for (size_t i = 0; i < n; ++i)
				if (Out[i] != nExpected++)
					bOrderOK = false;
		}
	});

	for (u32 i = 0; i < nCount;)
		if (Buffer.Enqueue(i))
			++i;

	Consumer.join();
	CHECK(bOrderOK);
}
