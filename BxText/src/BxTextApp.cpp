#include "cinder/app/AppBasic.h"
#include "cinder/app/AppImplMswRendererGl.h"
#include "cinder/Perlin.h"
#include "cinder/ImageIo.h"
#include "Resources.h"
#include "cinder/Utilities.h"
#include "cinder/Camera.h"
#include "Cinder/gl/Fbo.h"
#include "cinder/gl/Texture.h"
#include "cinder/Font.h"
#include "cinder/gl/TextureFont.h"

#include "FFGLPluginSDK.h"
#include "FFGLLib.h"

#include "atlstr.h"

#include "ofxFTGL.h"

#define FFPARAM_FONT	(0)
#define FFPARAM_TEXT	(1)
#define FFPARAM_SIZE  (2)
#define FFPARAM_HUE	(3)
#define FFPARAM_SAT (4)
#define FFPARAM_BRT (5)
#define FFPARAM_OPACITY (6)
#define FFPARAM_X	(7)
#define FFPARAM_Y	(8)

using namespace ci;
using namespace ci::app;

class ffglTest : public AppBasic, public CFreeFrameGLPlugin {
public:
	ffglTest ();
	//ffgl functions
	DWORD	SetParameter ( const SetParameterStruct* pParam );
	DWORD	GetParameter ( DWORD dwIndex );
	DWORD	ProcessOpenGL ( ProcessOpenGLStruct* pGL );
	DWORD InitGL ( const FFGLViewportStruct *vp );
	DWORD DeInitGL ();

	static DWORD __stdcall CreateInstance ( CFreeFrameGLPlugin **ppOutInstance )
	{
		*ppOutInstance = new ffglTest ();
		if ( *ppOutInstance != NULL )
			return FF_SUCCESS;
		return FF_FAIL;
	}
	//cinder stuff
	CameraPersp mCam;
	gl::Fbo	*	mFbo;
	Area viewport;

	string previousFont;
	string newFont;
	Area boundingBox;

	int margin;
	string alignment;

protected:
	ofxFTGLSimpleLayout * layoutText, *tempText;
	bool reload;
	bool initialized = false;
	bool cached = false;
	int counter = 0;

	// Parameters
	float m_size;
	float m_x;
	char * font;
	char * text;
	float m_y;
	float m_hue;
	float m_sat;
	float m_brt;
	float m_opacity;
	Color mTextColor;
	string str;
	string previousString;

	int m_initResources;

	GLint m_inputTextureLocation;
	GLint m_heatTextureLocation;
	GLint m_maxCoordsLocation;
	GLint m_heatAmountLocation;
};

static CFFGLPluginInfo PluginInfo (
	ffglTest::CreateInstance,	// Create method
	"BXTX",								// Plugin unique ID											
	"BxText",					// Plugin name											
	1,						   			// API major version number 													
	000,								  // API minor version number	
	1,										// Plugin major version number
	000,									// Plugin minor version number
	FF_EFFECT,						// Plugin type
	"Creates multiline text, pulls otf and ttf fonts from \"C:\\Windows\\Fonts\\<Font> (No file extension in font)",	// Plugin description
	"by Tim Cantwell" // About
	);

ffglTest::ffglTest ()
	:CFreeFrameGLPlugin (),
	m_initResources ( 1 ),
	m_maxCoordsLocation ( -1 )
{
	text = "Your String Here";
	font = "times";
	previousFont = "times";
	previousString;
	boundingBox = Area ();
	m_x = 0.5;
	m_y = 0.5;
	m_size = 0.5;
	m_hue = 0.5;
	m_sat = 1.0;
	m_brt = 1.0;
	m_opacity = 1.0;

	SetParamInfo ( FFPARAM_FONT, "Font", FF_TYPE_TEXT, font );
	SetParamInfo ( FFPARAM_TEXT, "Text", FF_TYPE_TEXT, text );
	SetParamInfo ( FFPARAM_SIZE, "Size", FF_TYPE_STANDARD, 0.5f );
	SetParamInfo ( FFPARAM_HUE, "Hue", FF_TYPE_STANDARD, 0.5f );
	SetParamInfo ( FFPARAM_SAT, "Saturation", FF_TYPE_STANDARD, 1.0f );
	SetParamInfo ( FFPARAM_BRT, "Brightness", FF_TYPE_STANDARD, 1.0f );
	SetParamInfo ( FFPARAM_OPACITY, "Alpha", FF_TYPE_STANDARD, 1.0f );
	SetParamInfo ( FFPARAM_X, "X", FF_TYPE_STANDARD, 0.5f );
	SetParamInfo ( FFPARAM_Y, "Y", FF_TYPE_STANDARD, 0.5f );

}


