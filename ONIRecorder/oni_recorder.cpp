#include "oni_recorder.h"

//------------------------------------------------
// ONI RECORDER
//------------------------------------------------

static const XnCodecID CODEC_DONT_CAPTURE = XN_CODEC_NULL;

#define START_CAPTURE_CHECK_RC(rc, what)												\
	if (nRetVal != XN_STATUS_OK)														\
	{																					\
		printf("Failed to %s: %s\n", what, xnGetStatusString(rc));						\
		delete g_Capture.pRecorder;														\
		g_Capture.pRecorder = NULL;														\
		return false;																	\
	}

//------------------------------------------------
// CONSTRUCTORS
//------------------------------------------------
ONIRecorder::ONIRecorder(ntk::OpenniGrabber *m_grabber):oni_device(NULL){
	if(m_grabber){
		g_grabber = m_grabber;

		xn::Device *m_device = NULL;
		xn::DepthGenerator *m_depth = NULL;
		xn::ImageGenerator *m_image = NULL;
		xn::IRGenerator *m_IR = NULL;
		xn::AudioGenerator *m_audio = NULL;

		m_device = &m_grabber->niDevice();

		if(m_grabber->niDepthGenerator().IsValid()){
			m_depth = &m_grabber->niDepthGenerator();
		}

		if(m_grabber->niRGBGenerator().IsValid()){
			m_image = &m_grabber->niRGBGenerator();
		}

		if(m_grabber->niIRGenerator().IsValid()){
			m_IR = &m_grabber->niIRGenerator();
		}

		//Audio not supported

		if(!m_device || !m_depth) return ;

		this->oni_device = new ONIDevice(&m_grabber->niContext(),m_device,m_depth,m_image,m_IR,m_audio);
	}
}

ONIRecorder::~ONIRecorder(){

}

//------------------------------------------------
// SETUP
//------------------------------------------------
void ONIRecorder::captureInit(){
	// Depth Formats
	int nIndex = 0;

	g_DepthFormat.pValues[nIndex] = XN_CODEC_16Z_EMB_TABLES;
	g_DepthFormat.pIndexToName[nIndex] = "PS Compression (16z ET)";
	nIndex++;

	g_DepthFormat.pValues[nIndex] = XN_CODEC_UNCOMPRESSED;
	g_DepthFormat.pIndexToName[nIndex] = "Uncompressed";
	nIndex++;

	g_DepthFormat.pValues[nIndex] = CODEC_DONT_CAPTURE;
	g_DepthFormat.pIndexToName[nIndex] = "Not Captured";
	nIndex++;

	g_DepthFormat.nValuesCount = nIndex;

	// Image Formats
	nIndex = 0;

	g_ImageFormat.pValues[nIndex] = XN_CODEC_JPEG;
	g_ImageFormat.pIndexToName[nIndex] = "JPEG";
	nIndex++;

	g_ImageFormat.pValues[nIndex] = XN_CODEC_UNCOMPRESSED;
	g_ImageFormat.pIndexToName[nIndex] = "Uncompressed";
	nIndex++;

 	g_ImageFormat.pValues[nIndex] = CODEC_DONT_CAPTURE;
 	g_ImageFormat.pIndexToName[nIndex] = "Not Captured";
	nIndex++;

	g_ImageFormat.nValuesCount = nIndex;

	// IR Formats
	nIndex = 0;

	g_IRFormat.pValues[nIndex] = XN_CODEC_UNCOMPRESSED;
	g_IRFormat.pIndexToName[nIndex] = "Uncompressed";
	nIndex++;

	g_IRFormat.pValues[nIndex] = CODEC_DONT_CAPTURE;
	g_IRFormat.pIndexToName[nIndex] = "Not Captured";
	nIndex++;

	g_IRFormat.nValuesCount = nIndex;

	// Audio Formats
	nIndex = 0;

	g_AudioFormat.pValues[nIndex] = XN_CODEC_UNCOMPRESSED;
	g_AudioFormat.pIndexToName[nIndex] = "Uncompressed";
	nIndex++;

 	g_AudioFormat.pValues[nIndex] = CODEC_DONT_CAPTURE;
 	g_AudioFormat.pIndexToName[nIndex] = "Not Captured";
	nIndex++;

	g_AudioFormat.nValuesCount = nIndex;

	// Init
	g_Capture.csFileName[0] = 0;
	g_Capture.State = NOT_CAPTURING;
	g_Capture.nCapturedFrameUniqueID = 0;
	g_Capture.csprintf[0] = '\0';
	g_Capture.bSkipFirstFrame = false;

	g_Capture.nodes[CAPTURE_DEPTH_NODE].captureFormat = XN_CODEC_16Z_EMB_TABLES;
	g_Capture.nodes[CAPTURE_IMAGE_NODE].captureFormat = XN_CODEC_JPEG;
	g_Capture.nodes[CAPTURE_IR_NODE].captureFormat = XN_CODEC_UNCOMPRESSED;
	g_Capture.nodes[CAPTURE_AUDIO_NODE].captureFormat = XN_CODEC_UNCOMPRESSED;
}

