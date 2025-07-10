#include "ScreenCapture.h"
#include <stdexcept> 
#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>

#pragma comment (lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
using namespace std;
 
ScreenCapture::ScreenCapture ()
{
    HRESULT hr = S_OK;

    D3D_FEATURE_LEVEL feature_levels[] = {
        D3D_FEATURE_LEVEL_11_0, 
        D3D_FEATURE_LEVEL_10_1, 
        D3D_FEATURE_LEVEL_10_0, 
        D3D_FEATURE_LEVEL_9_1
    };

    D3D_FEATURE_LEVEL feature_level_supported;

    hr = D3D11CreateDevice (
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        feature_levels, 
        ARRAYSIZE (feature_levels), 
        D3D11_SDK_VERSION,          
        &m_Device,                  
        &feature_level_supported,   
        &m_ImmediateContext 
    );

    if (FAILED (hr)) throw runtime_error ("Failed to create D3D11 device.");
    
    cout << "D3D11 Device and Context created successfully." << endl;
    
    IDXGIDevice* DxgiDevice = nullptr;

    hr = m_Device -> QueryInterface (__uuidof (IDXGIDevice), reinterpret_cast<void**> (&DxgiDevice)); 
    if (FAILED (hr)) throw runtime_error ("Failed to get DXGI Device.");
    
    IDXGIAdapter* DxgiAdapter = nullptr;

    hr = DxgiDevice -> GetParent(__uuidof (IDXGIAdapter), reinterpret_cast<void**> (&DxgiAdapter));
    DxgiDevice -> Release (); 
    if (FAILED(hr)) throw runtime_error ("Failed to get DXGI Adapter.");
    
    IDXGIOutput* DxgiOutput = nullptr;

    hr = DxgiAdapter -> EnumOutputs(0, &DxgiOutput);
    DxgiAdapter -> Release (); 
    if (FAILED (hr)) throw runtime_error ("Failed to get DXGI Output.");

    DXGI_OUTPUT_DESC outputDesc;

    DxgiOutput -> GetDesc (&outputDesc);
    m_Width = outputDesc.DesktopCoordinates.right - outputDesc.DesktopCoordinates.left;
    m_Height = outputDesc.DesktopCoordinates.bottom - outputDesc.DesktopCoordinates.top;

    IDXGIOutput1* DxgiOutput1 = nullptr;

    hr = DxgiOutput -> QueryInterface (__uuidof (IDXGIOutput1), reinterpret_cast<void**> (&DxgiOutput1));
    DxgiOutput -> Release (); 
    if (FAILED (hr)) throw runtime_error ("Failed to get DXGI Output 1.");
    

    hr = DxgiOutput1 -> DuplicateOutput (m_Device, &m_DeskDupl);
    DxgiOutput1 -> Release ();
    if (FAILED (hr)) throw runtime_error ("Failed to create desktop duplication.");
    

    cout << "Desktop duplication initialized successfully." << endl;
    cout << "Screen dimensions: " << m_Width << "x" << m_Height << endl;
    
    D3D11_TEXTURE2D_DESC desc;
    desc.Width = m_Width;
    desc.Height = m_Height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;
    hr = m_Device->CreateTexture2D(&desc, NULL, &m_StagingTexture);
    if (FAILED(hr)) {
        throw runtime_error("Failed to create staging texture.");
    }

    
}

cv::Mat ScreenCapture::grab_frame () 
{
    IDXGIResource* DesktopResource = nullptr;
    DXGI_OUTDUPL_FRAME_INFO FrameInfo;


    HRESULT hr = m_DeskDupl -> AcquireNextFrame (500, &FrameInfo, &DesktopResource);

    if (hr == DXGI_ERROR_WAIT_TIMEOUT) return cv::Mat();
    
    if (FAILED (hr)) throw runtime_error ("Failed to acquire next frame.");
    

    ID3D11Texture2D* AcquiredDesktopImage = nullptr;
    hr = DesktopResource -> QueryInterface (__uuidof (ID3D11Texture2D), reinterpret_cast<void**> (&AcquiredDesktopImage));
    DesktopResource -> Release ();
    if (FAILED (hr)) throw runtime_error ("Failed to get ID3D11Texture2D from desktop resource.");
    

    m_ImmediateContext -> CopyResource (m_StagingTexture, AcquiredDesktopImage);

    D3D11_MAPPED_SUBRESOURCE mapped_resource;
    UINT subresource = D3D11CalcSubresource (0, 0, 0);
    hr = m_ImmediateContext -> Map (m_StagingTexture, subresource, D3D11_MAP_READ, 0, &mapped_resource);
    
    if (FAILED (hr)) 
    {
        m_DeskDupl -> ReleaseFrame ();
        throw runtime_error ("Failed to map staging texture.");
    }

    cv::Mat frame_bgra (m_Height, m_Width, CV_8UC4, mapped_resource.pData, mapped_resource.RowPitch);
    
    cv::Mat frame;
    cv::cvtColor (frame_bgra, frame, cv::COLOR_BGRA2BGR);
    
    m_ImmediateContext -> Unmap (m_StagingTexture, subresource);
    m_DeskDupl -> ReleaseFrame ();
    
    AcquiredDesktopImage -> Release ();

    return frame;
}
