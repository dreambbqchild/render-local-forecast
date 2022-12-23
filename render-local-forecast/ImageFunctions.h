#pragma once
#include "FileSystem.h"
#include <dwrite.h>

HRESULT RenderForecastBitmap(std::wstring pathToRenderingsFolder, IWICImagingFactory* pWICFactory, ID2D1Factory* pD2DFactory, IDWriteFactory* pDWriteFactory, IWICBitmap** ppWICBitmap, int32_t forecastIndex);