/* Autogenerated with kurento-module-creator */

#include <gst/gst.h>
#include "MediaPipeline.hpp"
#include "MediaPipelineImpl.hpp"
#include <KmsShowFacesImplFactory.hpp>
#include "KmsShowFacesImpl.hpp"
#include <jsonrpc/JsonSerializer.hpp>
#include <KurentoException.hpp>

#define GST_CAT_DEFAULT kurento_kms_show_faces_impl
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);
#define GST_DEFAULT_NAME "KurentoKmsShowFacesImpl"

namespace kurento
{
namespace module
{
namespace datachannelexample
{

KmsShowFacesImpl::KmsShowFacesImpl (const boost::property_tree::ptree &config,
                                    std::shared_ptr<MediaPipeline> mediaPipeline) :
  FilterImpl (config,
              std::dynamic_pointer_cast<MediaObjectImpl> ( mediaPipeline) )
{
  g_object_set (element, "filter-factory", "imageoverlaymetadata", NULL);

  g_object_get (G_OBJECT (element), "filter", &imageOverlay, NULL);

  if (imageOverlay == NULL) {
    throw KurentoException (MEDIA_OBJECT_NOT_AVAILABLE,
                            "Media Object not available");
  }

  g_object_unref (imageOverlay);
}

MediaObjectImpl *
KmsShowFacesImplFactory::createObject (const boost::property_tree::ptree
                                       &config, std::shared_ptr<MediaPipeline> mediaPipeline) const
{
  return new KmsShowFacesImpl (config, mediaPipeline);
}

KmsShowFacesImpl::StaticConstructor KmsShowFacesImpl::staticConstructor;

KmsShowFacesImpl::StaticConstructor::StaticConstructor()
{
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, GST_DEFAULT_NAME, 0,
                           GST_DEFAULT_NAME);
}

} /* datachannelexample */
} /* module */
} /* kurento */