void ONIRecorder::captureBrowse(int){
	#if (XN_PLATFORM == XN_PLATFORM_WIN32)
	OPENFILENAMEA ofn;
	TCHAR *szFilter = TEXT("ONI Files (*.oni)\0")
		TEXT("*.oni\0")
		TEXT("All Files (*.*)\0")
		TEXT("*.*\0");

	ZeroMemory(&ofn,sizeof(OPENFILENAME));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.lpstrFilter = szFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = g_Capture.csFileName;
	ofn.nMaxFile = sizeof(g_Capture.csFileName);
	ofn.lpstrTitle = TEXT("Capture to...");
	ofn.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR;

	GetSaveFileName(&ofn); 

	if (g_Capture.csFileName[0] != 0)
	{
		if (strstr(g_Capture.csFileName, ".oni") == NULL)
		{
			strcat(g_Capture.csFileName, ".oni");
		}
	}
#else // not Win32
	strcpy(g_Capture.csFileName, "./Captured.oni");
#endif

	// as we waited for user input, it's probably better to discard first frame (especially if an accumulating
	// stream is on, like audio).
	g_Capture.bSkipFirstFrame = true;

	this->captureOpenWriteDevice();
}

bool ONIRecorder::captureOpenWriteDevice(){
	XnStatus nRetVal = XN_STATUS_OK;

	xn::NodeInfoList recordersList;
	nRetVal = oni_device->getContext()->EnumerateProductionTrees(XN_NODE_TYPE_RECORDER, NULL, recordersList);
	START_CAPTURE_CHECK_RC(nRetVal, "Enumerate recorders");
	// take first
	xn::NodeInfo chosen = *recordersList.Begin();

	g_Capture.pRecorder = new xn::Recorder;
	nRetVal = oni_device->getContext()->CreateProductionTree(chosen, *g_Capture.pRecorder);
	START_CAPTURE_CHECK_RC(nRetVal, "Create recorder");

	nRetVal = g_Capture.pRecorder->SetDestination(XN_RECORD_MEDIUM_FILE, g_Capture.csFileName);
	START_CAPTURE_CHECK_RC(nRetVal, "Set output file");

	return true;
}

//------------------------------------------------
// CAPTURE
//------------------------------------------------
void ONIRecorder::captureStart(int nDelay){
	if (g_Capture.csFileName[0] == 0)
	{
		captureBrowse(0);
	}

	if (g_Capture.csFileName[0] == 0)
		return;

	if (g_Capture.pRecorder == NULL)
	{
		if (!captureOpenWriteDevice())
			return;
	}

	XnUInt64 nNow;
	xnOSGetTimeStamp(&nNow);
	nNow /= 1000;

	g_Capture.nStartOn = (XnUInt32)nNow + nDelay;
	g_Capture.State = SHOULD_CAPTURE;

	this->stopBayerMode();
}

