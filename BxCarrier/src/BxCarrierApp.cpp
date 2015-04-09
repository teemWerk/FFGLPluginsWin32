/*
* This sample illustrates how to get audio data from an input device, such as a microphone,
* with audio::InputDeviceNode. It then visualizes the input in the frequency domain. The frequency
* spectrum analysis is accomplished with an audio::MonitorSpectralNode.
*
* The plot is similar to a typical spectrogram, where the x-axis represents the linear
* frequency bins (0 - samplerate / 2) and the y-axis is the magnitude of the frequency
* bin in normalized decibels (0 - 100).
*
* author: Richard Eakin (2014)
*/

#include "boost\interprocess\managed_shared_memory.hpp"
#include "boost\interprocess\allocators\allocator.hpp"
#include "boost\interprocess\containers\string.hpp"
#include <boost/interprocess/containers/vector.hpp>


#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/TextureFont.h"
#include "cinder/params/Params.h"
#include "cinder/Utilities.h"

#include "cinder/audio/Context.h"
#include "cinder/audio/MonitorNode.h"
#include "cinder/audio/Utilities.h"
#include "AveragingBuffer.h"

using namespace ci;
using namespace ci::app;
//using namespace std;
using namespace boost::interprocess;

typedef boost::interprocess::allocator<char, managed_shared_memory::segment_manager>
CharAllocator;
typedef boost::interprocess::basic_string<char, std::char_traits<char>, CharAllocator>
MyShmString;
typedef boost::interprocess::allocator<MyShmString, managed_shared_memory::segment_manager>
StringAllocator;
typedef vector<MyShmString, StringAllocator>
SharedVector;

class InputAnalyzer : public App {
public:
	void setup ();
	void shutdown ();
	void mouseDown ( MouseEvent event );
	void update ();
	void draw () override;
	//void prepareSettings ( Settings * settings );
	void drawLabels ();
	void printBinInfo ( int mouseX );

	params::InterfaceGlRef		mParams;
	vector<audio::InputDeviceNodeRef>		mInputDeviceNodes;
	vector<audio::MonitorNodeRef>	mMonitorNodes;
	vector<AveragingNodeRef>	mAveragingNodes;
	vector<audio::Buffer>					mMagSpectrum;

	//SpectrumPlot					mSpectrumPlot;
	gl::TextureFontRef				mTextureFont;
	managed_shared_memory segment;
	//StringAllocator string_alloc;
	//CharAllocator char_alloc;
	SharedVector * mSharedVector;
	vector<float *>mInstances;
	audio::OutputDeviceNodeRef output;
	float smoothing;
	float gain;
};

//typedef std::pair<double, int> MyType;
void InputAnalyzer::setup ()
{
	gl::enableVerticalSync ( false );
	shared_memory_object::remove ( "CarrierMemorySegment" );
	segment = managed_shared_memory ( create_only,
		"CarrierMemorySegment",  //segment name
		65536 );

	auto char_alloc = CharAllocator ( segment.get_segment_manager () );
	auto string_alloc = StringAllocator ( segment.get_segment_manager () );

	console () << "create shared memory" << std::endl;

	mSharedVector = segment.construct<SharedVector> ( "AvailableInputs" )( string_alloc );

	auto ctx = audio::Context::master ();

	gain = 1.0;

	mParams = params::InterfaceGl::create ( "Settings", toPixels ( ivec2 ( 300, 100 ) ) );
	mParams->addParam ( "Smoothing", &smoothing ).step ( 0.01 ).max ( 1.0 ).min ( 0.0 );
	mParams->addParam ( "Gain", &gain ).step ( 0.01 ).max ( 25.0 ).min ( 0.0 );
	// The InputDeviceNode is platform-specific, so you create it using a special method on the Context:
	auto monitorFormat = audio::MonitorNode::Format ();// .windowSize ( 1024 );

	//auto outputFormat = audio::Device::Format ().framesPerBlock ( 1024 );
	auto outputDevice = audio::Device::getDefaultOutput ();
	//outputDevice->updateFormat ( outputFormat );

	auto inputDevices = audio::Device::getInputDevices ();
	output = ctx->createOutputDeviceNode ( outputDevice );
	

	//auto nodeFormat = audio::Node::Format ()
	for ( auto dev : inputDevices )
	{
		mInputDeviceNodes.emplace_back ( ctx->createInputDeviceNode ( dev ) );
		console () << dev->getName () << std::endl;
		mMonitorNodes.emplace_back ( ctx->makeNode ( new audio::MonitorNode ( monitorFormat ) ) );
		
		MyShmString mString ( char_alloc );
		mString = dev->getName ().c_str ();
		mSharedVector->emplace_back ( mString );
		mInstances.emplace_back ( segment.construct<float>
			( dev->getName ().c_str () )  //name of the object
			[1024] ( 0.0f ) );
		mAveragingNodes.emplace_back ( ctx->makeNode ( new AveragingNode ( mInstances.back () ) ) );
	}

	for ( int i = 0; i < mInputDeviceNodes.size (); i++ )
	{
		mInputDeviceNodes[i] >> mAveragingNodes[i]>> output;//mMonitorNodes[i];
		mAveragingNodes[i] >> mMonitorNodes[i];
	}

	for ( auto node : mInputDeviceNodes )
	{
		node->enable ();
	}
	ctx->enable ();
	console() << ctx->getFramesPerBlock () << std::endl;
	getWindow ()->setTitle ( "Carrier v.0.0.1" );
	//DirectSoundCaptureEnumerate ( ( LPDSENUMCALLBACK ) DSEnumProc, NULL );
}

