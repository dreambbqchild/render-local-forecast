#include <d2d1.h>
#include <dwrite.h>
#include <Wincodec.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <wrl.h>

#include <string>
#include <vector>
#include <iostream>
#include <fstream>

using namespace Microsoft::WRL;

const UINT imageWidth = 1060;
const UINT imageHeight = 1100;
const INT32 totalForecasts = 10;

class ComInstanceController {
private:
    HRESULT hr; 

public:
    ComInstanceController() { hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED); }
    bool Initalized() { return SUCCEEDED(hr); }
    ~ComInstanceController() { CoUninitialize(); }
};

struct VideoFps {
    UINT32 numerator;
    UINT32 denominator;
};

#define INCREMENT_AND_TEST(i, len) i++; \
if(i == len) \
    break;

std::wstring GetTextForecast(std::wstring pathToRenderingsFolder, int index)
{
    FILE* f;
    wchar_t cFileName[MAX_PATH] = { 0 };
    swprintf_s(cFileName, L"%s/%02d.txt", pathToRenderingsFolder.c_str(), index);

    _wfopen_s(&f, cFileName, L"r, ccs=UTF-8");
    if (f == NULL)
        return std::wstring();

    fseek(f, 0, SEEK_END);
    long length = ftell(f) * 2;
    fseek(f, 0, SEEK_SET);
    std::wstring buffer(length, '\0');
    fread((char*)buffer.c_str(), length, 1, f);
    fclose(f);
    return buffer;
}

HRESULT LoadBitmapFromFile(IWICImagingFactory* pWICFactory, ID2D1Factory* pD2DFactory, std::wstring path, IWICBitmap** ppWICBitmap)
{
    ComPtr<ID2D1RenderTarget> pRenderTarget;
    ComPtr<IWICFormatConverter> pConverter;
    ComPtr<IWICBitmapDecoder> pDecoder;
    ComPtr<IWICBitmapFrameDecode> pSource;
    ComPtr<ID2D1Bitmap> pD2DBitmap;

    HRESULT hr = pWICFactory->CreateDecoderFromFilename(path.c_str(), NULL, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder);
    if (SUCCEEDED(hr))
        hr = pDecoder->GetFrame(0, &pSource);

    if (SUCCEEDED(hr))
        hr = pWICFactory->CreateFormatConverter(&pConverter);

    if (SUCCEEDED(hr))
        hr = pConverter->Initialize(pSource.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeMedianCut);    
    
    if (SUCCEEDED(hr))
        hr = pWICFactory->CreateBitmap(imageWidth, imageHeight, GUID_WICPixelFormat32bppBGR, WICBitmapCacheOnLoad, ppWICBitmap);

    if (SUCCEEDED(hr))
        hr = pD2DFactory->CreateWicBitmapRenderTarget(*ppWICBitmap, D2D1::RenderTargetProperties(), &pRenderTarget);

    if (SUCCEEDED(hr))
        hr = pRenderTarget->CreateBitmapFromWicBitmap(pConverter.Get(), &pD2DBitmap);
   
    if (SUCCEEDED(hr))
    {
        pRenderTarget->BeginDraw();
        pRenderTarget->Clear(D2D1::ColorF(53 / 255.0f, 56 / 255.0f, 62 / 255.0f));
        pRenderTarget->DrawBitmap(pD2DBitmap.Get(), D2D1::RectF(0, 40, 1060, 1060), 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, D2D1::RectF(0, 0, 1060, 1060));
        pRenderTarget->EndDraw();
    }

    return hr;
}

