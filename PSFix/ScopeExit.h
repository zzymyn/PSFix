#pragma once

#include <functional>

template <typename Func>
class ScopeExit
{
public:
	ScopeExit(Func&& func)
		: m_Func{ std::forward<Func>(func) }
	{
	}

	ScopeExit(const ScopeExit&) = delete;

	ScopeExit(ScopeExit&& o)
		: m_Func(std::move(o.m_Func))
		, m_Cancelled(o.m_Cancelled)
	{
		o.m_Cancelled = true;
	}

	ScopeExit& operator=(const ScopeExit&) = delete;

	ScopeExit& operator=(ScopeExit&&) = delete;

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
	bool m_Cancelled{ false };
};

template <typename Func>
ScopeExit<Func> OnScopeExit(Func&& func)
{
	return ScopeExit<Func>(std::forward<Func>(func));
}