#include "netsink.h"

		//Error-Checking Code//
/*
	char buffer[10];			
	DWORD err = WSAGetLastError();
	_itoa_s(err, buffer, 10);
	MessageBoxA(NULL, "No Data Sent!", buffer, MB_OK | MB_ICONERROR);
	return E_FAIL;
*/


//-----------------Internal Variables & Function Delarations-----------------//
char* szTargetPort = "6007"; //nullptr;
char* szTargetAddress = "192.168.0.14"; //nullptr; //192.168.0.14 <--- Local IP //192.168.0.12 <--- Target IP
char* szLocalPort = nullptr; //
SOCKET sOutSocket = NULL;

bool bSocketCreated = false;
void CreateInternalSocket();

WSADATA wsaDataA;

struct addrinfo hints;
struct addrinfo* pTargetAddress = NULL;

struct addrinfo* pLocalAddress = nullptr;

long nOutSize = 0;
int nBytesSent = 0;
//int nTotalSent = 0;

BYTE* pBuffer = nullptr;


//------------------------------------------------------Create Filter Allocator--------------------------------------------------------//


STDMETHODIMP CNetSinkAlloc::SetProperties(ALLOCATOR_PROPERTIES *pRequest, ALLOCATOR_PROPERTIES *pActual)
{
	return GetProperties(pActual) ;
}


//Return the properties actually being used on this allocator
STDMETHODIMP CNetSinkAlloc::GetProperties(ALLOCATOR_PROPERTIES* pProps)
{

    CheckPointer(pProps,E_POINTER);
  
    pProps -> cBuffers  = 8; // 4;
    pProps -> cbBuffer  = 65500; //8000;
    pProps -> cbAlign   = 1 ;
    pProps -> cbPrefix  = 0 ;

    return S_OK ;
}



STDMETHODIMP CNetSinkAlloc::Commit()
{
    return S_OK ;
}

//--------------------------------------------------------Create Filter Pin----------------------------------------------------------//


STDMETHODIMP CNetInputPin::GetAllocator(IMemAllocator **ppAllocator)
{
	CheckPointer(ppAllocator, E_POINTER);
    if (m_pAllocator)  
    {
        // We already have an allocator, so return that one.
        *ppAllocator = m_pAllocator;
        (*ppAllocator)->AddRef();
        return S_OK;
    }

	
	HRESULT hr;
	CNetSinkAlloc* pSinkAlloc = new CNetSinkAlloc(L"SinkAllocator", NULL, &hr);
    if (pSinkAlloc == nullptr)
    {
        return E_OUTOFMEMORY;
    }
    if (FAILED(hr))
    {
        delete pSinkAlloc;
        return hr;
	}


    // Return the IMemAllocator interface to the caller.
    return pSinkAlloc->QueryInterface(IID_IMemAllocator, (void**)ppAllocator);
}


STDMETHODIMP CNetInputPin::NotifyAllocator(IMemAllocator *pAllocator, BOOL bReadOnly)
{ 

	ALLOCATOR_PROPERTIES pProperties;

	HRESULT hr = pAllocator -> GetProperties(&pProperties);
	if(FAILED(hr))
	{
		return E_UNEXPECTED;
	}

	if(((pProperties.cbBuffer) != 8000) && ((pProperties.cbBuffer) != 4) )
	{
		return E_FAIL;
	}
	
	
	return S_OK;
} 


STDMETHODIMP CNetInputPin::GetAllocatorRequirements(ALLOCATOR_PROPERTIES* pProperties)
{
	pProperties -> cBuffers  = 8; //4; 
    pProperties -> cbBuffer  = 65500; //8000;
    pProperties -> cbAlign   = 1 ;
    pProperties -> cbPrefix  = 0 ;

	return S_OK;
}


