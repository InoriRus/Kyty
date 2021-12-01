#include "Kyty/Core/Hashmap.h"

#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/Hash.h"
#include "Kyty/Core/SafeDelete.h"

#include <cstring>

namespace Kyty::Core {

constexpr uint32_t HASH_MAX_KEY_SIZE   = 32;
constexpr uint32_t HASH_MAX_VALUE_SIZE = 16;

#define KYTY_HASH_DEFINE_INT(type)                                                                                                         \
	KYTY_HASH_DEFINE_CALC(type)                                                                                                            \
	{                                                                                                                                      \
		switch (sizeof(type))                                                                                                              \
		{                                                                                                                                  \
			case 8: return hash64(uint64_t(*key));                                                                                         \
			case 4: return hash32(uint32_t(*key));                                                                                         \
			case 2: return hash16(uint16_t(*key));                                                                                         \
			case 1: return hash8(uint8_t(*key));                                                                                           \
		}                                                                                                                                  \
		return hash((void*)key, sizeof(type));                                                                                             \
	}                                                                                                                                      \
	KYTY_HASH_DEFINE_EQUALS(type) { return (*key_a) == (*key_b); }

#define KYTY_HASH_DEFINE_PTR(type)                                                                                                         \
	KYTY_HASH_DEFINE_CALC(type)                                                                                                            \
	{                                                                                                                                      \
		switch (sizeof(type))                                                                                                              \
		{                                                                                                                                  \
			case 8: return hash64(uint64_t(*key));                                                                                         \
			case 4: return hash32(uint32_t(uintptr_t(*key)));                                                                              \
		}                                                                                                                                  \
		return hash((void*)key, sizeof(type));                                                                                             \
	}                                                                                                                                      \
	KYTY_HASH_DEFINE_EQUALS(type) { return (*key_a) == (*key_b); }

KYTY_HASH_DEFINE_INT(int8_t);
KYTY_HASH_DEFINE_INT(uint8_t);
KYTY_HASH_DEFINE_INT(int16_t);
KYTY_HASH_DEFINE_INT(uint16_t);
KYTY_HASH_DEFINE_INT(int32_t);
KYTY_HASH_DEFINE_INT(uint32_t);
KYTY_HASH_DEFINE_INT(int64_t);
KYTY_HASH_DEFINE_INT(uint64_t);
KYTY_HASH_DEFINE_INT(char16_t);
KYTY_HASH_DEFINE_INT(char32_t);
KYTY_HASH_DEFINE_PTR(int8_t*);
KYTY_HASH_DEFINE_PTR(uint8_t*);
KYTY_HASH_DEFINE_PTR(int16_t*);
KYTY_HASH_DEFINE_PTR(uint16_t*);
KYTY_HASH_DEFINE_PTR(int32_t*);
KYTY_HASH_DEFINE_PTR(uint32_t*);
KYTY_HASH_DEFINE_PTR(int64_t*);
KYTY_HASH_DEFINE_PTR(uint64_t*);
KYTY_HASH_DEFINE_PTR(char16_t*);
KYTY_HASH_DEFINE_PTR(char32_t*);
KYTY_HASH_DEFINE_PTR(void*);

class HashmapPrivate
{
public:
	struct Entry
	{
		uint8_t  key[HASH_MAX_KEY_SIZE];
		uint32_t hash;
		uint8_t  value[HASH_MAX_VALUE_SIZE];
		Entry*   next;
	};

	KYTY_CLASS_NO_COPY(HashmapPrivate);

	HashmapPrivate(uint32_t initial_capacity, uint32_t key_size, uint32_t value_size, hash_calc_func_t hash, hash_key_equals_func_t equals,
	               hash_key_copy_func_t key_copy, hash_value_copy_func_t value_copy, hash_key_free_func_t key_free,
	               hash_value_free_func_t value_free)
	    : bucket_count_initial(initial_capacity), m_bucket_count(bucket_count_initial), size_max((m_bucket_count * 3) / 4),
	      m_key_size(key_size), m_value_size(value_size), hash_func(hash), equals_func(equals), key_copy_func(key_copy),
	      value_copy_func(value_copy), key_free_func(key_free), value_free_func(value_free)
	{
		EXIT_IF(hash == nullptr);
		EXIT_IF(equals == nullptr);
		EXIT_IF(key_copy == nullptr);
		EXIT_IF(value_copy == nullptr);
		EXIT_IF(key_free == nullptr);
		EXIT_IF(value_free == nullptr);
		EXIT_IF(initial_capacity & (initial_capacity - 1));
		EXIT_IF(key_size > HASH_MAX_KEY_SIZE);
		EXIT_IF(value_size > HASH_MAX_VALUE_SIZE);
	}

