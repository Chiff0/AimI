#pragma once


#include <vector>
#include <opencv2/opencv.hpp>



class ScreenCapture {

public: 

    ScreenCapture();
    ~ScreenCapture();

    cv::Mat grab_frame();

private:

    ID3D11Device* m_Device = nullptr;
    ID3D11DeviceContext* m_ImmediateContext = nullptr;
    IDXGIOutputDuplication* m_DeskDupl = nullptr;
    ID3D11Texture2D* m_StagingTexture = nullptr;
    
    int width_ = 0;
    int height_ = 0;

};

