#pragma once
template <typename T>
class CMemoryPool
{
private:
	struct Node
	{
		Node* nextNode;
	};
	Node* _top;
	int _capacity; // 확보된 총 노드 개수
	int _useCount; // 현재 사용중인 노드 개수
	bool _placementNew;
#ifdef MEMORY_POOL_CHECK
	inline static int _idCounter = 0;
	int _id;
#endif

	// 노드 하나의 실제 할당 크기 (가드 포함 유무 반영)
	static size_t NodeSize()
	{
#ifdef MEMORY_POOL_CHECK
		return sizeof(Node) + sizeof(T) + sizeof(int);
#else
		return sizeof(Node) + sizeof(T);
#endif
	}

	// 노드 하나 생성
	bool PreCreateOne()
	{
		void* block = malloc(NodeSize());
		if (block == nullptr)
			return false;

		Node* node = (Node*)block;
		T* data = (T*)(node + 1);

		// 처음 생성될때 한번만 호출
		if (!_placementNew)
		{
			new (data) T;
		}

		node->nextNode = _top;
		_top = node;
		_capacity++;
		return true;
	}

	// 초기 노드 생성
	void Reserve(int count)
	{
		for (int i = 0; i < count; i++)
		{
			if (!PreCreateOne())
			{
				DebugBreak();
			}
		}
	}

public:
	CMemoryPool(int initialCnt = 0, bool placementNew = false) :_top(nullptr), _capacity(0), _useCount(0), _placementNew(placementNew)
	{
#ifdef MEMORY_POOL_CHECK
		_id = _idCounter++;
#endif
		if (initialCnt > 0)
			Reserve(initialCnt);
	}

	virtual ~CMemoryPool()
	{
		// Node 메모리 해제
		while (_top != nullptr)
		{
			Node* node = _top;
			if (!_placementNew) // 소멸자 호출
			{
				T* data = (T*)(node + 1);
				data->~T();
			}
			_top = _top->nextNode;
			free(node);
		}
	}

#ifdef MEMORY_POOL_CHECK
	T* Alloc()
	{
		if (_top != nullptr)
		{
			Node* node = _top;
			_top = _top->nextNode;

			T* data = (T*)((Node*)node + 1);
			if (_placementNew)
			{
				data = new (data) T;
			}
			_useCount++;

			*(uintptr_t*)node = _id; // 앞 가드
			*(int*)((char*)data + sizeof(T)) = _id; // 뒤 가드
			printf("[Reuse] Node addr: %p, T addr: %p	/	_useCount: %d, _capacity: %d	/	front: %lu,	rear: %d\n",
				(Node*)node, (T*)((Node*)node + 1), _useCount, _capacity, *(uintptr_t*)node, *(int*)((char*)data + sizeof(T)));

			return data; // nextNode를 숨긴다
		}

		void* node = malloc(sizeof(Node) + sizeof(T) + sizeof(int));
		if (node == nullptr)
			return nullptr;

		T* data = (T*)((Node*)node + 1);
		data = new (data) T; // 생성자 호출

		*(uintptr_t*)node = _id; // 앞 가드
		*(int*)((char*)data + sizeof(T)) = _id; // 뒤 가드

		_capacity++;
		_useCount++;


		printf("[Malloc] Node addr: %p, T addr: %p	/	_useCount: %d, _capacity: %d	/	front: %lu,	rear: %d\n",
			(Node*)node, (T*)((Node*)node + 1), _useCount, _capacity, *(uintptr_t*)node, *(int*)((char*)data + sizeof(T)));

		return data; // nextNode를 숨긴다


	}
#else
	T* Alloc(void)
	{
		if (_top != nullptr)
		{
			Node* node = _top;
			_top = _top->nextNode;

			T* data = (T*)((Node*)node + 1);
			if (_placementNew)
			{
				data = new (data) T; // 생성자 호출해서 주기
			}
			_useCount++;

			//printf("[Reuse] Node addr: %p, T addr: %p	/	_useCount: %d, _capacity: %d\n",
			//	(Node*)node, (T*)((Node*)node + 1), _useCount, _capacity);

			return data; // nextNode를 숨긴다
		}

		void* node = malloc(sizeof(Node) + sizeof(T));
		if (node == nullptr)
			return nullptr;

		T* data = (T*)((Node*)node + 1);
		data = new (data) T; // 생성자 호출

		_capacity++;
		_useCount++;

		//printf("[Malloc] Node addr: %p, T addr: %p	/	_useCount: %d, _capacity: %d\n",
		//	(Node*)node, (T*)((Node*)node + 1), _useCount, _capacity);

		return data; // nextNode를 숨긴다
	}
#endif



	bool Free(T* pData)
	{
		if (_placementNew)
		{
			pData->~T(); // 소멸자 호출해주기
		}

		Node* node = (Node*)(pData)-1;

#ifdef MEMORY_POOL_CHECK
		if (*(uintptr_t*)node != _id || *(int*)((char*)pData + sizeof(T)) != _id)
		{
			printf("[Free Error] overflow front: %lu , rear: %d\n", *(uintptr_t*)node, *(int*)((char*)pData + sizeof(T)));
			return false;
		}
#endif
		node->nextNode = _top;
		_top = node;
		_useCount--;

		//printf("[PUSH] _useCount: %d, _capacity: %d\n", _useCount, _capacity);
		return true;
	}

	int	GetCapacityCount(void) { return _capacity; }
	int	GetUseCount(void) { return _useCount; }

};