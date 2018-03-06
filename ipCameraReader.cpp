#include <openpose/producer/ipCameraReader.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <openpose/utilities/fastMath.hpp>

namespace op
{
    // Public IP cameras for testing (add ?x.mjpeg):
    // http://iris.not.iac.es/axis-cgi/mjpg/video.cgi?resolution=320x240?x.mjpeg
    // http://www.webcamxp.com/publicipcams.aspx

    IpCameraReader::IpCameraReader(const std::string & cameraPath) :
        VideoCaptureReader{cameraPath, ProducerType::IPCamera},
        mPathName{cameraPath},
        mFps{30.},
        mFrameNameCounter{-1},
        mThreadOpened{false}
    {
        try
        {
            if (isOpened())
            {
                // Start buffering thread
                mThreadOpened = true;
                mThread = std::thread{&IpCameraReader::bufferingThread, this};
            }
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }

    IpCameraReader::~IpCameraReader()
    {
        try
        {
            // Close and join thread
            if (mThreadOpened)
            {
                mCloseThread = true;
                mThread.join();
            }
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }

    std::string IpCameraReader::getFrameName()
    {
        try
        {
            return VideoCaptureReader::getFrameName();
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
            return "";
        }
    }

    double IpCameraReader::get(const int capProperty)
    {
        try
        {
            if (capProperty == CV_CAP_PROP_POS_FRAMES)
                return (double)mFrameNameCounter;
            else if (capProperty == CV_CAP_PROP_FPS)
                return mFps;
            else
                return VideoCaptureReader::get(capProperty);
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
            return 0.;
        }
    }

    void IpCameraReader::set(const int capProperty, const double value)
    {
        try
        {
            if (capProperty == CV_CAP_PROP_FPS)
                mFps = value;
            else
            VideoCaptureReader::set(capProperty, value);
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }

    cv::Mat IpCameraReader::getRawFrame()
    {
        try
        {
            mFrameNameCounter++; // Simple counter: 0,1,2,3,...

            // Retrieve frame from buffer
            cv::Mat cvMat;
            auto cvMatRetrieved = false;
            while (!cvMatRetrieved)
            {
                // Retrieve frame
                std::unique_lock<std::mutex> lock{mBufferMutex};
                if (!mBuffer.empty())
                {
                    std::swap(cvMat, mBuffer);
                    cvMatRetrieved = true;
                }
                // No frames available -> sleep & wait
                else
                {
                    lock.unlock();
                    std::this_thread::sleep_for(std::chrono::microseconds{5});
                }
            }
            return cvMat;

            // Naive implementation - No flashing buffers
            // return VideoCaptureReader::getRawFrame();
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
            return cv::Mat();
        }
    }

    void IpCameraReader::bufferingThread()
    {
        mCloseThread = false;
        while (!mCloseThread)
        {
            // Get frame
            auto cvMat = VideoCaptureReader::getRawFrame();
            // Move to buffer
            if (!cvMat.empty())
            {
                const std::lock_guard<std::mutex> lock{mBufferMutex};
                std::swap(mBuffer, cvMat);
            }
        }
    }
}