HRESULT CNetInputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	if(pMediaType == NULL)
		return E_POINTER;

	if(iPosition < 0)
		return E_INVALIDARG;

	if(iPosition > 2)
		return VFW_S_NO_MORE_ITEMS;


	switch(iPosition)
	{
		
		case 0: //Video
		{
			//pMediaType->SetFormatType(&MEDIATYPE_Video);

		}
		break;

		case 1: //Stream
		{
			//pMediaType->SetFormatType(&MEDIATYPE_Audio);

		}
		break;
		
		case 2: //Audio
		{
			
			WAVEFORMATEX* pWaveFormat = (WAVEFORMATEX*)pMediaType -> AllocFormatBuffer(sizeof(WAVEFORMATEX));
			if(pWaveFormat == nullptr)
			{
				return E_OUTOFMEMORY;
			}

			ZeroMemory(pWaveFormat, sizeof(WAVEFORMATEX));
			

			pMediaType -> SetType(&MEDIATYPE_Audio);
			pMediaType -> subtype = MEDIASUBTYPE_PCM;
			//pMediaType -> lSampleSize = 4;
			pMediaType -> SetFormatType(&FORMAT_WaveFormatEx);

			pWaveFormat -> wFormatTag = WAVE_FORMAT_PCM;
			pWaveFormat -> nChannels = 2;
			pWaveFormat -> nSamplesPerSec = 44100; 
			pWaveFormat -> nAvgBytesPerSec = 176400;
			pWaveFormat -> nBlockAlign = 4;
			pWaveFormat -> wBitsPerSample = 16;
			pWaveFormat -> cbSize = 0;
			
		}
		break;

	}

	return S_OK;
}



HRESULT CNetInputPin::CheckMediaType(const CMediaType* pMediaType)
{

	CheckPointer(pMediaType, E_POINTER);
	
	if(*(pMediaType -> Type()) != MEDIATYPE_Video)
	{
		if(*(pMediaType -> Type()) != MEDIATYPE_Audio)
		{		
			if(*(pMediaType -> Type()) != MEDIATYPE_Stream)
			{
				return E_ABORT; 
			}
		}
	}

	//Check for SubTypes we Support
	const GUID *SubType = pMediaType->Subtype();
	if (SubType == nullptr)
	{
		return E_POINTER;
	}
	else if((*SubType != MEDIASUBTYPE_PCM))
    {
        return E_ABORT;
    }

	
	//Check WAVEFORMATEX Properties
	WAVEFORMATEX* pWaveFormat = (WAVEFORMATEX*) pMediaType -> Format();
	if(pWaveFormat == nullptr)
	{
		return E_UNEXPECTED;
	}

	pWaveFormat -> nChannels = 2;
	pWaveFormat -> nSamplesPerSec = 44100;
	pWaveFormat -> nAvgBytesPerSec = 176400;
	pWaveFormat -> wBitsPerSample = 16;
	pWaveFormat -> nBlockAlign = 4;


	return S_OK;
}


//--------------------------------------------------------Create Filter Body----------------------------------------------------------//


CNetSink::CNetSink(REFCLSID RenderClass, LPCTSTR pName, LPUNKNOWN lpUnk, HRESULT* pHr) : CBaseRenderer(RenderClass, pName, lpUnk, pHr)
{
	if(WSAStartup(MAKEWORD(2,2), &wsaDataA))
	{
		*pHr = E_UNEXPECTED;
	}
	
	CNetInputPin* pNewInputPin = new CNetInputPin(this, pHr, L"Stream");
	if(pNewInputPin == nullptr)
	{
		*pHr = E_OUTOFMEMORY;
	}

	CNetSink::m_pInputPin = pNewInputPin;
	
}


//COM Support

CUnknown* WINAPI CNetSink::CreateInstance(LPUNKNOWN lpUnk, HRESULT *pHr)
{

	CNetSink* pNewNetSink = new CNetSink(IID_INetworkSink, L"NetworkSink", lpUnk, pHr);
	if(pNewNetSink == NULL)
	{
		*pHr = E_OUTOFMEMORY;
	}

	return pNewNetSink;
}



/*
STDMETHODIMP CNetSink::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv,E_POINTER);

	if (riid == IID_INetworkSink) 
    {
        return GetInterface((INetworkSink*) this, ppv);
    } 
	else
	{
		return E_NOINTERFACE;
	}
	

	return CUnknown::NonDelegatingQueryInterface(riid, ppv);
}
*/


