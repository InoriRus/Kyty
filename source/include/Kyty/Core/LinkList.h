#ifndef INCLUDE_KYTY_CORE_LINKLIST_H_
#define INCLUDE_KYTY_CORE_LINKLIST_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/SafeDelete.h"

#include <type_traits>

namespace Kyty::Core {

template <typename T>
class List;

// typedef struct {void *p;} ListIndex;
using ListIndex = struct
{
	void* p;
};

#define FOR_LIST(index, list) /*NOLINT(cppcoreguidelines-macro-usage)*/                                                                    \
	for (Core::ListIndex index = (list).First(); (list).IndexValid(index);                                                                 \
	     index                 = (list).Next(index)) /* NOLINT(bugprone-macro-parentheses) */

#define FOR_LIST_R(index, list) /*NOLINT(cppcoreguidelines-macro-usage)*/                                                                  \
	for (Core::ListIndex index = (list).Last(); (list).IndexValid(index);                                                                  \
	     index                 = (list).Prev(index)) /* NOLINT(bugprone-macro-parentheses) */

template <typename T>
class ListNode final
{
	/*private:*/

	friend class List<T>;

	explicit ListNode(const T& v): m_value(v), m_next(this), m_prev(this) {}

	/*virtual*/ ~ListNode() { Remove(); }

	void Remove()
	{
		m_prev->m_next = m_next;
		m_next->m_prev = m_prev;
		m_next         = this;
		m_prev         = this;
	}

	void InsertBefore(ListNode<T>* node)
	{
		m_next         = node;
		m_prev         = node->m_prev;
		node->m_prev   = this;
		m_prev->m_next = this;
	}

	[[nodiscard]] ListNode<T>* PrevNode() const { return m_prev; }

	// NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDelete)
	[[nodiscard]] ListNode<T>* NextNode() const { return m_next; }

	T& Value() { return m_value; }

	T            m_value;
	ListNode<T>* m_next;
	ListNode<T>* m_prev;

	KYTY_CLASS_NO_COPY(ListNode);
};

template <typename T>
class List
{
	/*private:*/
	template <typename T2>
	using IsNotListType = typename std::enable_if<!std::is_same<typename std::remove_reference<T2>::type, T>::value>::type;

public:
	using Index = ListIndex;

	List() = default;

	List(const List<T>& list) { (*this) = list; }

	List(List<T>&& list) noexcept { (*this) = list; }

	virtual ~List() { Clear(); }

	[[nodiscard]] uint32_t Size() const { return m_size; }

	void Clear()
	{
		if (m_head)
		{
			while (m_head->NextNode() != m_head)
			{
				delete m_head->NextNode();
			}
			DeleteProtected(m_head);
		}
		m_head = nullptr;
		m_size = 0;
	}

	virtual Index Add(const T& v) { return add(v); }

	[[nodiscard]] bool IndexValid(Index index) const { return !(index.p == nullptr); }

	[[nodiscard]] Index First() const
	{
		Index ret = {m_head};
		return ret;
	}

	[[nodiscard]] Index Last() const
	{
		Index ret = {m_head ? m_head->PrevNode() : nullptr};
		return ret;
	}

	[[nodiscard]] Index Next(Index index) const
	{
		auto* node = static_cast<ListNode<T>*>(index.p);
		if (node)
		{
			node = node->NextNode();
			if (node == m_head)
			{
				node = nullptr;
			}
		}
		Index ret = {node};
		return ret;
	}

	[[nodiscard]] Index Prev(Index index) const
	{
		auto* node = static_cast<ListNode<T>*>(index.p);
		if (node)
		{
			node = node->PrevNode();
			if (node->NextNode() == m_head)
			{
				node = nullptr;
			}
		}
		Index ret = {node};
		return ret;
	}

	T& operator[](Index index)
	{
		EXIT_IF(!IndexValid(index));

		auto* node = static_cast<ListNode<T>*>(index.p);

		return node->Value();
	}

	const T& operator[](Index index) const
	{
		EXIT_IF(!IndexValid(index));

		auto* node = static_cast<ListNode<T>*>(index.p);

		return node->Value();
	}

	[[nodiscard]] const T& At(Index index) const
	{
		EXIT_IF(!IndexValid(index));

		auto* node = static_cast<ListNode<T>*>(index.p);

		return node->Value();
	}

	[[nodiscard]] Index Find(const T& value) const
	{
		FOR_LIST(index, (*this))
		{
			if ((*this)[index] == value)
			{
				return index;
			}
		}

		Index ret = {nullptr};
		return ret;
	}

	template <typename T2, typename OP>
	[[nodiscard]] Index Find(const T2& t, OP&& op_eq) const
	{
		FOR_LIST(index, (*this))
		{
			if (op_eq((*this)[index], t))
			{
				return index;
			}
		}

		Index ret = {nullptr};
		return ret;
	}

	template <typename T2, typename T3, typename OP>
	[[nodiscard]] Index Find(const T2& t2, const T3& t3, OP&& op_eq) const
	{
		FOR_LIST(index, (*this))
		{
			if (op_eq((*this)[index], t2, t3))
			{
				return index;
			}
		}

		Index ret = {nullptr};
		return ret;
	}

	[[nodiscard]] bool Contains(const T& value) const { return IndexValid(Find(value)); }

	Index Remove(const T& value)
	{
		Index index = Find(value);
		return Remove(index);
	}

	Index Remove(Index index)
	{
		if (!IndexValid(index))
		{
			Index ret = {nullptr};
			return ret;
		}

		auto* node = static_cast<ListNode<T>*>(index.p);
		auto* next = static_cast<ListNode<T>*>(Next(index).p);

		if (node == m_head)
		{
			if (next == nullptr)
			{
				DeleteProtected(m_head);
				m_head    = nullptr;
				m_size    = 0;
				Index ret = {nullptr};
				return ret;
			}

			m_head = next;
		}

		DeleteProtected(node);
		m_size--;
		Index ret = {next};
		return ret;
	}

	// NOLINTNEXTLINE(bugprone-unhandled-self-assignment,cert-oop54-cpp)
	List<T>& operator=(const List<T>& list)
	{
		if (this != &list)
		{
			Clear();
			FOR_LIST(index, list)
			{
				add(list[index]);
			}
		}
		return *this;
	}

	List<T>& operator=(List<T>&& list) noexcept
	{
		*this = list;
		return *this;
	}

private:
	Index add(const T& v)
	{
		auto* node = new ListNode<T>(v);

		if (m_head)
		{
			node->InsertBefore(m_head);
		} else
		{
			m_head = node;
		}

		m_size++;

		Index ret = {node};
		return ret;
	}

	ListNode<T>* m_head = {nullptr};
	uint32_t     m_size = {0};
};

template <typename T>
class ListSet: public List<T>
{
public:
	typename List<T>::Index Add(const T& v) override
	{
		typename List<T>::Index index = this->Find(v);
		if (!this->IndexValid(index))
		{
			return List<T>::Add(v);
		}
		return index;
	}
};

} // namespace Kyty::Core

#endif /* INCLUDE_KYTY_CORE_LINKLIST_H_ */
