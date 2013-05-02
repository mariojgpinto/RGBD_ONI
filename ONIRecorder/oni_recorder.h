#ifndef _ONI_RECORDER
#define _ONI_RECORDER

//#include "ONIDevice.h"
#include <ntk/ntk.h>
#include <ntk/camera/openni_grabber.h>

#if (XN_PLATFORM == XN_PLATFORM_WIN32)
#include <Commdlg.h>
#include <Windows.h>
#endif

#include <XnCppWrapper.h>
#include <XnTypes.h>
#include <math.h>
#include <XnLog.h>

//------------------------------------------------
// ONI DEVICE
//------------------------------------------------
class ONIDevice{
	public:
		ONIDevice(xn::Context* m_context, xn::Device *m_device,
				  xn::DepthGenerator *m_depth, xn::ImageGenerator* m_image,
				  xn::IRGenerator *g_IR = 0, xn::AudioGenerator *g_Audio= 0);

		xn::Context* getContext(){return this->g_Context;}
		xn::Device* getDevice(){return this->g_Device;}
		xn::DepthGenerator* getDepthGenerator(){return this->g_Depth;}
		xn::ImageGenerator* getImageGenerator(){return this->g_Image;}
		xn::IRGenerator* getIRGenerator(){return this->g_IR;}
		xn::AudioGenerator* getAudioGenerator(){return this->g_Audio;}

		const xn::DepthMetaData* getDepthMetaData(){return this->g_Depth ? &this->g_DepthMD : NULL;}
		const xn::ImageMetaData* getImageMetaData(){return this->g_Image ? &this->g_ImageMD : NULL;}
		const xn::IRMetaData* getIRMetaData(){return this->g_IR ? &this->g_irMD : NULL;}
		const xn::AudioMetaData* getAudioMetaData(){return this->g_Audio ? &this->g_AudioMD : NULL;}

		xn::ProductionNode* getPrimaryProductionNode(){return this->g_pPrimary;}

		bool isDeviceOK(){return g_isDeviceOK;}
		bool isDepthOn(){return (g_bIsDepthOn);}
		bool isImageOn(){return (g_bIsImageOn);}
		bool isIROn(){return (g_bIsIROn);}
		bool isAudioOn(){return (g_bIsAudioOn);}

		
	private:
		xn::Context *g_Context;

		bool g_isDeviceOK;

		bool g_bIsDepthOn;
		bool g_bIsImageOn;
		bool g_bIsIROn;
		bool g_bIsAudioOn;

		xn::Device *g_Device;
		xn::DepthGenerator *g_Depth;
		xn::ImageGenerator *g_Image;
		xn::IRGenerator *g_IR;
		xn::AudioGenerator *g_Audio;
		//xn::Player *g_Player;

		xn::DepthMetaData g_DepthMD;
		xn::ImageMetaData g_ImageMD;
		xn::IRMetaData g_irMD;
		xn::AudioMetaData g_AudioMD;


		xn::ProductionNode *g_pPrimary;
};

// --------------------------------
// Defines
// --------------------------------
#define MAX_STRINGS 20

//------------------------------------------------
// ONI RECORDER
//------------------------------------------------
class ONIRecorder{
	public:
		// --------------------------------
		// Types
		// --------------------------------
		typedef enum
		{
			NOT_CAPTURING,
			SHOULD_CAPTURE,
			CAPTURING,
		} CapturingState;

		typedef enum
		{
			CAPTURE_DEPTH_NODE,
			CAPTURE_IMAGE_NODE,
			CAPTURE_IR_NODE,
			CAPTURE_AUDIO_NODE,
			CAPTURE_NODE_COUNT
		} CaptureNodeType;

		typedef struct NodeCapturingData
		{
			XnCodecID captureFormat;
			XnUInt32 nCapturedFrames;
			bool bRecording;
			xn::Generator* pGenerator;
		} NodeCapturingData;

		typedef struct CapturingData
		{
			NodeCapturingData nodes[CAPTURE_NODE_COUNT];
			xn::Recorder* pRecorder;
			char csFileName[XN_FILE_MAX_PATH];
			XnUInt32 nStartOn; // time to start, in seconds
			bool bSkipFirstFrame;
			CapturingState State;
			XnUInt32 nCapturedFrameUniqueID;
			char csprintf[500];
		} CapturingData;

		typedef struct
		{
			int nValuesCount;
			XnCodecID pValues[MAX_STRINGS];
			const char* pIndexToName[MAX_STRINGS];
		} NodeCodec;

	public:
		ONIRecorder(ntk::OpenniGrabber *m_grabber);
		~ONIRecorder();

		bool isReady(){return (oni_device) ? true : false;}

		void captureInit();
		void captureBrowse(int);
		bool captureOpenWriteDevice();

		void captureStart(int nDelay);
		void stopBayerMode();
		void startBayerMode();
		void captureRestart(int);
		XnStatus captureFrame();

		void captureStop(int);
		void captureCloseWriteDevice();

		//void captureSingleFrame(int);
		bool isCapturing();

	//public:
	//	void captureSetDepthFormat(int format);
	//	void captureSetImageFormat(int format);
	//	void captureSetIRFormat(int format);
	//	void captureSetAudioFormat(int format);
	//	const char* captureGetDepthFormatName();
	//	const char* captureGetImageFormatName();
	//	const char* captureGetIRFormatName();
	//	const char* captureGetAudioFormatName();

	private:
		ONIDevice * oni_device;

		CapturingData g_Capture;

		NodeCodec g_DepthFormat;
		NodeCodec g_ImageFormat;
		NodeCodec g_IRFormat;
		NodeCodec g_AudioFormat;

		ntk::OpenniGrabber* g_grabber;
};

#endif