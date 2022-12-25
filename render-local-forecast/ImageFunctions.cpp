#include "ImageFunctions.h"
#include <wrl.h>

using namespace Microsoft::WRL;
using namespace std;

ComPtr<IDWriteTextFormat> pConsolasFormat;
ComPtr<IDWriteTextFormat> pArialFormat;
ComPtr<ID2D1Bitmap> pforecastMap;

HRESULT RenderForecastInit(IWICImagingFactory* pWICFactory, ID2D1Factory* pD2DFactory, IDWriteFactory* pDWriteFactory)
{
    HRESULT hr = pDWriteFactory->CreateTextFormat(L"Consolas", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 32, L"", &pConsolasFormat);    

    if (SUCCEEDED(hr))
        hr = pDWriteFactory->CreateTextFormat(L"Arial", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 32, L"", &pArialFormat);

    if (SUCCEEDED(hr))
        hr = LoadD2DBitmapFromFile(pWICFactory, pD2DFactory, L"forecastmap.png", &pforecastMap);

    return hr;
}

HRESULT RenderForecastBitmap(wstring pathToRenderingsFolder, IWICImagingFactory* pWICFactory, ID2D1Factory* pD2DFactory, IDWriteFactory* pDWriteFactory, IWICBitmap** ppWICBitmap, int32_t forecastIndex)
{
    ComPtr<ID2D1RenderTarget> pRenderTarget;
    ComPtr<ID2D1SolidColorBrush> pWhiteBrush;    
    ComPtr<IWICMetadataQueryWriter> pMetadataQueryWriter;

    auto forecast = GetTextForecast(pathToRenderingsFolder, forecastIndex);
    HRESULT hr = pWICFactory->CreateBitmap(imageWidth, imageHeight, GUID_WICPixelFormat32bppBGR, WICBitmapCacheOnLoad, ppWICBitmap);

    if (SUCCEEDED(hr))
        hr = pD2DFactory->CreateWicBitmapRenderTarget(*ppWICBitmap, D2D1::RenderTargetProperties(), &pRenderTarget);

    if (SUCCEEDED(hr))
        hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pWhiteBrush);

    if (SUCCEEDED(hr))
        pRenderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);

    if (SUCCEEDED(hr))
    {
        pRenderTarget->BeginDraw();
        pRenderTarget->Clear(D2D1::ColorF(53 / 255.0f, 56 / 255.0f, 62 / 255.0f));
        pRenderTarget->DrawText(forecast.c_str(), (UINT32)forecast.length(), pConsolasFormat.Get(), D2D1::RectF(12, 0, 12 + imageWidth, imageHeight), pWhiteBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
        pRenderTarget->EndDraw();
    }

    return hr;
}

HRESULT DrawTextOnBackgroundAtOrigin(IDWriteFactory* pDWriteFactory, ID2D1RenderTarget* pRenderTarget, int32_t ix, int32_t iy, wstring text, ID2D1SolidColorBrush* pTextBrush, ID2D1SolidColorBrush* pFillBrush)
{
    ComPtr<IDWriteTextLayout> pLayout;
    DWRITE_TEXT_METRICS metrics = { 0 };

    HRESULT hr = pDWriteFactory->CreateTextLayout(text.c_str(), (UINT32)text.length(), pArialFormat.Get(), imageWidth, imageHeight, &pLayout);
    if (FAILED(hr))
        return hr;

    pLayout->GetMetrics(&metrics);

    float x = ix - metrics.width * 0.5f, y = iy - metrics.height * 0.5f;
    pRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(x - 8, y + 4, x + metrics.width + 8, y + metrics.height), 4, 4), pFillBrush);
    pRenderTarget->DrawText(text.c_str(), (UINT32)text.length(), pArialFormat.Get(), D2D1::RectF(x, y, x + metrics.width, y + metrics.height), pTextBrush, D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);

    return hr;
}

HRESULT RenderForecastMapBitmap(wstring title, wstring overlayTemplatePath, int32_t overlayIndex, vector<LocationData>& locationData, int32_t locationDataIndex, IWICImagingFactory* pWICFactory, ID2D1Factory* pD2DFactory, IDWriteFactory* pDWriteFactory, IWICBitmap** ppWICBitmap)
{
    ComPtr<ID2D1RenderTarget> pRenderTarget;
    ComPtr<ID2D1Bitmap> pOverlay;
    ComPtr<ID2D1SolidColorBrush> pBlackBrush;
    ComPtr<ID2D1SolidColorBrush> pWhiteBrush;
    
    wstring path(overlayTemplatePath.length() + 32, '\0');
    swprintf((wchar_t*)path.c_str(), path.length(), overlayTemplatePath.c_str(), overlayIndex);
    
    HRESULT hr = pWICFactory->CreateBitmap(imageWidth, imageHeight, GUID_WICPixelFormat32bppBGR, WICBitmapCacheOnLoad, ppWICBitmap);
    
    if (SUCCEEDED(hr))
        hr = pD2DFactory->CreateWicBitmapRenderTarget(*ppWICBitmap, D2D1::RenderTargetProperties(), &pRenderTarget);
    
    if (SUCCEEDED(hr))
        hr = LoadD2DBitmapFromFile(pWICFactory, pD2DFactory, path.c_str(), &pOverlay);

    if (SUCCEEDED(hr))
        hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pBlackBrush);

    if (SUCCEEDED(hr))
        hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 0.5f), &pWhiteBrush);

    if (SUCCEEDED(hr))
        pRenderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);        

    if (SUCCEEDED(hr))
    {
        pRenderTarget->BeginDraw();
        pRenderTarget->DrawBitmap(pforecastMap.Get(), D2D1::RectF(0, 0, imageWidth, imageHeight), 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, D2D1::RectF(32, 32, 32 + imageWidth, 32 + imageHeight));
        pRenderTarget->DrawBitmap(pOverlay.Get(), D2D1::RectF(0, 0, imageWidth, imageHeight), 0.75f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, D2D1::RectF(32, 32, 32 + imageWidth, 32 + imageHeight));
        for (auto& locDatum : locationData)
        {
            auto forecast = locDatum.forecast.at(locationDataIndex);
            hr = DrawTextOnBackgroundAtOrigin(pDWriteFactory, pRenderTarget.Get(), locDatum.x, locDatum.y, forecast, pBlackBrush.Get(), pWhiteBrush.Get());
            if (FAILED(hr))
                return hr;
        }

        DrawTextOnBackgroundAtOrigin(pDWriteFactory, pRenderTarget.Get(), imageWidth / 2, 16, title, pBlackBrush.Get(), pWhiteBrush.Get());
        pRenderTarget->EndDraw();
    }

    return hr;
}

void RenderForecastFree()
{
    pConsolasFormat.Reset();
    pforecastMap.Reset();
}