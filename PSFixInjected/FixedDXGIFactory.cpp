#include "FixedDXGIFactory.h"
#include <utility>
#include <iostream>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "windowscodecs.lib")

FixedDXGIFactory::FixedDXGIFactory(CComPtr<IDXGIFactory>&& factory)
	: m_RefCount(1)
	, m_Factory(std::move(factory))
{
}

FixedDXGIFactory::~FixedDXGIFactory()
{
}

HRESULT FixedDXGIFactory::QueryInterface(REFIID riid, void** ppvObject)
{
	if (riid == IID_IUnknown || riid == IID_IDXGIObject || riid == IID_IDXGIFactory)
	{
		*ppvObject = this;
		AddRef();
		return S_OK;
	}
	else
	{
		*ppvObject = nullptr;
		return E_NOINTERFACE;
	}
}

ULONG FixedDXGIFactory::AddRef(void)
{
	return InterlockedIncrement(&m_RefCount);
}

ULONG FixedDXGIFactory::Release(void)
{
	auto refCount = InterlockedDecrement(&m_RefCount);
	if (refCount == 0)
		delete this;
	return refCount;
}

HRESULT FixedDXGIFactory::SetPrivateData(REFGUID Name, UINT DataSize, const void* pData)
{
	return m_Factory->SetPrivateData(Name, DataSize, pData);
}

HRESULT FixedDXGIFactory::SetPrivateDataInterface(REFGUID Name, const IUnknown* pUnknown)
{
	return m_Factory->SetPrivateDataInterface(Name, pUnknown);
}

HRESULT FixedDXGIFactory::GetPrivateData(REFGUID Name, UINT* pDataSize, void* pData)
{
	return m_Factory->GetPrivateData(Name, pDataSize, pData);
}

HRESULT FixedDXGIFactory::GetParent(REFIID riid, void** ppParent)
{
	return m_Factory->GetParent(riid, ppParent);
}

HRESULT FixedDXGIFactory::EnumAdapters(UINT Adapter, IDXGIAdapter** ppAdapter)
{
	// This is where the magic happens.
	// Photoshop will crash on some setups if you have more than one graphics card.
	// We can fix this by intercepting calls to DXGIFactory::EnumAdapters and showing
	// only one graphics card to Photoshop.exe (and Sniffer.exe).

	if (Adapter > 0)
	{
		if (ppAdapter)
			ppAdapter = nullptr;
		return DXGI_ERROR_NOT_FOUND;
	}
	else
	{
		return m_Factory->EnumAdapters(Adapter, ppAdapter);
	}
}

HRESULT FixedDXGIFactory::MakeWindowAssociation(HWND WindowHandle, UINT Flags)
{
	return m_Factory->MakeWindowAssociation(WindowHandle, Flags);
}

HRESULT FixedDXGIFactory::GetWindowAssociation(HWND* pWindowHandle)
{
	return m_Factory->GetWindowAssociation(pWindowHandle);
}

HRESULT FixedDXGIFactory::CreateSwapChain(IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain)
{
	return m_Factory->CreateSwapChain(pDevice, pDesc, ppSwapChain);
}

HRESULT FixedDXGIFactory::CreateSoftwareAdapter(HMODULE Module, IDXGIAdapter** ppAdapter)
{
	return m_Factory->CreateSoftwareAdapter(Module, ppAdapter);
}
