#pragma once

#include "FTGL/ftgl.h"
#include "ofxFTGLFont.h"

typedef	FTGL::TextAlignment ofxFTGLTextAlignment;

#define FTGL_ALIGN_LEFT     FTGL::ALIGN_LEFT
#define FTGL_ALIGN_CENTER   FTGL::ALIGN_CENTER
#define FTGL_ALIGN_RIGHT    FTGL::ALIGN_RIGHT
#define FTGL_ALIGN_JUSTIFY  FTGL::ALIGN_JUSTIFY

class ofxFTGLSimpleLayout
: public ofxFTGLFont
{
    public:
        ofxFTGLSimpleLayout();
        ~ofxFTGLSimpleLayout();

        void unload();
        bool loadFont(fs::path filepath, float fontsize = 10, float depth = 0, bool bUsePolygons = false);

        Area getStringBoundingBox(wstring s, float x, float y);
        Area getStringBoundingBox(string s, float x, float y);
    
        void drawString(wstring s, float x, float y);
        void drawString(string s, float x, float y, float sizeMult);

        float       getLineLength() const;
        void        setLineLength(float length);
        float       getLineSpacing() const;
        void		setLineSpacing(float spacing);

        ofxFTGLTextAlignment getAlignment() const;
        void setAlignment(ofxFTGLTextAlignment alignment);

        FTSimpleLayout* layout;
};

