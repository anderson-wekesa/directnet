#include "netsink.h"
#include "netsource.h"

#pragma comment (lib, "WS2_32.Lib")
#pragma comment (lib, "wininet.Lib")

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

//---------------------------------------------------Declare Filter Information-------------------------------------------------------//

//Declare Supported Media Types
AMOVIESETUP_MEDIATYPE amMediaTypes[] = 
{
	{&MEDIATYPE_Video, &MEDIASUBTYPE_None},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_PCM},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_None} 
};



//Declare Pins
const AMOVIESETUP_PIN amInputPin =  //Network Sink Pin
{
	L"",
	true,
	false, //Pin is an Input Pin
	false,
	false,
	&CLSID_NetworkSink, //Obsolete
	NULL, //Obsolete
	3, //Supports Three Media Types: Audio, Video and Stream
	amMediaTypes
};

const AMOVIESETUP_PIN amOutputPin =  //Network Source Pin
{
	L"",
	false,
	true, //Pin is an Output Pin
	false,
	false,
	&CLSID_NetworkSource, //Obsolete
	NULL, //Obsolete
	3, //Supports Three Media Types: Audio, Video and Stream
	amMediaTypes
};



//Filter Information
AMOVIESETUP_FILTER amSinkFilter = 
{
	&CLSID_NetworkSink,
	L"Network Sink",
	MERIT_DO_NOT_USE,//MERIT_NORMAL,
	1, //Number of Pins
	&amInputPin
};

AMOVIESETUP_FILTER amSourceFilter = 
{
	&CLSID_NetworkSource,
	L"Network Source",
	MERIT_DO_NOT_USE,//MERIT_NORMAL,
	1, //Number of Pins
	&amOutputPin
};



CFactoryTemplate g_Templates[2] = 
{
	{
		L"Network Sink",
		&CLSID_NetworkSink,					
		CNetSink::CreateInstance,
		NULL,
		&amSinkFilter 
	},

	{
		L"Network Source",
		&CLSID_NetworkSource,					
		CNetSource::CreateInstance,
		NULL,
		&amSourceFilter 
	}
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2(TRUE);
} 

STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2(FALSE);
}





BOOL WINAPI DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpReserved)
{
	return DllEntryPoint(reinterpret_cast<HINSTANCE>(hDllHandle), dwReason, lpReserved);
}


