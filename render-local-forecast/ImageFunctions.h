#pragma once
#include "FileSystem.h"
#include "WxData.h"
#include <dwrite.h>

HRESULT RenderForecastInit(IWICImagingFactory* pWICFactory, ID2D1Factory* pD2DFactory, IDWriteFactory* pDWriteFactory);
HRESULT RenderForecastBitmap(std::wstring pathToRenderingsFolder, IWICImagingFactory* pWICFactory, ID2D1Factory* pD2DFactory, IDWriteFactory* pDWriteFactory, IWICBitmap** ppWICBitmap, int32_t forecastIndex);
HRESULT RenderForecastMapBitmap(std::wstring title, std::wstring overlayTemplatePath, int32_t overlayIndex, std::vector<LocationData>& locationData, int32_t locationDataIndex, IWICImagingFactory* pWICFactory, ID2D1Factory* pD2DFactory, IDWriteFactory* pDWriteFactory, IWICBitmap** ppWICBitmap);
void RenderForecastFree();