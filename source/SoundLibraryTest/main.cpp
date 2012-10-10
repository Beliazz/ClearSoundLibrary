#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <iostream>
#include <Windows.h>
#include <stdio.h>
#include <io.h>
#include <memory>
#include <fstream>

#ifdef _DEBUG
#define DEBUG_CLIENTBLOCK new( _CLIENT_BLOCK, __FILE__, __LINE__)
#else
#define DEBUG_CLIENTBLOCK new 
#endif

//XACT
#include <xact3.h>
#include <xact3d3.h>
#pragma comment (lib, "x3daudio.lib")

#include <sapi.h>
#pragma comment (lib, "sapi.lib")

#define SAFE_RELEASE(x)			if(x){x->Release(); x = NULL;}
#define SAFE_DELETE(x)			if(x){delete x; x = NULL;}

using namespace std;
using namespace tr1;

int WINAPI WinMain( __in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in LPSTR lpCmdLine, __in int nShowCmd )
{
	//////////////////////////////////////////////////////////////////////////
	// Set up checks for memory leaks.
	// always perform a leak check just before app exits
	int tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF;  
	_CrtSetDbgFlag(tmpDbgFlag);

	//////////////////////////////////////////////////////////////////////////
	// set up console
	AllocConsole();
	SetConsoleTitleW(L"Bloco-Engine Console");
	HANDLE stdOutputHandle = GetStdHandle( STD_OUTPUT_HANDLE ); 
	int hConsole = _open_osfhandle( (intptr_t)stdOutputHandle, 0x4000);
	FILE *pFile = _fdopen( hConsole, "w" );
	*stdout = *pFile;
	setvbuf( stdout, NULL, _IONBF, 0 );
	ios::sync_with_stdio();
	
	//////////////////////////////////////////////////////////////////////////
	// initialize COM
	::CoInitialize(NULL);

	//////////////////////////////////////////////////////////////////////////
	// file stream for opening files
	ifstream fileStream;
	
	//////////////////////////////////////////////////////////////////////////
	// load global settings
	shared_ptr<CHAR> pGlobalSettingsData = NULL;
	UINT globalSettingsDataSize = 0;
	fileStream.open ("SoundTest.xgs", ios::in|ios::binary|ios::ate);
	if (!fileStream.is_open())
	{
		return 1;
	}

	globalSettingsDataSize = (int)fileStream.tellg();
	pGlobalSettingsData = shared_ptr<char>(new char[globalSettingsDataSize]);
	fileStream.seekg(0, std::ios_base::beg);
	fileStream.read(pGlobalSettingsData.get(), globalSettingsDataSize);
	fileStream.close();

	//////////////////////////////////////////////////////////////////////////
	// load wave bank
	shared_ptr<CHAR> pWaveBankData = NULL;
	UINT waveBankDataSize = 0;
	fileStream.open ("Wave Bank.xwb", ios::in|ios::binary|ios::ate);
	if (!fileStream.is_open())
	{
		return 2;
	}

	waveBankDataSize = (int)fileStream.tellg();
	pWaveBankData = shared_ptr<char>(new char[waveBankDataSize]);
	fileStream.seekg(0, std::ios_base::beg);
	fileStream.read(pWaveBankData.get(), waveBankDataSize);
	fileStream.close();

	//////////////////////////////////////////////////////////////////////////
	// load sound bank
	shared_ptr<CHAR> pSoundBankData = NULL;
	UINT soundBankDataSize = 0;
	fileStream.open ("Sound Bank.xsb", ios::in|ios::binary|ios::ate);
	if (!fileStream.is_open())
	{
		return 3;
	}

	soundBankDataSize = (int)fileStream.tellg();
	pSoundBankData = shared_ptr<char>(new char[soundBankDataSize]);
	fileStream.seekg(0, std::ios_base::beg);
	fileStream.read(pSoundBankData.get(), soundBankDataSize);
	fileStream.close();

	//////////////////////////////////////////////////////////////////////////
	// create xact3 engine
	IXACT3Engine* pXact;
	if(FAILED(XACT3CreateEngine(0, &pXact)))
	{
		return 4;
	}

	//////////////////////////////////////////////////////////////////////////
	// initialize the xact3 engine
	// 
	// get renderer details
	XACT_RENDERER_DETAILS rendererDetails;
	if (FAILED(pXact->GetRendererDetails(0, &rendererDetails)))
		return 5;
	
	XACT_RUNTIME_PARAMETERS params;
	params.pRendererID = rendererDetails.rendererID;
	params.lookAheadTime = XACT_ENGINE_LOOKAHEAD_DEFAULT;
	params.pGlobalSettingsBuffer = pGlobalSettingsData.get();
	params.globalSettingsBufferSize = globalSettingsDataSize;
	params.globalSettingsFlags = 0;
	params.pMasteringVoice = NULL;
	params.pXAudio2 = NULL;
	params.fnNotificationCallback = NULL;

	if (FAILED(pXact->Initialize(&params)))
	{
		return 6;
	}

	//////////////////////////////////////////////////////////////////////////
	// create wave bank
	IXACT3WaveBank* pWaveBank;
	if (FAILED(pXact->CreateInMemoryWaveBank(pWaveBankData.get(), waveBankDataSize, 0, 0, &pWaveBank)))
		return 7;

	//////////////////////////////////////////////////////////////////////////
	// create sound bank
	IXACT3SoundBank* pSoundBank;
	if (FAILED(pXact->CreateSoundBank(pSoundBankData.get(), soundBankDataSize, 0, 0, &pSoundBank)))
		return 8;

	//////////////////////////////////////////////////////////////////////////
	// initialize xact3d
	X3DAUDIO_HANDLE hX3D;
	if (FAILED(XACT3DInitialize(pXact, hX3D)))
	{
		return 9;
	}

	//////////////////////////////////////////////////////////////////////////
	// create xact3d emitter
	X3DAUDIO_EMITTER emitter;
	X3DAUDIO_VECTOR position;
	X3DAUDIO_VECTOR up;
	X3DAUDIO_VECTOR front;
	X3DAUDIO_VECTOR velocity;

	ZeroMemory(&emitter, sizeof(emitter));
	ZeroMemory(&position, sizeof(position));
	ZeroMemory(&up, sizeof(up));
	ZeroMemory(&front, sizeof(front));
	ZeroMemory(&velocity, sizeof(velocity));

	up.y = 1.0f;
	front.z = 1.0f;
	position.z = 10.0f;

	emitter.ChannelCount = 1;
	emitter.ChannelRadius = 1.0f;
	emitter.pCone = NULL;
	emitter.OrientFront = front;
	emitter.OrientTop = up;
	emitter.CurveDistanceScaler = 1.0f;
	emitter.DopplerScaler = 0.0f;
	emitter.InnerRadius = 1.0f;
	emitter.InnerRadiusAngle = X3DAUDIO_PI/4.0f;
	emitter.pChannelAzimuths = NULL;
	emitter.pLFECurve = NULL;
	emitter.pLPFDirectCurve = NULL;
	emitter.pLPFReverbCurve = NULL;
	emitter.Position = position;
	emitter.pReverbCurve = NULL;
	emitter.pVolumeCurve = NULL;
	emitter.Velocity = velocity;

	//////////////////////////////////////////////////////////////////////////
	// create xact3d listener
	X3DAUDIO_LISTENER listener;

	position.z = 0.0f;

	listener.OrientFront = front;
	listener.OrientTop = up;
	listener.pCone = NULL;
	listener.Position = position;
	listener.Velocity = velocity;

	//////////////////////////////////////////////////////////////////////////
	// create dsp settings
	// 
	// get mix format
	WAVEFORMATEXTENSIBLE format;
	pXact->GetFinalMixFormat(&format);
 
	X3DAUDIO_DSP_SETTINGS dspSettings;
	FLOAT32 delayTimes[1];
	delayTimes[0] = 0.0f;

	dspSettings.DstChannelCount = format.Format.nChannels;
	dspSettings.SrcChannelCount = 1;
	dspSettings.pMatrixCoefficients = new FLOAT32[dspSettings.DstChannelCount * dspSettings.SrcChannelCount];
	dspSettings.pDelayTimes = delayTimes;

	ZeroMemory(dspSettings.pMatrixCoefficients, sizeof( FLOAT32 ) * dspSettings.DstChannelCount * dspSettings.SrcChannelCount);
	if (FAILED(XACT3DCalculate(hX3D, &listener, &emitter, &dspSettings)))
		return 10;

	//////////////////////////////////////////////////////////////////////////
	// prepare cue
	IXACT3Cue* pCue;
	if(FAILED(pSoundBank->Prepare(0, 0, 0, &pCue)))
		return 11;

	//////////////////////////////////////////////////////////////////////////
	// apply 3d settings
	if(FAILED(XACT3DApply(&dspSettings, pCue)))
		return 12;
	
	//////////////////////////////////////////////////////////////////////////
	// play cue
	if(FAILED(pCue->Play()))
		return 13;

	//////////////////////////////////////////////////////////////////////////
	// main loop
	DWORD state = XACT_CUESTATE_PLAYING;
	while(state == XACT_CUESTATE_PLAYING)
	{
		Sleep( 16 );

		emitter.Velocity.z = -10.0f; 
		emitter.Position.z += emitter.Velocity.z * (1.0f / 60.0f);
		
		printf("pos: %f, %f, %f;  %f, %f, %f\r", emitter.Position.x,  emitter.Position.y,  emitter.Position.z, 
												 listener.Position.x, listener.Position.y, listener.Position.z);

		//////////////////////////////////////////////////////////////////////////
		// calculate dsp settings
		if (FAILED(XACT3DCalculate(hX3D, &listener, &emitter, &dspSettings)))
			return 14;

		//////////////////////////////////////////////////////////////////////////
		// apply 3d settings
		if(FAILED(XACT3DApply(&dspSettings, pCue)))
			return 15;

		//////////////////////////////////////////////////////////////////////////
		// process sound
		if (FAILED(pXact->DoWork()))
			return 16;

		//////////////////////////////////////////////////////////////////////////
		// get cue state ( check if playing ) 
		if (FAILED(pCue->GetState(&state)))
			return 17;
	}

	//////////////////////////////////////////////////////////////////////////
	// clean up
	SAFE_DELETE(dspSettings.pMatrixCoefficients);
	pXact->ShutDown();
	SAFE_RELEASE(pXact)
	::CoUninitialize();

	//////////////////////////////////////////////////////////////////////////
	// wait for input before exiting
	// system("PAUSE");
	
	return 0;
}