HRESULT CNetSink::CheckMediaType(const CMediaType* pMediaType)
{

	if(*(pMediaType -> Type()) == MEDIATYPE_Video)
	{
		return S_OK; 
	}
	else if(*(pMediaType -> Type()) == MEDIATYPE_Audio)
	{
		return S_OK; 
	}
	else if(*(pMediaType -> Type()) == MEDIATYPE_Stream)
	{
		return S_OK; 
	}
	else if(pMediaType == NULL)
	{
		return E_POINTER;
	}
	else
	{
		return E_INVALIDARG;
	}
	
	return S_OK;
	
}


//If no Time Stamp is Specified, the Samples Recieved are Rendered Immediately (Which is Exactly What we Want Here)
HRESULT CNetSink::DoRenderSample(IMediaSample* pMediaSample)
{
	
	if(!bSocketCreated)
	{
		CreateInternalSocket();
		pMediaSample -> GetPointer(&pBuffer); 
		nOutSize = pMediaSample -> GetSize();
	}

	/*
	char buffer[10];			
	_itoa_s(nOutSize, buffer, 10);
	MessageBoxA(NULL, "Buffer Size", buffer, MB_OK | MB_ICONINFORMATION);
	*/

	//65,507 bytes is the maximum sample size that can be sent from the socket while actual buffer size is 88,200 bytes.
	nBytesSent = sendto(sOutSocket, (const char*)pBuffer, nOutSize, NULL, pTargetAddress -> ai_addr, pTargetAddress -> ai_addrlen);
	if(nBytesSent == SOCKET_ERROR)
	{
		
		char buffer[10];
		int err = WSAGetLastError();
		_itoa_s(err, buffer, 10);
		MessageBoxA(NULL, "No Data Sent!", buffer , MB_OK | MB_ICONERROR); 
		
	}
	
	return S_OK;
	
}




//Exported Functions

DECLSPEC STDMETHODIMP CNetSink::Target(char* szAddress, char* szPort)
{
	/*
	//Ensure Target() and Socket() are not used together
	if(sOutSocket != NULL) //Meaning Socket() was Previously Called
	{
		return E_FAIL;
	}

	szTargetAddress = szAddress;
	szTargetPort = szPort;

	CreateInternalSocket();
	*/

	return S_OK;
}


DECLSPEC STDMETHODIMP CNetSink::Socket(SOCKET sSocket, char* szAddress, char* szPort)
{	
	/*
	szTargetAddress = szAddress;
	szTargetPort = szPort;
	sOutSocket = sSocket; //Custom Socket will be used for streaming

	//The 'pTargetAddress' here will be used in the sendto() function
	if(getaddrinfo(szTargetAddress, szTargetPort, NULL, &pTargetAddress))
	{
		MessageBoxA(NULL, "Address Conversion Failed", "Error", MB_OK | MB_ICONERROR);
		return E_UNEXPECTED;
	}
	*/

	return S_OK;
}







//----------------------------------------------------------Internal Socket-------------------------------------------------------------//

void CreateInternalSocket()
{
	bSocketCreated = true;
	
	/*
	hints.ai_family = AF_INET;
	hints.ai_protocol = IPPROTO_IPV4;
	hints.ai_socktype = SOCK_DGRAM;
	*/

	if(getaddrinfo(szTargetAddress, szTargetPort, NULL, &pTargetAddress)) // &hints
	{
		MessageBoxA(NULL, "Address Conversion Failed", "Error", MB_OK | MB_ICONERROR);
		return;
	}

	sOutSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sOutSocket == NULL)
	{
		MessageBoxA(NULL, "Invalid Socket!", "Error", MB_OK | MB_ICONERROR);
		return;
	}


	//--------------------------------------------------------------------------------------------------------------------//
	/*
	struct addrinfo hints = {0};

	hints.ai_family = AF_INET;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_IPV4;

	if(getaddrinfo(NULL, szLocalPort, &hints, &pLocalAddress))
	{
		MessageBoxA(NULL, "Failed to Resolve Local IP Address and Port", "Error", MB_OK | MB_ICONERROR);
		return;
	}

	if(bind(sOutSocket, pLocalAddress ->ai_addr, sizeof(pLocalAddress->ai_addrlen)))
	{
		MessageBoxA(NULL, "Failed to Bind Socket", "Error", MB_OK | MB_ICONERROR);
		return;
	}
	*/

	//--------------------------------------------------------------------------------------------------------------------//


}