DWORD ffglTest::InitGL ( const FFGLViewportStruct *vp )
{
	viewport = Area ( vp->x, vp->y - vp->height, vp->x + vp->width, vp->y );


	mCam.setPerspective ( 60, ( float ) vp->width / ( float ) vp->height, 1, 1000 );
	mCam.lookAt ( Vec3f ( vp->width / 2.0, vp->height / 2.0, vp->height / tanf ( M_PI / 6.0f ) / 2.0f ), Vec3f ( vp->width / 2.0, vp->height / 2.0, 0.0 ), Vec3f ( 0, -1.0, 0.0 ) );

	gl::Fbo::Format format;
	format.setSamples ( 8 );

	mFbo = new gl::Fbo ( vp->width, vp->height, format );

	layoutText = new ofxFTGLSimpleLayout ();

	margin = 20;
	alignment = "LEFT";

	layoutText->loadFont ( fs::path ( "C:\\Windows\\Fonts\\times.ttf" ), 64 );
	layoutText->setLineLength ( vp->width * 2 );
	layoutText->setAlignment ( FTGL_ALIGN_CENTER );

	initialized = true;

	//cinder::CameraPersp cam ( width, height, 60.0f );

	glMatrixMode ( GL_PROJECTION );
	glLoadMatrixf ( mCam.getProjectionMatrix ().m );

	glMatrixMode ( GL_MODELVIEW );
	glLoadMatrixf ( mCam.getModelViewMatrix ().m );
	glScalef ( 1.0f, -1.0f, 1.0f );           // invert Y axis so increasing Y goes down.
	glTranslatef ( 0.0f, ( float ) -vp->height, 0.0f );

	mTextColor = Color ( cinder::CM_HSV, Vec3f ( m_hue, m_sat, m_brt ) );
	return FF_SUCCESS;
}

DWORD ffglTest::DeInitGL ()
{
	delete mFbo;
	layoutText->unload ();
	delete layoutText;

	return FF_SUCCESS;
}

DWORD ffglTest::ProcessOpenGL ( ProcessOpenGLStruct *pGL )
{
	gl::enableAlphaBlending ();

	if ( pGL->numInputTextures<1 ) return FF_FAIL;

	if ( pGL->inputTextures[0] == NULL ) return FF_FAIL;

	int width = mFbo->getWidth ();
	int height = mFbo->getHeight ();
	gl::pushMatrices ();
	glOrtho ( 0, width, height, 0, -1.0f, 1.0f );
	mFbo->bindFramebuffer ();

	//mTextColor = Color ( CM_HSV, m_hue, m_sat, m_brt );
	gl::clear ( cinder::ColorAf ( 0.0, 0.0, 0.0, 0.0 ) );
	gl::color ( ColorA ( CM_HSV, m_hue, m_sat, m_brt, m_opacity ) );

	layoutText->drawString ( str, width * m_x - width * m_size * 5, height - height * m_y - boundingBox.getHeight () * m_size * 2.5, m_size * 5 );


	mFbo->unbindFramebuffer ();



	glBindFramebufferEXT ( GL_FRAMEBUFFER_EXT, pGL->HostFBO );

	gl::color ( 1.0, 1.0, 1.0, 1.0 );

	gl::enable ( GL_TEXTURE_2D );

	FFGLTextureStruct &Texture = *( pGL->inputTextures[0] );

	FFGLTexCoords maxCoords = GetMaxGLTexCoords ( Texture );

	glBindTexture ( GL_TEXTURE_2D, Texture.Handle );

	glEnable ( GL_TEXTURE_2D );

	glEnableClientState ( GL_VERTEX_ARRAY );
	GLfloat verts[8];
	glVertexPointer ( 2, GL_FLOAT, 0, verts );
	glEnableClientState ( GL_TEXTURE_COORD_ARRAY );
	GLfloat texCoords[8];
	glTexCoordPointer ( 2, GL_FLOAT, 0, texCoords );

	verts[0 * 2 + 0] = width; verts[0 * 2 + 1] = 0;
	verts[1 * 2 + 0] = 0; verts[1 * 2 + 1] = 0;
	verts[2 * 2 + 0] = 0; verts[2 * 2 + 1] = height;
	verts[3 * 2 + 0] = width; verts[3 * 2 + 1] = height;

	texCoords[0 * 2 + 0] = maxCoords.s; texCoords[0 * 2 + 1] = maxCoords.t;
	texCoords[1 * 2 + 0] = 0; texCoords[1 * 2 + 1] = maxCoords.t;
	texCoords[2 * 2 + 0] = 0; texCoords[2 * 2 + 1] = 0;
	texCoords[3 * 2 + 0] = maxCoords.s; texCoords[3 * 2 + 1] = 0;

	glDrawArrays ( GL_TRIANGLE_FAN, 0, 4 );


	glBindTexture ( GL_TEXTURE_2D, 0 );

	glDisableClientState ( GL_TEXTURE_COORD_ARRAY );
	glDisableClientState ( GL_VERTEX_ARRAY );
	glDisable ( GL_TEXTURE_2D );

	gl::draw ( mFbo->getTexture (), Rectf ( 0, 0, width, height ) );
	gl::popMatrices ();

	gl::disableAlphaBlending ();

	return FF_SUCCESS;
}

