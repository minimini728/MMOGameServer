#include "RingBuffer.h"
#include <string.h>
#include <iostream>


RingBuffer::RingBuffer() :_bufferSize(17408), _buffer(new char[_bufferSize]),_head(0), _rear(0)
{
	//_buffer = new char[_bufferSize];
}
RingBuffer::RingBuffer(int bufferSize)
{
	_bufferSize = bufferSize;
	_buffer = new char[_bufferSize];
	_head = 0;
	_rear = 0;
}
int RingBuffer::GetBufferSize()
{
	return _bufferSize;
}
int RingBuffer::GetUseSize()
{
	if (_rear >= _head)
		return _rear - _head;
	return _bufferSize - (_head - _rear);
}
int RingBuffer::GetFreeSize()
{
	return _bufferSize - GetUseSize() - 1;
}
int RingBuffer::Enqueue(const char* data, int size)
{
	int freeSize = GetFreeSize();
	if (size > freeSize)
		size = freeSize;

	int firstCopySize;
	if (size <= _bufferSize - _rear)
	{
		firstCopySize = size;
		memcpy(_buffer + _rear, data, firstCopySize);
	}
	else
	{
		firstCopySize = _bufferSize - _rear;
		memcpy(_buffer + _rear, data, firstCopySize);

		int remainSize = size - firstCopySize;
		memcpy(_buffer, data + firstCopySize, remainSize);

	}

	_rear = (_rear + size) % _bufferSize;

	return size;
}
int RingBuffer::Dequeue(char* dest, int size)
{
	int useSize = GetUseSize();
	if (size > useSize)
		size = useSize;

	int firstCopySize;
	if (size <= _bufferSize - _head)
	{
		firstCopySize = size;
		memcpy(dest, _buffer + _head, firstCopySize);
	}
	else
	{
		firstCopySize = _bufferSize - _head;
		memcpy(dest, _buffer + _head, firstCopySize);

		int remainSize = size - firstCopySize;
		memcpy(dest + firstCopySize, _buffer, remainSize);

	}

	_head = (_head + size) % _bufferSize;

	// 버퍼가 비면 0으로 초기화
	if (_head == _rear)
	{
		_head = _rear = 0;
	}

	return size;
}
int RingBuffer::Peek(char* dest, int size)
{
	int useSize = GetUseSize();
	if (size > useSize)
		size = useSize;

	int firstCopySize;
	if (size <= _bufferSize - _head)
	{
		firstCopySize = size;
		memcpy(dest, _buffer + _head, firstCopySize);
	}
	else
	{
		firstCopySize = _bufferSize - _head;
		memcpy(dest, _buffer + _head, firstCopySize);

		int remainSize = size - firstCopySize;
		memcpy(dest + firstCopySize, _buffer, remainSize);

	}

	return size;
}

void RingBuffer::MoveFront(int size)
{
	_head = (_head + size) % _bufferSize;
}

void RingBuffer::MoveRear(int size)
{
	_rear = (_rear + size) % _bufferSize;
}
void RingBuffer::ClearBuffer()
{
	_head = 0;
	_rear = 0;
}

void RingBuffer::PrintBuffer()
{
	std::cout << "[ Ring Buffer State ] _head: " << _head << ", _rear: " << _rear << std::endl;
	std::cout << "Data in Buffer: ";

	int useSize = GetUseSize();
	if (useSize == 0)
	{
		std::cout << "(empty)" << std::endl;
		return;
	}

	int index = _head;
	for (int i = 0; i < useSize; i++)
	{
		std::cout << (int)_buffer[index] << " ";
		index = (index + 1) % _bufferSize;  // 원형 구조 적용
	}

	std::cout << std::endl;
}

char* RingBuffer::GetFrontBufferPtr(void)
{
	return &_buffer[_head];
}
char* RingBuffer::GetRearBufferPtr(void)
{
	return &_buffer[_rear];
}
int RingBuffer::DirectEnqueueSize(void)
{
	if (_rear >= _head)
	{
		if (_head == 0)
			return _bufferSize - _rear - 1; // full 버퍼 구분을 위해 한 칸 비워둠
		else
			return _bufferSize - _rear;
	}
	else
	{
		return _head - _rear - 1;
	}

}
int RingBuffer::DirectDequeueSize(void)
{
	if (_head <= _rear)
		return _rear - _head;
	else
		return _bufferSize - _head;
}