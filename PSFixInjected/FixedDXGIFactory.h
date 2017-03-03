#pragma once

#include <atlbase.h>
#include <dxgi.h>

class FixedDXGIFactory
	: public IDXGIFactory
{
public:
	FixedDXGIFactory(CComPtr<IDXGIFactory>&& factory);
	~FixedDXGIFactory();

private:
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;
	virtual ULONG STDMETHODCALLTYPE AddRef(void) override;
	virtual ULONG STDMETHODCALLTYPE Release(void) override;

	virtual HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID Name, UINT DataSize, const void* pData) override;
	virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID Name, const IUnknown* pUnknown) override;
	virtual HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID Name, UINT* pDataSize, void* pData) override;
	virtual HRESULT STDMETHODCALLTYPE GetParent(REFIID riid, void** ppParent) override;

	virtual HRESULT STDMETHODCALLTYPE EnumAdapters(UINT Adapter, IDXGIAdapter** ppAdapter) override;
	virtual HRESULT STDMETHODCALLTYPE MakeWindowAssociation(HWND WindowHandle, UINT Flags) override;
	virtual HRESULT STDMETHODCALLTYPE GetWindowAssociation(HWND* pWindowHandle) override;
	virtual HRESULT STDMETHODCALLTYPE CreateSwapChain(IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain) override;
	virtual HRESULT STDMETHODCALLTYPE CreateSoftwareAdapter(HMODULE Module, IDXGIAdapter** ppAdapter) override;

	ULONG m_RefCount;
	CComPtr<IDXGIFactory> m_Factory;
};