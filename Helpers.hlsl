#define UINT_MAX 0xffffffff

uint NDCToScreen(float ndc, float dimension)
{
    return (ndc / 2.0f + 0.5f) * dimension;
}

uint2 NDCToScreen(float2 ndc, float2 windowDimensions)
{
    return uint2(NDCToScreen(ndc.x, windowDimensions.x),
     NDCToScreen(-ndc.y, windowDimensions.y));
}

float ScreenToNDC(uint screen, float dimension)
{
    return (screen / dimension - 0.5f) * 2.0f;
}

float2 ScreenToNDC(uint2 screen, float2 windowDimensions)
{
    return float2(ScreenToNDC(screen.x, windowDimensions.x),
     -ScreenToNDC(screen.y, windowDimensions.y));
}

bool SolveQuadratic(float a, float b, float c, out float minValue, out float maxValue)
{
    minValue = -1;
    maxValue = 1;
    
    if (a != 0.0f)
    {
        float ba = b / (2 * a);
        float ca = c / a;
        float discr = ba * ba - ca;
        if (discr < 0)
        {
            //nothing
            return !(c < 0);
        }
        float d = sqrt(discr);
        if (a > 0)
        {
            d = -d; //signal that its hyperbolic
        }
        maxValue = -ba + d;
        minValue = -ba - d;
        return true;
    }
    if (b != 0.0f)
    {
        if (b > 0)
        {
            minValue = -c / b;
        }
        else
        {
            maxValue = -c / b;
        }
        return true;
    }
    return !(c < 0);
}

uint2 GetScreenLeftTop(uint index, uint2 textureDimensions, uint2 rasterizerDimensions)
{
    uint2 screenLeftTop;
    screenLeftTop.x = (index % (textureDimensions.x / rasterizerDimensions.x + 1)) * rasterizerDimensions.x;
    screenLeftTop.y = (index / (textureDimensions.x / rasterizerDimensions.x + 1)) * rasterizerDimensions.y;
    return screenLeftTop;
}