	virtual ~HashmapPrivate() { Clear(); }

	void Clear()
	{
		if (size > 0)
		{
			for (uint32_t i = 0; i < m_bucket_count; i++)
			{
				Entry* entry = buckets[i];
				while (entry != nullptr)
				{
					Entry* next = entry->next;
					value_free_func(entry->value);
					key_free_func(entry->key);
					Delete(entry);
					entry = next;
				}
			}
			DeleteArray(buckets);
			m_bucket_count = bucket_count_initial;
			size_max       = (m_bucket_count * 3) / 4;
			buckets        = nullptr;
			size           = 0;
		}
	}

	inline uint32_t HashKey(const void* key) const
	{
		auto h = hash_func(key);
		return h;
	}

	inline bool EqualKeys(const void* key_a, uint32_t hash_a, const void* key_b, uint32_t hash_b) const
	{
		if (key_a == key_b)
		{
			return true;
		}

		if (hash_a != hash_b)
		{
			return false;
		}

		return equals_func(key_a, key_b);
	}

	static inline uint32_t CalcIndex(uint32_t bucket_count, uint32_t hash) { return hash & (bucket_count - 1); }

	uint32_t Size() const { return size; }

	Entry* CreateEntry(const void* key, uint32_t hash, const void* value) const
	{
		auto* entry = new Entry;
		key_copy_func(entry->key, key);
		entry->hash = hash;
		value_copy_func(entry->value, value);
		entry->next = nullptr;
		return entry;
	}

	void ExpandIfNecessary()
	{
		if (size > size_max)
		{
			uint32_t new_bucket_count = m_bucket_count << 1u;
			auto**   new_buckets      = new Entry*[new_bucket_count];
			// NOLINTNEXTLINE(bugprone-sizeof-expression)
			std::memset(new_buckets, 0, sizeof(Entry*) * new_bucket_count);

			// Move over existing entries.
			for (uint32_t i = 0; i < m_bucket_count; i++)
			{
				Entry* entry = buckets[i];

				while (entry != nullptr)
				{
					Entry* next        = entry->next;
					size_t index       = CalcIndex(new_bucket_count, entry->hash);
					entry->next        = new_buckets[index];
					new_buckets[index] = entry;
					entry              = next;
				}
			}

			// Copy over internals.
			DeleteArray(buckets);
			buckets        = new_buckets;
			m_bucket_count = new_bucket_count;
			size_max       = (m_bucket_count * 3) / 4;
		}
	}

	void Put(const void* key, const void* value)
	{
		uint32_t hash  = HashKey(key);
		uint32_t index = CalcIndex(m_bucket_count, hash);

		if (buckets == nullptr)
		{
			buckets = new Entry*[m_bucket_count];
			// NOLINTNEXTLINE(bugprone-sizeof-expression)
			std::memset(buckets, 0, sizeof(Entry*) * m_bucket_count);
		}

		Entry** p = &(buckets[index]);

		for (;;)
		{
			Entry* current = *p;

			// Add a new entry.
			if (current == nullptr)
			{
				*p = CreateEntry(key, hash, value);
				size++;
				ExpandIfNecessary();
				return;
			}

			// Replace existing entry.
			if (EqualKeys(current->key, current->hash, key, hash))
			{
				value_free_func(current->value);
				value_copy_func(current->value, value);
				return;
			}

			// Move to next entry.
			p = &current->next;
		}
	}

	const void* Get(const void* key) const
	{
		if (size == 0)
		{
			return nullptr;
		}

		uint32_t hash  = HashKey(key);
		uint32_t index = CalcIndex(m_bucket_count, hash);

		Entry* entry = buckets[index];

		while (entry != nullptr)
		{
			if (EqualKeys(entry->key, entry->hash, key, hash))
			{
				return entry->value;
			}

			entry = entry->next;
		}

		return nullptr;
	}

	void* OperatorSqBr(const void* key, const void* default_value)
	{
		uint32_t hash  = HashKey(key);
		uint32_t index = CalcIndex(m_bucket_count, hash);

		if (buckets == nullptr)
		{
			buckets = new Entry*[m_bucket_count];
			// NOLINTNEXTLINE(bugprone-sizeof-expression)
			std::memset(buckets, 0, sizeof(Entry*) * m_bucket_count);
		}

		Entry** p = &(buckets[index]);

		for (;;)
		{
			Entry* current = *p;

			// Add a new entry.
			if (current == nullptr)
			{
				current = *p = CreateEntry(key, hash, default_value);
				size++;
				ExpandIfNecessary();
				return current->value;
			}

			// Replace existing entry.
			if (EqualKeys(current->key, current->hash, key, hash))
			{
				return current->value;
			}

			// Move to next entry.
			p = &current->next;
		}

		return nullptr;
	}

