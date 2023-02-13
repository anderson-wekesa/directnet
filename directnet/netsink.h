#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <WinInet.h>
#include <Windows.h>
#include "streams.h"
#include "renbase.h"

// {3584CC36-856B-47E2-98B7-0E1FCEF32C14}
static const GUID CLSID_NetworkSink = 
{ 0x3584cc36, 0x856b, 0x47e2, { 0x98, 0xb7, 0xe, 0x1f, 0xce, 0xf3, 0x2c, 0x14 } };

// {70CE1BE4-6A8A-4F45-ACD7-F78E83210488}
static const GUID IID_INetworkSink = 
{ 0x70ce1be4, 0x6a8a, 0x4f45, { 0xac, 0xd7, 0xf7, 0x8e, 0x83, 0x21, 0x4, 0x88 } };


#ifdef NETWORKSINK_EXPORTS
#define DECLSPEC __declspec(dllexport)
#else
#define DECLSPEC __declspec(dllimport)
#endif

DECLARE_INTERFACE_(INetworkSink, IUnknown)
{
	STDMETHOD (Target)(THIS_ char* szAddress, THIS_ char* szPort) PURE;
	STDMETHOD (Socket)(THIS_ SOCKET sSocket, THIS_ char* szAddress, THIS_ char* szPort) PURE;
};

class CNetInputPin;


class CNetSink : public CBaseRenderer, public INetworkSink
{
	public:
		//Custom Allocator
		//CMemAllocator cNetSinkAllocator; //(TCHAR* pName, LPUNKNOWN pUnk, HRESULT* phr);

		//COM Support
		DECLARE_IUNKNOWN
		//STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);
		static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpUnk, HRESULT *pHr);

		//Constructor
		CNetSink(REFCLSID RenderClass, LPCTSTR pName, LPUNKNOWN lpUnk, HRESULT* pHr); 

		//Destructor
		~CNetSink()
		{
			WSACleanup();
		}

		
		//CBaseRenderer Methods to Override
		HRESULT CheckMediaType(const CMediaType* pMediaType);
		HRESULT DoRenderSample(IMediaSample* pMediaSample);

		//INetworkSink Methods
		DECLSPEC STDMETHODIMP Target(char* szAddress, char* szPort);
		DECLSPEC STDMETHODIMP Socket(SOCKET sSocket, char* szAddress, char* szPort);
		

};




//NetSink Pin

//We have to override the base class input pin because we provide our own special allocator with a different buffer size from the default
class CNetInputPin : public CRendererInputPin
{
	public:

		// Constructor
		CNetInputPin(CBaseRenderer* pRenderer, HRESULT* pHr, LPCWSTR lpName) :
			CRendererInputPin(pRenderer, pHr, lpName) {}

		// Manage our custom allocator
		STDMETHODIMP GetAllocator(IMemAllocator **ppAllocator);
		STDMETHODIMP NotifyAllocator(IMemAllocator *pAllocator,BOOL bReadOnly);
		STDMETHODIMP GetAllocatorRequirements(ALLOCATOR_PROPERTIES* pProperties);
		HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
		HRESULT CheckMediaType(const CMediaType* pMediaType);
}; 




//Custom Allocator for the NetSink Pin
class CNetSinkAlloc : public CMemAllocator
{
	protected:
		//friend class CNetInputPin;

	public:

		//Constructor
		CNetSinkAlloc(TCHAR* pName, LPUNKNOWN pUnk, HRESULT* pHr) :
			CMemAllocator(pName, pUnk, pHr) {}

		//CMemAllocator Methods to Override
		STDMETHODIMP SetProperties(ALLOCATOR_PROPERTIES *pRequest, ALLOCATOR_PROPERTIES *pActual);
		STDMETHODIMP GetProperties(ALLOCATOR_PROPERTIES *  pProps);
		STDMETHODIMP Commit();
};