void InputAnalyzer::shutdown ()
{
	shared_memory_object::remove ( "CarrierMemorySegment" );
}

void InputAnalyzer::mouseDown ( MouseEvent event )
{
	//if( mSpectrumPlot.getBounds().contains( event.getPos() ) )
	//	printBinInfo( event.getX() );
}

void InputAnalyzer::update ()
{
	std::string optionsStr = "max=" + std::to_string ( 1.0 / smoothing );
	mParams->setOptions ( "Gain", optionsStr );
	for ( int i = 0; i < mAveragingNodes.size (); i++ )
	{
		mAveragingNodes[i]->setDepth ( smoothing );
		mAveragingNodes[i]->setRate ( gain );
	}
	
	//for ( int i = 0; i < mMonitorNodes.size (); i++ )
	//{
	//	auto buff = mMonitorNodes[i]->getBuffer ();
	//	for ( int j = 0; j < 1024; j++ )
	//	{
	//		mInstances[i][j] = ( mInstances[i][j] * smoothing + buff[j] * (1.0-smoothing) )*gain;
	//	}
	//}
	////console () << mMagSpectrum[100] << ", " << mMagSpectrum[500] << ", " << mMagSpectrum[700] << endl;
}

void InputAnalyzer::draw ()
{
	gl::clear ( Color ( 0.07f, 0.05f, 0.1f ) );
	mParams->draw ();
}

void InputAnalyzer::drawLabels ()
{
	if ( !mTextureFont )
		mTextureFont = gl::TextureFont::create ( Font ( Font::getDefault ().getName (), 16 ) );

	gl::color ( 0, 0.9f, 0.9f );

	// draw x-axis label
	std::string freqLabel = "Frequency (hertz)";
	//mTextureFont->drawString( freqLabel, Vec2f( getWindowCenter().x - mTextureFont->measureString( freqLabel ).x / 2, (float)getWindowHeight() - 20 ) );

	// draw y-axis label
	std::string dbLabel = "Magnitude (decibels, linear)";
	gl::pushModelView ();
	gl::translate ( 30, getWindowCenter ().y + mTextureFont->measureString ( dbLabel ).x / 2 );
	gl::rotate ( -90 );
	//mTextureFont->drawString( dbLabel, Vec2f::zero() );
	gl::popModelView ();
}

void InputAnalyzer::printBinInfo ( int mouseX )
{
}

CINDER_APP ( InputAnalyzer, RendererGl, [] ( AppMsw::Settings * settings ){settings->setWindowSize ( 350, 150 );
settings->setFrameRate ( 60 );
settings->setConsoleWindowEnabled(); } )