HRESULT RenderForecastBitmap(std::wstring pathToRenderingsFolder, IWICImagingFactory* pWICFactory, ID2D1Factory* pD2DFactory, IDWriteFactory* pDWriteFactory, IWICBitmap** ppWICBitmap, int32_t forecastIndex)
{
    ComPtr<ID2D1RenderTarget> pRenderTarget;
    ComPtr<ID2D1SolidColorBrush> pWhiteBrush;
    ComPtr<IDWriteTextFormat> pTextFormat;            
    ComPtr<IWICMetadataQueryWriter> pMetadataQueryWriter;

    auto forecast = GetTextForecast(pathToRenderingsFolder, forecastIndex);
    HRESULT hr = pWICFactory->CreateBitmap(imageWidth, imageHeight, GUID_WICPixelFormat32bppBGR, WICBitmapCacheOnLoad, ppWICBitmap);

    if (SUCCEEDED(hr))
        hr = pD2DFactory->CreateWicBitmapRenderTarget(*ppWICBitmap, D2D1::RenderTargetProperties(), &pRenderTarget);

    if (SUCCEEDED(hr))
        hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pWhiteBrush);

    if (SUCCEEDED(hr))
        hr = pDWriteFactory->CreateTextFormat(L"Consolas", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 32, L"", &pTextFormat);

    if (SUCCEEDED(hr))
        pRenderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);

    if (SUCCEEDED(hr))
    {
        pRenderTarget->BeginDraw();
        pRenderTarget->Clear(D2D1::ColorF(53 / 255.0f, 56 / 255.0f, 62 / 255.0f));
        pRenderTarget->DrawText(forecast.c_str(), (UINT32)forecast.length(), pTextFormat.Get(), D2D1::RectF(40, 0, imageWidth, imageHeight), pWhiteBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
        pRenderTarget->EndDraw();
    }

    return hr;
}

HRESULT RenderGifForecast(std::wstring pathToGifOutput, std::wstring pathToRenderingsFolder, IWICImagingFactory* pWICFactory, ID2D1Factory* pD2DFactory, IDWriteFactory* pDWriteFactory)
{
    WICPixelFormatGUID pixelFormat = GUID_WICPixelFormatDontCare;
    ComPtr<IWICStream> pStream;
    ComPtr<IWICBitmapEncoder> pEncoder;
    ComPtr<IWICMetadataQueryWriter> pMetadataQueryWriter;
    PROPVARIANT propValue = { 0 };

    HRESULT hr = pWICFactory->CreateStream(&pStream);
    if (SUCCEEDED(hr))
        hr = pStream->InitializeFromFilename(pathToGifOutput.c_str(), GENERIC_WRITE);
    if (SUCCEEDED(hr))
        hr = pWICFactory->CreateEncoder(GUID_ContainerFormatGif, NULL, &pEncoder);
    if (SUCCEEDED(hr))
        hr = pEncoder->Initialize(pStream.Get(), WICBitmapEncoderNoCache);

    if (SUCCEEDED(hr))
        hr = pEncoder->GetMetadataQueryWriter(&pMetadataQueryWriter);

    propValue.vt = VT_UI1 | VT_VECTOR;
    propValue.caub.cElems = 11;
    std::string text("NETSCAPE2.0");
    std::vector<uint8_t> chars(text.begin(), text.end());
    propValue.caub.pElems = chars.data();
    if (SUCCEEDED(hr))
        hr = pMetadataQueryWriter->SetMetadataByName(L"/appext/Application", &propValue);
    PropVariantInit(&propValue);

    propValue.vt = VT_UI1 | VT_VECTOR;
    propValue.caub.cElems = 5;
    std::vector<uint8_t> data({ 3, 1, 0, 0, 0 });
    propValue.caub.pElems = data.data();
    if (SUCCEEDED(hr))
        hr = pMetadataQueryWriter->SetMetadataByName(L"/appext/Data", &propValue);    

    for (auto i = -1; i < totalForecasts; i++)
    {
        ComPtr<IWICBitmapFrameEncode> pFrameEncoder;
        ComPtr<IWICBitmap> pWICBitmap;
        if (SUCCEEDED(hr))
            hr = pEncoder->CreateNewFrame(&pFrameEncoder, NULL);
        if (SUCCEEDED(hr))
            hr = pFrameEncoder->Initialize(NULL);
        if (SUCCEEDED(hr))
            hr = pFrameEncoder->SetSize(imageWidth, imageHeight);
        if (SUCCEEDED(hr))
            hr = pFrameEncoder->SetPixelFormat(&pixelFormat);

        if (SUCCEEDED(hr))
        {
            if (i == -1)
            {
                std::wstring wxFile = pathToRenderingsFolder + L"\\wx.png";
                hr = LoadBitmapFromFile(pWICFactory, pD2DFactory, wxFile, &pWICBitmap);
            }
            else
                hr = RenderForecastBitmap(pathToRenderingsFolder, pWICFactory, pD2DFactory, pDWriteFactory, &pWICBitmap, i);
        }

        if (SUCCEEDED(hr))
            hr = pFrameEncoder->GetMetadataQueryWriter(&pMetadataQueryWriter);

        PROPVARIANT value;
        PropVariantInit(&value);
        value.vt = VT_UI2;
        value.uiVal = 500;
        if (SUCCEEDED(hr))
            hr = pMetadataQueryWriter->SetMetadataByName(L"/grctlext/Delay", &value);
        PropVariantClear(&value);

        if (SUCCEEDED(hr))
            hr = pFrameEncoder->WriteSource(pWICBitmap.Get(), NULL);
        if (SUCCEEDED(hr))
            hr = pFrameEncoder->Commit();
    }

    if (SUCCEEDED(hr))
        hr = pEncoder->Commit();

    return hr;
}

