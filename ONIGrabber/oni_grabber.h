#ifndef _MP_ONI_GRABBER
#define _MP_ONI_GRABBER

#include <ntk/camera/rgbd_grabber.h>
#include <ntk/gesture/skeleton.h>

// OpenNI headers include windows.h on windows without preventing
// preprocessor namespace pollution.
// FIXME: Factor this out.
#ifdef _WIN32
#   define NOMINMAX
#   define VC_EXTRALEAN
#endif
#include <XnOpenNI.h>
#ifdef _WIN32
#   undef VC_EXTRALEAN
#   undef NOMINMAX
#endif

#include <XnCodecIDs.h>
#include <XnCppWrapper.h>
#include <XnVSessionManager.h>
#include <XnVPushDetector.h>

// Under certain odd circumstances, qhull/io.h can be incorrectly included
// by XnPlatformWin32.h, dragging True and False as preprocessor macros,
// breaking in turn flann headers.
// See also: ntk/gesture/skeleton.h
// FIXME: Ensure that such a broken build system is never generated and remove
// this kludge.
#ifdef _WIN32
#   ifdef True
#       undef True
#   endif
#   ifdef False
#       undef False
#   endif
#endif

namespace ntk
{

class BodyEventDetector;
//
//class ONIDriver
//{
//public:
//    struct DeviceInfo
//    {
//        std::string creation_info;
//        std::string camera_type;
//        std::string serial;
//        std::string vendor;
//        unsigned short vendor_id;
//        unsigned short product_id;
//        unsigned char bus;
//        unsigned char address;
//    };
//
//public:
//    ONIDriver();
//    ~ONIDriver();
//
//public:
//    xn::Context& niContext() { return m_ni_context; }
//    int numDevices() const { return m_device_nodes.size(); }
//    const DeviceInfo& deviceInfo(int index) const { return m_device_nodes[index]; }
//    void checkXnError(const XnStatus& status, const char* what) const;
//
//private:
//    void findSerialNumbers();
//
//private:
//    struct Config;
//    Config* m_config;
//
//private:
//    xn::Context m_ni_context;
//    std::vector<DeviceInfo> m_device_nodes;
//    XnLicense m_license;
//};

class ONIGrabber : public ntk::RGBDGrabber
{
public:
	ONIGrabber(const std::string& file_path);

	/*! set a new xml config file for the grabber
	  * call it before initialize() */
    void set_xml_config_file(const std::string & xml_filename);

    /*! Call it before starting the thread. */
    virtual bool connectToDevice();

    /*! Disconnect from Kinect. */
    virtual bool disconnectFromDevice();

    /*! Set the body event detector. */
    void setBodyEventDetector(BodyEventDetector* detector)
    { m_body_event_detector = detector; }

    /*! Returns the associated body event detector. */
    const BodyEventDetector* bodyDetector() const { return m_body_event_detector; }

    /*! Set the maximal number of tracked users. Default is one. */
    void setMaxUsers(int num) { m_max_num_users = num; }

    /*! Set whether color images should be in high resolution 1280x1024. */
    void setHighRgbResolution(bool hr) { m_high_resolution = hr; }

    /*! Set an optional subsampling factor for the depth image. */
    void setSubsamplingFactor(int factor);

    /*! Set whether images should be vertically mirrored. */
    void setMirrored(bool m) { m_mirrored = m; }

    /*! Set whether User and Body trackers are enabled. */
    void setTrackUsers(bool enable) { m_track_users = enable; }

    /*! Grab IR images instead of RGB images. */
    virtual void setIRMode(bool ir);

	/*! Set whether custom bayer decoding should be used. */
	void setCustomBayerDecoding(bool enable) { m_custom_bayer_decoding = enable; }


public:
    /*! Callback: New user was detected */
    void newUserCallback(XnUserID nId);

    /*! Callback: An existing user was lost */
    void lostUserCallback(XnUserID nId);

    /*! Callback: Detected a pose */
    void userPoseDetectedCallback(XnUserID nId);

    /*! Callback: Calibration started */
    void calibrationStartedCallback(XnUserID nId);

    /*! Callback: Finished calibration */
    void calibrationFinishedCallback(XnUserID nId, bool success);

private:
    void setDefaultBayerMode();
    void waitAndUpdateActiveGenerators();
    void estimateCalibration();

public:
    // Nite accessors.
    xn::DepthGenerator& niDepthGenerator() { return m_ni_depth_generator; }
	xn::ImageGenerator& niRGBGenerator() { return m_ni_rgb_generator; }
    xn::UserGenerator& niUserGenerator() { return m_ni_user_generator; }
	xn::Context& niContext(){ return m_context; }
    const xn::DepthGenerator& niDepthGenerator() const { return m_ni_depth_generator; }
	const xn::ImageGenerator& niRGBGenerator() const { return m_ni_rgb_generator; }
    const xn::UserGenerator& niUserGenerator() const { return m_ni_user_generator; }

	std::string* get_file_path(){return &this->m_file_path;}
	std::string* get_file_name(){return &this->m_file_name;}

	protected:
		/*! Thread loop. */
		virtual void run();

	private:
		//OpenniDriver& m_driver;
		xn::Player m_player;
		xn::Context m_context;
		std::string m_file_path;
		std::string m_file_name;

		int m_camera_id;
		
		RGBDImage m_current_image;
		xn::Device m_ni_device;
		xn::DepthGenerator m_ni_depth_generator;
		xn::ImageGenerator m_ni_rgb_generator;
		xn::IRGenerator m_ni_ir_generator;
		xn::UserGenerator m_ni_user_generator;
		xn::HandsGenerator m_ni_hands_generator;
		xn::GestureGenerator m_ni_gesture_generator;
		int m_subsampling_factor;
		XnLicense license;
		bool m_need_pose_to_calibrate;
		XnChar m_calibration_pose[20];
		int m_max_num_users;
		BodyEventDetector* m_body_event_detector;
		bool m_high_resolution;
		bool m_mirrored;
		bool m_custom_bayer_decoding;

		/*! the default xml config file */
		static const std::string DEFAULT_XML_CONFIG_FILE;

		/*! the xml config file to load in initialize() */
		std::string m_xml_config_file;

		bool m_track_users;
		bool m_get_infrared;
		bool m_has_rgb;

		QMutex m_ni_mutex;
};

} //ntk

#endif