	void Remove(const void* key)
	{
		if (size == 0)
		{
			return;
		}

		uint32_t hash  = HashKey(key);
		uint32_t index = CalcIndex(m_bucket_count, hash);

		// Pointer to the current entry.
		Entry** p       = &(buckets[index]);
		Entry*  current = nullptr;
		while ((current = *p) != nullptr)
		{
			if (EqualKeys(current->key, current->hash, key, hash))
			{
				key_free_func(current->key);
				value_free_func(current->value);
				*p = current->next;
				Delete(current);
				size--;
				if (size == 0)
				{
					DeleteArray(buckets);
					m_bucket_count = bucket_count_initial;
					size_max       = (m_bucket_count * 3) / 4;
					buckets        = nullptr;
				}
				return;
			}

			p = &current->next;
		}
	}

	void Start(uint32_t start_from) const
	{
		loop_entry = nullptr;

		if (size == 0)
		{
			return;
		}

		for (loop_index = start_from; loop_index < m_bucket_count; loop_index++)
		{
			loop_entry = buckets[loop_index];

			if (loop_entry != nullptr)
			{
				break;
			}
		}
	}

	bool End() const { return loop_entry == nullptr || loop_index >= m_bucket_count; }

	void Next() const
	{
		EXIT_IF(loop_entry == nullptr);

		loop_entry = loop_entry->next;

		if (loop_entry == nullptr)
		{
			Start(loop_index + 1);
		}
	}

	const void* Value() const
	{
		EXIT_IF(loop_entry == nullptr);

		return loop_entry->value;
	}

	const void* Key() const
	{
		EXIT_IF(loop_entry == nullptr);

		return loop_entry->key;
	}

	void ForEach(hash_callback_func_t callback, void* arg) const
	{
		for (Start(0); !End(); Next())
		{
			if (!callback(Key(), Value(), arg))
			{
				break;
			}
		}
	}

	uint32_t CollisionsCount() const
	{
		if (size == 0)
		{
			return 0;
		}

		uint32_t collisions = 0;
		for (uint32_t i = 0; i < m_bucket_count; i++)
		{
			Entry* entry = buckets[i];
			while (entry != nullptr)
			{
				if (entry->next != nullptr)
				{
					collisions++;
				}
				entry = entry->next;
			}
		}
		return collisions;
	}

	mutable uint32_t       loop_index = 0;
	mutable Entry*         loop_entry = nullptr;
	Entry**                buckets    = nullptr;
	uint32_t               bucket_count_initial;
	uint32_t               m_bucket_count;
	uint32_t               size = 0;
	uint32_t               size_max;
	uint32_t               m_key_size;
	uint32_t               m_value_size;
	hash_calc_func_t       hash_func;
	hash_key_equals_func_t equals_func;
	hash_key_copy_func_t   key_copy_func;
	hash_value_copy_func_t value_copy_func;
	hash_key_free_func_t   key_free_func;
	hash_value_free_func_t value_free_func;
};

HashmapBase::HashmapBase(uint32_t key_size, uint32_t value_size, hash_calc_func_t hash, hash_key_equals_func_t equals,
                         hash_key_copy_func_t key_copy, hash_value_copy_func_t value_copy, hash_key_free_func_t key_free,
                         hash_value_free_func_t value_free)
    : m_p(new HashmapPrivate(8, key_size, value_size, hash, equals, key_copy, value_copy, key_free, value_free))
{
}

HashmapBase::~HashmapBase()
{
	Delete(m_p);
}

uint32_t HashmapBase::Size() const
{
	return m_p->Size();
}

void HashmapBase::Put(const void* key, const void* value)
{
	m_p->Put(key, value);
}

const void* HashmapBase::Get(const void* key) const
{
	return m_p->Get(key);
}

void HashmapBase::Remove(const void* key)
{
	m_p->Remove(key);
}

void HashmapBase::Start() const
{
	m_p->Start(0);
}

bool HashmapBase::End() const
{
	return m_p->End();
}

void HashmapBase::Next() const
{
	m_p->Next();
}

const void* HashmapBase::Value() const
{
	return m_p->Value();
}

const void* HashmapBase::Key() const
{
	return m_p->Key();
}

void HashmapBase::ForEach(hash_callback_func_t callback, void* arg) const
{
	m_p->ForEach(callback, arg);
}

void* HashmapBase::OperatorSquareBrackets(const void* key, const void* default_value)
{
	void* r = m_p->OperatorSqBr(key, default_value);

	EXIT_IF(r == nullptr);

	return r;
}

void HashmapBase::Clear()
{
	m_p->Clear();
}

uint32_t HashmapBase::CollisionsCount() const
{
	return m_p->CollisionsCount();
}

} // namespace Kyty::Core