HRESULT InitializeSinkWriter(std::wstring pathToMp4Output, IMFSinkWriter** ppSinkWriter, DWORD* pStreamIndex, VideoFps fps, UINT32 frameDuration)
{    
    const UINT32 bitRate = 1000000;
    const GUID encodingFormat = MFVideoFormat_H264;
    const GUID inputFormat = MFVideoFormat_RGB32;

    ComPtr<IMFMediaType> pMediaTypeOut;
    ComPtr<IMFMediaType> pMediaTypeIn;

    HRESULT hr = MFCreateSinkWriterFromURL(pathToMp4Output.c_str(), NULL, NULL, ppSinkWriter);

    if (SUCCEEDED(hr))
        hr = MFCreateMediaType(&pMediaTypeOut);
    if (SUCCEEDED(hr))
        hr = pMediaTypeOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    if (SUCCEEDED(hr))
        hr = pMediaTypeOut->SetGUID(MF_MT_SUBTYPE, encodingFormat);
    if (SUCCEEDED(hr))
        hr = pMediaTypeOut->SetUINT32(MF_MT_AVG_BITRATE, bitRate);
    if (SUCCEEDED(hr))
        hr = pMediaTypeOut->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    if (SUCCEEDED(hr))
        hr = MFSetAttributeSize(pMediaTypeOut.Get(), MF_MT_FRAME_SIZE, imageWidth, imageHeight);
    if (SUCCEEDED(hr))
        hr = MFSetAttributeRatio(pMediaTypeOut.Get(), MF_MT_FRAME_RATE, fps.numerator, fps.denominator);
    if (SUCCEEDED(hr))
        hr = MFSetAttributeRatio(pMediaTypeOut.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    if (SUCCEEDED(hr))
        hr = (*ppSinkWriter)->AddStream(pMediaTypeOut.Get(), pStreamIndex);

    if (SUCCEEDED(hr))
        hr = MFCreateMediaType(&pMediaTypeIn);
    if (SUCCEEDED(hr))
        hr = pMediaTypeIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    if (SUCCEEDED(hr))
        hr = pMediaTypeIn->SetGUID(MF_MT_SUBTYPE, inputFormat);
    if (SUCCEEDED(hr))
        hr = pMediaTypeIn->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    if (SUCCEEDED(hr))
        hr = MFSetAttributeSize(pMediaTypeIn.Get(), MF_MT_FRAME_SIZE, imageWidth, imageHeight);
    if (SUCCEEDED(hr))
        hr = MFSetAttributeRatio(pMediaTypeIn.Get(), MF_MT_FRAME_RATE, fps.numerator, fps.denominator);
    if (SUCCEEDED(hr))
        hr = MFSetAttributeRatio(pMediaTypeIn.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    if (SUCCEEDED(hr))
        hr = (*ppSinkWriter)->SetInputMediaType(*pStreamIndex, pMediaTypeIn.Get(), NULL);

    if (SUCCEEDED(hr))
        hr = (*ppSinkWriter)->BeginWriting();

    return hr;
}

HRESULT RenderSingleVideoForecast(IWICImagingFactory* pWICFactory, ID2D1Factory* pD2DFactory, IDWriteFactory* pDWriteFactory, IMFSinkWriter* pWriter, DWORD streamIndex, IWICBitmap* pWICBitmap, INT32 frameDuration, LONGLONG& rtStart)
{
    ComPtr<IWICBitmapLock> pILock;
    ComPtr<IMFMediaBuffer> pBuffer;
    ComPtr<IWICBitmapFlipRotator> pFlipRotator;
    ComPtr<IWICBitmap> pWICFlippedBitmap;

    const LONG cbWidth = 4 * imageWidth;
    const DWORD cbBuffer = cbWidth * imageHeight;

    BYTE* pBitmapData = NULL;
    BYTE* pVideoData = NULL;
    WICRect rcLock = { 0, 0, imageWidth, imageHeight };
    UINT bitmapBufferSize = 0;

    HRESULT hr = MFCreateMemoryBuffer(cbBuffer, &pBuffer);
    if (SUCCEEDED(hr))
        hr = pWICFactory->CreateBitmapFlipRotator(&pFlipRotator);
    if (SUCCEEDED(hr))
        hr = pFlipRotator->Initialize(pWICBitmap, WICBitmapTransformFlipVertical);
    if (SUCCEEDED(hr))
        hr = pWICFactory->CreateBitmapFromSourceRect(pFlipRotator.Get(), 0, 0, imageWidth, imageHeight, &pWICFlippedBitmap);
    if (SUCCEEDED(hr))
        hr = pBuffer->Lock(&pVideoData, NULL, NULL);
    if(SUCCEEDED(hr))
        hr = pWICFlippedBitmap->Lock(&rcLock, WICBitmapLockWrite, &pILock);
    if (SUCCEEDED(hr))
        hr = pILock->GetDataPointer(&bitmapBufferSize, &pBitmapData);
    if (SUCCEEDED(hr) && pVideoData && pBitmapData)
        hr = MFCopyImage(pVideoData, cbWidth, pBitmapData, cbWidth, cbWidth, imageHeight);
    if (pBuffer)
        hr = pBuffer->Unlock();

    if (SUCCEEDED(hr))
        hr = pBuffer->SetCurrentLength(cbBuffer);
    
    ComPtr<IMFSample> pSample;
    if (SUCCEEDED(hr))
        hr = MFCreateSample(&pSample);
    if (SUCCEEDED(hr))
        hr = pSample->AddBuffer(pBuffer.Get());

    if (SUCCEEDED(hr))
        hr = pSample->SetSampleTime(rtStart);
    if (SUCCEEDED(hr))
        hr = pSample->SetSampleDuration(frameDuration);

    if (SUCCEEDED(hr))
        hr = pWriter->WriteSample(streamIndex, pSample.Get());

    if (SUCCEEDED(hr))
        rtStart += frameDuration;

    return hr;
}

HRESULT RenderVideoForecast(std::wstring pathToRenderingsFolder, std::wstring pathToMp4Output, IWICImagingFactory* pWICFactory, ID2D1Factory* pD2DFactory, IDWriteFactory* pDWriteFactory)
{
    const VideoFps fps = {1, 5};
    const UINT64 frameDuration = (UINT64)(10 * 1000 * 1000 / (fps.numerator / (float)fps.denominator));
    ComPtr<IMFSinkWriter> pSinkWriter;
    LONGLONG rtStart = 0;
    DWORD stream;
    HRESULT hr = MFStartup(MF_VERSION);

    if (SUCCEEDED(hr))
        hr = InitializeSinkWriter(pathToMp4Output, &pSinkWriter, &stream, fps, frameDuration);

    if (SUCCEEDED(hr))
    {
        ComPtr<IWICBitmap> pWICBitmap;
        std::wstring wxFile = pathToRenderingsFolder + L"\\wx.png";
        hr = LoadBitmapFromFile(pWICFactory, pD2DFactory, wxFile, &pWICBitmap);

        if (SUCCEEDED(hr))
            hr = RenderSingleVideoForecast(pWICFactory, pD2DFactory, pDWriteFactory, pSinkWriter.Get(), stream, pWICBitmap.Get(), frameDuration, rtStart);
    }

    if (SUCCEEDED(hr))
    {
        for (auto forecastIndex = 0; forecastIndex < totalForecasts; forecastIndex++)
        {
            ComPtr<IWICBitmap> pWICBitmap;
            hr = RenderForecastBitmap(pathToRenderingsFolder, pWICFactory, pD2DFactory, pDWriteFactory, &pWICBitmap, forecastIndex);
            if (FAILED(hr))
                break;

            hr = RenderSingleVideoForecast(pWICFactory, pD2DFactory, pDWriteFactory, pSinkWriter.Get(), stream, pWICBitmap.Get(), frameDuration, rtStart);
            if (FAILED(hr))            
                break;
        }
    }

    if (SUCCEEDED(hr))
        hr = pSinkWriter->Finalize();

    if (SUCCEEDED(hr))
        hr = MFShutdown();

    return hr;
}

int wmain(int argc, const wchar_t* argv[])
{
    ComInstanceController controller;
    if (!controller.Initalized())
        return 1;

    std::wstring pathToRenderingsFolder = L".\\renderings";
    std::wstring pathToMp4Output = L"output.mp4";
    std::wstring pathToGifOutput = L"output.gif";

    for (auto i = 0; i < argc; i++) 
    {
        if (!wcscmp(L"-r", argv[i])) 
        {
            INCREMENT_AND_TEST(i, argc);
            pathToRenderingsFolder = argv[i];
        }
        else if (!wcscmp(L"-m", argv[i]))
        {
            INCREMENT_AND_TEST(i, argc);
            pathToMp4Output = argv[i];
        }
        else if (!wcscmp(L"-g", argv[i]))
        {
            INCREMENT_AND_TEST(i, argc);
            pathToGifOutput = argv[i];
        }
    }

    ComPtr<ID2D1Factory> pD2DFactory;
    ComPtr<IDWriteFactory> pDWriteFactory;    
    ComPtr<IWICImagingFactory> pWICFactory;    

    auto hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, pD2DFactory.GetAddressOf());
    if (SUCCEEDED(hr))
        hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(pDWriteFactory), reinterpret_cast<IUnknown**>(pDWriteFactory.GetAddressOf()));
    if (SUCCEEDED(hr))
        hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pWICFactory));

    if (SUCCEEDED(hr))
        hr = RenderVideoForecast(pathToRenderingsFolder, pathToMp4Output, pWICFactory.Get(), pD2DFactory.Get(), pDWriteFactory.Get());

    /*if (SUCCEEDED(hr))
        hr = RenderGifForecast(pathToGifOutput, pathToRenderingsFolder, pWICFactory.Get(), pD2DFactory.Get(), pDWriteFactory.Get());*/

	return 0;
}