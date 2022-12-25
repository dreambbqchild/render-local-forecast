#include "ForecastRenderers.h"
#include <wrl.h>
#include <vector>

using namespace Microsoft::WRL;
using namespace std;

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
                hr = LoadWxBitmapFromFile(pWICFactory, pD2DFactory, wxFile, &pWICBitmap);
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