#include <boost/thread/mutex.hpp>
#include <stack>

template <typename T> class ConcurrentStack 
{
public:
	void push(const T& item) {
		boost::mutex::scoped_lock lock(m_mutex);
		m_stack.push(item);
	}
	T pop() 
	{
		boost::mutex::scoped_lock lock(m_mutex);
		int v = m_stack.top(); 
		m_stack.pop();
		return v;
	}
	T top() const { // note that we shouldn't return a reference,
		// because another thread might pop() this
		// object in the meanwhile
		boost::mutex::scoped_lock lock(m_mutex);
		return m_stack.top();
	}

private:
	mutable boost::mutex m_mutex;
	std::stack<T> m_stack;
};