#include <WinSock2.h>
#include <WS2tcpip.h>
#include <WinInet.h>
#include <Windows.h>
#include "streams.h"
#include "renbase.h"


// {BD2B39EA-A4C3-40A9-8B8F-90729B48A6B0}
static const GUID CLSID_NetworkSource = 
{ 0xbd2b39ea, 0xa4c3, 0x40a9, { 0x8b, 0x8f, 0x90, 0x72, 0x9b, 0x48, 0xa6, 0xb0 } };

// {AB6917F0-F022-4644-8E9B-C21D07DCD739}
static const GUID IID_INetworkSource = 
{ 0xab6917f0, 0xf022, 0x4644, { 0x8e, 0x9b, 0xc2, 0x1d, 0x7, 0xdc, 0xd7, 0x39 } };

#ifdef NETWORKSINK_EXPORTS
#define DECLSPEC __declspec(dllexport)
#else
#define DECLSPEC __declspec(dllimport)
#endif


DECLARE_INTERFACE_(INetworkSource, IUnknown)
{
	STDMETHOD(Source) (THIS_ char* dwPort, THIS_ char* szSource) PURE;
	STDMETHOD(Socket) (THIS_ SOCKET sSocket) PURE;
};

class CNetPin;

class CNetSource : public CSource, public INetworkSource
{
	public:

		//COM Support
		DECLARE_IUNKNOWN
		//STDMETHODIMP CNetSource::NonDelegatingQueryInterface(REFIID riid, void** ppv)
		static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpUnk, HRESULT *pHr);


		//Constructor
		CNetSource(TCHAR* pName, LPUNKNOWN lpUnk, CLSID clsid, HRESULT *pHr); 


		//INetworkSource Methods
		DECLSPEC STDMETHODIMP Source(char* dwPort, char* szSource);
		DECLSPEC STDMETHODIMP Socket(SOCKET sSocket);

};


//Filter Pin
class CNetPin : public CSourceStream//, public INetworkSource
{
	protected:
		int m_iFrameNumber;
		REFERENCE_TIME m_rtFrameLength;

	public:
		//COM Support
		DECLARE_IUNKNOWN

		//Constructor
		CNetPin(TCHAR* pObjectName, HRESULT* phr, CNetSource* pOwner, LPCWSTR pPinName)
			:CSourceStream(pObjectName, phr, pOwner,  pPinName){ m_iFrameNumber = 0; m_rtFrameLength = 0;}

		//CSourceStream Methods to Override
		HRESULT CheckMediaType(const CMediaType *pMediaType);
		HRESULT GetMediaType(int iPosition, CMediaType *pMediaType); //Used to declare the media types the pin supports
		HRESULT FillBuffer(IMediaSample *pMediaSample); //Fill the IMediaSample Buffer with Data from Network
		HRESULT DecideBufferSize(IMemAllocator *pIMemAlloc, ALLOCATOR_PROPERTIES *pProperties); // Ask for buffers of the size 
																								//appropriate to the agreed media type
		//HRESULT Dec

};