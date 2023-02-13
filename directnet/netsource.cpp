#include "netsource.h"

WSADATA wsaData;

//struct addrinfo* pLocalDetails = nullptr;
SOCKADDR_IN localdetails;
SOCKET sInSocket;

void CreateIncomingSocket();
bool bInSocketCreated = false;

BYTE* pIncomingBuffer = nullptr;
long nSize;

//--------------------------------------------------------Create Filter Body----------------------------------------------------------//

CNetSource::CNetSource(TCHAR* pName, LPUNKNOWN lpUnk, CLSID clsid, HRESULT *pHr) : CSource(pName, lpUnk, clsid, pHr)
{

	if(WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		MessageBoxA(NULL, "Failed to Initialize WinSock!", "Error", MB_OK | MB_ICONERROR);
		return;
	}

	
	CNetPin* pNewNetPin = new CNetPin(pName, pHr, this, L"Stream");
	if(pNewNetPin == NULL)
	{
		*pHr = E_OUTOFMEMORY;
	}
	

}

CUnknown* WINAPI CNetSource::CreateInstance(LPUNKNOWN lpUnk, HRESULT* pHr)
{
	CNetSource* pNewNetSource = new CNetSource(L"NetworkSource", lpUnk, CLSID_NetworkSource, pHr);
	if(pNewNetSource == NULL)
	{
		*pHr = E_OUTOFMEMORY;
	}

	return pNewNetSource;
}

/*
STDMETHODIMP CNetSource::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{

	if (riid == IID_INetworkSource) 
    {
        return GetInterface((INetworkSource *) this, ppv);
    } 
	

	return CUnknown::NonDelegatingQueryInterface(riid, ppv);
}
*/


//--------------------------------------------------------Create Filter Pin----------------------------------------------------------//


//Checks if the Downstream Filter Supports the Specified Media Types. Pin Connection will Fail if the Media Types are not Supported.
HRESULT CNetPin::CheckMediaType(const CMediaType *pMediaType)
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
    if (SubType == NULL)
	{
		return E_INVALIDARG;
	}

	if((*SubType != MEDIASUBTYPE_PCM))
    {
        return E_INVALIDARG;
    }
	
	//Check WAVEFORMATEX Properties
	WAVEFORMATEX* pWaveFormat = (WAVEFORMATEX*) pMediaType -> Format();
	if(pWaveFormat == nullptr)
	{
		return E_UNEXPECTED;
	}
	
	pWaveFormat -> nChannels = 2;
	pWaveFormat -> nSamplesPerSec = 44100;
	pWaveFormat -> wBitsPerSample = 16;
	pWaveFormat -> nBlockAlign = 4;

	pWaveFormat -> nAvgBytesPerSec = 376400;
	
	return S_OK;
}


//Allocate Memory for the Media Samples
HRESULT CNetPin::DecideBufferSize(IMemAllocator* pIMemAlloc, ALLOCATOR_PROPERTIES* pProperties)
{
	pProperties -> cBuffers = 8; //4;
	pProperties -> cbBuffer = 65500; //8000;
	pProperties -> cbPrefix = 0;
	pProperties -> cbAlign = 1;

	// Ask the allocator to reserve us some sample memory, NOTE the function
    // can succeed (that is return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted
	ALLOCATOR_PROPERTIES Actual;
    HRESULT hr = pIMemAlloc->SetProperties(pProperties, &Actual); //Specifies the number of buffers to allocate and the size of
																  //each buffer.
    if(FAILED(hr))
    {
		return E_OUTOFMEMORY;
    }

	hr = pIMemAlloc -> Commit(); 

	if(FAILED(hr))
    {
		return E_FAIL;
    }

    // Is this allocator unsuitable?
    if(Actual.cbBuffer < pProperties->cbBuffer)
    {
		MessageBoxA(NULL, "Buffer Error!", "Error", MB_OK | MB_ICONERROR);
        return E_FAIL;
    }

	return S_OK;
}


HRESULT CNetPin::GetMediaType(int iPosition, CMediaType *pMediaType)
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
			if(pWaveFormat == NULL)
			{
				return E_OUTOFMEMORY;
			}

			ZeroMemory(pWaveFormat, sizeof(WAVEFORMATEX));
			

			pMediaType -> SetType(&MEDIATYPE_Audio);
			pMediaType -> subtype = MEDIASUBTYPE_PCM;
			pMediaType -> lSampleSize = 4;
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




//This is the Core of the Filter. It Fills the IMediaSample Buffer with Data from Network.
HRESULT CNetPin::FillBuffer(IMediaSample *pMediaSample)
{
	
	if(!bInSocketCreated)
	{
		CreateIncomingSocket();
		pMediaSample -> GetPointer(&pIncomingBuffer);
		nSize = pMediaSample -> GetSize();
	}


	/*
	char buffer[10];			
	_itoa_s(nSize, buffer, 10);
	MessageBoxA(NULL, "No Data Sent!", buffer, MB_OK | MB_ICONERROR);
	*/
	
	if(recvfrom(sInSocket, (char*)pIncomingBuffer, nSize, NULL, NULL, NULL) == SOCKET_ERROR)
	{	
		MessageBoxA(NULL, "Failed to Recieve Datagram!", "Error", MB_OK | MB_ICONERROR);
	}
	
	
	/*
	HRESULT hr = CNetPin::Deliver(pMediaSample);
	if(FAILED(hr))
	{
		//MessageBoxA(NULL, "Sample not Delivered!", "Error", MB_OK | MB_ICONERROR);
	}

	pMediaSample -> Release();
	
	
	
	 REFERENCE_TIME rtStart = m_iFrameNumber * m_rtFrameLength;
	 REFERENCE_TIME rtStop  = rtStart + m_rtFrameLength;

	pMediaSample -> SetTime(&rtStart, &rtStop);
    m_iFrameNumber++;
	*/

	return S_OK;;
}



//Exported Functions

DECLSPEC STDMETHODIMP CNetSource::Source(char* dwPort, char* szSource)
{
	return S_OK;
}


DECLSPEC STDMETHODIMP CNetSource::Socket(SOCKET sSocket)
{
	return S_OK;
}



//----------------------------------------------------------Internal Socket-------------------------------------------------------------//

void CreateIncomingSocket()
{
	bInSocketCreated = true;

	struct addrinfo* pLocalInfo = NULL;
	struct addrinfo hints = {0};

	hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags = AI_PASSIVE;

    if(getaddrinfo(NULL, "6007", &hints, &pLocalInfo)) //&hints
    {
        MessageBoxA(NULL, "Failed to Resolve Local IP Address!", "Error", MB_OK | MB_ICONERROR);
        return;
    }

	sInSocket = socket(pLocalInfo -> ai_family, pLocalInfo -> ai_socktype, pLocalInfo -> ai_protocol);
    if (sInSocket == INVALID_SOCKET)
    {
        MessageBoxA(NULL, "Failed to Create Incoming Socket!", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    if(bind(sInSocket, pLocalInfo -> ai_addr, (int)pLocalInfo -> ai_addrlen))
    {
        MessageBoxA(NULL, "Failed to Bind Socket!", "Error", MB_OK | MB_ICONERROR);
        closesocket(sInSocket);
        return;
    }


}