#pragma once

#include "FTGL/ftgl.h"
#include <string>
#include "cinder/Area.h"
#include "cinder/app/AppBasic.h"

using namespace std;
using namespace cinder;

class ofxFTGLFont
{    
    public:
        ofxFTGLFont();
        ~ofxFTGLFont();

        virtual void unload();
        virtual bool loadFont(fs::path filePath, float fontsize, float depth = 0, bool bUsePolygons = false);
        bool isLoaded();

        void setSize(int size);
        int getSize();
    
        void setTracking(float tracking);
        float getTracking() const;

        float getLineHeight() const;
        float getAscender() const;
        float getDescender() const;
        float getXHeight() const;

        virtual Area getStringBoundingBox(wstring s, float x, float y);
        virtual Area getStringBoundingBox(string s, float x, float y);
        float stringHeight(string c);
        float stringWidth(string c);
		float getSpaceSize();
        virtual void drawString(wstring s, float x, float y);
        virtual void drawString(string s, float x, float y);

        FTFont* font;
    
    protected:
        bool loaded;
        FTPoint trackingPoint;
};

