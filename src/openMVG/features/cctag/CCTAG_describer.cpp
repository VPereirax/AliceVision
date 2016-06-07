#include "CCTAG_describer.hpp"

#include <cctag/ICCTag.hpp>

namespace openMVG {
namespace features {

struct CCTAG_Image_describer::CCTagParameters : public cctag::Parameters 
{
  CCTagParameters(size_t nRings) : cctag::Parameters(nRings) {}
  
  float _cannyThrLow;
  float _cannyThrHigh;
  
  bool setPreset(EDESCRIBER_PRESET preset)
  {
    switch(preset)
    {
    // Normal lighting conditions: normal contrast
    case LOW_PRESET:
    case MEDIUM_PRESET:
    case NORMAL_PRESET:
      _cannyThrLow = 0.01f;
      _cannyThrHigh = 0.04f;
    break;
    // Low lighting conditions: very low contrast
    case HIGH_PRESET:
    case ULTRA_PRESET:
      _cannyThrLow = 0.002f;
      _cannyThrHigh = 0.01f;
    break;
    }
    return true;
  }
};


CCTAG_Image_describer::CCTAG_Image_describer()
    :Image_describer(), _params(new CCTagParameters(3)), _doAppend(false) {}
    
CCTAG_Image_describer::CCTAG_Image_describer(const std::size_t nRings, const bool doAppend)
    :Image_describer(), _params(new CCTagParameters(nRings)), _doAppend(doAppend){}   

CCTAG_Image_describer::~CCTAG_Image_describer() 
{
  delete _params;
}

void CCTAG_Image_describer::Allocate(std::unique_ptr<Regions> &regions) const
{
  regions.reset( new CCTAG_Regions );
}

bool CCTAG_Image_describer::Set_configuration_preset(EDESCRIBER_PRESET preset)
{
  return _params->setPreset(preset);
}

void CCTAG_Image_describer::Set_use_cuda(bool use_cuda)
{
  _params->_useCuda = use_cuda;
}

bool CCTAG_Image_describer::Describe(const image::Image<unsigned char>& image,
    std::unique_ptr<Regions> &regions,
    const image::Image<unsigned char> * mask)
  {
    const int w = image.Width(), h = image.Height();
    
    if ( !_doAppend )
      Allocate(regions);  
    // else regions are added to the current vector of features/descriptors

    // Build alias to cached data
    CCTAG_Regions * regionsCasted = dynamic_cast<CCTAG_Regions*>(regions.get());
    // reserve some memory for faster keypoint saving

    regionsCasted->Features().reserve(regionsCasted->Features().size() + 50);
    regionsCasted->Descriptors().reserve(regionsCasted->Descriptors().size() + 50);
    
    boost::ptr_list<cctag::ICCTag> cctags;

    const cv::Mat graySrc(cv::Size(image.Width(), image.Height()), CV_8UC1, (unsigned char *) image.data(), cv::Mat::AUTO_STEP);
    //// Invert the image
    //cv::Mat invertImg;
    //cv::bitwise_not(graySrc,invertImg);
    cctag::cctagDetection(cctags,1,graySrc, *_params, 0 );
    
    for (const auto & cctag : cctags)
    {
      if ( cctag.getStatus() > 0 )
      {
        std::cout << " New CCTag: Id" << cctag.id() << " ; Location ( " << cctag.x() << " , " << cctag.y() << " ) " << std::endl;

        // Add its associated descriptor
        Descriptor<unsigned char,128> desc;
        for(int i=0; i< desc.size(); ++i)
        {
          desc[i] = (unsigned char) 0;
        }
        desc[cctag.id()] = (unsigned char) 255;
        regionsCasted->Descriptors().push_back(desc);
        regionsCasted->Features().push_back(SIOPointFeature(cctag.x(), cctag.y()));
      }
    }

    cctags.clear();

    return true;
  };

} // namespace features
} // namespace openMVG