void ONIRecorder::stopBayerMode(){
	this->g_grabber->acquireReadLock();

	//Allows Grabber to stop reading
	Sleep(1);

	this->oni_device->getImageGenerator()->SetIntProperty("InputFormat", 3);
	this->oni_device->getImageGenerator()->SetPixelFormat(XN_PIXEL_FORMAT_RGB24); 
	this->g_grabber->setCustomBayerDecoding(false);

	this->g_grabber->releaseReadLock();
}

void ONIRecorder::captureRestart(int){
	captureCloseWriteDevice();
	if (captureOpenWriteDevice())
		captureStart(0);
}

XnStatus ONIRecorder::captureFrame(){
	XnStatus nRetVal = XN_STATUS_OK;

	if (g_Capture.State == SHOULD_CAPTURE)
	{
		XnUInt64 nNow;
		xnOSGetTimeStamp(&nNow);
		nNow /= 1000;

		// check if time has arrived
		if (nNow >= g_Capture.nStartOn)
		{
			// check if we need to discard first frame
			if (g_Capture.bSkipFirstFrame)
			{
				g_Capture.bSkipFirstFrame = false;
			}
			else
			{
				// start recording
				for (int i = 0; i < CAPTURE_NODE_COUNT; ++i)
				{
					g_Capture.nodes[i].nCapturedFrames = 0;
					g_Capture.nodes[i].bRecording = false;
				}
				g_Capture.State = CAPTURING;

				// add all captured nodes
				if (oni_device->getDevice() != NULL)
				{
					nRetVal = g_Capture.pRecorder->AddNodeToRecording(*oni_device->getDevice(), XN_CODEC_UNCOMPRESSED);
					START_CAPTURE_CHECK_RC(nRetVal, "add device node");
				}

				if (oni_device->isDepthOn() && (g_Capture.nodes[CAPTURE_DEPTH_NODE].captureFormat != CODEC_DONT_CAPTURE))
				{
					nRetVal = g_Capture.pRecorder->AddNodeToRecording(*oni_device->getDepthGenerator(), g_Capture.nodes[CAPTURE_DEPTH_NODE].captureFormat);
					START_CAPTURE_CHECK_RC(nRetVal, "add depth node");
					g_Capture.nodes[CAPTURE_DEPTH_NODE].bRecording = TRUE;
					g_Capture.nodes[CAPTURE_DEPTH_NODE].pGenerator = oni_device->getDepthGenerator();
				}

				if (oni_device->isImageOn() && (g_Capture.nodes[CAPTURE_IMAGE_NODE].captureFormat != CODEC_DONT_CAPTURE))
				{
					nRetVal = g_Capture.pRecorder->AddNodeToRecording(*oni_device->getImageGenerator(), g_Capture.nodes[CAPTURE_IMAGE_NODE].captureFormat);
					START_CAPTURE_CHECK_RC(nRetVal, "add image node");
					g_Capture.nodes[CAPTURE_IMAGE_NODE].bRecording = TRUE;
					g_Capture.nodes[CAPTURE_IMAGE_NODE].pGenerator = oni_device->getImageGenerator();
				}

				if (oni_device->isIROn() && (g_Capture.nodes[CAPTURE_IR_NODE].captureFormat != CODEC_DONT_CAPTURE))
				{
					nRetVal = g_Capture.pRecorder->AddNodeToRecording(*oni_device->getIRGenerator(), g_Capture.nodes[CAPTURE_IR_NODE].captureFormat);
					START_CAPTURE_CHECK_RC(nRetVal, "add IR stream");
					g_Capture.nodes[CAPTURE_IR_NODE].bRecording = TRUE;
					g_Capture.nodes[CAPTURE_IR_NODE].pGenerator = oni_device->getIRGenerator();
				}

				if (oni_device->isAudioOn() && (g_Capture.nodes[CAPTURE_AUDIO_NODE].captureFormat != CODEC_DONT_CAPTURE))
				{
					nRetVal = g_Capture.pRecorder->AddNodeToRecording(*oni_device->getAudioGenerator(), g_Capture.nodes[CAPTURE_AUDIO_NODE].captureFormat);
					START_CAPTURE_CHECK_RC(nRetVal, "add Audio stream");
					g_Capture.nodes[CAPTURE_AUDIO_NODE].bRecording = TRUE;
					g_Capture.nodes[CAPTURE_AUDIO_NODE].pGenerator = oni_device->getAudioGenerator();
				}
			}
		}
	}

	if (g_Capture.State == CAPTURING)
	{
		// There isn't a real need to call Record() here, as the WaitXUpdateAll() call already makes sure
		// recording is performed.
		nRetVal = g_Capture.pRecorder->Record();
		XN_IS_STATUS_OK(nRetVal);

		// count recorded frames
		for (int i = 0; i < CAPTURE_NODE_COUNT; ++i)
		{
			if (g_Capture.nodes[i].bRecording && g_Capture.nodes[i].pGenerator->IsDataNew())
				g_Capture.nodes[i].nCapturedFrames++;
		}
	}
	return XN_STATUS_OK;
}

