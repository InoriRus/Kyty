#ifndef INCLUDE_KYTY_CORE_HASHMAP_H_
#define INCLUDE_KYTY_CORE_HASHMAP_H_

#include "Kyty/Core/Common.h"

#include <new>

namespace Kyty::Core {

class HashmapPrivate;

//#if KYTY_PLATFORM == KYTY_PLATFORM_WINDOWS
//#define HASH_CALL __stdcall
//#else
#define KYTY_HASH_CALL
//#endif

#define KYTY_HASH_DEFINE_CALC(type)                                                                                                        \
	template <>                                                                                                                            \
	uint32_t KYTY_HASH_CALL hash_calc<>(type const* key)
#define KYTY_HASH_DEFINE_EQUALS(type)                                                                                                      \
	template <>                                                                                                                            \
	bool KYTY_HASH_CALL hash_key_equals(type const* key_a, type const* key_b)
#define KYTY_HASH_CALLBACK(name, K, V, arg)                                                                                                \
	static void KYTY_HASH_CALL name(K* key, V* value, void* arg) /* NOLINT(bugprone-macro-parentheses) */

template <class T>
uint32_t KYTY_HASH_CALL hash_calc(const T* key);
template <class T>
bool KYTY_HASH_CALL hash_key_equals(const T* key_a, const T* key_b);

template <class T>
void KYTY_HASH_CALL hash_key_copy(T* key_dst, const T* key_src)
{
	new (key_dst) T(*key_src);
}

template <class T>
void KYTY_HASH_CALL hash_value_copy(T* value_dst, const T* value_src)
{
	new (value_dst) T(*value_src);
}

template <class T>
void KYTY_HASH_CALL hash_key_free(T* key)
{
	key->~T();
}

template <class T>
void KYTY_HASH_CALL hash_value_free(T* value)
{
	value->~T();
}

using hash_calc_func_t       = uint32_t (*)(const void*);
using hash_key_equals_func_t = bool (*)(const void*, const void*);
using hash_key_copy_func_t   = void (*)(void*, const void*);
using hash_value_copy_func_t = void (*)(void*, const void*);
using hash_key_free_func_t   = void (*)(void*);
using hash_value_free_func_t = void (*)(void*);
using hash_callback_func_t   = bool (*)(const void*, const void*, void*);

class HashmapBase
{
public:
	HashmapBase(uint32_t key_size, uint32_t value_size, hash_calc_func_t hash, hash_key_equals_func_t equals, hash_key_copy_func_t key_copy,
	            hash_value_copy_func_t value_copy, hash_key_free_func_t key_free, hash_value_free_func_t value_free);

	virtual ~HashmapBase();

	void Clear();

	[[nodiscard]] uint32_t Size() const;

	void        Put(const void* key, const void* value);
	const void* Get(const void* key) const;
	void        Remove(const void* key);
	void*       OperatorSquareBrackets(const void* key, const void* default_value);

	void                      Start() const;
	[[nodiscard]] bool        End() const;
	void                      Next() const;
	[[nodiscard]] const void* Value() const;
	[[nodiscard]] const void* Key() const;
	void                      ForEach(hash_callback_func_t callback, void* arg) const;
	[[nodiscard]] uint32_t    CollisionsCount() const;

	KYTY_CLASS_NO_COPY(HashmapBase);

private:
	HashmapPrivate* m_p;
};

template <class K, class V>
class Hashmap
{
public:
	Hashmap()
	    : m_b(sizeof(K), sizeof(V), // NOLINT(bugprone-sizeof-expression)
	          reinterpret_cast<hash_calc_func_t>(hash_calc<K>), reinterpret_cast<hash_key_equals_func_t>(hash_key_equals<K>),
	          reinterpret_cast<hash_key_copy_func_t>(hash_key_copy<K>), reinterpret_cast<hash_value_copy_func_t>(hash_value_copy<V>),
	          reinterpret_cast<hash_key_free_func_t>(hash_key_free<K>), reinterpret_cast<hash_value_free_func_t>(hash_value_free<V>))
	{
	}
	virtual ~Hashmap() = default;

	void Clear() { m_b.Clear(); }

	[[nodiscard]] uint32_t Size() const { return m_b.Size(); }

	V& operator[](const K& key)
	{
		V def {};
		return *(static_cast<V*>(m_b.OperatorSquareBrackets(&key, &def)));
	}

	V& GetOrPutDef(const K& key, const V& def) { return *(static_cast<V*>(m_b.OperatorSquareBrackets(&key, &def))); }

	void Put(const K& key, const V& value) { m_b.Put(&key, &value); }

	[[nodiscard]] V Get(const K& key, const V& default_value = V()) const
	{
		const V* v = static_cast<const V*>(m_b.Get(&key));
		return (v ? *v : default_value);
	}

	[[nodiscard]] const V* Find(const K& key) const { return static_cast<const V*>(m_b.Get(&key)); }

	[[nodiscard]] bool Contains(const K& key) const { return m_b.Get(&key); }

	void Remove(const K& key) { m_b.Remove(&key); }

	void Start() const { m_b.Start(); }

	[[nodiscard]] bool End() const { return m_b.End(); }

	void Next() const { m_b.Next(); }

	[[nodiscard]] const V& Value() const { return *(static_cast<const V*>(m_b.Value())); }

	[[nodiscard]] const K& Key() const { return *(static_cast<const K*>(m_b.Key())); }

	void ForEach(bool(KYTY_HASH_CALL* callback)(const K* key, const V* value, void* context), void* arg) const
	{
		m_b.ForEach(reinterpret_cast<hash_callback_func_t>(callback), arg);
	}

	uint32_t CollisionsCount() { return m_b.CollisionsCount(); }

	bool operator==(const Hashmap<K, V>& other) const
	{
		if (Size() != other.Size())
		{
			return false;
		}
		bool ok       = true;
		auto callback = [&ok, &other](const K* key, const V* value)
		{
			if (!(other.Find(*key) && other.Get(*key) == *value))
			{
				ok = false;
				return false;
			}
			return true;
		};
		ForEach([](const K* key, const V* value, void* func) { return (*static_cast<decltype(callback)*>(func))(key, value); }, &callback);
		return ok;
	}

	bool operator!=(const Hashmap<K, V>& other) const { return !(*this == other); }

	KYTY_CLASS_NO_COPY(Hashmap);

private:
	HashmapBase m_b;
};

#define FOR_HASH(h) for ((h).Start(); !(h).End(); (h).Next()) /*NOLINT(cppcoreguidelines-macro-usage)*/

} // namespace Kyty::Core

#endif /* INCLUDE_KYTY_CORE_HASHMAP_H_ */