DWORD ffglTest::GetParameter ( DWORD dwIndex )
{
	DWORD dwRet;

	switch ( dwIndex ) {

	case FFPARAM_SIZE:
		*( ( float * ) ( unsigned ) &dwRet ) = m_size;
		return dwRet;
		break;
	case FFPARAM_X:
		*( ( float * ) ( unsigned ) &dwRet ) = m_x;
		return dwRet;
		break;
	case FFPARAM_Y:
		*( ( float * ) ( unsigned ) &dwRet ) = m_y;
		return dwRet;
		break;
	case FFPARAM_TEXT:
		*( ( char ** ) ( unsigned ) &dwRet ) = text;
		return dwRet;
		break;
	case FFPARAM_HUE:
		*( ( float * ) ( unsigned ) &dwRet ) = m_hue;
		return dwRet;
		break;
	case FFPARAM_SAT:
		*( ( float * ) ( unsigned ) &dwRet ) = m_sat;
		return dwRet;
		break;
	case FFPARAM_BRT:
		*( ( float * ) ( unsigned ) &dwRet ) = m_brt;
		return dwRet;
		break;
	case FFPARAM_OPACITY:
		*( ( float * ) ( unsigned ) &dwRet ) = m_opacity;
		return dwRet;
		break;
	case FFPARAM_FONT:
		*( ( char ** ) ( unsigned ) &dwRet ) = font;
		return dwRet;
		break;
	default:
		return FF_FAIL;
	}
}

DWORD ffglTest::SetParameter ( const SetParameterStruct* pParam )
{
	if ( pParam != NULL ) {

		switch ( pParam->ParameterNumber ) {
		case FFPARAM_FONT:
			font = *( ( char ** ) ( unsigned ) &( pParam->NewParameterValue ) );
			newFont = string ( font );
			if ( initialized &  newFont != previousFont )
			{
				previousFont = newFont;
				fs::path fontPath = fs::path ( "C:\\Windows\\Fonts\\" + newFont + ".ttf" );
				if ( boost::filesystem::exists ( fontPath ) )
				{
					layoutText->loadFont ( fontPath, 64 );
					layoutText->setLineLength ( mFbo->getWidth () * 2 );
					layoutText->setAlignment ( FTGL_ALIGN_CENTER );
				}
			}
			break;
		case FFPARAM_TEXT:
			text = *( ( char ** ) ( unsigned ) &( pParam->NewParameterValue ) );
			str = string ( text );
			if ( initialized & str != previousString )
			{
				previousString = str;
				boundingBox = layoutText->getStringBoundingBox ( str, 0, 0 );
			}
			break;
		case FFPARAM_HUE:
			m_hue = *( ( float * ) ( unsigned ) &( pParam->NewParameterValue ) );
			break;
		case FFPARAM_SAT:
			m_sat = *( ( float * ) ( unsigned ) &( pParam->NewParameterValue ) );
			break;
		case FFPARAM_BRT:
			m_brt = *( ( float * ) ( unsigned ) &( pParam->NewParameterValue ) );
			break;
		case FFPARAM_OPACITY:
			m_opacity = *( ( float * ) ( unsigned ) &( pParam->NewParameterValue ) );
			break;
		case FFPARAM_SIZE:
			m_size = *( ( float * ) ( unsigned ) &( pParam->NewParameterValue ) );
			break;
		case FFPARAM_X:
			m_x = *( ( float * ) ( unsigned ) &( pParam->NewParameterValue ) );
			break;
		case FFPARAM_Y:
			m_y = *( ( float * ) ( unsigned ) &( pParam->NewParameterValue ) );
			break;

		default:
			return FF_FAIL;
		}

		return FF_SUCCESS;

	}

	return FF_FAIL;
}