bool ONIRecorder::isCapturing(){
	return (this->g_Capture.State != NOT_CAPTURING);
}

//------------------------------------------------
// CAPTURE STOP
//------------------------------------------------
void ONIRecorder::captureStop(int){
	if (g_Capture.State != NOT_CAPTURING)
	{
		g_Capture.State = NOT_CAPTURING;
		captureCloseWriteDevice();
	}

	this->startBayerMode();
}

void ONIRecorder::captureCloseWriteDevice(){
	if (g_Capture.pRecorder != NULL)
	{
		g_Capture.pRecorder->Release();
		delete g_Capture.pRecorder;
		g_Capture.pRecorder = NULL;
		g_Capture.csFileName[0] = 0;
	}
}

void ONIRecorder::startBayerMode(){
	this->g_grabber->acquireReadLock();

	//Allows Grabber to stop reading
	Sleep(1);

	this->oni_device->getImageGenerator()->SetIntProperty("InputFormat", 6);
	this->oni_device->getImageGenerator()->SetPixelFormat(XN_PIXEL_FORMAT_GRAYSCALE_8_BIT); 
	this->g_grabber->setCustomBayerDecoding(true);

	this->g_grabber->releaseReadLock();
}
		


//------------------------------------------------
// ONI DEVICE
//------------------------------------------------

ONIDevice::ONIDevice(xn::Context* m_context, xn::Device *m_device,
					 xn::DepthGenerator *m_depth, xn::ImageGenerator* m_image,
					 xn::IRGenerator *m_IR, xn::AudioGenerator *m_Audio): 
						g_pPrimary(NULL), g_Context(m_context){
		
	if(m_device){
		this->g_Device = m_device;
	}
	else{
		printf("ERROR m_device\n");
		g_isDeviceOK = false;
		return;
	}

	if(m_depth){
		this->g_Depth = m_depth;
		this->g_bIsDepthOn = true;
	}
	else{
		printf("ERROR m_depth\n");
		this->g_bIsDepthOn = false;
	}

	if(m_image){
		this->g_Image = m_image;
		this->g_bIsImageOn = true;
	}
	else{
		printf("ERROR m_image\n");
		this->g_bIsImageOn = false;
	}

	if(m_IR){
		this->g_IR = m_IR;
		this->g_bIsIROn = true;
	}
	else{
		printf("ERROR m_IR\n");
		this->g_bIsIROn = false;
	}

	if(m_Audio){
		this->g_Audio = m_Audio;
		this->g_bIsAudioOn = true;
	}
	else{
		printf("ERROR m_Audio\n");
		this->g_bIsAudioOn = false;
	}
}