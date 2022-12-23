#include "ComInstanceController.h"
#include "ForecastRenderers.h"

#include <wrl.h>
using namespace Microsoft::WRL;

#define INCREMENT_AND_TEST(i, len) i++; \
if(i == len) \
    break;

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