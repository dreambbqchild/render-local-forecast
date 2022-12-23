#include "ImageFunctions.h"
#include <wrl.h>

using namespace Microsoft::WRL;
using namespace std;

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
        pRenderTarget->DrawText(forecast.c_str(), (UINT32)forecast.length(), pTextFormat.Get(), D2D1::RectF(12, 0, imageWidth, imageHeight), pWhiteBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
        pRenderTarget->EndDraw();
    }

    return hr;
}