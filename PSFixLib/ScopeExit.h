#pragma once

#include <functional>

template <typename Func>
class ScopeExit
{
public:
	ScopeExit(Func&& func)
		: m_Func(std::forward<Func>(func))
		, m_Cancelled(false)
	{
	}

	~ScopeExit()
	{
		if (!m_Cancelled)
			m_Func();
	}

	void Cancel()
	{
		m_Cancelled = true;
	}

private:
	Func m_Func;
	bool m_Cancelled;
};

template <typename Func>
ScopeExit<Func> OnScopeExit(Func&& func)
{
	return ScopeExit<Func>(std::forward<Func>(func));
}